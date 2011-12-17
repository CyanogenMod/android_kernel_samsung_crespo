ccflags-y := \
 -D__linux__ -DLINUX \
 -Idrivers/gpu/pvr \
 -DSUPPORT_DBGDRV_EVENT_OBJECTS \
 -DLDM_PLATFORM

dbgdrv-$(CONFIG_PVR_PDUMP) := \
	dbgdriv.o \
	handle.o \
	hostfunc.o \
	hotkey.o \
	ioctl.o \
	main.o

obj-$(CONFIG_PVR_PDUMP) := dbgdrv.o
