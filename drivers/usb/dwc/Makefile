#
# OTG infrastructure and transceiver drivers
#
obj-$(CONFIG_USB_DWC_OTG)	+= dwc.o

dwc-objs := cil.o cil_intr.o param.o

dwc-objs += apmppc.o

ifneq ($(CONFIG_DWC_DEVICE_ONLY),y)
dwc-objs += hcd.o hcd_intr.o \
		hcd_queue.o
endif

ifneq ($(CONFIG_DWC_HOST_ONLY),y)
dwc-objs += pcd.o pcd_intr.o
endif
