// Copyright 2009 The Go Authors. All rights reserved.
// // Use of this source code is governed by a BSD-style
// // license that can be found in the LICENSE file.
#include <libcutil.h>
#include <assert.h>

#define M0 (sizeof(uintptr)==4 ? 2860486313UL : 33054211828000289ULL)
#define M1 (sizeof(uintptr)==4 ? 3267000013UL : 23344194077549503ULL)

static void
strhash(uintptr *h, uintptr s, void *a)
{
    byte *b;
    uintptr hash;
    uintptr len;
    b = ((String*)a)->str;
    len = ((String*)a)->len;
    hash = M0 ^ *h;
    while(len > 0) {
        hash = (hash ^ *b) * M1;
        b++;
        len--;
    }
    *h = hash;
}

static void
strequal(bool *eq, uintptr s, void *a, void *b)
{
    intgo alen;
    byte *s1, *s2;

    alen = ((String*)a)->len;
    if(alen != ((String*)b)->len) {
        *eq = false;
        return;
    }
    s1 = ((String*)a)->str;
    s2 = ((String*)b)->str;
    if(s1 == s2) {
        *eq = true;
        return;
    }
    *eq = memcmp(s1, s2, (size_t)alen) == 0;
}

static void
strcopy(uintptr s, void *a, void *b)
{
    if (b == nil) {
        ((String*)a)->str = 0;
        ((String*)a)->len = 0;
        return;
    }
    ((String*)a)->str = ((String*)b)->str;
    ((String*)a)->len = ((String*)b)->len;
}

static uint32 fastrand;
static uint32
fastrand1(void)
{
    uint32 x;

    x = fastrand;
    x += x;
    if(x & 0x80000000L)
        x ^= 0x88888eefUL;
    fastrand = x;
    return x;
}

Alg StrAlg = { strhash, strequal, strcopy };
Type StrType = { sizeof(String), sizeof(uintptr), &StrAlg };
MapType StrMapType = { &StrType, &StrType };

// Maximum number of key/value pairs a bucket can hold.
#define BUCKETSIZE 8

// Maximum average load of a bucket that triggers growth.
#define LOAD 6.5

// Picking LOAD: too large and we have lots of overflow
// buckets, too small and we waste a lot of space.  I wrote
// a simple program to check some stats for different loads:
// (64-bit, 8 byte keys and values)
//        LOAD    %overflow  bytes/entry     hitprobe    missprobe
//        4.00         2.13        20.77         3.00         4.00
//        4.50         4.05        17.30         3.25         4.50
//        5.00         6.85        14.77         3.50         5.00
//        5.50        10.55        12.94         3.75         5.50
//        6.00        15.27        11.67         4.00         6.00
//        6.50        20.90        10.79         4.25         6.50
//        7.00        27.14        10.15         4.50         7.00
//        7.50        34.03         9.73         4.75         7.50
//        8.00        41.10         9.40         5.00         8.00
//
// %overflow   = percentage of buckets which have an overflow bucket
// bytes/entry = overhead bytes used per key/value pair
// hitprobe    = # of entries to check when looking up a present key
// missprobe   = # of entries to check when looking up an absent key
//
// Keep in mind this data is for maximally loaded tables, i.e. just
// before the table grows.  Typical tables will be somewhat less loaded.

// Maximum key or value size to keep inline (instead of mallocing per element).
// Must fit in a uint8.
// Fast versions cannot handle big values - the cutoff size for
// fast versions in ../../cmd/gc/walk.c must be at most this value.
#define MAXKEYSIZE 128
#define MAXVALUESIZE 128

typedef struct Bucket
{
    uint8  tophash[BUCKETSIZE]; // top 8 bits of hash of each entry (0 = empty)
    struct Bucket *overflow;           // overflow bucket, if any
    byte   data[1];             // BUCKETSIZE keys followed by BUCKETSIZE values
} Bucket;
// NOTE: packing all the keys together and then all the values together makes the
// code a bit more complicated than alternating key/value/key/value/... but it allows
// us to eliminate padding which would be needed for, e.g., map[int64]int8.

// Low-order bit of overflow field is used to mark a bucket as already evacuated
// without destroying the overflow pointer.
// Only buckets in oldbuckets will be marked as evacuated.
// Evacuated bit will be set identically on the base bucket and any overflow buckets.
#define evacuated(b) (((uintptr)(b)->overflow & 1) != 0)
#define overflowptr(b) ((Bucket*)((uintptr)(b)->overflow & ~(uintptr)1))

