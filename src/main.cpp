/*NOTE: BEFORE USING --------------------------------------------------------

Use VSCode and platformio to flash the ESP32. Make sure to change platformio.ini configs to the specific boards you are using.
This code has been tested on a FireBeetle ESP32-2 and a standard WROOM devboard. 

Wire your board as follows from this github: https://github.com/2dom/PxMatrix.
*/
//Based on https://github.com/PanickingLynx/ProtOpenSource 
//The code uses a modified wiring scheme, make sure to read which pins go where!.
//Modified by @Katxe

//Uncomment this if your matrices are displaying errors like color shifts or flickering pixels (Mostly the case with matrices bought from AliExpress)
#define PxMATRIX_SPI_FREQUENCY 19900000

//This sets double buffering to be true. It allows for us to change images on the matrix seamlessly
#define double_buffer true

//Include Project dependencies
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <PxMatrix.h>

//Load Sprites
#include "images.h"
#include "animations.h"

//Loads in the bluetooth module
//#include "bt/bluetooth.h"
//uncomment this when reimplementing bluetooth

#define BOOP_PIN 35 //This pin is used to detect booping and confirm bluetooth pairing. Connect it to a positive output of your sensor relative to ground.

#define MIC_PIN 25 //Connect the OUT pin on the MAX4466 onto this pin.

//Connect these pins to your matrix. More information on wiring can be found here: https://github.com/2dom/PxMatrix
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15 
#define P_OE 16

//Define matrix dimensions
#define matrix_width 128
#define matrix_height 32

//These are our function predefinitions 
void getMicrophoneLevel(); 
void drawFace(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);
void playAnimation(int offset_x, int offset_y, Animation animation, bool overlay);
bool boopSensor();
void isSpeaking(uint8_t picker); 
void isIdle();
void Task2code( void * pvParameters );
void drawFaceAbberation(int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);
void holdFace(int time, bool abberation, int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror);

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//Predefining up task variables for dual core operation.
TaskHandle_t Task1; 
TaskHandle_t Task2;

//These variables keep track of time for the facial animation loop
unsigned long previousMillis; 
unsigned long blinkMillis;
unsigned long currentMillis = millis();

// This defines the 'on' time of the display in use. The larger this number,
// the brighter the display. The ESP will crash if this number is too high.
uint8_t display_draw_time = 100; //10-50 is usually fine
PxMATRIX display(128, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);


//This defines the pointer array to our facial animations which will be used in getMicrophoneLevel()
//With this current setup, index 0-4 is for talking, 0-9 is for loud talking and 10-14 is for yell
const uint32_t* talkSelector[] = {
  talk1, talk2, talk3, talk4, talk5,
  loud1, loud2, loud3, loud4, loud5,
  yell1, yell2, yell3, yell4, yell5
};

