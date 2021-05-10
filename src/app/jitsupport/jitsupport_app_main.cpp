
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
#include "hardware/alloc.h"
#include "hardware/callback.h"
#include "hardware/wifictl.h"     
#include "jitsupport_mqtt.h"

#define USE_SERIAL Serial
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>



//---------- Estrutura do Ticket---------

typedef struct{
  char  ticket_id[30];
  char  workstation[30];
  char  risk[30];
  char  call_time[30];
  char  description[50];
  char  user[50];
  char  status[30];
  bool  owner_flag=false;
  uint8_t  state=EMPTY;
} Ticket_t;

Ticket_t all_Tickets[MAX_NUMBER_TICKETS]; 
Ticket_t  myticket;

//----A fazer ainda ------
typedef struct{
  char  Member_Name[30]="";
  bool  online=false;
  uint8_t  state=EMPTY;
} Team_t;

Team_t Team_Members[10];

//------Prototipos---JIT APP---

Ticket_t *remove_ticket(Ticket_t *ticket_remover);
Ticket_t *busca_ticket2 (Ticket_t *myticket);
uint8_t Insere_ticket2 (Ticket_t *myticket);
void Insere_ticket (Ticket_t myticket,Ticket_t *lista);
void busca_ticket (Ticket_t * lista);
uint8_t get_number_tickets();
void printCard3(uint8_t index, uint8_t vibration_intensity);
static void removefromArray(lv_obj_t *obj, lv_event_t event);
void getWatchUser();
void sendRequest(lv_obj_t *obj, lv_event_t event);
static void toggle_Cards_Off();
static void toggle_Cards_On();
static void btn2_handler(lv_obj_t *obj, lv_event_t event);
static void btn1_handler(lv_obj_t *obj, lv_event_t event);
Ticket_t *busca_ticket_index (uint8_t index);
static void exit_jitsupport_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
uint8_t atualiza_ticket(Ticket_t *atualiza);
void sendCanceled(lv_obj_t *obj, lv_event_t event);
void show_watch_status(lv_obj_t *obj, lv_event_t event);
void Get_TeamMembers(void * pvParameters);
void show_team_status(lv_obj_t *obj, lv_event_t event);
//------Prototipos--WIFI------

bool jit_wifictl_event_cb( EventBits_t event, void *arg );



//------Prototipos--MQTT----

void MQTT_callback(char* topic, byte* message, unsigned int length);
bool jitsupport_mqttctrl_event_cb(EventBits_t event, void *arg );




//---------------MQTT---------------------

const char* mqtt_server = MQTT_SERVER;
WiFiClient espClient;
PubSubClient client(espClient);

bool once_flag=false;
uint8_t idTeam;
String NomeTopicoReceber="";
String NomeTopicoAtualizar="";
String NomeTopicoArea="";
char nometopico[15];
char atualizartopico[15];
char areatopico[30];
char payload[200];
char nomepeq[10]= "a";
char nomefull[100];

EventGroupHandle_t xMqttEvent=NULL;
portMUX_TYPE DRAM_ATTR mqttMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t _Get_User_Task,_Get_TeamMembers_Task=NULL;
callback_t *mqtt_callback = NULL;

void Get_User(void * pvParameters);
void Get_User(void * pvParameters);
bool mqtt_send_event_cb( EventBits_t event, void *arg);



//----------------API----------------------

char* GetWatchById_host = "http://172.24.72.137/JITAPI/Smartwatch/GetByIP/";
char GetWatchById_Url[50] = {0};
char ip_address[15];

String meuip;
HTTPClient http;
StaticJsonDocument<200> result;
StaticJsonDocument<200> userjson;
int httpCode = 0;
TTGOClass *twatch;
volatile bool pegueiUser = false;
uint8_t counter = 0;

uint8_t atual = 0;
char bufatual [4];
char buftotal [4];

bool jitsupport_powermgm_loop_cb( EventBits_t event, void *arg );


//------------OBJETOS GRAFICOS---------------

lv_obj_t *jitsupport_app_main_tile = NULL;
lv_style_t jitsupport_app_main_style;
lv_style_t jitsupport_app_main_jitsupportstyle;
lv_obj_t *jitsupport_app_main_jitsupportlabel = NULL;
LV_IMG_DECLARE(exit_32px);
LV_FONT_DECLARE(Ubuntu_72px);

static lv_style_t stl_view;
static lv_style_t stl_bg_card;
static lv_style_t stl_btn1;
static lv_style_t stl_btn2;
static lv_style_t stl_btn3;
static lv_style_t stl_topline;
static lv_style_t stl_transp;
static lv_style_t stl_btnStatus;

