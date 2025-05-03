#Use this script to convert .gif files into an Animation varialbe which can be used in the playAnimation() function
#The Animation variable is the file name
import PIL.Image as Image
from PIL import ImageSequence
import glob as glb

if __name__ == '__main__':
    f = open("./animations.h", "w")
    f.write("class Animation{\n\tpublic:\n\t\tconst uint32_t* animation;\n\t\tuint32_t numFrames;\n\t\tuint32_t arrayLength;\n\t\tuint32_t Width;\n\t\tuint32_t Height;\n\n\t\tAnimation(const uint32_t* a):animation(a){\n\t\t\tnumFrames = animation[0];\n\t\t\tarrayLength = animation[1];\n\t\t\tWidth = animation[2];\n\t\t\tHeight = animation[3];\n\t\t}\n};\n\n")

    for filename in glb.glob("./*.gif"):
        name = filename.split(".")[1].split("\\")[-1]
        image = Image.open(filename)
        frame_count = 0
        frame_times = ""
        all_frames_data = []  
        frame_offsets = []    
        total_size = 0
        
        print("Encoding: "+filename.split(".")[1].split("\\")[-1])    
        for idx, frame in enumerate(ImageSequence.Iterator(image)):
            if name == "apple" and not idx % 10 == 0:
                continue
            unencoded_data = []
            frame_times+=str(frame.info['duration'])+","
            #Set to True to enable mirroring
            if False:
                newframe = Image.new(mode="RGB", size=(frame.size[0]*2, frame.size[1]))
                newframe.paste(frame, (0,0))
                newframe.paste(frame, (frame.size[0],0))
            else:
                newframe = frame.convert('RGB')
            height = newframe.size[1]
            width = newframe.size[0]
            
            for y in range(height):
                for x in range(width):
                    r, g, b = newframe.getpixel((x, y))
                    unencoded_data.append([r, g, b])

            #Encode and compress the data:
            #We use 0xffffffff as a mask for since all these values are signed
            prevInt = (unencoded_data[0][0] << 24 | unencoded_data[0][1] << 16 | unencoded_data[0][2] << 8) & 0xffffffff
            counter = 0
            integer = 0
            encoded_data = [] 

            for item in unencoded_data[1:]:
                #ENCODING SCHEME: RRRRRRRR GGGGGGGG BBBBBBBB WWWWWWWWW
                #R - Red (8 bits)
                #B - Blue (8 bits)
                #G - Green (8 bits)
                #W - Number of times to repeat pixel (8 bits)
                integer = (item[0] << 24 | item[1] << 16 | item[2] << 8) & 0xffffffff
                if prevInt == integer and counter < 255:
                    counter+=1
                    
                else:
                    encoded_data.append(prevInt | (counter & 0xffffffff))
                    counter = 0
                    print("Encoded uint32: {}".format(bin(encoded_data[-1] & 0xffffffff)))

                prevInt = integer
            frame_count += 1
            
            #appends the last element
            encoded_data.append(prevInt | (counter & 0xffffffff))
            array_size = len(encoded_data)
            total_size += array_size

            #Encode metadata into 32 bit integer:
            #ENCODING SCHEME: AAAAAAAAAAA WWWWWWWWWWWWWWWWWWWW
            #A - Array Size (12 bits)
            #D - Frame duration in ms (20 bits)
            encoded_metadata = (array_size << 20 | frame.info['duration']) & 0xffffffff
            all_frames_data.append([encoded_metadata] + encoded_data)

        # Add animation metadata
        # 0th entry: Number of frames
        # 1st entry: Data size
        # 2nd entry: Animation Width
        # 3rd entry: Animation Height
        all_frames_data.insert(0,height)
        all_frames_data.insert(0,width)
        all_frames_data.insert(0,total_size+frame_count)
        all_frames_data.insert(0,frame_count)

        print("Array size: {}".format(len(encoded_data)))

        # Write the combined data array for the animation
        f.write("const uint32_t PROGMEM {}_DATA[] = {{\n".format(name))
        f.write(", ".join(map(str, all_frames_data)).replace("]","").replace("[","") + "\n};\n")
        f.write("Animation "+name+"("+name+"_DATA"+");\n\n")

    f.close()
print("-------------ENCODING FINISHED-------------")