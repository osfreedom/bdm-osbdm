/*
 * data.c - libelf internal variables.
 * Copyright (C) 1995 - 1998, 2007 Michael Riepe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <private.h>

#ifndef lint
static const char rcsid[] = "@(#) $Id: data.c,v 1.1 2008/06/16 00:01:22 cjohns Exp $";
#endif /* lint */

unsigned _elf_version = EV_NONE;
int _elf_errno = 0;
int _elf_fill = 0;

#if ENABLE_SANITY_CHECKS
#define SANITY_CHECKS	-1
#else
#define SANITY_CHECKS	0
#endif

int _elf_sanity_checks = SANITY_CHECKS;