static lv_obj_t * bg_card;
static lv_obj_t * team_card;
static lv_obj_t * status_card;
static lv_obj_t * lbl_teamMembers;
static lv_obj_t * lbl_workstation;
static lv_obj_t * lbl_risk;
static lv_obj_t * lbl_calltime;
static lv_obj_t * lbl_status;
static lv_obj_t * lbl_description;
static lv_obj_t * lbl_user;
static lv_obj_t * lbl_statusText;

static lv_obj_t * btn1;
static lv_obj_t * btn2;
static lv_obj_t * btn3;


static lv_obj_t * lbl_btn_status;
static lv_obj_t * lbl_btn_team;


static lv_obj_t * lbl_actualcard;
static lv_obj_t * lbl_totalcard;
static lv_obj_t * lbl_count_separator;

static lv_obj_t * lbl_IP;
static lv_obj_t * lbl_UserName;
static lv_obj_t * lbl_MQTT;
static lv_obj_t * lbl_Area;

static lv_obj_t * btn_back;
static lv_obj_t * btn_next;
static lv_obj_t * btn_exit;
static lv_obj_t * btn_config;
static lv_obj_t *btn_status;
static lv_obj_t *btn_team;

//----------------------------------------------------------------------------
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
    // TOP HORIZONTAL LINE

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
    

   // BUTTON 2 STYLE
    lv_style_init(&stl_btn3);
    lv_style_set_bg_color(&stl_btn3, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x1f, 0x1f));
    lv_style_set_text_color(&stl_btn3, LV_STATE_DEFAULT, LV_COLOR_WHITE);


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
    

  //-----Status Card------ 
    status_card = lv_obj_create(jitsupport_cont, NULL);
    lv_obj_set_pos(status_card, 0, 37);
    lv_obj_set_width(status_card,240);
    lv_obj_set_height(status_card,167);
    lv_obj_add_style(status_card, LV_OBJ_PART_MAIN, &stl_view);

    // LABEL NO CARD
 
    lbl_IP = lv_label_create(status_card, NULL);
    lv_label_set_text(lbl_IP, "0.0.0.0");
    lv_obj_align(lbl_IP, status_card, LV_ALIGN_IN_TOP_LEFT, 5, 80);

    
    lbl_UserName = lv_label_create(status_card, NULL);
    lv_label_set_text(lbl_UserName, "No Name Found");
    lv_obj_align(lbl_UserName, status_card, LV_ALIGN_IN_TOP_LEFT, 5, 100);

    lbl_Area = lv_label_create(status_card, NULL);
    lv_label_set_text(lbl_Area, "No Area Found");
    lv_obj_align(lbl_Area, status_card, LV_ALIGN_IN_TOP_LEFT, 5, 120);
    
    
    lbl_MQTT = lv_label_create(status_card, NULL);
    lv_label_set_text(lbl_MQTT, "MQTT NOT CONNECTED !");
    lv_obj_align(lbl_MQTT, status_card, LV_ALIGN_IN_TOP_LEFT, 5, 140);
 


  //-----Team Ground Card------ 
    team_card = lv_obj_create(jitsupport_cont, NULL);
    lv_obj_set_pos(team_card, 0, 37);
    lv_obj_set_width(team_card,240);
    lv_obj_set_height(team_card,167);
    lv_obj_add_style(team_card, LV_OBJ_PART_MAIN, &stl_view);


  // -------Team Label---------

    lbl_teamMembers = lv_label_create(team_card, NULL);
    lv_label_set_text(lbl_teamMembers, "");
    lv_obj_align(lbl_teamMembers, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);


    //-----Back Ground Card------ 
    bg_card = lv_obj_create(jitsupport_cont, NULL);
    lv_obj_set_pos(bg_card, 0, 37);
    lv_obj_set_width(bg_card,240);
    lv_obj_set_height(bg_card,167);
    lv_obj_add_style(bg_card, LV_OBJ_PART_MAIN, &stl_bg_card);

    // WORKSTATION LABEL
     
    lbl_workstation = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_workstation, "WORKSTATION");
    lv_obj_align(lbl_workstation, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);

   // CALL TIME LABEL
    
    lbl_calltime = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_calltime, "12: 00");
    lv_obj_align(lbl_calltime, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 33);

    // RISK LABEL
    
    lbl_risk = lv_label_create(bg_card, NULL);
    lv_label_set_recolor(lbl_risk, true);
    lv_label_set_text(lbl_risk, "RISCO");
    lv_obj_align(lbl_risk, lbl_calltime, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // DESCRIPTION LABEL
    
    lbl_description = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_description, "DESCRIPTION");
    lv_obj_align(lbl_description, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 70);

    // Ticket Status

    lbl_statusText = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_statusText, "Ticket:");
    lv_obj_align(lbl_statusText, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 100);


    // STATUS LABEL
    
    lbl_status = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_status, "Status   ");
    lv_obj_align(lbl_status, lbl_statusText, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // USER LABEL

    lbl_user = lv_label_create(bg_card, NULL);
    lv_label_set_text(lbl_user, "    ");
    lv_obj_align(lbl_user, lbl_status, LV_ALIGN_OUT_RIGHT_MID, 15, 0);


    // BUTTON 1

    btn1 = lv_btn_create(bg_card, NULL);
    lv_obj_set_pos(btn1, 10, 130);
    lv_obj_set_width(btn1,75);
    lv_obj_set_height(btn1,30);
    lv_obj_set_event_cb(btn1, sendRequest);
   
    
    // BUTTON 1 LABEL

    lv_obj_t * lbl_btn1;
    lbl_btn1 = lv_label_create(btn1, NULL);
    lv_label_set_text(lbl_btn1, "Confirmar");
    lv_label_set_text(lbl_btn1, LV_SYMBOL_OK);
    lv_obj_add_style(btn1, LV_OBJ_PART_MAIN, &stl_btn1);


    // BUTTON 2

    btn2 = lv_btn_create(bg_card, NULL);
    lv_obj_set_pos(btn2, 155, 130);
    lv_obj_set_width(btn2,75);
    lv_obj_set_height(btn2,30);
    lv_obj_set_event_cb(btn2, sendCanceled);
    
    
    //lv_obj_set_event_cb(btn2, removefromArray);
    
    // BUTTON 2 LABEL
    lv_obj_t * lbl_btn2;
    lbl_btn2 = lv_label_create(btn2, NULL);
    lv_label_set_text(lbl_btn2, "Recusar");
    lv_label_set_text(lbl_btn2, LV_SYMBOL_CLOSE);
    lv_obj_add_style(btn2, LV_OBJ_PART_MAIN, &stl_btn2);



    // BUTTON 3 - Close Card 
    btn3 = lv_btn_create(bg_card, NULL);
    lv_obj_set_pos(btn3, 205, 5);
    lv_obj_set_width(btn3,30);
    lv_obj_set_height(btn3,30);
    lv_obj_set_event_cb(btn3, removefromArray);
    
    
    // BUTTON 3 LABEL
    lv_obj_t * lbl_btn3;
    lbl_btn3 = lv_label_create(btn3, NULL);
    lv_label_set_text(lbl_btn3, "Fechar");
    lv_label_set_text(lbl_btn3, LV_SYMBOL_CLOSE);
    lv_obj_add_style(btn3, LV_OBJ_PART_MAIN, &stl_btn3);


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
    lv_obj_set_width(btn_exit,60);
    lv_obj_set_height(btn_exit,35);
    lv_obj_align(btn_exit, jitsupport_cont, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_add_style(btn_exit, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_exit;
    lbl_btn_exit = lv_label_create(btn_exit, NULL);
    lv_label_set_text(lbl_btn_exit, LV_SYMBOL_HOME);
    lv_obj_set_event_cb(btn_exit, exit_jitsupport_app_main_event_cb);

    btn_config= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_config,60);
    lv_obj_set_height(btn_config,35);
    lv_obj_align(btn_config, jitsupport_cont, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
    lv_obj_add_style(btn_config, LV_OBJ_PART_MAIN, &stl_transp);
    lv_obj_t * lbl_btn_config;
    lbl_btn_config = lv_label_create(btn_config, NULL);
    lv_label_set_text(lbl_btn_config, LV_SYMBOL_SETTINGS);

    btn_status= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_status,60);
    lv_obj_set_height(btn_status,35);
    lv_obj_align(btn_status, jitsupport_cont, LV_ALIGN_IN_TOP_MID, -30, 0);
    lv_obj_add_style(btn_status, LV_OBJ_PART_MAIN, &stl_transp);    
    lbl_btn_status = lv_label_create(btn_status, NULL);
    lv_label_set_text(lbl_btn_status, LV_SYMBOL_CLOSE);
    lv_obj_set_event_cb(btn_status, show_watch_status);


    btn_team= lv_btn_create(jitsupport_cont, NULL);
    lv_obj_set_width(btn_team,60);
    lv_obj_set_height(btn_team,35);
    lv_obj_align(btn_team, jitsupport_cont, LV_ALIGN_IN_TOP_MID, 30, 0);
    lv_obj_add_style(btn_team, LV_OBJ_PART_MAIN, &stl_transp);
    lbl_btn_team = lv_label_create(btn_team, NULL);
    lv_label_set_text(lbl_btn_team, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_event_cb(btn_team, show_team_status);


    // INIT CARD

    lv_obj_set_hidden(status_card,true);
    lv_obj_set_hidden(team_card,true);

     
     mqttctrl_setup();



    client.setServer(mqtt_server, MQTT_PORT);
    client.setKeepAlive(MQTT_KEEPALIVE_SECONDS);
    client.setCallback(MQTT_callback);

    xMqttEvent=xEventGroupCreate(); 
   
  //---- Task para GET POST USER
     xTaskCreatePinnedToCore( Get_User,                                 /* Function to implement the task */
                             "Get User",                                /* Name of the task */
                              3000,                                     /* Stack size in words */
                              NULL,                                     /* Task input parameter */
                              0,                                        /* Priority of the task */
                              &_Get_User_Task,                          /* Task handle. */
                              0 );
  

   mqqtctrl_register_cb(MQTT_DISCONNECTED_FLAG | MQTT_CONNECTED_FLAG, jitsupport_mqttctrl_event_cb,  "jitsupport Mqtt CB " );
   powermgm_register_loop_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, jitsupport_powermgm_loop_cb, "jitsupport app loop" );
   wifictl_register_cb( WIFICTL_CONNECT | WIFICTL_DISCONNECT | WIFICTL_OFF | WIFICTL_ON | WIFICTL_SCAN | WIFICTL_WPS_SUCCESS | WIFICTL_WPS_FAILED | WIFICTL_CONNECT_IP, jit_wifictl_event_cb, "JIT Wifi Event" );
}



void Get_TeamMembers(void * pvParameters)
{

  char getpost[100]="http://172.24.72.137/JITAPI/Smartwatch/GetByTeam/";
  char aux[4]="";
  sprintf(aux, "%d",idTeam);
  strcat(getpost, aux);
 
  while(1)
  {
   
      if(wifictl_get_event( WIFICTL_CONNECT ))
      {
      
          http.begin(getpost); //HTTP
          httpCode = http.GET();
          log_i("%d",httpCode); 

          if(httpCode > 0) {
            
              if(httpCode == HTTP_CODE_OK) {

                String payload = http.getString();
                http.end();
                log_i("%s",payload);

                DynamicJsonDocument doc3(3072);
                DeserializationError error = deserializeJson(doc3, payload);

                if (error) {
                  log_i("deserializeJson() failed: ");
                }
                else{
                      uint8_t num=0;
                      StaticJsonDocument<256> userObj;

                      for (JsonObject elem : doc3.as<JsonArray>()) {

                        const char* user = elem["user"];
                        deserializeJson(userObj, user);
                        auto text = userObj[0]["text"].as<const char*>(); 
                        char *UserTrim = strtok((char *)text," ");
                        strtok(NULL, " "); 
                        strcpy(Team_Members[num].Member_Name,UserTrim);
                        Team_Members[num].state=FULL;
                                        
                        log_i("Membro %d: %s", num, Team_Members[num].Member_Name);
                        num++; 
                    
                      }
                      
                      //-------------- INSERE TEAM MEMBER -----------------------
                      
                      if(strcmp(Team_Members[0].Member_Name," ")==0)
                      {
                        // Rolou algum problema Pelo Menos uma pessoa é obrigatório ter no time 
                        log_i("Task Team Member EMPTY !");

                      }
                      else{
                        
                          //Melhorar implementação 

                          lv_obj_t * lbl_Teamstatus[num-1];
                          lv_obj_t * lbl_team;

                          lbl_team = lv_label_create(team_card, NULL);
                          lv_label_set_text(lbl_team, "TEAM MEMBERS: ");
                          lv_obj_align(lbl_team, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 5);


                          for( int a=0 ; a < num;  a++)
                          {
                                lbl_Teamstatus[a] = lv_label_create(team_card, NULL);
                                lv_label_set_text(lbl_Teamstatus[a], Team_Members[a].Member_Name);
                                lv_obj_align(lbl_Teamstatus[a], NULL, LV_ALIGN_IN_TOP_LEFT, 10, 30 + 13*a);
                          }
                          
                          log_i("Task Team Member Deleted !");
                          vTaskDelete(NULL);   
                        }

                }

             } 
     
          }
       }

    vTaskDelay(2000/ portTICK_PERIOD_MS );
  }
}

void Get_User(void * pvParameters)
{

  log_i("Inicialização de Procura do User");
  while(1)
  {           
          if(wifictl_get_event( WIFICTL_CONNECT ))
          {
            if(!pegueiUser)getWatchUser();
            else
            {             

                MQTT2_set_client(ip_address);
                MQTT2_set_subscribe_topics(nometopico,atualizartopico,areatopico);
                
                log_i("setando evento de conexão mqtt");
                vTaskDelay(1000/ portTICK_PERIOD_MS );
                mqqtctrl_set_event(MQTT_START_CONNECTION);

                xTaskCreatePinnedToCore( Get_TeamMembers,                        /* Function to implement the task */
                                              "Get User",                              /* Name of the task */
                                              5000,                                   /* Stack size in words */
                                                NULL,                                   /* Task input parameter */
                                                0,                                      /* Priority of the task */
                                                &_Get_TeamMembers_Task,                 /* Task handle. */
                                                0 );   
                vTaskDelete(NULL);  
            }            
          }
          else log_i("Não ta rolando WIFI");     
      vTaskDelay(2000/ portTICK_PERIOD_MS );
  }

}

bool jit_wifictl_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case WIFICTL_CONNECT:

        log_i("AI CONNECTED");

        break;


        default:
        
        log_i("AI DISCONNECTED");

        break;
        
        }
        return true ;
}


