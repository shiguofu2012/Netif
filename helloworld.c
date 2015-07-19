/******************************************************************************
*
* (c) Copyright 2010-2013 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information of Xilinx, Inc.
* and is protected under U.S. and international copyright and other
* intellectual property laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any rights to the
* materials distributed herewith. Except as otherwise provided in a valid
* license issued to you by Xilinx, and to the maximum extent permitted by
* applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
* FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
* IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
* and (2) Xilinx shall not be liable (whether in contract or tort, including
* negligence, or under any other theory of liability) for any loss or damage
* of any kind or nature related to, arising under or in connection with these
* materials, including for any direct, or any indirect, special, incidental,
* or consequential loss or damage (including loss of data, profits, goodwill,
* or any type of loss or damage suffered as a result of any action brought by
* a third party) even if such damage or loss was reasonably foreseeable or
* Xilinx had been advised of the possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-safe, or for use in
* any application requiring fail-safe performance, such as life-support or
* safety devices or systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any other applications
* that could lead to death, personal injury, or severe property or
* environmental damage (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and liability of any use of
* Xilinx products in Critical Applications, subject only to applicable laws
* and regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
* AT ALL TIMES.
*
******************************************************************************/
/*****************************************************************************/
/**
 *
 * @file xaxidma_example_sg_intr.c
 *
 * This file demonstrates how to use the xaxidma driver on the Xilinx AXI
 * DMA core (AXIDMA) to transfer packets in interrupt mode when the AXIDMA
 * core is configured in Scatter Gather Mode
 *
 * We show how to do multiple packets transfers, as well as how to do multiple
 * BDs per packet transfers.
 *
 * This code assumes a loopback hardware widget is connected to the AXI DMA
 * core for data packet loopback.
 *
 * To see the debug print, you need a Uart16550 or uartlite in your system,
 * and please set "-DDEBUG" in your compiler options. You need to rebuild your
 * software executable.
 *
 * Make sure that MEMORY_BASE is defined properly as per the HW system. The
 * h/w system built in Area mode has a maximum DDR memory limit of 64MB. In
 * throughput mode, it is 512MB.  These limits are need to ensured for
 * proper operation of this code.
 *
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.00a jz   05/18/10 First release
 * 2.00a jz   08/10/10 Second release, added in xaxidma_g.c, xaxidma_sinit.c,
 *		       		   updated tcl file, added xaxidma_porting_guide.h, removed
 *		         	   workaround for endianness
 * 4.00a rkv  02/22/11 Name of the file has been changed for naming consistency
 *		       		   Added interrupt support for Zynq.
 * 5.00a srt  03/05/12 Added Flushing and Invalidation of Caches to fix CRs
 *		       		   648103, 648701.
 *		       		   Added V7 DDR Base Address to fix CR 649405.
 * 6.00a srt  01/24/12 Changed API calls to support MCDMA driver.
 * 7.00a srt  06/18/12 API calls are reverted back for backward compatibility.
 * 7.01a srt  11/02/12 Buffer sizes (Tx and Rx) are modified to meet maximum
 *		       DDR memory limit of the h/w system built with Area mode
 * 7.02a srt  03/01/13 Updated DDR base address for IPI designs (CR 703656).
 *
 * </pre>
 *
 * ***************************************************************************
 */
/***************************** Include Files *********************************/
#include"my_operation.h"


/************************** Function Prototypes ******************************/
#ifdef XPAR_UARTNS550_0_BASEADDR
static void Uart550_Setup(void);
#endif



/************************** Variable Definitions *****************************/

/*
 * This is for the CARD test without insert into the PCIe.Buffer
 */

#ifdef DEBUG
volatile u8 SrcBuffer[10][1000] __attribute__ ((aligned(64)));
#endif

/*****************************************************************************/
/**
*
* Main function
*
* This function is the main entry of the interrupt test. It does the following:
*	- Set up the output terminal if UART16550 is in the hardware build
*	- Initialize the DMA engine
*	- Set up Tx and Rx channels
*	- Set up the interrupt system for the Tx and Rx interrupts
*	- Submit a transfer
*	- Wait for the transfer to finish
*	- Check transfer status
*	- Disable Tx and Rx interrupts
*	- Print test status and exit
*
* @param	None
*
* @return	- XST_SUCCESS if tests pass
*		- XST_FAILURE if fails.
*
* @note		None.
*
******************************************************************************/

int main(void)
{
	int Status;


	/* Initial setup for Uart16550 */
#ifdef XPAR_UARTNS550_0_BASEADDR

	Uart550_Setup();

#endif
	/*************************
	 * initinalize the buffer
	 */
#ifdef DEBUG
	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 1000; ++j)
			SrcBuffer[i][j] = 'a' + j % 26;
	}
