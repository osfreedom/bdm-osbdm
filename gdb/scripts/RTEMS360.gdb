#
# Prepare to download and run an RTEMS executable image
#

#
# Load BDM/CPU32 support
#
source ===PREFIX===/lib/gdbScripts/SAL68360.gdb
source ===PREFIX===/lib/gdbScripts/showCPU32Exception.gdb

#
# Connect to the BDM interface
#
target bdm /dev/bdmcpu320
bdm_setdelay 0

#
# Catch exceptions and display the exception stack
#
break _uhoh
commands
displayExceptionStack
end

#
# Regain control on fatal errors
#
break rtems_panic
break _Internal_error_Occurred
break rtems_fatal_error_occurred

#
# Quit when the RTEMS executable finishes
#
break _mainDone
commands
quit
end

#
# Get the hardware to a known state
#
initBoard
