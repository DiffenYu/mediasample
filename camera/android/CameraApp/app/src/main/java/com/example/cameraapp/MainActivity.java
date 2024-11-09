package com.example.cameraapp;

import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "CameraApp";

    private CameraDevice cameraDevice;
    private CameraCaptureSession captureSession;
    private SurfaceView surfaceView;
    private Button btnToggleCamera;
    private Button btnSwitchCamera;
    private boolean isCameraActive = false;
    private boolean isFrontCamera = false;
    private CameraManager cameraManager;
    private String cameraId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surfaceView);
        btnToggleCamera = findViewById(R.id.btnToggleCamera);
        btnSwitchCamera = findViewById(R.id.btnSwitchCamera);

        cameraManager = (CameraManager) getSystemService(CAMERA_SERVICE);

        // Set up the Surface for camera preview
        SurfaceHolder holder = surfaceView.getHolder();
        holder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                // Initialize camera when surface is created
                try {
                    openCamera();
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                // Handle surface changes if needed
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                // Release camera when surface is destroyed
                closeCamera();
            }
        });

        // Button to toggle the camera on/off
        btnToggleCamera.setOnClickListener(v -> {
            if (isCameraActive) {
                closeCamera();
            } else {
                try {
                    openCamera();
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }
            }
        });

        // Button to switch between front and back camera
        btnSwitchCamera.setOnClickListener(v -> {
            isFrontCamera = !isFrontCamera;
            try {
                closeCamera();
                openCamera();
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
        });
    }

    // Open camera with selected id
    private void openCamera() throws CameraAccessException {
        cameraId = isFrontCamera ? getFrontCameraId() : getBackCameraId();

        if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        cameraManager.openCamera(cameraId, new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
                cameraDevice = camera;
                isCameraActive = true;
                Log.d(TAG, "Camera opened");
                startPreview();
            }

            @Override
            public void onDisconnected(CameraDevice camera) {
                cameraDevice.close();
                Log.d(TAG, "Camera disconnected");
            }

            @Override
            public void onError(CameraDevice camera, int error) {
                Log.e(TAG, "Camera error: " + error);
            }
        }, null);
    }

    // Start camera preview
    private void startPreview() {
        try {
            Surface surface = surfaceView.getHolder().getSurface(); // 获取Surface对象
            CaptureRequest.Builder captureRequestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            captureRequestBuilder.addTarget(surface); // 将Surface作为目标

            // 创建CaptureSession
            List<Surface> surfaces = new ArrayList<>();
            surfaces.add(surface); // 将Surface添加到列表

            cameraDevice.createCaptureSession(
                    surfaces, // 使用传统的ArrayList来包含Surface
                    new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(CameraCaptureSession session) {
                            if (cameraDevice == null) return;
                            captureSession = session;
                            try {
                                captureSession.setRepeatingRequest(captureRequestBuilder.build(), null, null);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed(CameraCaptureSession session) {
                            Log.e(TAG, "Capture session configuration failed");
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    // Get ID of the back camera
    private String getBackCameraId() throws CameraAccessException {
        for (String id : cameraManager.getCameraIdList()) {
            CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(id);
            Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
            if (facing != null && facing == CameraCharacteristics.LENS_FACING_BACK) {
                return id;
            }
        }
        return null;
    }

    // Get ID of the front camera
    private String getFrontCameraId() throws CameraAccessException {
        for (String id : cameraManager.getCameraIdList()) {
            CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(id);
            Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
            if (facing != null && facing == CameraCharacteristics.LENS_FACING_FRONT) {
                return id;
            }
        }
        return null;
    }

    // Close the camera when it's no longer needed
    private void closeCamera() {
        if (cameraDevice != null) {
            cameraDevice.close();
            cameraDevice = null;
            isCameraActive = false;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (isCameraActive) {
            closeCamera();
        }
    }
}