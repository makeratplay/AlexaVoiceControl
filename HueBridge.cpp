#include "HueBridge.h"
#include <vector>
#include <WiFi.h>
#include <WebServer.h>
#include "templates.h"
#include "SimpleJson.h"

unsigned char HueBridge::addDevice(const char *device_name)
{
    device_t device;
    unsigned int device_id = lights.size();

    // init properties
    device.name = strdup(device_name);
    device.state = false;
    device.bri = 254;
    device.hue = 0;
    device.sat = 0;
    device.ct = 153;   // must be 153 - 500
    device.mode = 'x'; // possible balues 'hs', 'xy', 'ct'

    // create the uniqueid
    String mac = WiFi.macAddress();

    snprintf(device.uniqueid, 27, "%s:%s-%02X", mac.c_str(), "00:00", device_id);

    // Attach
    lights.push_back(device);
    DEBUG_MSG_HUE("Device '%s' added as #%d\n", device_name, device_id);
    return device_id;
}

void HueBridge::start()
{

    webServer.on("/description.xml", HTTP_GET, [this]() { handle_GetDescription(); });
    webServer.on("/api", HTTP_POST, [this]() { handle_PostDeviceType(); });
    webServer.on("/api/{}/lights", HTTP_GET, [this]() { handle_GetState(); });
    webServer.on("/api/{}/lights/{}", HTTP_GET, [this]() { handle_GetState(); });
    webServer.on("/api/{}/lights/{}/state", HTTP_PUT, [this]() { handle_PutState(); });
    webServer.on("/", HTTP_GET, [this]() { handle_root(); });
    webServer.on("/debug/clip.html", HTTP_GET, [this]() { handle_clip(); });

    webServer.onNotFound( [this]() { handle_CORSPreflight(); });

    webServer.enableCORS();
    webServer.begin();
    DEBUG_MSG_HUE("HTTP server started");

    upnp.init();
}

void HueBridge::handle()
{
    webServer.handleClient();
    upnp.handle();
}

/*
    GET /description.xml

    Sample response 

    <root xmlns="urn:schemas-upnp-org:device-1-0">
        <specVersion>
            <major>1</major>
            <minor>0</minor>
        </specVersion>
        <URLBase>http://192.168.86.47:80/</URLBase>
        <device>
            <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
            <friendlyName>Philips hue (192.168.86.47:80)</friendlyName>
            <manufacturer>Royal Philips Electronics</manufacturer>
            <manufacturerURL>http://www.philips.com</manufacturerURL>
            <modelDescription>Philips hue Personal Wireless Lighting</modelDescription>
            <modelName>Philips hue bridge 2012</modelName>
            <modelNumber>929000226503</modelNumber>
            <modelURL>http://www.meethue.com</modelURL>
            <serialNumber>f008d1d2cb4c</serialNumber>
            <UDN>uuid:2f402f80-da50-11e1-9b23-f008d1d2cb4c</UDN>
            <presentationURL>index.html</presentationURL>
        </device>
    </root>


*/
void HueBridge::handle_GetDescription()
{
    DEBUG_MSG_HUE("\nHandling handle_GetDescription (GET %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

    IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    char response[strlen_P(HUE_DESCRIPTION_TEMPLATE) + 64];
    snprintf_P(
        response, sizeof(response),
        HUE_DESCRIPTION_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], // URLBase
        ip[0], ip[1], ip[2], ip[3], // friendlyName
        mac.c_str(),                         // serialNumber
        mac.c_str()                          // UDN
    );

    webServer.send(200, "text/xml", response);

    DEBUG_MSG_HUE(response);
}

