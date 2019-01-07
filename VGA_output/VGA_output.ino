//Timer code adopted from : http://www.gammon.com.au/forum/?id=11608
//---------------------------------------------------------------------------//
#include "TimerHelpers.h"
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#define nop asm volatile ("nop\n\t")
#define nop_10 nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;
const int horizontalChunks = 44/2; // 2#'s per byte: 76543210 is: xbgrxBGR (where r comes before R)
volatile int verticalBackPorchLineCnt = 0;
const byte verticalBackPorchLines     = 10;
volatile int LinesDrawn = 0;
volatile byte vChunk    = 0;
byte vChunkpart = 0;
#define partsPerChunk 12
const int verticalPixels = 480;
const int verticalChunks = verticalPixels/partsPerChunk;
// Random Mode
uint16_t randCnt = 0;
boolean RandomMode = true;
byte randHori=0,randVert=0,randVal=0;
// Memory some of these could be volatile, but meh..
const boolean DrawBox = false; // for debug
boolean ClearEnb = false;
boolean SetEnb   = false;
boolean DrawEnb  = true;
byte verticalOffset   = 0;
byte horizontalOffset = 0;
byte FrameCnt = 0;
byte FSM[5] = {0,0,0,0,0};
byte PixelMap[verticalChunks*2][horizontalChunks];
// PINS (do not change)
const byte redPin   = 4;
const byte greenPin = 5;
const byte bluePin  = 6;
const byte hSyncPin = 3;  // Timer 2 OC2B (why it can't move)
const byte vSyncPin = 10; // Timer 1 OC1B (why it can't move)
//---------------------------------------------------------------------------//
#define F_CPU 16000000UL
#define BAUD 115200
// #define BAUD 9600
#include <util/setbaud.h>
//---------------------------------------------------------------------------//
void uart_init(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    #if USE_2X
    UCSR0A |= _BV(U2X0);
    #else
    UCSR0A &= ~(_BV(U2X0));
    #endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);    /* 8-bit data */
    UCSR0B = _BV(RXEN0);//  | _BV(TXEN0);  /* Enable RX  // and TX */
}
//---------------------------------------------------------------------------//
ISR(TIMER1_OVF_vect) { // vSync
    sei();      // Let this interupt get interupted (makes sure the line timing is right)
    FrameCnt++;
    if(SetEnb) setPixels();
    if(ClearEnb) clearPixels();
    else {
        LinesDrawn = 0;
        vChunk = verticalOffset;
        verticalBackPorchLineCnt = verticalBackPorchLines;
        if(RandomMode) randomFunction();
    }
    if(FrameCnt==0) {   // every 256 frames do something that takes long
        DrawEnb = false; // So the picture doesn't get messed up
        if(RandomMode) {
            boolean test=true;
            for(int i=0;i<verticalChunks;i++)
                for(int j=0;j<horizontalChunks;j++)
                    if(PixelMap[i*2][j]!=0) test=false;
            if(test) setPixels();
        }
        DrawEnb = true;
    }
}
//---------------------------------------------------------------------------//
ISR(TIMER2_OVF_vect){ // hSync
    if(verticalBackPorchLineCnt==0) {
        if(LinesDrawn!=verticalPixels) drawCurrentLine();
    } else {
        verticalBackPorchLineCnt--;
    }
    pollSerial();
}
//---------------------------------------------------------------------------//
void setup() {
    uart_init();
    // clearPixels();
    setPixels();
    if(DrawBox) {
        for(int i=0;i<verticalChunks;i++)
        for(int j=0;j<horizontalChunks;j++) {
            PixelMap[i*2][j] = ((i==0)||(j==0)||(i==verticalChunks-1)||(j==horizontalChunks-1)||(i==j))? (1<<4)+4 : 0;
            PixelMap[i*2+1][j] = PixelMap[i*2][j];
        }
    }
    // Disable Timer 0
    TIMSK0 = 0;  // No interrupts on Timer 0
    OCR0A  = 0;  // pin D6
    OCR0B  = 0;  // pin D5
    // Timer 1 - vSync (every 16666.667us, width:2 lines = 63.4us ~= 64us)
    pinMode(vSyncPin, OUTPUT);
    Timer1::setMode(15, Timer1::PRESCALE_1024, Timer1::CLEAR_B_ON_COMPARE);
    OCR1A  = 259; // pin D9  : 16666us / (1024us/16) = 260 -> [0,259]
    OCR1B  = 0;   // pin D10 : 64us    / (1024us/16) = 1   -> [0,0]
    TIFR1  = _BV(TOV1);   // Clear overflow flag
    TIMSK1 = _BV(TOIE1);  // Interrupt on overflow on timer 1
    // Timer 2 - hSync (every (1/60)/525=31.746us ~= 32us, width:98 pixels = 3.8us ~= 4us)
    pinMode(hSyncPin, OUTPUT);
    Timer2::setMode(7, Timer2::PRESCALE_8, Timer2::CLEAR_B_ON_COMPARE);
    OCR2A  = 63; // pin D11 : 32us / (8us/16) = 64 -> [0,63]
    OCR2B  = 7;  // pin D3  : 4us  / (8us/16) = 8  -> [0,7]
    TIFR2  = _BV(TOV2);   // Clear overflow flag
    TIMSK2 = _BV(TOIE2);  // Interrupt on overflow on timer 2
    // Sleep between horizontal sync pulses
    set_sleep_mode(SLEEP_MODE_IDLE);
    // Pins for outputting the colour information
    pinMode(redPin,   OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin,  OUTPUT);
}
//---------------------------------------------------------------------------//
void drawCurrentLine() {
    register byte* PixelMapPtr = &(PixelMap[vChunk*2][horizontalOffset]);
    // Increment LinesDrawn and chunks here to add/overlap to/with front porch
    LinesDrawn++;
    if(vChunkpart == partsPerChunk-1) {
        vChunkpart = 0;
        if(vChunk == verticalChunks-1) {vChunk=0;nop;nop;nop;nop;} // Balanced
        else                           {vChunk++;                } // Balanced
    } else {
        nop_10; // Balancer
        vChunkpart++;
    }
    if(DrawEnb) {
        for(register byte i=0;i<horizontalChunks;i++) {
            PORTD = *PixelMapPtr;
            PixelMapPtr++;
            nop;nop;
            PORTD <<= 4;
        }
        nop;nop;   // Stretch final pixel
        PORTD = 0; // and set to black
    }
}
//---------------------------------------------------------------------------//
void clearPixels() {
    ClearEnb = false; // handshake
    for(uint16_t y=0;y<verticalChunks*2;y++)
        for(uint16_t x=0;x<horizontalChunks;x++)
            PixelMap[y][x] = 0;
}
//---------------------------------------------------------------------------//
void setPixels() {
    SetEnb = false; // handshake
    for(uint16_t y=0;y<verticalChunks*2;y++)
        for(uint16_t x=0;x<horizontalChunks;x++)
            PixelMap[y][x] = 255;
}
//---------------------------------------------------------------------------//
void randomFunction() {
    // Random shifting
    verticalOffset   ^= randVal;
    verticalOffset   %= verticalChunks;
    horizontalOffset ^= randVal;
    horizontalOffset %= horizontalChunks;
    // Pixel Values
    // PixelMap[randVert*2+0][randHori]  |= randVal; // used for toggle testing
    // PixelMap[randVert*2+1][randHori]  |= randVal; // copy
    PixelMap[randVert*2+0][randHori]  &= 255^randVal;
    PixelMap[randVert*2+1][randHori]  &= 255^randVal;
    // PixelMap[randVert*2+0][randHori] ^= randVal; // Two copies (used for horizontal tracking)
    // PixelMap[randVert*2+1][randHori] ^= randVal; // Two copies
    // Next Horizontal
    randHori -= randVal;
    randHori %= horizontalChunks;
    // Next Value and Vertical
    randVal  += analogRead(A0);
    randVert += randVal;
    randVert *= randVal;
    randVert %= verticalChunks;

}
//---------------------------------------------------------------------------//
void pollSerial() {
    if(UCSR0A & (1<<7)) ShiftFSM(UDR0);
    else                CrunchFSM();
}
//---------------------------------------------------------------------------//
void CrunchFSM() {
    if((FSM[0]==FSM[4])&&(FSM[0]=='_')) {   // Pixel Code
        PixelMap[FSM[2]*2+0][FSM[1]] = FSM[3];
        PixelMap[FSM[2]*2+1][FSM[1]] = FSM[3];
    } else if((FSM[0]==FSM[1])&&(FSM[1]==FSM[3])&&(FSM[3]==FSM[4])) { // Other Code (@2)
        if     (FSM[0]=='.'){ClearEnb=true;verticalOffset=0;horizontalOffset=0;}
        else if(FSM[0]=='r') RandomMode = !RandomMode;
        else if(FSM[0]=='v') verticalOffset = FSM[2];
        else if(FSM[0]=='h') horizontalOffset = FSM[2];
        FSM[0] = 128; // So we don't toggle back and forth (and don't stick at zero)
        FSM[1] = 128; // might not be needed
        FSM[3] = 128; // might not be needed
        FSM[4] = 128; // might not be needed
    }
}
//---------------------------------------------------------------------------//
void ShiftFSM(char newChar) {
    // Unroll loop for slight speedup
    FSM[4]=FSM[3];
    FSM[3]=FSM[2];
    FSM[2]=FSM[1];
    FSM[1]=FSM[0];
    FSM[0]=newChar;
}
//---------------------------------------------------------------------------//
// Loop sleep to ensure we start up clean when the INTs kick
void loop() {while(true) sleep_mode();}
// For some reason we need a while loop
//(otw the serial-rx/fsm-related screen-glips get way worse)
//---------------------------------------------------------------------------//
