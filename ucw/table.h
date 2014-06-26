/*
 *	UCW Library -- Table printer
 *
 *	(c) 2014 Robert Kessl <robert.kessl@economia.cz>
 */

#ifndef _UCW_TABLE_H
#define _UCW_TABLE_H

#include <ucw/fastbuf.h>
#include <ucw/mempool.h>

#ifdef CONFIG_UCW_CLEAN_ABI
#define table_append_bool ucw_table_append_bool
#define table_append_double ucw_table_append_double
#define table_append_int ucw_table_append_int
#define table_append_intmax ucw_table_append_intmax
#define table_append_printf ucw_table_append_printf
#define table_append_str ucw_table_append_str
#define table_append_u64 ucw_table_append_u64
#define table_append_uint ucw_table_append_uint
#define table_append_uintmax ucw_table_append_uintmax
#define table_cleanup ucw_table_cleanup
#define table_col_bool ucw_table_col_bool
#define table_col_bool_fmt ucw_table_col_bool_fmt
#define table_col_bool_name ucw_table_col_bool_name
#define table_col_double ucw_table_col_double
#define table_col_double_fmt ucw_table_col_double_fmt
#define table_col_double_name ucw_table_col_double_name
#define table_col_fbend ucw_table_col_fbend
#define table_col_fbstart ucw_table_col_fbstart
#define table_col_int ucw_table_col_int
#define table_col_int_fmt ucw_table_col_int_fmt
#define table_col_int_name ucw_table_col_int_name
#define table_col_intmax ucw_table_col_intmax
#define table_col_intmax_fmt ucw_table_col_intmax_fmt
#define table_col_intmax_name ucw_table_col_intmax_name
#define table_col_printf ucw_table_col_printf
#define table_col_s64 ucw_table_col_s64
#define table_col_s64_fmt ucw_table_col_s64_fmt
#define table_col_s64_name ucw_table_col_s64_name
#define table_col_str ucw_table_col_str
#define table_col_str_fmt ucw_table_col_str_fmt
#define table_col_str_name ucw_table_col_str_name
#define table_col_u64 ucw_table_col_u64
#define table_col_u64_fmt ucw_table_col_u64_fmt
#define table_col_u64_name ucw_table_col_u64_name
#define table_col_uint ucw_table_col_uint
#define table_col_uint_fmt ucw_table_col_uint_fmt
#define table_col_uint_name ucw_table_col_uint_name
#define table_col_uintmax ucw_table_col_uintmax
#define table_col_uintmax_fmt ucw_table_col_uintmax_fmt
#define table_col_uintmax_name ucw_table_col_uintmax_name
#define table_end ucw_table_end
#define table_end_row ucw_table_end_row
#define table_fmt_blockline ucw_table_fmt_blockline
#define table_fmt_human_readable ucw_table_fmt_human_readable
#define table_fmt_machine_readable ucw_table_fmt_machine_readable
#define table_get_col_idx ucw_table_get_col_idx
#define table_get_col_list ucw_table_get_col_list
#define table_init ucw_table_init
#define table_set_col_order ucw_table_set_col_order
#define table_set_col_order_by_name ucw_table_set_col_order_by_name
#define table_set_formatter ucw_table_set_formatter
#define table_set_gary_options ucw_table_set_gary_options
#define table_set_option ucw_table_set_option
#define table_set_option_value ucw_table_set_option_value
#define table_start ucw_table_start
#endif

/***
 * Table definitions
 * -----------------
 ***/

/** Types of columns. These are seldom used explicitly, using a column definition macro is preferred. **/
enum column_type {
  COL_TYPE_STR,		// String
  COL_TYPE_INT,		// int
  COL_TYPE_S64,		// Signed 64-bit integer
  COL_TYPE_INTMAX,	// intmax_t
  COL_TYPE_UINT,	// unsigned int
  COL_TYPE_U64,		// Unsigned 64-bit integer
  COL_TYPE_UINTMAX,	// uintmax_t
  COL_TYPE_BOOL,	// bool
  COL_TYPE_DOUBLE,	// double
  COL_TYPE_ANY,		// Any type
  COL_TYPE_LAST
};

/** Justify cell contents to the left. **/
#define CELL_ALIGN_LEFT     (1U << 31)

// CELL_FLAG_MASK has 1's in bits used for column flags,
// CELL_WIDTH_MASK has 1's in bits used for column width.
#define CELL_FLAG_MASK	(CELL_ALIGN_LEFT)
#define CELL_WIDTH_MASK	(~CELL_FLAG_MASK)

/**
 * Definition of a single table column.
 * Usually, this is generated using the `TABLE_COL_`'type' macros.
 * Fields marked with `[*]` are user-accessible.
 **/
