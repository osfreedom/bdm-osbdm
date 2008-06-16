/* $Id:
 *
 * Portions of this program which I authored may be used for any purpose
 * so long as this notice is left intact. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

# include "flashcfm.h"
# include "flash_filter.h"
# include <stdint.h>

# if HOST_FLASHING
# include <stdio.h>
# include <BDMlib.h>
# include <string.h>
# endif

#define __IPSBAR                             0x40000000
#define MCF_CFM_CFMMCR                       (__IPSBAR + 0x1D0000) //16
#define MCF_CFM_CFMCLKD                      (__IPSBAR + 0x1D0002) //8
#define MCF_CFM_CFMUSTAT                     (__IPSBAR + 0x1D0020) //8
#define MCF_CFM_CFMCMD                       (__IPSBAR + 0x1D0024) //8
#define MCF_CFM_CFMPROT                      (__IPSBAR + 0x1D0010) //32
#define MCF_CFM_CFMSACC                      (__IPSBAR + 0x1D0014) //32
#define MCF_CFM_CFMDACC                      (__IPSBAR + 0x1D0018) //32

#define MCF_CFM_CFMCLKD_DIV(x)               (((x)&0x3F)<<0)
#define MCF_CFM_CFMCLKD_PRDIV8               (0x40)
#define MCF_CFM_CFMCLKD_DIVLD                (0x80)

#define MCF_CFM_CFMUSTAT_BLANK               (0x04)
#define MCF_CFM_CFMUSTAT_CCIF                (0x40)
#define MCF_CFM_CFMUSTAT_CBEIF               (0x80)

#define MCF_CFM_CFMCMD_BLANK_CHECK           (0x5)
#define MCF_CFM_CFMCMD_PAGE_ERASE_VERIFY     (0x6)
#define MCF_CFM_CFMCMD_WORD_PROGRAM          (0x20)
#define MCF_CFM_CFMCMD_PAGE_ERASE            (0x40)
#define MCF_CFM_CFMCMD_MASS_ERASE            (0x41)

#define MCF_CLOCK_SYNCR           (__IPSBAR + 0x120000) //16
#define MCF_CLOCK_SYNSR           (__IPSBAR + 0x120002) //8
#define MCF_CLOCK_CCHR            (__IPSBAR + 0x120008) //8
        
#define MCF_CLOCK_SYNSR_LOCK      (0x08)
#define MCF_CLOCK_CCHR_PFD(x)     (((x)&0x07)<<0)

typedef struct {
    uint32_t backdoor_address_start;
    uint32_t backdoor_address_end;
    uint32_t flash_address;
} chiptype_t;

# if HOST_FLASHING

static inline uint8_t read_byte(uint32_t a_addr)
{
    uint8_t result;
    if(bdmReadByte(a_addr, &result) < 0)
        fprintf (stderr, "read_byte(0x%08x): %s\n", a_addr, bdmErrorString());
    return result;
}

static inline uint16_t read_word(uint32_t a_addr)
{
    uint16_t result;
    if(bdmReadWord(a_addr, &result) < 0)
        fprintf (stderr, "read_word(0x%08x): %s\n", a_addr, bdmErrorString());
    return result;
}

static inline uint32_t read_longword(uint32_t a_addr)
{
    unsigned long result;
    if(bdmReadLongWord(a_addr, &result) < 0)
        fprintf (stderr, "read_longword(0x%08x): %s\n", a_addr, bdmErrorString());
    return (uint32_t) result;
}

static inline void write_byte(uint32_t a_addr, uint8_t a_x)
{
    if(bdmWriteByte(a_addr, a_x) < 0)
        fprintf (stderr, "write_byte(0x%08x): %s\n", a_addr, bdmErrorString());
}

static inline void write_word(uint32_t a_addr, uint16_t a_x)
{
    if(bdmWriteWord(a_addr, a_x) < 0)
        fprintf (stderr, "write_word(0x%08x): %s\n", a_addr, bdmErrorString());
}

static inline void write_longword(uint32_t a_addr, uint32_t a_x)
{
    if(bdmWriteLongWord(a_addr, a_x) < 0)
        fprintf (stderr, "write_longword(0x%08x): %s\n", a_addr, bdmErrorString());
}

static inline uint32_t swap32(uint32_t a_v)
{
    return (uint32_t) (((a_v >> 24) & 0x000000ff) | ((a_v << 24) & 0xff000000) | 
           ((a_v >> 8) & 0x0000ff00) | ((a_v << 8) & 0x00ff0000));
}

#else

#define read_byte(A) (*((volatile uint8_t *) (A)))
#define read_word(A) (*((volatile uint16_t *) (A)))
#define read_longword(A) (*((volatile uint32_t *) (A)))
#define write_byte(A, B) *((volatile uint8_t *)(A)) = (B)
#define write_word(A, B) *((volatile uint16_t *)(A)) = (B)
#define write_longword(A, B) *((volatile uint32_t *)(A)) = (B)

#endif
 
/* We need a unique symbol to associate a (target-based) plugin with its
   (host-based) driver.
*/
static char driver_magic[] = "flashcfm";

