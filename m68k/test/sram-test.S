/*
 *
 *  This file contains the entry point for the application.
 *  The name of this entry point is compiler dependent.
 *  It jumps to the BSP which is responsible for performing
 *  all initialization.
 *
 *  Author:
 *    David Fiddes, D.J.Fiddes@hw.ac.uk
 *    http://www.calm.hw.ac.uk/davidf/coldfire/
 *
 *    Chris Johns, ccj@acm.org
 *    home web page, arrh you could then see me ...
 *
 *  COPYRIGHT (c) 1989-1998.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright assigned to U.S. Government, 1994.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  This is a hack up to get some code to be able to download
 *  to the SRAM of a coldfire to test single step, go and
 *  hitting a break.
 *
 */

#define SRAMBAR 0x20000000

        .sect code
        
entry:

        move.w  #0x2700,%sr    | First turn off all interrupts!
        move.l  #SRAMBAR,%sp    | Put the stack in SRAM
        movea.l #0xa0a0a0a0,%a0    
        movea.l #0xa1a1a1a1,%a1    
        movea.l #0xa2a2a2a2,%a2    
        movea.l #0xa3a3a3a3,%a3    
        movea.l #0xa4a4a4a4,%a4    
        movea.l #0xa5a5a5a5,%a5    
        movea.l #0xa6a6a6a6,%a6    
        movea.l #0xa7a7a7a7,%a7    
        move.l  #0xd0d0d0d0,%d0    
        move.l  #0xd1d1d1d1,%d1    
        move.l  #0xd2d2d2d2,%d2    
        move.l  #0xd3d3d3d3,%d3    
        move.l  #0xd4d4d4d4,%d4    
        move.l  #0xd5d5d5d5,%d5    
        move.l  #0xd6d6d6d6,%d6    
        move.l  #0xd7d7d7d7,%d7    

        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop

        halt