//Code in setup only runs once
void setup(){
  //This sets up our mic and boop pins, letting us read data values through them
  pinMode(MIC_PIN, INPUT);

  //This sets up bluetooth. *WIP*
  //setupBluetooth();

  //This sets up dual core processing. The 2nd core is required to constantly refresh the display with values from the video buffer
  xTaskCreatePinnedToCore(
    Task2code,    // Task function.
    "Task2",      // name of task.
    11000,        //Stack size of task
    NULL,         // parameter of the task
    24,           // priority of the task
    &Task2,       // Task handle to keep track of created task
    0             // pin task to core 1
  );

  //This sets up Serial as well as the display settings and the booting screen.
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

//This is the threshold value that will trigger the boop animation when the signal on that pin exceeds or equals to.
uint16_t boopThreshold = 2500;

//This function checks if the protogen has been booped by comparing the boop signal to the threshold. 
//It returns true when booped and false if not booped.
bool boopSensor(){
  if(analogRead(BOOP_PIN) >= boopThreshold){

    //We set this variable up to randomly pick from a pool of 4 animations.
    //random(x,y) excludes the upper value so we set it to 4 instead of 3. 
    int rand = random(0,6);

    //We use a switch case to play the animation depending on which integer rand selects.
    //If you add more animations, be sure to increase the upper value for rand.
    switch(rand){
      case 0:
        playAnimation(0, 0, bk, true);
        break;
      case 1:
        holdFace(5000, false, 0, 0, bsod, false, false);
        break;
      case 2:
        holdFace(5000, true, 0, 0, smug, false, true);
        break;
      case 3:
        playAnimation(0, 0, bk2_, true);
        break;
      case 4:
        holdFace(2500, true, 0, 0, amogus, false, true);
        break;
      case 5:
        holdFace(5000, false, 0, 0, bsod2, false, false);
        break;
    }
    return true;
  }else{
    return false;
  }
}

//This is our main loop
void loop(){

  if(!boopSensor()){
    getMicrophoneLevel();
  }

/* BLUETOOTH ***WIP***
  //This runs our bluetooth loop and returns a code
  uint8_t btMessage = btServerLoop();

  //This handles bluetooth related functions
    switch(btMessage){  
      
      //Code 0, run normally
      case 0:
        if(!boopSensor()){
          getMicrophoneLevel();
        }
      break;
      
      //Code 1, play bluetooth display image
      case 1:

      break;
  }
  */
}

//These are the activation thresholds for the talking animations to play. You will most likely need to change these values depending on mic position, manufacturer, etc.
//To change, I recommend using Serial.println(microphoneLevel) and viewing the serial plotter.
const int talk_threshold = 300;
const int loud_threshhold = 600;
const int yelling_threshhold = 1500;

//This is the baseline signal you get from your microphone with no sound. You will have to measure this by setting it to 0 and using Serial.println(microphoneLevel) and viewing what value shows up on the serial plotter with no sound.
const int baseline = 1945; 

uint8_t facePicker = 0;

//These are our variables for the rolling average function.
const int avgWindow = 2;
int avgIndex = 0;
int avgSum = 0;
int avgBuffer[avgWindow];

//This function takes an integer and averages it with the previous value sent to it. It basically functions as a cheap low-pass filter to remove sudden loud noises and other people talking
//val - the value you want the rolling average of
//returns the rolling average of val
int rollingAverage(int val){
  avgSum = avgSum - avgBuffer[avgIndex];       // Remove the oldest entry from the sum
  avgBuffer[avgIndex] = val;           // Add the newest reading to the window
  avgSum = avgSum + val;                 // Add the newest reading to the sum
  avgIndex = (avgIndex+1) % avgWindow;   // Increment the index, and wrap to 0 if it exceeds the window size

  return avgSum / avgWindow;      // Divide the sum of the window by the window size for the result
}

int blinkTimer = 0;

//This function listens to the mic signal and based off of that, plays a facial animation for the given sound level.
void getMicrophoneLevel(){

  //This reads from the mic pin and substract the baseline to end up with the sound's waveform.
  //Because we only care about loudness for selecting which speaking animation to play, we absolute value the baseline
  //Then, we apply a rolling average to the signal as a low-pass filter
  int microphoneLevel = rollingAverage(abs(analogRead(MIC_PIN)- baseline)); //Read pin value

  Serial.println(microphoneLevel);

  //Depending on how large the mic signal is, it picks a facial animation to play for the given loudness range.
  //Make sure to order the if else statements from largest to smallest or else it'll just play only the smallest value
  if(microphoneLevel > yelling_threshhold){
    facePicker = random(10,15);
  }else if(microphoneLevel > loud_threshhold){
    facePicker = random(5,10);
  }else if(microphoneLevel > talk_threshold){
    facePicker = random(0,5);
  }

  //If the mic signal is greater than the talking threshold, it then uses the face picked by the code above to play the speaking animation.
  //If it's less than the talking threshold, it plays the idle face animation.
  if (microphoneLevel > talk_threshold){
    isSpeaking(facePicker);
  }else{ 
    isIdle();
  }
}

//This function plays the speaking animations.
//When it selects the face, it keeps playing it for 110-200ms. Every 5000-1000ms, it plays a blink animation.
//picker - which face to select from our talkSelector[] array
void isSpeaking(uint8_t picker){
  // Open mouth normally
  display.clearDisplay();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= random(10,200)){
    if(currentMillis - blinkMillis >= random(5000, 10000)){
      drawFaceAbberation(0, 0, blink, true, true);
      blinkMillis = currentMillis;
    }else{
        drawFaceAbberation(0, 0, talkSelector[picker], true, true);
    }
    
    delay(100);
    currentMillis = millis();
  }
}

//This displays our idle animation for when no speaking is detected. Works the same way as isSpeaking() minus the face picker variable.
void isIdle(){
  display.clearDisplay();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= random(10,200)){
    
    if(currentMillis - blinkMillis >= random(2000, 10000)){
      drawFaceAbberation(0, 0, blink, true, true);
      blinkMillis = currentMillis;
    }else{
        drawFaceAbberation(0, 0, normal, true, true);
    }

    delay(100);
    currentMillis = millis();
  }
}

//This function holds an image on the secreen for a given number of milliseconds.
void holdFace(int time, bool abberation, int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror){
  display.clearDisplay();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= time){
    
    if(abberation){
      drawFaceAbberation(offset_x, offset_y, image, overlay, mirror);

    }else{
      drawFace(offset_x, offset_y, image, overlay, mirror);
    }
    
    delay(100);
    currentMillis = millis();
  }
}

//This function plays an animation generated by convgif.py in FacialSprites
//offset_x - The x position offset of our animation
//offset_y - The y position offset of our animation
//animation - This is the animation variable generated by convgif. It is the filename of the .gif file inputted into convgif.py
//overlay - This determines whether or not to overlay the next frame ontop of the previous frame. Usually you want to set it to true unless you have smaller frames
void playAnimation(int offset_x, int offset_y, Animation animation, bool overlay){

  //Inside the Animation class are 3 variables:
  //numFrames - how many frames there are in total (also the array lengths)
  //animation - the pointer array to each frame
  //frameTimes - an array containing times in ms to keep the frame for.
  //All these values are automatically filled for you via convgif.py
  for(int i = 0; i < animation.numFrames; i++){
    drawFace(offset_x, offset_y, animation.animation[i], overlay, true);
    delay(animation.frameTimes[i]);
  };
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

//This refreshes the display every 1ms to the currently selected frame buffer
void Task2code( void * pvParameters ) {  
  for (;;) {
    delay(1);
    display.display(display_draw_time);
  }
}

//TODO: ADD IN BLUETOOTH