// Initialize bucket to the empty state.  This only works if BUCKETSIZE==8!
#define clearbucket(b) { *(uint64*)((b)->tophash) = 0; (b)->overflow = nil; }

struct Hmap
{
    uintgo  count;        // # live cells == size of map.  Must be first (used by len() builtin)
    // @TODO no support of indirection pointer: uint32  flags;
    uint8   B;            // log_2 of # of buckets (can hold up to LOAD * 2^B items)
    uint8   keysize;      // key size in bytes
    uint8   valuesize;    // value size in bytes
    uint16  bucketsize;   // bucket size in bytes
    uintptr hash0;        // hash seed
    byte    *buckets;     // array of 2^B Buckets. may be nil if count==0.
    byte    *oldbuckets;  // previous bucket array of half the size, non-nil only when growing
    uintptr nevacuate;    // progress counter for evacuation (buckets less than this have been evacuated)
};

static void
hash_init(MapType *t, Hmap *h, uint32 hint)
{
    uint8 B;
    byte *buckets;
    uintptr i;
    uintptr keysize, valuesize, bucketsize;
    Bucket *b;

    // figure out how big we have to make everything
    keysize = t->key->size;
    assert(keysize <= MAXKEYSIZE);
    valuesize = t->elem->size;
    assert(valuesize <= MAXVALUESIZE);
    bucketsize = offsetof(Bucket, data[0]) + (keysize + valuesize) * BUCKETSIZE;

    // invariants we depend on.  We should probably check these at compile time
    // somewhere, but for now we'll do it here.
    assert(t->key->align <= BUCKETSIZE);
    assert(t->elem->align <= BUCKETSIZE);
    assert(t->key->size % t->key->align == 0);
    assert(t->elem->size % t->elem->align == 0);
    assert(BUCKETSIZE == 8);
    if (sizeof(void*) == 4) {
        assert(t->key->align <= 4);
        assert(t->elem->align <= 4);
    } 

    // find size parameter which will hold the requested # of elements
    B = 0;
    while(hint > BUCKETSIZE && hint > LOAD * ((uintptr)1 << B))
        B++;

    // allocate initial hash table
    // If hint is large zeroing this memory could take a while.
    if(B == 0) {
        // done lazily later.
        buckets = nil;
    } else {
        buckets = malloc(bucketsize << B);
        for(i = 0; i < (uintptr)1 << B; i++) {
            b = (Bucket*)(buckets + i * bucketsize);
            clearbucket(b);
        }
    }

    // initialize Hmap
    // Note: we save all these stores to the end so gciter doesn't see
    // a partially initialized map.
    h->count = 0;
    h->B = B;
    h->keysize = keysize;
    h->valuesize = valuesize;
    h->bucketsize = bucketsize;
    h->hash0 = fastrand1();
    h->buckets = buckets;
    h->oldbuckets = nil;
    h->nevacuate = 0;
}

