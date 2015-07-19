/*
 * my_operation.c
 *
 *  Created on: Jan 24, 2015
 *      Author: liang
 */

#include"my_operation.h"
#include"ael_registers.h"
/*
 * Device instance definitions
 */
XAxiDma AxiDma;
XAxiPcie axi_pcie;
XGpio gpio;
XEmacLite EmacLiteInstance;

XIntc Intc;	/* Instance of the Interrupt Controller */

char AxiEthernetMAC[6] = { 0x00, 0x0A, 0x35, 0x01, 0x02, 0x03 };

/*
 * Flags interrupt handlers use to notify the application context the events.
 */
volatile int TxDone;
volatile int RxDone;
volatile int Error;

/*
 * Buffer for transmit packet. Must be 32-bit aligned to be used by DMA.
 */
//u32 *Packet = (u32 *) TX_BUFFER_BASE;

u8 RcvBuffer[RX_BD_NUMS][MAX_PKT_LEN];

int init_acl(XEmacLite *InstancePtr);
void delay(u16 delay_time);
int get_acl_status(XEmacLite *InstancePtr);


void send_msi()
{
	//XGpio_SetDataDirection(&gpio, 1, 0);
	XGpio_WriteReg(gpio.BaseAddress, XGPIO_TRI_OFFSET, 0);
	//XGpio_DiscreteWrite(&gpio, 1, 0x1);
	XGpio_WriteReg(gpio.BaseAddress, XGPIO_DATA_OFFSET, 1);
	int i = 0;
	for(i = 0; i < DELAY; ++i);
	//XGpio_DiscreteWrite(&gpio, 1, 0x0);
	XGpio_WriteReg(gpio.BaseAddress, XGPIO_DATA_OFFSET, 0);
}


inline void setMAC()
{
	int i;
	for(i = 0; i < 6; i++)
		Xil_Out8(REG_BASEADDRESS + MACREG_OFFSET + i, AxiEthernetMAC[i]);
}

int init_device()
{
	XAxiDma_Config *Config;
	XAxiPcie_Config *pcie_cfgPtr;
	int Status;
	u32* base, *high;

	XEmacLite *EmacLiteInstPtr = &EmacLiteInstance;
	XEmacLite_Config *ConfigPtr;

	ConfigPtr = XEmacLite_LookupConfig(EMAC_DEVICE_ID);
	XEmacLite_CfgInitialize(EmacLiteInstPtr, ConfigPtr, ConfigPtr->BaseAddress);


	base = (u32*)REG_BASEADDRESS, high = base + 0xFFFF;
	while(base < high)
	{
		Xil_Out32(base, 0);
		base++;
	}

	 Status = XGpio_Initialize(&gpio, GPIO_ID);
	 if (Status != XST_SUCCESS)  {
		  return XST_FAILURE;
	 }


	//init_acl(EmacLiteInstPtr);

	Config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);

		return XST_FAILURE;
	}

	/* Initialize DMA engine */
	XAxiDma_CfgInitialize(&AxiDma, Config);

	if(!XAxiDma_HasSg(&AxiDma)) {
		xil_printf("Device configured as Simple mode \r\n");
		return XST_FAILURE;
	}

	setMAC();
	Xil_DCacheInvalidateRange(REG_BASEADDRESS + MACREG_OFFSET, 8);
	XGpio_SetDataDirection(&gpio, 1, 1);
	if(XGpio_DiscreteRead(&gpio, 1) != 0x1)
		print("This device do not support msi interrupt\r\n");
	else
		print("MSI is ready\r\n");

	u32 sts_tx, sts_rx;

	xil_printf("MM2S CTRL: %x\tS2MM CTRL: %x\r\n",
			Xil_In32(DMA_BASE), Xil_In32(DMA_BASE + 0x30));
	//xil_printf("MM2S status: %x\tS2MM status: %x\r\n", sts_tx, sts_rx);
	//Xil_Out32(DMA_BASE, Xil_In32(DMA_BASE) | 0x4);
	//Xil_Out32(DMA_BASE + 0x30, Xil_In32(DMA_BASE + 0x30) | 0x4);

	sts_tx = Xil_In32(DMA_BASE + 0x04);
	sts_rx = Xil_In32(DMA_BASE + 0x34);
	while(((sts_tx & 0x01) == 0) || ((sts_rx & 0x01) == 0))
	{
		Xil_Out32(DMA_BASE, 0);
		Xil_Out32(DMA_BASE + 0x30, 0);
		sts_tx = Xil_In32(DMA_BASE + 0x04);
		sts_rx = Xil_In32(DMA_BASE + 0x34);
		int delay  = 0;
		while(delay < 0xFF)delay++;
		xil_printf("MM2S status: %x\tS2MM status: %x\r\n", sts_tx, sts_rx);
	}
	xil_printf("MM2S status: %x\tS2MM status: %x\r\n", sts_tx, sts_rx);

	/* Set up TX/RX channels to be ready to transmit and receive packets */
	Status = TxSetup(&AxiDma);

	if (Status != XST_SUCCESS) {

		xil_printf("Failed TX setup\r\n");
		return XST_FAILURE;
	}

	Xil_DCacheInvalidateRange(RX_DESC_BASEADDRESS, 128 * 64);


	Status = RxSetup(&AxiDma);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed RX setup\r\n");
		return XST_FAILURE;
	}