# if HOST_FLASHING
static char *prog_entry (void)
{
    return "flashcfm_prog";
}
# endif

/* The actual programming function
 */
static int flashcfm_prog (void *chip_descr,
		         unsigned long pos, unsigned char *data,
		         unsigned long cnt)
{
    chiptype_t *ct = (chiptype_t *) chip_descr;
    uint32_t offset = ct->backdoor_address_start + (pos - ct->flash_address);
    int n = 0;
    cnt /= 4;
    
    if (read_byte(MCF_CFM_CFMCLKD) & MCF_CFM_CFMCLKD_DIVLD)
	{
		while(!(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CBEIF))
		{
		}
		
		for(n = 0; n < cnt; ++n)
		{
# if HOST_FLASHING
            // when flashing from the host we need to swap the endianess as 
            // the memory coming in is in the target machine endianness.  given
            // the longword write already does an endianess swap, we need to
            // swap here such that it gets swapped back!
            write_longword(offset + n * 4, swap32(((uint32_t *) data)[n]));
#else 
            // this is for the plugin
            write_longword(offset + n * 4, ((uint32_t *) data)[n]);
#endif
			write_byte(MCF_CFM_CFMCMD, MCF_CFM_CFMCMD_WORD_PROGRAM);
			write_byte(MCF_CFM_CFMUSTAT, MCF_CFM_CFMUSTAT_CBEIF);

			if(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CBEIF)
			{
				continue;
			}
			
			while( !(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CCIF))
			{
			}
		}
	}

	return n * 4;
}

/* Set required clock speeds for flashing, and disable any flash protection
 */
static void init_chip(void *chip_descr)
{
    if (!(read_byte(MCF_CFM_CFMCLKD) & MCF_CFM_CFMCLKD_DIVLD))
	{
        /* I'm going to set the system clock here!
         * It would be better to read it!
         */

        
        /* Clock source is 25.0000 MHz external crystal 
           Clock mode: Normal PLL mode 
           Processor/Bus clock frequency = 60.00 MHz 
           Loss of clock detection disabled 
           Reset on loss of lock disabled 
        */
        
        /* Divide 25.0000 MHz clock to get 5.00 MHz PLL input clock */
        write_byte(MCF_CLOCK_CCHR, MCF_CLOCK_CCHR_PFD(0x4));

        /* Set RFD+1 to avoid frequency overshoot and wait for PLL to lock */
        write_word(MCF_CLOCK_SYNCR, 0x4103);
        while ((read_byte(MCF_CLOCK_SYNSR) & MCF_CLOCK_SYNSR_LOCK) == 0)
            ;

        /* Set desired RFD=0 and MFD=4 and wait for PLL to lock */
        write_word(MCF_CLOCK_SYNCR, 0x4003);
        while ((read_byte(MCF_CLOCK_SYNSR) & MCF_CLOCK_SYNSR_LOCK) == 0)
            ;
        /* Switch to using PLL */        
        write_word(MCF_CLOCK_SYNCR, 0x4007);
        
        /* Set the flash clock divider
         */
        write_byte(MCF_CFM_CFMCLKD, 
                   MCF_CFM_CFMCLKD_DIV(21) | MCF_CFM_CFMCLKD_PRDIV8);
    }

    write_longword(MCF_CFM_CFMPROT, 0x0);
    write_longword(MCF_CFM_CFMSACC, 0x0);
    write_longword(MCF_CFM_CFMDACC, 0x0);
    write_word(MCF_CFM_CFMMCR, 0x0);
}

/* Initiate erase operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is erased. The erase operation
   can be called several times before flash_29_erase_wait() is called for
   simultanous erasure of multiple sectors.
 */
