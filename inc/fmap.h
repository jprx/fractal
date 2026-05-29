#ifndef FMAP_H
#define FMAP_H

#include <fractal.h>

BEGIN_C_HEADER

// Every time the user calls fmap, one of these is allocated to track the pages occupied
typedef struct fmap_alloc_t {
    struct fmap_alloc_t *f_next;
    usize f_base_va;
    usize f_n_copies;
    usize f_log_stride;
} fmap_alloc_t;

// Metadata for the gigamap, if it exists
typedef struct {
    u64     *g_l2_pagetable;
    u64     *g_l3_pagetable;
    virt_t   g_map_va; // VA of the gmap
    phys_t   g_map_pa; // PA of the default page
    virt_t   g_backing_va; // VA that always points to the default page, no matter what's carved out
    bool     g_is_mapped;
    PagePrivilege g_privs;
} gigamap_t;

void cleanup_fmap_list(struct task_t *t);
void init_gmap(gigamap_t *g);

END_C_HEADER

#endif // FMAP_H
