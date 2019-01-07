// Global
int VerticalLines = 40;
int HorizontalLines = 44;//48 gets glipy;
int VerticalOffset = 0;
int HorizontalOffset = 0;
int PixelMap[][] = new int[VerticalLines][HorizontalLines];
// Serial
boolean SerialEnabled = true;
String inString = "";
import processing.serial.*;
Serial myPort;
// other
int Scaler = 10;
int Boarder = 50;
boolean RandomMode = false;
boolean DrawENB = false;
//-------------------------------------------------------------------------------//
void setup() {
    size((HorizontalLines*Scaler)+2*Boarder,(VerticalLines*Scaler)+2*Boarder);
    // Serial
    if(SerialEnabled) {
        println("Available ports are:");
        println(Serial.list());
        myPort = new Serial(this, Serial.list()[0], 115200);
        // myPort = new Serial(this, Serial.list()[0], 9600);
        myPort.write("_"); // set up fsm
    }
    for(int i=0;i<VerticalLines;i++)
        for(int j=0;j<HorizontalLines;j++)
            PixelMap[i][j]=0;
}
//-------------------------------------------------------------------------------//
void draw() {
    //-----------Draw Prepare------------//
    background(0);
    if(RandomMode) {
        int i = (int)random(VerticalLines);
        int j = (int)random(HorizontalLines/2); // /2 here because we pack 2 bits
        int v = (int)random(8);
        int v2 = (int)random(8);
        PixelMap[i][2*j] = v;
        PixelMap[i][2*j+1] = v2;
        String StringOut = "_" + (char)((PixelMap[i][2*j]<<4) + PixelMap[i][2*j+1]) + (char)(i) + (char)(j) + "_";
        myPort.write(StringOut);
        println("Sent:" + StringOut);
        VerticalOffset = (int)random(VerticalLines);
        StringOut = "vv" + (char)VerticalOffset + "vv";
        myPort.write(StringOut);
        println("Sent:" + StringOut);
        HorizontalOffset = (int)random(HorizontalLines/2); // /2 here because we pack 2 bits
        StringOut = "hh" + (char)HorizontalOffset + "hh";
        myPort.write(StringOut);
        println("Sent:" + StringOut);
    }
    //-----------Begin Draw------------//
    if(DrawENB) {
        for(int i=0;i<VerticalLines;i++) {
            int y = (i+VerticalOffset)%VerticalLines;
            for(int j=0;j<HorizontalLines;j++) {
                int x = (j+HorizontalOffset*2)%HorizontalLines; // note *2 because 2-in-one-byte
                int r1 = ((PixelMap[y][x]   )%2)*255;
                int g1 = ((PixelMap[y][x]>>1)%2)*255;
                int b1 = ((PixelMap[y][x]>>2)%2)*255;
                fill(r1,g1,b1);
                stroke(r1,g1,b1);
                rect((j*Scaler)+Boarder,(i*Scaler)+Boarder,((j+1)*Scaler)+Boarder,((i+1)*Scaler)+Boarder);
            }
        }
        // Boarder
        fill(64);
        stroke(64);
        rect(0,0,Boarder,(VerticalLines*Scaler)+2*Boarder);
        rect(0,0,(HorizontalLines*Scaler)+2*Boarder,Boarder);
        rect((HorizontalLines*Scaler)+Boarder,0,(HorizontalLines*Scaler)+2*Boarder,(VerticalLines*Scaler)+2*Boarder);
        rect(0,(VerticalLines*Scaler)+Boarder,(HorizontalLines*Scaler)+2*Boarder,(VerticalLines*Scaler)+2*Boarder);
    }
}
//-------------------------------------------------------------------------------//
void mousePressed(){
    println("MouseX:" + mouseX + "MouseY:" + mouseY);
    if(keyPressed) {
        int x = (((mouseX - Boarder)/Scaler)+HorizontalOffset*2)%HorizontalLines;
        int y = (((mouseY - Boarder)/Scaler)+VerticalOffset)%VerticalLines;
        if((x>=0)&&(x<HorizontalLines)&&(y>=0)&&(y<VerticalLines)) {
            if     (key==' ') PixelMap[y][x]  = 0;
            else if(key=='r') PixelMap[y][x] ^= 1;
            else if(key=='g') PixelMap[y][x] ^= 2;
            else if(key=='b') PixelMap[y][x] ^= 4;
            if((key==' ')||(key=='r')||(key=='g')||(key=='b')) {
                String StringOut = "_" + (char)((PixelMap[y][(x/2)<<1]<<4) + PixelMap[y][((x/2)<<1)+1]) + (char)(y) + (char)(x>>1) + "__"; // Add push through
                myPort.write(StringOut);
                println("Sent:" + StringOut);
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyPressed(){
    if     (key=='R') RandomMode = !RandomMode;
    else if(key=='d') DrawENB = !DrawENB;
    else if(key=='a') { // Analog random mode
        myPort.write("rrrrr");
        println("Sent:rrrrr");
    } else if((key=='p')||(key=='P')) {
        String toSend = "" + (char)key + (char)key + (char)key + (char)key + (char)key;
        myPort.write(toSend);
        println("Sent:" + toSend);
    } else if(key == CODED) {
        if(keyCode == UP) {
            VerticalOffset++;
            VerticalOffset %= VerticalLines;
            String toSend = "vv" + (char)VerticalOffset + "vv";
            myPort.write(toSend);
            println("Sent:" + toSend);
        } else if(keyCode == DOWN) {
            if(VerticalOffset!=0) VerticalOffset--;
            else                  VerticalOffset = VerticalLines-1;
            VerticalOffset %= VerticalLines;
            String toSend = "vv" + (char)VerticalOffset + "vv";
            myPort.write(toSend);
            println("Sent:" + toSend);
        } else if(keyCode == LEFT) {
            HorizontalOffset++;
            HorizontalOffset %= HorizontalLines/2;
            String toSend = "hh" + (char)HorizontalOffset + "hh";
            myPort.write(toSend);
            println("Sent:" + toSend);
        } else if(keyCode == RIGHT) {
            if(HorizontalOffset!=0) HorizontalOffset--;
            else                    HorizontalOffset = (HorizontalLines/2)-1; // /2 because 2-in-one-byte
            HorizontalOffset %= HorizontalLines/2;
            String toSend = "hh" + (char)HorizontalOffset + "hh";
            myPort.write(toSend);
            println("Sent:" + toSend);
        }
    } else if(key=='c') {
        // clear
        HorizontalOffset = 0;
        VerticalOffset = 0;
        myPort.write(".....");
        println("Sent:.....");
        for(int i=0;i<VerticalLines;i++)
            for(int j=0;j<HorizontalLines;j++)
                PixelMap[i][j]=0;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

