//This header file defines functions relating to the display and outputtting images 

//Define matrix dimensions
#define matrix_width 128
#define matrix_height 32

//Load dependencies
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <PxMatrix.h>

//Load Sprites
#include "images.h"
#include "animations.h"

//Connect these pins to your matrix. More information on wiring can be found here: https://github.com/2dom/PxMatrix
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5 //After pin C in the PxMatrix graphic from the link above, the pin says GND. Wire that as pin D if using a 32x64 display
#define P_E 15 //Don't actually wire this pin, the 32x64 displays only require P_D for the scan pattern. If you wire this it will break.
#define P_OE 16

// This defines the 'on' time of the display in use. The larger this number,
// the brighter the display. The ESP will crash if this number is too high.
uint8_t display_draw_time = 100; //10-50 is usually fine
PxMATRIX display(128, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

//Function predefines
void drawFaceAbberation(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);
void drawFace(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);
void playAnimation(int offset_x, int offset_y, Animation animation, bool overlay, bool mirror);
void holdFace(int time, bool abberation, int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);
void startUP();

//This function initializes the display
void startUP(){
  Serial.begin(115200);
  display.begin(16);
  display.display(display_draw_time);
  display.setFastUpdate(false);
  display.setBrightness(255);
  display.setPanelsWidth(2);
  display.setTextColor(display.color565(0,0,255));
  display.setCursor(16, 0);
  display.print("BOOTING UP...."); 
  display.setTextColor(display.color565(255,255,255));
  display.setCursor(16, 8);
  display.print("KATXE PROTOGEN OS"); 
  display.showBuffer();
  delay(3000);
  display.showBuffer();
  display.clearDisplay();
}

//This function draws images generated by convimg.py in FacialSprites.
//offset_x - The x offset for the image
//offset_y - The y offset for the image
//image[] - The image to draw on the screen. The first 3 entries are width, height and array size. The images are encoded using run length encodinbg (RLE) and are formatted as follows: first 3 bytes are RGB, last byte is how many times to repeat.
//overlay - This function uses a double buffer for seamless transitions. Setting to false will cause the display not to switch buffers, letting you overlay images on top of eachother.
//mirror - This mirrors the image if set to true
void drawFace(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror){
  
    //From our array, this extracts the metadata of our image from the first 3 entries.
    int imgWidth = image[0];
    int imgHeight = image[1];
    int arraySize = image[2];
  
    //This clears our frame buffer if overlay is set to false
    if(!overlay){
      display.clearDisplay();
    }
    
    //This keeps track of which pixel in our real image we are at
    int pixel_index = 0;
  
    for(int i = 0; i < arraySize; i++){
  
      //This selects the current chunk of pixels to process
      //We offset by 3 because first 3 entries are width, height and array size
      uint32_t pix = image[i+3];
  
      //This unpacks the first 3 bytes into RGB values
      uint8_t r = (pix >> 24 & 0xFF);
      uint8_t g = (pix >> 16 & 0xFF);
      uint8_t b = (pix >> 8 & 0xFF);
  
      //This unpacks the last byte and tells the loop below how many times to repeat the RGB values above
      uint8_t count = pix & 0xFF;
  
      //This then draws those RGB values 'count' times.
      for(int j = 0; j <= count; j++){
  
        //To get which x position in the image, we use an modulo operation which binds x between 0 and the image width. When x exceeds image width, it sets back to 0 and we increment y
        int x = (pixel_index % imgWidth);
        //To get the y position in the image, we perform integer division of the pixel's index by the width. Integer divison floors the resultant value which only increments for multiples of imgWidth.
        int y = floor(pixel_index / imgWidth);
        //We then increment our pixel counter
        pixel_index++;
  
        //This then draws the image to the buffer
        display.drawPixelRGB888(x + offset_x, y + offset_y, r, g, b);
  
        //If mirroring was enabled, draw the image on the other side of the display mirrored
        if(mirror){
          display.drawPixelRGB888(offset_x + imgWidth*2 - x - 1, y + offset_y, r, g, b);
        }
      };
    }
  
    //This then shows the displays the frame buffer to the matrix
    display.showBuffer();
  }

//This function draws images generated by convimg.py in FacialSprites and adds in a chromatic abberation effect by shifting the RGB channels by a given amount
//offset_x - The x offset for the image
//offset_y - The y offset for the image
//image[] - The image to draw on the screen. The first 3 entries are width, height and array size. The images are encoded using run length encodinbg (RLE) and are formatted as follows: first 3 bytes are RGB, last byte is how many times to repeat.
//overlay - This function uses a double buffer for seamless transitions. Setting to false will cause the display not to switch buffers, letting you overlay images on top of eachother.
//mirror - This mirrors the image if set to true
void drawFaceAbberation(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror) {

    //From our array, this extracts the metadata of our image from the first 3 entries.
      int imgWidth = image[0];
      int imgHeight = image[1];
      int arraySize = image[2];
  
    //This clears our frame buffer if overlay is set to false
      if (!overlay) {
          display.clearDisplay();
      }
  
      //This stores each color channel
      uint8_t redChannel[imgWidth * imgHeight];
      uint8_t greenChannel[imgWidth * imgHeight];
      uint8_t blueChannel[imgWidth * imgHeight];
  
      //Extracts the r, g and b values from the entries in the image[] array and put it into its corresponding channel
      int pixel_index = 0;
      for (int i = 0; i < arraySize; i++) {
          uint32_t pix = image[i + 3]; //Shift by 3 because first 3 entries are metadata
          uint8_t r = (pix >> 24 & 0xFF);
          uint8_t g = (pix >> 16 & 0xFF);
          uint8_t b = (pix >> 8 & 0xFF);
          uint8_t count = pix & 0xFF;
  
          //Add in the number of times to repeat the pixel (from RLE encoding)
          for (int j = 0; j <= count; j++) {
              redChannel[pixel_index] = r;
              greenChannel[pixel_index] = g;
              blueChannel[pixel_index] = b;
              pixel_index++;
          }
      }
  
      //Calculate the offset for each channel and direction
      int redOffsetX = random(-1, 2); 
      int redOffsetY = random(-1, 2);
      int greenOffsetX = random(-1, 2);
      int greenOffsetY = random(-1, 2);
      int blueOffsetX = random(-1, 2);
      int blueOffsetY = random(-1, 2);
  
      //Draw the pixels
      for (int i = 0; i < imgHeight*imgWidth; i++) {
  
        //To get which x position in the image, we use an modulo operation which binds x between 0 and the image width. When x exceeds image width, it sets back to 0 and we increment ys
        int x = (i % imgWidth);
        //To get the y position in the image, we perform integer division of the pixel's index by the width. Integer divison floors the resultant value which only increments for multiples of imgWidth.
        int y = floor(i / imgWidth);
  
          //Calculate the position of the shifted pixels
          int redX = x + redOffsetX;
          int redY = y + redOffsetY;
          int greenX = x + greenOffsetX;
          int greenY = y + greenOffsetY;
          int blueX = x + blueOffsetX;
          int blueY = y + blueOffsetY;
  
          //Use the constrain function to make sure that the pixel locations are within bound
          redX = constrain(redX, 0, imgWidth - 1);
          redY = constrain(redY, 0, imgHeight - 1);
          greenX = constrain(greenX, 0, imgWidth - 1);
          greenY = constrain(greenY, 0, imgHeight - 1);
          blueX = constrain(blueX, 0, imgWidth - 1);
          blueY = constrain(blueY, 0, imgHeight - 1);
  
          //Extract the pixels at the calculated shift positions
          uint8_t r = redChannel[redY * imgWidth + redX];
          uint8_t g = greenChannel[greenY * imgWidth + greenX];
          uint8_t b = blueChannel[blueY * imgWidth + blueX];
  
          //Draw the pixels
          display.drawPixelRGB888(x + offset_x, y + offset_y, r, g, b);
  
          //If mirroring was enabled, draw the image on the other side of the display mirrored
          if (mirror) {
              display.drawPixelRGB888(offset_x + imgWidth * 2 - x - 1, y + offset_y, r, g, b);
          }
      }
  
      // Show the frame buffer
      display.showBuffer();
  }

//This function plays an animation generated by convgif.py in FacialSprites
//offset_x - The x position offset of our animation
//offset_y - The y position offset of our animation
//animation - This is the animation variable generated by convgif. It is the filename of the .gif file inputted into convgif.py
//overlay - This determines whether or not to overlay the next frame ontop of the previous frame. Usually you want to set it to true unless you have smaller frames
void playAnimation(int offset_x, int offset_y, Animation animation, bool overlay, bool mirror){

    //Inside the Animation class are 5 variables:
    //animation - array containing our animation (first 5 entries are metadata)
    //numFrames - Total number of frames contained in the animation
    //arrayLength - Length of the animation data
    //Width - Width of the animation
    //Height - Height of the animation
    //All these values are automatically filled for you via convgif.py

		uint32_t numFrames = animation.numFrames;
		uint32_t arrayLength = animation.arrayLength;
		uint32_t Width = animation.Width;
		uint32_t Height = animation.Height;
    const uint32_t* aniArray = animation.animation;

    //This keeps track of how much we need to offset in the animation array by to reach the next frame
    //Initially set to 4 since first 4 bytes is metadata
    uint32_t offset = 4;

    for(int i = 0; i < numFrames; i++){

      //This clears our frame buffer if overlay is set to false
      if(!overlay){
        display.clearDisplay();
      }
      
      //Unpacks the frame's metadata
      uint32_t frameMetaData = aniArray[offset];
      uint32_t frameDataSize = (frameMetaData >> 20 & 0xFFF);
      uint32_t frameDuration = (frameMetaData & 0xFFFFF);

      int pixel_index = 0;
      for(int j = 0; j < frameDataSize; j++){
        
        //This selects the current chunk of pixels to process
        //We offset by 3 because first 3 entries are width, height and array size
        uint32_t pix = aniArray[j+offset+1];
    
        //This unpacks the first 3 bytes into RGB values
        uint8_t r = (pix >> 24 & 0xFF);
        uint8_t g = (pix >> 16 & 0xFF);
        uint8_t b = (pix >> 8 & 0xFF);
    
        //This unpacks the last byte and tells the loop below how many times to repeat the RGB values above
        uint8_t count = pix & 0xFF;
    
        //This then draws those RGB values 'count' times.
        for(int k = 0; k <= count; k++){
    
          //To get which x position in the image, we use an modulo operation which binds x between 0 and the image width. When x exceeds image width, it sets back to 0 and we increment y
          int x = (pixel_index % Width);
          //To get the y position in the image, we perform integer division of the pixel's index by the width. Integer divison floors the resultant value which only increments for multiples of imgWidth.
          int y = floor(pixel_index / Width);
          //We then increment our pixel counter
          pixel_index++;
    
          //This then draws the image to the buffer
          display.drawPixelRGB888(x + offset_x, y + offset_y, r, g, b);
    
          //If mirroring was enabled, draw the image on the other side of the display mirrored
          if(mirror){
            display.drawPixelRGB888(offset_x + Width*2 - x - 1, y + offset_y, r, g, b);
          }
        };
      };

      //Update the offset counter for the next frame
      offset+=frameDataSize+1;

      // Show the frame buffer
      display.showBuffer();
      delay(frameDuration);
    };
  }
  
  //This refreshes the display every 1ms to the currently selected frame buffer
  void Task2code( void * pvParameters ) {  
    for (;;) {
      delay(1);
      display.display(display_draw_time);
    }
  }