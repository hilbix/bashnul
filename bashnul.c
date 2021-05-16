/* bash cannot read NUL:
 * -e: encode 00 to 01 02 and 01 to 01 03
 * -d: decode 01 02 to 00 and 01 03 to 01
 *
 * This Works is placed under the terms of the Copyright Less License,
 * see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

static void
OOPS(int err, const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  if (errno)
    fprintf(stderr, ": %s", strerror(errno));
  fprintf(stderr, "\n");
  exit(err);
}

static void
writer(const char *buf, int len)
{
  int	loop;

  if (!len)
    return;
  if (len<0)
    OOPS(23, "FATAL INTERNAL ERROR, len=%d", len);
  for (loop=0; len; )
    {
      int	put;

      if (++loop>=1000)
        OOPS(23, "write is looping");
      put	= write(1, buf, (size_t)len);
      if (put<0)
        {
          if (errno == EAGAIN || errno==EINTR)
            continue;
          OOPS(23, "write failed");
        }
      if (!put)
        exit(23);	/* silently tell error (in case output goes away)	*/
      len	-= put;
      buf	+= put;
    }
}

static int
bashnul(void (*fn)(char *, int))
{
  char	buf[10*BUFSIZ];
  int	loop;

  for (loop=0; ++loop<1000; )
    {
      int	got;

      got	= read(0, buf, sizeof buf);
      if (got<0)
        {
          if (errno == EAGAIN || errno==EINTR)
            continue;
          OOPS(23, "read failed");
        }
      fn(buf, got);
      if (!got)
        return 0;
      loop	= 0;
    }
  OOPS(23, "read is looping");
  return 23;
}

static void
encode(char *buf, int len)
{
  char *pos;

  for (pos=buf; len; pos++, len--)
    switch (*pos)
      {
      char	c;
      int	n;

      case 0:	/* 00 => 01 02	*/
      case 1:	/* 01 => 01 03	*/
        c	= *pos;
        *pos	= 1;
        n	= pos-buf+1;
        writer(buf, n);
        buf	= pos;
        *pos	= c+2;
        break;
      }
  writer(buf, pos-buf);
}

static void
decode(char *buf, int len)
{
  static int	dangling;
  char		*pos;

  if (!len && dangling)
    OOPS(23, "stream ended with 01, expected at least 1 byte to follow");
  for (pos=buf; len; pos++, len--)
    if (dangling)
      switch (*pos)
        {
        default:
          OOPS(23, "encountered unknown 01 %02x sequence, only 01 02 or 01 03 are valid!", (unsigned char)*pos);
        case 2:
        case 3:
          *pos		-= 2;
          dangling	= 0;
          break;
        }
    else
      switch (*pos)
        {
        case 0:
          OOPS(23, "encountered NUL, should have been escaped to 01 02");
        case 1:
          writer(buf, pos-buf);
          buf		= pos+1;
          dangling	= 1;
          break;
        }
  writer(buf, pos-buf);
}

int
main(int argc, char **argv)
{
  const char	*lc;
  char		arg;

  if (argc!=2 || argv[1][0]!='-' || ((arg=argv[1][1])!='d' && arg!='e') || argv[1][2])
    OOPS(42
        , "Usage: %s -d|-e\n"
          "\tescapes (-e) or deescaes (-d) 00 into 01 02 and 01 into 01 03\n"
          "\tneeds to run under LC_ALL=C"
        , argv[0]);

  lc	= getenv("LC_ALL");
  if (!lc || strcmp(lc, "C"))
    OOPS(23, "please set LC_ALL=C");

  return bashnul(arg=='e' ? encode : decode);
}

