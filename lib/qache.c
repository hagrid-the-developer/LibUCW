/*
 *      Simple and Quick Shared Memory Cache
 *
 *	(c) 2005 Martin Mares <mj@ucw.cz>
 */

#include "lib/lib.h"
#include "lib/fastbuf.h"
#include "lib/qache.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/user.h>

/*
 *  On-disk format:
 *	qache_header
 *	qache_entry[max_entries]	table of entries and their keys
 *	u32 qache_hash[hash_size]	hash table pointing to keys
 *	padding				to a multiple of block size
 *	blocks[]			data blocks, each block starts with u32 next_ptr
 */

struct qache_header {
  u32 magic;				/* QCACHE_MAGIC */
  u32 block_size;			/* Parameters as in qache_params */
  u32 block_shift;			/* block_size = 1 << block_shift */
  u32 num_blocks;
  u32 format_id;
  u32 entry_table_start;		/* Array of qache_entry's */
  u32 max_entries;
  u32 hash_table_start;			/* Hash table containing all keys */
  u32 hash_size;
  u32 first_data_block;
};

#define QACHE_MAGIC 0xb79f6d12

struct qache_entry {
  u32 lru_prev, lru_next;		/* Entry #0: head of the cyclic LRU list */
  u32 data_len;				/* Entry #0: number of free blocks, Free entries: ~0U */
  u32 first_data_block;			/* Entry #0: first free block */
  qache_key_t key;
  u32 hash_next;			/* Entry #0: first free entry, Free entries: next free */
};

struct qache {
  struct qache_header *hdr;
  struct qache_entry *entry_table;
  u32 *hash_table;
  int fd;
  byte *mmap_data;
  uns file_size;
  byte *file_name;
  uns locked;
  uns data_block_size;
};

#define first_free_entry entry_table[0].hash_next
#define first_free_block entry_table[0].first_data_block
#define num_free_blocks entry_table[0].data_len

static int
qache_open_existing(struct qache *q, struct qache_params *par)
{
  if ((q->fd = open(q->file_name, O_RDWR, 0)) < 0)
    return 0;

  struct stat st;
  byte *err = "stat failed";
  if (fstat(q->fd, &st) < 0)
    goto close_and_fail;

  err = "invalid file size";
  if (st.st_size < (int)sizeof(struct qache_header) || (st.st_size % par->block_size))
    goto close_and_fail;
  q->file_size = st.st_size;

  err = "requested size change";
  if (q->file_size != par->cache_size)
    goto close_and_fail;

  err = "cannot mmap";
  if ((q->mmap_data = mmap(NULL, q->file_size, PROT_READ | PROT_WRITE, MAP_SHARED, q->fd, 0)) == MAP_FAILED)
    goto close_and_fail;
  struct qache_header *h = (struct qache_header *) q->mmap_data;

  err = "incompatible format";
  if (h->magic != QACHE_MAGIC ||
      h->block_size != par->block_size ||
      h->format_id != par->format_id)
    goto unmap_and_fail;

  err = "incomplete file";
  if (h->num_blocks*h->block_size != q->file_size)
    goto unmap_and_fail;

  /* FIXME: Audit cache file contents */

  log(L_INFO, "Cache %s: using existing data", q->file_name);
  return 1;

 unmap_and_fail:
  munmap(q->mmap_data, q->file_size);
 close_and_fail:
  log(L_INFO, "Cache %s: ignoring old contents (%s)", q->file_name, err);
  close(q->fd);
  return 0;
}

