/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 */

/* Copyright (C) Samsung Electronics System LSI. */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <plat/regs-fb.h>
#include <linux/console.h>
#include <linux/workqueue.h>
#include <linux/version.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

#include "s3c_lcd.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
#define	S3C_CONSOLE_LOCK()		console_lock()
#define	S3C_CONSOLE_UNLOCK()	console_unlock()
#else
#define	S3C_CONSOLE_LOCK()		acquire_console_sem()
#define	S3C_CONSOLE_UNLOCK()	release_console_sem()
#endif

static int fb_idx = 0;

#define S3C_MAX_BACKBUFFERS 	2
#define S3C_MAX_BUFFERS (S3C_MAX_BACKBUFFERS+1)

#define S3C_DISPLAY_FORMAT_NUM 1
#define S3C_DISPLAY_DIM_NUM 1

#define VSYNC_IRQ 0x61

#define DC_S3C_LCD_COMMAND_COUNT 1

typedef struct S3C_FRAME_BUFFER_TAG
{
	IMG_CPU_VIRTADDR bufferVAddr;
	IMG_SYS_PHYADDR bufferPAddr;
	IMG_UINT32 byteSize;
	IMG_UINT32 yoffset; //y offset from SysBuffer

} S3C_FRAME_BUFFER;

typedef void *		 S3C_HANDLE;

typedef enum tag_s3c_bool
{
	S3C_FALSE = 0,
	S3C_TRUE  = 1,
	
} S3C_BOOL, *S3C_PBOOL;

typedef struct S3C_SWAPCHAIN_TAG
{
	unsigned long   ulBufferCount;
	S3C_FRAME_BUFFER  	*psBuffer;
	
} S3C_SWAPCHAIN;

typedef struct S3C_VSYNC_FLIP_ITEM_TAG
{
	S3C_HANDLE		  hCmdComplete;
	S3C_FRAME_BUFFER	*psFb;
	unsigned long	  ulSwapInterval;
	S3C_BOOL		  bValid;
	S3C_BOOL		  bFlipped;
	S3C_BOOL		  bCmdCompleted;

} S3C_VSYNC_FLIP_ITEM;

typedef struct fb_info S3C_FB_INFO;

typedef struct S3C_LCD_DEVINFO_TAG
{
	IMG_UINT32 						ui32DeviceID;
	DISPLAY_INFO 					sDisplayInfo;
	S3C_FB_INFO 					*psFBInfo;

	// sys surface info
	S3C_FRAME_BUFFER				sSysBuffer;

	// number of supported format
	IMG_UINT32 						ui32NumFormats;
	IMG_UINT32 						ui32NumFrameBuffers;

	// list of supported display format
	DISPLAY_FORMAT 					asDisplayFormatList[S3C_DISPLAY_FORMAT_NUM];

	IMG_UINT32 						ui32NumDims;
	DISPLAY_DIMS					asDisplayDimList[S3C_DISPLAY_DIM_NUM];

	// jump table into DC
	PVRSRV_DC_SRV2DISP_KMJTABLE 	sDCJTable;

	// backbuffer info
	S3C_FRAME_BUFFER				asBackBuffers[S3C_MAX_BACKBUFFERS];

	S3C_SWAPCHAIN					*psSwapChain;

	S3C_VSYNC_FLIP_ITEM				asVSyncFlips[S3C_MAX_BUFFERS];

	unsigned long					ulInsertIndex;
	unsigned long					ulRemoveIndex;
	S3C_BOOL						bFlushCommands;

	struct workqueue_struct 		*psWorkQueue;
	struct work_struct				sWork;
	struct mutex					sVsyncFlipItemMutex;

} S3C_LCD_DEVINFO;

// jump table into pvr services
static PVRSRV_DC_DISP2SRV_KMJTABLE gsPVRJTable;

static S3C_LCD_DEVINFO *gpsLCDInfo;

/*****************************************************************************
 * Video-decode carveout decls
 */

#define S3C_MAX_VIDEO_BUFFERS	3

/* Y planes + UV planes @ max resolution, page aligned */
#define S3C_VIDEO_Y_SIZE	ALIGN(1280 * 720, PAGE_SIZE)
#define S3C_VIDEO_UV_SIZE	ALIGN(1280 * 360, PAGE_SIZE)
#define S3C_VIDEO_CARVEOUT_SIZE	\
	S3C_MAX_VIDEO_BUFFERS * (S3C_VIDEO_Y_SIZE + S3C_VIDEO_UV_SIZE)

typedef struct S3C_VIDBUF_DEVINFO_TAG
{
	IMG_UINT32			ui32DeviceID;
	S3C_SWAPCHAIN		*psSwapChain;
	DISPLAY_INFO 		sDisplayInfo;
	S3C_FRAME_BUFFER	asVideoBuffers[S3C_MAX_VIDEO_BUFFERS];

} S3C_VIDBUF_DEVINFO;

static S3C_VIDBUF_DEVINFO gsYBufInfo =
{
	.sDisplayInfo.ui32MaxSwapInterval		= 1,
	.sDisplayInfo.ui32MaxSwapChains			= 1,
	.sDisplayInfo.ui32MaxSwapChainBuffers	= S3C_MAX_VIDEO_BUFFERS,
	.sDisplayInfo.szDisplayName				= "s3c_lcd_y",
};

static S3C_VIDBUF_DEVINFO gsUVBufInfo =
{
	.sDisplayInfo.ui32MaxSwapInterval		= 1,
	.sDisplayInfo.ui32MaxSwapChains			= 1,
	.sDisplayInfo.ui32MaxSwapChainBuffers	= S3C_MAX_VIDEO_BUFFERS,
	.sDisplayInfo.szDisplayName				= "s3c_lcd_uv",
};

