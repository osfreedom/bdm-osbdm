# invoke by "source run376.gdb"

echo Setting bdm\n

#set prompt (gdb68)

file tst

target bdm /dev/bdm
#target bdm /dev/icd_bdm0
#target bdm /dev/pd_bdm0

#===========================================================
# sets chipselects and configuration
define bdm_hw_init
echo bdm_hw_init ...\n

set remotecache off
bdm_timetocomeup 0
bdm_autoreset off
bdm_setdelay 100
bdm_reset
bdm_setdelay 0
set $sfc=5
set $dfc=5

# system configuration

# 0xFFFA00 - SIMCR - SIM Configuration Register
# 15    14    13    12 11     10 9  8 7    6  5 4 3     0
# EXOFF FRZSW FRZBM 0  SLVEN  0  SHEN SUPV MM 0 0 IARB
# 0     0     0     0  DATA11 0  0  0 1    1  0 0 1 1 1 1
set *(short *)0xfffa00=0x42cf

# 0xFFFA21 - SYPCR - System Protection Control Register 
# 7   6      5 4 3   2   1 0
# SWE SWP    SWT HME BME BMT
# 1   MODCLK 0 0 0   0   0 0
set *(char *)0xfffa21=0x06

# 0xYFFA27 - SWSR - Software Service Register
# write 0x55 0xAA for watchdog 

# 0xFFFA04 - SYNCR Clock Synthesizer Control Register 
# 15 14 13        8 7    6 5 4     3     2     1     0
# W  X  Y           EDIV 0 0 SLIMP SLOCK RSTEN STSIM STEXT
# 0  0  1 1 1 1 1 1 0    0 0 U     U     0     0     0
set *(short *)0xfffa04=0x7f00

# $YFFA17 - PEPAR - Port E Pin Assignment Register
# 7     6     5     4     3     2     1      0
# PEPA7 PEPA6 PEPA5 PEPA4 PEPA3 PEPA2 PEPA1  PEPA0
# SIZ1  SIZ0  AS    DS    RMC   AVEC  DSACK1 DSACK0
#   1 .. control signal, 0 .. port F
#   after reset determined by DATA8
set *(char*)0xfffa17=0xf4

# 0xFFFA1F - PFPAR - Port F Pin Assignment Register
# 7     6     5     4     3     2     1     0
# PFPA7 PFPA6 PFPA5 PFPA4 PFPA3 PFPA2 PFPA1 PFPA0
# INT7  INT6  INT5  INT4  INT3  INT2  INT1  MODCLK
#   1 .. control signal, 0 .. port F
#   after reset determined by DATA9
set *(char*)0xfffa1f=0

# Setup internal RAM

# setup TPU RAM at 0x8000
#   TRAMMCR
set *(short *)0xFFFB00=0x8000
#   TRAMBAR
set *(short *)0xFFFB04=0xFFE000>>8
#   TRAMMCR
set *(short *)0xFFFB00=0

# 0xYFFA44 - CSPAR0 - Chip Select Pin Assignment Register 0 
# 15 14 13 12    11 10    9 8      7 6      5 4      3 2      1 0
# 0  0  CSPA0[6] CSPA0[5] CSPA0[4] CSPA0[3] CSPA0[2] CSPA0[1] CSBOOT
# 0  0  DATA2 1  DATA2 1  DATA2 1  DATA1 1  DATA1 1  DATA1 1  1 DATA0
#       CS5      CS4      CS3      CS2      CS1      CS0      CSBOOT
#       FC2 PC2  FC1 PC1  FC0 PC0  BGACK    BG       BR
#
# 00 Discrete Output
# 01 Alternate Function
# 10 Chip Select (8-Bit Port)
# 11 Chip Select (16-Bit Port)
#
set *(short *)0xfffa44=0x3fff


# 0xFFFA46 - CSPAR1 - Chip Select Pin Assignment Register 1
# 15 14 13 12 11 10 9 8      7 6      5 4      3 2      1 0
# 0  0  0  0  0  0  CSPA1[4] CSPA1[3] CSPA1[2] CSPA1[1] CSPA1[0]
# 0  0  0  0  0  0  DATA7 1  DATA76 1 DATA75 1 DATA74 1 DATA73 1
#                   CS10     CS9      CS8      CS7      CS6
#                   A23 ECLK A22 PC6  A21 PC5  A20 PC4  A19 PC3
#
set *(short *)0xfffa46=0x03ff

#
# Chip selects configuration
#
# 0xFFFA48 - CSBARBT - Chip-Select Base Address Register Boot ROM
# 0xFFFA4C..0xFFFA74 - CSBAR[10:0] - Chip-Select Base Address Registers
# 15  14  13  12  11  10  9   8   7   6   5   4   3   2   0
# A23 A22 A21 A20 A19 A18 A17 A16 A15 A14 A13 A12 A11 BLKSZ
# reset 0x0003 for CSBARBT and 0x0000 for CSBAR[10:0]  
#
# BLKSZ Size Address Lines Compared
# 000   2k   ADDR[23:11]
# 001   8k   ADDR[23:13]
# 010   16k  ADDR[23:14]
# 011   64k  ADDR[23:16]
# 100   128k ADDR[23:17]
# 101   256k ADDR[23:18]
# 110   512k ADDR[23:19]
# 111   1M   ADDR[23:20]
#
#
# 0xFFFA4A - CSORBT - Chip-Select Option Register Boot ROM
# 0xFFFA4E..0xFFFA76 - CSOR[10:0] - Chip-Select Option Registers
# 15   14 13  12 11 10   9       6  5  4  3 1  0
# MODE BYTE   R/W   STRB DSACK      SPACE IPL  AVEC
# 0    1  1   1  1  0    1 1 0 1 1  1  0  0 0  0  - for CSORBT
#
# BYTE  00 Disable, 01 Lower Byte, 10 Upper Byte, 11 Both Bytes
# R/W   00 Reserved,01 Read Only,  10 Write Only, 11 Read/Write
# SPACE 00 CPU,     01 User,       10 Supervisor, 11 Supervisor/User
#
set *(long *)0xfffa48=0x0e0468b0  
# BOOT ROM 0x0e0000 128k  RO UL

