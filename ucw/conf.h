/*
 *	UCW Library -- Configuration files
 *
 *	(c) 2001--2006 Robert Spalek <robert@ucw.cz>
 *	(c) 2003--2014 Martin Mares <mj@ucw.cz>
 *	(c) 2014 Pavel Charvat <pchar@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#ifndef	_UCW_CONF_H
#define	_UCW_CONF_H

#include <ucw/clists.h>
#include <ucw/gary.h>

#ifdef CONFIG_UCW_CLEAN_ABI
#define cf_close_group ucw_cf_close_group
#define cf_declare_rel_section ucw_cf_declare_rel_section
#define cf_declare_section ucw_cf_declare_section
#define cf_delete_context ucw_cf_delete_context
#define cf_dump_sections ucw_cf_dump_sections
#define cf_find_item ucw_cf_find_item
#define cf_get_pool ucw_cf_get_pool
#define cf_init_section ucw_cf_init_section
#define cf_journal_block ucw_cf_journal_block
#define cf_journal_commit_transaction ucw_cf_journal_commit_transaction
#define cf_journal_new_transaction ucw_cf_journal_new_transaction
#define cf_journal_rollback_transaction ucw_cf_journal_rollback_transaction
#define cf_load ucw_cf_load
#define cf_malloc ucw_cf_malloc
#define cf_malloc_zero ucw_cf_malloc_zero
#define cf_modify_item ucw_cf_modify_item
#define cf_new_context ucw_cf_new_context
#define cf_open_group ucw_cf_open_group
#define cf_parse_double ucw_cf_parse_double
#define cf_parse_int ucw_cf_parse_int
#define cf_parse_ip ucw_cf_parse_ip
#define cf_parse_u64 ucw_cf_parse_u64
#define cf_printf ucw_cf_printf
#define cf_reload ucw_cf_reload
#define cf_revert ucw_cf_revert
#define cf_set ucw_cf_set
#define cf_set_journalling ucw_cf_set_journalling
#define cf_strdup ucw_cf_strdup
#define cf_switch_context ucw_cf_switch_context
#endif

struct mempool;

/***
 * [[conf_ctxt]]
 * Configuration contexts
 * ~~~~~~~~~~~~~~~~~~~~~~
 *
 * The state of the configuration parser is stored within a configuration context.
 * One such context is automatically created during initialization of the library
 * and you need not care about more, as long as you use a single configuration file.
 *
 * In full generality, you can define as many contexts as you wish and switch
 * between them. Each thread has its own pointer to the current context, which
 * must not be shared with other threads.
 ***/

/** Create a new configuration context. **/
struct cf_context *cf_new_context(void);

/**
 * Free a configuration context. The context must not be set as current
 * for any thread, nor can it be the default context.
 *
 * All configuration settings made within the context are rolled back
 * (except when journalling is turned off). All memory allocated on behalf
 * of the context is freed, which includes memory obtained by calls to
 * @cf_malloc().
 **/
void cf_delete_context(struct cf_context *cc);

/**
 * Make the given configuration context current and return the previously
 * active context. Both the new and the old context may be NULL.
 **/
struct cf_context *cf_switch_context(struct cf_context *cc);

/***
 * [[conf_load]]
 * Safe configuration loading
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * These functions can be used to to safely load or reload configuration.
 */

/**
 * Load configuration from @file.
 * Returns a non-zero value upon error. In that case, all changes to the
 * configuration specified in the file are undone.
 **/
int cf_load(const char *file);

/**
 * Reload configuration from @file, replace the old one.
 * If @file is NULL, reload all loaded configuration files and re-apply
 * bits of configuration passed to @cf_set().
 * Returns a non-zero value upon error. In that case, all configuration
 * settings are rolled back to the state before calling this function.
 **/
int cf_reload(const char *file);

