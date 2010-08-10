#
# Display an exception stack
#
define displayExceptionStack
set $frsr = *(unsigned short *)$sp
set $frpc = *(unsigned long *)((unsigned long)$sp + 2)
set $frfvo = *(unsigned short *)((unsigned long)$sp + 6)
set $frcode = $frfvo >> 12
set $frvect = ($frfvo & 0xFFF) >> 2
printf "EXCEPTION -- SR:0x%X  PC:0x%X  FRAME:0x%x  VECTOR:%d\n", $frsr, $frpc, $frcode, $frvect
if $frcode==0x2
  set $fripc = *(unsigned long *)((unsigned long)$sp + 0x8)
  set $frssw = *(unsigned short *)((unsigned long)$sp + 0x16)
  printf "       INSTRUCTION PC:0x%X\n", $fripc,
end
if $frcode==0xC
  set $frfault = *(unsigned long *)((unsigned long)$sp + 0x8)
  set $fripc = *(unsigned long *)((unsigned long)$sp + 0x10)
  set $frssw = *(unsigned short *)((unsigned long)$sp + 0x16)
  printf "       ADDRESS:0x%X  INSTRUCTION PC:0x%X  SSW:0x%X\n", $frfault, $fripc, $frssw
end
end
