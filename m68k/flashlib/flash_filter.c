/* $Id: flash_filter.c,v 1.7 2008/09/09 11:48:50 cjohns Exp $
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

#include "flash29.h"
#include "flashcfm.h"
#include "flashintelc3.h"
#include "flash_filter.h"

#if HOST_FLASHING
# include <errno.h>
# include <inttypes.h>
# include <stdint.h>
# include <stdio.h>
# include <limits.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/types.h> 
# include <sys/stat.h> 
# include <unistd.h>
# include <elf-utils.h>
# include <BDMlib.h>
#endif

/* Interface to flash modules.
 */
typedef struct
{
  void (*init) (int);
  int struct_size;
  char *driver_magic;
  int (*download_struct) (void *, uint32_t);
  uint32_t (*search_chip) (void *, char *, uint32_t);
  void (*erase) (void *, int32_t);
  int (*blank_chk) (void *, int32_t);
  int (*erase_wait) (void *);
  uint32_t (*prog) (void *, uint32_t, unsigned char *, uint32_t);
  char *(*prog_entry) (void);
  unsigned char *p_code;
  uint32_t p_entry;
  uint32_t p_len;
  uint32_t ram;
  uint32_t len;
} alg_t;

/* Known flashing algorithms
 */
alg_t algorithm[] = {
  {init_flash29},
  {init_flashcfm},
  {init_flashintelc3},
};

/* Description of memory areas.
 */
typedef struct area_s
{
  uint32_t adr;
  uint32_t end;                         /* last byte inclusive */
  alg_t *alg;
  void *chip_descriptor;
  struct area_s *next;
} area_t;

/* Memory areas. Assume RAM for unregistered areas.
 */
static area_t ram_area = { 0, ~0, NULL, NULL, NULL, };
static area_t *area = NULL;

/* Maximum size of a chip_descriptor. Since the chip-descriptor is algorithm's
   internal information, we don't know the size of the descriptor until we
   know which algorithm to use. Therefore we need to allocate a maximum size
   for autodetection.
 */
static int maxsiz = 0;

/* Variables.
 */
typedef struct variable_s
{
  struct variable_s *next;
  const char *name;
  uint32_t value;
} variable_t;

static variable_t *s_variables = 0;

/* Initialize all known algorithms, determine maximum size of chip descriptors
   and register RAM for whole address range.
 */
static void
init (void)
{
  int i;

  if (area)
    return;                             /* Already initialized */

  for (i = 0; i < NUMOF(algorithm); i++) {
    algorithm[i].p_code = 0;
    algorithm[i].init(i);
    if (maxsiz < algorithm[i].struct_size)
      maxsiz = algorithm[i].struct_size;
  }

  area = &ram_area;  /* register RAM fallback for whole address range */
}

/* This is the callback for the init functions of the basic flash modules.
   Each flash module shall register itself with this function.
   Doing it this way keeps algorithm's private data private and minimizes
   pollution of namespace.
 */
void
register_algorithm(int num,
                   char *driver_magic,
                   int struct_size,
                   int (*download_struct) (void *, uint32_t),
                   uint32_t (*search_chip) (void *, char *, uint32_t),
                   void (*erase) (void *, int32_t),
                   int (*blank_chk) (void *, int32_t),
                   int (*erase_wait) (void *),
                   uint32_t (*prog) (void *, uint32_t, unsigned char *,
                                     uint32_t), char *(*prog_entry) (void))
{
  algorithm[num].driver_magic = strdup(driver_magic);   /* FIXME */
  algorithm[num].struct_size = struct_size;
  algorithm[num].download_struct = download_struct;
  algorithm[num].search_chip = search_chip;
  algorithm[num].erase = erase;
  algorithm[num].blank_chk = blank_chk;
  algorithm[num].erase_wait = erase_wait;
  algorithm[num].prog = prog;
  algorithm[num].prog_entry = prog_entry;

  algorithm[num].p_code = NULL;
  algorithm[num].p_entry = 0;
  algorithm[num].p_len = 0;
  algorithm[num].ram = 0;
  algorithm[num].len = 0;
}

/* Search memory area where ADR belongs to.
 */
