/****************************************************************************
 *   Aug 21 17:26:00 2020
 *   Copyright  2020  Chris McNamee
 *   Email: chris.mcna@gmail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include <TTGO.h>
#include "Arduino.h"

#include "jitsupport_mqtt.h"
#include "hardware/powermgm.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#include "hardware/wifictl.h"     

/*************MQTT*******************/


WiFiClient espClient2;
PubSubClient client2(espClient2);

TaskHandle_t _mqttStatus_Task, _mqttCtrl_Task;


void Mqtt_status_task( void * pvParameters );
void Mqtt_Ctrl_task( void * pvParameters );

EventGroupHandle_t xMqttCtrlEvent;

void MQTT2_callback(char* topic, byte* message, unsigned int length);

void mqttctrl_setup()
{

    client2.setServer(MQTT_SERVER, MQTT_PORT);
    client2.setKeepAlive(120);
    client2.setCallback(MQTT2_callback);

    xMqttCtrlEvent=xEventGroupCreate(); 

  //---- Task para Monitoração da Conexão MQTT
     xTaskCreatePinnedToCore( Mqtt_status_task,                              /* Function to implement the task */
                             "Mqtt Status task",                             /* Name of the task */
                              3000,                                          /* Stack size in words */
                              NULL,                                          /* Task input parameter */
                              1,                                             /* Priority of the task */
                              &_mqttStatus_Task,                             /* Task handle. */
                              0 );


  //---- Task para Reestabelecimento da Conexão MQTT
     xTaskCreatePinnedToCore( Mqtt_Ctrl_task,                               /* Function to implement the task */
                             "Mqtt Contrl",                                 /* Name of the task */
                              configMINIMAL_STACK_SIZE + 1024,              /* Stack size in words */
                              NULL,                                         /* Task input parameter */
                              1,                                            /* Priority of the task */
                              &_mqttCtrl_Task,                              /* Task handle. */
                              0 );

}


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

void Mqtt_status_task(void * pvParameters ){

  static uint8_t once_flag=0;
  
    while (1) {

      switch(client2.state()){    
        
        case(MQTT_CONNECTED):

            log_i("oi estou conectado");
            if(once_flag==0)
            {
              log_i("oi ME INSCREVI");
              client2.subscribe("hohoho\teste",1);
              client2.subscribe("hohoho\teste2",1);
              once_flag=1;
            }
          
        break;

        case(MQTT_CONNECT_FAILED):
                     
           log_i("MQTT Conection Failed...");
           xEventGroupSetBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG);   
        break;

        case(MQTT_DISCONNECTED):
            
            log_i("MQTT Disconnected... ");
            xEventGroupSetBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG);           

        break;

        case(MQTT_CONNECTION_TIMEOUT):

            log_i("MQTT timeout..."); 
            xEventGroupSetBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG);           

        break;

        case(MQTT_CONNECTION_LOST):
     
            log_i("MQTT lost connection... ");
            xEventGroupSetBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG);    
    
        break;     

        default:
         log_i("MQTT NOT TREATED STATE:");  
         log_i("%s",client2.state());   
      }
        vTaskDelay(CHECK_MQTT_CONNECTION_MILLI_SECONDS / portTICK_PERIOD_MS );
    } 
  }


void Mqtt_Ctrl_task(void * pvParameters)
{  
    EventBits_t xBits;
    UBaseType_t uxHighWaterMark;



    while(1)
    {     
          xBits=xEventGroupWaitBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG,pdTRUE,pdTRUE,portMAX_DELAY); 
          uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
          //log_i("STACK USAGE %s", uxHighWaterMark);


          log_i("MQTT reconnection...");
          if (client2.connect("Marreco", MQTT_USER, MQTT_PSSWD,"status_team/16", 1, 1,"oi", MQTT_CLEAN_SESSION))
          {
            log_i("MQQT Connected");    

            // client.connect(ip_address, MQTT_USER, MQTT_PSSWD,"status_team/16", 1, 1,"oi", MQTT_CLEAN_SESSION)
          }
          else  log_i("Failed !");     
      }

}
   

void MQTT2_callback(char* topic, byte* message, unsigned int length)
{

  log_i("Message arrived on topic: ");
  log_i("%s", topic);


}