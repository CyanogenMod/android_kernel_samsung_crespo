ccflags-y := \
	-Wno-pointer-sign\
	-fno-strict-overflow\
	-fconserve-stack\
	-Wdeclaration-after-statement\
	-Wpointer-arith\
	-Wmissing-format-attribute\
	-Wno-format-zero-length\
	-Wmissing-prototypes\
	-Wstrict-prototypes\
	-Wno-unused-parameter\
	-Wno-sign-compare\
	-Werror\
	-fno-strict-aliasing\
	-Wno-pointer-arith\
	-Os

ccflags-y += -DLINUX -D__linux__ -DANDROID
ccflags-y += -Idrivers/gpu/pvr

ccflags-y += \
	-DSUPPORT_SGX_NEW_STATUS_VALS \
	-DSUPPORT_SGX_HWPERF \
	-DSYS_USING_INTERRUPTS \
	-DPVR_SECURE_HANDLES \
	-DTRANSFER_QUEUE \
	-DSUPPORT_SGX \
	-DSUPPORT_HW_RECOVERY \
	-DLDM_PLATFORM \
	-DPVR_LINUX_USING_WORKQUEUES \
	-DPVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE \
	-DPVR_LINUX_TIMERS_USING_WORKQUEUES \
	-DSYS_CUSTOM_POWERLOCK_WRAP \
	-DSUPPORT_MEMINFO_IDS \
	-DSYS_SGX_ACTIVE_POWER_LATENCY_MS=100 \
	-DPVRSRV_MODNAME="\"pvrsrvkm\"" \
	-Idrivers/gpu/pvr/sgx \
	-DSUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS \
	-DPVRSRV_MMU_MAKE_READWRITE_ON_DEMAND \
	-DPVRSRV_AVOID_RANGED_INVALIDATE \
	-DPVR_LDM_DRIVER_REGISTRATION_NAME="\"pvrsrvkm\""

ccflags-$(CONFIG_PVR_BUILD_RELEASE) += \
	-DPVR_BUILD_TYPE="\"release\"" -DRELEASE

ccflags-$(CONFIG_PVR_BUILD_DEBUG) += \
	-DPVR_BUILD_TYPE="\"debug\"" -DDEBUG

ccflags-$(CONFIG_PVR_NEED_PVR_DPF) += -DPVRSRV_NEED_PVR_DPF
ccflags-$(CONFIG_PVR_NEED_PVR_ASSERT) += -DPVRSRV_NEED_PVR_ASSERT

ccflags-$(CONFIG_PVR_DEBUG_MEMORY) += \
	-DDEBUG_LINUX_MEMORY_ALLOCATIONS \
	-DDEBUG_LINUX_MEM_AREAS \
	-DDEBUG_LINUX_MMAP_AREAS

ccflags-$(CONFIG_PVR_DEBUG_BRIDGE_KM) += -DDEBUG_BRIDGE_KM
ccflags-$(CONFIG_PVR_DEBUG_TRACE_BRIDGE_KM) += -DDEBUG_TRACE_BRIDGE_KM
ccflags-$(CONFIG_PVR_DEBUG_BRIDGE_KM_DISPATCH_TABLE) += -DDEBUG_BRIDGE_KM_DISPATCH_TABLE

ccflags-$(CONFIG_PVR_PERCONTEXT_PB) += -DSUPPORT_PERCONTEXT_PB
ccflags-$(CONFIG_PVR_SGX_LOW_LATENCY_SCHEDULING) += -DSUPPORT_SGX_LOW_LATENCY_SCHEDULING
ccflags-$(CONFIG_PVR_ACTIVE_POWER_MANAGEMENT) += -DSUPPORT_ACTIVE_POWER_MANAGEMENT
ccflags-$(CONFIG_PVR_USSE_EDM_STATUS_DEBUG) += -DPVRSRV_USSE_EDM_STATUS_DEBUG
ccflags-$(CONFIG_PVR_DUMP_MK_TRACE) += -DPVRSRV_DUMP_MK_TRACE

ccflags-$(CONFIG_PVR_PDUMP) += \
	-DPDUMP -DSUPPORT_PDUMP_MULTI_PROCESS

ccflags-$(CONFIG_PVR_LINUX_MEM_AREA_POOL) += \
	-DPVR_LINUX_MEM_AREA_POOL_MAX_PAGES=CONFIG_PVR_LINUX_MEM_AREA_POOL_MAX_PAGES \
	-DPVR_LINUX_MEM_AREA_USE_VMAP -DPVR_LINUX_MEM_AREA_POOL_ALLOW_SHRINK

pvrsrvkm-y := \
	osfunc.o \
	mutils.o \
	mmap.o \
	module.o \
	pdump.o \
	proc.o \
	pvr_bridge_k.o \
	pvr_debug.o \
	mm.o \
	mutex.o \
	event.o \
	osperproc.o \
	buffer_manager.o \
	devicemem.o \
	deviceclass.o \
	handle.o \
	hash.o \
	metrics.o \
	pvrsrv.o \
	queue.o \
	ra.o \
	resman.o \
	power.o \
	mem.o \
	pdump_common.o \
	bridged_support.o \
	bridged_pvr_bridge.o \
	perproc.o \
	lists.o \
	refcount.o \
	sgx/bridged_sgx_bridge.o \
	sgx/sgxinit.o \
	sgx/sgxpower.o \
	sgx/sgxreset.o \
	sgx/sgxutils.o \
	sgx/sgxkick.o \
	sgx/sgxtransfer.o \
	sgx/mmu.o \
	sgx/pb.o

ccflags-$(CONFIG_ARCH_S5PV210) += \
	-DPVR_BUILD_DIR="\"smdkc110_android\"" \
	-Idrivers/gpu/pvr/s5pc110 \
	-DDISPLAY_CONTROLLER=s3c_lcd \
    -DSLSI_S5PC110

ccflags-$(CONFIG_ARCH_S5PV210) += \
	-DSGX540 -DSUPPORT_SGX540 \
	-DSGX_CORE_REV=120

pvrsrvkm-$(CONFIG_ARCH_S5PV210) += \
	s5pc110/sysconfig.o \
	s5pc110/sysutils.o

s3c_lcd-y := \
	s3c_lcd/s3c_displayclass.o \
	s3c_lcd/s3c_lcd.o

obj-$(CONFIG_PVR_PDUMP) += dbgdrv/

obj-$(CONFIG_PVR_SGX) += pvrsrvkm.o

obj-$(CONFIG_ARCH_S5PV210) += s3c_lcd.o
