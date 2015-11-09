package io.github.melvincabatuan.coloredgray;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.TextureView;
import android.widget.ImageView;

import java.io.IOException;
import java.util.List;

public class MainActivity extends Activity implements TextureView.SurfaceTextureListener, Camera.PreviewCallback {

    static {
        System.loadLibrary("ImageProcessing");
    }

    private Camera mCamera;
    private TextureView tv;
    private byte[] videoSource;
    private ImageView ivR;
    private ImageView ivG;
    private ImageView ivB;
    private Bitmap imageR, imageG, imageB;
    private double current_fps;
    private double total_fps; // for average frame rate computation

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tv = (TextureView) findViewById(R.id.preview);
        ivR = (ImageView) findViewById(R.id.imageViewR);
        ivG = (ImageView) findViewById(R.id.imageViewG);
        ivB = (ImageView) findViewById(R.id.imageViewB);

        tv.setSurfaceTextureListener(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }



    int i = 0;
    long now, oldnow, count = 0;

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {

        /// Measure frame rate:
        i++;
        now = System.nanoTime()/1000;
        if (i>3) {
            current_fps = 1000000L / (now - oldnow);
            Log.d("onPreviewFrame: ", "Measured: " + current_fps + " fps.");
            total_fps += current_fps;
            count++;

            if(count%10 == 0){ // Log average every 10 frames
                Log.d("onPreviewFrame: ", "AVERAGE: " + total_fps/count + " fps after " + count + " frames." );
            }
        }

        oldnow = now;


        if (mCamera != null){
            decode(imageR, data, 0xFFFF0000);
            decode(imageG, data, 0xFF00FF00);
            decode(imageB, data, 0xFF0000FF);

            ivR.invalidate();
            ivG.invalidate();
            ivB.invalidate();

            mCamera.addCallbackBuffer(videoSource);
        }
    }

    public native void decode(Bitmap pTarget, byte[] pSource, int pFilter);

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {

        /// Use front-facing camera (if available)

        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();

        for (int camNo = 0; camNo < Camera.getNumberOfCameras(); camNo++) {
            Camera.CameraInfo camInfo = new Camera.CameraInfo();
            Camera.getCameraInfo(camNo, camInfo);

            if (camInfo.facing==(Camera.CameraInfo.CAMERA_FACING_FRONT)) {
                mCamera = Camera.open(camNo);
            }
        }
        if (mCamera == null) { /// Xperia LT15i has no front-facing camera, defaults to back camera
            mCamera = Camera.open();
        }


        try{


            mCamera.setPreviewTexture(surface);
            mCamera.setPreviewCallbackWithBuffer(this);
            mCamera.setDisplayOrientation(0);

            Camera.Size size = findBestResolution(width,height);
            PixelFormat pixelFormat = new PixelFormat();
            PixelFormat.getPixelFormatInfo(mCamera.getParameters().getPreviewFormat(), pixelFormat);
            int sourceSize = size.width * size.height * pixelFormat.bitsPerPixel / 8;

            /// Camera size and video format
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setPreviewSize(size.width, size.height);
            parameters.setPreviewFormat(PixelFormat.YCbCr_420_SP);
            mCamera.setParameters(parameters);

            /// Video buffer and bitmaps
            videoSource = new byte[sourceSize];
            imageR = Bitmap.createBitmap(size.width, size.height, Bitmap.Config.ARGB_8888);
            imageG = Bitmap.createBitmap(size.width, size.height, Bitmap.Config.ARGB_8888);
            imageB = Bitmap.createBitmap(size.width, size.height, Bitmap.Config.ARGB_8888);

            ivR.setImageBitmap(imageR);
            ivG.setImageBitmap(imageG);
            ivB.setImageBitmap(imageB);

            /// Queue video frame buffer and start camera preview
            mCamera.addCallbackBuffer(videoSource);
            mCamera.startPreview();

        } catch (IOException e){
            mCamera.release();
            mCamera = null;
            throw new IllegalStateException();
        }
    }



    private Camera.Size findBestResolution(int pWidth, int pHeight){
        List<Camera.Size> sizes = mCamera.getParameters().getSupportedPreviewSizes();
        Camera.Size selectedSize = mCamera.new Size(0,0);

        for(Camera.Size size: sizes){
            if ((size.width <= pWidth) && (size.height <= pHeight) && (size.width >= selectedSize.width) && (size.height >= selectedSize.height )){
                selectedSize = size;
            }
        }

        if((selectedSize.width == 0) || (selectedSize.height == 0)){
            selectedSize = sizes.get(0);
        }

        return selectedSize;
    }


    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {

        // Release camera

        if (mCamera != null){
            mCamera.stopPreview();
            mCamera.release();

            mCamera = null;
            videoSource = null;

            imageR.recycle();; imageR = null;
            imageG.recycle();; imageG = null;
            imageB.recycle();; imageB = null;
        }
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {

    }
}
