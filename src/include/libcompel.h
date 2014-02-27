#ifndef __LIBCOMPEL_H__
#define __LIBCOMPEL_H__

#include "uapi/libcompel.h"

struct parasite_ctl_s;
struct load_info_s;
struct argv_desc_s;

extern int libcompel_fill_argv(struct parasite_ctl_s *ctl, struct load_info_s *info, struct argv_desc_s *d);

#endif /* __LIBCOMPEL_H__ */
