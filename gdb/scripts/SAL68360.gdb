#####################################################################
#       Board setup for SAL Communications Controller card          #
#####################################################################

define initBoard

bdm_reset

#
# Step 4: Set up module base address register
#
set $dpram = 0x0e000000
set $mbar = $dpram|0x101
set $regb = $dpram+0x1000

#
# I/O is to supervisor program space
#
set $dfc=5
set $sfc=5

#
# Step 7: Deal with clock synthesizer
#
set {unsigned char}($regb+0x000c) = 0x8f
set {unsigned char}($regb+0x0010) = 0xd000
set {unsigned short}($regb+0x0014) = 0x8000

#
# Step 8: Initialize system protection
#
set {unsigned char}($regb+0x0022) = 0x7f

#
# Step 9: Clear parameter RAM and reset communication processor module
# Ignore this for now, since all we want to use is the memory controller
#

#
# Step 10: Write PEPAR
#
set {unsigned short}($regb+0x0016) = 0x0180

#
# Step 11: Remap Chip Select 0 (CS0}
#
set {unsigned long}($regb+0x0040) = 0x17940120
set {unsigned long}($regb+0x0054) = 0x4ff80004
set {unsigned long}($regb+0x0050) = 0x0f000003

#
# Step 12: Initialize the system RAM
#
set {unsigned long}($regb+0x0064) = 0x0f000001
set {unsigned long}($regb+0x0060) = 0x00000001
set $ramjnk={unsigned long}0@8

#
# Determine RAM size
#
set {char}0 = 0
if ({char}0x00801000 == 0)
	set {char}0 = 1
	if ({char}0x000801000 == 1)
		set {unsigned long}($regb+0x0040) = 0x178C0120
	end
end

#
# Step 15: Set module configuration register
#
set {unsigned long}($regb+0x0000) = 0x00004c7f

end

define hook-run
initBoard
end