//-------------------------APP-EVENT ACTIONS-------------------------


static void exit_jitsupport_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_maintile( LV_ANIM_OFF );
                                        break;
    }
}



bool jitsupport_powermgm_loop_cb( EventBits_t event, void *arg ) {

    switch(event) {
        case POWERMGM_STANDBY:  
              
            //    if(wifi_connected==1) vTaskResume( _mqttCheck_Task );
            break;
        case POWERMGM_WAKEUP:

             //   if(wifi_connected==1) vTaskResume( _mqttCheck_Task );
            break;
        case POWERMGM_SILENCE_WAKEUP:
                
              // if(wifi_connected==1) vTaskResume( _mqttCheck_Task );             
                    
            break;
        }

return( true );
}


//-------------------------MQTT FUNCTIONS------------------------


bool jitsupport_mqttctrl_event_cb(EventBits_t event, void *arg )
{

    switch(event) {
        case MQTT_DISCONNECTED_FLAG:  
              
             lv_label_set_text(lbl_btn_status, LV_SYMBOL_CLOSE);
             lv_label_set_text(lbl_MQTT, "MQTT_DISCONNECTED!");  
            break;
        case MQTT_CONNECTED_FLAG:
        
            lv_label_set_text(lbl_btn_status, LV_SYMBOL_OK);
            lv_label_set_text(lbl_MQTT, "MQTT_CONNECTED");  

            break;

        }

return( true );
}




