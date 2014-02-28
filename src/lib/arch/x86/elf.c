#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "compiler.h"
#include "xmalloc.h"
#include "types.h"
#include "err.h"
#include "log.h"
#include "elf.h"

#undef LOG_PREFIX
#define LOG_PREFIX "elf: "

#if 0
/* Check if pointer is out-of-bound */
static bool __ptr_oob(void *ptr, void *start, size_t size)
{
	void *end = (void *)((unsigned long)start + size);
	return ptr > end || ptr < start;
}

#define ptr_oob(p, start, size)		(__ptr_oob((void *)p, (void *)start, (size_t)size))
#define ptr_oob_t(p, start, size)	(__ptr_oob((void *)p, (void *)start, (size_t)size - (sizeof(*p))))
#endif

static int elf_header_check(load_info_t *info)
{
	if (info->len < sizeof(*(info->hdr)))
		return -ENOEXEC;

	if (memcmp(info->hdr->e_ident, ELFMAG, SELFMAG) != 0	||
	    info->hdr->e_type != ET_REL				||
	    info->hdr->e_machine != EM_X86_64			||
	    info->hdr->e_shentsize != sizeof(Elf64_Shdr))
		return -ENOEXEC;

	if (info->hdr->e_shoff >= info->len			||
	    (info->hdr->e_shnum * sizeof(Elf64_Shdr) >
	     info->len - info->hdr->e_shoff))
		return -ENOEXEC;

	return 0;
}

static int rewrite_section_headers(load_info_t *info)
{
	unsigned int i;

	info->sechdrs[0].sh_addr = 0;

	for (i = 1; i < info->hdr->e_shnum; i++) {
		Elf64_Shdr *shdr = &info->sechdrs[i];

		if (shdr->sh_type != SHT_NOBITS &&
		    info->len < shdr->sh_offset + shdr->sh_size) {
			pr_err("Plugin len %lu truncated\n", info->len);
			return -ENOEXEC;
		}

		shdr->sh_addr = (size_t)info->hdr + shdr->sh_offset;
	}

	return 0;
}

static int simplify_symbols(load_info_t *info)
{
	Elf64_Shdr *symsec = &info->sechdrs[info->index.sym];
	Elf64_Sym *sym = (void *)symsec->sh_addr;
	unsigned long secbase;
	unsigned int i;
	int ret = 0;

	for (i = 1; i < symsec->sh_size / sizeof(Elf64_Sym) && ret == 0; i++) {
		const char *name = info->strtab + sym[i].st_name;

		switch (sym[i].st_shndx) {
		case SHN_COMMON:
			pr_debug("Common symbol: %s\n", name);
			pr_err("Please compile with -fno-common\n");
			ret = -ENOEXEC;
			break;
		case SHN_ABS:
			pr_debug("Absolute symbol: %32s 0x%08lx\n", name, (long)sym[i].st_value);
			break;
		case SHN_UNDEF:
			if (strcmp(name, "_GLOBAL_OFFSET_TABLE_")) {
				pr_err("Unknown symbol: %32s 0x%08lx\n", name, (long)sym[i].st_value);
				ret = -ENOENT;
			} else
				pr_debug("Undef symbol: %32s 0x%08lx\n", name, (long)sym[i].st_value);
			break;
		default:
			secbase = info->sechdrs[sym[i].st_shndx].sh_addr;
			sym[i].st_value += secbase;
			pr_debug("sym %32s %8lx -> %8lx\n", name, sym[i].st_value - secbase, sym[i].st_value);
			break;
		}
	}

	return ret;
}

static int apply_relocate_add(Elf64_Shdr *sechdrs, const char *strtab, unsigned int symindex,
			      unsigned int relsec, long addr_delta)
{
	Elf64_Rela *rel = (void *)sechdrs[relsec].sh_addr;
	Elf64_Sym *sym;
	unsigned int i;
	void *loc;
	u64 val;

	const char *reloc_types[] = {
		"R_X86_64_NONE",
		"R_X86_64_64",
		"R_X86_64_PC32",
		"R_X86_64_GOT32",
		"R_X86_64_PLT32",
		"R_X86_64_COPY",
		"R_X86_64_GLOB_DAT",
		"R_X86_64_JUMP_SLOT",
		"R_X86_64_RELATIVE",
		"R_X86_64_GOTPCREL",
		"R_X86_64_32",
		"R_X86_64_32S",
		"R_X86_64_16",
		"R_X86_64_PC16",
		"R_X86_64_8",
		"R_X86_64_PC8",
		"R_X86_64_NUM",
	};

	pr_debug("Applying relocate section %2u to %2u\n", relsec, sechdrs[relsec].sh_info);

	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
		loc = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr + rel[i].r_offset;
		sym = (Elf64_Sym *)sechdrs[symindex].sh_addr + ELF64_R_SYM(rel[i].r_info);

		val = (s64)sym->st_value + rel[i].r_addend;

		pr_debug("type %2d (%18s) st_value %8Lx r_addend %10Li loc %8Lx (val %8Lx -> ",
			 (int)ELF64_R_TYPE(rel[i].r_info),
			 reloc_types[(int)ELF64_R_TYPE(rel[i].r_info)],
			 sym->st_value, rel[i].r_addend, (u64)loc, (u64)val);

		switch (ELF64_R_TYPE(rel[i].r_info)) {
		case R_X86_64_NONE:
			break;
		case R_X86_64_64:
			*(u64 *)loc = val + addr_delta;
			break;
		case R_X86_64_32:
			*(u32 *)loc = val;
			if (val != *(u32 *)loc)
				goto overflow;
			break;
		case R_X86_64_32S:
			*(s32 *)loc = val;
			if ((s64)val != *(s32 *)loc)
				goto overflow;
			break;
		case R_X86_64_PLT32:
		case R_X86_64_PC32:
			val -= (u64)loc;
			*(u32 *)loc = val;
			break;
		default:
			pr_err("Unknown rela relocation: %llu (%s)\n",
			       ELF64_R_TYPE(rel[i].r_info),
			       reloc_types[(int)ELF64_R_TYPE(rel[i].r_info)]);
			return -ENOEXEC;
		}

		pr_debug("%8Lx)\n", (u64)val);
	}
	return 0;

