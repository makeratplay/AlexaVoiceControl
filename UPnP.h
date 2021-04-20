

#pragma once

#define UPnP_UDP_MULTICAST_IP     IPAddress(239,255,255,250)
#define UPnP_UDP_MULTICAST_PORT   1900
#define UPnP_TCP_PORT             80

//#define DEBUG_UPnP                Serial
#ifdef DEBUG_UPnP
    #if defined(ARDUINO_ARCH_ESP32)
        #define DEBUG_MSG_UPnP(fmt, ...) { DEBUG_UPnP.printf_P((PGM_P) PSTR(fmt), ## __VA_ARGS__); }
    #else
        #error Platform not supported
    #endif
#else
    #define DEBUG_MSG_UPnP(...)
#endif


#if defined(ESP32)
    #include <WiFi.h>
#else
	#error Platform not supported
#endif

#include <WiFiUdp.h>
#include "templates.h"

PROGMEM const char UPnP_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=100\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n"
    "hue-bridgeid: %s\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"
    "USN: uuid:2f402f80-da50-11e1-9b23-%s::upnp:rootdevice\r\n"
    "\r\n";


class UPnP {
    public:
        void init();
        void handle();

    private:
        WiFiUDP _udp;
        unsigned int _tcp_port = UPnP_TCP_PORT;

        void _handleUDP();
        void _onUDPData(const IPAddress remoteIP, unsigned int remotePort, void *data, size_t len);
        void _sendUDPResponse();
};