// Moves entries in oldbuckets[i] to buckets[i] and buckets[i+2^k].
// We leave the original bucket intact, except for the evacuated marks, so that
// iterators can still iterate through the old buckets.
static void
evacuate(MapType *t, Hmap *h, uintptr oldbucket)
{
    Bucket *b;
    Bucket *nextb;
    Bucket *x, *y;
    Bucket *newx, *newy;
    uintptr xi, yi;
    uintptr newbit;
    uintptr hash;
    uintptr i;
    byte *k, *v;
    byte *xk, *yk, *xv, *yv;
    byte *ob;

    b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
    newbit = (uintptr)1 << (h->B - 1);

    if(!evacuated(b)) {
        // TODO: reuse overflow buckets instead of using new ones, if there
        // is no iterator using the old buckets.  (If CanFreeBuckets and !OldIterator.)
        x = (Bucket*)(h->buckets + oldbucket * h->bucketsize);
        y = (Bucket*)(h->buckets + (oldbucket + newbit) * h->bucketsize);
        clearbucket(x);
        clearbucket(y);
        xi = 0;
        yi = 0;
        xk = x->data;
        yk = y->data;
        xv = xk + h->keysize * BUCKETSIZE;
        yv = yk + h->keysize * BUCKETSIZE;
        do {
            for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
                if(b->tophash[i] == 0)
                    continue;
                hash = h->hash0;
                t->key->alg->hash(&hash, t->key->size, k);
                // NOTE: if key != key, then this hash could be (and probably will be)
                // entirely different from the old hash.  We effectively only update
                // the B'th bit of the hash in this case.
                if((hash & newbit) == 0) {
                    if(xi == BUCKETSIZE) {
                        newx = malloc(h->bucketsize);
                        clearbucket(newx);
                        x->overflow = newx;
                        x = newx;
                        xi = 0;
                        xk = x->data;
                        xv = xk + h->keysize * BUCKETSIZE;
                    }
                    x->tophash[xi] = b->tophash[i];
                    t->key->alg->copy(t->key->size, xk, k); // copy value
                    t->elem->alg->copy(t->elem->size, xv, v);
                    xi++;
                    xk += h->keysize;
                    xv += h->valuesize;
                } else {
                    if(yi == BUCKETSIZE) {
                        newy = malloc(h->bucketsize);
                        clearbucket(newy);
                        y->overflow = newy;
                        y = newy;
                        yi = 0;
                        yk = y->data;
                        yv = yk + h->keysize * BUCKETSIZE;
                    }
                    y->tophash[yi] = b->tophash[i];
                    t->key->alg->copy(t->key->size, yk, k);
                    t->elem->alg->copy(t->elem->size, yv, v);
                    yi++;
                    yk += h->keysize;
                    yv += h->valuesize;
                }
            }

            // mark as evacuated so we don't do it again.
            // this also tells any iterators that this data isn't golden anymore.
            nextb = b->overflow;
            b->overflow = (Bucket*)((uintptr)nextb + 1);

            b = nextb;
        } while(b != nil);

        b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
        while((nextb = overflowptr(b)) != nil) {
            b->overflow = nextb->overflow;
            free(nextb);
        }
    }

    // advance evacuation mark
    if(oldbucket == h->nevacuate) {
        h->nevacuate = oldbucket + 1;
        if(oldbucket + 1 == newbit) { // newbit == # of oldbuckets
            // free main bucket array
            ob = h->oldbuckets;
            h->oldbuckets = nil;
            free(ob);
        }
    }
}

static void
grow_work(MapType *t, Hmap *h, uintptr bucket)
{
    uintptr noldbuckets;

    noldbuckets = (uintptr)1 << (h->B - 1);

    // make sure we evacuate the oldbucket corresponding
    // to the bucket we're about to use
    evacuate(t, h, bucket & (noldbuckets - 1));

    // evacuate one more oldbucket to make progress on growing
    if(h->oldbuckets != nil)
        evacuate(t, h, h->nevacuate);
}

static void
hash_grow(MapType *t, Hmap *h)
{
    byte *old_buckets;
    byte *new_buckets;

    // allocate a bigger hash table
    assert(h->oldbuckets == nil);
    old_buckets = h->buckets;
    // NOTE: this could be a big malloc, but since we don't need zeroing it is probably fast.
    new_buckets = malloc((uintptr)h->bucketsize << (h->B + 1));

    // commit the grow (atomic wrt gc)
    h->B++;
    h->oldbuckets = old_buckets;
    h->buckets = new_buckets;
    h->nevacuate = 0;

}

// returns ptr to value associated with key *keyp, or nil if none.
// if it returns non-nil, updates *keyp to point to the currently stored key.
static byte*
hash_lookup(MapType *t, Hmap *h, byte **keyp)
{
    void *key;
    uintptr hash;
    uintptr bucket, oldbucket;
    Bucket *b;
    uint8 top;
    uintptr i;
    bool eq;
    byte *k, *v;

    key = *keyp;
    if(h->count == 0)
        return nil;
    hash = h->hash0;
    t->key->alg->hash(&hash, t->key->size, key);
    bucket = hash & (((uintptr)1 << h->B) - 1);
    if(h->oldbuckets != nil) {
        oldbucket = bucket & (((uintptr)1 << (h->B - 1)) - 1);
        b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
        if(evacuated(b)) {
            b = (Bucket*)(h->buckets + bucket * h->bucketsize);
        }
    } else {
        b = (Bucket*)(h->buckets + bucket * h->bucketsize);
    }
    top = hash >> (sizeof(uintptr)*8 - 8);
    if(top == 0)
        top = 1;
    do {
        for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
            if(b->tophash[i] == top) {
                t->key->alg->equal(&eq, t->key->size, key, k);
                if(eq) {
                    *keyp = k;
                    return v;
                }
            }
        }
        b = b->overflow;
    } while(b != nil);
    return nil;
}