struct table_column {
  const char *name;		// [*] Name of the column displayed in table header
  int width;			// [*] Width of the column (in characters) OR'ed with column flags
  const char *fmt;		// [*] Default format of each cell in the column
  enum column_type type;	// [*] Type of the cells in the column
};

/**
 * Definition of a table. Contains column definitions, per-table settings
 * and internal data. Please use only fields marked with `[*]`.
 **/
struct table {
  struct table_column *columns;		// [*] Definition of columns
  int column_count;			// [*] Number of columns (calculated by table_init())
  struct mempool *pool;			// Memory pool used for storing table data. Contains global state
					// and data of the current row.
  struct mempool_state pool_state;	// State of the pool after the table is initialized, i.e., before
					// per-row data have been allocated.

  char **col_str_ptrs;			// Values of cells in the current row (allocated from the pool)

  uint *column_order;			// [*] Order of the columns in the print-out of the table
  uint cols_to_output;			// [*] Number of columns that are printed
  const char *col_delimiter;		// [*] Delimiter that is placed between columns
  const char *append_delimiter;		// [*] Separator of multiple values in a single cell (see table_append_...())
  uint print_header;			// [*] 0 indicates that table header should not be printed

  struct fastbuf *out;			// [*] Fastbuffer to which the table is printed
  int last_printed_col;			// Index of the last column which was set. -1 indicates start of row.
					// Used for example for appending to the current column.
  int row_printing_started;		// Indicates that a row has been started. Duplicates last_printed_col, but harmlessly.
  struct fbpool fb_col_out;		// Per-cell fastbuf, see table_col_fbstart()
  int col_out;				// Index of the column that is currently printed using fb_col_out

  // Back-end used for table formatting and its private data
  struct table_formatter *formatter;
  void *data;
};

/****
 * In most cases, table descriptions are constructed using the following macros.
 * See the examples above for more details.
 *
 *  * `TBL_COLUMNS` indicates the start of definition of columns
 *  * `TBL_COL_`'type'`(name, width)` defines a column of a given type
 *  * `TBL_COL_`'type'`_FMT(name, width, fmt)` defines a column with a custom format string
 *  * `TBL_COL_END` ends the column definitions
 *  * `TBL_COL_ORDER` specifies custom ordering of columns in the output
 *  * `TBL_COL_DELIMITER` and `TBL_APPEND_DELIMITER` override default delimiters
 *  * `TBL_OUTPUT_HUMAN_READABLE` requests human-readable formatting (this is the default)
 *  * `TBL_OUTPUT_MACHINE_READABLE` requests machine-readable TSV output
 *  * `TBL_OUTPUT_BLOCKLINE` requests block formatting (each cell printed a pair of a key and value on its own line)
 *
 ***/

#define TBL_COL_STR(_name, _width)            { .name = _name, .width = _width, .fmt = "%s", .type = COL_TYPE_STR }
#define TBL_COL_INT(_name, _width)            { .name = _name, .width = _width, .fmt = "%d", .type = COL_TYPE_INT }
#define TBL_COL_S64(_name, _width)            { .name = _name, .width = _width, .fmt = "%lld", .type = COL_TYPE_S64 }
#define TBL_COL_UINT(_name, _width)           { .name = _name, .width = _width, .fmt = "%u", .type = COL_TYPE_UINT }
#define TBL_COL_U64(_name, _width)            { .name = _name, .width = _width, .fmt = "%llu", .type = COL_TYPE_U64 }
#define TBL_COL_INTMAX(_name, _width)         { .name = _name, .width = _width, .fmt = "%jd", .type = COL_TYPE_INTMAX }
#define TBL_COL_UINTMAX(_name, _width)        { .name = _name, .width = _width, .fmt = "%ju", .type = COL_TYPE_UINTMAX }
#define TBL_COL_HEXUINT(_name, _width)        { .name = _name, .width = _width, .fmt = "0x%x", .type = COL_TYPE_UINT }
#define TBL_COL_DOUBLE(_name, _width, _prec)  { .name = _name, .width = _width, .fmt = "%." #_prec "lf", .type = COL_TYPE_DOUBLE }
#define TBL_COL_BOOL(_name, _width)           { .name = _name, .width = _width, .fmt = "%s", .type = COL_TYPE_BOOL }
#define TBL_COL_ANY(_name, _width)            { .name = _name, .width = _width, .fmt = 0, .type = COL_TYPE_ANY }