/**
 * Parse some part of configuration passed in @string.
 * The syntax is the same as in the <<config:,configuration file>>.
 * Returns a non-zero value upon error. In that case, all changes to the
 * configuration specified by the already executed parts of the string
 * are undone.
 **/
int cf_set(const char *string);

/**
 * Sometimes, the configuration is split to multiple files and when only
 * some of the are loaded, the settings are not consistent -- for example,
 * they might have been rejected by a commit hook, because a mandatory setting
 * is missing.
 *
 * This function opens a configuration group, in which multiple files can be
 * loaded and all commit hooks are deferred until the group is closed.
 **/
void cf_open_group(void);

/**
 * Close a group opened by @cf_open_group(). Returns a non-zero value upon error,
 * which usually means that a commit hook has failed.
 **/
int cf_close_group(void);

/**
 * Return all configuration items to their initial state before loading the
 * configuration file. If journalling is disabled, it does nothing.
 **/
void cf_revert(void);

/*** === Data types [[conf_types]] ***/

enum cf_class {				/** Class of the configuration item. **/
  CC_END,				// end of list
  CC_STATIC,				// single variable or static array
  CC_DYNAMIC,				// dynamically allocated array
  CC_PARSER,				// arbitrary parser function
  CC_SECTION,				// section appears exactly once
  CC_LIST,				// list with 0..many nodes
  CC_BITMAP				// of up to 32 items
};

enum cf_type {				/** Type of a single value. **/
  CT_INT, CT_U64, CT_DOUBLE,		// number types
  CT_IP,				// IP address
  CT_STRING,				// string type
  CT_LOOKUP,				// in a string table
  CT_USER,				// user-defined type (obsolete)
  CT_XTYPE				// extended type
};

struct fastbuf;

/**
 * A parser function gets an array of (strdup'ed) strings and a pointer with
 * the customized information (most likely the target address).  It can store
 * the parsed value anywhere in any way it likes, however it must first call
 * @cf_journal_block() on the overwritten memory block.  It returns an error
 * message or NULL if everything is all right.
 **/
typedef char *cf_parser(uint number, char **pars, void *ptr);
/**
 * A parser function for user-defined types gets a string and a pointer to
 * the destination variable.  It must store the value within [ptr,ptr+size),
 * where size is fixed for each type.  It should not call @cf_journal_block().
 **/
typedef char *cf_parser1(char *string, void *ptr);
/**
 * An init- or commit-hook gets a pointer to the section or NULL if this
 * is the global section.  It returns an error message or NULL if everything
 * is all right.  The init-hook should fill in default values (needed for
 * dynamically allocated nodes of link lists or for filling global variables
 * that are run-time dependent).  The commit-hook should perform sanity
 * checks and postprocess the parsed values.  Commit-hooks must call
 * @cf_journal_block() too.  Caveat! init-hooks for static sections must not
 * use @cf_malloc() but normal <<memory:xmalloc()>>.
 **/
typedef char *cf_hook(void *ptr);
/**
 * Dumps the contents of a variable of a user-defined type.
 **/
typedef void cf_dumper1(struct fastbuf *fb, void *ptr);
/**
 * Similar to init-hook, but it copies attributes from another list node
 * instead of setting the attributes to default values.  You have to provide
 * it if your node contains parsed values and/or sub-lists.
 **/
typedef char *cf_copier(void *dest, void *src);

struct cf_user_type {			/** Structure to store information about user-defined variable type. **/
  uint size;				// of the parsed attribute
  char *name;				// name of the type (for dumping)
  cf_parser1 *parser;			// how to parse it
  cf_dumper1 *dumper;			// how to dump the type
};

struct cf_section;
struct cf_item {			/** Single configuration item. **/
  const char *name;			// case insensitive
  int number;				// length of an array or #parameters of a parser (negative means at most)
  void *ptr;				// pointer to a global variable or an offset in a section
  union cf_union {
    struct cf_section *sec;		// declaration of a section or a list
    cf_parser *par;			// parser function
    const char * const *lookup;		// NULL-terminated sequence of allowed strings for lookups
    struct cf_user_type *utype;		// specification of the user-defined type (obsolete)
    const struct xtype *xtype;		// specification of the extended type
  } u;
  enum cf_class cls:16;			// attribute class
  enum cf_type type:16;			// type of a static or dynamic attribute
};

