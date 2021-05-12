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

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#include "jitsupport_app_main.h"

#include "hardware/wifictl.h"
#include "hardware/callback.h"
#include "hardware/display.h"
#include "hardware/powermgm.h"
#include "hardware/motor.h"


//***************  MQTT ******************//

WiFiClient espClient2;
PubSubClient client2(espClient2);
EventGroupHandle_t xMqttCtrlEvent=NULL;

//*************** TASKS *****************//

TaskHandle_t _mqttStatus_Task=NULL;
TaskHandle_t _mqttCtrl_Task=NULL;
TaskHandle_t _mqtt_init_task=NULL;
TaskHandle_t _mqttPublish_Task=NULL;

//*************** QUEUE *****************//

QueueHandle_t xMQTT_Publish_Queue;



void Mqtt_init_task( void * pvParameters );
void Mqtt_status_task( void * pvParameters );
void Mqtt_Ctrl_task( void * pvParameters );
void Mqtt_Publish_task( void * pvParameters );

callback_t *mqttctrl_callback = NULL;
//portMUX_TYPE DRAM_ATTR mqttcrlMux = portMUX_INITIALIZER_UNLOCKED;


//***************PROTOTIPOS*****************//

void MQTT2_callback(char* topic, byte* message, unsigned int length);
bool jit_mqtt_powermgm_loop_cb(EventBits_t event, void *arg );



void mqqtctrl_set_event( EventBits_t bits );
void mqqtctrl_clear_event( EventBits_t bits);
EventBits_t mqqtctrl_get_event( EventBits_t bits);
bool mqqtctrl_send_event_cb( EventBits_t event );
void MQTT2_set_client(char *ip_adrress);

//**************Variaveis**********************//

char receive_topic[15];
char update_topic[15];
char area_topic[20];  
char ip_client[15];

void mqttctrl_setup()
{

    client2.setServer(MQTT_SERVER, MQTT_PORT);
    client2.setKeepAlive(120);
    client2.setCallback(MQTT2_callback);
    
    xMQTT_Publish_Queue = xQueueCreate(5,int(MQTT_PUBLISH_PAYLOAD_SIZE));
    if(xMQTT_Publish_Queue){
      log_e("Error to create MQTT_Publish_Queue");
    }
  
    
    xMqttCtrlEvent=xEventGroupCreate();
    mqqtctrl_clear_event(MQTT_START_CONNECTION | MQTT_DISCONNECTED_FLAG|MQTT_CONNECTED_FLAG); 

  
   //---- Task para Monitoração da Conexão MQTT
     xTaskCreatePinnedToCore( Mqtt_init_task,                              /* Function to implement the task */
                             "Mqtt Init task",                             /* Name of the task */
                              2000,                                          /* Stack  Last measure 1368 */
                              NULL,                                          /* Task input parameter */
                              0,                                             /* Priority of the task */
                              &_mqtt_init_task,                             /* Task handle. */
                              0);
  
  //---- Task para Monitoração da Conexão MQTT
     xTaskCreatePinnedToCore( Mqtt_status_task,                              /* Function to implement the task */
                             "Mqtt Status task",                             /* Name of the task */
                              3000,                                          /* Stack  Last measure 1368 */
                              NULL,                                          /* Task input parameter */
                              1,                                             /* Priority of the task */
                              &_mqttStatus_Task,                             /* Task handle. */
                              0);
    vTaskSuspend(_mqttStatus_Task);

  //---- Task para Reestabelecimento da Conexão MQTT
     xTaskCreatePinnedToCore( Mqtt_Ctrl_task,                               /* Function to implement the task */
                             "Mqtt Contrl",                                 /* Name of the task */
                              2000,                                         /* Stack size in words */
                              NULL,                                         /* Task input parameter */
                              2,                                            /* Priority of the task */
                              &_mqttCtrl_Task,                              /* Task handle. */
                              0);
   //vTaskSuspend(_mqttCtrl_Task);


     xTaskCreatePinnedToCore( Mqtt_Publish_task,                               /* Function to implement the task */
                             "Mqtt Publish",                                 /* Name of the task */
                              2000,                                         /* Stack size in words */
                              NULL,                                         /* Task input parameter */
                              1,                                            /* Priority of the task */
                              &_mqttPublish_Task,                              /* Task handle. */
                              0);



  powermgm_register_loop_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, jit_mqtt_powermgm_loop_cb, "jitsupport app loop" );

}

