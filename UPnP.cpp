
#include "UPnP.h"

void UPnP::handle()
{
    _handleUDP();
}

void UPnP::init()
{
    // UDP setup
    #ifdef ESP32
        _udp.beginMulticast(UPnP_UDP_MULTICAST_IP, UPnP_UDP_MULTICAST_PORT);
    #else
        #error Platform not supported
    #endif
    DEBUG_MSG_UPnP("[UPnP] UDP server started\n");
}

/*
    Sample response message

    HTTP/1.1 200 OK
    EXT:
    CACHE-CONTROL: max-age=100
    LOCATION: http://192.168.86.47:80/description.xml
    SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0
    hue-bridgeid: f008d1d2cb4c
    ST: urn:schemas-upnp-org:device:basic:1
    USN: uuid:2f402f80-da50-11e1-9b23-f008d1d2cb4c::upnp:rootdevice

*/
void UPnP::_sendUDPResponse()
{
    IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    char response[strlen(UPnP_UDP_RESPONSE_TEMPLATE) + 128];
    snprintf_P(
        response, sizeof(response),
        UPnP_UDP_RESPONSE_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,  // LOCATION
        mac.c_str(), // hue-bridgeid
        mac.c_str()  // USN
        );

    DEBUG_MSG_UPnP("\n[UPnP] Responding to M-SEARCH request from %s:%d\n%s", _udp.remoteIP().toString().c_str(), _udp.remotePort(), response);

    _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
    #if defined(ESP32)
        _udp.printf(response);
    #else
        #error Platform not supported
    #endif
    _udp.endPacket();
}

/*
    Sample message received from Amazon Echo

    M-SEARCH * HTTP/1.1
    HOST: 239.255.255.250:1900
    ST: ssdp:all
    MAN: "ssdp:discover"
    MX: 3

*/ 
void UPnP::_handleUDP()
{
    int len = _udp.parsePacket();
    if (len > 0)
    {
        unsigned char data[len + 1];
        _udp.read(data, len);
        data[len] = 0;
        String request = (const char *)data;
        if (request.indexOf("M-SEARCH") >= 0)
        {
            DEBUG_MSG_UPnP("\n[UPnP] M-SEARCH received from  %s:%d\n%s", _udp.remoteIP().toString().c_str(), _udp.remotePort(), (const char *)data);
            if ((request.indexOf("ssdp:discover") > 0) || (request.indexOf("upnp:rootdevice") > 0) || (request.indexOf("device:basic:1") > 0))
            {
                _sendUDPResponse();
            }
        }
    }
}
