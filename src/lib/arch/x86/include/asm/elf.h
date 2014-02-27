#ifndef __ASM_LIBCOMPEL_ELF_H__
#define __ASM_LIBCOMPEL_ELF_H__

#include <sys/types.h>
#include <elf.h>

typedef struct {
	Elf64_Ehdr	*ehdr;
} elf_map_t;

#endif /* __ASM_LIBCOMPEL_ELF_H__ */
