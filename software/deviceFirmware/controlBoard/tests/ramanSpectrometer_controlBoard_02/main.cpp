#include "mbed.h"
#include "DS18B20.h"
#include "OneWireDefs.h"

#define THERMOMETER DS18B20

#define     msg_cuvette_tray            0x10
#define     msg_cuvette_temp            0x11
#define     msg_cuvette_status          0x12
#define     msg_cuvette_tray_pos        0x13
#define     msg_cuvette_pelt_status     0x14
#define     msg_cuvette_stepper         0x15

#define     msg_filter_wheel            0x20
#define     msg_filter_pos              0x21

#define     msg_laser_temp              0x30
#define     msg_laser_power_status      0x31
#define     msg_laser_ttl_status        0x32
#define     msg_laser_good_status       0x33
#define     msg_laser_color             0x34
#define     msg_uv_index                0x35

#define     msg_ccd_pelt_status         0x40

#define     msg_shutter_servo_pos       0x50
#define     msg_shutter_status          0x51

#define     msg_board_ID                0x60
#define     msg_board_serial            0x61
#define     msg_board_status            0x62
#define     msg_board_time              0x63
#define     msg_board_model             0x64
#define     msg_board_version           0x65
#define     req_reboot                  0x66

#define     msg_ambient_temp            0x70
#define     req_baro                    0x71
#define     req_humidity                0x72


#define     cmd_cuvette                 0xA0
#define     tray_open                   0xA1
#define     tray_close                  0xA2
#define     cuvette_peltier             0xA3
#define     req_cuvette_status          0xA4
#define     req_cuvette_temp            0xA5
#define     req_cuvette_pelt_status     0xA6
#define     req_cuvette_tray_pos        0xA7

#define     cmd_filter_wheel            0xB0
#define     filter_select               0xB1
#define     filter_reset                0xB2
#define     req_filter_ID               0xB3
#define     req_filter_status           0xB4
#define     req_filter_count            0xB5
#define     req_filter_pos              0xB6

#define     cmd_laser                   0xC0
#define     laser_power                 0xC1
#define     laser_ttl                   0xC2
#define     req_laser_good_status       0xC3
#define     req_laser_color             0xC4
#define     req_laser_temp              0xC5
#define     req_uv_index                0xC6

#define     cmd_ccd_peltier             0xD3
#define     ccd_pelt_power              0xD4

#define     cmd_shutter_servo           0xE0
#define     shutter_open                0xE1
#define     shutter_close               0xE2
#define     shutter_deflect             0xE3
#define     shutter_alternate           0xE4
#define     req_shutter_state           0xE5
#define     req_shutter_status          0xE6

#define     packet_start                0xF0
#define     packet_ack                  0xF1
#define     packet_flag                 0xFE
#define     packet_end                  0xFF


DigitalIn button(USER_BUTTON);
DigitalOut grnLED(LED1);
DigitalOut cuvette_IN1(PC_14);
DigitalOut cuvette_IN2(PC_15);
DigitalOut cuvette_IN3(PH_0);
DigitalOut cuvette_IN4(PH_1);
DigitalOut filter_IN1(PA_4);
DigitalOut filter_IN2(PB_0);
DigitalOut filter_IN3(PC_1);
DigitalOut filter_IN4(PC_0);

double temp;
THERMOMETER device(PC_8);
Serial raspi(USBTX, USBRX);

typedef union bytes {
    double d ;
    char c[8];
} bytes;

void printDoubleToHex(double d)
{
    bytes b;
    b.d = d;
    raspi.printf("%f -> %x %x %x %x %x %x %x %x\r\n", b.d, b.c[0], b.c[1], b.c[2], b.c[3], b.c[4], b.c[5], b.c[6], b.c[7]);
}

void printHexToDouble(char *arr)
{
    bytes b;
    for (int i=0; i<8; ++i)
        b.c[i] = arr[i];
    raspi.printf("received hex: %x %x %x %x %x %x %x %x \r\n", b.c[0], b.c[1], b.c[2], b.c[3], b.c[4], b.c[5], b.c[6], b.c[7]);
    raspi.printf("convert to: %f\r\n", b.d);
}

void readTemp(int deviceNum)
{
    temp = device.readTemperature(deviceNum);
    if (deviceNum == 0) {
//        raspi.printf("Cuvette Temperature: %f\r\n", temp);
        const char begin[2]= {packet_flag, packet_start};
        raspi.printf(begin);
        printDoubleToHex(temp);
        const char stop[2]= {packet_flag, packet_end};
        raspi.printf(stop);

    }
    if (deviceNum == 1) {
//        raspi.printf("Laser Emitter Temperature: %f\r\n", temp);
        const char begin[2]= {packet_flag, packet_start};
        raspi.printf(begin);
        printDoubleToHex(temp);
        const char stop[2]= {packet_flag, packet_end};
        raspi.printf(stop);

    }
//    raspi.printf("Device %d is %f",deviceNum, temp);
    wait(0.5);
}

int err;
char* command;
int main()
{
    raspi.baud(921600);
    wait(10);
//    raspi.printf("meridianScientific_ramanSpectrometer_controlBoard_V0\r\n");

    while (!device.initialize());    // keep calling until it works

    int deviceCount = device.getDeviceCount();
//    raspi.printf("Located %d sensors\n\r",deviceCount);

    device.setResolution(twelveBit);
    while(1) {
//        for (int i = 0; i < deviceCount; i++) {
//            readTemp(i);
//        }
        if (raspi.readable()) {
            if (raspi.getc() == cmd_laser) {
                grnLED = 1;
                if (raspi.getc() == req_laser_temp) {
                    readTemp(2);
                }
            }
            if (raspi.getc() == cmd_cuvette) {
                grnLED = 1;
                if (raspi.getc() == req_cuvette_temp) {
                    readTemp(1);
                }
            }
        }
        wait(1);
        grnLED = 0;
    }
}