#ifndef DEBUG
	print("waiting the recv address...\r\n");
	Status = Xil_In32(STAREG);
	while(!(Status & 0x2))
	{
		Xil_DCacheInvalidateRange(STAREG, 4);
		Status = Xil_In32(STAREG);
	}
#endif
	print("-----------------------------------\r\nset address ok\r\n");

	print("-----------------------------------\r\n");

	/*int Index;
	XAxiDma_Bd *BdCurPtr;
	BdCurPtr = (XAxiDma_Bd*)RX_BD_SPACE_BASE;
	for(Index = 0; Index < 128; Index++)
	{
		*(u32*)((u8*)BdCurPtr + 0x08) = (u32)RcvBuffer[Index];
		//*(u32*)((u8*)BdCurPtr + 0x18) = MAX_PKT_LEN;
		BdCurPtr += 1;
	}


	*(u32*)(0x41E00000 + 0x40) = (u32)(BdCurPtr - 1);*/
	u32 cr = Xil_In32(DMA_BASE + 0x30);
	Xil_Out32(DMA_BASE + 0x30, cr | 0x01);
	*(u32*)(0x41E00000 + 0x40) = (u32)((XAxiDma_Bd*)RX_BD_SPACE_BASE + 127);
	/* Set up Interrupt system  */
	Status = SetupIntrSystem(&Intc, &AxiDma, TX_INTR_ID, RX_INTR_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed intr setup\r\n");
		return XST_FAILURE;
	}

	pcie_cfgPtr = XAxiPcie_LookupConfig(XPAR_PCI_EXPRESS_DEVICE_ID);
	if (pcie_cfgPtr == NULL) {
		print("Failed to initialize PCIe End Point Instance\r\n");
		return XST_FAILURE;
	}
	Status = XAxiPcie_CfgInitialize(&axi_pcie, pcie_cfgPtr,
			pcie_cfgPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		print("Failed to initialize PCIe End Point Instance\r\n");
		return Status;
	}

	printPci_info(&axi_pcie);
	test_status(EmacLiteInstPtr);
	return XST_SUCCESS;
}

