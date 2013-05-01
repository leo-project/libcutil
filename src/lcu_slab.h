/* slabs memory allocation */
#ifndef _LCU_SLABS_H
#define _LCU_SLABS_H

#include "libcutil.h"

typedef struct lcu_slab_header {
    struct lcu_slab_header* next;
} lcu_slab_header;

typedef struct lcu_slab_used {
    void* ptr;
    unsigned char* used_bitmap; /* store where in an slab are used by bitmap(0:free, 1:used) */
    struct lcu_slab_used* next;
} lcu_slab_used;

typedef struct {
    unsigned int size;              /* sizes of items */
    unsigned int perslab;           /* how many items per slab */
    void* slots;                    /* list of item ptrs */
    unsigned int sl_curr;           /* total free items in list */
    void* end_page_ptr;             /* pointer to next free item at end of page, or 0 */
    unsigned int end_page_free;     /* number of items remaining at end of last alloced page */
    unsigned int slabs;             /* how many slabs were allocated for this class */
    lcu_slab_used* slab_used_list;  /* linked list of lcu_slab_used pointer */
    size_t requested;               /* The number of requested bytes */
} lcu_slab_class;

#define     POWER_LARGEST       200
#define     MAX_NUMBER_OF_SLAB_CLASSES  (POWER_LARGEST + 1)

typedef struct {
    lcu_slab_class slabclass[MAX_NUMBER_OF_SLAB_CLASSES];
    size_t item_min;
    size_t item_max;
    size_t mem_limit;
    size_t mem_malloced;
    int power_largest;
    void* pool_freelist;
} lcu_slab;

/* export for test */
void* lcu_slab_pool_new(lcu_slab* ps);
void lcu_slab_pool_free(lcu_slab* ps, void* ptr2slab);
bool lcu_slab_used_add(lcu_slab_class* psc, void* ptr2slab);
void* lcu_slab_used_remove(lcu_slab_class* psc, lcu_slab_used* psu);
lcu_slab_used* lcu_slab_used_search(lcu_slab* ps, lcu_slab_class* psc, char* ptr_in_slab);
void lcu_slab_used_on(lcu_slab_class* psc, lcu_slab_used* psu, char* ptr_in_slab);
void lcu_slab_used_off(lcu_slab_class* psc, lcu_slab_used* psu, char* ptr_in_slab);
bool lcu_slab_used_is_empty(lcu_slab_class* psc, lcu_slab_used* psu);

/** Init the subsystem. 1st argument is the limit on no. of bytes to allocate,
    0 if no limit. 2nd argument is the growth factor; each slab will use a chunk
    size equal to the previous slab's chunk size times this factor.
    3rd argument specifies if the slab allocator should allocate all memory
    up front (if true), or allocate memory in chunks as it is needed (if false)
*/
void lcu_slab_init(lcu_slab* ps, const size_t limit, const double factor, 
        size_t min, size_t max);
/** Allocate object of given length. 0 on error */ /*@null@*/
void* lcu_slab_alloc(lcu_slab* ps, const size_t size);
/** Free previously allocated object */
void lcu_slab_free(lcu_slab* ps, void *ptr);

#endif
