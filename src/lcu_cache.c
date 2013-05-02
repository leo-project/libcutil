#include <libcutil.h>

void lcu_cache_init(lcu_cache* lc, lcu_cache_opt opt, size_t limit, double factor, size_t min, size_t max) {
    int64 hint = (int64)(limit / ((min + max) / 8)); // @FIXME maybe to be configurable is better
    lc->hmap = lcu_map_init(&StrMapType, hint);
    lc->lru = lcu_lru_create();
    lc->slab = malloc(sizeof(lcu_slab));
    lcu_slab_init(lc->slab, limit, factor, min, max);
    lc->opt = opt;
    lc->mem_active_size = 0;
}

void lcu_cache_eldest(lcu_cache* pc, String* key, String* val) {
    lcu_lru_item* item = lcu_lru_eldest(pc->lru);
    if (item == NULL) {
        key->str = NULL;
        key->len = 0;
        val->str = NULL;
        val->len = 0;
    } else {
        key->str = lcu_lru_item_key(item);
        key->len = lcu_lru_item_keylen(item);
        val->str = lcu_lru_item_value(item);
        val->len = lcu_lru_item_vallen(item);
    }
}

void lcu_cache_iterator(lcu_cache* pc, String* key, String* val) {
    lcu_lru_item* item = lcu_lru_iterator(pc->lru);
    if (item == NULL) {
        key->str = NULL;
        key->len = 0;
        val->str = NULL;
        val->len = 0;
    } else {
        key->str = lcu_lru_item_key(item);
        key->len = lcu_lru_item_keylen(item);
        val->str = lcu_lru_item_value(item);
        val->len = lcu_lru_item_vallen(item);
    }
}

void lcu_cache_iterator_next(lcu_cache* pc, String* key, String* val) {
    lcu_lru_item* item = lcu_lru_iterator_next(pc->lru);
    if (item == NULL) {
        key->str = NULL;
        key->len = 0;
        val->str = NULL;
        val->len = 0;
    } else {
        key->str = lcu_lru_item_key(item);
        key->len = lcu_lru_item_keylen(item);
        val->str = lcu_lru_item_value(item);
        val->len = lcu_lru_item_vallen(item);
    }
}

void lcu_cache_get(lcu_cache* lc, String key, String* val) {
    lcu_lru_item* item;
    bool pres;
    lcu_map_access(&StrMapType, lc->hmap, (byte*)&key, (byte*)val, &pres);
    if (pres) {
        item = (lcu_lru_item*)val->str;
        lcu_lru_touch(lc->lru, item);
        val->str = lcu_lru_item_value(item);
        val->len = lcu_lru_item_vallen(item);
    }
}

static void destroy_cb(void* p, uint8_t* key, uint32_t klen, uint8_t* val, uint32_t vlen) {
    String skey = {key, klen};
    lcu_cache* lc = (lcu_cache*)p;
    lcu_map_assign(&StrMapType, lc->hmap, (byte*)&skey, nil);
    lcu_slab_free(lc->slab, key);
    lc->mem_active_size -= klen + vlen;
}

bool lcu_cache_put(lcu_cache* lc, String key, String val) {
    lcu_lru_item* item;
    String tmpval;
    byte* newkeystr;
    byte* newvalstr;
    bool pres;
    lcu_map_access(&StrMapType, lc->hmap, (byte*)&key, (byte*)&tmpval, &pres);
    if (pres) {
        lcu_cache_delete(lc, key);
    }
    // slab buffer size 
    size_t slabsize = key.len + val.len;
    if (lc->opt == auto_eject_on && (lc->mem_active_size + slabsize) > lc->slab->mem_limit) {
        lcu_lru_eject_by_size(lc->lru, lc->mem_active_size + slabsize - lc->slab->mem_limit, NULL, NULL);
    }
    void* slab = lcu_slab_alloc(lc->slab, slabsize);
    if (slab == NULL) {
        lcu_lru_eject_by_size(lc->lru, lc->slab->item_max, NULL, NULL);
        slab = lcu_slab_alloc(lc->slab, slabsize);
        if (slab == NULL) return false;
    }
    newkeystr = slab;
    memcpy(newkeystr, key.str, key.len);
    newvalstr = (byte*)slab + key.len;
    memcpy(newvalstr, val.str, val.len);
    item = lcu_lru_insert(lc->lru, (uint8_t*)newkeystr, key.len, 
            (uint8_t*)newvalstr, val.len, destroy_cb, lc);
    if (item == NULL) {
        return false;
    }
    tmpval.str = (byte*)item;
    key.str = newkeystr;
    lcu_map_assign(&StrMapType, lc->hmap, (byte*)&key, (byte*)&tmpval);
    lc->mem_active_size += lcu_lru_item_size(item);
    return true;
}
bool lcu_cache_delete(lcu_cache* lc, String key) {
    lcu_lru_item* item;
    String val;
    bool pres;
    lcu_map_access(&StrMapType, lc->hmap, (byte*)&key, (byte*)&val, &pres);
    if (!pres) {
        return false;
    }
    item = (lcu_lru_item*)val.str;
    lcu_lru_remove_and_destroy(lc->lru, item);
    return true;
}
void lcu_cache_destroy(lcu_cache* lc) {
    // @TODO release correctly(map/slab)
    lcu_lru_destroy(lc->lru);
    free(lc->slab);
}

