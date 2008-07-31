/*
 * $Id: elf-utils.c,v 1.2 2008/07/31 01:53:44 cjohns Exp $
 * 
 * Motorola Background Debug Mode Driver
 * Copyright (C) 2008  Chris Johns
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
 *
 * ELF Support for libelf.
 */

#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <elf-utils.h>

#if defined (__WIN32__)
#define ELF_OPEN_MODE (O_RDONLY | O_BINARY)
#else
#define ELF_OPEN_MODE (O_RDONLY)
#endif

void
elf_handle_init (elf_handle* handle)
{
  memset (handle, 0, sizeof (*handle));
}

int
elf_open (const char* file, elf_handle* handle, elf_output output)
{
  if (handle->fd)
  {
    if (output)
      output ("elf-utils: ELF handle already initialised\n");
    return 0;
  }

  if (elf_version (EV_CURRENT) == EV_NONE)
  {
    if (output)
      output ("elf-utils: ELF library initialization failed: %s\n",
              elf_errmsg (-1));
    return 0;
  }

  handle->output = output;

  if ((handle->fd = open (file, ELF_OPEN_MODE, 0)) < 0)
  {
    if (handle->output)
      output ("elf-utils: open %s failed\n", file);
    return 0;
  }

  if ((handle->elf = elf_begin (handle->fd, ELF_C_READ, NULL)) == NULL)
  {
    close (handle->fd);
    handle->fd = 0;
    if (handle->output)
      output ("elf-utils: elf_begin failed: %s\n", elf_errmsg (-1));
    return 0;
  }

  handle->file = strdup (file);
  handle->ek = elf_kind (handle->elf);

  /* 
   * Check the executable header.
   */
  if (gelf_getehdr (handle->elf, &handle->ehdr) == NULL)
  {
    if (handle->output)
      output ("elf-utils: elf_getehdr failed: %s\n", elf_errmsg (-1));
    elf_close (handle);
    return 0;
  }

  if (handle->ehdr.e_machine != EM_68K)
  {
    if (handle->output)
      output ("elf-utils: machine type not Motorola 68000\n");
    elf_close (handle);
    return 0;
  }

  if ((handle->ehdr.e_type != ET_REL) && (handle->ehdr.e_type != ET_EXEC))
  {
    output ("elf-utils: file type no relocable or executable\n");
    elf_close (handle);
    return 0;
  }

  if ((handle->eclass = gelf_getclass (handle->elf)) == ELFCLASSNONE)
  {
    if (handle->output)
      output ("elf-utils: elf_getclass failed: %s\n", elf_errmsg (-1));
    elf_close (handle);
    return 0;
  }

  /* 
   * No 64-bit Coldfires yet !
   */
  if (handle->eclass != ELFCLASS32)
  {
    if (handle->output)
      output ("elf-utils: ELF 64-bit, only 32-bit support: %s\n",
              handle->file);
    elf_close (handle);
    return 0;
  }

  if (elf_getshnum (handle->elf, &handle->shnum) == 0)
  {
    if (handle->output)
      output ("elf-utils: elf_getshnum failed: %s\n", elf_errmsg (-1));
    elf_close (handle);
    return 0;
  }

  if (elf_getshstrndx (handle->elf, &handle->shstrndx) == 0)
  {
    if (handle->output)
      output ("elf-utils: elf_getshstrndx failed: %s\n", elf_errmsg (-1));
    elf_close (handle);
    return 0;
  }

  if (elf_getphnum (handle->elf, &handle->phnum) == 0)
  {
    if (handle->output)
      output ("elf-utils: elf_getphnum failed: %s\n", elf_errmsg (-1));
    elf_close (handle);
    return 0;
  }

  return 1;
}

int
elf_close (elf_handle* handle)
{
  if (!handle->fd)
    return 0;

  elf_end (handle->elf);
  close (handle->fd);
  free (handle->file);

  handle->elf = NULL;
  handle->file = NULL;
  handle->fd = 0;

  return 1;
}

int
elf_has_symbol_table (elf_handle* handle)
{
  Elf_Scn* section = NULL;
  while ((section = elf_nextscn (handle->elf, section)) != 0)
  {
    GElf_Shdr shdr;
    if (gelf_getshdr (section, &shdr) != &shdr)
    {
      if (shdr.sh_type == SHT_SYMTAB)
      {
        Elf_Data* data = NULL;
        
        data = elf_getdata (section, data);

        if ((data == NULL) || (data->d_size == 0))
          return 0;

        return 1;
      }
    }
  }
  return 0;
}

int
elf_get_symbol (elf_handle* handle, const char* label, GElf_Sym* sym)
{
  Elf_Scn* section = NULL;
  while ((section = elf_nextscn (handle->elf, section)) != 0)
  {
    GElf_Shdr shdr;

    if (gelf_getshdr (section, &shdr) == &shdr)
    {
      if (shdr.sh_type == SHT_SYMTAB)
      {
        Elf_Data* data = NULL;
        int       symbol = 0;
        char*     name;
        
        data = elf_getdata (section, data);

        if ((data == NULL) || (data->d_size == 0))
          return 0;

        while (gelf_getsym (data, symbol, sym) == sym)
        {
          name = elf_strptr (handle->elf,
                             shdr.sh_link, (size_t) sym->st_name);
          if (!name)
          {
            if (handle->output)
              handle->output ("elf-utils: find symbol: %s\n",
                              elf_errmsg (elf_errno ()));
            return 0;
          }

          if (strcmp (label, name) == 0)
            return 1;

          symbol++;
        }
      }
    }
  }

  return 0;
}

