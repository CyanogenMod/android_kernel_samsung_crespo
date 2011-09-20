/* include/linux/screen_dimmer.h */

#ifndef _LINUX_SCREEN_DIMMER_H
#define _LINUX_SCREEN_DIMMER_H

struct screendimmer_implementation
{
    void (* enable)(void);
    void (* disable)(void);
};

void register_screendimmer_implementation(struct screendimmer_implementation * imp);
void touchscreen_pressed(void);
bool screen_is_dimmed(void);

#endif
