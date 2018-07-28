#ifndef RAT_H
#define RAT_H

#include "pager.h"

void rat_init();
void rat_cleanup();
void rat_push_pager(Pager *p);
void rat_pop_pager();
void rat_render();
size_t rat_num_pagers();

#endif