void printPci_info(XAxiPcie* XlnxEndPointPtr)
{
	u32 Status;
	u32 InterruptMask;
	u32 HeaderData;
	u8  BusNum;
	u8  DeviceNum;
	u8  FunctionNum;
	u8  PortNumber;
	/* See what interrupts are currently enabled */
	XAxiPcie_GetEnabledInterrupts(XlnxEndPointPtr, &InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	/* Disable.all interrupts */
	XAxiPcie_DisableInterrupts(XlnxEndPointPtr,
						XAXIPCIE_IM_ENABLE_ALL_MASK);

	/* See what interrupts are currently pending */
	XAxiPcie_GetPendingInterrupts(XlnxEndPointPtr, &InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/* Clear the pending interrupt */
	XAxiPcie_ClearPendingInterrupts(XlnxEndPointPtr,
						XAXIPCIE_ID_CLEAR_ALL_MASK);

	/*
	 * Read enabled interrupts and pending interrupts
	 * to verify the previous two operations and also
	 * to test those two API functions
	 */
	XAxiPcie_GetEnabledInterrupts(XlnxEndPointPtr, &InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	XAxiPcie_GetPendingInterrupts(XlnxEndPointPtr, &InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);


	/* Make sure link is up. */
	while(1)
	{
		Status = XAxiPcie_IsLinkUp(XlnxEndPointPtr);
		if (Status != TRUE ) {
			xil_printf("Link is not up\r\n");
		}
		else break;
	}

	xil_printf("Link is up\r\n");


	/* See if root complex has already configured this end point. */

	XAxiPcie_ReadLocalConfigSpace(XlnxEndPointPtr, PCIE_CFG_CMD_STATUS_REG,
								&HeaderData);

	xil_printf("PCIe Command/Status Register is %08X\r\n", HeaderData);

	if (HeaderData & PCIE_CFG_CMD_BUSM_EN) {

		xil_printf("Root Complex has configured this end point\r\n");
	}
	else {
		xil_printf("Root Complex has NOT yet configured this"
							" end point\r\n");
	}

	XAxiPcie_GetRequesterId(XlnxEndPointPtr, &BusNum,
				&DeviceNum, &FunctionNum, &PortNumber);

	xil_printf("Bus Number is %02X\r\n"
				"Device Number is %02X\r\n"
				"Function Number is %02X\r\n"
				"Port Number is %02X\r\n",
				BusNum, DeviceNum, FunctionNum, PortNumber);

	/* Read my configuration space */
	XAxiPcie_ReadLocalConfigSpace(XlnxEndPointPtr, PCIE_CFG_ID_REG,
								&HeaderData);
	xil_printf("PCIe Vendor ID/Device ID Register is %08X\r\n",
								HeaderData);

	XAxiPcie_ReadLocalConfigSpace(XlnxEndPointPtr, PCIE_CFG_CMD_STATUS_REG,
								&HeaderData);

	xil_printf("PCIe Command/Status Register is %08X\r\n", HeaderData);

	XAxiPcie_ReadLocalConfigSpace(XlnxEndPointPtr, PCIE_CFG_CAH_LAT_HD_REG,
								&HeaderData);

	xil_printf("PCIe Header Type/Latency Timer Register is %08X\r\n",
								HeaderData);

	XAxiPcie_ReadLocalConfigSpace(XlnxEndPointPtr, PCIE_CFG_BAR_ZERO_REG,
								&HeaderData);

	xil_printf("PCIe BAR 0 is %08X\r\n", HeaderData);

}

/*****************************************************************************/
/*
*
* This is the DMA TX callback function to be called by TX interrupt handler.
* This function handles BDs finished by hardware.
*
* @param	TxRingPtr is a pointer to TX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void TxCallBack(XAxiDma_BdRing * TxRingPtr)
{
	//XAxiDma_Bd *BdCurPtr;
	XAxiDma_Bd *CURBd;
	//int Status;
	int Index;

	CURBd = (XAxiDma_Bd*)Xil_In32(DMA_BASE + XAXIDMA_CDESC_OFFSET);
	Index = CURBd - (XAxiDma_Bd*)TX_BD_SPACE_BASE;
	//xil_printf("CURBd: %x, Index:%d\r\n", CURBd, Index);
	Xil_Out32(REG_BASEADDRESS + TX_HEAD_REG,
			Index);
	Xil_DCacheInvalidateRange(REG_BASEADDRESS + TX_HEAD_REG, 4);
	//Status = Xil_In32(INTREG);
	//if((Status & 0x2) == 0)
	{
		Xil_Out32(INTREG, 0x2);
		send_msi();
	}

	//xil_printf("HEAD: %d, TAIL: %d\r\n", Xil_In32(TX_HEAD), Xil_In32(TX_TAIL));
	//TxDone = 1;
}

/*****************************************************************************/
/*
*
* This is the DMA TX Interrupt handler function.
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware. Otherwise, if a completion interrupt
* presents, then it calls the callback function.
*
* @param	Callback is a pointer to TX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void TxIntrHandler(void *Callback)
{
	XAxiDma_BdRing *TxRingPtr = (XAxiDma_BdRing *) Callback;
	u32 IrqStatus;
	int TimeOut;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_BdRingGetIrq(TxRingPtr);
	//xil_printf("Tx IrqStatus: %x\r\n", IrqStatus);

	/* Acknowledge pending interrupts */
	XAxiDma_BdRingAckIrq(TxRingPtr, IrqStatus);

	/* If no interrupt is asserted, we do not do anything
	 */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {

		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK)) {

		XAxiDma_BdRingDumpRegs(TxRingPtr);
		u32 RegBase = TxRingPtr->ChanBase;

		XAxiDma_Bd *cur = (XAxiDma_Bd *)XAxiDma_ReadReg(RegBase, XAXIDMA_CDESC_OFFSET);
		int i;
		print("The Descriptor:\r\n");
		for(i = 0; i < 10; ++i)
		{
			xil_printf("%08x  ", Xil_In32((u8*)cur + i * 4));
		}
		print("\r\n");

		Error = 1;

		/*
		 * Reset should never fail for transmit channel
		 */
		XAxiDma_Reset(&AxiDma);

		TimeOut = RESET_TIMEOUT_COUNTER;

		while (TimeOut) {
			if (XAxiDma_ResetIsDone(&AxiDma)) {
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/*
	 * If Transmit done interrupt is asserted, call TX call back function
	 * to handle the processed BDs and raise the according flag
	 */
	if ((IrqStatus & XAXIDMA_IRQ_IOC_MASK) || (IrqStatus & XAXIDMA_IRQ_DELAY_MASK)) {
		TxCallBack(TxRingPtr);
	}
}

/*****************************************************************************/
/*
*
* This is the DMA RX callback function called by the RX interrupt handler.
* This function handles finished BDs by hardware, attaches new buffers to those
* BDs, and give them back to hardware to receive more incoming packets
*
* @param	RxRingPtr is a pointer to RX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
int rx_index;
void RxCallBack(XAxiDma_BdRing * RxRingPtr)
{
	//int BdCount;
	XAxiDma_Bd *BdPtr;
	u32 head, tail;
	int Status;


	BdPtr = (XAxiDma_Bd*)Xil_In32(DMA_BASE + 0x38);

	head = Xil_In32(RX_HEAD), tail = Xil_In32(RX_TAIL);
	//BdCurPtr = *(u32*)(0x41E00000 + 0x38);
	Status = Xil_In32(REG_BASEADDRESS + INTMASK_OFFSET);
	if((Status & 0x01) != 1)
	{
		//Xil_Out32(REG_BASEADDRESS + 0x18, RecvIndex);
		//Xil_DCacheInvalidateRange(REG_BASEADDRESS + 0x18, 4);
		Xil_Out32(REG_BASEADDRESS + INTREG_OFFSET, 0x01);
		Xil_DCacheInvalidateRange(REG_BASEADDRESS + INTREG_OFFSET, 4);
		send_msi();
	}
	if(rx_index != tail)
	{
		*(u32*)(DMA_BASE + 0x40) = RX_BD_SPACE_BASE + ((tail - 1 + 128) % 128) * 64;
		rx_index = tail;
	}
	Xil_Out32(RX_HEAD, BdPtr - (XAxiDma_Bd*)RX_BD_SPACE_BASE);
	Xil_DCacheInvalidateRange(RX_HEAD, 4);

	/* Get finished BDs from hardware */
	/*BdCount = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);

	BdCurPtr = BdPtr;
	for (Index = 0; Index < BdCount; Index++) {

		BdSts = XAxiDma_BdGetSts(BdCurPtr);
		if ((BdSts & XAXIDMA_BD_STS_ALL_ERR_MASK) ||
		    (!(BdSts & XAXIDMA_BD_STS_COMPLETE_MASK))) {
			Error = 1;
			break;
		}
		xil_printf("Rx Len: %d\r\n", *(u32*)((u8*)BdCurPtr + 0x1C) & 0x3FFFFF);
		BdCurPtr = XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
		RxDone += 1;
	}*/
}

/*****************************************************************************/
/*
*
* This is the DMA RX interrupt handler function
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware. Otherwise, if a completion interrupt
* presents, then it calls the callback function.
*
* @param	Callback is a pointer to RX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void RxIntrHandler(void *Callback)
{
	XAxiDma_BdRing *RxRingPtr = (XAxiDma_BdRing *) Callback;
	u32 IrqStatus;
	int TimeOut;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_BdRingGetIrq(RxRingPtr);

	//xil_printf("Rx IrqStatus: %x\r\n", IrqStatus);
	/* Acknowledge pending interrupts */
	XAxiDma_BdRingAckIrq(RxRingPtr, IrqStatus);

	/*
	 * If no interrupt is asserted, we do not do anything
	 */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {
		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK)) {

		XAxiDma_BdRingDumpRegs(RxRingPtr);
		u32 RegBase = RxRingPtr->ChanBase;
		XAxiDma_Bd *cur = (XAxiDma_Bd *)XAxiDma_ReadReg(RegBase, XAXIDMA_CDESC_OFFSET);
		int i;
		print("The Descriptor:\r\n");
		for(i = 0; i < 10; ++i)
		{
			xil_printf("%08x  ", Xil_In32((u8*)cur + i * 4));
		}
		print("\r\n");

		Error = 1;

		/* Reset could fail and hang
		 * NEED a way to handle this or do not call it??
		 */
		XAxiDma_Reset(&AxiDma);

		TimeOut = RESET_TIMEOUT_COUNTER;

		while (TimeOut) {
			if(XAxiDma_ResetIsDone(&AxiDma)) {
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/*
	 * If completion interrupt is asserted, call RX call back function
	 * to handle the processed BDs and then raise the according flag.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_IOC_MASK) || (IrqStatus & XAXIDMA_IRQ_DELAY_MASK)) {
		RxCallBack(RxRingPtr);
	}
}

/*****************************************************************************/
/*
*
* This function setups the interrupt system so interrupts can occur for the
* DMA, it assumes INTC component exists in the hardware system.
*
* @param	IntcInstancePtr is a pointer to the instance of the INTC.
* @param	AxiDmaPtr is a pointer to the instance of the DMA engine
* @param	TxIntrId is the TX channel Interrupt ID.
* @param	RxIntrId is the RX channel Interrupt ID.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not succesful
*
* @note		None.
*
******************************************************************************/

int SetupIntrSystem(XIntc * IntcInstancePtr,
			   XAxiDma * AxiDmaPtr, u16 TxIntrId, u16 RxIntrId)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(AxiDmaPtr);
	XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(AxiDmaPtr);
	int Status;

#ifdef XPAR_INTC_0_DEVICE_ID

	/* Initialize the interrupt controller and connect the ISRs */
	Status = XIntc_Initialize(IntcInstancePtr, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed init intc\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Connect(IntcInstancePtr, TxIntrId,
			       (XInterruptHandler) TxIntrHandler, TxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed tx connect intc\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Connect(IntcInstancePtr, RxIntrId,
			       (XInterruptHandler) RxIntrHandler, RxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed rx connect intc\r\n");
		return XST_FAILURE;
	}

	/* Start the interrupt controller */
	Status = XIntc_Start(IntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to start intc\r\n");
		return XST_FAILURE;
	}

	XIntc_Enable(IntcInstancePtr, TxIntrId);
	XIntc_Enable(IntcInstancePtr, RxIntrId);

#else

	XScuGic_Config *IntcConfig;


	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TxIntrId, 0xA0, 0x3);

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, RxIntrId, 0xA0, 0x3);
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, TxIntrId,
				(Xil_InterruptHandler)TxIntrHandler,
				TxRingPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	Status = XScuGic_Connect(IntcInstancePtr, RxIntrId,
				(Xil_InterruptHandler)RxIntrHandler,
				RxRingPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(IntcInstancePtr, TxIntrId);
	XScuGic_Enable(IntcInstancePtr, RxIntrId);
#endif

	/* Enable interrupts from the hardware */

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)INTC_HANDLER,
			(void *)IntcInstancePtr);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function disables the interrupts for DMA engine.
*
* @param	IntcInstancePtr is the pointer to the INTC component instance
* @param	TxIntrId is interrupt ID associated w/ DMA TX channel
* @param	RxIntrId is interrupt ID associated w/ DMA RX channel
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void DisableIntrSystem(XIntc * IntcInstancePtr,
					u16 TxIntrId, u16 RxIntrId)
{
#ifdef XPAR_INTC_0_DEVICE_ID
	/* Disconnect the interrupts for the DMA TX and RX channels */
	XIntc_Disconnect(IntcInstancePtr, TxIntrId);
	XIntc_Disconnect(IntcInstancePtr, RxIntrId);
#else
	XScuGic_Disconnect(IntcInstancePtr, TxIntrId);
	XScuGic_Disconnect(IntcInstancePtr, RxIntrId);
#endif
}

/*****************************************************************************/
/*
*
* This function sets up RX channel of the DMA engine to be ready for packet
* reception
*
* @param	AxiDmaInstPtr is the pointer to the instance of the DMA engine.
*
* @return	- XST_SUCCESS if the setup is successful.
*		- XST_FAILURE if fails.
*
* @note		None.
*
******************************************************************************/
int RxSetup(XAxiDma * AxiDmaInstPtr)
{
	XAxiDma_BdRing *RxRingPtr;
	int Status;
	XAxiDma_Bd BdTemplate;
	XAxiDma_Bd *BdPtr;
	XAxiDma_Bd *BdCurPtr;
	int BdCount;
	int FreeBdCount;
	u32 RxBufferPtr;
	int Index;

	RxRingPtr = XAxiDma_GetRxRing(&AxiDma);

	/* Disable all RX interrupts before RxBD space setup */
	XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Setup Rx BD space */
	BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT,
				RX_BD_SPACE_HIGH - RX_BD_SPACE_BASE + 1);

	Status = XAxiDma_BdRingCreate(RxRingPtr, RX_BD_SPACE_BASE,
					RX_BD_SPACE_BASE,
					XAXIDMA_BD_MINIMUM_ALIGNMENT, BdCount);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd create failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	/*
	 * Setup a BD template for the Rx channel. Then copy it to every RX BD.
	 */
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(RxRingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd clone failed with %d\r\n", Status);
		return XST_FAILURE;
	}

	/* Attach buffers to RxBD ring so we are ready to receive packets */
	FreeBdCount = XAxiDma_BdRingGetFreeCnt(RxRingPtr);


	/*Status = XAxiDma_BdRingAlloc(RxRingPtr, FreeBdCount, &BdPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("Rx bd alloc failed with %d\r\n", Status);
			return XST_FAILURE;
		}*/
	BdCurPtr = BdPtr = (XAxiDma_Bd*)RX_BD_SPACE_BASE;

	*(u32*)(0x41E00000 + 0x38) = (u32)(BdCurPtr);

	/*for(Index = 0; Index < FreeBdCount; Index++)
	{
		*(u32*)((u8*)BdCurPtr + 0x08) = (u32)RcvBuffer[Index];
		*(u32*)((u8*)BdCurPtr + 0x18) = MAX_PKT_LEN;
		BdCurPtr += 1;
	}*/

	xil_printf("RX cur bd: %x\r\n", *(u32*)(0x41E00000 + 0x38));

	/*Status = XAxiDma_BdRingAlloc(RxRingPtr, FreeBdCount, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx bd alloc failed with %d\r\n", Status);
		return XST_FAILURE;
	}
    BdCurPtr = BdPtr;
	for (Index = 0; Index < FreeBdCount; Index++) {

		Status = XAxiDma_BdSetBufAddr(BdCurPtr, (u32)RcvBuffer[Index]);
		if (Status != XST_SUCCESS) {
			xil_printf("Rx set buffer addr %x on BD %x failed %d\r\n",
			(unsigned int)RxBufferPtr,
			(unsigned int)BdCurPtr, Status);

			return XST_FAILURE;
		}

		Status = XAxiDma_BdSetLength(BdCurPtr, MAX_PKT_LEN,
					RxRingPtr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("Rx set length %d on BD %x failed %d\r\n",
			    MAX_PKT_LEN, (unsigned int)BdCurPtr, Status);

			return XST_FAILURE;
		}

		XAxiDma_BdSetCtrl(BdCurPtr, 0);

		XAxiDma_BdSetId(BdCurPtr, RcvBuffer[Index]);

		//RxBufferPtr += MAX_PKT_LEN;
		BdCurPtr = XAxiDma_BdRingNext(RxRingPtr, BdCurPtr);
	}*/

	/*
	 * Set the coalescing threshold, so only one receive interrupt
	 * occurs for this example
	 *
	 * If you would like to have multiple interrupts to happen, change
	 * the COALESCING_COUNT to be a smaller value
	 */

	Status = XAxiDma_BdRingSetCoalesce(RxRingPtr, 1,
			DELAY_TIMER_COUNT);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx set coalesce failed with %d\r\n", Status);
		return XST_FAILURE;
	}


	/*Status = XAxiDma_BdRingToHw(RxRingPtr, FreeBdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx ToHw failed with %d\r\n", Status);
		return XST_FAILURE;
	}*/

	/* Enable all RX interrupts */
	XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Start RX DMA channel */
	/*Status = XAxiDma_BdRingStart(RxRingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Rx start BD ring failed with %d\r\n", Status);
		return XST_FAILURE;
	}*/

#ifdef DEBUG
	*(u32*)(0x41E00000 + 0x40) = (u32)(BdCurPtr - 1);
#else
	//*(u32*)(0x41E00000 + 0x40) = (u32)(BdCurPtr + 127);
#endif

	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* This function sets up the TX channel of a DMA engine to be ready for packet
* transmission.
*
* @param	AxiDmaInstPtr is the pointer to the instance of the DMA engine.
*
* @return	- XST_SUCCESS if the setup is successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
int TxSetup(XAxiDma * AxiDmaInstPtr)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma);
	XAxiDma_Bd BdTemplate;
	int Status;
	u32 BdCount;

	/* Disable all TX interrupts before TxBD space setup */
	XAxiDma_BdRingIntDisable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/* Setup TxBD space  */
	BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT,
			(u32)TX_BD_SPACE_HIGH - (u32)TX_BD_SPACE_BASE + 1);

	Status = XAxiDma_BdRingCreate(TxRingPtr, TX_BD_SPACE_BASE,
				     TX_BD_SPACE_BASE,
				     XAXIDMA_BD_MINIMUM_ALIGNMENT, BdCount);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed create BD ring\r\n");
		return XST_FAILURE;
	}

	/*
	 * Like the RxBD space, we create a template and set all BDs to be the
	 * same as the template. The sender has to set up the BDs as needed.
	 */
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(TxRingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed clone BDs\r\n");
		return XST_FAILURE;
	}

	/*
	 * Set the coalescing threshold, so only one transmit interrupt
	 * occurs for this example
	 *
	 * If you would like to have multiple interrupts to happen, change
	 * the COALESCING_COUNT to be a smaller value
	 */
	Xil_Out32(0x41E00000 + 0x08, TX_BD_SPACE_BASE);
	u32 ctrl;
	ctrl = Xil_In32(DMA_BASE);
	Xil_Out32(DMA_BASE, ctrl | 0x01);
	Status = XAxiDma_BdRingSetCoalesce(TxRingPtr, NUMS_INTR,
			DELAY_TIMER_COUNT);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed set coalescing"
		" %d/%d\r\n",COALESCING_COUNT, DELAY_TIMER_COUNT);
		return XST_FAILURE;
	}

	/* Enable all TX interrupts */
	XAxiDma_BdRingIntEnable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	/*u32 ctrl = Xil_In32(DMA_BASE);
	Xil_Out32(DMA_BASE, ctrl | 0x01);*/

	/* Start the TX channel */
	/*Status = XAxiDma_BdRingStart(TxRingPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed tx bd start with %d\r\n", Status);
		return XST_FAILURE;
	}*/


	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* This function non-blockingly transmits all packets through the DMA engine.