int
elf_get_section_hdr (elf_handle* handle, int secindex, GElf_Shdr* shdr)
{
  Elf_Scn* section = NULL;
  int      count = 1;

  while ((section = elf_nextscn (handle->elf, section)) != 0)
  {
    if (count == secindex)
    {
      if (gelf_getshdr (section, shdr) == shdr)
      {
        return 1;
      }
    }
    count++;
  }
  return 0;
}

void*
elf_get_section_data (elf_handle* handle, int secindex, uint32_t* size)
{
  Elf_Scn* section = NULL;
  int      count = 1;

  *size = 0;
  while ((section = elf_nextscn (handle->elf, section)) != 0)
  {
    if (count == secindex)
    {
      Elf_Data* data = NULL;
      void*     buffer;

      data = elf_getdata (section, data);

      if (data == NULL)
        return NULL;

      *size = (uint32_t) data->d_size;
      return data->d_buf;
    }
    count++;
  }
  return 0;
}

void*
elf_get_section_data_sym (elf_handle* handle, const char* label)
{
  GElf_Sym esym;

  if (elf_get_symbol (handle, label, &esym))
  {
    Elf_Scn* section = NULL;
    int      count = 1;

    while ((section = elf_nextscn (handle->elf, section)) != 0)
    {
      if (count == esym.st_shndx)
      {
        Elf_Data* data = NULL;

        data = elf_getdata (section, data);

        if ((data == NULL) || (data->d_size == 0))
          return NULL;

        return ((uint8_t *) data->d_buf) + esym.st_value;
      }
      count++;
    }
  }

  return NULL;
}

int 
elf_map_over_sections (elf_handle* handle, elf_section_handler handler,
                       const char* sname)
{
  /*
   * We need to iterate over the sections, and then for each section we decide
   * to load, we need to find the program header for that section.  we match
   * the program header by finding the program header that has the same virtaul
   * address range.  This yields us with the physical address which is needed
   * to know where to load the section on the target.  I don't know if this is
   * the correct way to do this, if not, please fix.
   */
  
  Elf_Scn* section = NULL;
  int      count = 1;
  size_t   num_headers = 0;

  if (elf_getphnum (handle->elf, &num_headers) == 0)
  {
    if (handle->output)
      handle->output ("elf-utils: elf_getphnum error\n");
    return 0;
  }
  
  while ((section = elf_nextscn (handle->elf, section)) != 0)
  {
    GElf_Shdr shdr;

    if (gelf_getshdr (section, &shdr) == &shdr) {
      const char* name;
      name = elf_strptr (handle->elf, handle->shstrndx, shdr.sh_name);

      if (!sname || (strcmp (sname, name) == 0))
      {
        /*
         * Find a LOAD program header in the same virtual address range.
         */
        GElf_Addr vaddr = shdr.sh_addr - shdr.sh_offset;
        GElf_Phdr phdr;
        size_t i;
        
        for (i = 0; i < num_headers; ++i)
        {
          if (gelf_getphdr (handle->elf, i, &phdr) != &phdr)
          {
            if (handle->output)
              handle->output ("elf-utils: gelf_getphdr error\n");
            return 0;
          }
          
          if(phdr.p_type == PT_LOAD)
          {
            if(phdr.p_vaddr <= shdr.sh_addr && 
               phdr.p_vaddr + phdr.p_memsz > shdr.sh_addr)
              break;
          }
        }

        if(!handler (handle, (i == num_headers) ? 0 : &phdr, &shdr, name, count))
          return 0;
      }
    }

    ++count;
  }
  return 1;    
}

#define PRINT_FORMAT   " %-12s %d\n"
#define PRINT_FORMAT_X " %-12s 0x%x\n"

void
elf_show_exeheader (elf_handle* handle)
{
#define EHDR_PRINT_FIELD(N) do {                                    \
    handle->output (PRINT_FORMAT ,#N, (uintmax_t) handle->ehdr.N);  \
  } while (0)

  char* s;

  if (!handle->output)
    return;

  switch (handle->ek)
  {
    case ELF_K_AR:
      s = "ar(1) archive";
      break;
    case ELF_K_ELF:
      s = "elf object";
      break;
    case ELF_K_NONE:
      s = "data";
      break;
    default:
      s = "unrecognized";
  }

  handle->output ("%s: %s\n", handle->file, s);

  EHDR_PRINT_FIELD (e_type);
  EHDR_PRINT_FIELD (e_machine);
  EHDR_PRINT_FIELD (e_version);
  EHDR_PRINT_FIELD (e_entry);
  EHDR_PRINT_FIELD (e_phoff);
  EHDR_PRINT_FIELD (e_shoff);
  EHDR_PRINT_FIELD (e_flags);
  EHDR_PRINT_FIELD (e_ehsize);
  EHDR_PRINT_FIELD (e_phentsize);
  EHDR_PRINT_FIELD (e_shentsize);

  handle->output (PRINT_FORMAT, "(shnum)", (uintmax_t) handle->shnum);
  handle->output (PRINT_FORMAT, "(shstrndx)", (uintmax_t) handle->shstrndx);
  handle->output (PRINT_FORMAT, "(phnum)", (uintmax_t) handle->phnum);
}

void
elf_show_symbol (elf_handle* handle, GElf_Sym* esym)
{
#define ESYM_PRINT_FIELD(N) do {                               \
    handle->output (PRINT_FORMAT_X ,#N, (uintmax_t) esym->N);  \
  } while (0)

  if (!handle->output)
    return;

  ESYM_PRINT_FIELD (st_name);
  ESYM_PRINT_FIELD (st_value);
  ESYM_PRINT_FIELD (st_size);
  ESYM_PRINT_FIELD (st_info);
  ESYM_PRINT_FIELD (st_other);
  ESYM_PRINT_FIELD (st_shndx);
}
