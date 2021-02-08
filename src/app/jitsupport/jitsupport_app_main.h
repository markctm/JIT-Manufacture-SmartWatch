
#include "AsyncJson.h"
#include "ArduinoJson.h"

#ifndef _JITSUPPORT_APP_MAIN_H
    #define _JITSUPPORT_APP_MAIN_H

    #include <TTGO.h>

    void jitsupport_app_main_setup( uint32_t tile_num );
    void newticket( JsonObject jsonObj);
    

    
#endif // _JITSUPPORT_APP_MAIN_H