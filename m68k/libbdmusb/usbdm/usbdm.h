/*
    BDM USB common code 
    Copyright (C) 2010 Rafael Campos 
    Rafael Campos Las Heras <rafael@freedom.ind.br>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef USBDM_H
#define USBDM_H

#include "../bdmusb.h"

unsigned char usbdm_get_capabilities(bdmusb_dev* dev);
unsigned char usbdm_set_options(bdmusb_dev* dev);
//unsigned char usbdm_set_speed( bdmusb_dev* dev);
unsigned char usbdm_connect(int dev);
#endif /* USBDM_H */