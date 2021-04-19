
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_APP_MAIN_H
    #define _JITSUPPORT_APP_MAIN_H

    #include <TTGO.h>
    #include "hardware/callback.h"

    #define MAX_NUMBER_TICKETS              30
    #define EMPTY                           1
    #define FULL                            0
    

    //#define OLD_APP_JIT   
    //#define NO_HTTP_RESPONSE
    #define NEW_MQTT_IMPLEMENTATION



    #define VIBRATION_DISABLE               0
    #define VIBRATION_SMOOTH                20
    #define VIBRATION_INTENSE               70


    void jitsupport_app_main_setup( uint32_t tile_num );
    void newticket( JsonObject jsonObj);
    
    
    void MQTT_callback(char* topic, byte* message, unsigned int length);
    void MQTT2_publish(char *atualizartopico, char *payload);
    
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