#include "BluetoothSerial.h"
#define BOOP_PIN 36 //This pin is used to detect booping and confirm bluetooth pairing. Connect it to a positive output of your sensor relative to ground.
#define BUFFER_SIZE 1024 //This defines the size of our bluetooth buffer
int     BUFFER_INDEX;
uint8_t    BUFFER_ARRAY[BUFFER_SIZE];

BluetoothSerial SerialBT; //Bluetooth object

//Variables for SSP (Simple Secure Pairing)
boolean confirmRequestPending = false;
void BTAuthCompleteCallback(boolean success);
void BTConfirmRequestCallback(uint32_t numVal);

bool btConnectionStatus; //Keeps track of bluetooth connection status. True if connected, false if not
bool btStatus; //Keeps track if bluetooth is on or off

void BTAuthCompleteCallback(boolean success){
    confirmRequestPending = false;
    if (success)
    {
        Serial.println("Pairing success.");
    }

    else
    {
        Serial.println("Pairing failed, rejected by user.");
    }
}

void BTConfirmRequestCallback(uint32_t numVal){
    confirmRequestPending = true;
    Serial.println(numVal);
}

//This function sends a string to the device
void btWrite(const char* value)
{
    SerialBT.write((const uint8_t*)value, strlen(value));
}


//This sets up bluetooth
void setupBluetooth(){
    //Sets up the boop pin which will be used to confirm bluetooth connection
    pinMode(BOOP_PIN, INPUT);

    //Enables SSP
    SerialBT.enableSSP();
    SerialBT.onConfirmRequest(BTConfirmRequestCallback);
    SerialBT.onAuthComplete(BTAuthCompleteCallback);

    //Names the protogen
    String devName("KATXE PROTOGEN");

    //Starts bluetooth service
    btStatus = SerialBT.begin(devName);

    //Checks to see if bluetooth was successful
    if (SerialBT.available()){
        Serial.println("Bluetooth ON");
    } else Serial.println("Bluetooth OFF");

    //The connection status is set to false
    btConnectionStatus = false;
}

/*
This is our bluetooth loop function. It returns a code from 0-255 which tells main.cpp what operation to perform.
0 - No operation
1 - Display bluetooth request screen
*/
uint8_t btServerLoop(){
    //Exits the loop if bluetooth isn't turned on 
    if(!btStatus){
        return;
    }
    
    //Waits for a confirmation when pairing
    if (confirmRequestPending)
    {
        if(digitalRead(BOOP_PIN) == HIGH){
            SerialBT.confirmReply(true);
        }
        delay(10);
        return 1;
    }

    //Updates the status of the bluetooth connection
    if (btConnectionStatus != SerialBT.hasClient()){
        btConnectionStatus = !btConnectionStatus;
    }
    
    //Exits the loop if there is no bluetooh connection
    if (!btConnectionStatus){
        return 0;
    }
    
    //Exits the loop if there is no data available
    if(!SerialBT.available()){
        return 0;
    }

    //Set the buffer's index to 0
    BUFFER_INDEX = 0;

    //When data is available, add it to the buffer
    while(SerialBT.available()){
        //Reads the incoming bye
        int data = SerialBT.read();

        //Save it in the buffer and increment its index
        BUFFER_ARRAY[BUFFER_INDEX++] = (byte)data;
    }
    /*
    The first 2 bytes are the command identifier
    The next 2 bytes are any metadata such as array length
    The rest is the data
    */
   uint16_t btCmd = BUFFER_ARRAY[0] >> 8 & 0xFF | BUFFER_ARRAY[1]; //This variable is what command to perform
   uint16_t btMetaData = BUFFER_ARRAY[2] >> 8 & 0xFF | BUFFER_ARRAY[3]; //This variable is the metadata given
}