static void
qache_create(struct qache *q, struct qache_params *par)
{
  q->fd = open(q->file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (q->fd < 0)
    die("Cache %s: unable to create (%m)", q->file_name);
  struct fastbuf *fb = bfdopen_shared(q->fd, 16384);

  struct qache_header h;
  bzero(&h, sizeof(h));
  h.magic = QACHE_MAGIC;
  h.block_size = par->block_size;
  h.block_shift = fls(h.block_size);
  h.num_blocks = par->cache_size / par->block_size;
  h.format_id = par->format_id;
  h.entry_table_start = sizeof(h);
  h.max_entries = h.num_blocks;
  h.hash_table_start = h.entry_table_start + (h.max_entries+1) * sizeof(struct qache_entry);
  h.hash_size = 1;
  while (h.hash_size < h.max_entries)
    h.hash_size *= 2;
  h.first_data_block = (h.hash_table_start + h.hash_size * 4 + h.block_size - 1) >> h.block_shift;
  if (h.first_data_block >= h.num_blocks)
    die("Cache %s: Requested size is too small even to hold the maintenance structures", q->file_name);
  bwrite(fb, &h, sizeof(h));

  /* Entry #0: heads of all lists */
  ASSERT(btell(fb) == h.entry_table_start);
  struct qache_entry ent;
  bzero(&ent, sizeof(ent));
  ent.first_data_block = h.first_data_block;
  ent.data_len = h.num_blocks - h.first_data_block;
  ent.hash_next = 1;
  bwrite(fb, &ent, sizeof(ent));

  /* Other entries */
  bzero(&ent, sizeof(ent));
  ent.data_len = ~0U;
  for (uns i=1; i<=h.max_entries; i++)
    {
      ent.hash_next = (i == h.max_entries ? 0 : i+1);
      bwrite(fb, &ent, sizeof(ent));
    }

  /* The hash table */
  ASSERT(btell(fb) == h.hash_table_start);
  for (uns i=0; i<h.hash_size; i++)
    bputl(fb, 0);

  /* Padding */
  ASSERT(btell(fb) <= h.first_data_block << h.block_shift);
  while (btell(fb) < h.first_data_block << h.block_shift)
    bputc(fb, 0);

  /* Data blocks */
  for (uns i=h.first_data_block; i<h.num_blocks; i++)
    {
      bputl(fb, (i == h.num_blocks-1 ? 0 : i+1));
      for (uns i=4; i<h.block_size; i+=4)
	bputl(fb, 0);
    }

  ASSERT(btell(fb) == par->cache_size);
  bclose(fb);
  log(L_INFO, "Cache %s: created (%d bytes, %d slots, %d buckets)", q->file_name, par->cache_size, h.max_entries, h.hash_size);

  if ((q->mmap_data = mmap(NULL, par->cache_size, PROT_READ | PROT_WRITE, MAP_SHARED, q->fd, 0)) == MAP_FAILED)
    die("Cache %s: mmap failed (%m)", par->cache_size);
  q->file_size = par->cache_size;
}

struct qache *
qache_open(struct qache_params *par)
{
  struct qache *q = xmalloc_zero(sizeof(*q));
  q->file_name = xstrdup(par->file_name);

  ASSERT(par->block_size >= 8 && !(par->block_size & (par->block_size-1)));
  par->cache_size = ALIGN(par->cache_size, par->block_size);

  if (par->force_reset <= 0 && qache_open_existing(q, par))
    ;
  else if (par->force_reset < 0)
    die("Cache %s: read-only access requested, but no data available");
  else
    qache_create(q, par);

  /* FIXME: Remember `closed correctly' status */

  q->hdr = (struct qache_header *) q->mmap_data;
  q->entry_table = (struct qache_entry *) (q->mmap_data + q->hdr->entry_table_start);
  q->hash_table = (u32 *) (q->mmap_data + q->hdr->hash_table_start);
  q->data_block_size = q->hdr->block_size - 4;
  return q;
}

void
qache_close(struct qache *q, uns retain_data)
{
  munmap(q->mmap_data, q->file_size);
  close(q->fd);
  if (!retain_data && unlink(q->file_name) < 0)
    log(L_ERROR, "Cache %s: unlink failed (%m)", q->file_name);
  xfree(q->file_name);
  xfree(q);
}

static void
qache_msync(struct qache *q, uns start, uns len)
{
  len += (start % PAGE_SIZE);
  start -= start % PAGE_SIZE;
  len = ALIGN(len, PAGE_SIZE);
  if (msync(q->mmap_data + start, len, MS_ASYNC | MS_INVALIDATE) < 0)
    log(L_ERROR, "Cache %s: msync failed: %m", q->file_name);
  /* FIXME: Do we need this on Linux? */
}

static void
qache_msync_block(struct qache *q, uns blk)
{
  qache_msync(q, blk << q->hdr->block_shift, q->hdr->block_size);
}

static void
qache_lock(struct qache *q)
{
  /* We cannot use flock() since it happily permits locking a shared fd (e.g., after fork()) multiple times */
  ASSERT(!q->locked);
  struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = sizeof(struct qache_header) };
  if (fcntl(q->fd, F_SETLKW, &fl) < 0)
    die("fcntl lock on %s: %m", q->file_name);
  q->locked = 1;
}

static void
qache_unlock(struct qache *q, uns dirty)
{
  ASSERT(q->locked);
  if (dirty)				/* Sync header, entry table and hash table */
    qache_msync(q, 0, q->hdr->first_data_block << q->hdr->block_shift);
  struct flock fl = { .l_type = F_UNLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = sizeof(struct qache_header) };
  if (fcntl(q->fd, F_SETLKW, &fl) < 0)
    die("fcntl unlock on %s: %m", q->file_name);
  q->locked = 0;
}

static uns
qache_hash(struct qache *q, qache_key_t *key)
{
  uns h = ((*key)[0] << 24) | ((*key)[1] << 16) | ((*key)[2] << 8) | (*key)[3];
  return h % q->hdr->hash_size;
}

