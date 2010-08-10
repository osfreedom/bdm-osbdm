#!/bin/sh
MAJOR=`modstat -n bdm | tail -1 | awk '{print $3}'`
if [ ! -c /dev/bdmcpu320 ]
then
  mknod -m 666 /dev/bdmcpu320 c $MAJOR 0
fi
if [ ! -c /dev/bdmcpu321 ]
then
  mknod -m 666 /dev/bdmcpu321 c $MAJOR 1
fi
if [ ! -c /dev/bdmcpu322 ]
then
  mknod -m 666 /dev/bdmcpu322 c $MAJOR 2
fi
if [ ! -c /dev/bdmcf0 ]
then
  mknod -m 666 /dev/bdmcf0 c $MAJOR 4
fi
if [ ! -c /dev/bdmcf1 ]
then
  mknod -m 666 /dev/bdmcf1 c $MAJOR 5
fi
if [ ! -c /dev/bdmcf2 ]
then
  mknod -m 666 /dev/bdmcf2 c $MAJOR 6
fi
if [ ! -c /dev/bdmidc0 ]
then
  mknod -m 666 /dev/bdmidc0 c $MAJOR 8
fi
if [ ! -c /dev/bdmidc1 ]
then
  mknod -m 666 /dev/bdmidc1 c $MAJOR 9
fi
if [ ! -c /dev/bdmidc2 ]
then
  mknod -m 666 /dev/bdmidc2 c $MAJOR 10
fi
echo "created bdm devices, major number $MAJOR"
ls -l /dev/bdm*
