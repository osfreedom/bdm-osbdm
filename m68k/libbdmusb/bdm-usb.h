/*
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

#ifndef _BDM_USB_H_
#define _BDM_USB_H_

#if __cplusplus
extern "C" 
{
#endif

#include "bdm-iface.h"

#include "tblcf/tblcf.h"
#include "tblcf/tblcf_usb.h"
#include "usbdm/usbdm.h"

int bdm_usb_open(const char* name, bdm_iface** iface);
int bdm_usb_set_options(int dev_dec, usbdm_options_type_e *cfg_options);

#if __cplusplus
}
#endif

#endif