static int InitVidBufs(IMG_UINT32 ui32FBOffset);
static void DeinitVidBufs(void);

/****************************************************************************/

extern IMG_BOOL IMG_IMPORT PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);

static void AdvanceFlipIndex(S3C_LCD_DEVINFO *psDevInfo,
							 unsigned long	*pulIndex)
{
	unsigned long	ulMaxFlipIndex;

	ulMaxFlipIndex = psDevInfo->psSwapChain->ulBufferCount - 1;
	if (ulMaxFlipIndex >= psDevInfo->ui32NumFrameBuffers)
	{
		ulMaxFlipIndex = psDevInfo->ui32NumFrameBuffers -1;
	}

	(*pulIndex)++;

	if (*pulIndex > ulMaxFlipIndex )
	{
		*pulIndex = 0;
	}
}

static IMG_VOID ResetVSyncFlipItems(S3C_LCD_DEVINFO* psDevInfo)
{
	unsigned long i;

	psDevInfo->ulInsertIndex = 0;
	psDevInfo->ulRemoveIndex = 0;

	for(i=0; i < psDevInfo->ui32NumFrameBuffers; i++)
	{
		psDevInfo->asVSyncFlips[i].bValid = S3C_FALSE;
		psDevInfo->asVSyncFlips[i].bFlipped = S3C_FALSE;
		psDevInfo->asVSyncFlips[i].bCmdCompleted = S3C_FALSE;
	}
}

static IMG_VOID S3C_Flip(S3C_LCD_DEVINFO  *psDevInfo,
					   S3C_FRAME_BUFFER *fb)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	S3C_CONSOLE_LOCK();

	sFBVar = psDevInfo->psFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = fb->yoffset;

	ulYResVirtual = fb->yoffset + sFBVar.yres;

	if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psFBInfo, &sFBVar);
		if (res != 0)
		{
			printk("%s: fb_set_var failed (Y Offset: %d, Error: %d)\n", __FUNCTION__, fb->yoffset, res);
		}
	}
	else
	{
		res = fb_pan_display(psDevInfo->psFBInfo, &sFBVar);
		if (res != 0)
		{
			printk( "%s: fb_pan_display failed (Y Offset: %d, Error: %d)\n", __FUNCTION__, fb->yoffset, res);
		}
	}

	S3C_CONSOLE_UNLOCK();
}

static void FlushInternalVSyncQueue(S3C_LCD_DEVINFO*psDevInfo)
{
	S3C_VSYNC_FLIP_ITEM*  psFlipItem;

	mutex_lock(&psDevInfo->sVsyncFlipItemMutex);

	psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->ulRemoveIndex];

	while(psFlipItem->bValid)
	{
		if(psFlipItem->bFlipped == S3C_FALSE)
		{
			S3C_Flip (psDevInfo, psFlipItem->psFb);
		}

		if(psFlipItem->bCmdCompleted == S3C_FALSE)
		{
			gsPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_TRUE);
		}

		AdvanceFlipIndex(psDevInfo, &psDevInfo->ulRemoveIndex);

		psFlipItem->bFlipped = S3C_FALSE;
		psFlipItem->bCmdCompleted = S3C_FALSE;
		psFlipItem->bValid = S3C_FALSE;

		psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->ulRemoveIndex];
	}

	psDevInfo->ulInsertIndex = 0;
	psDevInfo->ulRemoveIndex = 0;

	mutex_unlock(&psDevInfo->sVsyncFlipItemMutex);
}

static void VsyncWorkqueueFunc(struct work_struct *psWork)
{
	S3C_VSYNC_FLIP_ITEM *psFlipItem;
	S3C_LCD_DEVINFO *psDevInfo = container_of(psWork, S3C_LCD_DEVINFO, sWork);

	if(psDevInfo == NULL)
	{
		return;
	}

	mutex_lock(&psDevInfo->sVsyncFlipItemMutex);

	psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->ulRemoveIndex];

	while(psFlipItem->bValid)
	{
		if(psFlipItem->bFlipped)
		{
			if(!psFlipItem->bCmdCompleted)
			{
				gsPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_TRUE);
				psFlipItem->bCmdCompleted = S3C_TRUE;
			}

			psFlipItem->ulSwapInterval--;

			if(psFlipItem->ulSwapInterval == 0)
			{
				AdvanceFlipIndex(psDevInfo, &psDevInfo->ulRemoveIndex);
				psFlipItem->bCmdCompleted = S3C_FALSE;
				psFlipItem->bFlipped = S3C_FALSE;
				psFlipItem->bValid = S3C_FALSE;
			}
			else
			{
				break;
			}
		}
		else
		{
			S3C_Flip (psDevInfo, psFlipItem->psFb);
			psFlipItem->bFlipped = S3C_TRUE;
			break;
		}

		psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->ulRemoveIndex];
	}

	mutex_unlock(&psDevInfo->sVsyncFlipItemMutex);
}

static S3C_BOOL CreateVsyncWorkQueue(S3C_LCD_DEVINFO *psDevInfo)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psDevInfo->psWorkQueue = alloc_workqueue("vsync_workqueue",
											 WQ_UNBOUND | WQ_HIGHPRI, 1);
#else
	psDevInfo->psWorkQueue = create_rt_workqueue("vsync_workqueue");
#endif

	if (psDevInfo->psWorkQueue == IMG_NULL)
	{
		printk("fail to create vsync_handler workqueue\n");
		return S3C_FALSE;
	}

	INIT_WORK(&psDevInfo->sWork, VsyncWorkqueueFunc);
	mutex_init(&psDevInfo->sVsyncFlipItemMutex);

	return S3C_TRUE;
}

