#include "cg_hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

/* The hashmap is unordered.
   As long as no item is deleted the insertion order is kept
 */

#define PERTURB_SHIFT 5
#define MAP_SIZE(map) ((map)->table_size)
#if SIZEOF_VOID_P > 4
#define MAP_IXSIZE(map)                          \
    (MAP_SIZE(map) <= 0xff ?                     \
        1 : MAP_SIZE(map) <= 0xffff ?            \
            2 : MAP_SIZE(map) <= 0xffffffff ?    \
                4 : sizeof(int64_t))
#else
#define MAP_IXSIZE(map)                          \
    (MAP_SIZE(map) <= 0xff ?                     \
        1 : MAP_SIZE(map) <= 0xffff ?            \
            2 : sizeof(int32_t))
#endif
#define MAP_ENTRIES(map) \
    ((cgns_hashmap_entry*)(&((int8_t*)((map)->map_indices))[MAP_SIZE(map) * MAP_IXSIZE(map)]))

#define MAP_MASK(map) (((map)->table_size)-1)
#define IS_POWER_OF_2(x) (((x) & (x-1)) == 0)


/* lookup indices.  returns MAPIX_EMPTY, MAPIX_DUMMY, or ix >=0 */
static inline map_ssize_t
cgi_hashmap_get_index(const cgns_hashmap_keyobject *keys, map_ssize_t i)
{
    map_ssize_t s = MAP_SIZE(keys);
    map_ssize_t ix;

    if (s <= 0xff) {
        const int8_t *indices = (const int8_t*)(keys->map_indices);
        ix = indices[i];
    }
    else if (s <= 0xffff) {
        const int16_t *indices = (const int16_t*)(keys->map_indices);
        ix = indices[i];
    }
#if SIZEOF_VOID_P > 4
    else if (s > 0xffffffff) {
        const int64_t *indices = (const int64_t*)(keys->map_indices);
        ix = indices[i];
    }
#endif
    else {
        const int32_t *indices = (const int32_t*)(keys->map_indices);
        ix = indices[i];
    }
    assert(ix >= MAPIX_DUMMY);
    return ix;
}

/* write to indices. */
static inline void
cgi_hashmap_set_index(cgns_hashmap_keyobject *keys, map_ssize_t i, map_ssize_t ix)
{
    map_ssize_t s = MAP_SIZE(keys);

    assert(ix >= MAPIX_DUMMY);

    if (s <= 0xff) {
        int8_t *indices = (int8_t*)(keys->map_indices);
        assert(ix <= 0x7f);
        indices[i] = (char)ix;
    }
    else if (s <= 0xffff) {
        int16_t *indices = (int16_t*)(keys->map_indices);
        assert(ix <= 0x7fff);
        indices[i] = (int16_t)ix;
    }
#if SIZEOF_VOID_P > 4
    else if (s > 0xffffffff) {
        int64_t *indices = (int64_t*)(keys->map_indices);
        indices[i] = ix;
    }
#endif
    else {
        int32_t *indices = (int32_t*)(keys->map_indices);
        assert(ix <= 0x7fffffff);
        indices[i] = (int32_t)ix;
    }
}

#if SIZEOF_MAP_USIZE_T == 4
#define _fnvprefix 0x811c9dc5
#define _fnvmult 0x01000193
#elif SIZEOF_MAP_USIZE_T == 8
#define _fnvprefix 0xcbf29ce484222325
#define _fnvmult 0x00000100000001B3
#else
#define _fnvprefix 0
#define _fnvmult 0
#endif

static map_ssize_t
cgi_hash_cstr(const char* a)
{
    const unsigned char* p = (unsigned char *) a;
    map_ssize_t len;
    map_usize_t x;
    map_ssize_t remainder, blocks;
    union {
        map_usize_t value;
        unsigned char bytes[SIZEOF_MAP_USIZE_T];
    } block;
    
    len = (map_ssize_t) strlen(a);
    if (len == 0) {
        return 0;
    }
    
    remainder = len % SIZEOF_MAP_USIZE_T;
    if (remainder == 0) {
        remainder = SIZEOF_MAP_USIZE_T;
    }
    blocks = (len - remainder) / SIZEOF_MAP_USIZE_T;
    x = (map_usize_t) _fnvprefix;
    x ^= (map_usize_t) *p << 7;
    while (blocks--) {
        memcpy(block.bytes, p, SIZEOF_MAP_USIZE_T);
        x = (_fnvmult * x) ^ block.value;
        p += SIZEOF_MAP_USIZE_T;
    }
    /* add remainder */
    for (; remainder > 0; remainder--)
        x = (_fnvmult * x) ^ (map_usize_t)*p++;
    x ^= (map_usize_t) len;
    /* no suffix */
    if (x == (map_usize_t)-1)
        x = (map_usize_t)-2;
    return x;
}

