#include <check.h>
#include "libcutil.h"

static lcu_slab slab;

START_TEST(pool)
{  
    void* ps1 = lcu_slab_pool_new(&slab); 
    fail_unless(NULL != ps1);
    fail_unless(NULL == slab.pool_freelist);
    void* ps2 = lcu_slab_pool_new(&slab); 
    fail_unless(NULL != ps2);
    fail_unless(NULL == slab.pool_freelist);
    lcu_slab_pool_free(&slab, ps1);
    fail_unless(ps1 == slab.pool_freelist);
    lcu_slab_pool_free(&slab, ps2);
    fail_unless(ps2 == slab.pool_freelist);
    lcu_slab_header *psh = (lcu_slab_header*)slab.pool_freelist;
    fail_unless(ps1 == psh->next);
    void* ps3 = lcu_slab_pool_new(&slab);
    fail_unless(ps2 == ps3);
    fail_unless(ps1 == slab.pool_freelist);
    void* ps4 = lcu_slab_pool_new(&slab);
    fail_unless(ps1 == ps4);
    fail_unless(NULL == slab.pool_freelist);
    void* ps5 = lcu_slab_pool_new(&slab);
    fail_unless(NULL != ps5);
    fail_unless(NULL == slab.pool_freelist);
}
END_TEST

START_TEST(slab_used_list)
{  
    lcu_slab_class* psc = &slab.slabclass[1];
    void* ps = lcu_slab_pool_new(&slab);
    bool ret = lcu_slab_used_add(psc, ps);
    fail_unless(ret);
    fail_unless(NULL != psc->slab_used_list);
    fail_unless(ps == psc->slab_used_list->ptr);
    fail_unless(NULL == psc->slab_used_list->next);
    size_t need_byte = (size_t)ceil(psc->perslab / 8.0);
    void* pv = malloc(need_byte);
    memset(pv, 0, need_byte);
    fail_unless(0 == memcmp(pv, psc->slab_used_list->used_bitmap, need_byte));
    void* ps2 = lcu_slab_pool_new(&slab);
    ret = lcu_slab_used_add(psc, ps2);
    fail_unless(ret);
    fail_unless(NULL != psc->slab_used_list);
    fail_unless(ps2 == psc->slab_used_list->ptr);
    fail_unless(NULL != psc->slab_used_list->next);
    fail_unless(NULL == psc->slab_used_list->next->next);
    void* ps3 = lcu_slab_pool_new(&slab);
    ret = lcu_slab_used_add(psc, ps3);
    fail_unless(ret);
    lcu_slab_used* psu = lcu_slab_used_search(&slab, psc, ((char*)ps + 4));
    fail_unless(psu ==  psc->slab_used_list->next->next);
    psu = lcu_slab_used_search(&slab, psc, ((char*)ps2));
    fail_unless(psu ==  psc->slab_used_list->next);
    psu = lcu_slab_used_search(&slab, psc, ((char*)ps3 + slab.item_max));
    fail_unless(psu ==  psc->slab_used_list);
    psu = lcu_slab_used_search(&slab, psc, 0);
    fail_unless(psu ==  NULL);
    void* ps4 = lcu_slab_used_remove(psc, psc->slab_used_list->next);
    fail_unless(ps4 ==  ps2);
    fail_unless(psc->slab_used_list->ptr == ps3);
    fail_unless(psc->slab_used_list->next->ptr == ps);
    fail_unless(psc->slab_used_list->next->next == NULL);
    void* ps5 = lcu_slab_used_remove(psc, psc->slab_used_list);
    fail_unless(ps5 ==  ps3);
    fail_unless(psc->slab_used_list->ptr == ps);
    fail_unless(psc->slab_used_list->next == NULL);
}
END_TEST

START_TEST(slab_used_bitmap)
{  
    lcu_slab_class* psc = &slab.slabclass[1];
    void* ps = lcu_slab_pool_new(&slab);
    lcu_slab_used_add(psc, ps);
    lcu_slab_used* psu = psc->slab_used_list;
    fail_unless(lcu_slab_used_is_empty(psc, psu));
    char* pc1 = (char*)(psu->ptr) + psc->size * 8; // should point to the head of 2byte
    lcu_slab_used_on(psc, psu, pc1);
    fail_unless(psu->used_bitmap[1] == 1);
    fail_unless(!lcu_slab_used_is_empty(psc, psu));
    char* pc2 = (char*)(psu->ptr) + psc->size * 9;
    lcu_slab_used_on(psc, psu, pc2);
    fail_unless(psu->used_bitmap[1] == 3);
    fail_unless(!lcu_slab_used_is_empty(psc, psu));
    lcu_slab_used_off(psc, psu, pc1);
    fail_unless(psu->used_bitmap[1] == 2);
    fail_unless(!lcu_slab_used_is_empty(psc, psu));
    lcu_slab_used_off(psc, psu, pc2);
    fail_unless(psu->used_bitmap[1] == 0);
    fail_unless(lcu_slab_used_is_empty(psc, psu));
}
END_TEST