*
* @param	AxiDmaInstPtr points to the DMA engine instance
*
* @return
* 		- XST_SUCCESS if the DMA accepts all the packets successfully,
* 		- XST_FAILURE if error occurs
*
* @note		None.
*
******************************************************************************/
/*int SendPacket(XAxiDma * AxiDmaInstPtr)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(AxiDmaInstPtr);
	u8 *TxPacket;
	u8 Value;
	XAxiDma_Bd *BdPtr, *BdCurPtr;
	int Status;
	int Index, Pkts;
	u32 BufferAddr;


	if (MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT >
			TxRingPtr->MaxTransferLen) {

		xil_printf("Invalid total per packet transfer length for the "
		    "packet %d/%d\r\n",
		    MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT,
		    TxRingPtr->MaxTransferLen);

		return XST_INVALID_PARAM;
	}

	TxPacket = (u8 *) Packet;

	Value = 0xC;

	for(Index = 0; Index < MAX_PKT_LEN * NUMBER_OF_BDS_TO_TRANSFER;
								Index ++) {
		TxPacket[Index] = Value;

		Value = (Value + 1) & 0xFF;
	}

	Xil_DCacheFlushRange((u32)TxPacket, MAX_PKT_LEN *
							NUMBER_OF_BDS_TO_TRANSFER);

	Status = XAxiDma_BdRingAlloc(TxRingPtr, NUMBER_OF_BDS_TO_TRANSFER,
								&BdPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed bd alloc\r\n");
		return XST_FAILURE;
	}

	BufferAddr = (u32) Packet;
	BdCurPtr = BdPtr;


	for(Index = 0; Index < NUMBER_OF_PKTS_TO_TRANSFER; Index++) {

		for(Pkts = 0; Pkts < NUMBER_OF_BDS_PER_PKT; Pkts++) {
			u32 CrBits = 0;

			Status = XAxiDma_BdSetBufAddr(BdCurPtr, BufferAddr);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set buffer addr %x on BD %x failed %d\r\n",
				(unsigned int)BufferAddr,
				(unsigned int)BdCurPtr, Status);

				return XST_FAILURE;
			}

			Status = XAxiDma_BdSetLength(BdCurPtr, MAX_PKT_LEN,
						TxRingPtr->MaxTransferLen);
			if (Status != XST_SUCCESS) {
				xil_printf("Tx set length %d on BD %x failed %d\r\n",
				MAX_PKT_LEN, (unsigned int)BdCurPtr, Status);

				return XST_FAILURE;
			}

			if (Pkts == 0) {

				CrBits |= XAXIDMA_BD_CTRL_TXSOF_MASK;

#if (XPAR_AXIDMA_0_SG_INCLUDE_STSCNTRL_STRM == 1)

				Status = XAxiDma_BdSetAppWord(BdCurPtr,
				    XAXIDMA_LAST_APPWORD,
				    MAX_PKT_LEN * NUMBER_OF_BDS_PER_PKT);

				if (Status != XST_SUCCESS) {
					xil_printf("Set app word failed with %d\r\n",
					Status);
				}
#endif
			}

			if(Pkts == (NUMBER_OF_BDS_PER_PKT - 1)) {

				CrBits |= XAXIDMA_BD_CTRL_TXEOF_MASK;
			}

			XAxiDma_BdSetCtrl(BdCurPtr, CrBits);
			XAxiDma_BdSetId(BdCurPtr, BufferAddr);

			BufferAddr += MAX_PKT_LEN;
			BdCurPtr = XAxiDma_BdRingNext(TxRingPtr, BdCurPtr);
		}
	}

	Status = XAxiDma_BdRingToHw(TxRingPtr, NUMBER_OF_BDS_TO_TRANSFER,
						BdPtr);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to hw, length %d\r\n",
			(int)XAxiDma_BdGetLength(BdPtr,
					TxRingPtr->MaxTransferLen));

		return XST_FAILURE;
	}

	return XST_SUCCESS;
}*/

