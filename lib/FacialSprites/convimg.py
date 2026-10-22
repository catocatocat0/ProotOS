# Converts .png images into an RGB565 RLE-encoded array which can be read by the drawFace() function
import PIL.Image as Image
import glob as glb
import time

if __name__ == '__main__':
    f = open("./images.h", "w")

    for filename in glb.glob("./*.png"):
        unencoded_data = []
        frame = Image.open(filename)

        print("Encoding: " + filename.split(".")[1].split("\\")[-1])
        # Set to True to enable mirroring
        if False:
            newframe = Image.new(mode="RGB", size=(frame.size[0] * 2, frame.size[1]))
            newframe.paste(frame, (0, 0))
            newframe.paste(frame, (frame.size[0], 0))
        else:
            newframe = frame.convert('RGB')

        height = newframe.size[1]
        width = newframe.size[0]

        f.write("const uint32_t static PROGMEM " + filename.split(".")[1].split("\\")[-1] + "[] = {" + str(width) + ", " + str(height) + ", ")
        for y in range(height):
            for x in range(width):
                r, g, b = newframe.getpixel((x, y))
                # Convert 24-bit RGB888 (8-8-8) to 16-bit RGB565 (5-6-5)
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                unencoded_data.append(rgb565)

        # Encode and compress the data using Run-Length Encoding (RLE):
        # Bits 23-8: 16-bit RGB565 color (RRRRRGGGGGGBBBBB)
        # Bits 7-0:  8-bit repeat count (0-255)
        prevInt = (unencoded_data[0] << 8) & 0xffffffff
        counter = 0
        integer = 0
        encoded_data = []

        for item in unencoded_data[1:]:
            integer = (item << 8) & 0xffffffff
            if prevInt == integer and counter < 255:
                counter += 1
            else:
                encoded_data.append(prevInt | (counter & 0xff))
                counter = 0
                print("Encoded uint32: {}".format(bin(encoded_data[-1] & 0xffffffff)))

            prevInt = integer

        # Appends the last element
        encoded_data.append(prevInt | (counter & 0xff))

        print("Array size: {}".format(len(encoded_data)))

        # Adds in the size of the image array to the 3rd element:
        f.write("{0}, ".format(len(encoded_data)))

        # Write the encoded image to a file:
        for elements in encoded_data:
            f.write("{0}, ".format(elements))
        f.write("};\n")

    f.close()
    print("-------------ENCODING FINISHED-------------")