static void destroyVsyncWorkQueue(S3C_LCD_DEVINFO *psDevInfo)
{
	destroy_workqueue(psDevInfo->psWorkQueue);
	mutex_destroy(&psDevInfo->sVsyncFlipItemMutex);
}

static irqreturn_t S3C_VSyncISR(int irq, void *dev_id)
{
	if( dev_id != gpsLCDInfo)
	{
		return IRQ_NONE;
	}

	queue_work(gpsLCDInfo->psWorkQueue, &gpsLCDInfo->sWork);

	return IRQ_HANDLED;
}

static IMG_VOID S3C_InstallVsyncISR(void)
{	
	if(request_irq(VSYNC_IRQ, S3C_VSyncISR, IRQF_SHARED , "s3cfb", gpsLCDInfo))
	{
		printk("S3C_InstallVsyncISR: Couldn't install system LISR on IRQ %d", VSYNC_IRQ);
		return;
	}
}

static IMG_VOID S3C_UninstallVsyncISR(void)
{	
	free_irq(VSYNC_IRQ, gpsLCDInfo);
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 ui32DeviceID,
								 IMG_HANDLE *phDevice,
								 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	PVR_UNREFERENCED_PARAMETER(psSystemBufferSyncData);

	if(ui32DeviceID == gpsLCDInfo->ui32DeviceID)
	{
		*phDevice = (IMG_HANDLE)gpsLCDInfo;
	}
	else if(ui32DeviceID == gsYBufInfo.ui32DeviceID)
	{
		*phDevice = (IMG_HANDLE)&gsYBufInfo;
	}
	else if(ui32DeviceID == gsUVBufInfo.ui32DeviceID)
	{
		*phDevice = (IMG_HANDLE)&gsUVBufInfo;
	}
	else
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE		hDevice,
								  IMG_UINT32		*pui32NumFormats,
								  DISPLAY_FORMAT	*psFormat)
{
	S3C_LCD_DEVINFO *psLCDInfo = (S3C_LCD_DEVINFO*)hDevice;
	int i;

	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*pui32NumFormats = S3C_DISPLAY_FORMAT_NUM;

	if(psFormat)
	{
		for (i = 0 ; i < S3C_DISPLAY_FORMAT_NUM ; i++)
			psFormat[i] = psLCDInfo->asDisplayFormatList[i];
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCDims(IMG_HANDLE		hDevice,
							   DISPLAY_FORMAT	*psFormat,
							   IMG_UINT32		*pui32NumDims,
							   DISPLAY_DIMS		*psDim)
{
	int i;

	S3C_LCD_DEVINFO *psLCDInfo = (S3C_LCD_DEVINFO*)hDevice;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*pui32NumDims = S3C_DISPLAY_DIM_NUM;

	if(psDim)
	{
		for (i = 0 ; i < S3C_DISPLAY_DIM_NUM ; i++)
			psDim[i] = psLCDInfo->asDisplayDimList[i];
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	S3C_LCD_DEVINFO *psDevInfo = (S3C_LCD_DEVINFO*)hDevice;

	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSysBuffer;
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	S3C_LCD_DEVINFO *psDevInfo = (S3C_LCD_DEVINFO*)hDevice;

	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*psDCInfo = psDevInfo->sDisplayInfo;
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE		hDevice,
									IMG_HANDLE		hBuffer,
									IMG_SYS_PHYADDR	**ppsSysAddr,
									IMG_UINT32		*pui32ByteSize,
									IMG_VOID		**ppvCpuVAddr,
									IMG_HANDLE		*phOSMapInfo,
									IMG_BOOL		*pbIsContiguous,
									IMG_UINT32		  *pui32TilingStride)
{
	S3C_FRAME_BUFFER *psBuffer = (S3C_FRAME_BUFFER *)hBuffer;

	PVR_UNREFERENCED_PARAMETER(pui32TilingStride);
	PVR_UNREFERENCED_PARAMETER(hDevice);

	if(!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*phOSMapInfo = IMG_NULL;
	*pbIsContiguous = IMG_TRUE;

	*ppvCpuVAddr = (IMG_VOID *)psBuffer->bufferVAddr;
	*ppsSysAddr = &psBuffer->bufferPAddr;
	*pui32ByteSize = psBuffer->byteSize;

	return PVRSRV_OK;
}

static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,
									  IMG_UINT32 ui32Flags,
									  DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
									  DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
									  IMG_UINT32 ui32BufferCount,
									  PVRSRV_SYNC_DATA **ppsSyncData,
									  IMG_UINT32 ui32OEMFlags,
									  IMG_HANDLE *phSwapChain,
									  IMG_UINT32 *pui32SwapChainID)
{
	IMG_UINT32 i;

	S3C_FRAME_BUFFER *psBuffer;
	S3C_SWAPCHAIN *psSwapChain;
	S3C_LCD_DEVINFO *psDevInfo = (S3C_LCD_DEVINFO*)hDevice;

	PVR_UNREFERENCED_PARAMETER(ui32OEMFlags);
	PVR_UNREFERENCED_PARAMETER(pui32SwapChainID);

	if(!hDevice || !psDstSurfAttrib || !psSrcSurfAttrib || !ppsSyncData || !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(ui32BufferCount > psDevInfo->ui32NumFrameBuffers)
	{
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	if(psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_FLIP_CHAIN_EXISTS);
	}

	psSwapChain = (S3C_SWAPCHAIN *)kmalloc(sizeof(S3C_SWAPCHAIN),GFP_KERNEL);
	if(!psSwapChain)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psBuffer = (S3C_FRAME_BUFFER*)kmalloc(sizeof(S3C_FRAME_BUFFER) * ui32BufferCount, GFP_KERNEL);
	if(!psBuffer)
	{
		kfree(psSwapChain);
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;

	psBuffer[0].bufferPAddr = psDevInfo->sSysBuffer.bufferPAddr;
	psBuffer[0].bufferVAddr = psDevInfo->sSysBuffer.bufferVAddr;
	psBuffer[0].byteSize = psDevInfo->sSysBuffer.byteSize;
	psBuffer[0].yoffset = psDevInfo->sSysBuffer.yoffset;

	for (i=1; i<ui32BufferCount; i++)
	{
		psBuffer[i].bufferPAddr = psDevInfo->asBackBuffers[i-1].bufferPAddr;
		psBuffer[i].bufferVAddr = psDevInfo->asBackBuffers[i-1].bufferVAddr;
		psBuffer[i].byteSize = psDevInfo->asBackBuffers[i-1].byteSize;
		psBuffer[i].yoffset = psDevInfo->asBackBuffers[i-1].yoffset;
	}

	*phSwapChain = (IMG_HANDLE)psSwapChain;
	*pui32SwapChainID =(IMG_UINT32)psSwapChain;	

	psDevInfo->psSwapChain = psSwapChain;

    ResetVSyncFlipItems(psDevInfo);
	S3C_InstallVsyncISR();

	return PVRSRV_OK;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
									   IMG_HANDLE hSwapChain)
{
	S3C_SWAPCHAIN *sc = (S3C_SWAPCHAIN *)hSwapChain;
	S3C_LCD_DEVINFO *psLCDInfo = (S3C_LCD_DEVINFO*)hDevice;

	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	FlushInternalVSyncQueue(psLCDInfo);

	S3C_Flip(psLCDInfo, &psLCDInfo->sSysBuffer);

	kfree(sc->psBuffer);
	kfree(sc);

	if (psLCDInfo->psSwapChain == sc)
		psLCDInfo->psSwapChain = NULL;	

	ResetVSyncFlipItems(psLCDInfo);

	S3C_UninstallVsyncISR();

	return PVRSRV_OK;
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE	hDevice,
								 IMG_HANDLE	hSwapChain,
								 IMG_RECT	*psRect)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(psRect);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE	hDevice,
								 IMG_HANDLE	hSwapChain,
								 IMG_RECT	*psRect)
{

	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(psRect);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE	hDevice,
									  IMG_HANDLE	hSwapChain,
									  IMG_UINT32	ui32CKColour)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(ui32CKColour);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE	hDevice,
									  IMG_HANDLE	hSwapChain,
									  IMG_UINT32	ui32CKColour)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(ui32CKColour);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static IMG_VOID S3CSetState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	S3C_LCD_DEVINFO	*psDevInfo;

	psDevInfo = (S3C_LCD_DEVINFO*)hDevice;

	if (ui32State == DC_STATE_FLUSH_COMMANDS)
	{
		if (psDevInfo->psSwapChain != 0)
		{
			FlushInternalVSyncQueue(psDevInfo);
		}

		psDevInfo->bFlushCommands =S3C_TRUE;
	}
	else if (ui32State == DC_STATE_NO_FLUSH_COMMANDS)
	{
		psDevInfo->bFlushCommands = S3C_FALSE;
	}
}

static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
								 IMG_HANDLE hSwapChain,
								 IMG_UINT32 *pui32BufferCount,
								 IMG_HANDLE *phBuffer)
{
	S3C_LCD_DEVINFO *psLCDInfo = (S3C_LCD_DEVINFO*)hDevice;
	int	i;

	if(!hDevice || !hSwapChain || !pui32BufferCount || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*pui32BufferCount = psLCDInfo->psSwapChain->ulBufferCount;

	phBuffer[0] = (IMG_HANDLE)(&(psLCDInfo->sSysBuffer));
	for (i=0; i < (*pui32BufferCount) - 1; i++)
	{
		phBuffer[i+1] = (IMG_HANDLE)&psLCDInfo->asBackBuffers[i];
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE	hDevice,
								   IMG_HANDLE	hBuffer,
								   IMG_UINT32	ui32SwapInterval,
								   IMG_HANDLE	hPrivateTag,
								   IMG_UINT32	ui32ClipRectCount,
								   IMG_RECT		*psClipRect)
{
	PVR_UNREFERENCED_PARAMETER(ui32SwapInterval);
	PVR_UNREFERENCED_PARAMETER(hPrivateTag);
	PVR_UNREFERENCED_PARAMETER(psClipRect);

	if(!hDevice || !hBuffer || ui32ClipRectCount != 0)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE hDevice,
								   IMG_HANDLE hSwapChain)
{

	if(!hDevice	|| !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  S3C_LCD_DEVINFO *psDevInfo,
							  S3C_FRAME_BUFFER *psFb,
							  IMG_UINT32 ui32SwapInterval)
{
	S3C_VSYNC_FLIP_ITEM *psFlipItem;

	if(ui32SwapInterval == 0)
	{
		S3C_Flip(psDevInfo, psFb);
		gsPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);
		return IMG_TRUE;
	}

	mutex_lock(&psDevInfo->sVsyncFlipItemMutex);

	psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->ulInsertIndex];

	if(psFlipItem->bValid)
	{
		mutex_unlock(&psDevInfo->sVsyncFlipItemMutex);
		return IMG_FALSE;
	}

	if(psDevInfo->ulInsertIndex == psDevInfo->ulRemoveIndex)
	{
		S3C_Flip(psDevInfo, psFb);
		psFlipItem->bFlipped = S3C_TRUE;
	}
	else
	{
		psFlipItem->bFlipped = S3C_FALSE;
	}

	psFlipItem->hCmdComplete = hCmdCookie;
	psFlipItem->psFb = psFb;
	psFlipItem->ulSwapInterval = (unsigned long)ui32SwapInterval;

	psFlipItem->bValid = S3C_TRUE;

	AdvanceFlipIndex(psDevInfo, &psDevInfo->ulInsertIndex);

	mutex_unlock(&psDevInfo->sVsyncFlipItemMutex);
	return IMG_TRUE;
}

static IMG_BOOL ProcessFlip(IMG_HANDLE	hCmdCookie,
							IMG_UINT32	ui32DataSize,
							IMG_VOID	*pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	S3C_LCD_DEVINFO *psDevInfo;

	/* Check parameters  */
	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	/* Validate data packet  */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		return IMG_FALSE;
	}

	psDevInfo = (S3C_LCD_DEVINFO*)psFlipCmd->hExtDevice;

	if (psDevInfo->bFlushCommands)
	{
		gsPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);
		return IMG_TRUE;
	}

	return ProcessFlipV1(hCmdCookie,
						 psDevInfo,
						 psFlipCmd->hExtBuffer,
						 psFlipCmd->ui32SwapInterval);
}

