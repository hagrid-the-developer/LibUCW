/*
 *      Test of KMP search
 *
 *      (c) 2006, Pavel Charvat <pchar@ucw.cz>
 */

#include "lib/lib.h"
#include "lib/mempool.h"
#include <string.h>

#if 0
#define TRACE(x...) do{log(L_DEBUG, x);}while(0)
#else
#define TRACE(x...) do{}while(0)
#endif

/* TEST1 - multiple searches */

#define KMP_PREFIX(x) GLUE_(kmp1,x)
#define KMP_WANT_CLEANUP
#include "lib/kmp.h"
#define KMPS_PREFIX(x) GLUE_(kmp1s1,x)
#define KMPS_KMP_PREFIX(x) GLUE_(kmp1,x)
#define KMPS_WANT_BEST
#include "lib/kmp-search.h"
#define KMPS_PREFIX(x) GLUE_(kmp1s2,x)
#define KMPS_KMP_PREFIX(x) GLUE_(kmp1,x)
#define KMPS_VARS uns count;
#define KMPS_INIT(kmp,src,s) do{ s->u.count = 0; }while(0)
#define KMPS_FOUND(kmp,src,s) do{ s->u.count++; }while(0)
#include "lib/kmp-search.h"

static void
test1(void)
{
  TRACE("Running test1");
  struct kmp1_struct kmp;
  kmp1_init(&kmp);
  kmp1_add(&kmp, "ahoj");
  kmp1_add(&kmp, "hoj");
  kmp1_add(&kmp, "aho");
  kmp1_build(&kmp);
  struct kmp1s1_search s1;
  kmp1s1_search(&kmp, &s1, "asjlahslhalahosjkjhojsas");
  TRACE("Best match has %d characters", s1.best->len);
  ASSERT(s1.best->len == 3);
  struct kmp1s2_search s2;
  kmp1s2_search(&kmp, &s2, "asjlahslhalahojsjkjhojsas");
  ASSERT(s2.u.count == 4);
  kmp1_cleanup(&kmp);
}

/* TEST2 - various tracing */

#define KMP_PREFIX(x) GLUE_(kmp2,x)
#define KMP_USE_UTF8
#define KMP_TOLOWER
#define KMP_ONLYALPHA
#define KMP_STATE_VARS byte *str; uns id;
#define KMP_ADD_EXTRA_ARGS uns id
#define KMP_ADD_EXTRA_VAR byte *
#define KMP_ADD_INIT(kmp,src,v) do{ v = src; }while(0)
#define KMP_ADD_NEW(kmp,src,v,s) do{ TRACE("Inserting string %s with id %d", v, id); \
  s->u.str = v; s->u.id = id; }while(0)
#define KMP_ADD_DUP(kmp,src,v,s) do{ TRACE("String %s already inserted", v); }while(0)
#define KMP_WANT_CLEANUP
#define KMP_WANT_SEARCH
#define KMPS_ADD_CONTROLS
#define KMPS_MERGE_CONTROLS
#define KMPS_WANT_BEST
#define KMPS_FOUND(kmp,src,s) do{ TRACE("String %s with id %d found", s->out->u.str, s->out->u.id); }while(0)
#define KMPS_STEP(kmp,src,s) do{ TRACE("Got to state %p after reading %d", s->s, s->c); }while(0)
#define KMPS_EXIT(kmp,src,s) do{ if (s->best->len) TRACE("Best match is %s", s->best->u.str); } while(0)
#include "lib/kmp.h"

static void
test2(void)
{
  TRACE("Running test2");
  struct kmp2_struct kmp;
  kmp2_init(&kmp);
  kmp2_add(&kmp, "ahoj", 1);
  kmp2_add(&kmp, "ahoj", 2);
  kmp2_add(&kmp, "hoj", 3);
  kmp2_add(&kmp, "aho", 4);
  kmp2_add(&kmp, "aba", 5);
  kmp2_add(&kmp, "aba", 5);
  kmp2_add(&kmp, "pěl", 5);
  kmp2_build(&kmp);
  kmp2_run(&kmp, "Šíleně žluťoučký kůň úpěl ďábelské ódy labababaks sdahojdhsaladsjhla");
  kmp2_cleanup(&kmp);
}