#define TBL_COL_STR_FMT(_name, _width, _fmt)            { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_STR }
#define TBL_COL_INT_FMT(_name, _width, _fmt)            { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_INT }
#define TBL_COL_S64_FMT(_name, _width, _fmt)            { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_S64 }
#define TBL_COL_UINT_FMT(_name, _width, _fmt)           { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_UINT }
#define TBL_COL_U64_FMT(_name, _width, _fmt)            { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_U64 }
#define TBL_COL_INTMAX_FMT(_name, _width, _fmt)         { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_INTMAX }
#define TBL_COL_UINTMAX_FMT(_name, _width, _fmt)        { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_UINTMAX }
#define TBL_COL_HEXUINT_FMT(_name, _width, _fmt)        { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_UINT }
#define TBL_COL_BOOL_FMT(_name, _width, _fmt)           { .name = _name, .width = _width, .fmt = _fmt, .type = COL_TYPE_BOOL }

#define TBL_COL_END { .name = 0, .width = 0, .fmt = 0, .type = COL_TYPE_LAST }

#define TBL_COLUMNS  .columns = (struct table_column [])
#define TBL_COL_ORDER(order) .column_order = (int *) order, .cols_to_output = ARRAY_SIZE(order)
#define TBL_COL_DELIMITER(_delimiter_) .col_delimiter = _delimiter_
#define TBL_APPEND_DELIMITER(_delimiter_) .append_delimiter = _delimiter_

#define TBL_OUTPUT_HUMAN_READABLE     .formatter = &table_fmt_human_readable
#define TBL_OUTPUT_BLOCKLINE          .formatter = &table_fmt_blockline
#define TBL_OUTPUT_MACHINE_READABLE   .formatter = &table_fmt_machine_readable

/**
 * Initialize a table definition. The structure should already contain
 * the definitions of columns.
 **/
void table_init(struct table *tbl);

/** Destroy a table definition, freeing all memory used by it. **/
void table_cleanup(struct table *tbl);

/**
 * Start printing of a table. This is a prerequisite to setting of column values.
 * After @table_start() is called, it is no longer possible to change parameters
 * of the table by `table_set_`'something' nor by direct access to the table structure.
 **/
void table_start(struct table *tbl, struct fastbuf *out);

/**
 * This function must be called after all the rows of the current table are printed,
 * making the table structure ready for the next table. You can call `table_set_`'something'
 * between @table_end() and @table_start().
 **/
void table_end(struct table *tbl);

/***
 * Filling tables with data
 * ------------------------
 *
 * For each column type, there are functions for filling of cells
 * of the particular type:
 *
 *   * `table_col_`'type'`(table, idx, value)` sets the cell in column `idx`
 *     to the `value`
 *   * `table_col_`'type'`_fmt(table, idx, fmt, ...)` does the same with
 *     a custom printf-like format string
 *   * `table_col_`'type'`_name(table, name, value)` refers to the column
 *     by its name instead of its index.
 *   * `table_append_`'type'`(table, value)` appends a value to the most
 *     recently accessed cell.
 ***/

#define TABLE_COL_PROTO(_name_, _type_) void table_col_##_name_(struct table *tbl, int col, _type_ val);\
  void table_col_##_name_##_name(struct table *tbl, const char *col_name, _type_ val);\
  void table_col_##_name_##_fmt(struct table *tbl, int col, const char *fmt, _type_ val) FORMAT_CHECK(printf, 3, 0);

// table_col_<type>_fmt has one disadvantage: it is not possible to
// check whether fmt contains format that contains formatting that is
// compatible with _type_

TABLE_COL_PROTO(int, int);
TABLE_COL_PROTO(uint, uint);
TABLE_COL_PROTO(double, double);
TABLE_COL_PROTO(str, const char *);
TABLE_COL_PROTO(intmax, intmax_t);
TABLE_COL_PROTO(uintmax, uintmax_t);
TABLE_COL_PROTO(s64, s64);
TABLE_COL_PROTO(u64, u64);

void table_col_bool(struct table *tbl, int col, uint val);
void table_col_bool_name(struct table *tbl, const char *col_name, uint val);
void table_col_bool_fmt(struct table *tbl, int col, const char *fmt, uint val);
#undef TABLE_COL_PROTO

#define TABLE_APPEND_PROTO(_name_, _type_) void table_append_##_name_(struct table *tbl, _type_ val)
TABLE_APPEND_PROTO(int, int);
TABLE_APPEND_PROTO(uint, uint);
TABLE_APPEND_PROTO(double, double);
TABLE_APPEND_PROTO(str, const char *);
TABLE_APPEND_PROTO(intmax, intmax_t);
TABLE_APPEND_PROTO(uintmax, uintmax_t);
TABLE_APPEND_PROTO(s64, s64);
TABLE_APPEND_PROTO(u64, u64);
void table_append_bool(struct table *tbl, int val);
#undef TABLE_APPEND_PROTO

/**
 * Set a particular cell of the current row to a string formatted
 * by sprintf(). This function can set a column of an arbitrary type.
 **/
void table_col_printf(struct table *tbl, int col, const char *fmt, ...) FORMAT_CHECK(printf, 3, 4);