START_TEST(slab_it)
{  
    // The freed slab exist at both(slots, end_page)
    // @condition
    // lcu_slab.item_min = 128
    // lcu_slab.item_max = 1024 * 1024
    // slab class:12, chunk size:278528, perslab:3
    size_t alloc_size = 240000;
    int clsid = 12;
    slab.pool_freelist = NULL;

    void* p1 = lcu_slab_alloc(&slab, alloc_size);
    void* p2 = lcu_slab_alloc(&slab, alloc_size);
    lcu_slab_free(&slab, p1);
    fail_unless(slab.pool_freelist == NULL);
    lcu_slab_class* psc = &slab.slabclass[clsid];
    fail_unless(psc->slab_used_list != NULL);
    fail_unless(psc->slab_used_list->next == NULL);
    fail_unless(psc->sl_curr == 1);
    fail_unless(psc->end_page_ptr != NULL);
    fail_unless(psc->end_page_free == 1);
    lcu_slab_free(&slab, p2);
    fail_unless(slab.pool_freelist != NULL);
    fail_unless(psc->slab_used_list == NULL);
    fail_unless(psc->sl_curr == 0);
    fail_unless(psc->end_page_ptr == NULL);
    fail_unless(psc->end_page_free == 0);
    // The freed slab exist at end_page
    void* p3 = lcu_slab_alloc(&slab, alloc_size);
    fail_unless(p1 == p3);
    fail_unless(slab.pool_freelist == NULL);
    fail_unless(psc->slots == NULL);
    lcu_slab_free(&slab, p3);
    fail_unless(slab.pool_freelist != NULL);
    lcu_slab_header *psh = (lcu_slab_header*)slab.pool_freelist;
    fail_unless(psh->next == NULL);
    fail_unless(psc->slab_used_list == NULL);
    fail_unless(psc->sl_curr == 0);
    fail_unless(psc->end_page_ptr == NULL);
    fail_unless(psc->end_page_free == 0);
    // The freed slab exist at slots
    void* p11 = lcu_slab_alloc(&slab, alloc_size);
    lcu_slab_alloc(&slab, alloc_size);
    void* p13 = lcu_slab_alloc(&slab, alloc_size);
    void* p14 = lcu_slab_alloc(&slab, alloc_size); // second slab start
    void* p15 = lcu_slab_alloc(&slab, alloc_size);
    void* p16 = lcu_slab_alloc(&slab, alloc_size);
    fail_unless(psc->end_page_ptr == NULL);
    fail_unless(psc->end_page_free == 0);
    fail_unless(psc->slab_used_list != NULL);
    fail_unless(psc->slab_used_list->next != NULL);
    fail_unless(psc->slab_used_list->next->next == NULL);
    lcu_slab_free(&slab, p11);
    lcu_slab_free(&slab, p13);
    lcu_slab_free(&slab, p14);
    lcu_slab_free(&slab, p15);
    fail_unless(psc->slots != NULL);
    fail_unless(psc->sl_curr == 4);
    lcu_slab_free(&slab, p16); // moved from slots to pool_list
    fail_unless(psc->slots != NULL);
    fail_unless(psc->sl_curr == 2);
    psh = (lcu_slab_header*)psc->slots;
    size_t* psize = (size_t*)(psh + 1);
    fail_unless((++psize) == p13);
    psh = psh->next;
    psize = (size_t*)(psh + 1);
    fail_unless((++psize) == p11);
    fail_unless(psh->next == NULL);
    fail_unless(psc->end_page_ptr == NULL);
    fail_unless(psc->end_page_free == 0);
    psh = (lcu_slab_header*)slab.pool_freelist;
    fail_unless(psh != NULL);
    fail_unless(psh->next == NULL);
    void* p17 = lcu_slab_alloc(&slab, alloc_size);
    fail_unless(p17 == p13);
    fail_unless(psc->slots != NULL);
    fail_unless(psc->sl_curr == 1);
}
END_TEST

START_TEST(slab_exceed_limit)
{  
    void* p1;
    int i;
    lcu_slab slab_ex;
    const size_t EXPECTED_MAX_ITEM = 4;
    const size_t ITEM_MIN = 32;
    const size_t ITEM_MAX = 64 * 1024;
    const size_t ALLOC_SIZE = ITEM_MAX - sizeof(lcu_slab_header) - sizeof(size_t);
    memset(&slab_ex, 0, sizeof(lcu_slab));
    lcu_slab_init(&slab_ex, ITEM_MAX * EXPECTED_MAX_ITEM, 2, ITEM_MIN, ITEM_MAX);
    for (i = 0; i < EXPECTED_MAX_ITEM; i++) {
        p1 = lcu_slab_alloc(&slab_ex, ALLOC_SIZE);
        fail_unless(p1 != NULL);
    }
    void* p2 = lcu_slab_alloc(&slab_ex, ALLOC_SIZE);
    fail_unless(p2 == NULL);
    lcu_slab_free(&slab_ex, p1);
    // success if request size belongs to the above same slab
    void* p3 = lcu_slab_alloc(&slab_ex, ALLOC_SIZE);
    fail_unless(p3 != NULL);
    lcu_slab_free(&slab_ex, p3);
    // success even if request size belongs to a different slab
    void* p4 = lcu_slab_alloc(&slab_ex, 512);
    fail_unless(p4 != NULL);
}
END_TEST

Suite* slab_suite(void) {
    Suite *s = suite_create("lcu_slab.c");
    /* Core test case */
    memset(&slab, 0, sizeof(slab));
    lcu_slab_init(&slab, 0, 2, 128, 1024 * 1024);
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, pool);
    tcase_add_test(tc_core, slab_used_list);
    tcase_add_test(tc_core, slab_used_bitmap);
    tcase_add_test(tc_core, slab_it);
    tcase_add_test(tc_core, slab_exceed_limit);
    suite_add_tcase(s, tc_core);
    return s;
}