/* TEST3 - random tests */

#define KMP_PREFIX(x) GLUE_(kmp3,x)
#define KMP_STATE_VARS uns index;
#define KMP_ADD_EXTRA_ARGS uns index
#define KMP_ADD_EXTRA_VAR byte *
#define KMP_ADD_INIT(kmp,src,v) do{ v = src; }while(0)
#define KMP_ADD_NEW(kmp,src,v,s) do{ s->u.index = index; }while(0)
#define KMP_ADD_DUP(kmp,src,v,s) do{ *v = 0; }while(0)
#define KMP_WANT_CLEANUP
#define KMP_WANT_SEARCH
#define KMPS_VARS uns sum, *cnt;
#define KMPS_FOUND(kmp,src,s) do{ ASSERT(s->u.cnt[s->out->u.index]); s->u.cnt[s->out->u.index]--; s->u.sum--; }while(0)
#include "lib/kmp.h"

static void
test3(void)
{
  TRACE("Running test3");
  struct mempool *pool = mp_new(1024);
  for (uns testn = 0; testn < 100; testn++)
  {
    mp_flush(pool);
    uns n = random_max(100);
    byte *s[n];
    struct kmp3_struct kmp;
    kmp3_init(&kmp);
    for (uns i = 0; i < n; i++)
      {
        uns m = random_max(10);
        s[i] = mp_alloc(pool, m + 1);
        for (uns j = 0; j < m; j++)
	  s[i][j] = 'a' + random_max(3);
        s[i][m] = 0;
        kmp3_add(&kmp, s[i], i);
      }
    kmp3_build(&kmp);
    for (uns i = 0; i < 10; i++)
      {
        uns m = random_max(100);
        byte b[m + 1];
        for (uns j = 0; j < m; j++)
	  b[j] = 'a' + random_max(4);
        b[m] = 0;
        uns cnt[n];
	struct kmp3_search search;
	search.u.sum = 0;
	search.u.cnt = cnt;
        for (uns j = 0; j < n; j++)
          {
	    cnt[j] = 0;
	    if (*s[j])
	      for (uns k = 0; k < m; k++)
	        if (!strncmp(b + k, s[j], strlen(s[j])))
	          cnt[j]++, search.u.sum++;
	  }
        kmp3_search(&kmp, &search, b);
        ASSERT(search.u.sum == 0);
      }
    kmp3_cleanup(&kmp);
  }
  mp_delete(pool);
}

/* TEST4 - user-defined character type */

struct kmp4_struct;
struct kmp4_state;

static inline int
kmp4_eq(struct kmp4_struct *kmp UNUSED, byte *a, byte *b)
{
  return (a == b) || (a && b && *a == *b);
}

static inline uns
kmp4_hash(struct kmp4_struct *kmp UNUSED, struct kmp4_state *s, byte *c)
{
  return (c ? (*c << 16) : 0) + (uns)(addr_int_t)s;
}

#define KMP_PREFIX(x) GLUE_(kmp4,x)
#define KMP_CHAR byte *
#define KMP_CONTROL_CHAR NULL
#define KMP_GET_CHAR(kmp,src,c) ({ c = src++; !!*c; })
#define KMP_GIVE_HASHFN
#define KMP_GIVE_EQ
#define KMP_WANT_CLEANUP
#define KMP_WANT_SEARCH
#define KMPS_FOUND(kmp,src,s) do{ TRACE("found"); }while(0)
#define KMPS_ADD_CONTROLS
#define KMPS_MERGE_CONTROLS
#include "lib/kmp.h"

static void
test4(void)
{
  TRACE("Running test4");
  struct kmp4_struct kmp;
  kmp4_init(&kmp);
  kmp4_add(&kmp, "ahoj");
  kmp4_build(&kmp);
  kmp4_run(&kmp, "djdhaskjdahoahaahojojshdaksjahdahojskj");
  kmp4_cleanup(&kmp);
}

int
main(void)
{
  test1();
  test2();
  test3();
  test4();
  return 0;
}
