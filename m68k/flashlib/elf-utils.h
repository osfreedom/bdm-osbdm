/*
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

#if !defined (_ELF_UTILS_H_)
#define _ELF_UTILS_H_

#define __LIBELF_INTERNAL__ 1
#include <gelf.h>

/*
 * Output routine.
 */
typedef int (*elf_output) (const char *format, ...);

/*
 * Handle to access the file via libelf.
 */
typedef struct
{
  char* file;
  int fd;
  Elf* elf;
  Elf_Kind ek;
  GElf_Ehdr ehdr;
  int eclass;
  size_t shnum;
  size_t shstrndx;
  size_t phnum;
  elf_output output;
} elf_handle;

/*
 * Section handler.
 */
typedef int (*elf_section_handler) (elf_handle* handle,
                                    GElf_Phdr*  phdr, 
                                    GElf_Shdr*  shdr,
                                    const char* sname, 
                                    int         sindex);

void elf_handle_init (elf_handle* handle);

int elf_open (const char* file, elf_handle* handle, elf_output output);
int elf_close (elf_handle* handle);

int elf_get_symbol (elf_handle* handle, const char* label, GElf_Sym* sym);

int elf_get_section_hdr (elf_handle* handle, int secindex, GElf_Shdr* shdr);

void* elf_get_section_data (elf_handle* handle, int secindex,
                            uint32_t* size);
void* elf_get_section_data_sym (elf_handle* handle, const char* label);
int elf_map_over_sections (elf_handle* handle, 
                           elf_section_handler handler, const char* sname);

void elf_show_exeheader (elf_handle* handle);
void elf_show_symbol (elf_handle* handle, GElf_Sym* esym);

#endif
