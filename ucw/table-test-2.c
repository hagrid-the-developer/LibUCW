/*
 *	Unit tests of table printer
 *
 *	(c) 2014 Robert Kessl <robert.kessl@economia.cz>
 */

#include <ucw/lib.h>
#include <ucw/table.h>
#include <ucw/xtypes-extra.h>
#include <ucw/opt.h>
#include <stdio.h>

enum test_table_cols {
  TEST_COL0_SIZE, TEST_COL1_TS
};

static struct table_template test_tbl = {
  TBL_COLUMNS {
    [TEST_COL0_SIZE] = TBL_COL_SIZE_FMT("size", 15, XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_BYTE)),
    [TEST_COL1_TS] = TBL_COL_TIMESTAMP("ts", 20),
    TBL_COL_END
  },
  TBL_FMT_HUMAN_READABLE,
};

static void do_test(void)
{
  struct fastbuf *out;
  out = bfdopen_shared(1, 4096);
  struct table *tbl = table_init(&test_tbl);
  table_start(tbl, out);

  u64 test_time = 1403685533;
  s64 test_size = 4LU*(1024LU * 1024LU * 1024LU);

  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  tbl->column_order[TEST_COL0_SIZE].fmt = XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_KILOBYTE);
  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  tbl->column_order[TEST_COL0_SIZE].fmt = XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_MEGABYTE);
  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  tbl->column_order[TEST_COL0_SIZE].fmt = XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_GIGABYTE);
  tbl->column_order[TEST_COL1_TS].fmt = XT_TIMESTAMP_FMT_DATETIME;
  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  test_size = test_size * 1024LU;
  tbl->column_order[TEST_COL0_SIZE].fmt = XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_TERABYTE);
  tbl->column_order[TEST_COL1_TS].fmt = XT_TIMESTAMP_FMT_DATETIME;
  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  table_end(tbl);
  table_cleanup(tbl);

  bclose(out);
}

static struct table_template test_tbl2 = {
  TBL_COLUMNS {
    [TEST_COL0_SIZE] = TBL_COL_SIZE_FMT("size", 15, XT_SIZE_FMT_UNIT(XT_SIZE_UNIT_BYTE)),
    [TEST_COL1_TS] = TBL_COL_TIMESTAMP("ts", 20),
    TBL_COL_END
  },
  TBL_FMT_HUMAN_READABLE,
};

static void do_test2(void)
{
  struct fastbuf *out;
  out = bfdopen_shared(1, 4096);
  struct table *tbl = table_init(&test_tbl2);
  table_set_col_order_by_name(tbl, "");
  const char *err = table_set_option_value(tbl, "cols", "size[MB,TB,KB],size[MB],size[GB],size[TB],size[auto],ts[datetime],ts[timestamp],ts[],size[MB,KB]");
  if(err) {
    opt_failure("err in table_set_option_value: '%s'.", err);
    abort();
  }
  table_start(tbl, out);

  u64 test_time = 1403685533;
  s64 test_size = 4LU*(1024LU * 1024LU);

  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  test_size = test_size * 1024LU;

  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  test_size = test_size * 1024LU;

  table_col_size(tbl, TEST_COL0_SIZE, test_size);
  table_col_timestamp(tbl, TEST_COL1_TS, test_time);
  table_end_row(tbl);

  table_end(tbl);
  table_cleanup(tbl);

  bclose(out);
}

static void do_test3(void)
{
  struct fastbuf *out;
  out = bfdopen_shared(1, 4096);

  struct table *tbl = table_init(&test_tbl2);
  table_set_col_order_by_name(tbl, "");
  const char *err = table_set_option_value(tbl, "cols", "size[MB,TB,KB],size[MB],size[GB],size[TB],size[auto],ts[datetime],ts[timestamp],size[MB,KB]");
  bprintf(out, "Error occured: %s\n", err);
  table_cleanup(tbl);


  tbl = table_init(&test_tbl2);
  table_set_col_order_by_name(tbl, "");
  err = table_set_option_value(tbl, "cols", "size[MB,TB,KB],size[MB],size[GB],size[TB],size[auto],ts[,,datetime,timestamp],ts[],size[MB,KB");
  bprintf(out, "Error occured: %s\n", err);
  table_cleanup(tbl);


  tbl = table_init(&test_tbl2);
  table_set_col_order_by_name(tbl, "");
  err = table_set_option_value(tbl, "cols", "size[MB,TB,KB");
  bprintf(out, "Error occured: %s\n", err);
  table_cleanup(tbl);


  tbl = table_init(&test_tbl2);
  table_set_col_order_by_name(tbl, "");
  err = table_set_option_value(tbl, "cols", "size[MB,TB,KB]");
  bprintf(out, "Error occured: %s\n", err);
  table_cleanup(tbl);

  bclose(out);
}

int main(int argc UNUSED, char **argv UNUSED)
{
  do_test();
  do_test2();
  do_test3();

  return 0;
}

