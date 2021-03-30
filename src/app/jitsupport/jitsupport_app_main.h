
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_APP_MAIN_H
    #define _JITSUPPORT_APP_MAIN_H

    #include <TTGO.h>


    #define MAX_NUMBER_TICKETS          10
    #define EMPTY                       1
    #define FULL                        0

    void jitsupport_app_main_setup( uint32_t tile_num );
    void newticket( JsonObject jsonObj);
    

    
#endif // _JITSUPPORT_APP_MAIN_H