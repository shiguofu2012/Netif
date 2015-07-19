/*
 * my_operation.h
 *
 *  Created on: Jan 24, 2015
 *      Author: liang
 */

#ifndef MY_OPERATION_H_
#define MY_OPERATION_H_

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xdebug.h"
#include "xintc.h"
#include"xaxipcie.h"
#include"registers.h"
#include"xgpio.h"
#include "xemaclite.h"

extern void xil_printf(const char *format, ...);

//#define DEBUG

/******************** Constant Definitions **********************************/
/*
 * Device hardware build related constants.
 */
#define NUMS_INTR 64

#define EMAC_DEVICE_ID		XPAR_EMACLITE_0_DEVICE_ID
#define DMA_BASE 0x41E00000
#define DMA_TAIL (DMA_BASE + 0x10)
#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID
#define DDR_BASE_ADDR		XPAR_V6DDR_0_S_AXI_BASEADDR
#define GPIO_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)
#define RX_INTR_ID		XPAR_INTC_0_AXIDMA_0_S2MM_INTROUT_VEC_ID
#define TX_INTR_ID		XPAR_INTC_0_AXIDMA_0_MM2S_INTROUT_VEC_ID

#define RX_BD_SPACE_BASE	(RX_DESC_BASEADDRESS)
#define RX_BD_SPACE_HIGH	(RX_BD_SPACE_BASE + 0x00001FFF)
#define TX_BD_SPACE_BASE	(TX_DESC_BASEADDRESS)
#define TX_BD_SPACE_HIGH	(TX_BD_SPACE_BASE + 0x0001FFF)
//#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)
//#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)
//#define RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x004FFFFF)

#define INTC_DEVICE_ID          XPAR_INTC_0_DEVICE_ID


#define RX_BD_NUMS 			1024
/* Timeout loop counter for reset
 */
#define RESET_TIMEOUT_COUNTER	10000

/*
 * Buffer and Buffer Descriptor related constant definition
 */
#define MAX_PKT_LEN		1536

/*
 * Number of BDs in the transfer example
 * We show how to submit multiple BDs for one transmit.
 * The receive side gets one completion per transfer.
 */
#define NUMBER_OF_BDS_PER_PKT		12
#define NUMBER_OF_PKTS_TO_TRANSFER 	11
#define NUMBER_OF_BDS_TO_TRANSFER	(NUMBER_OF_PKTS_TO_TRANSFER * \
						NUMBER_OF_BDS_PER_PKT)

/* The interrupt coalescing threshold and delay timer threshold
 * Valid range is 1 to 255
 *
 * We set the coalescing threshold to be the total number of packets.
 * The receive side will only get one completion interrupt for this example.
 */
#define COALESCING_COUNT		NUMBER_OF_PKTS_TO_TRANSFER
#define DELAY_TIMER_COUNT		80

/*
 * The pcie configuration space offset registers
 */
/*
 * Define the offset within the PCIE configuration space from
 * the beginning of the PCIE configuration space.
 */
#define PCIE_ID

#define PCIE_CFG_ID_REG			0x0000	/* Vendor ID/Device ID
						 * offset */
#define PCIE_CFG_CMD_STATUS_REG		0x0001	/* Command/Status Register
						 * Offset */
#define PCIE_CFG_CAH_LAT_HD_REG		0x0003	/* Cache Line/Latency
						 * Timer/Header Type/BIST
						 * Register Offset */
#define PCIE_CFG_BAR_ZERO_REG		0x0004	/* Bar 0 offset */


#define PCIE_CFG_CMD_BUSM_EN		0x00000004 /* Bus master enable */

#define DELAY	  5               //this is for the gpio interrupt


#define INTC_HANDLER	XIntc_InterruptHandler

int CheckData(int Length, u8 StartValue);
void TxCallBack(XAxiDma_BdRing * TxRingPtr);
void TxIntrHandler(void *Callback);
void RxCallBack(XAxiDma_BdRing * RxRingPtr);
void RxIntrHandler(void *Callback);
void send_msi();
int init_device();

inline void setMAC();

int SetupIntrSystem(XIntc * IntcInstancePtr,
			   XAxiDma * AxiDmaPtr, u16 TxIntrId, u16 RxIntrId);
void DisableIntrSystem(XIntc * IntcInstancePtr,
					u16 TxIntrId, u16 RxIntrId);

int RxSetup(XAxiDma * AxiDmaInstPtr);
int TxSetup(XAxiDma * AxiDmaInstPtr);
int SendPacket(XAxiDma * AxiDmaInstPtr);

void printPci_info(XAxiPcie* XlnxEndPointPtr);

extern XAxiDma AxiDma;
extern XIntc Intc;
extern XAxiPcie axi_pcie;
extern XGpio gpio;

extern u8 RcvBuffer[RX_BD_NUMS][MAX_PKT_LEN] __attribute__ ((aligned(64)));

extern volatile int TxDone;
extern volatile int RxDone;
extern volatile int Error;

#endif /* MY_OPERATION_H_ */
