# invoke by "source run376.gdb"

echo Setting bdm\n

file m.out

target bdm /dev/bdm
#target bdm /dev/icd_bdm0
#target bdm /dev/pd_bdm0

#===========================================================
# sets chipselects and configuration

define set_CS_BR
  #$arg0 = BA31-BA11
  #$arg1 = Offset From REGB
  set *(unsigned int*)($ptr_REGB+$arg1) = ((unsigned int)$arg0)+1
  #p /ox *(unsigned int*)($ptr_REGB+$arg1)
end

define set_CS_OR
  #$arg0 = AM27-AM11
  #$arg1 = Number Of Wait States
  #$arg2 = Sram Port Size
  #$arg3 = Offset From REGB
  set *(unsigned int*)($ptr_REGB+$arg3) = $arg0+(($arg1+1) << 28)+($arg2 << 1)
  #p /ox *(unsigned int*)($ptr_REGB+$arg3)
end

define bdm_hw_init
echo bdm_hw_init ...\n

set remotecache off
bdm_timetocomeup 0
bdm_autoreset off
bdm_setdelay 70
bdm_reset
bdm_setdelay 0
set $sfc=5
set $dfc=5


# system configuration

# MBAR Module Base Address Register
# 31    30          13    12  11  10  9    8         1     0
# BA31  BA30  ....  BA13  0   0   0   AS8  AS7  ...  AS0   V
# BA = BaseAddress
# AS = Address Space = Maskovani adresniho prostoru
# V = data valid
# set *(unsigned int *)0x0003ff00=0
set $sfc=7
set $dfc=7
set $ptr_MBAR = (unsigned int *)0x0003ff00
set *$ptr_MBAR = 0x0ffffe001
set $ptr_DPRBASE = (unsigned char *)0x0ffffe000
set $ptr_REGB = $ptr_DPRBASE + 0x1000
set $sfc=5
set $dfc=5


#diody
set $ptr_PADIR = (unsigned short *)($ptr_REGB + 0x550)
set *$ptr_PADIR = 0xf000
set $ptr_PAPAR = (unsigned short *)($ptr_REGB + 0x552)
set *$ptr_PAPAR = 0
set $ptr_PAODR = (unsigned short *)($ptr_REGB + 0x554)
set *$ptr_PAODR = 0xffff
set $ptr_PADAT = (unsigned short *)($ptr_REGB + 0x556)
set *$ptr_PADAT = 0xefff

# MCR Module Configuration Register - urcuje, zda konfigurace SIM60 se muze cist/zapisovat kdykoli
# 31      30     29   28 ... 17  16    15    14     13  12     11     10  9         8  7     6       5       4  3         0
# BR040ID2-BR040ID0   -      -   BSTM  ASTM  FRZ1-FRZ0  BCLROID2-BCLOID0  SHEN1-SHEN0  SUPV  BCLRISM2-BCLRISM0  IARB3-IARB0
#                                                                                      nebo BCLRIID2-BCLRIID0
# 0       0      0    -      -   0     0     ?       ?
set *(unsigned int *)($ptr_REGB + 0x000) = 0x00006c7f
#set *(unsigned int *)($ptr_REGB + 0x000) = 0x00006c71

#PEPAR config
set *(unsigned short*)($ptr_REGB + 0x16) = 0x0080

#GMR
set *(unsigned int *)($ptr_REGB + 0x40) = 0x00001100

#SYPCR
set *($ptr_REGB + 0x22) = 0x03

#settings for chip selects

#set_CS_BR BaseAddress OffsetFromREGB
#set_CS_OR AddressMask NumberOfWaitStates(0-14) SramPortSize OffsetFromREGB

#CS0 - BootFlash 1 MB - 16Bit
set_CS_BR 0x0000000 0x50
set_CS_OR 0xff00000 0 1 0x54