static area_t *
search_area (adr)
{
  area_t *a;

  for (a = area; a; a = a->next)
    if (a->adr <= adr && a->end >= adr)
      return a;

  return NULL;
}

#if HOST_FLASHING

/* register target flash plugins. The adr/len defines a RAM area on the
   target that can be used to download plugin and contents.
 */
int
elf_flash_plugin_load(int (*prfunc) (const char *format, ...),
                      uint32_t adr, uint32_t len, const char *fname)
{
  struct stat sb;
  elf_handle handle;
  const char *dmagic;
  const char* prefix = PREFIX;
  static const char suffix[] = "share/m68k-bdm/plugins/";
  char* name = (char*) fname;
  int i;

  elf_handle_init(&handle);

  if (name[0] != '/')
  {
    if (stat (name, &sb) < 0)
    {
      if (errno == ENOENT)
      {
	int prefixlen = strlen(prefix);
	int needsep = prefix[prefixlen - 1] != '/';
	int pathlen = prefixlen + needsep + sizeof suffix + strlen(fname);

	name = malloc(pathlen);
	sprintf(name, "%s%s%s%s", prefix, needsep ? "/" : "", suffix, fname);

        if (stat (name, &sb) < 0)
        {
          if (errno == ENOENT)
          {
            prfunc ("cannot find plugin: %s\n", fname);
            prfunc ("searched:\n  .\n  %s/%s\n", PREFIX, suffix);
            return 0;
          }
        }
      }
    }
  }

  if (!elf_open (name, &handle, prfunc))
  {
    if (prfunc)
      prfunc ("open %s failed", name);
    return 0;
  }

  dmagic = elf_get_section_data_sym(&handle, "driver_magic");

  if (!dmagic)
  {
    if (prfunc)
      prfunc ("no 'driver_magic' symbol found");
    elf_close (&handle);
    return 0;
  }

  for (i = 0; i < NUMOF(algorithm); i++)
  {
    uint32_t value;
    GElf_Sym entrysym;
    GElf_Shdr shdr;
    void *data;
    uint32_t size;

    if (!STREQ(dmagic, algorithm[i].driver_magic))
      continue;

    if (!elf_get_symbol(&handle, algorithm[i].prog_entry(), &entrysym))
    {
      if (prfunc)
        prfunc("could not find prog entry symbol %s",
               algorithm[i].prog_entry());
      elf_close(&handle);
      return 0;
    }

    if (!elf_get_section_hdr(&handle, entrysym.st_shndx, &shdr))
    {
      if (prfunc)
        prfunc("could not get section header for prog entry");
      elf_close(&handle);
      return 0;
    }

    data = elf_get_section_data(&handle, entrysym.st_shndx, &size);
    if (!data)
    {
      if (prfunc)
        prfunc("could not get section data for prog entry");
      elf_close(&handle);
      return 0;
    }

    algorithm[i].p_entry = entrysym.st_value;
    algorithm[i].p_len = size;
    algorithm[i].p_code = malloc(size);
    memcpy(algorithm[i].p_code, data, size);

    algorithm[i].ram = adr;
    algorithm[i].len = len;
    break;
  }

  if (prfunc)
  {
    if (i < NUMOF(algorithm))
      prfunc("%s loaded, size:%d", dmagic, algorithm[i].p_len);
    else
      prfunc("no algorithm %s found", dmagic);
  }

  elf_close(&handle);
  return 1;
}

int
srec_flash_plugin_load(int (*prfunc) (const char *format, ...),
                       uint32_t adr, uint32_t len, const char *fname)
{
  return 0;
}

int
flash_plugin(int (*prfunc) (const char *format, ...),
             uint32_t adr, uint32_t len, char *argv[])
{
  int cnt;

  init();

  for (cnt = 0; argv[cnt]; cnt++)
  {
    if (!elf_flash_plugin_load(prfunc, adr, len, argv[cnt]))
      if (!srec_flash_plugin_load(prfunc, adr, len, argv[cnt]))
        if (prfunc)
          prfunc("no suitable loader found: %s", argv[cnt]);
  }
  return 0;
}

#endif

