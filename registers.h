/*
 * Registers.h
 *
 *  Created on: 2013-6-20
 *      Author: Administrator
 */


#ifndef REGISTERS_H_
#define REGISTERS_H_

#include "xparameters.h"
#include "xil_assert.h"
#include "xil_io.h"

#define REG_BASEADDRESS				XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
#define STAREG_OFFSET				0x00  //status register
#define INTMASK_OFFSET				0x04  // interrupt mask
#define INTREG_OFFSET				0x08  //interrupt register
#define CMDREG_OFFSET				0x0c  //command register
#define MACREG_OFFSET				0x10  //mac address
#define RXBUFFPTR_OFFSET			0x18  //recv ptr
#define TXBUFFPTR_OFFSET			0x1C  //send ptr  This two is using for reset or device wake up read from here.
#define TX_HEAD_REG   0x30
#define TX_TAIL_REG   0x34
#define RX_HEAD_REG   0x38
#define RX_TAIL_REG   0x3C

#define TX_HEAD  (REG_BASEADDRESS + TX_HEAD_REG)
#define TX_TAIL	 (REG_BASEADDRESS + TX_TAIL_REG)
#define RX_HEAD  (REG_BASEADDRESS + RX_HEAD_REG)
#define RX_TAIL  (REG_BASEADDRESS + RX_TAIL_REG)
#define RX_DESC_BASEADDRESS_OFFSET	1024  //recv desc
#define TX_DESC_BASEADDRESS_OFFSET	 17024//send desc

#define RX_IPIF_MAXOFFSET		2000
#define TX_IPIF_MAXOFFSET		1968
//added by shiguofu
#define SND_SIZE 20
#define RCV_SIZE 20

/***********************************************************
 * This is for the translation of the pcieBAR to axiBAR
 ***********************************************************/
#define PCIEBASE XPAR_PCI_EXPRESS_BASEADDR
#define AXI2PCIE0 0x20C
#define AXI2PCIE1 0x214
#define AXIBAR0 XPAR_PCI_EXPRESS_AXIBAR_0
#define AXIBAR1 XPAR_PCI_EXPRESS_AXIBAR_1
/***********************************************************/

#define WR_RXBUFFPTR_REG(rxbuffptr)	*((u32*)RXBUFFPTR) = rxbuffptr
#define WR_TXBUFFPTR_REG(txbuffptr)	*((u32*)TXBUFFPTR) = txbuffptr

//added by shiguofu
#define WRSEND_ADDR(offset, value) Xil_Out32(TX_DESC_BASEADDRESS + offset * TX_DESC_OFFSET + 4, value);
#define WRRECV_ADDR(offset, value) Xil_Out32(RX_DESC_BASEADDRESS + offset * RX_DESC_OFFSET + 4, value);

#define FRAME_SIZE 1536
typedef char EthernetFrame[FRAME_SIZE] __attribute__ ((aligned(64)));

#define STAREG						(REG_BASEADDRESS + STAREG_OFFSET)
#define INTMASK						(REG_BASEADDRESS + INTMASK_OFFSET)
#define INTREG						(REG_BASEADDRESS + INTREG_OFFSET)
#define CMDREG						(REG_BASEADDRESS + CMDREG_OFFSET)
#define MACREG						(REG_BASEADDRESS + MACREG_OFFSET)
#define RXBUFFPTR					(REG_BASEADDRESS + RXBUFFPTR_OFFSET)
#define TXBUFFPTR					(REG_BASEADDRESS + TXBUFFPTR_OFFSET)
#define RX_DESC_BASEADDRESS			(REG_BASEADDRESS + RX_DESC_BASEADDRESS_OFFSET)
#define TX_DESC_BASEADDRESS			(REG_BASEADDRESS + TX_DESC_BASEADDRESS_OFFSET)


//#define RX_DESC_BASEADDRESS		0x8B600000
#define RX_DESC_OFFSET				8
#define RX_ADDR_BASEADDRESS			(RX_DESC_BASEADDRESS + 4)//0x8B600004
#define RX_ADDR_OFFSET				8

//#define TX_DESC_BASEADDRESS		0x8B604000
#define TX_DESC_OFFSET				8
#define TX_ADDR_BASEADDRESS			(TX_DESC_BASEADDRESS + 4)//0x8B604004
#define TX_ADDR_OFFSET				8
/*
#define ReadRecvDesc(offset) \
		*(u32*)( (RX_DESC_BASEADDRESS) + (offset) * 4)

#define ReadRecvBuffAddr(offset) \
		*(u32*)( (RX_DESC_BASEADDRESS) + (RX_DESC_ADDR_OFFSET)+ (offset) * 4)
*/

////////////////////////////////////////////////////
#define RXOK  			0x030000
#define RXFREE			0x020000
#define SENDOK			0x0002FFFF
#define DATARDYMASK  	0x030000
#define DATAREADY		0x030000


#define	LOCAL_BUFF_STATE_MASK	0xC0000000
#define LOCAL_BUFF_DATA_READY	0x40000000
#define LOCAL_BUFF_BUSY			0x80000000
//#define LOCAL_BUFF_TRANSFER		0x80000000
#define LOCAL_BUFF_COMPLETE		0x00000000
#define LOCAL_BUFF_FREE			0x00000000

#define FRAME_SIZE 1536


#define COMMANDINFOMASK		0xFFFF0000
inline u32 ReadRecvDesc(u16 offset);
inline u32 ReadRecvBuffAddr(u16 offset);
inline u32 ReadRecvCmdInfo(u16 offset);
void WriteRecvDesc(u16 offset,u32 value);

inline u32 ReadSendDesc(u16 offset);
inline u32 ReadSendBuffAddr(u16 offset);
inline void WriteSendDesc(u16 offset,u32 value);

//added by shiguofu
void prt_hex(char *p, int len);
void PayloadData();
//void print_Eth(EthernetFrame eth);

#endif /* REGISTERS_H_ */