struct cf_section {			/** A section. **/
  uint size;				// 0 for a global block, sizeof(struct) for a section
  cf_hook *init;			// fills in default values (no need to bzero)
  cf_hook *commit;			// verifies parsed data (optional)
  cf_copier *copy;			// copies values from another instance (optional, no need to copy basic attributes)
  struct cf_item *cfg;			// CC_END-terminated array of items
  uint flags;				// for internal use only
};

/***
 * [[conf_macros]]
 * Convenience macros
 * ~~~~~~~~~~~~~~~~~~
 *
 * You could create the structures manually, but you can use these macros to
 * save some typing.
 */

/***
 * Declaration of <<struct_cf_section,`cf_section`>>
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * These macros can be used to configure the <<struct_cf_section,`cf_section`>>
 * structure.
 ***/

/**
 * Data type of a section.
 * If you store the section into a structure, use this macro.
 *
 * Storing a section into a structure is useful mostly when you may have multiple instances of the
 * section (eg. <<conf_multi,array or list>>).
 *
 * Example:
 *
 *   struct list_node {
 *     cnode n;		// This one is for the list itself
 *     char *name;
 *     uint value;
 *   };
 *
 *   static struct clist nodes;
 *
 *   static struct cf_section node = {
 *     CF_TYPE(struct list_node),
 *     CF_ITEMS {
 *       CF_STRING("name", PTR_TO(struct list_node, name)),
 *       CF_UINT("value", PTR_TO(struct list_node, value)),
 *       CF_END
 *     }
 *   };
 *
 *   static struct cf_section section = {
 *     CF_LIST("node", &nodes, &node),
 *     CF_END
 *   };
 *
 * You could use <<def_CF_STATIC,`CF_STATIC`>> or <<def_CF_DYNAMIC,`CF_DYNAMIC`>>
 * macros to create arrays.
 */
#define CF_TYPE(s)	.size = sizeof(s)
/**
 * An init <<hooks,hook>>.
 * You can use this to initialize dynamically allocated items (for a dynamic array or list).
 * The hook returns an error message or NULL if everything was OK.
 */
#define CF_INIT(f)	.init = (cf_hook*) f
/**
 * A commit <<hooks,hook>>.
 * You can use this one to check sanity of loaded data and postprocess them.
 * You must call @cf_journal_block() if you change anything.
 *
 * Return error message or NULL if everything went OK.
 **/
#define CF_COMMIT(f)	.commit = (cf_hook*) f
/**
 * A <<hooks,copy function>>.
 * You need to provide one for too complicated sections where a memcpy is not
 * enough to copy it properly. It happens, for example, when you have a dynamically
 * allocated section containing a list of other sections.
 *
 * You return an error message or NULL if you succeed.
 **/
#define CF_COPY(f)	.copy = (cf_copier*) f		/**  **/
#define CF_ITEMS	.cfg = ( struct cf_item[] )	/** List of sub-items. **/
#define CF_END		{ .cls = CC_END }		/** End of the structure. **/
/***
 * Declaration of a configuration item
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * Each of these describe single <<struct_cf_item,configuration item>>. They are mostly
 * for internal use, do not use them directly unless you really know what you are doing.
 ***/

/**
 * Static array of items.
 * Expects you to allocate the memory and provide pointer to it.
 **/
#define CF_STATIC(n,p,T,t,c)	{ .cls = CC_STATIC, .type = CT_##T, .name = n, .number = c, .ptr = CHECK_PTR_TYPE(p,t*) }
/**
 * Dynamic array of items.
 * Expects you to provide pointer to your pointer to data and it will allocate new memory for it
 * and set your pointer to it.
 **/
