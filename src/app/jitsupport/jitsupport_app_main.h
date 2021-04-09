
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_APP_MAIN_H
    #define _JITSUPPORT_APP_MAIN_H

    #include <TTGO.h>
    #include "hardware/callback.h"

    #define MAX_NUMBER_TICKETS          10
    #define EMPTY                       1
    #define FULL                        0
    #define OLD_APP_JIT   


    #define MQTT_CONNECTED_FLAG          _BV(0)
    #define MQTT_DISCONNECTED_FLAG       _BV(1)
  



    void jitsupport_app_main_setup( uint32_t tile_num );
    void newticket( JsonObject jsonObj);
    
    /*
    * @brief registers a callback function which is called on a corresponding event
     * 
     * @param   event               possible values: MQTT_CONNECTED_FLAG, MQTT_DISCONNECTED_FLAG
     * @param   callback_func       pointer to the callback function 
     * @param   id                  pointer to an string
     */
    bool mqtt_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );

    /*
     * @brief trigger a mqtt status event
     * 
     * @param   bits    event to trigger, example: POWERMGM_WIFI_ON_REQUEST for switch an WiFi
     */
    void mqtt_set_event( EventBits_t bits );
    /**
     * @brief clear a mqtt event
     * 
     * @param   bits    event to trigger, example: POWERMGM_WIFI_ON_REQUEST for switch an WiFi
     */
    void mqtt_clear_event( EventBits_t bits );



    bool mqtt_send_event_cb( EventBits_t event, void *arg );
    


#endif // _JITSUPPORT_APP_MAIN_H