static uns
qache_hash_find(struct qache *q, qache_key_t *key, uns pos_hint)
{
  ASSERT(q->locked);

  if (pos_hint && pos_hint <= q->hdr->max_entries && !memcmp(q->entry_table[pos_hint].key, key, sizeof(*key)))
    return pos_hint;

  uns h = qache_hash(q, key);
  for (uns e = q->hash_table[h]; e; e=q->entry_table[e].hash_next)
    if (!memcmp(q->entry_table[e].key, key, sizeof(*key)))
      return e;
  return 0;
}

static void
qache_hash_insert(struct qache *q, uns e)
{
  uns h = qache_hash(q, &q->entry_table[e].key);
  q->entry_table[e].hash_next = q->hash_table[h];
  q->hash_table[h] = e;
}

static void
qache_hash_remove(struct qache *q, uns e)
{
  struct qache_entry *entry = &q->entry_table[e];
  uns *hh = &q->hash_table[qache_hash(q, &entry->key)];
  for (uns f = *hh; f; hh=&(q->entry_table[f].hash_next))
    if (!memcmp(q->entry_table[f].key, entry->key, sizeof(qache_key_t)))
      {
	*hh = entry->hash_next;
	return;
      }
  ASSERT(0);
}

static uns
qache_alloc_entry(struct qache *q)
{
  uns e = q->first_free_entry;
  ASSERT(q->locked && e);
  struct qache_entry *entry = &q->entry_table[e];
  ASSERT(entry->data_len == ~0U);
  q->first_free_entry = entry->hash_next;
  entry->data_len = 0;
  return e;
}

static void
qache_free_entry(struct qache *q, uns e)
{
  struct qache_entry *entry = &q->entry_table[e];
  ASSERT(q->locked && entry->data_len != ~0U);
  entry->data_len = ~0U;
  entry->hash_next = q->first_free_entry;
  q->first_free_entry = e;
}

static inline void *
get_block_start(struct qache *q, uns block)
{
  ASSERT(block && block < q->hdr->num_blocks);
  return q->mmap_data + (block << q->hdr->block_shift);
}

static inline void *
get_block_data(struct qache *q, uns block)
{
  return (byte*)get_block_start(q, block) + 4;
}

static inline u32
get_block_next(struct qache *q, uns block)
{
  return *(u32*)get_block_start(q, block);
}

static inline void
set_block_next(struct qache *q, uns block, uns next)
{
  *(u32*)get_block_start(q, block) = next;
}

static uns
qache_alloc_block(struct qache *q)
{
  ASSERT(q->locked && q->num_free_blocks);
  uns blk = q->first_free_block;
  q->first_free_block = get_block_next(q, blk);
  q->num_free_blocks--;
  return blk;
}

static void
qache_free_block(struct qache *q, uns blk)
{
  ASSERT(q->locked);
  set_block_next(q, blk, q->first_free_block);
  qache_msync_block(q, blk);
  q->first_free_block = blk;
  q->num_free_blocks++;
}

static void
qache_lru_insert(struct qache *q, uns e)
{
  struct qache_entry *head = &q->entry_table[0];
  struct qache_entry *entry = &q->entry_table[e];
  ASSERT(q->locked && !entry->lru_prev && !entry->lru_next);
  uns succe = head->lru_next;
  struct qache_entry *succ = &q->entry_table[succe];
  head->lru_next = e;
  entry->lru_prev = 0;
  entry->lru_next = succe;
  succ->lru_prev = e;
}

static void
qache_lru_remove(struct qache *q, uns e)
{
  ASSERT(q->locked);
  struct qache_entry *entry = &q->entry_table[e];
  q->entry_table[entry->lru_prev].lru_next = entry->lru_next;
  q->entry_table[entry->lru_next].lru_prev = entry->lru_prev;
  entry->lru_prev = entry->lru_next = 0;
}

static uns
qache_lru_get(struct qache *q)
{
  return q->entry_table[0].lru_prev;
}

static void
qache_ll_delete(struct qache *q, uns e)
{
  struct qache_entry *entry = &q->entry_table[e];
  uns blk = entry->first_data_block;
  while (entry->data_len)
    {
      uns next = get_block_next(q, blk);
      qache_free_block(q, blk);
      blk = next;
      if (entry->data_len >= q->data_block_size)
	entry->data_len -= q->data_block_size;
      else
	entry->data_len = 0;
    }
  qache_lru_remove(q, e);
  qache_hash_remove(q, e);
  qache_free_entry(q, e);
}