void MQTT_callback(char* topic, byte* message, unsigned int length) {
  
  log_i("Message arrived on topic: ");
  log_i("%s", topic);

  //------- JSON DOC------------
  StaticJsonDocument<200> result;
  deserializeJson(result, message);
  JsonObject object = result.as<JsonObject>();
  
  log_i("Valor JSON %d:",object.isNull());

  if(object.isNull()==false)
  {
      // OK VALID JSON ! 
     log_i("Aqui chegou");
     log_i("%s",NomeTopicoReceber);
      
      
      //**************************************************
      //
      //     TOPICO RECEBER
      //
      //***************************************************

      if (String(topic) == NomeTopicoReceber) {
            

            strcpy(myticket.ticket_id,result["id"]);
            strcpy(myticket.workstation,result["workstation"]);
            strcpy(myticket.risk,result["risk"]);
            strcpy(myticket.call_time,result["calltime"]);
            strcpy(myticket.description,result["description"]);    
            strcpy(myticket.status,"Open"); 
            log_i("\n Ticket ID: %s \n Workstation ID: %s \n Risk ID: %s \n Calltime ID: %s \n Description: %s\n",myticket.ticket_id,myticket.workstation,myticket.risk,myticket.call_time,myticket.description);         
            log_i("sizeof (ticket) = %d\n", sizeof (myticket));

            if(!(strcmp(myticket.risk,"1")))strcpy(myticket.risk,"- Rodando");  
            if(!(strcmp(myticket.risk,"0")))strcpy(myticket.risk,"#ff0000 - Downtime#"); 
          
            log_i("%s", myticket.risk);

            if(busca_ticket2(&myticket)==nullptr)
            {
              // OK TICKET NOVO PODE INSERIR
              Insere_ticket2(&myticket);
              atual = get_number_tickets();
              printCard3(atual-1,VIBRATION_INTENSE);

              toggle_Cards_On();
            }
      }
      
      //**************************************************
      //
      //     TOPICO  ATUALIZAR
      //
      //***************************************************
      
      else if(String(topic) == NomeTopicoAtualizar){
      
          Ticket_t atualiza;    
          log_i("****ATUALIZAR CHAMADO ****");

          //Pegando dados do JSON FILE 
          strcpy(atualiza.ticket_id,result["TicketId"]);
          strcpy(atualiza.user,result["UserName"]);  
          strcpy(atualiza.status,result["Status"]); 
          log_i("ID: %s  User: %s  Status: %s",atualiza.ticket_id,atualiza.user,atualiza.status);

          //Reduzindo o Tamanho do Nome
          char *Search_UserNameTrim = strtok((char *)atualiza.user," "); 
          strcpy(atualiza.user,Search_UserNameTrim);       
          log_i("%s",atualiza.user);   

          if(busca_ticket2(&atualiza)!=nullptr)
          {
              log_i("Entrei aqui");
              uint8_t index = atualiza_ticket(&atualiza);
              
              if(index>=0) 
              {
                printCard3(index,VIBRATION_INTENSE);               
                if(strcmp(atualiza.status,"OnGoing")==0){    
                                      
                }
                
                if(strcmp(atualiza.status,"Done")==0){

                      //----OK TICKET EXISTE------------- 
                      log_i("Removing Ticket....");
                      removefromArray(btn2,LV_EVENT_CLICKED);

                }
              }

          }
        }
      
      //**************************************************
      //
      //     TOPICO  AREA
      //
      //***************************************************
        
        else if(String(topic) == NomeTopicoArea){
         
          log_i("Teste de Reset");
          char comando[30];
          
          strcpy(comando,result["comando"]);
          log_i("%s",comando);
                    
          if(strcmp(comando,"reset")==0){  
            
            log_i("Reset Command Received");
            cmd_reset();

          }

          if(strcmp(comando,"update")==0){  
            
            log_i("Reset Command Received");
            delay(2000);
            cmd_reset();
            
          }

        }

    }
    else
    {
        //NOT VALID JSON MESSAGE !! 
        log_i("Not a Valid Json Message");
    }
 
}



