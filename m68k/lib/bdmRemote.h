/*
 * Motorola Background Debug Mode Remote Library
 * Copyright (C) 1998  Chris Johns
 *
 * Based on `ser-tcp.c' in the gdb sources.
 *
 * 31-11-1999 Chris Johns (ccj@acm.org)
 * Extended to support remote operation. See bdmRemote.c for details.
 *
 * Chris Johns <cjohns@users.sourceforge.net>
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
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _BDM_REMOTE_LIB_H_
#define _BDM_REMOTE_LIB_H_

#if __cplusplus
extern "C" 
{
#endif

#include "bdm-iface.h"
#include "BDMlib.h"

/*
 * Internal to the BDM library.
 */

int bdmRemoteName (const char *name);
int bdmRemoteOpen (const char *name, bdm_iface** iface);

#if __cplusplus
}
#endif

#endif
