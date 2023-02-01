package com.home.anpr

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.net.wifi.WifiManager
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.text.format.Formatter.formatIpAddress
import android.util.Log
import android.widget.Toast
import androidx.core.content.ContextCompat


private const val PERMISSION_REQUEST_CODE = 10
private val PERMISSIONS_REQUIRED = arrayOf(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)

class Main: Activity() {

    private val tag     = "Main"
    private lateinit var fullscreenContent: GLSurfaceView

    fun getIP():String {
        val wifiManager = getSystemService(WIFI_SERVICE) as WifiManager
        return formatIpAddress(wifiManager.connectionInfo.ipAddress)
    }

    private fun hasPermissions(context: Context) = PERMISSIONS_REQUIRED.all {
        ContextCompat.checkSelfPermission(context, it) == PackageManager.PERMISSION_GRANTED
    }

    private fun begin() {

        fullscreenContent= MyGLView(this, getIP())
        setContentView(fullscreenContent)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.isNotEmpty()) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {

                Toast.makeText(applicationContext, "Permission denied", Toast.LENGTH_LONG).show()
                finish()
            } else {
                begin()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.i(tag, "onCreate")

        if (! hasPermissions(applicationContext)) {

            requestPermissions(PERMISSIONS_REQUIRED, PERMISSION_REQUEST_CODE)
        } else {
            begin()
        }
    }

    override fun onResume() {
        super.onResume()
        Log.i(tag, "onResume")
        fullscreenContent.onResume()
    }

    override fun onPause() {
        Log.i(tag, "onPause")
        fullscreenContent.onPause()
        super.onPause()
    }
}