/**
 * Append a string formatted by sprintf() to the most recently filled cell.
 * This function can work with columns of an arbitrary type.
 **/
void table_append_printf(struct table *tbl, const char *fmt, ...) FORMAT_CHECK(printf, 2, 3);

/**
 * Alternatively, a string cell can be constructed as a stream.
 * This function creates a fastbuf stream connected to the contents
 * of the particular cell. Before you close the stream by @table_col_fbend(),
 * no other operations with cells are allowed.
 **/
struct fastbuf *table_col_fbstart(struct table *tbl, int col);

/**
 * Close the stream that is used for printing of the current column.
 **/
void table_col_fbend(struct table *tbl);

/**
 * Called when all cells of the current row have their values filled in.
 * It sends the completed row to the output stream.
 **/
void table_end_row(struct table *tbl);

/**
 * Resets data in current row.
 **/
void table_reset_row(struct table *tbl);

/***
 * Configuration functions
 * -----------------------
 ***/

/**
 * Find the index of a column with name @col_name. Returns -1 if there is no such column.
 **/
int table_get_col_idx(struct table *tbl, const char *col_name);

/**
 * Returns a comma-and-space-separated list of column names, allocated from table's internal
 * memory pool.
 **/
const char *table_get_col_list(struct table *tbl);

/**
 * Sets the order in which the columns are printed. The @col_order parameter is used until @table_end() or
 * @table_cleanup() is called. The table stores only the pointer and the memory pointed to by @col_order is
 * allocated and deallocated by the caller.
 **/
void table_set_col_order(struct table *tbl, int *col_order, int col_order_size);

/**
 * Returns 1 if col_idx will be printed, 0 otherwise.
 **/
int table_col_is_printed(struct table *tbl, uint col_idx);

/**
 * Sets the order in which the columns are printed. The specification is a string with comma-separated column
 * names. Returns NULL for success and an error message otherwise. The string is not referenced after
 * this function returns.
 **/
const char *table_set_col_order_by_name(struct table *tbl, const char *col_order);

/**
 * Sets table formatter. See below for the list of formatters.
 **/
void table_set_formatter(struct table *tbl, struct table_formatter *fmt);

/**
 * Set a table option. All options have a key and a value. Currently,
 * the following keys are defined (other keys can be accepted by formatters):
 *
 * [options="header"]
 * |===================================================================================================
 * | key	| value				| meaning
 * | `header`	| 0 or 1			| set whether a table header should be printed
 * | `noheader`	| 'none'			| equivalent to `header`=0
 * | `cols`	| comma-separated column list	| set order of columns
 * | `fmt`	| `human`/`machine`/`block`	| set table formatter to one of the built-in formatters
 * | `col-delim`| string			| set column delimiter
 * |===================================================================================================
 **/
const char *table_set_option_value(struct table *tbl, const char *key, const char *value);

/**
 * Sets a table option given as 'key'`:`'value' or 'key' (with no value).
 **/
const char *table_set_option(struct table *tbl, const char *opt);

/**
 * Sets several table option in 'key'`:`'value' form, stored in a growing array.
 * This is frequently used for options given on the command line.
 **/
const char *table_set_gary_options(struct table *tbl, char **gary_table_opts);

/***
 * Formatters
 * ----------
 *
 * Transformation of abstract cell data to the characters in the output stream
 * is under control of a formatter (which serves as a back-end of the table printer).
 * There are several built-in formatters, but you can define your own.
 *
 * A formatter is described by a structure, which contains pointers to several
 * call-back functions, which are called by the table printer at specific occasions.
 *
 * The formatter can keep its internal state in the `data` field of `struct table`
 * and allocate temporary data from the table's memory pool. Memory allocated in
 * the `row_output` call-back is freed before the next row begins. Memory allocated
 * between the beginning of `table_start` and the end of `table_end` is freed after
 * `table_end`. Memory allocated by `process_option` when no table is started
 * is kept until @table_cleanup().
 ***/

/** Definition of a formatter back-end. **/
struct table_formatter {
  void (*row_output)(struct table *tbl);	// [*] Function that outputs one row
  void (*table_start)(struct table *tbl);	// [*] table_start callback (optional)
  void (*table_end)(struct table *tbl);		// [*] table_end callback (optional)
  bool (*process_option)(struct table *tbl, const char *key, const char *value, const char **err);
	// [*] Process table option and possibly return an error message (optional)
};

/** Standard formatter for human-readable output. **/
extern struct table_formatter table_fmt_human_readable;

/** Standard formatter for machine-readable output (tab-separated values). **/
extern struct table_formatter table_fmt_machine_readable;

/**
 * Standard formatter for block output. Each cell is output on its own line
 * of the form `column_name: value`. Rows are separated by blank lines.
 **/
extern struct table_formatter table_fmt_blockline;

#endif