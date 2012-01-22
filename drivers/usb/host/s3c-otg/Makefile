#
# Makefile for USB OTG Host Controller Drivers
#

obj-$(CONFIG_USB_S3C_OTG_HOST) += s3c_otg_hcd.o

s3c_otg_hcd-objs	+= s3c-otg-hcdi-driver.o s3c-otg-hcdi-hcd.o
s3c_otg_hcd-objs	+= s3c-otg-transfer-common.o s3c-otg-transfer-nonperiodic.o \
				s3c-otg-transfer-periodic.o
s3c_otg_hcd-objs	+= s3c-otg-scheduler-ischeduler.o s3c-otg-scheduler-scheduler.o \
				s3c-otg-scheduler-readyq.o
s3c_otg_hcd-objs	+= s3c-otg-oci.o
s3c_otg_hcd-objs	+= s3c-otg-transferchecker-common.o \
				s3c-otg-transferchecker-control.o \
				s3c-otg-transferchecker-bulk.o \
				s3c-otg-transferchecker-interrupt.o
s3c_otg_hcd-objs	+= s3c-otg-isr.o
s3c_otg_hcd-objs	+= s3c-otg-roothub.o
