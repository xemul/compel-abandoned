#ifndef __COMPEL_ELF_H__
#define __COMPEL_ELF_H__

#include <sys/types.h>
#include <elf.h>

typedef struct load_info_s {
	/*
	 * When elf is injected into another process address
	 * space we need the caller to initialize the delta,
	 * which will be used for relocations fixups and
	 * symbols lookup.
	 *
	 * All other members are treated as belonging to
	 * caller address space.
	 */
	long			addr_delta;

	Elf64_Ehdr		*hdr;
	size_t			len;

	Elf64_Shdr		*sechdrs;
	char			*secstrings;
	char			*strtab;

	struct {
		unsigned int	sym;
		unsigned int	str;

		unsigned int	symtab;
		unsigned int	symtab_link;

		unsigned int	init;
	} index;
} load_info_t;

extern int load_elf_plugin(load_info_t *info);
extern unsigned long lookup_elf_plugin_symbol(load_info_t *info, const char *symbol_name, bool local);

#define lookup_elf_plugin_local_symbol(info, name)	\
	lookup_elf_plugin_symbol(info, name, true)

#define lookup_elf_plugin_remote_symbol(info, name)	\
	lookup_elf_plugin_symbol(info, name, false)

#endif /* __COMPEL_ELF_H__ */