overflow:
	pr_err("overflow in relocation type %d val %Lx\n", (int)ELF64_R_TYPE(rel[i].r_info), val);
	pr_err("plugin is likely not compiled with -mcmodel=kernel\n");
	return -ENOEXEC;
}

static int apply_relocations(const load_info_t *info)
{
	unsigned int i;
	int err = 0;

	for (i = 1; i < info->hdr->e_shnum; i++) {
		unsigned int infosec = info->sechdrs[i].sh_info;

		if (infosec >= info->hdr->e_shnum)
			continue;

		if (!(info->sechdrs[infosec].sh_flags & SHF_ALLOC))
			continue;

		if (info->sechdrs[i].sh_type == SHT_REL) {
			pr_err("SHT_REL unsupported\n");
			err = -EINVAL;
		} else if (info->sechdrs[i].sh_type == SHT_RELA)
			err = apply_relocate_add(info->sechdrs, info->strtab,
						 info->index.sym, i,
						 info->addr_delta);
		if (err < 0)
			break;
	}
	return err;
}

/*
 * FIXME Better use some hash table or something, since
 * there might be a bunch of symbols.
 */
unsigned long lookup_elf_plugin_symbol(load_info_t *info,
				       const char *symbol_name,
				       bool local)
{
	Elf64_Shdr *symsec = &info->sechdrs[info->index.sym];
	Elf64_Sym *sym = (void *)symsec->sh_addr;
	unsigned int i;

	for (i = 1; i < symsec->sh_size / sizeof(Elf64_Sym); i++) {
		const char *name = info->strtab + sym[i].st_name;

		if (!strcmp(name, symbol_name)) {
			if (local)
				return sym[i].st_value;
			else
				return sym[i].st_value + info->addr_delta;
		}
	}

	return 0;
}

static int fixup_init(load_info_t *info)
{
	Elf64_Shdr *initsec = &info->sechdrs[info->index.init];
	unsigned long *where;

	if (!info->index.init)
		return -ENOENT;

	where = (void *)lookup_elf_plugin_symbol(info, "__export_std_plugin_begin", true);
	if (!where)
		return -ENOENT;
	*where = (unsigned long)initsec->sh_addr + info->addr_delta;

	where = (void *)lookup_elf_plugin_symbol(info, "__export_std_plugin_size", true);
	if (!where)
		return -ENOENT;
	*where = (unsigned long)initsec->sh_size;

	return 0;
}

static int setup_load_headers(load_info_t *info)
{
	unsigned int i;

	info->sechdrs	= (void *)info->hdr + info->hdr->e_shoff;
	info->secstrings= (void *)info->hdr + info->sechdrs[info->hdr->e_shstrndx].sh_offset;

	for (i = 1; i < info->hdr->e_shnum; i++) {
		if (info->sechdrs[i].sh_type == SHT_SYMTAB) {
			info->index.sym = i;
			info->index.str = info->sechdrs[i].sh_link;
			info->strtab = (char *)info->hdr + info->sechdrs[info->index.str].sh_offset;
			break;
		} else if (info->sechdrs[i].sh_type == SHT_PROGBITS) {
			if (!strcmp(&info->secstrings[info->sechdrs[i].sh_name], ".compel.init")) {
				info->index.init = i;
			}
		}
	}

	if (info->index.sym == 0) {
		pr_warn("Plugin has no symbols (stripped?)\n");
		return -ENOEXEC;
	}

	return 0;
}

static int load_elf_headers(load_info_t *info)
{
	int err = 0;

	if (!err)
		err = elf_header_check(info);
	if (!err)
		err = setup_load_headers(info);

	/*
	 * FIXME Check that we have all sections we need.
	 */

	if (!err)
		err = rewrite_section_headers(info);
	if (!err)
		err = simplify_symbols(info);

	return err;
}

int load_elf_plugin(load_info_t *info)
{
	int err = 0;

	if (!err)
		err = load_elf_headers(info);
	if (!err)
		err = apply_relocations(info);
	if (!err)
		err = fixup_init(info);

	return err;
}
