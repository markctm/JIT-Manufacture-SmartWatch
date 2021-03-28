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

#include "jitsupport_app.h"
#include "jitsupport_app_main.h"
#include "hardware/powermgm.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "hardware/motor.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define USE_SERIAL Serial

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>


typedef struct{
  const char*  ticket_id;
  const char*  workstation;
  const char*  risk;
  const char*  call_time;
  const char*  description;
  const char*  counter;
  const char*  status;  
} Ticket;

Ticket myticket;



/***************Protótipos*************/


void MQTT_callback(char* topic, byte* message, unsigned int length);
static void removefromArray(lv_obj_t *obj, lv_event_t event);
void getWatchUser();
void sendRequest(lv_obj_t *obj, lv_event_t event);
static void toggle_Cards_Off();
static void toggle_Cards_On();
static void btn2_handler(lv_obj_t *obj, lv_event_t event);
static void btn1_handler(lv_obj_t *obj, lv_event_t event);
static void pub_mqtt(lv_obj_t *obj, lv_event_t event);
static void exit_jitsupport_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
void printCard(uint8_t posic);
void mqtt_reconnect();
void printCard2(Ticket *myticket);
/*************MQTT*******************/

const char* mqtt_server = MQTT_SERVER;
WiFiClient espClient;
PubSubClient client(espClient);
TaskHandle_t _mqttCheck_Task;
void Check_MQTT_Task( void * pvParameters );

uint8_t idTeam;
String NomeTopicoReceber="";
String NomeTopicoAtualizar="";
char nometopico[15];
char atualizartopico[15];
char payload[100];
char nomepeq[10]= "a";
char nomefull[100];


/************APIS ********************************/

char* GetWatchById_host = "http://10.57.16.40/JITAPI/Smartwatch/GetByIP/";
char GetWatchById_Url[50] = {0};
char ip_address[15];

String meuip;
HTTPClient http;
StaticJsonDocument<200> result;
StaticJsonDocument<200> userjson;
int httpCode = 0;
TTGOClass *twatch;
bool pegueiUser = false;


bool jitsupport_powermgm_event_cb( EventBits_t event, void *arg );

long jitsupport_milliseconds = 0;   //NP
time_t jitprevtime;  //NP


//-------------------------

lv_obj_t *jitsupport_app_main_tile = NULL;
lv_style_t jitsupport_app_main_style;
lv_style_t jitsupport_app_main_jitsupportstyle;
lv_obj_t *jitsupport_app_main_jitsupportlabel = NULL;
LV_IMG_DECLARE(exit_32px);
LV_FONT_DECLARE(Ubuntu_72px);

uint8_t counter = 0;
uint8_t atual = 0;
char chamados[50][50];
char bufatual [2];
char buftotal [2];
uint8_t num_tickets = 10;

static lv_style_t stl_view;
static lv_style_t stl_bg_card;
static lv_style_t stl_btn1;
static lv_style_t stl_btn2;
static lv_style_t stl_topline;
static lv_style_t stl_transp;

static lv_obj_t * bg_card;
static lv_obj_t * lbl_workstation;
static lv_obj_t * lbl_risk;
static lv_obj_t * lbl_calltime;
static lv_obj_t * lbl_status;
static lv_obj_t * lbl_description;
static lv_obj_t * lbl_user;

static lv_obj_t * btn1;
static lv_obj_t * btn2;
static lv_obj_t * lbl_actualcard;
static lv_obj_t * lbl_totalcard;
static lv_obj_t * lbl_count_separator;

static lv_obj_t * lbl_IP;
static lv_obj_t * lbl_RSSI;

static lv_obj_t * btn_back;
static lv_obj_t * btn_next;
static lv_obj_t * btn_exit;
static lv_obj_t * btn_config;


//* *********************************************************

static void exit_jitsupport_app_main_event_cb( lv_obj_t * obj, lv_event_t event );

void jitsupport_app_task( lv_task_t * task );


