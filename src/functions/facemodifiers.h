#include <Arduino.h>

//Base class for defining special effects (modifiers) for the drawFace function
//The process() function modifies the current entry in pixelBuffer which is then displayed by drawFace(). This is done by splitting up the pixelBuffer value into its RGB components, modifying the values, and then repacking it as so:
//the frameComplete function runs after a single frame is finished and likewise, frameBegin is what to do when a frame starts.
class FaceModifier {
public:
    virtual ~FaceModifier() = default;
    virtual void frameBegin(uint32_t* pixelBuffer, int imgWidth, int imgHeight) {}
    virtual uint32_t process(int index, uint32_t* pixelBuffer, int imgWidth, int imgHeight) = 0;
    virtual void frameComplete(uint32_t* pixelBuffer, int imgWidth, int imgHeight) {}
};

//Chromatic abberation modifier
class AberrationModifier : public FaceModifier {
public:
    int redOffsetX, redOffsetY, greenOffsetX, greenOffsetY, blueOffsetX, blueOffsetY;
    AberrationModifier():redOffsetX(random(-1,2)), 
                        redOffsetY(random(-1,2)), 
                        greenOffsetX(random(-1,2)), 
                        greenOffsetY(random(-1,2)), 
                        blueOffsetX(random(-1,2)), 
                        blueOffsetY(random(-1,2)){
    };

    void frameBegin(uint32_t* pixelBuffer, int imgWidth, int imgHeight) override {  
        //Calculate the offset for each channel and direction
        redOffsetX = random(-1, 2); 
        redOffsetY = random(-1, 2);
        greenOffsetX = random(-1, 2);
        greenOffsetY = random(-1, 2);
        blueOffsetX = random(-1, 2);
        blueOffsetY = random(-1, 2);
    }

    uint32_t process(int index, uint32_t* pixelBuffer, int imgWidth, int imgHeight) override {
        int x = index % imgWidth;
        int y = index / imgWidth;

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

        //Find the pixel which corresponds to each shift
        uint32_t redPixel = pixelBuffer[redY * imgWidth + redX];
        uint32_t greenPixel = pixelBuffer[greenY * imgWidth + greenX];
        uint32_t bluePixel = pixelBuffer[blueY * imgWidth + blueX];

        //Isolate the individual color values
        uint8_t r = (redPixel >> 24) & 0xFF;  
        uint8_t g = (greenPixel  >> 16) & 0xFF;
        uint8_t b = (bluePixel >> 8) & 0xFF;

        //We return our modified value
        return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) & 0xFFFFFFFF);
    }

    void frameComplete(uint32_t* pixelBuffer, int imgWidth, int imgHeight) override {
    }
};

//CRT modifier
class CRTModifier : public FaceModifier {
public:
    int scanLineOffset;

    CRTModifier(){
        scanLineOffset = 0;
    }

    uint32_t process(int index, uint32_t* pixelBuffer, int imgWidth, int imgHeight) override {
        int x = index % imgWidth;
        int y = index / imgWidth;

        int displayY = y;
        int distance = (displayY - scanLineOffset + imgHeight) % imgHeight;
        
        //Starting from 0.85 brightness, we generate a gradient that is 5 pixels high and changes 0.15 in brightness the further down we go
        if(distance < 5) {
            float intensity = 0.85f - distance * 0.15f;
            uint32_t pix = pixelBuffer[index];
            uint8_t r = (pix >> 24) & 0xFF;
            uint8_t g = (pix >> 16) & 0xFF;
            uint8_t b = (pix >> 8) & 0xFF;

            r = static_cast<uint8_t>(r * intensity);
            g = static_cast<uint8_t>(g * intensity);
            b = static_cast<uint8_t>(b * intensity);

            //We return our modified value
            return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) & 0xFFFFFFFF);
        }

        return pixelBuffer[index];
    }

    //Update where to move the gradient whenver the frame ends
    void frameComplete(uint32_t* pixelBuffer, int imgWidth, int imgHeight) override {
        scanLineOffset = (scanLineOffset + 1) % imgHeight;
    }
};
