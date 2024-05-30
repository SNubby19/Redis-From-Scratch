#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

struct HNode
{
    HNode *next = nullptr;
    uint64_t hcode = 0;
};

struct HTab
{
    HNode **tab = nullptr; // array of HNodes
    size_t mask = 0;       // 2^n - 1
    size_t size = 0;
};

// overarching hashtable interface
struct HMap
{
    HTab ht1; // newer hash table
    HTab ht2; // older hash table
    size_t resizing_pos = 0;
};
// keep 2 copies of the hash table, ht2 is kept empty  for progressive resizing (ie when our ratio breaks the threshold)

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_destroy(HMap *hmap);