#define CF_DYNAMIC(n,p,T,t,c)	{ .cls = CC_DYNAMIC, .type = CT_##T, .name = n, .number = c, .ptr = CHECK_PTR_TYPE(p,t**) }
#define CF_PARSER(n,p,f,c)	{ .cls = CC_PARSER, .name = n, .number = c, .ptr = p, .u.par = (cf_parser*) f }					/** A low-level parser. **/
#define CF_SECTION(n,p,s)	{ .cls = CC_SECTION, .name = n, .number = 1, .ptr = p, .u.sec = s }						/** A sub-section. **/
#define CF_LIST(n,p,s)		{ .cls = CC_LIST, .name = n, .number = 1, .ptr = CHECK_PTR_TYPE(p,clist*), .u.sec = s }				/** A list with sub-items. **/
#define CF_BITMAP_INT(n,p)	{ .cls = CC_BITMAP, .type = CT_INT, .name = n, .number = 1, .ptr = CHECK_PTR_TYPE(p,u32*) }			/** A bitmap. **/
#define CF_BITMAP_LOOKUP(n,p,t)	{ .cls = CC_BITMAP, .type = CT_LOOKUP, .name = n, .number = 1, .ptr = CHECK_PTR_TYPE(p,u32*), .u.lookup = t }	/** A bitmap with named bits. **/
/***
 * Basic configuration items
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * They describe basic data types used in the configuration. This should be enough for
 * most real-life purposes.
 *
 * The parameters are as follows:
 *
 * * @n -- name of the item.
 * * @p -- pointer to the variable where it shall be stored.
 * * @c -- count.
 **/
#define CF_INT(n,p)		CF_STATIC(n,p,INT,int,1)		/** Single `int` value. **/
#define CF_INT_ARY(n,p,c)	CF_STATIC(n,p,INT,int,c)		/** Static array of integers. **/
#define CF_INT_DYN(n,p,c)	CF_DYNAMIC(n,p,INT,int,c)		/** Dynamic array of integers. **/
#define CF_UINT(n,p)		CF_STATIC(n,p,INT,uint,1)		/** Single `uint` (`unsigned`) value. **/
#define CF_UINT_ARY(n,p,c)	CF_STATIC(n,p,INT,uint,c)		/** Static array of unsigned integers. **/
#define CF_UINT_DYN(n,p,c)	CF_DYNAMIC(n,p,INT,uint,c)		/** Dynamic array of unsigned integers. **/
#define CF_U64(n,p)		CF_STATIC(n,p,U64,u64,1)		/** Single unsigned 64bit integer (`u64`). **/
#define CF_U64_ARY(n,p,c)	CF_STATIC(n,p,U64,u64,c)		/** Static array of u64s. **/
#define CF_U64_DYN(n,p,c)	CF_DYNAMIC(n,p,U64,u64,c)		/** Dynamic array of u64s. **/
#define CF_DOUBLE(n,p)		CF_STATIC(n,p,DOUBLE,double,1)		/** Single instance of `double`. **/
#define CF_DOUBLE_ARY(n,p,c)	CF_STATIC(n,p,DOUBLE,double,c)		/** Static array of doubles. **/
#define CF_DOUBLE_DYN(n,p,c)	CF_DYNAMIC(n,p,DOUBLE,double,c)		/** Dynamic array of doubles. **/
#define CF_IP(n,p)		CF_STATIC(n,p,IP,u32,1)			/** Single IPv4 address. **/
#define CF_IP_ARY(n,p,c)	CF_STATIC(n,p,IP,u32,c)			/** Static array of IP addresses. **/.
#define CF_IP_DYN(n,p,c)	CF_DYNAMIC(n,p,IP,u32,c)		/** Dynamic array of IP addresses. **/

