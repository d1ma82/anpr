#include <random>
#include <opencv2/gapi.hpp>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/imgproc.hpp>
#include <opencv2/gapi/ocl/goclkernel.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/ml.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>

//#include "assets.h"
#include "anpr.h"
#include "log.h"



typedef std::vector<std::vector<cv::Point>> Contours;
typedef cv::GArray<cv::GArray<cv::Point>> GContours;

// number plate size for training stuff
static const int SAMPLE_WIDTH=144;
static const int SAMPLE_HEIGHT=33;
// Approx char size from 2-3 meters
static const int CHAR_SIZE=20;

static const char strchars[]
    {'0','1','2','3','4','5','6','7','8','9','A', 'B', 'E', 'K', 'M', 'H',
     'O', 'P', 'C', 'T', 'Y', 'X'};

static std::string to_str(int i) {
    
    std::stringstream ss;
    ss<<i;
    return ss.str();
}

//static cv::Ptr<cv::ml::SVM> classifier {nullptr};
//static cv::dnn::Net net;
/*
static bool verify_char_sizes(cv::UMat input) {

    const float aspect        =   45.0f/77.0f;
    float char_aspect         =   (float)input.cols/(float)input.rows;
    const float error         =   0.35;
    const float min_height    =   CHAR_SIZE - CHAR_SIZE * error;
    const float max_height    =   CHAR_SIZE + CHAR_SIZE * error;
    //We have a different aspect ratio for number 1, and it can be ~0.2
    const float min_aspect    =   0.2;
    const float max_aspect    =   aspect+aspect*error;

    float pixel_area    =   cv::countNonZero(input);
    float area          =   input.cols*input.rows;
    //% of pixel in area
    float perc_pixels   =   pixel_area/area;

    bool pass = perc_pixels < 0.8 && 
                char_aspect > min_aspect && 
                char_aspect < max_aspect && 
                input.rows >= min_height && 
                input.rows < max_height;  
    return pass;    
}

cv::UMat preproccess_char(cv::UMat input) {

    int h                    =   input.rows;
    int w                    =   input.cols;
    int m                    =   std::max(w,h);
    cv::Mat transform        =   cv::Mat::eye(2,3,CV_32F);
    transform.at<float>(0,2) =   m/2-w/2;
    transform.at<float>(1,2) =   m/2-h/2;
    cv::UMat wrap(m, m, input.type());
    cv::warpAffine(input, wrap, transform.getUMat(cv::ACCESS_READ), wrap.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));

    cv::UMat result;
    cv::resize(wrap, result, cv::Size(CHAR_SIZE, CHAR_SIZE));
    return result;
}

static void segment_chars(Plate& plate) {

    cv::UMat thresh;
    cv::threshold(plate.image, thresh, 60, 255, cv::THRESH_BINARY_INV);

    cv::UMat img_contours;
    thresh.copyTo(img_contours);
    Contours contours;
    cv::findContours(img_contours, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);  

    Contours::iterator itc = contours.begin();
    while (itc!=contours.end()) {   
         
        cv::Rect rect = cv::boundingRect(*itc);
        cv::UMat roi(thresh, rect);
        if (verify_char_sizes(roi)) {

            roi=preproccess_char(roi);
            plate.chars.push_back({roi, rect});
        }
        ++itc;
    } 
}

static void train_plate_detector() {

    if (! asset::manager->open("ml/SVM.xml")) return;

    cv::FileStorage fs;
    fs.open(asset::manager->read(), cv::FileStorage::READ|cv::FileStorage::MEMORY);
    cv::Mat SVM_training;
    cv::Mat SVM_classes;
    fs["TrainingData"] >> SVM_training;
    fs["classes"] >> SVM_classes;
    fs.release();

    if (! classifier) classifier = cv::ml::SVM::create();
    classifier->setType(cv::ml::SVM::C_SVC);
    classifier->setKernel(cv::ml::SVM::LINEAR);
    classifier->setDegree(0.0);
    classifier->setGamma(1.0);
    classifier->setCoef0(0);
    classifier->setC(1);
    classifier->setNu(0.0);
    classifier->setP(0);
    classifier->setTermCriteria(cv::TermCriteria(cv::TermCriteria::MAX_ITER, 1000, 0.01));

    cv::Ptr<cv::ml::TrainData> data = 
        cv::ml::TrainData::create(SVM_training, cv::ml::ROW_SAMPLE, SVM_classes);
    
    classifier->train(data);
    asset::manager->close();
}

static bool is_plate(cv::UMat sample) {

    cv::UMat p = sample.reshape(1, 1);
    p.convertTo(p, CV_32FC1);
    float result = classifier->predict(p);
    return (int)result==1;
}

static cv::UMat hist_eq(cv::UMat input) {

    cv::UMat out(input.size(), input.type());

    if (input.channels()>=3) {
        
        cv::UMat hsv;
        std::vector<cv::UMat> hsv_split;
        cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);
        cv::split(hsv, hsv_split);
        cv::equalizeHist(hsv_split[2], hsv_split[2]);
        cv::merge(hsv_split, hsv);
        cv::cvtColor(hsv, out, cv::COLOR_HSV2BGR);
    } 
    else if (input.channels()==1) {
        cv::Mat _out;
        cv::equalizeHist(input.getMat(cv::ACCESS_READ), _out);
        out = _out.getUMat(cv::ACCESS_READ);
    }
    return out;
}

void filter::ANPR::detect_chars(Plate& plate) {

    if (plate.image.empty()) return;
    plate.chars.clear();
    segment_chars(plate);
}

void filter::ANPR::classify_chars(Plate& plate) {

    if (plate.chars.empty()) return;
    plate.text.clear();
    std::sort(plate.chars.begin(), plate.chars.end(), 
        [] (const Char& c1, const Char& c2) { return c1.pos.x<c2.pos.x; });

    for (auto& char_seg: plate.chars) {
        cv::UMat blob;
        cv::dnn::blobFromImage(char_seg.img, blob, 1.0, {CHAR_SIZE, CHAR_SIZE});
        net.setInput(blob);
        cv::UMat out; net.forward(out);
        cv::Point pos;
        cv::minMaxLoc(out, nullptr, nullptr, nullptr, &pos);
        plate.text << strchars[pos.x]; 
    }     
    //LOGI("%s", plate.text.str().c_str())
    cv::putText(ROI, plate.text.str(), plate.position.tl(), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255,0,0), 2);  
}

static bool preprocess_and_check_is_plate(
    const cv::RotatedRect& min_rect, 
    cv::UMat& input,
    Plate& plate
) {
    float r             = (float)min_rect.size.width / (float)min_rect.size.height;
    float a             = min_rect.angle;
    if (r<1)          a = a-90;
    cv::Size rect_size  = min_rect.size;
    if (r<1)              cv::swap(rect_size.width, rect_size.height);
    cv::UMat rot        = cv::getRotationMatrix2D(min_rect.center, a, 1).getUMat(cv::ACCESS_READ);
    cv::UMat rotated;
    cv::warpAffine(input, rotated, rot, input.size(), cv::INTER_CUBIC);
    cv::UMat crop;
    cv::getRectSubPix(rotated, rect_size, min_rect.center, crop);
    cv::UMat resized(SAMPLE_HEIGHT, SAMPLE_WIDTH, input.type());
    cv::resize(crop, resized, resized.size(), 0, 0, cv::INTER_CUBIC);
    //cv::UMat gray;
    //cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
    //cv::blur(gray, gray, cv::Size(3,3));
    resized=hist_eq(resized);

    bool result = is_plate(resized);
    if (result) plate = Plate(resized, min_rect.boundingRect());
    
    return result;
}

// Мы проводим базовую проверку обнаруженных областей на основе их площади и
// соотношения сторон. Мы будем считать, что область может быть номерным знаком,
// если соотношение сторон составляет приблизительно 520/110 = 4,727272,
// с погрешностью 40% и площадью, основанной на минимуме 15 пикселей
// и максимуме 125 пикселей для высоты пластины. Эти значения
// рассчитываются в зависимости от размера изображения и положения камеры
static bool verify_plate_sizes(cv::RotatedRect& candidate, int mode) {

    static const float error   =   0.5f;
    static const float aspect  =   4.7272f;

    static const int min = 15*aspect*15;
    static const int max = 125*aspect*125;

    static const float rmin  =   aspect-aspect*error;
    static const float rmax  =   aspect+aspect*error;

    int area    =   candidate.size.height*candidate.size.width;
    float r     =   std::max(candidate.size.height, candidate.size.width) /
                    std::min(candidate.size.height, candidate.size.width);
    bool pass;
    
    switch (mode) {
        case 0:     pass = area>min && area<max; break;
        case 1:     pass = (area>min && area<max) && (r>rmin && r<rmax); break;
        default:    pass = false;
    }
    #ifdef DEBUG
           // LOGI("--> sz: %dx%d, a: %f, area: %d:%d:%d, resp %f:%f:%f, %s\n", 
           //     (int)candidate.size.width, (int)candidate.size.height, candidate.angle, min, max, area, rmin, rmax, r, pass? "pass": "no")
    #endif 
    return pass;
}



/// @brief
// Мы можем внести еще больше улучшений, используя
// свойство белого фона номерного знака. Мы можем использовать алгоритм заливки, 
// чтобы получить Rotated Rect для точной обрезки.
// Первым шагом к обрезке номерного знака является получение нескольких точек рядом с центром последнего
// Rotated Rect. Затем мы получим минимальный размер пластины
// между шириной и высотой и будем использовать его для генерации случайных семян вблизи центра кандидата.
// Мы хотим выделить белую область, и нам нужно несколько семян, чтобы коснуться хотя
// бы одного белого пикселя. Затем для каждого начального элемента мы используем функцию заливки, чтобы нарисовать
// новое изображение маски для сохранения новой ближайшей области обрезки:
/// @param input  in   Кадр камеры
/// @param rects  in   предпологаемые места номерных знаков
/// @param plates out  Найденные номерные знаки
static void flood(
    cv::UMat& input, 
    cv::UMat& gray_input,
    std::vector<cv::RotatedRect>& rects,
    Plate& plate
) {
    static const int lo_diff      = 30;
    static const int up_diff      = 30;
    static const int connectivity = 4;
    static const int num_seeds    = 5;
    static const int flags        = connectivity|(255<<8)|cv::FLOODFILL_FIXED_RANGE|cv::FLOODFILL_MASK_ONLY;

    for (int i=0; i<rects.size(); i++) {
        
        float min_size  = std::min(rects[i].size.width, rects[i].size.height);
        min_size        = min_size-(min_size*0.5);
        if (min_size<1) continue;
        auto gen        = std::bind(std::normal_distribution<float>(min_size, 0.5), std::default_random_engine{});
        cv::RotatedRect min_rect;
        bool pass       = false;
        

        for (int j=0; j<num_seeds; j++) {

            cv::Point seed;
            seed.x  = rects[i].center.x+gen();
            seed.y  = rects[i].center.y+gen();
            if (seed.x<0 || seed.y<0 || seed.y>=input.rows || seed.x>=input.cols) continue;
#ifdef DEBUG
           // cv::circle(input, seed, 1, cv::Scalar(0,255,255), -1);
#endif
            cv::UMat _mask = cv::UMat::zeros(gray_input.rows+2, gray_input.cols+2, CV_8UC1);   
            int area=cv::floodFill(gray_input, _mask, seed, cv::Scalar(255,0,0), nullptr, 
                cv::Scalar(lo_diff,lo_diff,lo_diff), cv::Scalar(up_diff, up_diff, up_diff), flags);                

            cv::Mat mask = _mask.getMat(cv::ACCESS_READ);
            std::vector<cv::Point> points_interest;
            cv::Mat_<uchar>::iterator it_mask = mask.begin<uchar>();
            cv::Mat_<uchar>::iterator end     = mask.end<uchar>();

            while (it_mask!=end) {
                if (*it_mask==255) points_interest.push_back(it_mask.pos());
                ++it_mask;
            }

            min_rect = cv::minAreaRect(points_interest);

            if (verify_plate_sizes(min_rect, 1) && preprocess_and_check_is_plate(min_rect, gray_input, plate)) {
             
#ifdef DEBUG
                //cv::Point2f pts[4]; min_rect.points(pts);
                //for (int j=0; j<4; j++) cv::line(input, pts[j], pts[(j+1)%4], cv::Scalar(0,0,255)); 
#endif           
                break;
            } else {

#ifdef DEBUG
            //cv::Point2f pts[4]; min_rect.points(pts);
            //for (int j=0; j<4; j++) cv::line(input, pts[j], pts[(j+1)%4], cv::Scalar(255,0,0)); 
#endif               
            }
        }
    }
}*/