uns
qache_insert(struct qache *q, qache_key_t *key, uns pos_hint, void *data, uns size)
{
  qache_lock(q);

  uns e = qache_hash_find(q, key, pos_hint);
  if (e)
    qache_ll_delete(q ,e);

  uns blocks = (size + q->data_block_size - 1) / q->data_block_size;
  if (blocks > q->hdr->num_blocks - q->hdr->first_data_block)
    {
      qache_unlock(q, 0);
      return 0;
    }
  while (q->num_free_blocks < blocks)
    {
      e = qache_lru_get(q);
      ASSERT(e);
      qache_ll_delete(q, e);
    }
  e = qache_alloc_entry(q);
  struct qache_entry *entry = &q->entry_table[e];
  entry->data_len = size;
  memcpy(entry->key, key, sizeof(*key));

  entry->first_data_block = 0;
  while (size)
    {
      uns chunk = (size % q->data_block_size ) ? : q->data_block_size;
      uns blk = qache_alloc_block(q);
      set_block_next(q, blk, entry->first_data_block);
      memcpy(get_block_data(q, blk), data+size-chunk, chunk);
      qache_msync_block(q, blk);
      entry->first_data_block = blk;
      size -= chunk;
    }

  qache_lru_insert(q, e);
  qache_hash_insert(q, e);
  qache_unlock(q, 1);
  return e;
}

uns
qache_lookup(struct qache *q, qache_key_t *key, uns pos_hint, void **datap, uns *sizep, uns start)
{
  qache_lock(q);
  uns e = qache_hash_find(q, key, pos_hint);
  if (e)
    {
      struct qache_entry *entry = &q->entry_table[e];
      qache_lru_remove(q, e);
      qache_lru_insert(q, e);
      if (sizep)
	{
	  uns size = *sizep;
	  uns avail = (size > entry->data_len) ? 0 : entry->data_len - size;
	  uns xfer = MIN(*sizep, avail);
	  *sizep = avail;
	  if (datap)
	    {
	      if (!*datap)
		*datap = xmalloc(xfer);
	      uns blk = entry->first_data_block;
	      while (start >= q->data_block_size)
		{
		  blk = get_block_next(q, blk);
		  start -= q->data_block_size;
		}
	      byte *data = *datap;
	      while (xfer)
		{
		  uns len = MIN(xfer, q->data_block_size - start);
		  memcpy(data, get_block_data(q, blk), len);
		  blk = get_block_next(q, blk);
		  data += len;
		  xfer -= len;
		  start = 0;
		}
	    }
	}
      else
	ASSERT(!datap);
    }
  qache_unlock(q, 1);			/* Yes, modified -- we update the LRU */
  return e;
}

uns
qache_delete(struct qache *q, qache_key_t *key, uns pos_hint)
{
  qache_lock(q);
  uns e = qache_hash_find(q, key, pos_hint);
  if (e)
    qache_ll_delete(q, e);
  qache_unlock(q, 1);
  return e;
}

void
qache_debug(struct qache *q)
{
  log(L_DEBUG, "Cache %s: block_size=%d (%d data), num_blocks=%d (%d first data), %d entries, %d hash slots",
      q->file_name, q->hdr->block_size, q->data_block_size, q->hdr->num_blocks, q->hdr->first_data_block,
      q->hdr->max_entries, q->hdr->hash_size);

  log(L_DEBUG, "Table of cache entries:");
  log(L_DEBUG, "\tEntry\tLruPrev\tLruNext\tDataLen\tDataBlk\tHashNxt\tKey");
  for (uns e=0; e<=q->hdr->max_entries; e++)
    {
      struct qache_entry *ent = &q->entry_table[e];
      byte ky[2*sizeof(qache_key_t)+1];
      for (uns i=0; i<sizeof(qache_key_t); i++)
	sprintf(ky+2*i, "%02x", ent->key[i]);
      log(L_DEBUG, "\t%d\t%d\t%d\t%d\t%d\t%d\t%s", e, ent->lru_prev, ent->lru_next, ent->data_len,
	  ent->first_data_block, ent->hash_next, ky);
    }

  log(L_DEBUG, "Hash table:");
  for (uns h=0; h<q->hdr->hash_size; h++)
    log(L_DEBUG, "\t%04x\t%d", h, q->hash_table[h]);

  log(L_DEBUG, "Next pointers:");
  for (uns blk=q->hdr->first_data_block; blk<q->hdr->num_blocks; blk++)
    log(L_DEBUG, "\t%d\t%d", blk, get_block_next(q, blk));
}

#ifdef TEST

int main(int argc UNUSED, char **argv UNUSED)
{
  struct qache_params par = {
    .file_name = "tmp/test",
    .block_size = 256,
    .cache_size = 65536,
    .force_reset = 0,
    .format_id = 0xfeedcafe
  };
  struct qache *q = qache_open(&par);

  qache_key_t key = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
  byte data[1000] = { '1', '2', '3', '4', '5' };
  qache_insert(q, &key, 0, data, 1000);
  qache_debug(q);

  qache_lookup(q, &key, 0, NULL, NULL, 0);

  qache_close(q, 1);
  return 0;
}

#endif