uint8_t get_number_tickets()
{
    uint8_t ct=0;

    for(int i=0; i< MAX_NUMBER_TICKETS; i++)
    {
        if(all_Tickets[i].state==FULL)ct++;
    }
    log_i("Numero de Tickets %d",ct );
return ct;

}



uint8_t Insere_ticket2 (Ticket_t *myticket)
{

    for(int i=0; i< MAX_NUMBER_TICKETS; i++)
    {
        if(all_Tickets[i].state==EMPTY)
        {
            
            strcpy(all_Tickets[i].ticket_id,myticket->ticket_id);
            strcpy(all_Tickets[i].workstation,myticket->workstation);  
            strcpy(all_Tickets[i].risk,myticket->risk);  
            strcpy(all_Tickets[i].call_time,myticket->call_time);  
            strcpy(all_Tickets[i].description,myticket->description);  
            strcpy(all_Tickets[i].status,myticket->status);    
            all_Tickets[i].state=FULL;    

            log_i("Ticket Salvo");
          return 1;
        }
      
    }
log_i("NUMERO MAIXIMO DE TICKETS ATINGIDO !!!");
return 0; 

}

Ticket_t *busca_ticket2 (Ticket_t *myticket)
{   
    Ticket_t *ptr;

    for(int i=0; i< MAX_NUMBER_TICKETS; i++)
    {
        if(strcmp(all_Tickets[i].ticket_id,myticket->ticket_id)==0)
        {
          log_i("Ticket Found !");
          ptr=&all_Tickets[i];
          return ptr;
        }
      
    }

  log_i("Ticket NOT Found !");
  return nullptr; 
}