#CS1 - Flash 2MB - 32Bit
set_CS_BR 0x0200000 0x60
set_CS_OR 0xfe00000 0 0 0x64

#CS7 - SRAM 2 MB - 32Bit
set_CS_BR 0x0400000 0xc0
set_CS_OR 0xfe00000 0 0 0xc4

#CS6 - USB Chip - 8Bit
set_CS_BR 0x0800000 0xb0
set_CS_OR 0xf800000 0 2 0xb4




# CPU registers

# SR=PS Status Register
# 15 14 13 12 11 10  8 7 6 5 4 3 2 1 0
# T1 T0  S  0  0 IP___ 0 0 0 X N Z V C
# 0   0  1  0  0 1 1 1 0 0 0 U U U U U

bdm_setdelay 1

bdm_status

end
#===========================================================

# sets well defined values into VBR
define vbr_set_all
  set $vec_num=0
  set $vbr_val=(unsigned)$vbr
  while $vec_num<256
    set *(unsigned*)($vbr_val+$vec_num*4)=($vec_num*16)+0xf0000
    set $vec_num++
  end
end

# Test writability of RAM location
define bdm_test_ram_acc
  echo testing ...
  p /x $arg0
  set $ram_addr=(unsigned int)$arg0
  set $old_ram_val=*(int*)$ram_addr
  set *(int*)$ram_addr=0x12345678
  if *(int*)$ram_addr!=0x12345678
    echo Error1\n
  end
  set *(char*)$ram_addr=0xab
  if *(int*)$ram_addr!=0xab345678
    echo Error2\n
  end
  set *(char*)($ram_addr+1)=0xcd
  if *(int*)$ram_addr!=0xabcd5678
    echo Error3\n
  end
  set *(char*)($ram_addr+3)=0x01
  if *(int*)$ram_addr!=0xabcd5601
    echo Error4\n
  end
  set *(char*)($ram_addr+2)=0xef
  if *(int*)$ram_addr!=0xabcdef01
    echo Error5\n
  end
  set *(int*)$ram_addr=$old_ram_val
end

# Read flash identification
define bdm_read_flash_id
  set $flash_base=(int)$arg0&~0xffff
  output /x $flash_base
  echo \n
  set *(char*)($flash_base+0x555*2+1)=0xf0
  set *(char*)($flash_base+0x555*2+1)=0xaa
  set *(char*)($flash_base+0x2aa*2+1)=0x55
  set *(char*)($flash_base+0x555*2+1)=0x90
  p /x *(char*)($flash_base+0x00*2+1)
  set *(char*)($flash_base+0x555*2+1)=0xf0
  set *(char*)($flash_base+0x555*2+1)=0xaa
  set *(char*)($flash_base+0x2aa*2+1)=0x55
  set *(char*)($flash_base+0x555*2+1)=0x90
  p /x *(char*)($flash_base+0x01*2+1)
end

define bdm_read_flash1_id
  bdm_read_flash_id 0x800000
end

define bdm_read_flash2_id
  bdm_read_flash_id 0x900000
end

define bdm_test_flash_write
#  set $flash_base=(int)$arg0 & ~0xfffff
  set $flash_base=(int)$arg0
  output /x $flash_base
  echo \n
# set *(char*)($flash_base+0x5555*2+1)=0xf0
#  set *(short*)($flash_base+0x25554)=0xaaaa
#  set *(short*)($flash_base+0x02aaa)=0x5555
#  set *(short*)($flash_base+0x25554)=0xA0A0
  set *(short*)(0x825554)=0xf0f0
  set *(short*)(0x825554)=0xaaaa
  set *(short*)(0x802aaa)=0x5555
  set *(short*)(0x825554)=0xA0A0
  set *(short*)($arg0)=$arg1
end


bdm_hw_init

b do_trap_break
b exception_hook_nop
#b profi_rx_internal
#b write
#b smc_uart_tx
#b smc_interrupt
#b quicc_init

b main

#run