#define USABLE_FRACTION(n) (((n) << 1)/3)

static const unsigned int BitLengthTable[32] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

unsigned int _bit_length(unsigned long d) {
   unsigned int d_bits = 0;
   while (d >= 32) {
       d_bits += 6;
       d >>= 6;
   }
   d_bits += BitLengthTable[d];
   return d_bits;
}

/* Find the smallest table_size >= minsize. */
static inline map_ssize_t
cgi_calculate_keysize(map_ssize_t minsize)
{
#if SIZEOF_LONG == SIZEOF_VOID_P
    minsize = (minsize | MAP_MINSIZE) - 1;
    return 1LL << _bit_length(minsize | (MAP_MINSIZE-1));
#elif defined(_MSC_VER)
    // On 64bit Windows, sizeof(long) == 4.
    minsize = (minsize | MAP_MINSIZE) - 1;
    unsigned long msb;
    _BitScanReverse64(&msb, (uint64_t)minsize);
    return 1LL << (msb + 1);
#else
    map_ssize_t size;
    for (size = MAP_MINSIZE;
            size < minsize && size > 0;
            size <<= 1)
        ;
    return size;
#endif
}


/* estimate_keysize is reverse function of USABLE_FRACTION.
 *
 * This can be used to reserve enough size to insert n entries without
 * resizing.
 */
static inline map_ssize_t
cgi_estimate_keysize(map_ssize_t n)
{
    return cgi_calculate_keysize((n*3 + 1) / 2);
}

/* GROWTH_RATE. Growth rate upon hitting maximum load.
 * Currently set to used*2.
 * This means that hashtable double in size when growing without deletions,
 * but have more head room when the number of deletions is on a par with the
 * number of insertions. 
 *
 */
#define GROWTH_RATE(d) ((d)->ma_used*2)

/* This immutable, empty cgns_hashmap_keyobject is used for HashMap_Clear()
 * (which cannot fail and thus can do no allocation).
 */
static cgns_hashmap_keyobject empty_keys_struct = {
        1, /* table_size */
        0, /* map_usable (immutable) */
        0, /* map_nentries */
        {MAPIX_EMPTY, MAPIX_EMPTY, MAPIX_EMPTY, MAPIX_EMPTY,
         MAPIX_EMPTY, MAPIX_EMPTY, MAPIX_EMPTY, MAPIX_EMPTY}, /* map_indices */
};

#define MAP_EMPTY_KEYS &empty_keys_struct

static cgns_hashmap_keyobject*
cgi_new_keys_object(map_ssize_t size)
{
    cgns_hashmap_keyobject *keymap;
    map_ssize_t es, usable;

    assert(size >= MAP_MINSIZE);
    assert(IS_POWER_OF_2(size));

    usable = USABLE_FRACTION(size);
    if (size <= 0xff) {
        es = 1;
    }
    else if (size <= 0xffff) {
        es = 2;
    }
#if SIZEOF_VOID_P > 4
    else if (size <= 0xffffffff) {
        es = 4;
    }
#endif
    else {
        es = sizeof(map_ssize_t);
    }

    keymap = malloc(sizeof(cgns_hashmap_keyobject)
                         + es * size
                         + sizeof(cgns_hashmap_entry) * usable);
    if (keymap == NULL) {
        return NULL;
    }
    keymap->table_size = size;
    keymap->map_usable = usable;
    keymap->map_nentries = 0;
    memset(&keymap->map_indices[0], 0xff, es * size);
    //memset(&keymap->map_indices[0], MAPIX_EMPTY, es * size);
    memset(MAP_ENTRIES(keymap), 0, sizeof(cgns_hashmap_entry) * usable);
    // Set to false the index value;
    cgns_hashmap_entry* ep = MAP_ENTRIES(keymap);
    for (map_ssize_t i = 0; i < usable; i++) {
        ep->me_value = -1;
        ep++;
    }
    return keymap;
}