static S3C_BOOL InitDev(struct fb_info **s3c_fb_Info)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	S3C_BOOL eError = S3C_TRUE;

	S3C_CONSOLE_LOCK();

	if (fb_idx < 0 || fb_idx >= num_registered_fb)
	{
		eError = S3C_FALSE;
		goto errRelSem;
	}

	psLINFBInfo = registered_fb[fb_idx];

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		printk("Couldn't get framebuffer module\n");
		eError = S3C_FALSE;
		goto errRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(psLINFBInfo, 0);
		if (res != 0)
		{
			printk("Couldn't open framebuffer: %d\n", res);
			eError = S3C_FALSE;
			goto errModPut;
		}
	}

	*s3c_fb_Info = psLINFBInfo;

errModPut:
	module_put(psLINFBOwner);
errRelSem:
	S3C_CONSOLE_UNLOCK();

	return eError;
}

static void DeInitDev(S3C_LCD_DEVINFO  *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psFBInfo;
	struct module *psLINFBOwner;

	S3C_CONSOLE_LOCK();

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	S3C_CONSOLE_UNLOCK();
}

int s3c_displayclass_init(void)
{
	IMG_UINT32 aui32SyncCountList[DC_S3C_LCD_COMMAND_COUNT][2];
	PFN_CMD_PROC pfnCmdProcList[DC_S3C_LCD_COMMAND_COUNT];
	int rgb_format, bytes_per_pixel, bits_per_pixel;
	IMG_UINT32 num_of_fb, num_of_backbuffer;
	IMG_UINT32 byteSize, ui32FBOffset = 0;
	IMG_UINT32 pa_fb, va_fb, fb_size;
	struct fb_info *psLINFBInfo = 0;
	IMG_UINT32 screen_w, screen_h;
	int	i;

	if(InitDev(&psLINFBInfo) == S3C_FALSE)
	{
		return 0;
	}

	pa_fb = psLINFBInfo->fix.smem_start;
	va_fb = (unsigned long)phys_to_virt(psLINFBInfo->fix.smem_start);
	screen_w = psLINFBInfo->var.xres;
	screen_h = psLINFBInfo->var.yres;
	bits_per_pixel = psLINFBInfo->var.bits_per_pixel;
	fb_size = psLINFBInfo->fix.smem_len;

	switch (bits_per_pixel)
	{
	case 16:
		rgb_format = PVRSRV_PIXEL_FORMAT_RGB565;
		bytes_per_pixel = 2;
		break;
	case 32:
		rgb_format = PVRSRV_PIXEL_FORMAT_ARGB8888;
		bytes_per_pixel = 4;
		break;
	default:
		rgb_format = PVRSRV_PIXEL_FORMAT_ARGB8888;
		bytes_per_pixel = 4;
		break;
	}

	printk("PA FB = 0x%X, bits per pixel = %d\n", (unsigned int)pa_fb, (unsigned int)bits_per_pixel);
	printk("screen width=%d height=%d va=0x%x pa=0x%x\n", (int)screen_w, (int)screen_h, (unsigned int)va_fb, (unsigned int)pa_fb);
	printk("xres_virtual = %d, yres_virtual = %d, xoffset = %d, yoffset = %d\n", psLINFBInfo->var.xres_virtual,  psLINFBInfo->var.yres_virtual,  psLINFBInfo->var.xoffset,  psLINFBInfo->var.yoffset);
	printk("fb_size=%d\n", (int)fb_size);

	/* We'll share the framebuffer region with video decode buffers,
	 * so we need to make sure all the frame buffers are page aligned.
	 */
	BUG_ON(pa_fb != ALIGN(pa_fb, PAGE_SIZE));
	byteSize = ALIGN(screen_w * screen_h * bytes_per_pixel, PAGE_SIZE);

	num_of_fb = fb_size / byteSize;
	if(num_of_fb > S3C_MAX_BUFFERS)
		num_of_fb = S3C_MAX_BUFFERS;

	num_of_backbuffer = num_of_fb - 1;

	if (gpsLCDInfo != NULL)
		goto exit_out;

	gpsLCDInfo = (S3C_LCD_DEVINFO*)kmalloc(sizeof(S3C_LCD_DEVINFO),GFP_KERNEL);

	gpsLCDInfo->psFBInfo = psLINFBInfo;
	gpsLCDInfo->ui32NumFrameBuffers = num_of_fb;

	gpsLCDInfo->ui32NumFormats = S3C_DISPLAY_FORMAT_NUM;

	gpsLCDInfo->asDisplayFormatList[0].pixelformat = rgb_format;
	gpsLCDInfo->ui32NumDims = S3C_DISPLAY_DIM_NUM;
	gpsLCDInfo->asDisplayDimList[0].ui32ByteStride = (bytes_per_pixel) * screen_w;
	gpsLCDInfo->asDisplayDimList[0].ui32Height = screen_h;
	gpsLCDInfo->asDisplayDimList[0].ui32Width = screen_w;

	gpsLCDInfo->sSysBuffer.bufferPAddr.uiAddr = pa_fb;
	gpsLCDInfo->sSysBuffer.bufferVAddr = (IMG_CPU_VIRTADDR)va_fb;
	gpsLCDInfo->sSysBuffer.yoffset = 0;
	gpsLCDInfo->sSysBuffer.byteSize = (IMG_UINT32)byteSize;
	ui32FBOffset += byteSize;

	for (i=0 ; i < num_of_backbuffer; i++)
	{
		gpsLCDInfo->asBackBuffers[i].byteSize = gpsLCDInfo->sSysBuffer.byteSize;
		gpsLCDInfo->asBackBuffers[i].bufferPAddr.uiAddr = pa_fb + byteSize * (i+1);
		gpsLCDInfo->asBackBuffers[i].bufferVAddr = (IMG_CPU_VIRTADDR)phys_to_virt(gpsLCDInfo->asBackBuffers[i].bufferPAddr.uiAddr);
		gpsLCDInfo->asBackBuffers[i].yoffset = screen_h * (i + 1);
		ui32FBOffset += byteSize;

		printk("Back frameBuffer[%d].VAddr=%p PAddr=%p size=%d\n",
			i, 
			(void*)gpsLCDInfo->asBackBuffers[i].bufferVAddr,
			(void*)gpsLCDInfo->asBackBuffers[i].bufferPAddr.uiAddr,
			(int)gpsLCDInfo->asBackBuffers[i].byteSize);
	}

	gpsLCDInfo->bFlushCommands = S3C_FALSE;
	gpsLCDInfo->psSwapChain = NULL;

	PVRGetDisplayClassJTable(&gsPVRJTable);

	gpsLCDInfo->sDCJTable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	gpsLCDInfo->sDCJTable.pfnOpenDCDevice = OpenDCDevice;
	gpsLCDInfo->sDCJTable.pfnCloseDCDevice = CloseDCDevice;
	gpsLCDInfo->sDCJTable.pfnEnumDCFormats = EnumDCFormats;
	gpsLCDInfo->sDCJTable.pfnEnumDCDims = EnumDCDims;
	gpsLCDInfo->sDCJTable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
	gpsLCDInfo->sDCJTable.pfnGetDCInfo = GetDCInfo;
	gpsLCDInfo->sDCJTable.pfnGetBufferAddr = GetDCBufferAddr;
	gpsLCDInfo->sDCJTable.pfnCreateDCSwapChain = CreateDCSwapChain;
	gpsLCDInfo->sDCJTable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
	gpsLCDInfo->sDCJTable.pfnSetDCDstRect = SetDCDstRect;
	gpsLCDInfo->sDCJTable.pfnSetDCSrcRect = SetDCSrcRect;
	gpsLCDInfo->sDCJTable.pfnSetDCDstColourKey = SetDCDstColourKey;
	gpsLCDInfo->sDCJTable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
	gpsLCDInfo->sDCJTable.pfnGetDCBuffers = GetDCBuffers;
	gpsLCDInfo->sDCJTable.pfnSwapToDCBuffer = SwapToDCBuffer;
	gpsLCDInfo->sDCJTable.pfnSwapToDCSystem = SwapToDCSystem;
	gpsLCDInfo->sDCJTable.pfnSetDCState = S3CSetState;

	gpsLCDInfo->sDisplayInfo.ui32MinSwapInterval=0;
	gpsLCDInfo->sDisplayInfo.ui32MaxSwapInterval=1;
	gpsLCDInfo->sDisplayInfo.ui32MaxSwapChains=1;
	gpsLCDInfo->sDisplayInfo.ui32MaxSwapChainBuffers = num_of_fb;
	gpsLCDInfo->sDisplayInfo.ui32PhysicalWidthmm= psLINFBInfo->var.width;// width of lcd in mm 
	gpsLCDInfo->sDisplayInfo.ui32PhysicalHeightmm= psLINFBInfo->var.height;// height of lcd in mm 

	strncpy(gpsLCDInfo->sDisplayInfo.szDisplayName, "s3c_lcd", MAX_DISPLAY_NAME_SIZE);

	if(ui32FBOffset + S3C_VIDEO_CARVEOUT_SIZE <= fb_size)
	{
		if(InitVidBufs(ui32FBOffset))
			return 1;
	}
	else
	{
		printk("No space for NV12 video carveout\n");
	}

	if(gsPVRJTable.pfnPVRSRVRegisterDCDevice(&gpsLCDInfo->sDCJTable,
		&gpsLCDInfo->ui32DeviceID) != PVRSRV_OK)
	{
		return 1;
	}

	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0;
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 2;

	if (gsPVRJTable.pfnPVRSRVRegisterCmdProcList(gpsLCDInfo->ui32DeviceID,
		&pfnCmdProcList[0], aui32SyncCountList, DC_S3C_LCD_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk("failing register commmand proc list deviceID:%d\n",(int)gpsLCDInfo->ui32DeviceID);
		return PVRSRV_ERROR_CANT_REGISTER_CALLBACK;
	}

	if(CreateVsyncWorkQueue(gpsLCDInfo) == S3C_FALSE)
	{
		printk("fail to CreateVsyncWorkQueue\n");
		return 1;
	}

exit_out:
	return 0;
}

void s3c_displayclass_deinit(void)
{
	destroyVsyncWorkQueue(gpsLCDInfo);
	DeInitDev(gpsLCDInfo);

	gsPVRJTable.pfnPVRSRVRemoveCmdProcList(gpsLCDInfo->ui32DeviceID,
										   DC_S3C_LCD_COMMAND_COUNT);

	DeinitVidBufs();

	gsPVRJTable.pfnPVRSRVRemoveDCDevice(gpsLCDInfo->ui32DeviceID);

	kfree(gpsLCDInfo);
	gpsLCDInfo = NULL;
}

/*****************************************************************************
 * Video-decode carveout workaround starts here
 */

static PVRSRV_ERROR EnumVidBufFormats(IMG_HANDLE hDevice,
									  IMG_UINT32 *pui32NumFormats,
									  DISPLAY_FORMAT *psFormat)
{
	DISPLAY_FORMAT sVidBufFormat = {
		/* Fake format to keep PVR2D happy */
		.pixelformat = PVRSRV_PIXEL_FORMAT_ARGB8888,
	};

	PVR_UNREFERENCED_PARAMETER(hDevice);

	if(pui32NumFormats)
		*pui32NumFormats = 1;

	if(psFormat)
		*psFormat = sVidBufFormat;

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumVidBufDims(IMG_HANDLE hDevice,
								   DISPLAY_FORMAT *psFormat,
								   IMG_UINT32 *pui32NumDims,
								   DISPLAY_DIMS *psDim)
{
	DISPLAY_DIMS sVidBufDim = {
        .ui32Width = 1280,
        .ui32Height = 720,
		.ui32ByteStride = 1280,
	};

	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(psFormat);

	if((S3C_VIDBUF_DEVINFO *)hDevice == &gsUVBufInfo)
		sVidBufDim.ui32Height /= 2;

	if(pui32NumDims)
		*pui32NumDims = 1;

	if(psDim)
		psDim[0] = sVidBufDim;

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetVidBufSystemBuffer(IMG_HANDLE hDevice,
										  IMG_HANDLE *phBuffer)
{
	S3C_VIDBUF_DEVINFO *psDevInfo = (S3C_VIDBUF_DEVINFO *)hDevice;

	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* FIXME: Is this really necessary? */
	*phBuffer = (IMG_HANDLE)&psDevInfo->asVideoBuffers[0];
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetVidBufInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	S3C_VIDBUF_DEVINFO *psDevInfo = (S3C_VIDBUF_DEVINFO*)hDevice;

	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*psDCInfo = psDevInfo->sDisplayInfo;
	return PVRSRV_OK;
}

static PVRSRV_ERROR
CreateVidBufSwapChain(IMG_HANDLE hDevice,
					  IMG_UINT32 ui32Flags,
					  DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
					  DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
					  IMG_UINT32 ui32BufferCount,
					  PVRSRV_SYNC_DATA **ppsSyncData,
					  IMG_UINT32 ui32OEMFlags,
					  IMG_HANDLE *phSwapChain,
					  IMG_UINT32 *pui32SwapChainID)
{
	S3C_VIDBUF_DEVINFO *psDevInfo = (S3C_VIDBUF_DEVINFO*)hDevice;
	S3C_FRAME_BUFFER *psBuffer;
	S3C_SWAPCHAIN *psSwapChain;
	IMG_UINT32 i;

	PVR_UNREFERENCED_PARAMETER(pui32SwapChainID);
	PVR_UNREFERENCED_PARAMETER(psDstSurfAttrib);
	PVR_UNREFERENCED_PARAMETER(psSrcSurfAttrib);
	PVR_UNREFERENCED_PARAMETER(ui32OEMFlags);
	PVR_UNREFERENCED_PARAMETER(ppsSyncData);

	if(!hDevice || !phSwapChain || (psDevInfo != &gsYBufInfo &&
									psDevInfo != &gsUVBufInfo))
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(ui32BufferCount > S3C_MAX_VIDEO_BUFFERS)
	{
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	if(psDevInfo->psSwapChain)
	{
		return PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
	}

	psSwapChain = (S3C_SWAPCHAIN *)kmalloc(sizeof(S3C_SWAPCHAIN), GFP_KERNEL);
	if(!psSwapChain)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psBuffer = (S3C_FRAME_BUFFER *)kmalloc(sizeof(S3C_FRAME_BUFFER) * ui32BufferCount, GFP_KERNEL);
	if(!psBuffer)
	{
		kfree(psSwapChain);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;

	for (i = 0; i < ui32BufferCount; i++)
	{
		psBuffer[i] = psDevInfo->asVideoBuffers[i];
	}

	*phSwapChain = (IMG_HANDLE)psSwapChain;
	*pui32SwapChainID =(IMG_UINT32)psSwapChain;	
	psDevInfo->psSwapChain = psSwapChain;

	return PVRSRV_OK;
}

static PVRSRV_ERROR DestroyVidBufSwapChain(IMG_HANDLE hDevice,
										   IMG_HANDLE hSwapChain)
{
	S3C_VIDBUF_DEVINFO *psDevInfo = (S3C_VIDBUF_DEVINFO*)hDevice;

	if(!psDevInfo || hSwapChain != (IMG_HANDLE)psDevInfo->psSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	kfree(psDevInfo->psSwapChain->psBuffer);

	kfree(psDevInfo->psSwapChain);
	psDevInfo->psSwapChain = NULL;

	return PVRSRV_OK;
}

static IMG_VOID SetVidBufState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(ui32State);
}

static PVRSRV_ERROR GetVidBufBuffers(IMG_HANDLE hDevice,
									 IMG_HANDLE hSwapChain,
									 IMG_UINT32 *pui32BufferCount,
									 IMG_HANDLE *phBuffer)
{
	S3C_VIDBUF_DEVINFO *psDevInfo = (S3C_VIDBUF_DEVINFO*)hDevice;
	int	i;

	if(!hDevice || !hSwapChain || !pui32BufferCount || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*pui32BufferCount = psDevInfo->psSwapChain->ulBufferCount;

	for (i = 0; i < *pui32BufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psDevInfo->asVideoBuffers[i];
	}

	return PVRSRV_OK;
}

static PVRSRV_DC_SRV2DISP_KMJTABLE gsYUVDCJTable =
{
	.ui32TableSize			= sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE),

	/* These understand video buffers, or are no-ops */
	.pfnOpenDCDevice		= OpenDCDevice,
	.pfnCloseDCDevice		= CloseDCDevice,
	.pfnSetDCDstRect		= SetDCDstRect,
	.pfnSetDCSrcRect		= SetDCSrcRect,
	.pfnSetDCDstColourKey	= SetDCDstColourKey,
	.pfnSetDCSrcColourKey	= SetDCSrcColourKey,
	.pfnSwapToDCBuffer		= SwapToDCBuffer,
	.pfnSwapToDCSystem		= SwapToDCSystem,
	.pfnGetBufferAddr		= GetDCBufferAddr,

	/* These return PVRSRV_ERROR_NOT_SUPPORTED */
	.pfnGetDCSystemBuffer	= GetVidBufSystemBuffer,
	.pfnGetDCInfo			= GetVidBufInfo,

	/* These work as documented */
	.pfnEnumDCFormats		= EnumVidBufFormats,
	.pfnEnumDCDims			= EnumVidBufDims,
	.pfnCreateDCSwapChain	= CreateVidBufSwapChain,
	.pfnDestroyDCSwapChain	= DestroyVidBufSwapChain,
	.pfnGetDCBuffers		= GetVidBufBuffers,
	.pfnSetDCState			= SetVidBufState,
};

static int InitVidBufs(IMG_UINT32 ui32FBOffset)
{
	int i;

	if(gsPVRJTable.pfnPVRSRVRegisterDCDevice(&gsYUVDCJTable,
		&gsYBufInfo.ui32DeviceID) != PVRSRV_OK)
	{
		printk("Failed to register YBuf device!\n");
		return 1;
	}

	if(gsPVRJTable.pfnPVRSRVRegisterDCDevice(&gsYUVDCJTable,
		&gsUVBufInfo.ui32DeviceID) != PVRSRV_OK)
	{
		printk("Failed to register UVBuf device!\n");
		return 1;
	}

	for (i = 0; i < S3C_MAX_VIDEO_BUFFERS; i++)
	{
		gsYBufInfo.asVideoBuffers[i].bufferPAddr.uiAddr =
			gpsLCDInfo->sSysBuffer.bufferPAddr.uiAddr + ui32FBOffset +
			(i * S3C_VIDEO_Y_SIZE);
		gsYBufInfo.asVideoBuffers[i].bufferVAddr =
			gpsLCDInfo->sSysBuffer.bufferVAddr + ui32FBOffset +
			(i * S3C_VIDEO_Y_SIZE);
		gsYBufInfo.asVideoBuffers[i].byteSize = S3C_VIDEO_Y_SIZE;
		gsYBufInfo.asVideoBuffers[i].yoffset = 0;

		printk("Video Y Buffer[%d].VAddr=%p PAddr=%p size=%d\n",
			i, 
			(void*)gsYBufInfo.asVideoBuffers[i].bufferVAddr,
			(void*)gsYBufInfo.asVideoBuffers[i].bufferPAddr.uiAddr,
			(int)gsYBufInfo.asVideoBuffers[i].byteSize);
	}

	for (i = 0; i < S3C_MAX_VIDEO_BUFFERS; i++)
	{
		gsUVBufInfo.asVideoBuffers[i].bufferPAddr.uiAddr =
			gpsLCDInfo->sSysBuffer.bufferPAddr.uiAddr + ui32FBOffset +
			(S3C_MAX_VIDEO_BUFFERS * S3C_VIDEO_Y_SIZE) +
			(i * S3C_VIDEO_UV_SIZE);
		gsUVBufInfo.asVideoBuffers[i].bufferVAddr =
			gpsLCDInfo->sSysBuffer.bufferVAddr + ui32FBOffset +
			(S3C_MAX_VIDEO_BUFFERS * S3C_VIDEO_Y_SIZE) +
			(i * S3C_VIDEO_UV_SIZE);
		gsUVBufInfo.asVideoBuffers[i].byteSize = S3C_VIDEO_UV_SIZE;
		gsUVBufInfo.asVideoBuffers[i].yoffset = 0;

		printk("Video UV Buffer[%d].VAddr=%p PAddr=%p size=%d\n",
			i, 
			(void*)gsUVBufInfo.asVideoBuffers[i].bufferVAddr,
			(void*)gsUVBufInfo.asVideoBuffers[i].bufferPAddr.uiAddr,
			(int)gsUVBufInfo.asVideoBuffers[i].byteSize);
	}

	return 0;
}

static void DeinitVidBufs(void)
{
	if(gsUVBufInfo.ui32DeviceID)
	{
		gsPVRJTable.pfnPVRSRVRemoveDCDevice(gsUVBufInfo.ui32DeviceID);
	}

	if(gsYBufInfo.ui32DeviceID)
	{
		gsPVRJTable.pfnPVRSRVRemoveDCDevice(gsYBufInfo.ui32DeviceID);
	}
}
