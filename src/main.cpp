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

//load function libraries (to organize better and reduce clutter)
#include "functions/misc.h"
#include "functions/drawfaces.h"

//Loads in the bluetooth module
//#include "bt/bluetooth.h"
//uncomment this when reimplementing bluetooth

#define MIC_PIN 25 //Connect the OUT pin on the MAX4466 onto this pin.
#define BOOP_PIN 33 //This pin is used to detect booping and confirm bluetooth pairing. Connect it to a positive output of your sensor relative to ground.
#define FACE_SELECTOR 32 //This pin cycles through the protogen emotions

//These are our function predefines 
void isSpeaking(uint8_t picker); 
void isIdle();
void Task2code( void * pvParameters );
void IRAM_ATTR boopISR();

//Predefining up task variables for dual core operation.
TaskHandle_t Task1; 
TaskHandle_t Task2;

//These variables keep track of time for the facial animation loop
unsigned long previousMillis; 
unsigned long blinkMillis;
unsigned long currentMillis = millis();

//This defines the list of facial expressions for your protogen
const uint32_t* faceSelector[] = {
  normal, smug
};

//When you add faces to faceSelector, make sure to change this number to how many faces you put in
uint8_t numFaces = 2; 

//Face selector
uint8_t currFace = 1;

//This defines the pointer array to our facial animations which will be used in getMicrophoneLevel()
//With this current setup, index 0-4 is for talking, 0-9 is for loud talking and 10-14 is for yell
const uint32_t* talkSelector[] = {
  talk1, talk2, talk3, talk4, talk5,
  loud1, loud2, loud3, loud4, loud5,
  yell1, yell2, yell3, yell4, yell5
};

//These are the activation thresholds for the talking animations to play. You will most likely need to change these values depending on mic position, manufacturer, etc.
//To change, I recommend using Serial.println(microphoneLevel) and viewing the serial plotter.
const int talk_threshold = 300;
const int loud_threshhold = 600;
const int yelling_threshhold = 1500;

//This is the baseline signal you get from your microphone with no sound. You will have to measure this by setting it to 0 and using Serial.println(microphoneLevel) and viewing what value shows up on the serial plotter with no sound.
const int baseline = 1945; 
uint8_t facePicker = 0;

//This variable keeps track of whether or not the protogen is booped
bool booped = false;

//Code in setup only runs once
void setup(){
  //This sets up our mic pin
  pinMode(MIC_PIN, INPUT);

  //This sets up our interrupt functions
  pinMode(BOOP_PIN, INPUT_PULLUP);
  attachInterrupt(BOOP_PIN, boopISR, RISING);
  attachInterrupt(FACE_SELECTOR, boopISR, FALLING);

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

  startUP();
}

//This is our main loop
void loop(){

  if(booped){
    //Turn off booping. This is also added in each case switch argument to disable boop buffering if someone boops you twice in a row
    booped = false;

    //We use a switch case to play the animation depending on which integer rand selects.
    //If you add more animations, be sure to increase the upper value for rand.
    int rand = random(0,6);
    switch(rand){
        case 0:
          playAnimation(0, 0, bk, true, true);
          booped = false;
          break;
        case 1:
          holdFace(5000, false, 0, 0, bsod, false, false);
          booped = false;
          break;
        case 2:
          playAnimation(0, 0, bk1_, false, true);
          booped = false;
          break;
        case 3:
          playAnimation(0, 0, bk2_, true, true);
          booped = false;
          break;
        case 4:
          holdFace(2500, true, 0, 0, amogus, false, true);
          booped = false;
          break;
        case 5:
          holdFace(5000, false, 0, 0, bsod2, false, false);
          booped = false;
          break;
      }
  }

  //This reads from the mic pin and substract the baseline to end up with the sound's waveform.
  //Because we only care about loudness for selecting which speaking animation to play, we absolute value the baseline
  //Then, we apply a rolling average to the signal as a low-pass filter
  int microphoneLevel = rollingAverage(abs(analogRead(MIC_PIN)- baseline)); //Read pin value
      
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

//Boop sensor interrupt, when the boop sensor pin gets pulled to HIGH, it'll interrupt the facial animations and play a boop animation
void IRAM_ATTR boopISR(){
  booped = true;
}

//This interrupt allows us to cycle through facial expressions.
//We define prevButton for debouncing the signal.
unsigned long prevButton = 0;
void IRAM_ATTR selectFaceISR(){
  if (millis() - prevButton > 250){

    if(currFace > numFaces){
      currFace = 1;
    }else{
      currFace++;
    };

    prevButton = millis();
  };
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
        drawFaceAbberation(0, 0, faceSelector[currFace - 1], true, true);
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

//TODO: ADD IN BLUETOOTH