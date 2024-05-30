#include "hashtable.h"

static void h_init(HTab *htab, size_t n)
{
    assert(n > 0 && ((n - 1) & n) == 0); // checks if init size is positive and an exact power of 2
    htab->tab = (HNode **)calloc(sizeof(HNode *), n);
    htab->mask = n - 1;
    htab->size = 0;
}

static void h_insert(HTab *htab, HNode *node)
{
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

static HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *))
{
    if (!htab->tab)
    {
        return nullptr;
    }

    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos]; // this is a pointer to the pointer of the element?
    for (HNode *cur; (cur = *from) != nullptr; from = &cur->next)
    {
        if (cur->hcode == key->hcode && eq(cur, key))
        {
            return from;
        }
    }

    return nullptr;
}

static HNode *h_detach(HTab *htab, HNode **from)
{
    HNode *node = *from;
    // from is the pointer to the pointer to the node, so this line just stores the pointer to the node in node while still
    // maintaining access to the node
    // the line below uses the fact that we still have access to the node and set the pointer to the node to the next node

    *from = node->next;
    htab->size--;
    return node;
}

const size_t k_resizing_work = 128; // constant work

static void hm_help_resizing(HMap *hmap)
{
    size_t nwork = 0;
    while (nwork < k_resizing_work && hmap->ht2.size > 0) // dont transer more than 128 keys??
    {
        // scan for nodes from ht2 and move them to ht1
        HNode **from = &hmap->ht2.tab[hmap->resizing_pos];
        if (!*from)
        {
            hmap->resizing_pos++;
            continue;
        }

        h_insert(&hmap->ht1, h_detach(&hmap->ht2, from));
        nwork++;
    }

    if (hmap->ht2.size == 0 && hmap->ht2.tab)
    {
        // done
        free(hmap->ht2.tab);
        hmap->ht2 = HTab{};
    }
}

static void hm_start_resizing(HMap *hmap)
{
    assert(hmap->ht2.tab == NULL);
    hmap->ht2 = hmap->ht1;
    h_init(&hmap->ht1, (hmap->ht1.mask + 1) * 2); // assing the table into a new table with double the size
    hmap->resizing_pos = 0;
}

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, eq);
    from = from ? from : h_lookup(&hmap->ht2, key, eq);
    return from ? *from : nullptr;
}

const size_t k_max_load_factor = 8;

void hm_insert(HMap *hmap, HNode *node)
{
    if (!hmap->ht1.tab)
    {
        h_init(&hmap->ht1, 4); // initialize the table if it is empty;
    }
    h_insert(&hmap->ht1, node); // insert key into the newer table.

    if (!hmap->ht2.tab)
    {
        size_t load_factor = hmap->ht1.size / (hmap->ht1.mask + 1);
        if (load_factor >= k_max_load_factor)
        {
            hm_start_resizing(hmap); // if the load factor is larger than it should be, we create a larger table
        }
    }
    hm_help_resizing(hmap); // this exists to transfer some keys from the old table to the other
}

HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    if (HNode **from = h_lookup(&hmap->ht1, key, eq))
    {
        return h_detach(&hmap->ht1, from);
    }
    if (HNode **from = h_lookup(&hmap->ht2, key, eq))
    {
        return h_detach(&hmap->ht2, from);
    }
    return nullptr;
}

size_t hm_size(HMap *hmap)
{
    return hmap->ht1.size + hmap->ht2.size;
}

void hm_destroy(HMap *hmap)
{
    free(hmap->ht1.tab);
    free(hmap->ht2.tab);
    *hmap = HMap{};
}