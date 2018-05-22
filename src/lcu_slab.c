#include "libcutil.h"

#define     SETTING_CHUNK_SIZE  128
#define     SETTING_ITEM_SIZE_MAX   1024 * 1024 * 1
#define     POWER_SMALLEST      1
#define     CHUNK_ALIGN_BYTES   8
#define     SETTING_VERBOSE     1

/*
 * slab pool management
 */
void* lcu_slab_pool_new(lcu_slab* ps) {
    void* ptr;
    lcu_slab_header* psh;
    if (ps->pool_freelist == NULL) {
        dprintf("lcu_slab_pool_new limit:%u alloc:%u max:%u min:%u \n",
                ps->mem_limit, ps->mem_malloced, ps->item_max, ps->item_min);
        if (ps->mem_limit &&
            (ps->mem_malloced + ps->item_max > ps->mem_limit)) {
            return NULL;
        }
        ptr = malloc(ps->item_max);
        if (!ptr) return NULL;
        ps->mem_malloced += ps->item_max;
        psh = (lcu_slab_header*)ptr;
        psh->next = NULL;
        ps->pool_freelist = ptr;
    }
    psh = ps->pool_freelist;
    ps->pool_freelist = psh->next;
    return (void*)psh;
}

void lcu_slab_pool_free(lcu_slab* ps, void* ptr) {
    lcu_slab_header* psh;
    psh = (lcu_slab_header*)ptr;
    psh->next = ps->pool_freelist;
    ps->pool_freelist = psh;
}

/*
 * slab used linked list management per lcu_slab_class
 */
bool lcu_slab_used_add(lcu_slab_class* psc, void* ptr2slab) {
    size_t need_byte;
    lcu_slab_used* psu = (lcu_slab_used*)malloc(sizeof(lcu_slab_used));
    if (!psu) return false;
    need_byte = (size_t)ceil(psc->perslab / 8.0);
    psu->used_bitmap = (unsigned char*)malloc(need_byte);
    if (!psu->used_bitmap) return false;
    memset(psu->used_bitmap, 0, need_byte);
    psu->ptr = ptr2slab;
    psu->next = psc->slab_used_list;
    psc->slab_used_list = psu;
    return true;
}

void* lcu_slab_used_remove(lcu_slab_class* psc, lcu_slab_used* psu_target) {
    void* pret;
    lcu_slab_used* psu = psc->slab_used_list;
    lcu_slab_used* pprev = NULL;
    while (psu != NULL) {
        if (psu == psu_target) {
            if (pprev) {
                pprev->next = psu->next;
            } else {
                psc->slab_used_list = psu->next;
            }
            pret = psu->ptr;
            free(psu->used_bitmap);
            free(psu);
            return pret;
        }
        pprev = psu;
        psu = psu->next;
    }
    return NULL;
}

lcu_slab_used* lcu_slab_used_search(lcu_slab* ps, lcu_slab_class* psc, char* ptr_in_slab) {
    lcu_slab_used* psu = psc->slab_used_list;
    char* pstart;
    char* pend;
    while (psu != NULL) {
        pstart = (char*)psu->ptr;
        pend = pstart + ps->item_max;
        if (ptr_in_slab >= pstart && ptr_in_slab <= pend) return psu;
        psu = psu->next;
    }
    return NULL;
}

/*
 * slab free space management per slab
 */
#define SLABLIST_USED_IDX(pi, pbi, psct, pslt, ptr_in_slab) \
    size_t byte_offset = (size_t)(ptr_in_slab - ((char*)pslt->ptr)); \
    *pi = (size_t)(byte_offset / psct->size); \
    *pbi = (size_t)round(index / 8)

inline void lcu_slab_used_on(lcu_slab_class* psc, lcu_slab_used* psu, char* ptr_in_slab) {
    size_t index;
    size_t bmp_index;
    SLABLIST_USED_IDX(&index, &bmp_index, psc, psu, ptr_in_slab);
    unsigned char bitmask = (unsigned char)(1 << (index % 8));
    psu->used_bitmap[bmp_index] |= bitmask;
}

inline void lcu_slab_used_off(lcu_slab_class* psc, lcu_slab_used* psu, char* ptr_in_slab) {
    size_t index;
    size_t bmp_index;
    SLABLIST_USED_IDX(&index, &bmp_index, psc, psu, ptr_in_slab);
    unsigned char bitmask = ~(unsigned char)(1 << (index % 8));
    psu->used_bitmap[bmp_index] &= bitmask;
}

