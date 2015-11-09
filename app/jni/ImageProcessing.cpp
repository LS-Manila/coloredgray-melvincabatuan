#include "io_github_melvincabatuan_coloredgray_MainActivity.h"

#include <android/bitmap.h>
#include <android/log.h>

#include <opencv2/core/core.hpp>

#define  LOG_TAG    "ColoredGray"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define toInt(pValue) \
 (0xff & (int32_t) pValue)

#define max(pValue1, pValue2) \
 (pValue1<pValue2) ? pValue2 : pValue1

#define clamp(pValue, pLowest, pHighest) \
 ((pValue < 0) ? pLowest : (pValue > pHighest) ? pHighest: pValue)

#define color(pColorR, pColorG, pColorB) \
           (0xFF000000 | ((pColorB << 6)  & 0x00FF0000) \
                       | ((pColorG >> 2)  & 0x0000FF00) \
                       | ((pColorR >> 10) & 0x000000FF))



// AVERAGE: 19.89 fps after 500 frames.
void NV21toYUV( const cv::Mat &srcNV21, uint32_t* bitmapContent, int pFilter){
 
 
   int32_t j, i;
   int32_t nRows = 2 * srcNV21.rows / 3; // number of lines
   int32_t nCols = srcNV21.cols;   // number of columns 
   int32_t uvRowIndex, yIndex;
   int32_t y11, y12, y21, y22;
   int32_t colorV, colorU;
    
   for (j = 0, uvRowIndex = 0, yIndex = 0; j < nRows - 1; j+=2, uvRowIndex++, yIndex += nCols) {    
      
      const uchar* current = srcNV21.ptr<const uchar>(j);   // current NV21 row
      const uchar* next    = srcNV21.ptr<const uchar>(j+1); // next NV21 row  
           
      const uchar* uv_row  = srcNV21.ptr<const uchar>(nRows + uvRowIndex); // uv row
    
    
      for (i = 0; i < nCols - 1; i += 2, yIndex += 2) {
      
          // Get Ys
          
          	y11 = 1192 * (max(toInt(current[i])   - 16, 0)); 
          	y12 = 1192 * (max(toInt(current[i+1]) - 16, 0));
          	y21 = 1192 * (max(toInt(next[i])      - 16, 0));
          	y22 = 1192 * (max(toInt(next[i+1])    - 16, 0));      
      
          // Get V & U
          
             colorV = 1192 * (toInt(uv_row[i])   - 16); // 128
             colorU = 1192 * (toInt(uv_row[i+1]) - 16); //
                 
                 
          // Assign to a 2 x 2 block
          
            bitmapContent[yIndex] = color(clamp(y11,  0, 262143), colorU, colorV);
            bitmapContent[yIndex] &= pFilter;
            
            bitmapContent[yIndex + 1] = color(clamp(y12,  0, 262143), colorU, colorV);
            bitmapContent[yIndex + 1] &= pFilter;
            
            bitmapContent[yIndex + nCols] = color(clamp(y21,  0, 262143), colorU, colorV);
            bitmapContent[yIndex + nCols] &= pFilter; 
            
            bitmapContent[yIndex + nCols + 1] = color(clamp(y22,  0, 262143), colorU, colorV);
            bitmapContent[yIndex + nCols + 1] &= pFilter;                 
       }
         
    }
}
  
  
  
  
  
 





// AVERAGE: 25.69 fps after 1770 frames. // Colored Gray
void NV21toGray( const cv::Mat &srcGray, uint32_t* bitmapContent, int pFilter){
 
   int32_t colorR, colorG, colorB;
   int32_t yIndex, i, j;
   int32_t y1192;   
   
   int32_t nRows = srcGray.rows;   // number of lines
   int32_t nCols = srcGray.cols;   // number of columns 
   
   
   /// Convert to 1D array if Continuous
   if (srcGray.isContinuous()) {
        nCols = nCols * nRows;
		nRows = 1; // it is now a 
   }
   

   for (j = 0, yIndex = 0; j < nRows; ++j){ 
   
        //uchar* rowdata = reinterpret_cast<uchar*>(srcGray.data); //failed 
        const char* rowdata = srcGray.ptr<char>(j);                //works
 
        for(i = 0; i < nCols; ++i, ++yIndex ){
               
            y1192 = 1192 * (max(toInt(rowdata[i]) - 16, 0));
            
            /// Just print Luma Y in Green Channel
            colorR = y1192;  // Luma Y
            colorG = y1192;
            colorB = y1192;
                        
            colorR = clamp(colorR, 0, 262143);
            colorG = clamp(colorG, 0, 262143);
            colorB = clamp(colorB, 0, 262143);
 
            bitmapContent[yIndex] = color(colorR, colorG, colorB);
            bitmapContent[yIndex] &= pFilter;
        }
   }
}




/*
 * Class:     io_github_melvincabatuan_coloredgray_MainActivity
 * Method:    decode
 * Signature: (Landroid/graphics/Bitmap;[BI)V
 */


JNIEXPORT void JNICALL Java_io_github_melvincabatuan_coloredgray_MainActivity_decode
  (JNIEnv * pEnv, jobject pClass, jobject pTarget, jbyteArray pSource, jint pFilter){

   AndroidBitmapInfo bitmapInfo;
   uint32_t* bitmapContent;

   if(AndroidBitmap_getInfo(pEnv, pTarget, &bitmapInfo) < 0) abort();
   if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) abort();
   if(AndroidBitmap_lockPixels(pEnv, pTarget, (void**)&bitmapContent) < 0) abort();

   /// Access source array data... OK
   jbyte* source = (jbyte*)pEnv->GetPrimitiveArrayCritical(pSource, 0);
   if (source == NULL) abort();

   /// Store jbyte source into Mat data structure as unsigned char
   cv::Mat src(bitmapInfo.height + bitmapInfo.height/2, bitmapInfo.width, CV_8UC1, (unsigned char *)source);
   
   /// Output luminance Y in the Green channel
   NV21toYUV(src, bitmapContent, pFilter); //ok
   // NV21toGray(src.rowRange(0, bitmapInfo.height), bitmapContent, pFilter);

   /// Release Java byte buffer and unlock backing bitmap
   pEnv-> ReleasePrimitiveArrayCritical(pSource,source,0);
   if (AndroidBitmap_unlockPixels(pEnv, pTarget) < 0) abort();

}