/* FIXME: Backwards compatibility only, should not be used at is will be removed soon. */
#define CF_UNS CF_UINT
#define CF_UNS_ARY CF_UINT_ARY
#define CF_UNS_DYN CF_UINT_DYN

/**
 * A string.
 * You provide a pointer to a `char *` variable and it will fill it with
 * dynamically allocated string. For example:
 *
 *   static char *string = "Default string";
 *
 *   static struct cf_section section = {
 *     CF_ITEMS {
 *       CF_STRING("string", &string),
 *       CF_END
 *     }
 *   };
 **/
#define CF_STRING(n,p)		CF_STATIC(n,p,STRING,char*,1)
#define CF_STRING_ARY(n,p,c)	CF_STATIC(n,p,STRING,char*,c)		/** Static array of strings. **/
#define CF_STRING_DYN(n,p,c)	CF_DYNAMIC(n,p,STRING,char*,c)		/** Dynamic array of strings. **/
/**
 * One string out of a predefined set.
 * You provide the set as an array of strings terminated by NULL (similar to @argv argument
 * of main()) as the @t parameter.
 *
 * The configured variable (pointer to `int`) is set to index of the string.
 * So, it works this way:
 *
 *   static *strings[] = { "First", "Second", "Third", NULL };
 *
 *   static int variable;
 *
 *   static struct cf_section section = {
 *     CF_ITEMS {
 *       CF_LOOKUP("choice", &variable, strings),
 *       CF_END
 *     }
 *   };
 *
 * Now, if the configuration contains `choice "Second"`, `variable` will be set to 1.
 **/
#define CF_LOOKUP(n,p,t)	{ .cls = CC_STATIC, .type = CT_LOOKUP, .name = n, .number = 1, .ptr = CHECK_PTR_TYPE(p,int*), .u.lookup = t }
/**
 * Static array of strings out of predefined set.
 **/
#define CF_LOOKUP_ARY(n,p,t,c)	{ .cls = CC_STATIC, .type = CT_LOOKUP, .name = n, .number = c, .ptr = CHECK_PTR_TYPE(p,int*), .u.lookup = t }
/**
 * Dynamic array of strings out of predefined set.
 **/
#define CF_LOOKUP_DYN(n,p,t,c)	{ .cls = CC_DYNAMIC, .type = CT_LOOKUP, .name = n, .number = c, .ptr = CHECK_PTR_TYPE(p,int**), .u.lookup = t }
/**
 * A user-defined type.
 * See <<custom_parser,creating custom parsers>> section if you want to know more.
 **/
#define CF_USER(n,p,t)		{ .cls = CC_STATIC, .type = CT_USER, .name = n, .number = 1, .ptr = p, .u.utype = t }
/**
 * Static array of user-defined types (all of the same type).
 * See <<custom_parser,creating custom parsers>> section.
 **/
#define CF_USER_ARY(n,p,t,c)	{ .cls = CC_STATIC, .type = CT_USER, .name = n, .number = c, .ptr = p, .u.utype = t }
/**
 * Dynamic array of user-defined types.
 * See <<custom_parser,creating custom parsers>> section.
 **/
#define CF_USER_DYN(n,p,t,c)	{ .cls = CC_DYNAMIC, .type = CT_USER, .name = n, .number = c, .ptr = p, .u.utype = t }
/**
 * An extended type.
 * See <<xtypes:,extended types>> if you want to know more.
 **/
#define CF_XTYPE(n,p,t)		{ .cls = CC_STATIC, .type = CT_XTYPE, .name = n, .number = 1, .ptr = p, .u.xtype = t }
/**
 * Static array of extended types (all of the same type).
 * See <<xtypes:,extended types>>.
 **/
#define CF_XTYPE_ARY(n,p,t,c)	{ .cls = CC_STATIC, .type = CT_XTYPE, .name = n, .number = c, .ptr = p, .u.xtype = t }
/**
 * Dynamic array of extended types.
 * See <<xtypes:,extended types>>.
 **/
