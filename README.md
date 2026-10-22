# Katxe's ProotOS
 
A library for driving protogens using a HUB75 style matrix using an ESP32

Based on PanickingLynx's ProtOpenSource code! Check them out here: https://github.com/PanickingLynx/ProtOpenSource!

This program lets you turn .png and .gif files into code that displays however you want on HUB75 displays!

## Features

Comes with boop functionality! Use any sensor like capacitive, buttons or even IR!

Has microphone and talking animation capabilities using the max4466!

## Instructions:

You will need platformio (I recommend VSCode and the Platformio extension) to upload your protogen code

You will also need to download the latest version of Python.

For Windows users, navigate to ProotOS/lib/FacialSprites and then run "install python dependencies.bat". You can then modify the image files inside the folder and run "run convimg.bat" and or "run convgif.bat" to convert the images into code. Use the drawFace() function in main.cpp to draw display the images on your HUB75 matrix. 

## How to wire up the HUB75 LEDs:
wiring diagram: <img width="583" height="453" alt="Wiring Guide" src="https://github.com/user-attachments/assets/285ed0df-ffb1-4587-ad7e-f1badb578bee" />
video tutorial: https://drive.google.com/file/d/1GCi4A4m8zJsYjqoZM69Rh0MwG5as3JI5/view?usp=sharing
