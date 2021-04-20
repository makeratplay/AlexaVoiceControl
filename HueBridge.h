#pragma once

#include <vector>
#include <WebServer.h>
#include "UPnP.h"

#define DEBUG_HUE                Serial
#ifdef DEBUG_HUE
    #if defined(ARDUINO_ARCH_ESP32)
        #define DEBUG_MSG_HUE(fmt, ...) { DEBUG_HUE.printf_P((PGM_P) PSTR(fmt), ## __VA_ARGS__); }
    #else
        #error Platform not supported
    #endif
#else
    #define DEBUG_MSG_HUE(...)
#endif


typedef struct {
    char * name;
    bool state;
    unsigned char bri;
    char uniqueid[28];
    short ct;
    unsigned int hue;
    unsigned char sat;
    char mode; 

} device_t;

typedef std::function<void(unsigned char, bool, unsigned char, short, unsigned int, unsigned char, char)> TSetStateCallback;

class HueBridge
{
    public:
        unsigned char addDevice(const char * device_name);
        void start();
        void handle();

        void onSetState(TSetStateCallback fn) { _setCallback = fn; }
        void setState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode);

    private:
        void handle_GetDescription();
        void handle_PostDeviceType();
        void handle_GetState(); 
        String deviceJson(unsigned char id);
        void handle_PutState();
        void handle_root();
        void handle_clip();
        void handle_CORSPreflight();
        void handle_NotFound();
        

        std::vector<device_t> lights;
        UPnP upnp; 
        WebServer webServer; 
        TSetStateCallback _setCallback = NULL;
        String uuid = "";
};