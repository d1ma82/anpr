# Android NDK ANPR system
min API 24 req

An example of ANPR system using NDK Camera2 API.
The algorithm in the first approximation looks like this:
Server:
1. Connect camera
2. Get frame via OpenGL
3. Read from OpenGL to OpenCV
4. Draw region of interest
5. Create RTMP server
Client:
1. Connect to server
2. Get frame
3. Proccess it throw anpr algorithm
4. Show result

While proccess frame we use Sobel filter to detect vertical edges, next
via close operation find possible regions.
For better detecting plates we exploit that fact that plates have a lot of white space, so
using a flooding we can determine with great accuracy plate position.
Next a SVM machine, which is trained to detect is it plate or not, say result.
If it plate begin to proccess chars, here we train a TensorFlow model for OCR stuff
