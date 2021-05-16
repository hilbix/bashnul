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

#define	MAXBUF	(25*BUFSIZ)

struct buf
  {
    int		max, pos;
    char	buf[0];
  };

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

static struct buf *
buf_alloc(int len)
{
  struct buf	*buf;

  buf	= malloc(len + sizeof *buf);
  if (!buf)
    OOPS(23, "out of memory");
  buf->max	= len;
  buf->pos	= 0;
  return buf;
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
bashnul(void (*fn)(struct buf *, struct buf *, int), int rlen)
{
  struct buf	*rbuf, *wbuf;
  int	loop;

  rbuf	= buf_alloc(rlen);		/* small or big	*/
  wbuf	= buf_alloc(MAXBUF*2);		/* always big	*/

  for (loop=0; ++loop<1000; )
    {
      int	got;

      got	= read(0, rbuf->buf+rbuf->pos, (size_t)(rbuf->max-rbuf->pos));
      if (got<0)
        {
          if (errno == EAGAIN || errno==EINTR)
            continue;
          OOPS(23, "read failed");
        }
      rbuf->pos	+= got;
      fn(rbuf, wbuf, got);
      writer(wbuf->buf, wbuf->pos);
      wbuf->pos	= 0;
      if (!got)
        return 0;
      loop	= 0;
    }
  OOPS(23, "read is looping");
  return 23;
}

static void
encode(struct buf *rbuf, struct buf *wbuf, int ign)
{
  char	*r = rbuf->buf;
  char	*w = wbuf->buf+wbuf->pos;
  int	len;

  if (wbuf->max - wbuf->pos < rbuf->pos*2)
    OOPS(23, "internal error, write buffer size too small");
  /* we know here, that rbuf fits into wbuf	*/
  for (len = rbuf->pos; len; r++, len--)
    switch (*r)
      {
      case 0:	/* 00 => 01 02	*/
      case 1:	/* 01 => 01 03	*/
        *w++	= 1;
        *w++	= *r+2;
        break;

      default:
        *w++ = *r;
        break;
      }
  wbuf->pos	= w-wbuf->buf;
  rbuf->pos	= 0;
}

static void
decode(struct buf *rbuf, struct buf *wbuf, int more)
{
  char	*r = rbuf->buf;
  char	*w = wbuf->buf+wbuf->pos;
  int	len;

  if (wbuf->max - wbuf->pos < rbuf->pos)
    OOPS(23, "internal error, write buffer size too small");
  /* we know here, that rbuf fits into wbuf	*/
  for (len = rbuf->pos; len; r++, len--)
    {
      switch (*r)
        {
        case 0:
          OOPS(23, "encountered NUL, should have been escaped to 01 02");
        default:
          *w++	= *r;
          continue;

        case 1:
          break;
        }
      if (--len)
        switch (*++r)
          {
          default:
            OOPS(23, "encountered unknown 01 %02x sequence, only 01 02 or 01 03 are valid!", (unsigned char)*r);
          case 2:
          case 3:
            *w++	= *r-2;
            continue;
          }
      rbuf->buf[0]	= *r;	/* ==1	*/
      len		= 1;
      break;
    }
  wbuf->pos	= w-wbuf->buf;
  rbuf->pos	= len;	/* 0 or 1	*/
  if (len && !more)
    OOPS(23, "stream ended with 01, expected at least 1 byte to follow");
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

  return arg=='e' ? bashnul(encode, MAXBUF) : bashnul(decode, MAXBUF*2);
}