#define CF_XTYPE_DYN(n,p,t,c)	{ .cls = CC_DYNAMIC, .type = CT_XTYPE, .name = n, .number = c, .ptr = p, .u.xtype = t }

/**
 * Any number of dynamic array elements
 **/
#define CF_ANY_NUM		-0x7fffffff

#define DARY_LEN(a) GARY_SIZE(a)		/** Length of an dynamic array. An alias for `GARY_SIZE`. **/

/***
 * [[alloc]]
 * Memory allocation
 * ~~~~~~~~~~~~~~~~~
 *
 * Each configuration context has one or more <<mempool:,memory pools>>, where all
 * data related to the configuration are stored.
 *
 * The following set of functions allocate from these pools. The allocated memory
 * is valid as long as the current configuration (when the configuration file is
 * reloaded or rolled back, or the context is deleted, it gets lost).
 *
 * Memory allocated from within custom parsers should be allocated from the pools.
 *
 * Please note that the pool is not guaranteed to exist before you call cf_load(),
 * cf_set(), or cf_getopt() on the particular context.
 ***/
struct mempool *cf_get_pool(void); /** Return a pointer to the current configuration pool. **/
void *cf_malloc(uint size);	/** Returns @size bytes of memory allocated from the current configuration pool. **/
void *cf_malloc_zero(uint size);	/** Like @cf_malloc(), but zeroes the memory. **/
char *cf_strdup(const char *s);	/** Copy a string into @cf_malloc()ed memory. **/
char *cf_printf(const char *fmt, ...) FORMAT_CHECK(printf,1,2); /** printf() into @cf_malloc()ed memory. **/

/***
 * [[journal]]
 * Undo journal
 * ~~~~~~~~~~~~
 *
 * The configuration system uses a simple journaling mechanism, which makes
 * it possible to undo changes to configuration. A typical example is loading
 * of configuration by cf_load(): internally, it creates a transaction, applies
 * all changes specified by the configuration and if one of them fails, the whole
 * journal is replayed to restore the whole original state. Similarly, cf_reload()
 * uses the journal to switch between configurations.
 *
 * In most cases, you need not care about the journal, except when you need
 * to change some data from a <<hooks,hook>>, or if you want to call cf_modify_item() and then
 * undo the changes.
 ***/
/**
 * This function can be used to disable the whole journalling mechanism.
 * It saves some memory, but it makes undoing of configuration changes impossible,
 * which breaks for example cf_reload().
 **/
void cf_set_journalling(int enable);
/**
 * When a block of memory is about to be changed, put the old value
 * into journal with this function. You need to call it from a <<hooks,commit hook>>
 * if you change anything. It is used internally by low-level parsers.
 * <<custom_parser,Custom parsers>> do not need to call it, it is called
 * before them.
 **/
void cf_journal_block(void *ptr, uint len);
#define CF_JOURNAL_VAR(var) cf_journal_block(&(var), sizeof(var))	// Store a single value into the journal

struct cf_journal_item;		/** Opaque identifier of the journal state. **/
/**
 * Starts a new transaction. It returns the current state so you can
 * get back to it. The @new_pool parameter tells if a new memory pool
 * should be created and used from now.
 **/
struct cf_journal_item *cf_journal_new_transaction(uint new_pool);
/**
 * Marks current state as a complete transaction. The @new_pool
 * parameter tells if the transaction was created with new memory pool
 * (the parameter must be the same as the one with
 * @cf_journal_new_transaction() was called with). The @oldj parameter
 * is the journal state returned from last
 * @cf_journal_new_transaction() call.
 **/
void cf_journal_commit_transaction(uint new_pool, struct cf_journal_item *oldj);
/**
 * Returns to an old journal state, reverting anything the current
 * transaction did. The @new_pool parameter must be the same as the
 * one you used when you created the transaction. The @oldj parameter
 * is the journal state you got from @cf_journal_new_transaction() --
 * it is the state to return to.
 **/
