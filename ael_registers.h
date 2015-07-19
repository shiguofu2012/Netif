/*******************************************************************************
 *
 *  NetFPGA-10G http://www.netfpga.org

 *
 *  File:
 *        helloworld.h
 *
 *  Project:
 *        nic
 *
 *  Author:
 *        Mario Flajslik
 *
 *  Description:
 *        Example header file to initialize AEL2005 in 10G mode through
 *        MDIO and dump PHY chip status.
 *
 *        Currently only 10GBASE-SR and Direct Attach are supported.
 *        This firmware will detect port mode according to SFF-8472.
 *        The default mode is Direct Attach.
 *
 *        Our experiment shows that the Direct Attach EDC also applies
 *        to 10GBASE-SR modules. To save the BRAM resource and minimize
 *        the program size, both 10GBASE-SR and Direct Attach use the
 *        same EDC code. If you have problems getting 10GBASE-SR running
 *        please enable 10GBASE-SR EDC by setting AEL2005_SR marco to 1.
 *
 *  Copyright notice:
 *        Copyright (C) 2010, 2011 The Board of Trustees of The Leland Stanford
 *                                 Junior University
 *
 *  Licence:
 *        This file is part of the NetFPGA 10G development base package.
 *
 *        This file is free code: you can redistribute it and/or modify it under
 *        the terms of the GNU Lesser General Public License version 2.1 as
 *        published by the Free Software Foundation.
 *
 *        This package is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *        Lesser General Public License for more details.
 *
 *        You should have received a copy of the GNU Lesser General Public
 *        License along with the NetFPGA source package.  If not, see
 *        http://www.gnu.org/licenses/.
 *
 */

#ifndef AEL_REGISTERS_H
#define AEL_REGISTERS_H
#include"xil_io.h"
#include"xemaclite.h"
#include"xwdttb_l.h"
enum {
	AEL_I2C_CTRL        = 0xc30a,
	AEL_I2C_DATA        = 0xc30b,
	AEL_I2C_STAT        = 0xc30c
};

/* PHY module I2C device address */
enum {
	MODULE_DEV_ADDR	= 0xa0,
	SFF_DEV_ADDR	= 0xa2,
};

enum {
	MODE_TWINAX,
	MODE_SR,
	MODE_LR, // Not supported
	MODE_LRM // Not supported
};


extern const u16 sw_rst[];
extern const u16 plat_mdio_patch[];
extern const u16 trans_mdio_patch[];
extern const u16 act_uc_pause[];
extern const u16 twinax_edc[];
extern const u16 regs0[];
extern const u16 regs1[];


int ael2005_read (XEmacLite *InstancePtr, u32 PhyAddress, u32 PhyDev, u16 address, u16 *data);
int ael2005_write(XEmacLite *InstancePtr, u32 PhyAddress, u32 PhyDev, u16 address, u16 data);
// The following functions are commented out to minimize executable size
int ael2005_i2c_write(XEmacLite *InstancePtr, u32 PhyAddress, u16 dev_addr, u16 word_addr, u16 data);
int ael2005_i2c_read (XEmacLite *InstancePtr, u32 PhyAddress, u16 dev_addr, u16 word_addr, u16 *data);
int ael2005_sleep(int ms);
int ael2005_initialize(XEmacLite *InstancePtr, u32 PhyAddress);
int test_status(XEmacLite *InstancePtr);

#endif