static int
prog_clone(area_t * area, uint32_t adr, unsigned char *data, uint32_t size)
{
#if HOST_FLASHING
  char driver_magic[1024];              /* FIXME: should be dynamically */
  uint32_t mem;
  uint32_t len;
  static uint32_t entry = 0;
  static uint32_t content = 0;
  static area_t *last_area = NULL;
  uint32_t sp, ra;
  uint32_t sent = 0;
  uint32_t num;
  unsigned long wrote_num = 0;
  int cpu_type;

  if (!area->alg)
    return 0;
  if (bdmGetProcessor(&cpu_type) < 0)
    return 0;

  if (!area->alg->p_code) {
    /* No plugin loaded -> use host-only mode. */
    return area->alg->prog (area->chip_descriptor, adr, data, size);
  }

  mem = area->alg->ram;
  len = area->alg->len;

  if (area != last_area) {
    /* Download plugin and chip descriptor into target. */
    entry = area->alg->download_struct (area->chip_descriptor, mem);
    bdmWriteMemory (entry, area->alg->p_code, area->alg->p_len);
    content = entry + area->alg->p_len;
    last_area = area;
  }

  while (sent < size) {
    sp = mem + len;

    num = sp - 0x200 - content; /* FIXME: stack size should be
                                   determined dynamically */
    if (num > size - sent)
      num = size - sent;

    /* Set up stack frame. 
     * Be careful with the longword writes, they must be longword aligned! 
     */
    switch (cpu_type) {
      case BDM_CPU32:
        sp -= 4;
        ra = sp;
        bdmWriteWord (sp, 0x4afa);       /* BGND instruction */
        break;
      case BDM_COLDFIRE:
        sp -= 4;
        ra = sp;
        bdmWriteWord (sp, 0x4ac8);       /* HALT instruction */
        break;
      default:
        return 0;
    }

    sp -= 4;
    bdmWriteLongWord (sp, num);          /* amount of data to flash */
    sp -= 4;
    bdmWriteLongWord (sp, content);      /* adr of memory contents */
    sp -= 4;
    bdmWriteLongWord (sp, adr);          /* destination adr */
    sp -= 4;
    bdmWriteLongWord (sp, mem);          /* chip descriptor */
    sp -= 4;
    bdmWriteLongWord (sp, ra);           /* return address to BGND */

    /* Download contents. */
    bdmWriteMemory (content, data, num);

    /* Set stack pointer and program counter. */
    bdmWriteRegister(BDM_REG_A7, sp);
    bdmWriteSystemRegister(BDM_REG_RPC, entry + area->alg->p_entry);

    /* Execute */
    bdmGo();
    while (!((bdmStatus ()) & (BDM_TARGETSTOPPED | BDM_TARGETHALT))) ;
    
    bdmReadRegister (BDM_REG_D0, &wrote_num);

    if (num != (uint32_t) wrote_num) {
      printf ("Returned 0x%08" PRIxMAX "\n", wrote_num);   
      break;                            /* write failed */
    }
    
    sent += num;
    data += num;
    adr += num;
  }
#else
  /* FIXME: to be done */
#endif

  return sent;
}

/* Register a new memory region. The new region should always overlap with
   the RAM fallback (which is registered by the init function). The overlap
   is resolved by shrinking/splitting the RAM fallback area. Returns 1 on
   success, 0 otherwise.
 */
