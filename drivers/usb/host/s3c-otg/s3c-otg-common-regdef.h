/****************************************************************************
*  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
*
*  [File Name]   : s3c-otg-common-regdef.h
*  [Description] :
*
*  [Author]      : Kyu Hyeok Jang { kyuhyeok.jang@samsung.com }
*  [Department]  : System LSI Division/Embedded Software Center
*  [Created Date]: 2007/12/15
*  [Revision History]
*      (1) 2007/12/15   by Kyu Hyeok Jang { kyuhyeok.jang@samsung.com }
*          - Created
*
****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef _OTG_REG_DEF_H
#define _OTG_REG_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
	u32	OPHYPWR;
	u32	OPHYCLK;
	u32 	ORSTCON;
}OTG_PHY_REG, *PS_OTG_PHY_REG;

#define GOTGCTL				0x000		// OTG Control & Status
#define GOTGINT				0x004		// OTG Interrupt
#define GAHBCFG				0x008		// Core AHB Configuration
#define GUSBCFG				0x00C		// Core USB Configuration
#define GRSTCTL				0x010		// Core Reset
#define GINTSTS				0x014		// Core Interrupt
#define GINTMSK				0x018		// Core Interrupt Mask
#define GRXSTSR				0x01C		// Receive Status Debug Read/Status Read
#define GRXSTSP				0x020		// Receive Status Debug Pop/Status Pop
#define GRXFSIZ				0x024		// Receive FIFO Size
#define GNPTXFSIZ				0x028		// Non-Periodic Transmit FIFO Size
#define GNPTXSTS				0x02C		// Non-Periodic Transmit FIFO/Queue Status
#define GPVNDCTL				0x034		// PHY Vendor Control
#define GGPIO					0x038		// General Purpose I/O
#define GUID					0x03C		// User ID
#define GSNPSID				0x040		// Synopsys ID
#define GHWCFG1				0x044		// User HW Config1
#define GHWCFG2				0x048		// User HW Config2
#define GHWCFG3				0x04C		// User HW Config3
#define GHWCFG4				0x050		// User HW Config4

#define HPTXFSIZ				0x100		// Host Periodic Transmit FIFO Size
#define DPTXFSIZ1				0x104		// Device Periodic Transmit FIFO-1 Size
#define DPTXFSIZ2				0x108		// Device Periodic Transmit FIFO-2 Size
#define DPTXFSIZ3				0x10C		// Device Periodic Transmit FIFO-3 Size
#define DPTXFSIZ4				0x110		// Device Periodic Transmit FIFO-4 Size
#define DPTXFSIZ5				0x114		// Device Periodic Transmit FIFO-5 Size
#define DPTXFSIZ6				0x118		// Device Periodic Transmit FIFO-6 Size
#define DPTXFSIZ7				0x11C		// Device Periodic Transmit FIFO-7 Size
#define DPTXFSIZ8				0x120		// Device Periodic Transmit FIFO-8 Size
#define DPTXFSIZ9				0x124		// Device Periodic Transmit FIFO-9 Size
#define DPTXFSIZ10				0x128		// Device Periodic Transmit FIFO-10 Size
#define DPTXFSIZ11				0x12C		// Device Periodic Transmit FIFO-11 Size
#define DPTXFSIZ12				0x130		// Device Periodic Transmit FIFO-12 Size
#define DPTXFSIZ13				0x134		// Device Periodic Transmit FIFO-13 Size
#define DPTXFSIZ14				0x138		// Device Periodic Transmit FIFO-14 Size
#define DPTXFSIZ15				0x13C		// Device Periodic Transmit FIFO-15 Size

//*********************************************************************
// Host Mode Registers
//*********************************************************************
// Host Global Registers

// Channel specific registers
#define HCCHAR_ADDR	0x500
#define HCCHAR(n)		0x500 + ((n)*0x20)
#define HCSPLT(n)		0x504 + ((n)*0x20)
#define HCINT(n)			0x508 + ((n)*0x20)
#define HCINTMSK(n)		0x50C + ((n)*0x20)
#define HCTSIZ(n)		0x510 + ((n)*0x20)
#define HCDMA(n)			0x514 + ((n)*0x20)

#define HCFG					0x400		// Host Configuration
#define HFIR					0x404		// Host Frame Interval
#define HFNUM				0x408		// Host Frame Number/Frame Time Remaining
#define HPTXSTS				0x410		// Host Periodic Transmit FIFO/Queue Status
#define HAINT				0x414		// Host All Channels Interrupt
#define HAINTMSK 			0x418		// Host All Channels Interrupt Mask

// Host Port Control & Status Registers

#define HPRT					0x440		// Host Port Control & Status

// Device Logical Endpoints-Specific Registers

#define DIEPCTL 				0x900					// Device IN Endpoint 0 Control
#define DOEPCTL(n) 			0xB00 + ((n)*0x20))		// Device OUT Endpoint 0 Control
#define DIEPINT(n) 			0x908 + ((n)*0x20))		// Device IN Endpoint 0 Interrupt
#define DOEPINT(n) 			0xB08 + ((n)*0x20))		// Device OUT Endpoint 0 Interrupt
#define DIEPTSIZ(n) 			0x910 + ((n)*0x20))		// Device IN Endpoint 0 Transfer Size
#define DOEPTSIZ(n) 			0xB10 + ((n)*0x20))		// Device OUT Endpoint 0 Transfer Size
#define DIEPDMA(n)			0x914 + ((n)*0x20))		// Device IN Endpoint 0 DMA Address
#define DOEPDMA(n) 			0xB14 + ((n)*0x20))		// Device OUT Endpoint 0 DMA Address

#define EP_FIFO(n)			0x1000 + ((n)*0x1000))

#define PCGCCTL				0x0E00

//
#define BASE_REGISTER_OFFSET	0x0
#define REGISTER_SET_SIZE		0x200

// Power Reg Bits
#define USB_RESET					0x8
#define MCU_RESUME					0x4
#define SUSPEND_MODE				0x2
#define SUSPEND_MODE_ENABLE_CTRL	0x1

// EP0 CSR
#define EP0_OUT_PACKET_RDY		0x1
#define EP0_IN_PACKET_RDY			0x2
#define EP0_SENT_STALL			0x4
#define DATA_END					0x8
#define SETUP_END					0x10
#define EP0_SEND_STALL			0x20
#define SERVICED_OUT_PKY_RDY		0x40
#define SERVICED_SETUP_END		0x80

// IN_CSR1_REG Bit definitions
#define IN_PACKET_READY			0x1
#define UNDER_RUN					0x4   // Iso Mode Only
#define FLUSH_IN_FIFO				0x8
#define IN_SEND_STALL				0x10
#define IN_SENT_STALL				0x20
#define IN_CLR_DATA_TOGGLE		0x40

// OUT_CSR1_REG Bit definitions
#define OUT_PACKET_READY			0x1
#define FLUSH_OUT_FIFO			0x10
#define OUT_SEND_STALL			0x20
#define OUT_SENT_STALL			0x40
#define OUT_CLR_DATA_TOGGLE		0x80

// IN_CSR2_REG Bit definitions
#define IN_DMA_INT_DISABLE		0x10
#define SET_MODE_IN				0x20

#define EPTYPE						(0x3<<18)
#define SET_TYPE_CONTROL			(0x0<<18)
#define SET_TYPE_ISO				(0x1<<18)
#define SET_TYPE_BULK				(0x2<<18)
#define SET_TYPE_INTERRUPT		(0x3<<18)

#define AUTO_MODE					0x80

// OUT_CSR2_REG Bit definitions
#define AUTO_CLR					0x40
#define OUT_DMA_INT_DISABLE		0x20

// Can be used for Interrupt and Interrupt Enable Reg - common bit def
#define EP0_IN_INT                	(0x1<<0)
#define EP1_IN_INT                	(0x1<<1)
#define EP2_IN_INT                	(0x1<<2)
#define EP3_IN_INT                	(0x1<<3)
#define EP4_IN_INT                	(0x1<<4)
#define EP5_IN_INT               	(0x1<<5)
#define EP6_IN_INT                	(0x1<<6)
#define EP7_IN_INT                	(0x1<<7)
#define EP8_IN_INT                	(0x1<<8)
#define EP9_IN_INT                	(0x1<<9)
#define EP10_IN_INT              	(0x1<<10)
#define EP11_IN_INT             	(0x1<<11)
#define EP12_IN_INT              	(0x1<<12)
#define EP13_IN_INT              	(0x1<<13)
#define EP14_IN_INT              	(0x1<<14)
#define EP15_IN_INT				(0x1<<15)
#define EP0_OUT_INT				(0x1<<16)
#define EP1_OUT_INT				(0x1<<17)
#define EP2_OUT_INT				(0x1<<18)
#define EP3_OUT_INT				(0x1<<19)
#define EP4_OUT_INT				(0x1<<20)
#define EP5_OUT_INT				(0x1<<21)
#define EP6_OUT_INT				(0x1<<22)
#define EP7_OUT_INT				(0x1<<23)
#define EP8_OUT_INT				(0x1<<24)
#define EP9_OUT_INT				(0x1<<25)
#define EP10_OUT_INT				(0x1<<26)
#define EP11_OUT_INT				(0x1<<27)
#define EP12_OUT_INT				(0x1<<28)
#define EP13_OUT_INT				(0x1<<29)
#define EP14_OUT_INT				(0x1<<30)
#define EP15_OUT_INT				(0x1<<31)

// GOTGINT
#define SesEndDet				(0x1<<2)

// GRSTCTL
#define TxFFlsh					(0x1<<5)
#define RxFFlsh					(0x1<<4)
#define INTknQFlsh				(0x1<<3)
#define FrmCntrRst				(0x1<<2)
#define HSftRst					(0x1<<1)
#define CSftRst					(0x1<<0)

#define CLEAR_ALL_EP_INTRS        0xffffffff

#define  EP_INTERRUPT_DISABLE_ALL   	0x0   // Bits to write to EP_INT_EN_REG - Use CLEAR

// DMA control register bit definitions
#define RUN_OB							0x80
#define STATE							0x70
#define DEMAND_MODE					0x8
#define OUT_DMA_RUN					0x4
#define IN_DMA_RUN						0x2
#define DMA_MODE_EN					0x1


#define REAL_PHYSICAL_ADDR_EP0_FIFO		(0x520001c0) //Endpoint 0 FIFO
#define REAL_PHYSICAL_ADDR_EP1_FIFO		(0x520001c4) //Endpoint 1 FIFO
#define REAL_PHYSICAL_ADDR_EP2_FIFO		(0x520001c8) //Endpoint 2 FIFO
#define REAL_PHYSICAL_ADDR_EP3_FIFO		(0x520001cc) //Endpoint 3 FIFO
#define REAL_PHYSICAL_ADDR_EP4_FIFO		(0x520001d0) //Endpoint 4 FIFO

// GAHBCFG
#define MODE_DMA					(1<<5)
#define MODE_SLAVE					(0<<5)
#define BURST_SINGLE				(0<<1)
#define BURST_INCR					(1<<1)
#define BURST_INCR4				(3<<1)
#define BURST_INCR8				(5<<1)
#define BURST_INCR16				(7<<1)
#define GBL_INT_MASK				(0<<0)
#define GBL_INT_UNMASK			(1<<0)

// For USB DMA
//BOOL InitUsbdDriverGlobals(void);  	//:-)
//void UsbdDeallocateVm(void);	   	//:-)
//BOOL UsbdAllocateVm(void);	   		//:-)
//void UsbdInitDma(int epnum, int bufIndex,int bufOffset);	//:-)


//by Kevin

//////////////////////////////////////////////////////////////////////////

/*
inline	u32	ReadReg(
						u32 uiOffset
						)
{
	volatile u32 *pbReg = ctrlr_base_reg_addr(uiOffset);
	u32 uiValue = (u32) *pbReg;
	return uiValue;
}

inline	void	WriteReg(
						u32 uiOffset,
						u32 bValue
						)
{
	volatile ULONG *pbReg = ctrlr_base_reg_addr(uiOffset);
	*pbReg = (ULONG) bValue;
}

inline	void	UpdateReg(
						  u32 uiOffset,
						  u32 bValue
						  )
{
	WriteReg(uiOffset, (ReadReg(uiOffset) | bValue));
};

inline	void	ClearReg(
						 u32	 uiOffeset,
						 u32 bValue
						 )
{
	WriteReg(uiOffeset, (ReadReg(uiOffeset) & ~bValue));
};
*/

#ifdef __cplusplus
}
#endif

#endif