/* Consumes a reference to the keys object */
static cgns_hashmap_object *
cgi_allocate_hashmap(cgns_hashmap_keyobject *keys)
{
    cgns_hashmap_object *mp;
    assert(keys != NULL);
    
	mp = (cgns_hashmap_object *) malloc(sizeof(cgns_hashmap_object));
	if (mp == NULL) {
        return NULL;
    }
    
    mp->ma_keys = keys;
    mp->ma_used = 0;

    return mp;
}


cgns_hashmap_object *
cgi_new_hashmap(void)
{
	return cgi_allocate_hashmap(MAP_EMPTY_KEYS);
}

cgns_hashmap_object *
cgi_new_presized_hashmap(map_ssize_t minused)
{
    const map_ssize_t max_presize = 128 * 1024;
    map_ssize_t newsize;
    cgns_hashmap_keyobject *new_keys;

    if (minused <= USABLE_FRACTION(MAP_MINSIZE)) {
        return cgi_new_hashmap();
    }
    /* There are no strict guarantee that returned hashmap can contain minused
     * items without resize.  So we create medium size table instead of very
     * large table.
     */
    if (minused > USABLE_FRACTION(max_presize)) {
        newsize = max_presize;
    }
    else {
        newsize = cgi_estimate_keysize(minused);
    }

    new_keys = cgi_new_keys_object(newsize);
    if (new_keys == NULL)
        return NULL;
    return cgi_allocate_hashmap(new_keys);
}