int init_acl(XEmacLite *EmacLiteInstPtr)
{
	u32 phyAddress = 0;
	int i = 0, j = 0, init_success = 0;
	print("loop init ael2005...\r\n");
	// try 10 times
	for(i = 0; i < 10; i++)
	{
		xil_printf("try loop init ael2005 %d time\r\n", i + 1);
		ael2005_initialize(EmacLiteInstPtr, phyAddress);
		print("... wait for check \r\n");
		delay(600);
		print("... ael2005 status: ");
		for(j = 0; j < 3; j++)
		{
			init_success = get_acl_status(EmacLiteInstPtr);
			xil_printf("%d", init_success);
			if(init_success)
			{
				xil_printf("-try init ael2005 success\r\n", i + 1);
				break;
			}
			delay(100);
		}
		print("\r\n");
		if(init_success)
		{
			break;
		}
	}
	if(init_success)
	{
		print("loop init ael2005 SUCCESS\r\n");
		return XST_SUCCESS;
	}
	print("loop init ael2005 FAILURE\r\n");
	return XST_FAILURE;
}


void delay(u16 delay_time)
{
	u32 i = delay_time * 0xFFF;
	for(;i>0;i--)
	{
		//
	}
}

int get_acl_status(XEmacLite *InstancePtr){
	int dev;
    u16 pma_status, pcs_status, phy_xs_status;
    dev = 0;
    ael2005_read(InstancePtr, dev, 1, 0x1, &pma_status);
	ael2005_read(InstancePtr, dev, 3, 0x1, &pcs_status);
	ael2005_read(InstancePtr, dev, 4, 0x1, &phy_xs_status);
	//xil_printf("DEBUG: %x, %x, %x\r\n", pma_status, pcs_status, phy_xs_status);
	if(((pma_status>>2) & 0x1) & ((pcs_status>>2) & 0x1) & ((phy_xs_status>>2) & 0x1)){
		return 1;
	}
	return 0;
}