void jitsupport_app_main_setup( uint32_t tile_num ) {

    jitsupport_app_main_tile = mainbar_get_tile_obj( tile_num );
    lv_style_copy( &jitsupport_app_main_style, mainbar_get_style());
    lv_style_set_bg_color(&jitsupport_app_main_style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_copy( &jitsupport_app_main_jitsupportstyle, &jitsupport_app_main_style);
    
    lv_style_init(&stl_view);
    lv_style_set_radius(&stl_view, LV_OBJ_PART_MAIN, 12);
    lv_style_set_bg_color(&stl_view, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_bg_opa(&stl_view, LV_OBJ_PART_MAIN, LV_OPA_COVER);
    lv_style_set_border_width(&stl_view, LV_OBJ_PART_MAIN, 0);

    lv_obj_t * jitsupport_cont = mainbar_obj_create( jitsupport_app_main_tile );
    lv_obj_set_size( jitsupport_cont, LV_HOR_RES , LV_VER_RES );
    lv_obj_add_style( jitsupport_cont, LV_OBJ_PART_MAIN, &stl_view );
    lv_obj_align( jitsupport_cont, jitsupport_app_main_tile, LV_ALIGN_CENTER, 0, 0 );

  

    //***************************
     //TOP HORIZONTAL LINE
    static lv_point_t line_points[] = { {10, 0}, {230, 0} };
    lv_obj_t *line1;
    line1 = lv_line_create(jitsupport_cont, NULL);
    lv_line_set_points(line1, line_points, 2);     /*Set the points*/
    lv_obj_add_style(line1, LV_OBJ_PART_MAIN, &stl_topline);
    lv_obj_align(line1, NULL, LV_ALIGN_IN_TOP_MID, 0, 35);
    // CARD BACKGROUND STYLE
    lv_style_init(&stl_bg_card);
    lv_style_set_bg_color(&stl_bg_card, LV_OBJ_PART_MAIN, LV_COLOR_YELLOW);


  // BUTTON 1 STYLE
    lv_style_init(&stl_btn1);
    lv_style_set_bg_color(&stl_btn1, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_set_text_color(&stl_btn1, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_border_color(&stl_btn1, LV_STATE_DEFAULT, LV_COLOR_GREEN);
 

  // BUTTON 2 STYLE
    lv_style_init(&stl_btn2);
    lv_style_set_bg_color(&stl_btn2, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x1f, 0x1f));
    lv_style_set_text_color(&stl_btn2, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    
    // TRANSP STYLE
    lv_style_init(&stl_transp);
    
    lv_style_set_bg_opa(&stl_transp,LV_STATE_DEFAULT,LV_OPA_TRANSP);
    lv_style_set_border_opa(&stl_transp,LV_STATE_DEFAULT,LV_OPA_TRANSP);
    lv_style_set_border_opa(&stl_transp,LV_BTN_STATE_RELEASED,LV_OPA_TRANSP);
    lv_style_set_border_opa(&stl_transp,LV_BTN_STATE_PRESSED,LV_OPA_TRANSP);
    lv_style_set_border_opa(&stl_transp,LV_STATE_FOCUSED,LV_OPA_TRANSP);
    lv_style_set_border_opa(&stl_transp,LV_STATE_PRESSED,LV_OPA_TRANSP);
    // lv_style_set_border_opa(&stl_transp,LV_BTN_STATE_ACTIVE,LV_OPA_TRANSP);
    
    // lv_style_set_border_width(&stl_transp,LV_BTN_STATE_ACTIVE,0);
    lv_style_set_border_width(&stl_transp,LV_BTN_STATE_PRESSED,0);
    // lv_style_set_border_width(&stl_transp,LV_BTN_STATE_ACTIVE,0);
    lv_style_set_border_width(&stl_transp,LV_BTN_STATE_CHECKED_RELEASED,0);
     lv_style_set_border_width(&stl_transp,LV_BTN_STATE_DISABLED,0);
     lv_style_set_border_width(&stl_transp,LV_BTN_STATE_CHECKED_PRESSED,0);
      lv_style_set_border_width(&stl_transp,LV_BTN_STATE_RELEASED,0);
      lv_style_set_border_width(&stl_transp,_LV_BTN_STATE_LAST,0);
    
    // LABEL NO CARD
    lv_obj_t * lbl_nocard;
    lbl_nocard = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_nocard, "Sem chamados.");
    lv_obj_align(lbl_nocard, jitsupport_cont, LV_ALIGN_CENTER, 0, 0);

    lbl_IP = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_IP, "0.0.0.0");
    lv_obj_align(lbl_IP, jitsupport_cont, LV_ALIGN_CENTER, 0, 20);

    
    lbl_RSSI = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_RSSI, "-0");
    lv_obj_align(lbl_RSSI, jitsupport_cont, LV_ALIGN_IN_LEFT_MID, 5, 40);
 
    
    bg_card = lv_obj_create(jitsupport_cont, NULL);
    lv_obj_set_pos(bg_card, 10, 40);
    lv_obj_set_width(bg_card,220);
    lv_obj_set_height(bg_card,166);
     lv_obj_add_style(bg_card, LV_OBJ_PART_MAIN, &stl_bg_card);


    

    // WORKSTATION LABEL
     
    lbl_workstation = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_workstation, "WORKSTATION");
    lv_obj_align(lbl_workstation, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

   // RISK LABEL
    
    lbl_risk = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_risk, "RISCO");
    lv_obj_align(lbl_risk, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 40);

    
    // CALL TIME LABEL
    
    lbl_status = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_status, "Status");
    lv_obj_align(lbl_status, NULL, LV_ALIGN_IN_TOP_RIGHT, -40, 100);
    // CALL TIME LABEL
    
    lbl_calltime = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_calltime, "12: 00");
    lv_obj_align(lbl_calltime, NULL, LV_ALIGN_IN_TOP_RIGHT, -10, 40);

    // DESCRIPTION LABEL
    
    lbl_description = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_description, "DESCRIPTION");
    lv_obj_align(lbl_description, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 70);

    lbl_user = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_user, "");
    lv_obj_align(lbl_user, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 100);


    // BUTTON 1
    btn1 = lv_btn_create(bg_card, NULL);
    lv_obj_set_pos(btn1, 10, 125);
    lv_obj_set_width(btn1,95);
    lv_obj_set_height(btn1,35);
    lv_obj_set_event_cb(btn1, sendRequest);
    
    // BUTTON 1 LABEL
    lv_obj_t * lbl_btn1;
    lbl_btn1 = lv_label_create(btn1, NULL);
    lv_label_set_text(lbl_btn1, "Confirmar");
    lv_label_set_text(lbl_btn1, LV_SYMBOL_OK);
    lv_obj_add_style(btn1, LV_OBJ_PART_MAIN, &stl_btn1);


    // BUTTON 2
    btn2 = lv_btn_create(bg_card, NULL);
    lv_obj_set_pos(btn2, 120, 125);
    lv_obj_set_width(btn2,95);
    lv_obj_set_height(btn2,35);
    lv_obj_set_event_cb(btn2, removefromArray);
    
    // BUTTON 2 LABEL
    lv_obj_t * lbl_btn2;
    lbl_btn2 = lv_label_create(btn2, NULL);
    lv_label_set_text(lbl_btn2, "Recusar");
    lv_label_set_text(lbl_btn2, LV_SYMBOL_CLOSE);
    lv_obj_add_style(btn2, LV_OBJ_PART_MAIN, &stl_btn2);


    
    lbl_count_separator = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_count_separator, "/");
    lv_obj_align(lbl_count_separator, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -5);
    
    
    lbl_actualcard = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_actualcard, "0");
    lv_obj_align(lbl_actualcard, lbl_count_separator, LV_ALIGN_IN_TOP_MID, -10, 0);

    
    lbl_totalcard = lv_label_create(jitsupport_cont, NULL);
    lv_label_set_text(lbl_totalcard, "0");
    lv_obj_align(lbl_totalcard, lbl_count_separator, LV_ALIGN_IN_TOP_MID, 10, 0);

    btn_back= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_back,80);
    lv_obj_set_height(btn_back,35);
    lv_obj_align(btn_back, jitsupport_cont, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(btn_back, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_back;
    lbl_btn_back = lv_label_create(btn_back, NULL);
    lv_label_set_text(lbl_btn_back, LV_SYMBOL_LEFT);
    
    lv_obj_set_event_cb(btn_back, btn1_handler);

    btn_next= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_next,80);
    lv_obj_set_height(btn_next,35);
    lv_obj_align(btn_next, jitsupport_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_style(btn_next, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_next;
    lbl_btn_next = lv_label_create(btn_next, NULL);
    lv_label_set_text(lbl_btn_next, LV_SYMBOL_RIGHT);
    lv_obj_set_event_cb(btn_next, btn2_handler);
    
    lv_obj_set_hidden(bg_card, true);

    
    btn_exit= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_exit,80);
    lv_obj_set_height(btn_exit,35);
    lv_obj_align(btn_exit, jitsupport_cont, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_add_style(btn_exit, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_exit;
    lbl_btn_exit = lv_label_create(btn_exit, NULL);
    lv_label_set_text(lbl_btn_exit, LV_SYMBOL_HOME);
    lv_obj_set_event_cb(btn_exit, exit_jitsupport_app_main_event_cb);

    btn_config= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_config,80);
    lv_obj_set_height(btn_config,35);
    lv_obj_align(btn_config, jitsupport_cont, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
    lv_obj_add_style(btn_config, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_config;
    lbl_btn_config = lv_label_create(btn_config, NULL);
    lv_label_set_text(lbl_btn_config, LV_SYMBOL_SETTINGS);
    // lv_obj_set_event_cb(btn_config, pub_mqtt);
    
    client.setServer(mqtt_server, 1883);
    client.setCallback(MQTT_callback);
   
  //---- Task para Monitoração da Conexão MQTT
     xTaskCreatePinnedToCore( Check_MQTT_Task,     /* Function to implement the task */
                             "Mqtt CheckTask",   /* Name of the task */
                              3000,             /* Stack size in words */
                              NULL,             /* Task input parameter */
                              1,                /* Priority of the task */
                              &_mqttCheck_Task,   /* Task handle. */
                              0 );
    //vTaskSuspend( _mqttCheck_Task );


   powermgm_register_loop_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, jitsupport_powermgm_event_cb, "jitsupport app loop" );

}


//-------------------------APP-EVENT ACTIONS-------------------------


static void exit_jitsupport_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_maintile( LV_ANIM_OFF );
                                        break;
    }
}



bool jitsupport_powermgm_event_cb( EventBits_t event, void *arg ) {

    switch(event) {
        case POWERMGM_STANDBY:  
              
                if(wifi_connected) vTaskResume( _mqttCheck_Task );
            break;
        case POWERMGM_WAKEUP:

                if(wifi_connected) vTaskResume( _mqttCheck_Task );
            break;
        case POWERMGM_SILENCE_WAKEUP:
                
               if(wifi_connected) vTaskResume( _mqttCheck_Task );             
                    
            break;
    }

  client.loop();;
return( true );
}



//-------------------------MQTT FUNCTIONS------------------------


void MQTT_callback(char* topic, byte* message, unsigned int length) {
  
  log_i("Message arrived on topic: ");
  log_i("%s", topic);

  //------- JSON DOC------------
  StaticJsonDocument<200> result;
  deserializeJson(result, message);
  JsonObject object = result.as<JsonObject>();
  
  if(object.isNull()==false)
  {
      // OK VALID JSON ! 

      if (String(topic) == NomeTopicoReceber) {
            
            log_i("****VALID JSON MESSAGE *****");                
            myticket.ticket_id= result["id"].as<const char*>();
            myticket.workstation= result["workstation"].as<const char*>();
            myticket.risk= result["calltime"].as<const char*>();
            myticket.call_time=result["risk"].as<const char*>();
            myticket.description= result["description"].as<const char*>();
            myticket.status="Open";
            
            log_i("\n Ticket ID: %s \n Workstation ID: %s \n Risk ID: %s \n Calltime ID: %s \n Description: %s\n",myticket.ticket_id,myticket.workstation,myticket.risk,myticket.call_time,myticket.description);
          
            printCard2(&myticket);

            toggle_Cards_On();



    /*     if(!(counter==num_tickets)){

              strcpy(chamados[(counter*num_tickets)+0],myticket.workstation);
              strcpy(chamados[(counter*num_tickets)+1],myticket.risk);
              
              log_i("TESTE DO RISCO");

              
              log_i("Bool 1:");
              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"1")==0);
              log_i("Bool 2:");
              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"0")==0);
              log_i("Bool 3:");
              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"1"));
              log_i("Bool 4:");
               

              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"0"));
              if(strcmp(chamados[(counter*num_tickets)+1],"1")==0){
                sprintf(chamados[(counter*num_tickets)+1],"Rodando");
                log_i("Linha rodando");
              }
              else if(strcmp(chamados[(counter*num_tickets)+1],"0")==0){
                log_i("Linha parada");
                sprintf(chamados[(counter*num_tickets)+1],"Parada");
              }

              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"1"));
              log_i("%d",strcmp(chamados[(counter*num_tickets)+1],"0"));
              strcpy(chamados[(counter*num_tickets)+2],myticket.call_time);
              strcpy(chamados[(counter*num_tickets)+3],myticket.description); 
              strcpy(chamados[(counter*num_tickets)+4],myticket.ticket_id);
              strcpy(chamados[(counter*num_tickets)+5],"Open");
              strcpy(chamados[(counter*num_tickets)+6],"");
              counter = counter +1;
              atual = counter;
              printCard(counter-1);          
              toggle_Cards_On();
                          
                  // twatch->motor->onec();
            }*/
       } // if String(topic) == NomeTopicoReceber)

      else if(String(topic) == NomeTopicoAtualizar){
      
            log_i("ATUALIZAR CHAMADO ************");
            
            
            // if(BLisOn){          
            //   }
            //   else{
            //     ttgo->openBL();
            //     BLisOn = true;
            //   }
            // ttgo->motor->onec();

            char idbuscado [3];
            char userbuscado [20];
            char statusbuscado [20];
            
            strcpy(idbuscado,result["TicketId"]);
            log_i("ID RECEBIDA:");
            log_i("%s",idbuscado);
            
            
            log_i("USUARIO RECEBIDO");
            
            strcpy(userbuscado,result["UserName"]);
            log_i("%s",userbuscado);
            
            for(int z=0;z<10;z++){
              
              log_i("%c",userbuscado[z]);

              if(isWhitespace(userbuscado[z])) break;            
              else nomepeq[z]=userbuscado[z];         
            }

            log_i("USUARIO trim");
            log_i("%s",nomepeq);
            String stats = result["Status"];
            stats.toCharArray(statusbuscado,20);
            log_i("STATUS RECEBIDO");
            log_i("%s",stats);
            log_i("%s",statusbuscado);
        

     
            log_i("Counter: %d",counter);

            for (int i = 4; i <= ((counter*num_tickets)-3); i=i+num_tickets)
            {
              log_i("i: ");
              log_i("%d",i);

              log_i("Ids: ");
              log_i("%s",chamados[i]);

              if(strcmp(chamados[i], idbuscado)==0){

                log_i("Achei meu chamado buscado");
                strcpy(chamados[i+1],statusbuscado);
                strcpy(chamados[i+2],nomepeq);
                log_i("%s",chamados[i-4]);
                log_i("%s",chamados[i-3]);
                log_i("%s",chamados[i-2]);
                log_i("%s",chamados[i-1]);
                log_i("%s",chamados[i]);
                log_i("%s",chamados[i+1]);
                log_i("%s",chamados[i+2]);
                atual=((i-4)/num_tickets)+1;
                printCard(atual-1);

                if(strcmp(statusbuscado,"Done")==0){
                  log_i("SIM, O STATUS EH DONE!!!!!!!");
                  removefromArray(btn2,LV_EVENT_CLICKED);
                }
                      
              break;
              }
            }
        }                    // twatch->motor->onec();         
    }
    else
    {
        //NOT VALID JSON MESSAGE !! 
        log_i("Not a Valid Json Message");
    }
 
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

void Check_MQTT_Task(void * pvParameters ){
  
    while (1) {

      switch(client.state()){ 
        
        case(MQTT_CONNECTED):
                          
           vTaskSuspend( _mqttCheck_Task );

        break;
        case(MQTT_CONNECT_FAILED):
                     
           log_i("MQTT Conection Failed...");
           mqtt_reconnect();

        break;
        case(MQTT_DISCONNECTED):
            
            log_i("MQTT Disconnected... ");           
            client.setCallback(MQTT_callback);
            client.connect("ESP32CLIENT");

        break;

        case(MQTT_CONNECTION_TIMEOUT):

            log_i("MQTT timeout... ");         
            mqtt_reconnect();

        break;

        case(MQTT_CONNECTION_LOST):
     
            log_i("MQTT lost connection... ");  
            mqtt_reconnect();

        break;     

        default:
         log_i("MQTT NOT TREATED STATE:");  
          log_i("%s",client.state());   
      }
        vTaskDelay(CHECK_MQTT_CONNECTION_MILLI_SECONDS/ portTICK_PERIOD_MS );
    } 
  }


static void pub_mqtt(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
        log_i("Mqtt publish \n");
      client.publish("esp32/output", "***** sending test message******");

    } 
    
}


void mqtt_reconnect()
{  
    // Attempt to connect
    if(!client.connected()){
     
        log_i("MQTT reconnection...");

        if (client.connect(ip_address)){
          log_i("MQQT Connected");

          if(!(pegueiUser)){
              getWatchUser();
          } 

          client.subscribe(nometopico);
          client.subscribe(atualizartopico);
          client.subscribe("ttwatch");
        }
        else  log_i("Failed !"); 
    
    }
}




//----------------APP FUNCTIONS---------------------------- 



void getWatchUser(){

    pegueiUser = true;
    meuip = WiFi.localIP().toString();
    meuip.toCharArray(ip_address,15);

    log_i("MEU IP É O SEGUINTE: %s",ip_address);
    lv_label_set_text(lbl_IP,ip_address);

    strcat(GetWatchById_Url, GetWatchById_host);
    strcat(GetWatchById_Url, ip_address);
    
    log_i("ENDEREÇO DE API:");
    log_i("%s",GetWatchById_Url); 
    int err = 0;
  
        

#ifdef   NO_HTTP_RESPONSE  
      String numerotopico = "11";            
      
      NomeTopicoReceber = "receber/" + numerotopico;
      NomeTopicoAtualizar = "atualizar/" + numerotopico;
      
      log_i("Nome Topico Receber:");
      log_i("%s",NomeTopicoReceber);
      log_i("Nome Topico Atualizar:");            
      log_i("%s",NomeTopicoAtualizar);
      
      NomeTopicoReceber.toCharArray(nometopico,15);
      NomeTopicoAtualizar.toCharArray(atualizartopico,15);

      client.subscribe(nometopico);
      client.subscribe(atualizartopico);

#endif       

        
        http.begin(GetWatchById_Url); //HTTP
      //  http.begin("http://192.168.0.8:3000/watch");
        httpCode = http.GET();
        log_i("%d",httpCode);
        
        if(httpCode > 0) {

          
            if(httpCode == HTTP_CODE_OK) {
                
                String payload = http.getString();
                deserializeJson(result, payload);

                idTeam = result["teamId"];
                log_i("ID do time getwatch:");
                log_i("%d",idTeam);
                
              
                String numerotopico = String(idTeam);       
                
                NomeTopicoReceber = "receber/" + numerotopico;
                NomeTopicoAtualizar = "atualizar/" + numerotopico;
                
                log_i("Nome Topico Receber:");
                log_i("%s",NomeTopicoReceber);
                log_i("Nome Topico Atualizar:");            
                log_i("%s",NomeTopicoAtualizar);
                
                NomeTopicoReceber.toCharArray(nometopico,15);
                NomeTopicoAtualizar.toCharArray(atualizartopico,15);

                client.subscribe(nometopico);
                client.subscribe(atualizartopico);

                auto user = result["user"].as<const char*>();
                log_i("%s",user);
                StaticJsonDocument<256> userObj;
                deserializeJson(userObj, user);
                auto id = userObj[0]["id"].as<int>();
                auto text = userObj[0]["text"].as<const char*>();
                strcpy(nomefull,text);
                log_i("%d",id);
                log_i("%s",text);
                lv_label_set_text(lbl_RSSI,text);
                
                
              } else {
                  log_i("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                  
              }

        http.end();

          }
}



void sendRequest(lv_obj_t *obj, lv_event_t event){

  if (event == LV_EVENT_CLICKED) {

     lv_obj_set_hidden(btn1, true);
     StaticJsonDocument<200> doc2;

      log_i("ESTE AQUI EH NOME ATUAL:");
      log_i("%s",nomefull);
      doc2["TicketId"] = chamados[((atual-1)*num_tickets)+4];
      doc2["UserName"] = nomefull;
      doc2["Status"] = "Accepted";
      doc2["Ip"] = ip_address;
      String requestBody;
      serializeJson(doc2, requestBody);
  
      log_i("Request que vou fazer:");
      log_i("%s",requestBody);
      requestBody.toCharArray(payload,100);
      client.publish(atualizartopico, payload);

  }
}


void printCard2(Ticket *myticket){

    lv_obj_set_hidden(bg_card, true); 

    if(myticket->status=="Open") lv_obj_set_hidden(btn1, false); 
    else lv_obj_set_hidden(btn1, true); 
  
    lv_obj_set_hidden(bg_card, false); 
    lv_label_set_text(lbl_workstation,myticket->workstation);
    lv_label_set_text(lbl_risk,myticket->risk);
    lv_label_set_text(lbl_calltime,myticket->call_time);
    lv_label_set_text(lbl_description,myticket->description);

    powermgm_set_event(POWERMGM_WAKEUP_REQUEST);
    motor_vibe(70);       
    mainbar_jump_to_tilenumber( jitsupport_app_get_app_main_tile_num(), LV_ANIM_OFF );
    statusbar_hide(true);


}





void printCard(uint8_t posic){

    lv_obj_set_hidden(bg_card, true); 

    if(strcmp(chamados[(posic*num_tickets)+5],"Open")==0){
      // log_i("mostrei o botao");
      lv_obj_set_hidden(btn1, false); 
      
    }
    else{
      // log_i("escondi o botao");
      lv_obj_set_hidden(btn1, true);
      
    }
    lv_obj_set_hidden(bg_card, false); 
    lv_label_set_text(lbl_workstation,chamados[(posic*num_tickets)+0]);
    lv_label_set_text(lbl_risk,chamados[(posic*num_tickets)+1]);
    lv_label_set_text(lbl_calltime,chamados[(posic*num_tickets)+2]);
    lv_label_set_text(lbl_description,chamados[(posic*num_tickets)+3]);
    lv_label_set_text(lbl_status,chamados[(posic*num_tickets)+5]);
    lv_label_set_text(lbl_user,chamados[(posic*num_tickets)+6]);

    log_i("%d",atual);
    log_i("/");
    log_i("%d",counter);
  
    sprintf (bufatual, "%d",atual);
    sprintf (buftotal, "%d",counter);
    lv_label_set_text(lbl_actualcard,bufatual);
    lv_label_set_text(lbl_totalcard,buftotal);

    powermgm_set_event(POWERMGM_WAKEUP_REQUEST);
    motor_vibe(70);       
    mainbar_jump_to_tilenumber( jitsupport_app_get_app_main_tile_num(), LV_ANIM_OFF );
    statusbar_hide(true);

}


static void btn1_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED && atual >1) {
        log_i("Clicked\n");
    
        atual = atual-1;
        log_i("back");
        log_i();
        log_i("%d",atual);
        log_i("/");
        log_i("%d",counter);

    printCard(atual-1);
    }    
}


