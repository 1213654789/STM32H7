#include "my_gui.h"
#include "..\lvgl\lvgl.h"
#include "..\lvgl\src\extra\widgets\win\lv_win.h"

void mygui(void)
{
    lv_obj_t* switch_obj = lv_switch_create(lv_scr_act());
    lv_obj_set_size(switch_obj, 60, 20);
    lv_obj_align(switch_obj, LV_ALIGN_CENTER, 0, 0);
    
   /* lv_obj_t* win = lv_win_create(lv_scr_act(), 40);
    lv_obj_set_size(win, 300, 200);
    lv_obj_center(win);
    lv_obj_set_style_border_width(win, 5, LV_STATE_DISABLED);*/
    
}

