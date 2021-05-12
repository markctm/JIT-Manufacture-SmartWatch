
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_MQTT_H
    
    #define _JITSUPPORT_MQTT_H
    #include <TTGO.h>
    #include "hardware/callback.h"


/* Possible values for client.state()
 MQTT_CONNECTION_TIMEOUT     -4
 MQTT_CONNECTION_LOST        -3
 MQTT_CONNECT_FAILED         -2
 MQTT_DISCONNECTED           -1
 MQTT_CONNECTED               0
 MQTT_CONNECT_BAD_PROTOCOL    1
 MQTT_CONNECT_BAD_CLIENT_ID   2
 MQTT_CONNECT_UNAVAILABLE     3
 MQTT_CONNECT_BAD_CREDENTIALS 4
 MQTT_CONNECT_UNAUTHORIZED    5
*/

#define MQTT_CONNECTED_FLAG                                  _BV(0)         /** @brief event mask for mqtt connected */
#define MQTT_DISCONNECTED_FLAG                               _BV(1)         /** @brief event mask for mqtt disconnected */
#define MQTT_START_CONNECTION                                _BV(2)         /** @brief event mask for mqtt disconnected */

#define MQTT_PUBLISH_PAYLOAD_SIZE                             200


void mqttctrl_setup(); 
bool mqqtctrl_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );
void MQTT2_publish(char atualizartopico, char payload);
void MQTT2_set_subscribe_topics(char *topico_receber, char * topico_atualizar, char * topico_area);
void mqqtctrl_set_event( EventBits_t bits );
void MQTT2_set_client(char *ip_adrress);


#endif // _JITSUPPORT_APP_MAIN_H