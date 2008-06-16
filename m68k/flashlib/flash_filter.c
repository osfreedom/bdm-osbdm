/* $Id: flash_filter.c,v 1.5 2008/06/16 00:01:21 cjohns Exp $
 *
 * Flash filtering layer.
 *
 * 2003-12-28 Josef Wolf (jw@raven.inka.de)
 * 2008 Chris Johns (cjohns@users.sourceforge.net)
 *
 * Portions of this program which I authored may be used for any purpose
 * so long as this notice is left intact. 
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
 */

/* This code was inspired by the filtering code from Pavel Pisa. But it was
   written from scratch to achieve stronger data encapsulation and enable
   target-mode plugins. For the long run, it should be extended for
   target-only operation mode.

   CCJ : Remove BFD support and replaced with libelf.
 */

# include "flash29.h"
# include "flashcfm.h"
# include "flash_filter.h"

# if HOST_FLASHING
#  include <stdint.h>
#  include <stdio.h>
#  include <limits.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <elf-utils.h>
#  include <BDMlib.h>
# endif

/* Interface to flash modules.
 */
typedef struct {
  void (*init) (int);
  int struct_size;
  char *driver_magic;
  int (*download_struct) (void *, unsigned long);
  int (*search_chip) (void *, char *, unsigned long);
  void (*erase) (void *, long);
  int (*blank_chk) (void *, long);
  int (*erase_wait) (void *);
  int (*prog) (void *, unsigned long, unsigned char *, unsigned long);
  char *(*prog_entry) (void);
  unsigned char *p_code;
  unsigned long p_entry;
  unsigned long p_len;
  unsigned long ram;
  unsigned long len;
} alg_t;

/* Known flashing algorithms
 */
alg_t algorithm[] = {
  { init_flash29 },
  { init_flashcfm },
};

/* Description of memory areas.
 */
typedef struct area_s {
  unsigned long adr;
  unsigned long size;
  alg_t *alg;
  void *chip_descriptor;
  struct area_s *next;
} area_t;

/* Memory areas. Assume RAM for unregistered areas.
 */
static area_t ram_area = { 0, ULONG_MAX, NULL, NULL, NULL, };
static area_t *area = NULL;

/* Maximum size of a chip_descriptor. Since the chip-descriptor is algorithm's
   internal information, we don't know the size of the descriptor until we
   know which algorithm to use. Therefore we need to allocate a maximum size
   for autodetection.
 */
static int maxsiz=0;

/* Initialize all known algorithms, determine maximum size of chip descriptors
   and register RAM for whole address range.
 */
static void init (void)
{
  int i;

  for (i=0; i<NUMOF(algorithm); i++) {
    if (algorithm[i].p_code)
      free(algorithm[i].p_code);
    algorithm[i].ram = 0;
    algorithm[i].len = 0;
  }
  
  if (area) return; /* Already initialized */

  for (i=0; i<NUMOF(algorithm); i++) {
    algorithm[i].init(i);
    if (maxsiz < algorithm[i].struct_size)
      maxsiz = algorithm[i].struct_size;
  }
    
  area = &ram_area; /* register RAM fallback for whole address range */
}

/* This is the callback for the init functions of the basic flash modules.
   Each flash module shall register itself with this function.
   Doing it this way keeps algorithm's private data private and minimizes
   pollution of namespace.
 */
void register_algorithm (
  int num,
  char *driver_magic,
  int struct_size,
  int (*download_struct) (void *, unsigned long),
  int (*search_chip) (void *, char *, unsigned long),
  void (*erase) (void *, long),
  int (*blank_chk) (void *, long),
  int (*erase_wait) (void *),
  int (*prog) (void *, unsigned long, unsigned char *, unsigned long),
  char *(*prog_entry)(void)
                         )
{
  algorithm[num].driver_magic     = strdup (driver_magic); /* FIXME */
  algorithm[num].struct_size      = struct_size;
  algorithm[num].download_struct  = download_struct;
  algorithm[num].search_chip      = search_chip;
  algorithm[num].erase            = erase;
  algorithm[num].blank_chk        = blank_chk;
  algorithm[num].erase_wait       = erase_wait;
  algorithm[num].prog             = prog;
  algorithm[num].prog_entry       = prog_entry;
}

/* Search memory area where ADR belongs to.
 */
static area_t *search_area (adr)
{
  area_t *a;

  for (a=area; a; a=a->next)
    if (a->adr<=adr && a->adr+a->size>adr) return a;

  return NULL;
}

/* FIXME: This is a quick-n-dirty ad-hoc to get a start. In the long run,
   we need to delegate memory allocation to the caller because we have no
   guarrantee that dynamic memory is available in target-only operation mode.
 */
static void *mymalloc (int size)
{
  return malloc (size);
}

# if HOST_FLASHING