/* 
    Handle creating an authorized user

    POST /api HTTP/1.1

    {"devicetype": "Echo"} 
    

    Sample Response

    [
        {
            "success": {
            "username":  "userid"
            }
        }
    ]

*/
void HueBridge::handle_PostDeviceType()
{
    DEBUG_MSG_HUE("Handling handle_PostDeviceType (POST %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

    String body = webServer.arg("plain");
    DEBUG_MSG_HUE(body.c_str());

    char buffer[strlen_P(HUE_USER_JSON_TEMPLATE) + 10];
    snprintf_P(
        buffer, sizeof(buffer),
        HUE_USER_JSON_TEMPLATE,
        "userid");

    // Handling devicetype request
    webServer.send(200, "application/json", buffer);
    DEBUG_MSG_HUE(buffer);
}

/*
    Handle fetching the list of lights:
        GET /api/userid/lights HTTP/1.1

        sample response:

        {
        "1": {
            "type":  "Extended color light",
            "name":  "nuclear reactor",
            "uniqueid":  "F0:08:D1:D2:CB:4C:00:00-00",
            "modelid":  "LCB001",
            "manufacturername":  "Signify Netherlands B.V.",
            "productname":  "Hue color downlight",
            "state":  {
            "on":  false,
            "bri":  254,
            "hue":  0,
            "sat":  0,
            "ct":  153,
            "colormode":  "xy",
            "effect":  "none",
            "mode":  "homeautomation",
            "reachable":  true
            },
            "swversion":  "1.53.3_r27175"
        }
        }


    Or fetching a single light:    
        GET /api/userid/lights/1 HTTP/1.1

        sample response: 

        {
        "type":  "Extended color light",
        "name":  "nuclear reactor",
        "uniqueid":  "F0:08:D1:D2:CB:4C:00:00-00",
        "modelid":  "LCB001",
        "manufacturername":  "Signify Netherlands B.V.",
        "productname":  "Hue color downlight",
        "state":  {
            "on":  false,
            "bri":  254,
            "hue":  0,
            "sat":  0,
            "ct":  153,
            "colormode":  "xy",
            "effect":  "none",
            "mode":  "homeautomation",
            "reachable":  true
        },
        "swversion":  "1.53.3_r27175"
        }        

*/
void HueBridge::handle_GetState()
{
    DEBUG_MSG_HUE("\nHandling handle_GetState (GET %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

    int pos = webServer.uri().indexOf("lights");

    unsigned char id = webServer.uri().substring(pos + 7).toInt();

    String response;
     
    if (0 == id)   // Client is requesting all devices
    {
        response += "{";
        for (unsigned char i = 0; i < lights.size(); i++)
        {
            if (i > 0)
            {
                response += ",";
            }
            response += "\"" + String(i + 1) + "\":" + deviceJson(i);
        }
        response += "}";
    }
    else   // Client is requesting a single device
    {
        response = deviceJson(id - 1);
    }

    webServer.send(200, "application/json", (char *)response.c_str());
    DEBUG_MSG_HUE(response.c_str());
}

String HueBridge::deviceJson(unsigned char id)
{
    if (id >= lights.size())
        return "{}";

    device_t device = lights[id];
    char buffer[strlen_P(HUE_DEVICE_JSON_TEMPLATE) + 76];
    snprintf_P(
        buffer, sizeof(buffer),
        HUE_DEVICE_JSON_TEMPLATE,
        device.name,
        device.uniqueid,
        device.state ? "true" : "false",
        device.bri,
        device.hue,
        device.sat,
        device.ct,
        device.mode == 'h' ? "hs" : device.mode == 'c' ? "ct" : "xy");
    return String(buffer);
}

/*
    PUT /api/userid/lights/1/state HTTP/1.1

    Handle commands that Alexa can send to us:
    1. Turn a light on and off
        {"on":true}  "Turn on light"
        {"on":false} "Turn light off"

    2. Set the brightness of a light (a value from 1 to 254)
        {"on":true,"bri":183} "Make light brighter"
        {"on":true,"bri":128} "Set light to 50%"
        {"on":true,"bri":254} "Set light to 100%"


    3. Set the color of a light by color temperature
        {"on":true,"ct":500} "Make light warmer"
        {"on":true,"ct":383} "Set ligth warm white"

    4. Set the color of a light by hue and saturation 
        {"on":true,"hue":9102,"sat":254} "Set light to gold"

    5. Set the color of a light by XY coordinates. Not handling this mode as I have 
       not seen Alexa send this command.


    Values that are sent from Alexa App
    Shades of White
        - Warm white      {"on":true,"ct" : 383}
        - Soft white      {"on":true,"ct" : 350}
        - White           {"on":true,"ct" : 284}
        - Daylight white  {"on":true,"ct" : 234}
        - Cool white      {"on":true,"ct" : 199}

    Colors
        - Red           {"on":true, "hue" : 0, "sat" : 254 }
        - Crimson       {"on":true, "hue" : 63351, "sat" : 231 }
        - Salmon        {"on":true, "hue" : 3095, "sat" : 132 }
        - Orange        {"on":true, "hue" : 7100, "sat" : 254 }
        - Gold          {"on":true, "hue" : 9102, "sat" : 254 }
        - Yellow        {"on":true, "hue" : 10923, "sat" : 254 }
        - Green         {"on":true, "hue" : 21845, "sat" : 254 }
        - Turquoise     {"on":true ,"hue" : 31675, "sat" : 183 }
        - Cyan          {"on":true ,"hue" : 32768, "sat" : 254 }
        - Sky blue      {"on":true, "hue" : 35862, "sat" : 107 }
        - Blue          {"on":true, "hue" : 43690, "sat" : 254 }
        - Purple        {"on":true, "hue" : 50426, "sat" : 219 }
        - Magenta       {"on":true, "hue" : 54613, "sat" : 254 }
        - Pink          {"on":true, "hue" : 63351, "sat" : 64 } 
        - Lavender      {"on":true, "hue" : 46421, "sat" : 127 }
   
*/
void HueBridge::handle_PutState()
{
    DEBUG_MSG_HUE("\nHandling handle_PutState (PUT %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

    unsigned char id = 0;

    int pos = webServer.uri().indexOf("lights");
    if (pos >= 0){
        id = webServer.uri().substring(pos + 7).toInt();
    }

    String body = webServer.arg("plain");
    DEBUG_MSG_HUE(body.c_str());

    if (body.length() == 0){
        char response[strlen_P(HUE_ERROR_TEMPLATE) + webServer.uri().length() + 40];
        snprintf_P(
            response, sizeof(response),
            HUE_ERROR_TEMPLATE,
            5,
            webServer.uri().c_str(),
            "invalid/missing parameters in body");        
        webServer.send(400, "application/json", response);
    }
    else if (id == 0 || id > lights.size()){
        char response[strlen_P(HUE_ERROR_TEMPLATE) + webServer.uri().length() + 30];
        snprintf_P(
            response, sizeof(response),
            HUE_ERROR_TEMPLATE,
            3,
            webServer.uri().c_str(),
            "resource not available");        
        webServer.send(400, "application/json", response);
    }
    else{
        --id;
        SimpleJson json;
        json.parse(body);

        unsigned char bri = json.hasPropery("bri") ? json["bri"].getInt() : 0;
        short ct = json.hasPropery("ct") ? json["ct"].getInt() : 0;
        unsigned int hue = json.hasPropery("hue") ? json["hue"].getInt() : 0;
        unsigned char sat = json.hasPropery("sat") ? json["sat"].getInt() : 0;

        //xy beats ct beats hue, sat
        char mode = json.hasPropery("xy") ? 'x' : json.hasPropery("ct") ? 'c' : 'h';
        setState(id, json["on"].getBool(), bri, ct, hue, sat, mode);

        char buffer[50];
        String rep = "[";
        snprintf(buffer, sizeof(buffer), "{\"success\":{\"/lights/%d/state/on\":%s}}", id + 1, lights[id].state ? "true" : "false");
        rep += buffer;
        if (json.hasPropery("bri"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/bri\":%d}}", id + 1, lights[id].bri);
            rep += buffer;
        }
        if (json.hasPropery("hue"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/hue\":%d}}", id + 1, lights[id].hue);
            rep += buffer;
        }
        if (json.hasPropery("sat"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/sat\":%d}}", id + 1, lights[id].sat);
            rep += buffer;
        }
        if (json.hasPropery("ct"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/ct\":%d}}", id + 1, lights[id].ct);
            rep += buffer;
        }
        rep += "]";
        webServer.send(200, "application/json", rep.c_str());
        DEBUG_MSG_HUE(rep.c_str());
    }
}

void HueBridge::setState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
    if (id < lights.size()){
        lights[id].state = state;
        lights[id].bri = bri != 0 ? bri : lights[id].bri;
        lights[id].ct = ct != 0 ? ct : lights[id].ct;
        lights[id].hue = hue;
        lights[id].sat = sat;
        lights[id].mode = mode;

        if (_setCallback)
        {
            _setCallback(id, lights[id].state, lights[id].bri, lights[id].ct, lights[id].hue, lights[id].sat, lights[id].mode);
        }
    }
}

void HueBridge::handle_root()
{
    DEBUG_MSG_HUE("\nHandling handle_root (GET %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());
    char response[strlen_P(INDEX_PAGE)];
    snprintf_P(
        response, sizeof(response),
        INDEX_PAGE);
    webServer.send(200, "text/html", response);
}

void HueBridge::handle_clip()
{
    DEBUG_MSG_HUE("\nHandling handle_clip (GET %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());
    char response[strlen_P(CLIP_PAGE)];
    snprintf_P(
        response, sizeof(response),
        CLIP_PAGE);
    webServer.send(200, "text/html", response);
}

void HueBridge::handle_CORSPreflight(){

    if ( webServer.method() == HTTP_OPTIONS ){
        DEBUG_MSG_HUE("\nHandling handle_CORSPreflight (OPTIONS %s) request from %s\n", webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

        webServer.sendHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS");
        webServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        webServer.send(204);
    }
    else{
        handle_NotFound();
    }
}

void HueBridge::handle_NotFound()
{
    String method = "";
    switch (webServer.method())
    {
    case HTTP_GET:
        method = F("GET");
        break;
    case HTTP_POST:
        method = F("POST");
        break;
    case HTTP_DELETE:
        method = F("DELETE");
        break;
    case HTTP_PUT:
        method = F("PUT");
        break;
    case HTTP_PATCH:
        method = F("PATCH");
        break;
    case HTTP_HEAD:
        method = F("HEAD");
        break;
    case HTTP_OPTIONS:
        method = F("OPTIONS");
        break;
    }

    DEBUG_MSG_HUE("\nhandle_NotFound (%s %s) request from %s\n", method, webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

    char response[strlen_P(HUE_ERROR_TEMPLATE) + webServer.uri().length() + 30];
    snprintf_P(
        response, sizeof(response),
        HUE_ERROR_TEMPLATE,
        4,
        webServer.uri().c_str(),
        "method, " + method + ", not available");


    webServer.send(404, F("application/json"), response);
    DEBUG_MSG_HUE(response);
}

