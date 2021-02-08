
#ifndef _jitsupport_APP_H
    #define _jitsupport_APP_H

    #include <TTGO.h>

//    #define jitsupport_WIDGET    // uncomment if an widget need

    void jitsupport_app_setup( void );
    void jitsupport_app_hide_app_icon_info( bool show );
    void jitsupport_app_hide_widget_icon_info( bool show );
    uint32_t jitsupport_app_get_app_setup_tile_num( void );
    uint32_t jitsupport_app_get_app_main_tile_num( void );
    void goto_jitsupport_app_event_cb();

#endif // _jitsupport_APP_H