static void flashcfm_erase (void *chip_descr, long sector_address)
{
    chiptype_t *ct = (chiptype_t *) chip_descr;
    
	if(read_byte(MCF_CFM_CFMCLKD) & MCF_CFM_CFMCLKD_DIVLD)
	{
		while(!(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CBEIF))
		{
		}

        if(sector_address == -1)
        {
            write_longword(ct->backdoor_address_start, 0);
            write_byte(MCF_CFM_CFMCMD, MCF_CFM_CFMCMD_MASS_ERASE);
        }
        else
        {
            write_longword(ct->backdoor_address_start + (sector_address - ct->flash_address), 0);
            write_byte(MCF_CFM_CFMCMD, MCF_CFM_CFMCMD_PAGE_ERASE);
        }
        write_byte(MCF_CFM_CFMUSTAT, MCF_CFM_CFMUSTAT_CBEIF);
	}
    else
    {
        //fprintf (stderr, "MCF_CFM_CFMCLKD not set\n");
    }
}

/* Blank check operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is checked.
 */
static int flashcfm_blank_chk (void *chip_descr, long sector_address)
{
    chiptype_t *ct = (chiptype_t *) chip_descr;
    int result = 0;
    
	if(read_byte(MCF_CFM_CFMCLKD) & MCF_CFM_CFMCLKD_DIVLD)
	{
		while(!(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CBEIF))
		{
		}

        if(sector_address == -1)
        {
            write_longword(ct->backdoor_address_start, 0);
            write_byte(MCF_CFM_CFMCMD, MCF_CFM_CFMCMD_BLANK_CHECK);
        }
        else
        {
            write_longword(ct->backdoor_address_start + (sector_address - ct->flash_address), 0);
            write_byte(MCF_CFM_CFMCMD, MCF_CFM_CFMCMD_PAGE_ERASE_VERIFY);
        }
        write_byte(MCF_CFM_CFMUSTAT, MCF_CFM_CFMUSTAT_CBEIF);
        
		while( !(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CCIF))
		{
		}
        
        if(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_BLANK)
        {
            result = 1;
            write_byte(MCF_CFM_CFMUSTAT, MCF_CFM_CFMUSTAT_BLANK);
        }
	}
    else
    {
        //fprintf (stderr, "MCF_CFM_CFMCLKD not set\n");
    }
    
    return result;
}

/* wait for queued erasing operations to finish
 */
static int flashcfm_erase_wait (void *chip_descr)
{
	if(read_byte(MCF_CFM_CFMCLKD) & MCF_CFM_CFMCLKD_DIVLD)
	{
		while( !(read_byte(MCF_CFM_CFMUSTAT) & MCF_CFM_CFMUSTAT_CCIF))
		{
		}
        return 1;
	}
    else
    {
        //fprintf (stderr, "MCF_CFM_CFMCLKD not set\n");
    }
    return 0;
}

# if HOST_FLASHING

/* autodetect
 */
static int flashcfm_search_chip (void *chip_descr, char *description,
				unsigned long pos)
{
    chiptype_t *ct = (chiptype_t *) chip_descr;
    
    /* TODO: some propper detection here.  fornow, read what we think
     * is flash bar, and see if it is at the same address!
     */
    
    unsigned long flashbar = 0;
    if(bdmReadControlRegister(0x0C04, &flashbar) >= 0)
    {
        if((flashbar & 1) && ((flashbar & 0xfff80000) == pos))
        {
            ct->backdoor_address_start = 0x44000000;
            ct->backdoor_address_end = 0x44040000;
            ct->flash_address = (uint32_t) pos;
            
            init_chip(ct);
            
            strcpy(description, "cfm256");
            return 256*1024;
        }
    }
    return 0;
}

/* FIXME: this function assumes that data sizes and alignment requirements
   on target and host are identical.
*/
static int download_struct (void *chip_descr, unsigned long adr)
{
    chiptype_t *ct = (chiptype_t *) chip_descr;

    bdmWriteLongWord(adr, ct->backdoor_address_start); adr += 4;
    bdmWriteLongWord(adr, ct->backdoor_address_end); adr += 4;
    bdmWriteLongWord(adr, ct->flash_address); adr += 4;

    return adr;
}


void init_flashcfm (int num)
{
    register_algorithm (num, driver_magic, sizeof(chiptype_t),
			download_struct,
			flashcfm_search_chip,
			flashcfm_erase, 
            flashcfm_blank_chk,
            flashcfm_erase_wait, 
            flashcfm_prog,
			prog_entry);
}
#else
void init_flashcfm (int num)
{
    register_algorithm (num, driver_magic, 0,
			0,
			0,
			flashcfm_erase, 
            flashcfm_blank_chk,
            flashcfm_erase_wait, 
            flashcfm_prog,
			0);
}
#endif
