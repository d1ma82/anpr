package com.home.anpr

import android.content.res.AssetManager
import android.util.Log
import android.view.Surface

class Wrapper {
    private val tag="Wrapper"
    private external fun greeting(name:String): String

    /// @brief  создаем камеру, Эта функция должна быть вызвана первой
    /// @param  assetManager
    /// @param  camera идентификатор камеры
    /// @param  width, height размер доступной области камеры
    /// @return указатель на объект камеры
    external fun createCamera(assetManager: AssetManager, width: Int, height: Int, camera: String, IP: String):Long

    /// @brief         Возвращаем характеристики выбранной камеры
    /// @param camera  указатель на объект камеры
    /// @return        Текстура, ширина и высота кадра
    external fun characteristic():IntArray

    /// @brief Информируем библиотеку о том что поверхность камеры создана
    /// @param camera указатель на объект камеры
    /// @param argv   поверхность предосмотра камеры
    external fun onPreviewSurfaceCreated(surface: Surface)
    external fun onPreviewSurfaceChanged(width: Int, height: Int)
    external fun onDraw(orientation: Int)

    external fun shot(path: String)

    external fun destroy()

    init {
        val greet=greeting("Kotlin")
        Log.d(tag, greet)
    }

    companion object {
        init {
            System.loadLibrary("anpr")
        }
    }
}