#endif
	xil_printf("\r\n--- Entering main() --- \r\n");

	Status = init_device();
	if(Status != XST_SUCCESS)
	{
		xil_printf("init error with %d\r\n", Status);

		return -1;
	}

	//XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma);
	XAxiDma_Bd *BdCurPtr;
	BdCurPtr = (XAxiDma_Bd*)TX_BD_SPACE_BASE;
	u32 head, tail, tmp;
	tmp = 0;
	xil_printf("S2MM cur: %x\r\n", Xil_In32(0x41E00000 + 0x38));
	xil_printf("MM2S CTRL: %x\tS2MM CTRL: %x\r\n",
			Xil_In32(DMA_BASE), Xil_In32(DMA_BASE + 0x30));
	while(1)
	{
#ifndef DEBUG
		head = Xil_In32(TX_HEAD);
		tail = Xil_In32(TX_TAIL);
		while(tmp == tail)
		{
			head = Xil_In32(TX_HEAD);
			tail = Xil_In32(TX_TAIL);
			Xil_DCacheInvalidateRange(TX_HEAD, 8);
		}
#endif
		TxDone = Error = 0;
		//BdCurPtr = BdPtr;
		/*for(i = 0; i < 10; i++)
		{
			Status = XAxiDma_BdSetBufAddr(BdCurPtr, (u32)SrcBuffer[i]);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set buffer addr %x on BD %x failed %d\r\n",
						(unsigned int)SrcBuffer,
						(unsigned int)BdPtr, Status);

				return XST_FAILURE;
			}

			Status = XAxiDma_BdSetLength(BdCurPtr, 1000,
							TxRingPtr->MaxTransferLen);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set length %d on BD %x failed %d\r\n",
						MAX_PKT_LEN, (unsigned int)BdPtr, Status);

				return XST_FAILURE;
			}

			CrBits = 0;
			CrBits |= XAXIDMA_BD_CTRL_TXSOF_MASK;
			CrBits |= XAXIDMA_BD_CTRL_TXEOF_MASK;
			XAxiDma_BdSetCtrl(BdCurPtr, CrBits);
			BdCurPtr = XAxiDma_BdRingNext(TxRingPtr, BdCurPtr);
		}*/



#ifdef DEBUG
		for(i = 0; i < 1; i++)
		{
			*(u32*)((u8*)BdCurPtr + 0x1C) &= ~0x80000000;
			*(u32*)((u8*)BdCurPtr + 0x08) = (u32)SrcBuffer[i];
			*(u32*)((u8*)BdCurPtr + 0x18) = 0x0C000000 | 1000;
			BdCurPtr = (u32)(*(u32*)BdCurPtr);
		}
		if(BdCurPtr != TX_BD_SPACE_BASE)
			*(u32*)DMA_TAIL = (u32)(BdCurPtr - 1);
		else *(u32*)DMA_TAIL = (u32)(BdCurPtr + 127);
		//if((BdCurPtr - (XAxiDma_Bd*)TX_BD_SPACE_BASE) == 128)
		//				BdCurPtr = (XAxiDma_Bd*)TX_BD_SPACE_BASE;
#else
		if(tmp != tail)
		{
			*(u32*)DMA_TAIL = (u32)(TX_BD_SPACE_BASE + 64 * ((tail + 128 - 1) % 128));
			tmp = tail;
		}

		//xil_printf("Tx start: %x\tDMA_TAIL: %x\r\n",TX_BD_SPACE_BASE, Xil_In32(DMA_TAIL));

		//print("\r\n");
#endif

	//////////////////////////////////////////////////////////////////

	/*while(!TxDone && !Error)
	{
		xil_printf("cur REG: %x\ttail REG: %x\r\n",
				Xil_In32(0x41E00000 + 0x08), Xil_In32(0x41E00000 + 0x10));
		//xil_printf("send status : %x\r\n", *(u32*)((u8*)BdCurPtr + 0x1C));
		//xil_printf("HEAD: %d, TAIL: %d\r\n", Xil_In32(TX_HEAD), Xil_In32(TX_TAIL));
		int i = 0;
		while(i < 0xFFFF)i++;
	}*/
	//xil_printf("send a packet\r\n", TxDone);

	//BdPtr = BdCurPtr;
		/*
	 * Wait TX done and RX done
	 */
	/*while (((TxDone < NUMBER_OF_BDS_TO_TRANSFER) ||
			(RxDone < NUMBER_OF_BDS_TO_TRANSFER)) && !Error) {
	}*/

	/*if (Error) {
		xil_printf("Failed test transmit%s done, "
					"receive%s done\r\n", TxDone? "":" not",
							RxDone? "":" not");
	}*/
#ifdef DEBUG
		else
		{

			Xil_DCacheInvalidateRange(RcvBuffer[0], 1000);
			for(i = 0; i < 20; i++)
			{
				xil_printf("%x", RcvBuffer[0][i]);
			}
			xil_printf("\r\nRx NUM: %d\r\n", RxDone);

		}
#endif

	/* Disable TX and RX Ring interrupts and return success */
	}
	DisableIntrSystem(&Intc, TX_INTR_ID, RX_INTR_ID);

Done:

	xil_printf("--- Exiting main() --- \r\n");

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

#ifdef XPAR_UARTNS550_0_BASEADDR
/*****************************************************************************/
/*
*
* Uart16550 setup routine, need to set baudrate to 9600 and data bits to 8
*
* @param	None
*
* @return	None
*
* @note		None.
*
******************************************************************************/
static void Uart550_Setup(void)
{

	XUartNs550_SetBaud(XPAR_UARTNS550_0_BASEADDR,
			XPAR_XUARTNS550_CLOCK_HZ, 9600);

	XUartNs550_SetLineControlReg(XPAR_UARTNS550_0_BASEADDR,
			XUN_LCR_8_DATA_BITS);
}
#endif