static void
hash_insert(MapType *t, Hmap *h, void *key, void *value)
{
    uintptr hash;
    uintptr bucket;
    uintptr i;
    bool eq;
    Bucket *b;
    Bucket *newb;
    uint8 *inserti;
    byte *insertk, *insertv;
    uint8 top;
    byte *k, *v;

    hash = h->hash0;
    t->key->alg->hash(&hash, t->key->size, key);
    if(h->buckets == nil) {
        h->buckets = malloc(h->bucketsize);
        b = (Bucket*)(h->buckets);
        clearbucket(b);
    }

again:
    bucket = hash & (((uintptr)1 << h->B) - 1);
    if(h->oldbuckets != nil)
        grow_work(t, h, bucket);
    b = (Bucket*)(h->buckets + bucket * h->bucketsize);
    top = hash >> (sizeof(uintptr)*8 - 8);
    if(top == 0)
            top = 1;
    inserti = 0;
    insertk = nil;
    insertv = nil;
    while(true) {
        for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
            if(b->tophash[i] != top) {
                if(b->tophash[i] == 0 && inserti == nil) {
                    inserti = &b->tophash[i];
                    insertk = k;
                    insertv = v;
                }
                continue;
            }
            t->key->alg->equal(&eq, t->key->size, key, k);
            if(!eq)
                continue;
            // already have a mapping for key.  Update it.
            t->key->alg->copy(t->key->size, k, key); // Need to update key for keys which are distinct but equal (e.g. +0.0 and -0.0)
            t->elem->alg->copy(t->elem->size, v, value);
            return;
        }
        if(b->overflow == nil)
                break;
        b = b->overflow;
    }

    // did not find mapping for key.  Allocate new cell & add entry.
    if(h->count >= LOAD * ((uintptr)1 << h->B) && h->count >= BUCKETSIZE) {
        hash_grow(t, h);
        goto again; // Growing the table invalidates everything, so try again
    }

    if(inserti == nil) {
        // all current buckets are full, allocate a new one.
        newb = malloc(h->bucketsize);
        clearbucket(newb);
        b->overflow = newb;
        inserti = newb->tophash;
        insertk = newb->data;
        insertv = insertk + h->keysize * BUCKETSIZE;
    }
    t->key->alg->copy(t->key->size, insertk, key);
    t->elem->alg->copy(t->elem->size, insertv, value);
    *inserti = top;
    h->count++;
}

static void
hash_remove(MapType *t, Hmap *h, void *key)
{
    uintptr hash;
    uintptr bucket;
    Bucket *b;
    uint8 top;
    uintptr i;
    byte *k;
    bool eq;
    
    if(h->count == 0)
        return;
    hash = h->hash0;
    t->key->alg->hash(&hash, t->key->size, key);
    bucket = hash & (((uintptr)1 << h->B) - 1);
    if(h->oldbuckets != nil)
        grow_work(t, h, bucket);
    b = (Bucket*)(h->buckets + bucket * h->bucketsize);
    top = hash >> (sizeof(uintptr)*8 - 8);
    if(top == 0)
        top = 1;
    do {
        for(i = 0, k = b->data; i < BUCKETSIZE; i++, k += h->keysize) {
            if(b->tophash[i] != top)
                continue;
            t->key->alg->equal(&eq, t->key->size, key, k);
            if(!eq)
                continue;

            b->tophash[i] = 0;
            h->count--;
            
            // TODO: consolidate buckets if they are mostly empty
            // can only consolidate if there are no live iterators at this size.
            return;
        }
        b = b->overflow;
    } while(b != nil);
}

Hmap*
lcu_map_init(MapType *typ, int64 hint)
{
    Hmap *h;

    assert(hint >= 0 && (int32)hint == hint);

    h = malloc(sizeof(*h));

    hash_init(typ, h, hint);

    // these calculations are compiler dependent.
    // figure out offsets of map call arguments.

    return h;
}

void
lcu_map_access(MapType *t, Hmap *h, byte *ak, byte *av, bool *pres)
{
    byte *res;
    Type *elem;

    elem = t->elem;
    if(h == nil || h->count == 0) {
        elem->alg->copy(elem->size, av, nil);
        *pres = false;
        return;
    }

    res = hash_lookup(t, h, &ak);

    if(res != nil) {
        *pres = true;
        elem->alg->copy(elem->size, av, res);
    } else {
        *pres = false;
        elem->alg->copy(elem->size, av, nil);
    }
}

void
lcu_map_assign(MapType *t, Hmap *h, byte *ak, byte *av)
{
    assert(h != nil);

    if(av == nil) {
            hash_remove(t, h, ak);
    } else {
            hash_insert(t, h, ak, av);
    }
}

uintgo 
lcu_map_count(Hmap* h) {
    return h->count;
}
