/* include/linux/touchkey.h */

#ifndef _LINUX_TOUCHKEY_H
#define _LINUX_TOUCHKEY_H

struct touchkey_implementation {
	void (*enable)(void);
	void (*disable)(void);
	int (*status)(void);
};

void register_touchkey_implementation(struct touchkey_implementation *imp);
#endif
