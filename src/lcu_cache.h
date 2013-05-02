#ifndef _LCU_CACHE_H_
#define _LCU_CACHE_H_

#include <libcutil.h>

typedef enum lcu_cache_opt {
    auto_eject_on,
    auto_eject_off
} lcu_cache_opt;
typedef struct lcu_cache {
    Hmap* hmap;
    lcu_lru* lru;
    lcu_slab* slab;
    lcu_cache_opt opt;
    size_t mem_active_size; // not malloced space size but actually used space size
    // mem_alloced_size are retrived from slab
    // max_limit are retrived from slab
    // item_size are retrieved from hmap(lcu_map_count)
} lcu_cache;

#define lcu_cache_mem_active_size(pc) ((pc)->mem_active_size)
#define lcu_cache_mem_alloc_size(pc) ((pc)->slab->mem_malloced)
#define lcu_cache_item_size(pc) (lcu_map_count((pc)->hmap))

void lcu_cache_init(lcu_cache* pc, lcu_cache_opt opt, size_t limit, double factor, size_t min, size_t max);
void lcu_cache_get(lcu_cache* pc, String key, String* val);
bool lcu_cache_put(lcu_cache* pc, String key, String val);
bool lcu_cache_delete(lcu_cache* pc, String key);
void lcu_cache_eldest(lcu_cache* pc, String* key, String* val);
void lcu_cache_iterator(lcu_cache* pc, String* key, String* val);
void lcu_cache_iterator_next(lcu_cache* pc, String* key, String* val);
void lcu_cache_destroy(lcu_cache* pc);

#endif
