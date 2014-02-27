#ifndef __ASM_LIBCOMPEL_PAGE_H__
#define __ASM_LIBCOMPEL_PAGE_H__

#define PAGE_SHIFT		(12)
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGES(len)		((len) >> PAGE_SHIFT)

#endif /* __ASM_LIBCOMPEL_PAGE_H__ */