void Mqtt_init_task( void * pvParameters )
{
  EventBits_t xBits;

    while(1)
    {     
          xBits=xEventGroupWaitBits(xMqttCtrlEvent,MQTT_START_CONNECTION, pdTRUE,pdTRUE,portMAX_DELAY);
          vTaskResume(_mqttStatus_Task);
          vTaskDelete(NULL);
    }
}


void MQTT2_callback(char* topic, byte* message, unsigned int length)
{
  log_i("Message arrived on topic: ");
  log_i("%s", topic);
  
  MQTT_callback(topic, message,length);

  
}



void Mqtt_Publish_task( void * pvParameters )
{
  
    char msg[MQTT_PUBLISH_PAYLOAD_SIZE]; 

    while(1)
    {     
          if(client2.state()==MQTT_CONNECTED)
          {  
            
            if(xQueueReceive(xMQTT_Publish_Queue,&msg,portMAX_DELAY)==pdTRUE)
            {
              log_i("publicação realizada"); 
              client2.publish(update_topic, msg);
            }
   
          }
          else
          {

            vTaskDelay(CHECK_MQTT_CONNECTION_MILLI_SECONDS / portTICK_PERIOD_MS );
          }
    }
}


void MQTT2_publish(char *atualizartopico, char *payload)
{
  
  // Inserindo Mensagem numa fila para envio 
  xQueueSend(xMQTT_Publish_Queue,payload,portMAX_DELAY);
  
}


bool jit_mqtt_powermgm_loop_cb(EventBits_t event, void *arg )
{
    switch(event) {
        case POWERMGM_STANDBY:  
            break;
        case POWERMGM_WAKEUP:
                // Alterar a frequência de verificação de conexão com o broker 
            break;
        case POWERMGM_SILENCE_WAKEUP:
                // Alterar a frequência de verificação de conexão com o broker                         
            break;
        }

client2.loop();  
return( true );
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
  bool subscribe_flag1,subscribe_flag2=false;

    while (1) {

      switch(client2.state()){    
        
        case(MQTT_CONNECTED):
            
          log_i("MQTT_CONNECTED"); 

          if(once_flag==0)
          {
            log_i("%s",receive_topic);
            log_i("%s",update_topic);
            log_i("%s",area_topic);


            subscribe_flag1=client2.subscribe(receive_topic,1);
            subscribe_flag2 =client2.subscribe(update_topic,1);
            subscribe_flag2 =client2.subscribe(area_topic,1);

            if((subscribe_flag1==true)&&(subscribe_flag2==true))
            {
                log_i(" Success while Subscribe"); 
                once_flag=1;
            }
            else
            {
                log_i(" Problem while Subscribe");
                once_flag=0;

            }      
          }

          mqqtctrl_set_event(MQTT_CONNECTED_FLAG);
          mqqtctrl_send_event_cb(MQTT_CONNECTED_FLAG);   
               
        break;

        case(MQTT_CONNECT_FAILED):
                     
           log_i("MQTT Conection Failed...");
            mqqtctrl_set_event(MQTT_DISCONNECTED_FLAG);
            mqqtctrl_send_event_cb(MQTT_DISCONNECTED_FLAG); 
        break;

        case(MQTT_DISCONNECTED):
            
            log_i("MQTT Disconnected... ");
            mqqtctrl_set_event(MQTT_DISCONNECTED_FLAG);
            mqqtctrl_send_event_cb(MQTT_DISCONNECTED_FLAG);          

        break;

        case(MQTT_CONNECTION_TIMEOUT):

          log_i("MQTT timeout..."); 
          mqqtctrl_set_event(MQTT_DISCONNECTED_FLAG);
          mqqtctrl_send_event_cb(MQTT_DISCONNECTED_FLAG);           

        break;

        case(MQTT_CONNECTION_LOST):
     
          log_i("MQTT lost connection... ");
          mqqtctrl_set_event(MQTT_DISCONNECTED_FLAG);
          mqqtctrl_send_event_cb(MQTT_DISCONNECTED_FLAG);
        break;     

        default:
         log_i("MQTT NOT TREATED STATE:");  
         log_i("%s",client2.state());   
      }
       
        
        
        vTaskDelay(CHECK_MQTT_CONNECTION_MILLI_SECONDS / portTICK_PERIOD_MS );
        
    }
    
  }
