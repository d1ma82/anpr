package com.home.anpr

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.util.Log
import android.view.OrientationEventListener
import android.view.Surface
import java.lang.Integer.max
import java.lang.Integer.min
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class Render(
    private val view:   GLSurfaceView,
    private val camera:   String,
    private val wrapper:   Wrapper,
    private val context:   Context,
    private val ip:         String
):
    GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {

    private val tag                 =   "Render"
    private var cameraHandle:Long   =   0
    private var texture:Int         =   0
    private var update              =   false
    private var width               =   0
    private var height              =   0
    private var orientation         =   0
    private lateinit var surfaceTexture: SurfaceTexture
    private lateinit var surface: Surface

    override fun onSurfaceCreated(p0: GL10?, p1: EGLConfig?) {

        width=view.width
        height=view.height
        Log.i(tag, "onSurfaceCreated, width=$width height=$height")

        val mOrientationListener: OrientationEventListener = object : OrientationEventListener(
            context
        ) {
            override fun onOrientationChanged(o: Int) {
                orientation = o
                //Log.i(tag, "${o}")
            }
        }

        if (mOrientationListener.canDetectOrientation()) {
            mOrientationListener.enable()
        }

        cameraHandle = wrapper.createCamera(context.assets, max(width, height), min(width, height), camera, ip)
        val characteristic = wrapper.characteristic()

        texture= characteristic[0]
        surfaceTexture = SurfaceTexture(texture)
        surfaceTexture.setDefaultBufferSize(characteristic[1], characteristic[2])
        surfaceTexture.setOnFrameAvailableListener(this)
        surface=Surface(surfaceTexture)
        wrapper.onPreviewSurfaceCreated(surface)
    }

    override fun onSurfaceChanged(p0: GL10?, width: Int, height: Int) {

        Log.i(tag, "onSurfaceChanged, width=$width height=$height")
        wrapper.onPreviewSurfaceChanged(width, height)
    }

    override fun onDrawFrame(p0: GL10?) {

        if (cameraHandle <= 0) return

        synchronized (this) {
            if (update) {
                surfaceTexture.updateTexImage()
                update = false
            }
        }
        wrapper.onDraw(orientation)
    }

    @Synchronized
    override fun onFrameAvailable(p0: SurfaceTexture?) {

        update = true
        view.requestRender()
    }

    fun onResume() {
        Log.i(tag, "onResume")
    }

    fun onPause() {
        Log.i(tag, "onPause")
        cameraHandle=0
        update=false
        wrapper.destroy()
        surfaceTexture.release()
    }
}