set *(long *)0xfffa4c=0x0003503e  
# CS0  RAM 0x000000 64k   WR U

set *(long *)0xfffa50=0x0003303e  
# CS1  RAM 0x000000 64k   WR L

set *(long *)0xfffa54=0x0003683e  
# CS2  RAM 0x000000 64k   RO UL

set *(long *)0xfffa58=0x00000000  
# CS3

set *(long *)0xfffa5C=0xfff8680f  
# CS4

set *(long *)0xfffa60=0xffe8783f  
# CS5

set *(long *)0xfffa64=0x100438f0  
# CS6  R/R 0x100000 128k  RW L

set *(long *)0xfffa68=0x100458f0  
# CS7  R/R 0x100000 128k  RW U

set *(long *)0xfffa6c=0x01036870  
# CS8  RAM 0x010000 64k   RO UL

set *(long *)0xfffa70=0x01033030  
# CS9  RAM 0x010000 64k   WR L

set *(long *)0xfffa74=0x01035030  
# CS10 RAM 0x010000 64k   WR U

#
# My change
#
set *(long *)0xfffa58=0x02036870  
# CS3  RAM 0x020000 64k   RO UL

set *(long *)0xfffa64=0x02033030  
# CS6  RAM 0x020000 64k   WR L

set *(long *)0xfffa68=0x02035030  
# CS7  RAM 0x020000 64k   WR U


# CPU registers

# SR=PS Status Register
# 15 14 13 12 11 10  8 7 6 5 4 3 2 1 0
# T1 T0  S  0  0 IP___ 0 0 0 X N Z V C
# 0   0  1  0  0 1 1 1 0 0 0 U U U U U

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
    printf "Error1  %08X\n",*(int*)$ram_addr
  end
  set *(char*)$ram_addr=0xab
  if *(int*)$ram_addr!=0xab345678
    printf "Error2  %08X\n",*(int*)$ram_addr
  end
  set *(char*)($ram_addr+1)=0xcd
  if *(int*)$ram_addr!=0xabcd5678
    printf "Error3  %08X\n",*(int*)$ram_addr
  end
  set *(char*)($ram_addr+3)=0x01
  if *(int*)$ram_addr!=0xabcd5601
    printf "Error4  %08X\n",*(int*)$ram_addr
  end
  set *(char*)($ram_addr+2)=0xef
  if *(int*)$ram_addr!=0xabcdef01
    printf "Error5  %08X\n",*(int*)$ram_addr
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
  set $flash_base=(int)$arg0 & ~0xffff
  output /x $flash_base
  echo \n
  set *(char*)($flash_base+0x555*2+1)=0xf0
  set *(char*)($flash_base+0x555*2+1)=0xaa
  set *(char*)($flash_base+0x2aa*2+1)=0x55
  set *(char*)($flash_base+0x555*2+1)=0xA0
  set *(char*)($arg0)=$arg1
end

define bdm_test_pwm0

  #BIUMCR - BIU Module Configuration Register $YFF400
  set *(short*)0xfff400=*(short*)0xfff400&~0x8000
  #CPCR - CPSM Control Register $YFF408  
  set *(short*)0xfff408=*(short*)0xfff408|8
  #PWM5SIC - PWM5 Status/Interrupt/Control Register $YFF428
  set *(short*)0xfff428=0x18
  #PWM5A1 - PWM5 Period Register $YFF42A
  set *(short*)0xfff42a=512
  #PWM5B1 - PWM5 Pulse Width Register $YFF42C
  set *(short*)0xfff42c=0

  if $arg0==0
    set *(short*)0xf87000=0
    set $pwm_val=0
  else
    if $arg0>0
      set *(char*)0xf87000=1
      set $pwm_val=$arg0
    else
      set *(char*)0xf87000=2
      set $pwm_val=-($arg0)
    end
  end
  #DDRQA
  set *(short*)0xfff208=0x8000
  #PORTQA
  set *(short*)0xfff206=~0x8000

  #PWM5B1 - PWM5 Pulse Width Register $YFF42C
  set *(short*)0xfff42c=$pwm_val

end

define bdm_test_usd_irc
  set $usd_irc_d=0xf88000
  set $usd_irc_c=0xf88001
  if $arg0!=0
    # Load Gate
    set *(unsigned char*)0xf88020=0
    # CMR
    set *(unsigned char*)$usd_irc_c=0x38
    # IOR
    set *(unsigned char*)$usd_irc_c=0x49
    # IDR
    set *(unsigned char*)$usd_irc_c=0x61
    # RLD - Reset BP, BT CT CPT S
    set *(unsigned char*)$usd_irc_c=0x05
    # DATA - PSC
    set *(unsigned char*)$usd_irc_d=0x02
    # RLD - Reset BP, PR0 -> PSC
    set *(unsigned char*)$usd_irc_c=0x1B
  end
  # RLD - Reset BP, CNTR -> OL
  set *(unsigned char*)$usd_irc_c=0x11

  set $usd_irc_val=((int)(*(unsigned char*)$usd_irc_d))
  set $usd_irc_val+=((int)(*(unsigned char*)$usd_irc_d))<<8
  set $usd_irc_val+=((int)(*(unsigned char*)$usd_irc_d))<<16
  print /x $usd_irc_val

end

bdm_hw_init

#b main

#run