void MQTT2_set_client(char *client_name)
{
   
  strcpy(ip_client,client_name);
  log_i("topico cadastrado %s",ip_client );

}


void MQTT2_set_subscribe_topics(char *topico_receber, char * topico_atualizar, char *topico_area )
{

  strcpy(receive_topic,topico_receber);
  strcpy(update_topic,topico_atualizar);
  strcpy(area_topic,topico_area);

  log_i("topico cadastrado %s",receive_topic);
  log_i("topico cadastrado %s",update_topic);
  log_i("topico cadastrado %s",area_topic);
  
}


void Mqtt_Ctrl_task(void * pvParameters)
{  
    EventBits_t xBits;

    static bool first_connection_flag=true;

    while(1)
    {     
          xBits=xEventGroupWaitBits(xMqttCtrlEvent,MQTT_DISCONNECTED_FLAG, pdTRUE,pdTRUE,portMAX_DELAY); 
          log_i("MQTT reconnection...");
          log_i("%s",ip_client);
          
          
          if(first_connection_flag)
          {
            // ----- ANTI RAJADA DE MENSAGENS NO BOOT -----

            if (client2.connect(ip_client, MQTT_USER, MQTT_PSSWD,"status_team", 1, 1,"off", 1)){
              
              // OK CONNECTED CLEAN SESSION TRUE 
              client2.disconnect();
              
              if(client2.connect(ip_client, MQTT_USER, MQTT_PSSWD,"status_team", 1, 1,"off", MQTT_CLEAN_SESSION)){
                
                // OK NOW CONNECTED CLEAN SESSION FALSE
                first_connection_flag=false;   
                log_i("MQQT Connected");  
              } 
              else  log_i("Failed !");
            } 

          }
          else
          {

            // IF YOU ARE HERE  YOU ARE FACING A MQTT RECONNECTION !!
            if (client2.connect(ip_client, MQTT_USER, MQTT_PSSWD,"status_team", 1, 1,"off", MQTT_CLEAN_SESSION))log_i("MQQT Connected");  
            else  log_i("Failed !"); 
          }

        
    }

}
   
void mqqtctrl_set_event( EventBits_t bits ){
   // portENTER_CRITICAL(&mqttcrlMux);
    xEventGroupSetBits( xMqttCtrlEvent, bits);
    //portEXIT_CRITICAL(&mqttcrlMux);
}

void mqqtctrl_clear_event( EventBits_t bits){
    //portENTER_CRITICAL(&mqttcrlMux);
    xEventGroupClearBits( xMqttCtrlEvent, bits);
    //portEXIT_CRITICAL(&mqttcrlMux);
}

EventBits_t mqqtctrl_get_event( EventBits_t bits){
    //portENTER_CRITICAL(&mqttcrlMux);
    EventBits_t temp = xEventGroupGetBits( xMqttCtrlEvent ) & bits;
    //portEXIT_CRITICAL(&mqttcrlMux);
    return( temp );
}

bool mqqtctrl_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    if ( mqttctrl_callback == NULL ) {
        mqttctrl_callback = callback_init( "mqttctrl" );
        if ( mqttctrl_callback == NULL ) {
            log_e("mqttctrl callback alloc failed");
            while(true);
        }
    }    
    return(callback_register( mqttctrl_callback, event, callback_func, id ));
}

bool mqqtctrl_send_event_cb( EventBits_t event ) {
    return( callback_send( mqttctrl_callback, event, (void*)NULL ) );
}



void cmd_reset() {

TTGOClass *ttgo = TTGOClass::getWatch();
log_i("System reboot by user");
motor_vibe(20);
delay(20);
display_standby();
ttgo->stopLvglTick();
SPIFFS.end();
log_i("SPIFFS unmounted!");
delay(500);
ESP.restart();    

}
