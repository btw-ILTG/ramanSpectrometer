#include "mbed.h"
#include "rtos.h"

//configure I/O
InterruptIn masterClock_int(PB_14);
InterruptIn shiftGate_int(PC_6);
AnalogIn pixelIn(A0);
DigitalOut led(LED1);
DigitalOut illuminator(PA_8);
DigitalOut blueLED(PB_5);
DigitalOut redLED(PB_10);
Serial raspi(USBTX, USBRX);

//testPin indicators - for use with logic analyzer to determine timing states
DigitalOut tp_signalElements(PC_3);
DigitalOut tp_leadingWaste(PC_2);
DigitalOut tp_trailingWaste(PH_1);
DigitalOut tp_t_INT(PH_0);
DigitalOut tp_dump(PC_15);

//define pin configuration for CCD control signals
#define     trigger                 PB_13
#define     shiftGate               PB_8
#define     icg                     PB_3
#define     masterClock             PB_4

//Set up control signals as a bus  (reversed)
BusOut tcd1304dg_clocks(trigger, shiftGate, icg, masterClock);

//define TCD1304AP/DG Characteristics
#define     signalElements          3648
#define     pixelTotal              3694
#define     leadingDummyElements    16
#define     leadShieldedElements    13
#define     headerElements          3
#define     trailingDummyElements   14
//#define     MV(x) ((0xFFF*x)/3300)
                                                //                      MIST  --  masterClock, icg, shiftGate, tigger
#define     transition_1            4           // beginning            0100
#define     transition_2            12          // clock_1              1100
#define     transition_3            4           // just before t2       0100
#define     transition_4            8           // icg fall             1000
#define     transition_5            2           // rise of sh for t3    0010  --  stretch here to increase integration time (must cycle masterClock)
#define     transition_6            10          // t3 masterHigh        1010  --  stretch here to increase integration time (must cycle masterClock)
#define     transition_7            2           // t3 masterLow         0010
#define     transition_8            8           // t1 masterHigh        1000  --  repeat
#define     transition_9            0           // t1 masterLow         0000  --  repeat
#define     transition_10           12          // t4 masterHigh        1100
#define     transition_11           4           // t4 masterLow         0100  --  repeat from here

int veryLow     = 10;
int low         = 100;
int mediumLow   = 1000;
int medium      = 10000;
int high        = 100000;
int veryHigh    = 5000000;

//define variables
long pixelCount;
int sensitivity                     = low;
double pixelData;
float firmwareVersion               = 0.5;
double pixelValue[signalElements] = { 0 };

void delta_encode(char *buffer, int length){
    char last = 0;
    for (int i = 0; i < length; i++){
        char current = buffer[i];
        buffer[i] = buffer[i] - last;
        last = current;
    }
}
 
void delta_decode(char *buffer, int length){
    char last = 0;
    for (int i = 0; i < length; i++){
        buffer[i] = buffer[i] + last;
        last = buffer[i];
    }
}

void printInfo(){
    time_t seconds = time(NULL);
    unsigned long *uid = (unsigned long *)0x1FF800D0; 
    char buffer[32];
    strftime(buffer, 32, "%I:%M %p\n", localtime(&seconds));
    raspi.printf("\r\nmeridianScientific\r\n");
    raspi.printf("ramanPi - The DIY 3D Printable RaspberryPi Raman Spectrometer\r\n");
    raspi.printf("imagingBoard                                         %s\r", buffer);
    raspi.printf("-------------------------------------------------------------\r\n");
    raspi.printf("Firmware Version: %f\r\n",firmwareVersion);
    raspi.printf("Unique ID: %08X %08X %08X \r\n", uid[0], uid[1], uid[2]);
    raspi.printf("Current Sensitivity: %d\r\n\r\n\r\n", sensitivity);    
}

void dataXfer_thread(void const *args){
    while(1){
        osSignalWait(0x2, osWaitForever);
//        for (int pixelNumber=0; pixelNumber<signalElements; pixelNumber++) {
//            raspi.printf("%i\t%4.12f\r\n", pixelNumber, 5 - ((pixelValue[pixelNumber - 1] * 5) / 4096.0));
//        }
        printInfo();
        osDelay(1000);
        printf("Done.\r\n>");
//        osDelay(5000);
    }
}