Ticket_t *busca_ticket_index (uint8_t index)
{   
    Ticket_t *ptr;
    ptr=&all_Tickets[index];
  return ptr; 
}


uint8_t atualiza_ticket(Ticket_t *atualiza)
 {
    log_i("Card Atualizado1 !");

    for(int i=0; i< MAX_NUMBER_TICKETS; i++)
    {
        log_i("Card Atualizado2 !");
        log_i("%s",atualiza->ticket_id);


        if(strcmp((const char *)all_Tickets[i].ticket_id,(const char *)atualiza->ticket_id)==0)
        {                     
             strcpy(all_Tickets[i].user,atualiza->user);
             strcpy(all_Tickets[i].status,atualiza->status);      
             log_i("Card Atualizado3!");

          return i;  
        } 
    }

   return -1; //Não encontrou nada 
 }


Ticket_t *remove_ticket(Ticket_t *ticket_remover)
 {
    Ticket_t *ptr;
    for(int i=0; i< MAX_NUMBER_TICKETS; i++)
    {
            if(strcmp(all_Tickets[i].ticket_id,ticket_remover->ticket_id)==0)
            {           
              // Reordena            
              for(uint8_t p=i; p<MAX_NUMBER_TICKETS-1; p++)
              {              
                 
                    //Realiza um shift nas posições da struct (Segue implemenatação Atual)
                    strcpy(all_Tickets[p].ticket_id,all_Tickets[p+1].ticket_id);
                    strcpy(all_Tickets[p].workstation,all_Tickets[p+1].workstation);
                    strcpy(all_Tickets[p].risk,all_Tickets[p+1].risk);
                    strcpy(all_Tickets[p].call_time,all_Tickets[p+1].call_time);
                    strcpy(all_Tickets[p].description,all_Tickets[p+1].description);
                    strcpy(all_Tickets[p].user,all_Tickets[p+1].user);
                    strcpy(all_Tickets[p].status,all_Tickets[p+1].status);
                    all_Tickets[p].state=all_Tickets[p+1].state;
                    all_Tickets[p].owner_flag=all_Tickets[p+1].owner_flag;
                  
              }                 
                  sprintf(buftotal, "%d",get_number_tickets());
                  lv_label_set_text(lbl_totalcard,buftotal);
            
              return ptr;              
            }
    }
 
  return nullptr; //Não encontrou nada 

}


