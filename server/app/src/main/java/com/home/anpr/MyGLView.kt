package com.home.anpr

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.Log
import android.view.View
import androidx.core.content.ContextCompat
import java.io.File


class MyGLView(context: Context, ip:String) : GLSurfaceView(context), View.OnClickListener {

    private val tag     = "MyGLView"
    private val camera  = "back"
    private var aDirArray: Array<File> = ContextCompat.getExternalFilesDirs(context, null)
    private val wrapper = Wrapper()
    private val render  = Render(this, camera, wrapper, context, ip)

    override fun onResume() {
        Log.i(tag, "onResume")
        super.onResume()
        render.onResume()
    }

    override fun onPause() {
        Log.i(tag, "onPause")
        render.onPause()
        super.onPause()
    }

    override fun onClick(view: View?) {

        if (aDirArray.size>1) {

            val aExtDcimDir = File(aDirArray[0], "")
            Log.i(tag, aExtDcimDir.absolutePath)
            wrapper.shot(aExtDcimDir.absolutePath)
        }
    }

    init {
        val aExtDcimDir = File(aDirArray[0], "")
        Log.i(tag, "Debug path :${aExtDcimDir.absolutePath}")
        Log.i(tag, "IP $ip")
        setEGLContextClientVersion(3)
        setRenderer(render)
        renderMode = RENDERMODE_WHEN_DIRTY
    }
}