/* register target flash plugins. The adr/len defines a RAM area on the
   target that can be used to download plugin and contents.
 */
int elf_flash_plugin_load (int (*prfunc) (const char *format, ...),
                           unsigned long adr, unsigned long len,
                           const char* fname)
{
  elf_handle  handle;
  const char* dmagic;
  int         i;
  
  elf_handle_init (&handle);

  if (!elf_open (fname, &handle, prfunc))
  {
    prfunc ("open %s failed", fname);
    return 0;
  }

  dmagic = elf_get_string (&handle, "driver_magic");

  if (!dmagic)
  {
    prfunc ("no 'driver_magic' symbol found");
    elf_close (&handle);
    return 0;
  }

	for (i = 0; i < NUMOF (algorithm); i++)
  {
    uint32_t value;
    
    if (!STREQ (dmagic, algorithm[i].driver_magic))
      continue;

    if (!elf_get_symbol_value (&handle,
                               algorithm[i].prog_entry (),
                               &value))
    {
      if (prfunc)
        prfunc ("%s: no entry point found: %s",
                dmagic, algorithm[i].prog_entry ());
      elf_close (&handle);
      return 0;
    }

    algorithm[i].p_entry = value;
    
    if (!elf_get_section_size (&handle, ".text", &value))
    {
      if (prfunc)
        prfunc ("%s: no .text section size found", dmagic);
      elf_close (&handle);
      return 0;
    }
    
    algorithm[i].p_len  = value;
    algorithm[i].p_code = elf_get_section_data (&handle, ".text");

    if (!algorithm[i].p_code)
    {
      if (prfunc)
        prfunc ("%s: could not load .text section", dmagic);
      elf_close (&handle);
      return 0;
    }
      
    algorithm[i].ram = adr;
    algorithm[i].len = len;
    break;
  }

  if (prfunc)
  {
    if (i < NUMOF (algorithm))
      prfunc ("%s loaded, size:%d", dmagic, algorithm[i].p_len);
    else
      prfunc ("no algorithm %s found", dmagic);
  }

  elf_close (&handle);
  
  return 1;
}

int srec_flash_plugin_load (int (*prfunc) (const char *format, ...),
                            unsigned long adr, unsigned long len,
                            const char* fname)
{
  return 0;
}

int flash_plugin (int (*prfunc) (const char *format, ...),
                  unsigned long adr, unsigned long len, char *argv[])
{
  int cnt;
  
  init();

  for (cnt = 0; argv[cnt]; cnt++)
  {
    if (!elf_flash_plugin_load (prfunc, adr, len, argv[cnt]))
      if (!srec_flash_plugin_load (prfunc, adr, len, argv[cnt]))
        if (prfunc)
          prfunc ("no suitable loader found: %s", argv[cnt]);
  }

}

# endif

static int prog_clone (area_t *area,
                       unsigned long adr, unsigned char *data,
                       unsigned long size)
{
# if HOST_FLASHING
  char driver_magic[1024]; /* FIXME: should be dynamically */
  unsigned long mem;
  unsigned long len;
  static unsigned long entry=0;
  static unsigned long content=0;
  static area_t *last_area=NULL;
  unsigned long sp, ra;
  unsigned long sent=0;
  unsigned long num;

  if (!area->alg) return 0;

  if (!area->alg->p_code) {
    /* No plugin loaded -> use host-only mode.
     */
    return area->alg->prog (area->chip_descriptor, adr, data, size);
  }

  mem = area->alg->ram;
  len = area->alg->len;

  if (area != last_area) {
    /* Download plugin and chip descriptor into target.
     */
    entry = area->alg->download_struct (area->chip_descriptor, mem);
    bdmWriteMemory (entry, area->alg->p_code, area->alg->p_len); /*FIXME*/
    content = entry + area->alg->p_len;
    last_area = area;
  }

  while (sent<size) {
    sp=mem+len;

    num = sp-0x200-content; /* FIXME: stack size should be determined
                               dynamically */
    if (num > size-sent)
      num = size-sent;

    /* Set up stack frame.
     *BE CARFEUL WITH THE LONGWORD WRITES, THEY MUST BE LONGWORD ALIGNED
     */
    //sp-=4; bdmWriteWord     (sp, 0x4afa);  /* BGND instruction */
    sp-=4; ra = sp; bdmWriteWord (sp, 0x4ac8);  /* HALT instruction */
    sp-=4; bdmWriteLongWord (sp, num);     /* amount of data to flash */
    sp-=4; bdmWriteLongWord (sp, content); /* adr of memory contents */
    sp-=4; bdmWriteLongWord (sp, adr);     /* destination adr */
    sp-=4; bdmWriteLongWord (sp, mem);     /* chip descriptor */
    sp-=4; bdmWriteLongWord (sp, ra);      /* return address to BGND */

    /* Download contents.
     */
    bdmWriteMemory (content, data, num);

    /* Set stack pointer and program counter.
     */
    bdmWriteRegister       (BDM_REG_A7,  sp);
    bdmWriteSystemRegister (BDM_REG_RPC, entry + area->alg->p_entry);

    /* Execute
     */
    bdmGo ();
    while (!((bdmStatus ()) & (BDM_TARGETSTOPPED|BDM_TARGETHALT)))
      ;
    bdmReadRegister (BDM_REG_D0, &num);

    sent += num;
    data += num;
    adr += num;

    if (!num) break; /* write failed */
  }
# else
  /* FIXME: to be done */
# endif

  return sent;
}