int
flash_register(char *description, uint32_t adr, char *hint_driver)
{
  int i;
  uint32_t size;
  area_t *bold, *new, *aold;            /* order is _b_efore, new, _a_fter */
  void *chip;

  init();
  if (!(chip = malloc (maxsiz)))
    return 0;

  /* Search area where the address belongs to. The result should be the RAM
     fallback. */
  bold = search_area(adr);

  for (i = 0; i < NUMOF(algorithm); i++) {
    if (hint_driver != NULL) {
      if (!STREQ(hint_driver, algorithm[i].driver_magic))
        continue;
    }

    if ((size = algorithm[i].search_chip(chip, description, adr))) {
      if (bold->end > adr + (size - 1)) {
        /* End of old area is bigger than end of new area -> add new area of
           old style behind new area. */
        if (!(aold = malloc(sizeof(*aold))))
          break;
        *aold = *bold;
        aold->adr = adr + size;
        aold->next = bold->next;
        bold->next = aold;
      }
      if (bold->adr < adr) {
        /* start of old area is less than start of new area -> add new area
           of old style in front of new area. */
        if (!(new = malloc(sizeof(*new))))
          return 0;
        new->next = bold->next;         /* insert new area into chain */
        bold->next = new;
        bold->end = adr - 1;            /* shrink old area */
      } else {
        new = bold;                     /* same start address -> just replace old area */
      }

      /* fill new area with contents */
      new->adr = adr;
      new->end = adr + (size - 1);
      new->alg = &algorithm[i];
      new->chip_descriptor = chip;

      return 1;
    }
  }

  free(chip);
  return 0;
}

/* Search area of address and call its erase algorithm.
 */
int
flash_erase(uint32_t adr, int32_t sector_offset)
{
  area_t *a;
  alg_t *alg;

  if (!(a = search_area(adr)))
    return 0;

  if (alg = a->alg) {
    if (alg->erase) {
      alg->erase(a->chip_descriptor, sector_offset);
      return 1;
    }
  }

  return 0;
}

/* Search area of address and call its blank check algorithm.
 */
int
flash_blank_chk(uint32_t adr, int32_t sector_offset)
{
  area_t *a;
  alg_t *alg;

  if (!(a = search_area(adr)))
    return 0;

  if (alg = a->alg) {
    if (alg->blank_chk) {
      return alg->blank_chk(a->chip_descriptor, sector_offset);
    }
  }

  return 0;
}

/* Search area of address and call its erase_wait algorithm.
 */
int
flash_erase_wait(uint32_t adr)
{
  area_t *a;
  alg_t *alg;

  if (!(a = search_area(adr)))
    return 0;

  if (alg = a->alg) {
    if (alg->erase_wait) {
      return alg->erase_wait(a->chip_descriptor);
    }
  }

  return 0;
}

/* Write to memory through registered algorithms.
 */
uint32_t
write_memory(uint32_t adr, unsigned char *data, uint32_t cnt)
{
  uint32_t ret;
  uint32_t wrote = 0;

  init();

  while (wrote < cnt) {
    area_t *area = search_area(adr + wrote);

    /* That much fits into this area. */
    uint32_t size = (area->end - (adr + wrote)) + 1;

    /* Limit size if it is more than we actually want to write. */
    if (size > cnt - wrote)
      size = cnt - wrote;

    if (area->alg && area->alg->prog) {
      ret = prog_clone(area, adr, data, size);
      wrote += ret;
      if (ret != size)
        return wrote;
    } else {
      /* no programming algorithm defined, assume RAM */
#if HOST_FLASHING
      if (bdmWriteMemory(adr, data, size) < 0)
        return wrote;
#else
      memcpy((unsigned char *) adr, data, size);
#endif
      wrote += size;
    }
  }
  return wrote;
}

int 
flash_set_var(const char *name, uint32_t value)
{
  /* find and update existing var */
  variable_t *var = s_variables;
  while(var) {
    if(strcmp(var->name, name) == 0) {
      var->value = value;
      return 1;
    }
    var = var->next;
  }

  /* add a new var */
  var = (variable_t *) malloc(sizeof(variable_t));
  if(!var)
    return 0;
  var->name = strdup(name);
  if(!var->name)
    return 0;
  var->value = value;
  var->next = s_variables;
  s_variables = var;
  
  return 1;
}

int 
flash_get_var(const char *name, uint32_t *value, uint32_t value_default)
{
  variable_t *var = s_variables;
  while(var) {
    if(strcmp(var->name, name) == 0) {
      *value = var->value;
      return 1;
    }
    var = var->next;
  }
  *value = value_default;
  return 0;
}

int 
flash_spin(int c)
{
#if HOST_FLASHING
  const char spin[] = {'|', '/', '-', '\\'};
  int n = c / 100;
  printf("\b%c", spin[n % 4]);
  fflush(stdout);  
#endif
  return ++c;
}
