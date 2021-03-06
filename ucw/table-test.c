/*
 *	Types used in table printer
 *
 *	(c) 2014 Robert Kessl <robert.kessl@economia.cz>
 */

#include <ucw/lib.h>
#include <ucw/table.h>
#include <ucw/xtypes-extra.h>
#include <ucw/opt.h>
#include <stdio.h>

enum test_table_cols {
  TEST_COL0_STR, TEST_COL1_INT, TEST_COL2_UINT, TEST_COL3_BOOL, TEST_COL4_DOUBLE, TEST_COL5_SIZE, TEST_COL6_TIME
};

static struct table_col_instance test_column_order[] = { TBL_COL(TEST_COL3_BOOL), TBL_COL(TEST_COL4_DOUBLE),
                       TBL_COL(TEST_COL2_UINT), TBL_COL(TEST_COL1_INT), TBL_COL(TEST_COL0_STR), TBL_COL_ORDER_END };

static struct table_template test_tbl = {
  TBL_COLUMNS {
    [TEST_COL0_STR] = TBL_COL_STR("col0_str", 20),
    [TEST_COL1_INT] = TBL_COL_INT("col1_int", 8),
    [TEST_COL2_UINT] = TBL_COL_UINT("col2_uint", 9),
    [TEST_COL3_BOOL] = TBL_COL_BOOL_FMT("col3_bool", 9, XTYPE_FMT_PRETTY),
    [TEST_COL4_DOUBLE] = TBL_COL_DOUBLE_FMT("col4_double", 11, XTYPE_FMT_PRETTY),
    [TEST_COL5_SIZE] = TBL_COL_SIZE("col5_size", 11),
    [TEST_COL6_TIME] = TBL_COL_TIMESTAMP("col6_timestamp", 20),
    TBL_COL_END
  },
  TBL_COL_ORDER(test_column_order),
  TBL_FMT_HUMAN_READABLE,
  TBL_COL_DELIMITER("\t"),
};

enum test_default_order_cols {
  TEST_DEFAULT_ORDER_COL0_INT, TEST_DEFAULT_ORDER_COL1_INT, TEST_DEFAULT_ORDER_COL2_INT
};

static struct table_template test_default_order_tbl = {
  TBL_COLUMNS {
    [TEST_DEFAULT_ORDER_COL0_INT] = TBL_COL_INT("col0_int", 8),
    [TEST_DEFAULT_ORDER_COL1_INT] = TBL_COL_INT("col1_int", 9),
    [TEST_DEFAULT_ORDER_COL2_INT] = TBL_COL_INT("col2_int", 9),
    TBL_COL_END
  },
  TBL_FMT_HUMAN_READABLE,
  TBL_COL_DELIMITER("\t"),
};

static void do_default_order_test(struct fastbuf *out)
{
  struct table *tbl = table_init(&test_default_order_tbl);

  table_start(tbl, out);

  table_col_int(tbl, TEST_DEFAULT_ORDER_COL0_INT, 0);
  table_col_int(tbl, TEST_DEFAULT_ORDER_COL1_INT, 1);
  table_col_int(tbl, TEST_DEFAULT_ORDER_COL2_INT, 2);
  table_end_row(tbl);

  table_col_int(tbl, TEST_DEFAULT_ORDER_COL0_INT, 10);
  table_col_int(tbl, TEST_DEFAULT_ORDER_COL1_INT, 11);
  table_col_int(tbl, TEST_DEFAULT_ORDER_COL2_INT, 12);
  table_end_row(tbl);

  table_end(tbl);
  table_cleanup(tbl);
}

/**
 * tests: table_col_int, table_col_uint, table_col_bool, table_col_double, table_col_printf
 **/
static void do_print1(struct table *test_tbl)
{
  table_col_str(test_tbl, TEST_COL0_STR, "sdsdf");
  table_col_int(test_tbl, TEST_COL1_INT, -10);
  table_col_int(test_tbl, TEST_COL1_INT, 10000);
  table_col_uint(test_tbl, TEST_COL2_UINT, 10);
  table_col_printf(test_tbl, TEST_COL2_UINT, "XXX-%u", 22222);
  table_col_bool(test_tbl, TEST_COL3_BOOL, 1);
  table_col_double(test_tbl, TEST_COL4_DOUBLE, 1.5);
  table_col_size(test_tbl, TEST_COL5_SIZE, (1024LLU*1024LLU*1024LLU*5LLU));
  table_col_timestamp(test_tbl, TEST_COL6_TIME, 1404305876);
  table_end_row(test_tbl);

  table_col_str(test_tbl, TEST_COL0_STR, "test");
  table_col_int(test_tbl, TEST_COL1_INT, -100);
  table_col_uint(test_tbl, TEST_COL2_UINT, 100);
  table_col_bool(test_tbl, TEST_COL3_BOOL, 0);
  table_col_double(test_tbl, TEST_COL4_DOUBLE, 1.5);
  table_col_size(test_tbl, TEST_COL5_SIZE, (1024LLU*1024LLU*1024LLU*2LLU));
  table_col_timestamp(test_tbl, TEST_COL6_TIME, 1404305909);
  table_end_row(test_tbl);
}