//----------------APP FUNCTIONS---------------------------- 

void getWatchUser(){

    meuip = WiFi.localIP().toString();
    meuip.toCharArray(ip_address,15);

    log_i("MEU IP É O SEGUINTE: %s",ip_address);
    lv_label_set_text(lbl_IP,ip_address);

    strcpy(GetWatchById_Url,"");
    strcat(GetWatchById_Url, GetWatchById_host);
    strcat(GetWatchById_Url, ip_address);
    
    log_i("ENDEREÇO DE API:");
    log_i("%s",GetWatchById_Url); 

        
#ifdef   NO_HTTP_RESPONSE  
      String numerotopico = "15";
                 
      NomeTopicoReceber = "receber/" + numerotopico;
      NomeTopicoAtualizar = "atualizar/" + numerotopico;
      
      log_i("Nome Topico Receber:");
      log_i("%s",NomeTopicoReceber);
      log_i("Nome Topico Atualizar:");            
      log_i("%s",NomeTopicoAtualizar);
      
      NomeTopicoReceber.toCharArray(nometopico,15);
      NomeTopicoAtualizar.toCharArray(atualizartopico,15);
      strcpy(nomefull,"Mark Carmo Testi Moreira");
      pegueiUser = true;   

      return;

#endif       

        
        http.begin(GetWatchById_Url); //HTTP
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

                auto user = result["user"].as<const char*>();
                auto area = result["area"].as<const char*>();
                log_i("%s",user);

                StaticJsonDocument<256> userObj;
                StaticJsonDocument<256> areaObj;
                deserializeJson(userObj, user);
                deserializeJson(areaObj, area);

                auto id = userObj[0]["id"].as<int>();
                auto text = userObj[0]["text"].as<const char*>();
                auto my_local = areaObj[0]["text"].as<const char*>();

                strcpy(nomefull,text);

                log_i("%d",id);
                log_i("%s",text);
                log_i("%s",my_local);

                lv_label_set_text(lbl_UserName,text);
                lv_label_set_text(lbl_Area, my_local);

                NomeTopicoArea = "general/" + String(my_local);  
                NomeTopicoArea.toCharArray(areatopico,30);
                pegueiUser = true;

                
              } else {
                  log_i("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                  pegueiUser = false;
              }

        http.end();

          }
}


void printCard3(uint8_t index, uint8_t vibration_intensity){

    lv_obj_set_hidden(bg_card, true); 

    if(strcmp(all_Tickets[index].status,"Open")==0)
    {         
           lv_obj_set_hidden(btn1, false);
           lv_obj_set_hidden(btn2, true);
           lv_obj_set_hidden(btn3, true);

    }
    
    else if((strcmp(all_Tickets[index].status,"Accepted")==0) && (all_Tickets[index].owner_flag==false))
    {
      lv_obj_set_hidden(btn1, true);
      lv_obj_set_hidden(btn2, true);
      lv_obj_set_hidden(btn3, false); 

    }
    else if((strcmp(all_Tickets[index].status,"Accepted")==0) && (all_Tickets[index].owner_flag==true))
    {
      lv_obj_set_hidden(btn1, true);
      lv_obj_set_hidden(btn2, false);  // Canceled status if pressed
       

    }
    else if((strcmp(all_Tickets[index].status,"OnGoing")==0) && (all_Tickets[index].owner_flag==true))
    {
      lv_obj_set_hidden(btn1, true);
      lv_obj_set_hidden(btn2, true);  
    
    }
    else if((strcmp(all_Tickets[index].status,"OnGoing")==0) && (all_Tickets[index].owner_flag==false))
    {
      lv_obj_set_hidden(btn1, true);
      lv_obj_set_hidden(btn2, true);
      lv_obj_set_hidden(btn3, false);  
    }
    else if((strcmp(all_Tickets[index].status,"Canceled")==0))
    {
      lv_obj_set_hidden(btn1, true);
      lv_obj_set_hidden(btn2, true);
      lv_obj_set_hidden(btn3, false); 
    }
    else {}
    

    lv_obj_set_hidden(bg_card, false); 
    lv_label_set_text(lbl_workstation,all_Tickets[index].workstation);
    lv_label_set_text(lbl_risk,all_Tickets[index].risk);
    lv_label_set_text(lbl_calltime,all_Tickets[index].call_time);
    lv_label_set_text(lbl_description,all_Tickets[index].description);
    lv_label_set_text(lbl_status,all_Tickets[index].status);
    lv_label_set_text(lbl_user,all_Tickets[index].user);

    sprintf(bufatual, "%d", atual);
    sprintf(buftotal, "%d",get_number_tickets());

    lv_label_set_text(lbl_actualcard,bufatual);
    lv_label_set_text(lbl_totalcard,buftotal);

    powermgm_set_event(POWERMGM_WAKEUP_REQUEST);
    motor_vibe(vibration_intensity);       
    mainbar_jump_to_tilenumber( jitsupport_app_get_app_main_tile_num(), LV_ANIM_OFF );
    statusbar_hide(true);


}