/* Register a new memory region. The new region should always overlap with
   the RAM fallback (which is registered by the init function). The overlap
   is resolved by shrinking/splitting the RAM fallback area. Returns 1 on
   success, 0 otherwise.
 */
int flash_register (char *description, unsigned long adr, char *hint_driver)
{
  int i;
  unsigned int size;
  area_t *bold, *new, *aold; /* order is _b_efore, new, _a_fter */
  void *chip;

  init();
  if (!(chip = mymalloc (maxsiz))) return 0;

  /* Search area where the address belongs to. The result should be the
     RAM fallback.
  */
  bold = search_area (adr);

  for (i=0; i<NUMOF(algorithm); i++) {
    if(hint_driver != NULL)
    {
      if(!STREQ(hint_driver, algorithm[i].driver_magic))
        continue;
    }
        
    if ((size = algorithm[i].search_chip (chip, description, adr))) {
      if (bold->adr+bold->size > adr+size) {
        /* End of old area is bigger than end of new area -> add new
           area of old style behind new area.
        */
        if (!(aold = mymalloc(sizeof (*aold))))
          break;
        *aold = *bold;
        aold->adr  = adr+size;
        aold->size = (bold->adr + bold->size) - (adr + size);
        aold->next = bold->next;
        bold->next = aold;
      }
      if (bold->adr < adr) {
        /* start of old area is less than start of new area -> add new
           area of old style in front of new area.
        */
        if (!(new = mymalloc (sizeof (*new))))
          break;
        new->next  = bold->next;      /* insert new area into chain */
        bold->next = new;
        bold->size = adr - bold->adr;  /* shrink old area */
      } else {
        new = bold; /* same start address -> just replace old area */
      }

      /* fill new area with contents
       */
      new->adr  = adr;
      new->size = size;
      new->alg  = &algorithm[i];
      new->chip_descriptor = chip;

      return 1;
    }
  }

  free (chip);
  return 0;
}

/* Search area of address and call its erase algorithm.
 */
int flash_erase (unsigned long adr, long sector_offset)
{
  area_t *a;
  alg_t *alg;

  if (!(a=search_area(adr))) return 0;

  if (alg = a->alg) {
    if (alg->erase) {
      alg->erase (a->chip_descriptor, sector_offset);
      return 1;
    }
  }

  return 0;
}

/* Search area of address and call its blank check algorithm.
 */
int flash_blank_chk (unsigned long adr, long sector_offset)
{
  area_t *a;
  alg_t *alg;

  if (!(a=search_area(adr))) return 0;

  if (alg = a->alg) {
    if (alg->blank_chk) {
      return alg->blank_chk (a->chip_descriptor, sector_offset);
    }
  }

  return 0;
}

/* Search area of address and call its erase_wait algorithm.
 */
int flash_erase_wait (unsigned long adr)
{
  area_t *a;
  alg_t *alg;

  if (!(a=search_area(adr))) return 0;

  if (alg = a->alg) {
    if (alg->erase_wait) {
      return alg->erase_wait (a->chip_descriptor);
    }
  }

  return 0;
}

/* Write to memory through registered algorithms.
 */
unsigned long write_memory (unsigned long adr, unsigned char *data,
                            unsigned long cnt)
{
  int ret;
  unsigned long wrote=0;

  init();

  while (wrote < cnt) {
    area_t *area = search_area (adr+wrote);

    /* That much fits into this area.
     */
    unsigned long size = area->size - ((adr+wrote) - area->adr);

    /* Limit size if it is more than we actually want to write.
     */
    if (size > cnt-wrote) size = cnt-wrote;

    if (area->alg && area->alg->prog) {
      ret = prog_clone (area, adr, data, size);
      wrote += ret;
      if (ret != size) return wrote;
    } else {
      /* no programming algorithm defined, assume RAM
       */
# if HOST_FLASHING
      if (bdmWriteMemory (adr, data, size) < 0)
        return wrote;
# else
      memcpy ((unsigned char *)adr, data, size);
# endif
      wrote += size;
    }
  }
  return wrote;
}