/* Search index of hash table from offset of entry table */
static map_ssize_t
cgi_index_lookup(cgns_hashmap_keyobject *k, map_ssize_t hash, map_ssize_t index)
{
    size_t mask = MAP_MASK(k);
    size_t perturb = (size_t)hash;
    size_t i = (size_t)hash & mask;

    for (;;) {
        map_ssize_t ix = cgi_hashmap_get_index(k, i);
        if (ix == index) {
            return i;
        }
        if (ix == MAPIX_EMPTY) {
            return MAPIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    // Not reachable ...
}

/* Search index of hash table from hash and name key
 * It also retrieves the pointed cgns node
 */
static map_ssize_t 
cgi_name_lookup(cgns_hashmap_object *mp, const char *key,
                 map_ssize_t hash, map_ssize_t *value_addr)
{   
    cgns_hashmap_entry *ep0 = MAP_ENTRIES(mp->ma_keys); // compact storage
	//
    size_t mask = MAP_MASK(mp->ma_keys);
    size_t perturb = (size_t)hash;
    size_t i = (size_t)hash & mask;

    for (;;) {
        map_ssize_t ix = cgi_hashmap_get_index(mp->ma_keys, i);
        if (ix == MAPIX_EMPTY) {
            *value_addr = -1;
            return MAPIX_EMPTY;
        }
        if (ix >= 0) {
            cgns_hashmap_entry *ep = &ep0[ix];
            if (ep->me_hash == hash && strcmp(ep->me_key, key) == 0) {
                *value_addr = ep->me_value;
                return ix;
            }
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    // unreachable
}

/* Internal function to find slot for an item from its hash
   when it is known that the key is not present in the hashtable. */
static map_ssize_t
cgi_find_empty_slot(cgns_hashmap_keyobject *keys, map_ssize_t hash)
{
    assert(keys != NULL);
    if (keys == NULL) { return -1; }
    const size_t mask = MAP_MASK(keys);
    size_t i = hash & mask;
    map_ssize_t ix = cgi_hashmap_get_index(keys, i);
    for (size_t perturb = hash; ix >= 0;) {
        perturb >>= PERTURB_SHIFT;
        i = (i*5 + perturb + 1) & mask;
        ix = cgi_hashmap_get_index(keys, i);
    }
    return i;
}

/*
Internal routine used by cgi_resize_hashmap() to build a hashtable of entries.
*/
static void
cgi_build_indices(cgns_hashmap_keyobject* keys, cgns_hashmap_entry* ep, map_ssize_t n)
{
    size_t mask = (size_t)MAP_SIZE(keys) - 1;
    for (map_ssize_t ix = 0; ix != n; ix++, ep++) {
        map_ssize_t hash = ep->me_hash;
        size_t i = hash & mask;
        for (size_t perturb = hash; cgi_hashmap_get_index(keys, i) != MAPIX_EMPTY;) {
            perturb >>= PERTURB_SHIFT;
            i = mask & (i * 5 + perturb + 1);
        }
        cgi_hashmap_set_index(keys, i, ix);
    }
}


/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
*/
static int
cgi_resize_hashmap(cgns_hashmap_object* mp, map_ssize_t newsize)
{
    map_ssize_t numentries;
    cgns_hashmap_keyobject* oldkeys;
    cgns_hashmap_entry* oldentries, * newentries;

    if (newsize <= 0) {
        //set message with memory error
        return -1;
    }
    assert(IS_POWER_OF_2(newsize));
    assert(newsize >= MAP_MINSIZE);

    oldkeys = mp->ma_keys;

    /* Allocate a new table. */
    mp->ma_keys = cgi_new_keys_object(newsize);
    if (mp->ma_keys == NULL) {
        mp->ma_keys = oldkeys;
        return -1;
    }
    // New table must be large enough.
    assert(mp->ma_keys->map_usable >= mp->ma_used);

    numentries = mp->ma_used;
    oldentries = MAP_ENTRIES(oldkeys);
    newentries = MAP_ENTRIES(mp->ma_keys);

    if (oldkeys->map_nentries == numentries) {
        memcpy(newentries, oldentries, numentries * sizeof(cgns_hashmap_entry));
    }
    else {
        cgns_hashmap_entry* ep = oldentries;
        for (map_ssize_t i = 0; i < numentries; i++) {
            while (ep->me_value == -1)
                ep++;
            newentries[i].me_hash = ep->me_hash;
            strcpy(newentries[i].me_key, ep->me_key);
            newentries[i].me_value = ep->me_value;
            ep++;
        }
    }
    free(oldkeys);

    cgi_build_indices(mp->ma_keys, newentries, numentries);
    mp->ma_keys->map_usable -= numentries;
    mp->ma_keys->map_nentries = numentries;
    return 0;
}

static int
cgi_insertion_resize(cgns_hashmap_object *mp)
{
    return cgi_resize_hashmap(mp, cgi_calculate_keysize(GROWTH_RATE(mp)));
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Returns -1 if an error occurred, or 0 on success.
*/
static int
cgi_insert_key(cgns_hashmap_object *mp, const char *key, map_ssize_t hash, map_ssize_t value)
{
    map_ssize_t old_value = -1;

    map_ssize_t ix = cgi_name_lookup(mp, key, hash, &old_value);
    if (ix == MAPIX_ERROR)
        return -1;

    if (ix == MAPIX_EMPTY) {
        /* Insert into new slot. */
        assert(old_value == -1);
        if (mp->ma_keys->map_usable <= 0) {
            /* Need to resize. */
            if (cgi_insertion_resize(mp) < 0)
                return -1;
        }
        map_ssize_t hashpos = cgi_find_empty_slot(mp->ma_keys, hash);
        cgns_hashmap_entry *ep;
        ep = &MAP_ENTRIES(mp->ma_keys)[mp->ma_keys->map_nentries];
        cgi_hashmap_set_index(mp->ma_keys, hashpos, mp->ma_keys->map_nentries);
        strcpy(ep->me_key, key);
        ep->me_hash = hash;
        ep->me_value = value;
		
        mp->ma_used++;
        mp->ma_keys->map_usable--;
        mp->ma_keys->map_nentries++;
        assert(mp->ma_keys->map_usable >= 0);

        return 0;
    }

    if (old_value != value) {
        assert(old_value != -1);
        MAP_ENTRIES(mp->ma_keys)[ix].me_value = value;
    }
    
    return 0;
}

// Same to insert but specialized for ma_keys = MAP_EMPTY_KEYS.
static int
cgi_insert_to_emptymap(cgns_hashmap_object *mp, const char *key, map_ssize_t hash, map_ssize_t value)
{
    assert(mp->ma_keys == MAP_EMPTY_KEYS);

    cgns_hashmap_keyobject *newkeys = cgi_new_keys_object(MAP_MINSIZE);
    if (newkeys == NULL) {
        return -1;
    }
    
    mp->ma_keys = newkeys;
    
    size_t hashpos = (size_t)hash & (MAP_MINSIZE-1);
    cgns_hashmap_entry *ep = MAP_ENTRIES(mp->ma_keys);
    cgi_hashmap_set_index(mp->ma_keys, hashpos, 0);
    strcpy(ep->me_key, key);
    ep->me_hash = hash;
    ep->me_value = value;
    mp->ma_used++;
	
    mp->ma_keys->map_usable--;
    mp->ma_keys->map_nentries++;
    return 0;
}

map_ssize_t
cgi_map_get_item(cgns_hashmap_object *op, const char *key)
{
    map_ssize_t ix;
    map_ssize_t hash;
    map_ssize_t value;

    if (op == NULL) {
        return -1;
    }
    
    hash = cgi_hash_cstr(key);
    if (hash == -1) {
        return -1;
    }

    ix = cgi_name_lookup(op, key, hash, &value);
    if (ix < 0)
        return -1;
    return value;
}

int
cgi_map_set_item(cgns_hashmap_object *op, const char *key, map_ssize_t value)
{
    map_ssize_t hash;
    if (op == NULL) {
        return -1;
    }
    assert(key);
    assert(value != -1);
    hash = cgi_hash_cstr(key);
    if (hash == -1){
       return -1;
    }

    if (op->ma_keys == MAP_EMPTY_KEYS) {
        return cgi_insert_to_emptymap(op, key, hash, value);
    }
    /* cgi_insert_key() handles any resizing that might be necessary */
    return cgi_insert_key(op, key, hash, value);
}


/* Return 1 if `key` is in hashmap `op`, 0 if not, and -1 on error. */
int
cgi_map_contains(cgns_hashmap_object *op, const char *key)
{
    map_ssize_t hash;
    map_ssize_t ix;
    map_ssize_t value;

    hash = cgi_hash_cstr(key);
    if (hash == -1){
        return -1;
    }
    ix = cgi_name_lookup(op, key, hash, &value);
    if (ix == MAPIX_ERROR)
        return -1;
    return (ix != MAPIX_EMPTY && value != -1);
}

void cgi_hashmap_clear(cgns_hashmap_object* op)
{
    cgns_hashmap_keyobject * oldkeys;

    if (op == NULL)
        return;

    oldkeys = op->ma_keys;
    /* Empty the hashtable... */

    op->ma_keys = MAP_EMPTY_KEYS;
    op->ma_used = 0;

    free(oldkeys);
}



int
_cg_del_shift_item_known_hash(cgns_hashmap_object* op, const char* key, map_ssize_t hash)
{
    map_ssize_t ix;
    cgns_hashmap_entry* ep;
    map_ssize_t old_value;

    if (op == NULL) {
        //error bad memory
        return -1;
    }
    assert(key);
    assert(hash != -1);
    
    ix = cgi_name_lookup(op, key, hash, &old_value);
    if (ix == MAPIX_ERROR)
        return -1;
    if (ix == MAPIX_EMPTY || old_value == -1) {
        // set error bad key
        return -1;
    }

    map_ssize_t hashpos = cgi_index_lookup(op->ma_keys, hash, ix);
    assert(hashpos >= 0);

    op->ma_used--;
    ep = &MAP_ENTRIES(op->ma_keys)[ix];
    cgi_hashmap_set_index(op->ma_keys, hashpos, MAPIX_DUMMY);;
    strcpy(ep->me_key, "\0");
    ep->me_value = -1;
    /* Shift down upper indices */
    ep = MAP_ENTRIES(op->ma_keys);
    for (map_ssize_t i = 0; i < op->ma_keys->map_usable; i++) {
        if ( ep->me_value > old_value ) {
            ep->me_value--;
        }
        ep++;
    }

    return 0;
}

int
cgi_map_del_shift_item(cgns_hashmap_object* op, const char* key)
{
    map_ssize_t hash;
    assert(key);
    hash = cgi_hash_cstr(key);
    if (hash == -1) {
        return -1;
    }
    return _cg_del_shift_item_known_hash(op, key, hash);
}
