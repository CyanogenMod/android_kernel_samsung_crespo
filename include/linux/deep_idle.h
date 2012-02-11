/* include/linux/deep_idle.h */

#ifndef _LINUX_DEEPIDLE_H
#define _LINUX_DEEPIDLE_H

bool deepidle_is_enabled(void);
void report_idle_time(int idle_state, int idle_time);

#endif