static void btn2_handler(lv_obj_t *obj, lv_event_t event)
{
     if (event == LV_EVENT_CLICKED && atual < counter) {
        log_i("Clicked\n");
    
        atual = atual+1;
        log_i("next");
        log_i();
        log_i("%d",atual);
        log_i("/");
        log_i("%d",counter);
        printCard(atual-1);
    }
   
    
}
static void toggle_Cards_On(){

        lv_obj_set_hidden(bg_card, false);
}

static void toggle_Cards_Off(){

        lv_obj_set_hidden(bg_card, true); 
}


static void removefromArray(lv_obj_t *obj, lv_event_t event){

  if (event == LV_EVENT_CLICKED) {

    uint8_t pos = atual-1;
    uint8_t tam = counter-1;
    uint8_t i;

    log_i("Counter:");
    log_i("%d",counter);
    log_i("Atual:");
    log_i("%d",atual);
    log_i("Pos:");
    log_i("%d",pos);
    log_i("Tam:");
    log_i("%d",tam);


    for(i=pos; i<tam; i++)
    {
              strcpy(chamados[(i*num_tickets)+0],chamados[(i*num_tickets)+num_tickets+0]);
              strcpy(chamados[(i*num_tickets)+1],chamados[(i*num_tickets)+num_tickets+1]);
              strcpy(chamados[(i*num_tickets)+2],chamados[(i*num_tickets)+num_tickets+2]);
              strcpy(chamados[(i*num_tickets)+3],chamados[(i*num_tickets)+num_tickets+3]);
              strcpy(chamados[(i*num_tickets)+4],chamados[(i*num_tickets)+num_tickets+4]);
              strcpy(chamados[(i*num_tickets)+5],chamados[(i*num_tickets)+num_tickets+5]);
              strcpy(chamados[(i*num_tickets)+6],chamados[(i*num_tickets)+num_tickets+6]);
    }
    counter--;

    log_i("Counter:");
    log_i("%d",counter);
    log_i("Atual:");
    log_i("%d",atual);

    if(atual==counter+1){

      log_i("Atual = counter+1");
      atual = counter;
    }

    printCard(atual-1);
  }
  if(counter==0){
      log_i("Counter=0");
      toggle_Cards_Off();
  }
  
}



void newticket(JsonObject jsonObj){
  

        strcpy(chamados[(counter*7)+0],jsonObj["workstation"]);
        strcpy(chamados[(counter*7)+1],jsonObj["risk"]);
        strcpy(chamados[(counter*7)+2],jsonObj["calltime"]);
        strcpy(chamados[(counter*7)+3],jsonObj["description"]);    
        strcpy(chamados[(counter*7)+4],jsonObj["id"]);
        strcpy(chamados[(counter*7)+5],"Open");
        strcpy(chamados[(counter*7)+6],"");
        counter = counter +1;
        atual = counter;
        printCard(counter-1);      
        goto_jitsupport_app_event_cb();
        
       
}











