/* Register support routines for the remote server for GDB.
   Copyright (C) 2001, 2002, 2004, 2005, 2007 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "server.h"
#include "regdef.h"

#include <stdlib.h>
#include <string.h>

extern int remote_debug;

/* The private data for the register cache.  Note that we have one
   per inferior; this is primarily for simplicity, as the performance
   benefit is minimal.  */

struct inferior_regcache_data
{
  int registers_valid;
  unsigned char *registers;
  unsigned char *shadow; /* handle a register being dirty */
};

static int register_bytes;

static struct reg *reg_defs;
static int num_registers;

const char **gdbserver_expedite_regs;

static struct inferior_regcache_data *
get_regcache (struct thread_info *inf, int fetch)
{
  struct inferior_regcache_data *regcache;

  regcache = (struct inferior_regcache_data *) inferior_regcache_data (inf);

  if (regcache == NULL)
    fatal ("no register cache");

  /* FIXME - fetch registers for INF */
  if (fetch && regcache->registers_valid == 0)
    {
      fetch_inferior_registers (0);
      regcache->registers_valid = 1;
    }

  return regcache;
}

void
regcache_invalidate_one (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct inferior_regcache_data *regcache;

  regcache = (struct inferior_regcache_data *) inferior_regcache_data (thread);

  if (regcache->registers_valid)
    {
      struct thread_info *saved_inferior = current_inferior;

      current_inferior = thread;
      store_inferior_registers (-1);
      current_inferior = saved_inferior;
    }

  regcache->registers_valid = 0;
}

void
regcache_invalidate ()
{
  for_each_inferior (&all_threads, regcache_invalidate_one);
}

int
registers_length (void)
{
  return 2 * register_bytes;
}

void *
new_register_cache (void)
{
  struct inferior_regcache_data *regcache;

  regcache = malloc (sizeof (*regcache));

  /* Make sure to zero-initialize the register cache when it is created,
     in case there are registers the target never fetches.  This way they'll
     read as zero instead of garbage.  */
  regcache->registers = calloc (1, register_bytes);
  regcache->shadow = calloc (1, register_bytes);
  if (regcache->registers == NULL || regcache->shadow == NULL)
    fatal ("Could not allocate register cache.");
  
  regcache->registers_valid = 0;

  return regcache;
}

void
free_register_cache (void *regcache_p)
{
  struct inferior_regcache_data *regcache
    = (struct inferior_regcache_data *) regcache_p;

  free (regcache->shadow);
  free (regcache->registers);
  free (regcache);
}

void
set_register_cache (struct reg *regs, int n)
{
  int offset, i;
  
  reg_defs = regs;
  num_registers = n;

  offset = 0;
  for (i = 0; i < n; i++)
    {
      regs[i].offset = offset;
      offset += regs[i].size;
    }

  register_bytes = offset / 8;
}

static int
register_dirty (int n)
{
  struct inferior_regcache_data *regcache = get_regcache (current_inferior, 0);
  return memcmp (regcache->registers + (reg_defs[n].offset / 8),
                 regcache->shadow + (reg_defs[n].offset / 8),
                 register_size (n)) != 0;
}

static unsigned char *
register_data (int n, int fetch)
{
  unsigned char *registers
    = get_regcache (current_inferior, fetch)->registers;

  return registers + (reg_defs[n].offset / 8);
}

static unsigned char *
register_shadow_data (int n)
{
  unsigned char *registers
    = get_regcache (current_inferior, 0)->shadow;

  return registers + (reg_defs[n].offset / 8);
}

/* Sending to GDB */
void
registers_to_string (char *buf)
{
  unsigned char *registers = get_regcache (current_inferior, 1)->registers;
  int i;
  char *bufptr = buf;
  unsigned char *regptr = registers;

  for (i = 0; i < num_registers; i++)
    {
      if ((reg_defs[i].flags & REG_NOT_ACCESSABLE) == 0)
        {
          if ((reg_defs[i].flags & REG_NON_CACHEABLE) && 
              ((reg_defs[i].flags & REG_WRITE_ONLY) == 0))
            {
              if (remote_debug)
                printf_filtered ("m68k-bdm: adding register %d to response\n", i);
              fetch_inferior_registers (i);
            }
        }
      convert_int_to_ascii(regptr, bufptr, reg_defs[i].size / 8);
      /* Increment bufptr by asciified size (2 chars per byte) */
      bufptr += (reg_defs[i].size / 8) * 2;
      /* Increment regptr by size of reg, defined in bits */
      regptr += reg_defs[i].size / 8;
    }
  if (remote_debug)
    printf_filtered ("m68k-bdm: built register string response: %s\n", buf);
}

/* Received from GDB */
void
registers_from_string (char *buf)
{
  int len = strlen (buf);
  unsigned char *registers = get_regcache (current_inferior, 1)->registers;
  int i;
  
  if (len != register_bytes * 2)
    {
      warning ("Wrong sized register packet (expected %d bytes, got %d)",
               2*register_bytes, len);
      if (len > register_bytes * 2)
        len = register_bytes * 2;
    }

  convert_ascii_to_int (buf, registers, len / 2);  

  for (i = 0; i < num_registers; i++)
    if (register_dirty (i) &&
        (reg_defs[i].flags & REG_NON_CACHEABLE) &&
        ((reg_defs[i].flags & (REG_NOT_ACCESSABLE | REG_READ_ONLY)) == 0))
        store_inferior_registers (i);
}

struct reg *
find_register_by_name (const char *name)
{
  int i;

  for (i = 0; i < num_registers; i++)
    if (!strcmp (name, reg_defs[i].name))
      return &reg_defs[i];
  fatal ("Unknown register %s requested", name);
  return 0;
}

int
find_regno (const char *name)
{
  int i;

  for (i = 0; i < num_registers; i++)
    if (!strcmp (name, reg_defs[i].name))
      return i;
  fatal ("Unknown register %s requested", name);
  return -1;
}

struct reg *
find_register_by_number (int n)
{
  return &reg_defs[n];
}

int
register_size (int n)
{
  return reg_defs[n].size / 8;
}

void
supply_register (int n, const void *buf)
{
  memcpy (register_data (n, 0), buf, register_size (n));
  /* backend places the register into the cache so it is not dirty */
  memcpy (register_shadow_data (n), buf, register_size (n));
}

void
supply_register_by_name (const char *name, const void *buf)
{
  supply_register (find_regno (name), buf);
}

int
collect_register (int n, void *buf)
{
  int dirty = register_dirty (n);
  memcpy (buf, register_data (n, 1), register_size (n));
  /* backend takes the register and places it into the inferior so it is not dirty */
  memcpy (register_shadow_data (n), register_data (n, 0), register_size (n));
  return dirty;
}

void
collect_register_as_string (int n, char *buf)
{
  convert_int_to_ascii (register_data (n, 1), buf, register_size (n));
}

void
collect_register_by_name (const char *name, void *buf)
{
  collect_register (find_regno (name), buf);
}
