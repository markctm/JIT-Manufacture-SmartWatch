
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_MQTT_H
    #define _JITSUPPORT_MQTT_H

    #include <TTGO.h>
    #include "hardware/callback.h"

#define MQTT_CONNECTED_FLAG       (1<<0)
#define MQTT_DISCONNECTED_FLAG    (1<<1)



void mqttctrl_setup(); 

    

    
#endif // _JITSUPPORT_APP_MAIN_H