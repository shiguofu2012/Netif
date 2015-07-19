/*
 * registers.c
 *
 *  Created on: 2013-6-24
 *      Author: Administrator
 */
#include "registers.h"
#include "xil_assert.h"
#include "xil_io.h"
#include"xil_cache.h"

inline u32 ReadRecvDesc(u16 offset)
{
	u32 addr = RX_DESC_BASEADDRESS + offset * RX_DESC_OFFSET;

	Xil_AssertNonvoid(offset < RX_IPIF_MAXOFFSET );
	return Xil_In32(addr);
}

inline u32 ReadRecvBuffAddr(u16 offset)
{
	u32 addr =  RX_ADDR_BASEADDRESS + offset * RX_ADDR_OFFSET;

	Xil_AssertNonvoid(offset < RX_IPIF_MAXOFFSET );
	return Xil_In32(addr);
}
//read high 16 bits of descriptor
inline u32 ReadRecvCmdInfo(u16 offset)
{
	return ( ReadRecvDesc(offset) & (COMMANDINFOMASK) );
}
inline void WriteRecvDesc(u16 offset,u32 value)
{
	u32 addr = RX_DESC_BASEADDRESS + offset * RX_DESC_OFFSET;

	Xil_AssertVoid(offset < (RX_IPIF_MAXOFFSET) );
	Xil_Out32(addr,value);
}

/*
void WriteRecvDescAddr(u16 offset, u32 value)
{
	u32 addr = RX_DESC_BASEADDRESS + offset * RX_DESC_OFFSET + 4;
	Xil_AssertVoid(offset < (RX_IPIF_MAXOFFSET) );
	Xil_Out32(addr,value);
}
*/

inline u32 ReadSendDesc(u16 offset)
{
	u32 addr = TX_DESC_BASEADDRESS + offset * TX_DESC_OFFSET;
//	xil_printf("addr:%8.8x\r\n",addr);
	Xil_AssertNonvoid(offset < TX_IPIF_MAXOFFSET );
	return Xil_In32(addr);
}
inline u32 ReadSendBuffAddr(u16 offset)
{
	u32 addr =  TX_ADDR_BASEADDRESS + offset * TX_ADDR_OFFSET;

	Xil_AssertNonvoid(offset < (TX_IPIF_MAXOFFSET) );
	return Xil_In32(addr);
}

inline void WriteSendDesc(u16 offset,u32 value)
{
	u32 addr = TX_DESC_BASEADDRESS + offset * TX_DESC_OFFSET;

	Xil_AssertVoid(offset < TX_IPIF_MAXOFFSET );
	Xil_Out32(addr,value);
}
/*
void WriteSendDescAddr(u16 offset, u32 value)
{
	u32 addr = TX_DESC_BASEADDRESS + offset * TX_DESC_OFFSET + 4;
	Xil_AssertVoid(offset < TX_IPIF_MAXOFFSET);
	Xil_Out32(addr, value);
}
*/