static char **cli_table_opts;

enum test_type_t {
  TEST_DEFAULT_COLUMN_ORDER = 1,
  TEST_INVALID_OPTION = 2,
  TEST_INVALID_ORDER = 3
};

static int test_to_perform = -1;

static struct opt_section table_printer_opts = {
  OPT_ITEMS {
    OPT_HELP("Options:"),
    OPT_STRING_MULTIPLE('T', "table", cli_table_opts, OPT_REQUIRED_VALUE, "\tSets options for the table."),
    OPT_SWITCH('d', 0, test_to_perform, TEST_DEFAULT_COLUMN_ORDER, OPT_SINGLE, "\tRun the test that uses the default column order."),
    OPT_SWITCH('i', 0, test_to_perform, TEST_INVALID_OPTION, OPT_SINGLE, "\tTest the output for invalid option."),
    OPT_SWITCH('n', 0, test_to_perform, TEST_INVALID_ORDER, OPT_SINGLE, "\tTest the output for invalid names of columns for the 'cols' option."),
    OPT_END
  }
};

static void process_command_line_opts(char *argv[], struct table *tbl)
{
  GARY_INIT(cli_table_opts, 0);

  opt_parse(&table_printer_opts, argv+1);
  const char *err = table_set_gary_options(tbl, cli_table_opts);
  if(err) {
    opt_failure("error while setting cmd line options: %s", err);
  }

  GARY_FREE(cli_table_opts);
}

static bool user_defined_option(struct table *tbl UNUSED, const char *key, const char *value, const char **err UNUSED)
{
  if(value == NULL && strcmp(key, "novaluekey") == 0) {
    printf("setting key: %s; value: (null)\n", key);
    return 1;
  }
  if(value != NULL && strcmp(value, "value") == 0 &&
     key != NULL && strcmp(key, "valuekey") == 0) {
    printf("setting key: %s; value: %s\n", key, value);
    return 1;
  }
  return 0;
}

static void test_option_parser(struct table *tbl)
{
  struct table_formatter test_option_fmtr = table_fmt_human_readable;
  test_option_fmtr.process_option = user_defined_option;

  const struct table_formatter *tmp_fmtr = tbl->formatter;
  tbl->formatter = &test_option_fmtr;

  const char *rv = table_set_option(tbl, "invalid:option");
  if(rv) printf("Tableprinter option parser returned error: \"%s\".\n", rv);

  rv = table_set_option(tbl, "invalid");
  if(rv) printf("Tableprinter option parser returned error: \"%s\".\n", rv);

  rv = table_set_option(tbl, "novaluekey");
  if(rv) printf("Tableprinter option parser returned error: \"%s\".\n", rv);

  rv = table_set_option(tbl, "valuekey:value");
  if(rv) printf("Tableprinter option parser returned error: \"%s\".\n", rv);

  tbl->formatter = tmp_fmtr;
}

int main(int argc UNUSED, char **argv)
{
  struct fastbuf *out;
  out = bfdopen_shared(1, 4096);

  struct table *tbl = table_init(&test_tbl);

  process_command_line_opts(argv, tbl);

  const char *rv = NULL;
  switch(test_to_perform) {
  case TEST_INVALID_ORDER:
    rv = table_set_option(tbl, "cols:test_col0_str,test_col1_int,xxx");
    if(rv) printf("Tableprinter option parser returned: '%s'.\n", rv);
    return 0;
  case TEST_DEFAULT_COLUMN_ORDER:
    do_default_order_test(out);
    bclose(out);
    return 0;
  case TEST_INVALID_OPTION:
    test_option_parser(tbl);
    bclose(out);
    return 0;
  };

  table_start(tbl, out);
  do_print1(tbl);
  table_end(tbl);
  table_cleanup(tbl);

  bclose(out);

  return 0;
}