void filter::ANPR::detect_plates(cv::UMat& input, Plate& plate) {

    // Прежде чем находить вертикальные края, нам нужно преобразовать цветное изображение в
    // изображение в оттенках серого (потому что цвет не может помочь нам в этой задаче) и удалить
    // возможный шум, создаваемый камерой или другим окружающим шумом.
    cv::UMat gray;//(input.rows, input.cols, CV_8UC1);
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    cv::UMat blured;
    cv::blur(gray, blured, cv::Size(5,5));

    // Чтобы найти вертикальные ребра, мы будем использовать фильтр Собеля
    cv::UMat sobel;
    cv::Sobel(blured, sobel, CV_8U, 1, 0, 3, 1, 0);

    cv::threshold(sobel, sobel, 0, 255, cv::THRESH_OTSU|cv::THRESH_BINARY);
    
    // Применяя морфологическую операцию close, мы можем удалить пробелы
    // между каждой вертикальной линией ребер и соединить все области с большим
    // количеством ребер. На этом шаге у нас есть возможные области, которые могут содержать
    // номера.
    cv::UMat elem = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(17,3)).getUMat(cv::ACCESS_READ);
    cv::morphologyEx(sobel, sobel, cv::MORPH_CLOSE, elem);

    // После применения этих функций у нас есть области на изображении, которые могли
    // бы содержать номерной знак; однако большинство областей не содержат номерных знаков.
    // Эти области могут быть разделены с помощью анализа подключенных компонентов или с
    // помощью функции findContours. Эта последняя функция извлекает контуры
    // двоичного изображения с помощью различных методов и результатов. Нам нужно только получить
    // внешние контуры с любыми иерархическими отношениями и любыми
    // результатами полигональной аппроксимации
    Contours contours;
    cv::findContours(sobel, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    Contours::iterator itc=contours.begin();
    std::vector<cv::RotatedRect> rects;

    while (itc!=contours.end()) {
        
        auto rect = cv::minAreaRect(*itc);

      //  if (verify_plate_sizes(rect, 0)) rects.push_back(rect);
        ++itc;
    }
#ifdef IMGDEBUG
    cv::drawContours(input, contours, -1, cv::Scalar(255,0,0));
#endif
    //flood(input, gray, rects, plate);  
}

void filter::ANPR::apply(int) {

    Plate plate;
    cv::UMat ROI {input->frame()(roi)};
    detect_plates(ROI, plate);
    //detect_chars(plate);
    //classify_chars(plate); 
}

filter::ANPR::ANPR(Filter* in, cv::Rect roi_input): input{in}, roi{roi_input} {
        
    //train_plate_detector();
    //if (! asset::manager->open("ml/anpr_10.pb")) return;
    //net = cv::dnn::readNetFromTensorflow(asset::manager->read(), asset::manager->size());
    //asset::manager->close();
}