void imaging_thread(void const *args){
    while(1){
//        osSignalWait(0x1, osWaitForever);//         m  icg  sh  trg
        blueLED = 1;
        tcd1304dg_clocks = transition_1; //         |    |  |   |
        tcd1304dg_clocks = transition_2; //          |   |  |   |
        tcd1304dg_clocks = transition_3; //         |    |  |   |
        tcd1304dg_clocks = transition_4; //          |  |   |   |
        for(int mcInc = 1; mcInc < sensitivity; mcInc++ ){
        tcd1304dg_clocks = transition_5; //         |   |    |  |
        tcd1304dg_clocks = transition_6; //          |  |    |  |
        }
        tcd1304dg_clocks = transition_7; //         |   |    |  |
        for(int mcInc = 1; mcInc < 80; mcInc++ ){        
        tcd1304dg_clocks = transition_8; //          |  |   |   |
        tcd1304dg_clocks = transition_9; //         |   |   |   |
        }
        tp_t_INT = 1;
        for(int mcInc = 1; mcInc < pixelTotal; mcInc++ ){        
        tcd1304dg_clocks = transition_10; //         |   |  |   |
        tcd1304dg_clocks = transition_11; //        |    |  |   |
        tp_signalElements = 1;
//          FLAG FOR PIXEL READ HERE
//            pixelCount++;
//            pixelValue[pixelCount] = pixelIn.read_u16();
//            if (mcInc >= 32){
//                raspi.printf("%i\t%4.12f\r\n", mcInc, (((pixelIn.read_u16()*3.3)/4096.0)));
//            }
//            raspi.printf("%i\t%4.12f\r\n", mcInc, (((pixelValue[pixelCount]*3.3)/4096.0)));
        tp_signalElements = 0;
        tcd1304dg_clocks = transition_10; //         |   |  |   |
        tcd1304dg_clocks = transition_11; //        |    |  |   |
        }
        tp_t_INT = 0;

//        tcd1304dg_clocks = 13;
        blueLED = 0;
    }
}

//define threads for imaging and data transfer
osThreadDef(imaging_thread, osPriorityRealtime, DEFAULT_STACK_SIZE);
osThreadDef(dataXfer_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

void init(){
    tp_signalElements = 0;
    tp_leadingWaste = 0;
    tp_trailingWaste = 0;
    tp_t_INT = 0;
    tp_dump = 0;
    illuminator = 0;
    redLED = 0;
    blueLED = 0;
    pixelCount = 0;
}    

int main(){
    set_time(1256729737); 
    tcd1304dg_clocks = 0;
    init();
    raspi.baud(921600);
//    raspi.printf("Greetings.\r\n\r\n");
    osThreadId img = osThreadCreate(osThread(imaging_thread), NULL);
    osThreadId xfr = osThreadCreate(osThread(dataXfer_thread), NULL);

    //command interpreter
    while(true){
        char c = raspi.getc();
        switch (c) {
            case 'r':
                osSignalSet(img, 0x1);
                raspi.printf("command:getSpectra\r\n>");
                break;
            case 'i':
                osSignalSet(xfr, 0x2);
                raspi.printf("command:information\r\n>");
                break;
            case '1':
                sensitivity = veryLow;
                raspi.printf("sensitivity:veryLow\r\n>");
                break;
            case '2':
                sensitivity = low;
                raspi.printf("sensitivity:low\r\n>");
                break;
            case '3':
                sensitivity = mediumLow;
                raspi.printf("sensitivity:mediumLow\r\n>");
                break;
            case '4':
                sensitivity = medium;
                raspi.printf("sensitivity:medium\r\n>");
                break;
            case '5':
                sensitivity = high;
                raspi.printf("sensitivity:high\r\n>");
                break;
            case '6':
                sensitivity = veryHigh;
                raspi.printf("sensitivity:veryHigh\r\n>");
                break;
            default:
                break;
        }
//        osDelay(10);
    }
}