void cf_journal_rollback_transaction(uint new_pool, struct cf_journal_item *oldj);

/***
 * [[declare]]
 * Section declaration
 * ~~~~~~~~~~~~~~~~~~~
 **/

/**
 * Plug another top-level section into the configuration system.
 * @name is the name in the configuration file,
 * @sec is pointer to the section description.
 * If @allow_unknown is set to 0 and a variable not described in @sec
 * is found in the configuration file, it produces an error.
 * If you set it to 1, all such variables are ignored.
 *
 * Please note that a single section definition cannot be used in multiple
 * configuration contexts simultaneously.
 **/
void cf_declare_section(const char *name, struct cf_section *sec, uint allow_unknown);
/**
 * Like @cf_declare_section(), but instead of item pointers, the section
 * contains offsets relative to @ptr. In other words, it does the same
 * as `CF_SECTION`, but for top-level sections.
 **/
void cf_declare_rel_section(const char *name, struct cf_section *sec, void *ptr, uint allow_unknown);
/**
 * If you have a section in a structure and you want to initialize it
 * (eg. if you want a copy of default values outside the configuration),
 * you can use this. It initializes it recursively.
 *
 * This is used mostly internally. You probably do not need it.
 **/
void cf_init_section(const char *name, struct cf_section *sec, void *ptr, uint do_bzero);

/***
 * [[bparser]]
 * Parsers for basic types
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Each of them gets a string to parse and pointer to store the value.
 * It returns either NULL or error message.
 *
 * The parsers support units. See <<config:units,their list>>.
 ***/
char *cf_parse_int(const char *str, int *ptr);		/** Parser for integers. **/
char *cf_parse_u64(const char *str, u64 *ptr);		/** Parser for 64 unsigned integers. **/
char *cf_parse_double(const char *str, double *ptr);	/** Parser for doubles. **/
char *cf_parse_ip(const char *p, u32 *varp);		/** Parser for IP addresses. **/

/***
 * [[conf_direct]]
 * Direct access
 * ~~~~~~~~~~~~~
 *
 * Direct access to configuration items.
 * You probably should not need this, but in your do, you have to handle
 * <<journal,journalling>> yourself.
 ***/

/**
 * List of operations used on items.
 * This macro is used to generate internal source code,
 * but you may be interested in the list of operations it creates.
 *
 * Each operation corresponds to the same-named operation
 * described in <<config:operations,configuration syntax>>.
 **/
#define CF_OPERATIONS T(CLOSE) T(SET) T(CLEAR) T(ALL) \
  T(APPEND) T(PREPEND) T(REMOVE) T(EDIT) T(AFTER) T(BEFORE) T(COPY) T(RESET)
  /* Closing brace finishes previous block.
   * Basic attributes (static, dynamic, parsed) can be used with SET.
   * Dynamic arrays can be used with SET, APPEND, PREPEND.
   * Sections can be used with SET.
   * Lists can be used with everything. */
#define T(x) OP_##x,
enum cf_operation { CF_OPERATIONS };	/** Allowed operations on items. See <<def_CF_OPERATIONS,`CF_OPERATIONS`>> for list (they have an `OP_` prefix -- it means you use `OP_SET` instead of just `SET`). **/
#undef T

/**
 * Searches for a configuration item called @name.
 * If it is found, it is copied into @item and NULL is returned.
 * Otherwise, an error is returned and @item is zeroed.
 **/
char *cf_find_item(const char *name, struct cf_item *item);
/**
 * Performs a single operation on a given item.
 **/
char *cf_modify_item(struct cf_item *item, enum cf_operation op, int number, char **pars);

/***
 * [[conf_dump]]
 * Debug dumping
 * ~~~~~~~~~~~~~
 ***/

struct fastbuf;
/**
 * Write the current state of all configuration items into @fb.
 **/
void cf_dump_sections(struct fastbuf *fb);

#endif
