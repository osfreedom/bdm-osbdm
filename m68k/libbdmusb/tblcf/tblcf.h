/*
    BDM USB common code 
    Copyright (C) 2010 Rafael Campos 
    Rafael Campos Las Heras <rafael@freedom.ind.br>

    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

    Changed to support the BDM project.
    Chris Johns (cjohns@user.sourgeforge.net)
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef TBLCF_H
#define TBLCF_H

#include "bdm_types.h"

/* returns version of the DLL in BCD format */
unsigned char tblcf_version(void);

/* closes currently open device */
//void tblcf_usb_close(int dev);

/* returns status of the last command: 0 on sucess and non-zero on failure */
unsigned char tblcf_get_last_sts(int dev);

/* requests bootloader execution on new power-up, returns 0 on success and
 * non-zero on failure */
unsigned char tblcf_request_boot(int dev);

/* resets the target to normal or BDM mode; returns 0 on success and non-zero
 * on failure */
unsigned char tblcf_target_reset(int dev, target_mode_e target_mode);

/* fills user supplied structure with current state of the BDM communication
 * channel; returns 0 on success and non-zero on failure */
unsigned char tblcf_bdm_sts(int dev, bdmcf_status_t *bdmcf_status);

/* brings the target into BDM mode; returns 0 on success and non-zero on
 * failure */
unsigned char tblcf_target_halt(int dev);

/* starts target execution from current PC address; returns 0 on success and
 * non-zero on failure */
unsigned char tblcf_target_go(int dev);

/* steps over a single target instruction; returns 0 on success and non-zero on
 * failure */
unsigned char tblcf_target_step(int dev);

/* resynchronizes communication with the target (in case of noise, etc.);
 * returns 0 on success and non-zero on failure */
unsigned char tblcf_resynchronize(int dev);

/* asserts the TA signal for the specified time (in 10us ticks); returns 0 on
 * success and non-zero on failure */
unsigned char tblcf_assert_ta(int dev, unsigned char duration_10us);

/* reads control register at the specified address and writes its contents into
 * the supplied buffer; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_creg(int dev, unsigned int address, unsigned long int * result);

/* writes control register at the specified address; returns 0 on success and
 * non-zero on failure */
void tblcf_write_creg(int dev, unsigned int address, unsigned long int value);

/* reads the specified debug register and writes its contents into the
 * supplied buffer; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_dreg(int dev, unsigned char dreg_index, unsigned long int * result);

/* writes specified debug register */
void tblcf_write_dreg(int dev, unsigned char dreg_index, unsigned long int value);

/* reads the specified register and writes its contents into the supplied
 * buffer; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_reg(int dev, unsigned char reg_index, unsigned long int * result);

/* writes specified register */
void tblcf_write_reg(int dev, unsigned char reg_index, unsigned long int value);

/* reads byte from the specified address; returns 0 on success and non-zero on
 * failure */
unsigned char tblcf_read_mem8(int dev, unsigned long int address, unsigned char * result);

/* reads word from the specified address; returns 0 on success and non-zero on
 * failure */
unsigned char tblcf_read_mem16(int dev, unsigned long int address, unsigned int * result);

/* reads long word from the specified address; returns 0 on success and
 * non-zero on failure */
unsigned char tblcf_read_mem32(int dev, unsigned long int address, unsigned long int * result);

/* writes byte at the specified address */
void tblcf_write_mem8(int dev, unsigned long int address, unsigned char value);

/* writes word at the specified address */
void tblcf_write_mem16(int dev, unsigned long int address, unsigned int value);

/* writes long word at the specified address */
void tblcf_write_mem32(int dev, unsigned long int address, unsigned long int value);

/* reads the requested number of bytes from target memory from the supplied
 * address and stores results into the user supplied buffer; uses byte accesses
 * only; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block8(int dev, unsigned long int address, 
                                unsigned long int bytecount, unsigned char *buffer);

/* reads the requested number of bytes from target memory from the supplied
 * address and stores results into the user supplied buffer; uses word
 * accesses; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block16(int dev, unsigned long int address, 
                                 unsigned long int bytecount, unsigned char *buffer);

/* reads the requested number of bytes from target memory from the supplied
 * address and stores results into the user supplied buffer; uses long word
 * accesses; returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block32(int dev, unsigned long int address,
                                 unsigned long int bytecount, unsigned char *buffer);

/* writes the requested number of bytes to target memory from the supplied
 * address; uses byte accesses only; returns 0 on success and non-zero on
 * failure (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns
 * 0) */
unsigned char tblcf_write_block8(int dev, unsigned long int address,
                                 unsigned long int bytecount, unsigned char *buffer);

/* writes the requested number of bytes to target memory at the supplied
 * address; uses word accesses; returns 0 on success and non-zero on failure
 * (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns 0) */
unsigned char tblcf_write_block16(int dev, unsigned long int address,
                                  unsigned long int bytecount, unsigned char *buffer);

/* writes the requested number of bytes to target memory at the supplied
 * address; uses long word accesses; returns 0 on success and non-zero on
 * failure (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns
 * 0) */
unsigned char tblcf_write_block32(int dev, unsigned long int address,
                                  unsigned long int bytecount, unsigned char *buffer);

/* JTAG - go from RUN-TEST/IDLE to SHIFT-DR (mode==0) or SHIFT-IR (mode!=0)
 * state */
unsigned char tblcf_jtag_sel_shift(int dev, unsigned char mode);

/* JTAG - go from RUN-TEST/IDLE to TEST-LOGIC-RESET state */
unsigned char tblcf_jtag_sel_reset(int dev);

/* JTAG - write data; parameter exit: ==0 : stay in SHIFT-xx, !=0 : go to
 * RUN-TEST/IDLE when done; data: shifted in LSB (last byte) first, unused bits
 * (if any) are in the MSB (first) byte */
void tblcf_jtag_write(int dev, unsigned char bit_count, 
                      unsigned char exit, unsigned char *buffer);

/* JTAG - read data; parameter exit: ==0 : stay in SHIFT-xx, !=0 : go to
 * RUN-TEST/IDLE when done; data: shifted in LSB (last byte) first, unused bits
 * (if any) are in the MSB (first) byte */
unsigned char tblcf_jtag_read(int dev, unsigned char bit_count, 
                              unsigned char exit, unsigned char *buffer);


#endif /* TBLCF_H */