inline bool lcu_slab_used_is_empty(lcu_slab_class* psc, lcu_slab_used* psu) {
    unsigned char* pcurrent = (unsigned char*)psu->used_bitmap;
    size_t need_byte = (size_t)ceil(psc->perslab / 8.0);
    while (need_byte > 0) {
        if (need_byte >= sizeof(unsigned int)) {
            if (*((unsigned int*)pcurrent)) return false;
            need_byte -= sizeof(unsigned int);
            pcurrent += sizeof(unsigned int);
        } else if (need_byte >= sizeof(unsigned short)) {
            if (*((unsigned short*)pcurrent)) return false;
            need_byte -= sizeof(unsigned short);
            pcurrent += sizeof(unsigned short);
        } else {
            if (*pcurrent) return false;
            need_byte -= sizeof(unsigned char);
            pcurrent += sizeof(unsigned char);
        }
    }
    return true;
}

static unsigned int slabs_clsid(lcu_slab* ps, const size_t size) {
    int res = POWER_SMALLEST;
    if (size == 0)
        return 0;
    while (size > ps->slabclass[res].size)
        if (res++ == ps->power_largest)     /* won't fit in the biggest slab */
            return 0;
    return res;
}

void lcu_slab_init(lcu_slab* ps, const size_t limit, const double factor,
        size_t min, size_t max) {
    int i = POWER_SMALLEST - 1;

    memset(ps, 0, sizeof(lcu_slab));
    if (limit > 0 && limit < ps->item_max) {
        ps->mem_limit = ps->item_max;
    } else {
        ps->mem_limit = limit;
    }
    ps->item_min = min;
    ps->item_max = max;
    unsigned int size = sizeof(lcu_slab_header) + ps->item_min;

    //memset(ps->slabclass, 0, sizeof(ps->slabclass));

    while (++i < POWER_LARGEST && size <= ps->item_max / factor) {
        /* Make sure items are always n-byte aligned */
        if (size % CHUNK_ALIGN_BYTES)
            size += CHUNK_ALIGN_BYTES - (size % CHUNK_ALIGN_BYTES);

        ps->slabclass[i].size = size;
        ps->slabclass[i].perslab = ps->item_max / ps->slabclass[i].size;
        size *= factor;
        if (SETTING_VERBOSE > 1) {
            fprintf(stderr, "slab class %3d: chunk size %9u perslab %7u\n",
                    i, ps->slabclass[i].size, ps->slabclass[i].perslab);
        }
    }

    ps->power_largest = i;
    ps->slabclass[ps->power_largest].size = ps->item_max;
    ps->slabclass[ps->power_largest].perslab = 1;
    if (SETTING_VERBOSE > 1) {
        fprintf(stderr, "slab class %3d: chunk size %9u perslab %7u\n",
                i, ps->slabclass[i].size, ps->slabclass[i].perslab);
        fprintf(stderr, "ps:%p\n", ps);
    }

}

static int do_slabs_newslab(lcu_slab* ps, lcu_slab_class* psc) {
    void* ptr = lcu_slab_pool_new(ps);
    dprintf("do_slabs_newslab:st slab:%p size:%u ptr_from_pool:%p\n", ps, psc->size, ptr);
    if (ptr == NULL) return 0;

    psc->end_page_ptr = ptr;
    psc->end_page_free = psc->perslab;

    bool ret = lcu_slab_used_add(psc, ptr);
    dprintf("do_slabs_newslab:ed slab:%p size:%u ret:%d\n", ps, psc->size, ret);
    if (!ret) return 0;

    return 1;
}

