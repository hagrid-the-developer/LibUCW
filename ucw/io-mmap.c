/*
 *	UCW Library -- Mapping of Files
 *
 *	(c) 1999--2012 Martin Mares <mj@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#include <ucw/lib.h>
#include <ucw/io.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

void *
mmap_file(const char *name, size_t *len, int writeable)
{
  int fd = ucw_open(name, writeable ? O_RDWR : O_RDONLY);
  ucw_stat_t st;
  void *x;

  if (fd < 0)
    die("open(%s): %m", name);
  if (ucw_fstat(fd, &st) < 0)
    die("fstat(%s): %m", name);
  if ((uintmax_t)st.st_size > SIZE_MAX)
    die("mmap(%s): File too large", name);
  if (len)
    *len = st.st_size;
  if (st.st_size)
    {
      x = mmap(NULL, st.st_size, writeable ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, fd, 0);
      if (x == MAP_FAILED)
	die("mmap(%s): %m", name);
    }
  else	/* For empty file, we can return any non-zero address */
    x = "";
  close(fd);
  return x;
}

void
munmap_file(void *start, size_t len)
{
  munmap(start, len);
}