static void toggle_Cards_On(){

        lv_obj_set_hidden(bg_card, false);
}

static void toggle_Cards_Off(){

        lv_obj_set_hidden(bg_card, true); 
}


static void removefromArray(lv_obj_t *obj, lv_event_t event){

 if (event == LV_EVENT_CLICKED) {

    remove_ticket(&all_Tickets[atual-1]);
    atual=get_number_tickets();
    printCard3(atual-1,VIBRATION_INTENSE);

    if(get_number_tickets()==0){
      log_i("Number Tickets=0");
      toggle_Cards_Off();
  }
  }

}


static void btn1_handler(lv_obj_t *obj, lv_event_t event)
{
    if ((event == LV_EVENT_CLICKED) && (atual > 1)) {
        log_i("Clicked\n");
    
        atual = atual-1;
        log_i("back");
        log_i();
        log_i("%d",atual);
        log_i("/");
        log_i("%d",counter);

      printCard3(atual-1,VIBRATION_DISABLE);
    }    
}


static void btn2_handler(lv_obj_t *obj, lv_event_t event)
{
     if ((event == LV_EVENT_CLICKED) && (atual< get_number_tickets())) {
        log_i("Clicked\n");
    
        atual = atual+1;
        log_i("next");
        log_i();
        log_i("%d",atual);
        log_i("/");
        log_i("%d",counter);
        printCard3(atual-1,VIBRATION_DISABLE);
    }
   
    
}


void sendCanceled(lv_obj_t *obj, lv_event_t event){

  if (event == LV_EVENT_CLICKED) {

      lv_obj_set_hidden(btn2, true);
      StaticJsonDocument<200> doc2;
  
     // all_Tickets[atual-1].owner_flag= true;  //YOU ARE THE OWNER AND THIS TICKET IS YOUR RESPONSIBILITY !!
      
      log_i("ESTE AQUI EH NOME ATUAL:");
      log_i("%s",nomefull);
      doc2["TicketId"] = all_Tickets[atual-1].ticket_id;
      doc2["UserName"] = nomefull;
      doc2["Status"] = "Canceled";
      doc2["Ip"] = ip_address;
      String requestBody;
      serializeJson(doc2, requestBody);
  
      log_i("Request que vou fazer:");
      log_i("%s",requestBody);
      requestBody.toCharArray(payload,200);

      MQTT2_publish(atualizartopico, payload);

  }

}


void sendRequest(lv_obj_t *obj, lv_event_t event){

  if (event == LV_EVENT_CLICKED) {

      lv_obj_set_hidden(btn1, true);
      StaticJsonDocument<200> doc2;
  
      all_Tickets[atual-1].owner_flag= true;  //YOU ARE THE OWNER AND THIS TICKET IS YOUR RESPONSIBILITY !!
      
      log_i("ESTE AQUI EH NOME ATUAL:");
      log_i("%s",nomefull);
      doc2["TicketId"] = all_Tickets[atual-1].ticket_id;
      doc2["UserName"] = nomefull;
      doc2["Status"] = "Accepted";
      doc2["Ip"] = ip_address;
      String requestBody;
      serializeJson(doc2, requestBody);
  
      log_i("Request que vou fazer:");
      log_i("%s",requestBody);
      requestBody.toCharArray(payload,200);

      MQTT2_publish(atualizartopico, payload);


  }

}



void show_watch_status(lv_obj_t *obj, lv_event_t event){


if (event == LV_EVENT_CLICKED) {

        
          if(lv_obj_get_hidden(status_card)==true)
          {
              lv_obj_set_hidden(status_card,false);
              lv_obj_set_hidden(bg_card,true);
              lv_obj_set_hidden(team_card,true); 


          }
          else
          {
              
              lv_obj_set_hidden(status_card,true);
              lv_obj_set_hidden(team_card,true); 
              if(get_number_tickets()>0)lv_obj_set_hidden(bg_card,false);


          }
}
}


void show_team_status(lv_obj_t *obj, lv_event_t event){

if (event == LV_EVENT_CLICKED) {

          
          if(lv_obj_get_hidden(team_card)==true)
          {
              //Ta escondido Team viewer           
              lv_obj_set_hidden(team_card,false);
              lv_obj_set_hidden(bg_card,true);
              lv_obj_set_hidden(status_card,true); 

          }
          else
          {
              
              lv_obj_set_hidden(team_card,true);
              lv_obj_set_hidden(status_card,true);
              if(get_number_tickets()>0)lv_obj_set_hidden(bg_card,false);  

          }
   
      
}

}