static void *do_slabs_alloc(lcu_slab* ps, const size_t size, unsigned int id) {
    lcu_slab_class *psc;
    void *ret = NULL;
    lcu_slab_header* psh = NULL;
    lcu_slab_used* psu = NULL;

    if (id < POWER_SMALLEST || id > ps->power_largest) {
        return NULL;
    }

    psc = &ps->slabclass[id];
    dprintf("alloc slab:%p class:%u ent_page_p:%p sl_curr:%u \n", ps, psc->size, psc->end_page_ptr, psc->sl_curr);

    /* fail unless we have space at the end of a recently allocated page,
       we have something on our freelist, or we could allocate a new page */
    if (! (psc->end_page_ptr != 0 || psc->sl_curr != 0 ||
           do_slabs_newslab(ps, psc) != 0)) {
        /* We don't have more memory available */
        ret = NULL;
    } else if (psc->sl_curr != 0) {
        /* return off our freelist */
        psh = (lcu_slab_header*)psc->slots;
        psc->slots = psh->next;
        psc->sl_curr--;
        ret = (void*)psh;
        psu = lcu_slab_used_search(ps, psc, (char*)ret);
        lcu_slab_used_on(psc, psu, (char*)ret);
    } else if (psc->end_page_ptr == NULL) {
        /* failed do_slabs_newslab */
        ret = NULL;
    } else {
        /* if we recently allocated a whole page, return from that */
        ret = psc->end_page_ptr;
        if (--psc->end_page_free != 0) {
            psc->end_page_ptr = ((char*)psc->end_page_ptr) + psc->size;
        } else {
            psc->end_page_ptr = 0;
        }
        psu = lcu_slab_used_search(ps, psc, (char*)ret);
        lcu_slab_used_on(psc, psu, (char*)ret);
    }

    if (ret) {
        psc->requested += size;
    }
    dprintf("alloc ps:%p sid:%u p:%p np:%p cnt:%u used:%lu rest:%u \n", ps, id, ret, psc->end_page_ptr, psc->sl_curr, psc->requested, psc->end_page_free);
    return ret;
}

static void do_slabs_free(lcu_slab* ps, void *ptr, const size_t size, unsigned int id) {
    lcu_slab_class* psc;
    lcu_slab_header* psh;
    lcu_slab_used* psu;
    bool ret;
    void* ppool;
    dprintf("free slab:%p ptr:%p size:%u id:%u \n", ps, ptr, size, id);
    if (id < POWER_SMALLEST || id > ps->power_largest)
        return;

    psc = &ps->slabclass[id];

    psh = (lcu_slab_header*)ptr;
    psh->next = psc->slots;
    psc->slots = psh;

    psc->sl_curr++;
    psc->requested -= size;

    dprintf("free slab:%p class:%u ent_page_p:%p sl_curr:%u \n", ps, psc->size, psc->end_page_ptr, psc->sl_curr);

    psu = lcu_slab_used_search(ps, psc, (char*)ptr);
    lcu_slab_used_off(psc, psu, (char*)ptr);
    ret = lcu_slab_used_is_empty(psc, psu);
    if (ret) {
        // release slab from freelist(slots)
        lcu_slab_header* pcur = psh;
        lcu_slab_header* pprev = NULL;
        while (pcur != NULL) {
            lcu_slab_used* psu_curr = lcu_slab_used_search(ps, psc, (char*)pcur);
            if (psu == psu_curr) {
                if (pprev) {
                    pprev->next = pcur->next;
                } else {
                    psc->slots = pcur->next;
                }
                psc->sl_curr--;
            } else {
                pprev = pcur;
            }
            pcur = pcur->next;
        }
        // release slab from end_page(end_page_ptr, end_page_free)
        lcu_slab_used* psu_endp = lcu_slab_used_search(ps, psc, (char*)psc->end_page_ptr);
        if (psu_endp == psu) {
            psc->end_page_ptr = NULL;
            psc->end_page_free = 0;
        }
        // release slab from slab used linked list
        ppool = lcu_slab_used_remove(psc, psu);
        lcu_slab_pool_free(ps, ppool);
    }
    return;
}

void *lcu_slab_alloc(lcu_slab* ps, size_t size) {
    void* ret;
    size += sizeof(lcu_slab_header) + sizeof(size_t);
    unsigned int id = slabs_clsid(ps, size);
    ret = do_slabs_alloc(ps, size, id);
    if (ret == NULL) {
        return NULL;
    }
    size_t* psize = (size_t*)((char*)ret + sizeof(lcu_slab_header));
    *psize = size;
    return (void*)(++psize);
}

void lcu_slab_free(lcu_slab* ps, void* ptr) {
    size_t* psize = (size_t*)((char*)ptr - sizeof(size_t));
    size_t size = *psize;
    unsigned int id = slabs_clsid(ps, size);
    void* ph = (void*)((char*)psize - sizeof(lcu_slab_header));
    do_slabs_free(ps, ph, size, id);
}
