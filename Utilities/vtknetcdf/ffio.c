/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *  See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */

#include "ncconfig.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>  /* DEBUG */
#include <errno.h>
#ifndef ENOERR
#define ENOERR 0
#endif
#include <fcntl.h>
#include <ffio.h>
#include <unistd.h>
#include <string.h>

#include "ncio.h"
#include "fbits.h"
#include "rnd.h"

#if !defined(NDEBUG) && !defined(X_INT_MAX)
#define  X_INT_MAX 2147483647
#endif
#if 0 /* !defined(NDEBUG) && !defined(X_ALIGN) */
#define  X_ALIGN 4
#endif

#define ALWAYS_NC_SHARE 0 /* DEBUG */

/* Begin OS */

/*
 * What is the preferred I/O block size?
 * (This becomes the default *sizehint == ncp->chunk in the higher layers.)
 * TODO: What is the the best answer here?
 */
static size_t
blksize(int fd)
{
  struct ffc_stat_s sb;
  struct ffsw sw;
  if (fffcntl(fd, FC_STAT, &sb, &sw) > -1)
  {
    if(sb.st_oblksize > 0)
      return (size_t) sb.st_oblksize;
  }
  /* else, silent in the face of error */
  return (size_t) 32768;
}

/*
 * Sortof like ftruncate, except won't make the
 * file shorter.
 */
static int
fgrow(const int fd, const off_t len)
{
  struct ffc_stat_s sb;
  struct ffsw sw;
  if (fffcntl(fd, FC_STAT, &sb, &sw) < 0)
    return errno;
  if (len < sb.st_size)
    return ENOERR;
  {
    const long dumb = 0;
      /* cache current position */
    const off_t pos = ffseek(fd, 0, SEEK_CUR);
    if(pos < 0)
      return errno;
    if (ffseek(fd, len-sizeof(dumb), SEEK_SET) < 0)
      return errno;
    if(ffwrite(fd, (void *)&dumb, sizeof(dumb)) < 0)
      return errno;
    if (ffseek(fd, pos, SEEK_SET) < 0)
      return errno;
  }
  /* else */
  return ENOERR;
}

/* End OS */
/* Begin ffio */

static int
ffio_pgout(ncio *const nciop, 
  off_t const offset,  const size_t extent,
  const void *const vp, off_t *posp)
{
#ifdef X_ALIGN
  assert(offset % X_ALIGN == 0);
  assert(extent % X_ALIGN == 0);
#endif

  if(*posp != offset)
  {
    if(ffseek(nciop->fd, offset, SEEK_SET) != offset)
    {
      return errno;
    }
    *posp = offset;
  }
  if(ffwrite(nciop->fd, vp, extent) != extent)
  {
    return errno;
  }
  *posp += extent;

  return ENOERR;
}


static int
ffio_pgin(ncio *const nciop,
  off_t const offset, const size_t extent,
  void *const vp, size_t *nreadp, off_t *posp)
{
  int status;
  ssize_t nread;

#ifdef X_ALIGN
  assert(offset % X_ALIGN == 0);
  assert(extent % X_ALIGN == 0);
#endif

  if(*posp != offset)
  {
    if(ffseek(nciop->fd, offset, SEEK_SET) != offset)
    {
      status = errno;
      return status;
    }
    *posp = offset;
  }

  errno = 0;
  nread = ffread(nciop->fd, vp, extent);
  if(nread != extent)
  {
    status = errno;
    if(nread == -1 || status != ENOERR)
      return status;
    /* else it's okay we read 0. */
  }
  *nreadp = nread;
  *posp += nread;

  return ENOERR;
}

/* */

typedef struct ncio_ffio {
  off_t pos;
  /* buffer */
  off_t  bf_offset; 
  size_t  bf_extent;
  size_t  bf_cnt;
  void  *bf_base;
} ncio_ffio;


static int
ncio_ffio_rel(ncio *const nciop, off_t offset, int rflags)
{
  ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;
  int status = ENOERR;

  assert(ffp->bf_offset <= offset);
  assert(ffp->bf_cnt != 0);
  assert(ffp->bf_cnt <= ffp->bf_extent);
#ifdef X_ALIGN
  assert(offset < ffp->bf_offset + X_ALIGN);
  assert(ffp->bf_cnt % X_ALIGN == 0 );
#endif

  if(fIsSet(rflags, RGN_MODIFIED))
  {
    if(!fIsSet(nciop->ioflags, NC_WRITE))
      return EPERM; /* attempt to write readonly file */

    status = ffio_pgout(nciop, ffp->bf_offset,
      ffp->bf_cnt,
      ffp->bf_base, &ffp->pos);
    /* if error, invalidate buffer anyway */
  }
  ffp->bf_offset = OFF_NONE;
  ffp->bf_cnt = 0;
  return status;
}


static int
ncio_ffio_get(ncio *const nciop,
    off_t offset, size_t extent,
    int rflags,
    void **const vpp)
{
  ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;
  int status = ENOERR;
#ifdef X_ALIGN
  size_t rem;
#endif
  
  if(fIsSet(rflags, RGN_WRITE) && !fIsSet(nciop->ioflags, NC_WRITE))
    return EPERM; /* attempt to write readonly file */

  assert(extent != 0);
  assert(extent < X_INT_MAX); /* sanity check */
  assert(offset < X_INT_MAX); /* sanity check */

  assert(ffp->bf_cnt == 0);

#ifdef X_ALIGN
  /* round to seekable boundaries */
  rem = offset % X_ALIGN;
  if(rem != 0)
  {
    offset -= rem;
    extent += rem;
  }

  {
    const size_t rndup = extent % X_ALIGN;
    if(rndup != 0)
      extent += X_ALIGN - rndup;
  }

  assert(offset % X_ALIGN == 0);
  assert(extent % X_ALIGN == 0);
#endif

  if(ffp->bf_extent < extent)
  {
    if(ffp->bf_base != NULL)
    {
      free(ffp->bf_base);
      ffp->bf_base = NULL;
      ffp->bf_extent = 0;
    }
    assert(ffp->bf_extent == 0);
    ffp->bf_base = malloc(extent);
    if(ffp->bf_base == NULL)
      return ENOMEM;
    ffp->bf_extent = extent;
  }

  status = ffio_pgin(nciop, offset,
     extent,
     ffp->bf_base,
     &ffp->bf_cnt, &ffp->pos);
  if(status != ENOERR)
    return status;

  ffp->bf_offset = offset;

  if(ffp->bf_cnt < extent)
  {
    (void) memset((char *)ffp->bf_base + ffp->bf_cnt, 0,
      extent - ffp->bf_cnt);
    ffp->bf_cnt = extent;
  }


#ifdef X_ALIGN
  *vpp = (char *)ffp->bf_base + rem;
#else
  *vpp = (char *)ffp->bf_base;
#endif
  return ENOERR;
}


static int
ncio_ffio_move(ncio *const nciop, off_t to, off_t from,
      size_t nbytes, int rflags)
{
  int status = ENOERR;
  off_t lower = from;  
  off_t upper = to;
  char *base;
  size_t diff = upper - lower;
  size_t extent = diff + nbytes;

  rflags &= RGN_NOLOCK; /* filter unwanted flags */

  if(to == from)
    return ENOERR; /* NOOP */
  
  if(to > from)
  {
    /* growing */
    lower = from;  
    upper = to;
  }
  else
  {
    /* shrinking */
    lower = to;
    upper = from;
  }

  diff = upper - lower;
  extent = diff + nbytes;

  status = ncio_ffio_get(nciop, lower, extent, RGN_WRITE|rflags,
      (void **)&base);

  if(status != ENOERR)
    return status;

  if(to > from)
    (void) memmove(base + diff, base, nbytes); 
  else
    (void) memmove(base, base + diff, nbytes); 
    
  (void) ncio_ffio_rel(nciop, lower, RGN_MODIFIED);

  return status;
}


static int
ncio_ffio_sync(ncio *const nciop)
{
  if(ffflush(nciop->fd) < 0)
    return errno;
  return ENOERR;
}

static void
ncio_ffio_free(void *const pvt)
{
  ncio_ffio *ffp = (ncio_ffio *)pvt;
  if(ffp == NULL)
    return;

  if(ffp->bf_base != NULL)
  {
    free(ffp->bf_base);
    ffp->bf_base = NULL;
    ffp->bf_offset = OFF_NONE;
    ffp->bf_extent = 0;
    ffp->bf_cnt = 0;
  }
}


static int
ncio_ffio_init2(ncio *const nciop, size_t *sizehintp)
{
  ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;

  assert(nciop->fd >= 0);

  ffp->bf_extent = *sizehintp;

  assert(ffp->bf_base == NULL);

  /* this is separate allocation because it may grow */
  ffp->bf_base = malloc(ffp->bf_extent);
  if(ffp->bf_base == NULL)
  {
    ffp->bf_extent = 0;
    return ENOMEM;
  }
  /* else */
  return ENOERR;
}


static void
ncio_ffio_init(ncio *const nciop)
{
  ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;

  *((ncio_relfunc **)&nciop->rel) = ncio_ffio_rel; /* cast away const */
  *((ncio_getfunc **)&nciop->get) = ncio_ffio_get; /* cast away const */
  *((ncio_movefunc **)&nciop->move) = ncio_ffio_move; /* cast away const */
  *((ncio_syncfunc **)&nciop->sync) = ncio_ffio_sync; /* cast away const */
  *((ncio_freefunc **)&nciop->free) = ncio_ffio_free; /* cast away const */

  ffp->pos = -1;
  ffp->bf_offset = OFF_NONE;
  ffp->bf_extent = 0;
  ffp->bf_cnt = 0;
  ffp->bf_base = NULL;
}

/* */

static void
ncio_free(ncio *nciop)
{
  if(nciop == NULL)
    return;

  if(nciop->free != NULL)
    nciop->free(nciop->pvt);
  
  free(nciop);
}


static ncio *
ncio_new(const char *path, int ioflags)
{
  size_t sz_ncio = M_RNDUP(sizeof(ncio));
  size_t sz_path = M_RNDUP(strlen(path) +1);
  size_t sz_ncio_pvt;
  ncio *nciop;
 
#if ALWAYS_NC_SHARE /* DEBUG */
  fSet(ioflags, NC_SHARE);
#endif

  if(fIsSet(ioflags, NC_SHARE))
    fprintf(stderr, "NC_SHARE not implemented for ffio\n");

  sz_ncio_pvt = sizeof(ncio_ffio);

  nciop = (ncio *) malloc(sz_ncio + sz_path + sz_ncio_pvt);
  if(nciop == NULL)
    return NULL;
  
  nciop->ioflags = ioflags;
  *((int *)&nciop->fd) = -1; /* cast away const */

  nciop->path = (char *) ((char *)nciop + sz_ncio);
  (void) strcpy((char *)nciop->path, path); /* cast away const */

        /* cast away const */
  *((void **)&nciop->pvt) = (void *)(nciop->path + sz_path);

  ncio_ffio_init(nciop);

  return nciop;
}


/* Public below this point */

/* TODO: Is this reasonable for this platform? */
static const size_t NCIO_MINBLOCKSIZE = 256;
static const size_t NCIO_MAXBLOCKSIZE = 268435456; /* sanity check, about X_SIZE_T_MAX/8 */

int
ncio_create(const char *path, int ioflags,
  size_t initialsz,
  off_t igeto, size_t igetsz, size_t *sizehintp,
  ncio **nciopp, void **const igetvpp)
{
  ncio *nciop;
  char *ControlString;
  int oflags = (O_RDWR|O_CREAT|O_TRUNC);
  int fd;
  int status;
  struct ffsw stat;

  if(initialsz < (size_t)igeto + igetsz)
    initialsz = (size_t)igeto + igetsz;

  fSet(ioflags, NC_WRITE);

  if(path == NULL || *path == 0)
    return EINVAL;

  nciop = ncio_new(path, ioflags);
  if(nciop == NULL)
    return ENOMEM;

  /* TODO: use *sizehintp for input? */
  ControlString = getenv("NETCDF_FFIOSPEC");
  if(ControlString == NULL)
  {
     ControlString="bufa:336:2";
  }

  if(fIsSet(ioflags, NC_NOCLOBBER))
    fSet(oflags, O_EXCL);

  fd = ffopens(path, oflags, 0666, 0, &stat, ControlString);
  if(fd < 0)
  {
    status = errno;
    goto unwind_new;
  }
  *((int *)&nciop->fd) = fd; /* cast away const */

  if(*sizehintp < NCIO_MINBLOCKSIZE || *sizehintp > NCIO_MAXBLOCKSIZE)
  {
    /* Use default */
    *sizehintp = blksize(fd);
  }
  else
  {
    *sizehintp = M_RNDUP(*sizehintp);
  }

  status = ncio_ffio_init2(nciop, sizehintp);
  if(status != ENOERR)
    goto unwind_open;

  if(initialsz != 0)
  {
    status = fgrow(fd, (off_t)initialsz);
    if(status != ENOERR)
      goto unwind_open;
  }

  if(igetsz != 0)
  {
    status = nciop->get(nciop,
        igeto, igetsz,
                          RGN_WRITE,
                          igetvpp);
    if(status != ENOERR)
      goto unwind_open;
  }

  *nciopp = nciop;
  return ENOERR;

unwind_open:
  (void) ffclose(fd);
  /* ?? unlink */
  /*FALLTHRU*/
unwind_new:
  ncio_free(nciop);
  return status;
}


int
ncio_open(const char *path,
  int ioflags,
  off_t igeto, size_t igetsz, size_t *sizehintp,
  ncio **nciopp, void **const igetvpp)
{
  ncio *nciop;
  char *ControlString;
  int oflags = fIsSet(ioflags, NC_WRITE) ? O_RDWR : O_RDONLY;
  int fd;
  int status;
  struct ffsw stat;

  if(path == NULL || *path == 0)
    return EINVAL;

  nciop = ncio_new(path, ioflags);
  if(nciop == NULL)
    return ENOMEM;

  /* TODO: use *sizehintp for input? */
  ControlString = getenv("NETCDF_FFIOSPEC");
  if(ControlString == NULL)
  {
     ControlString="bufa:336:2";
  }

  fd = ffopens(path, oflags, 0, 0, &stat, ControlString);
  if(fd < 0)
  {
    status = errno;
    goto unwind_new;
  }
  *((int *)&nciop->fd) = fd; /* cast away const */

  if(*sizehintp < NCIO_MINBLOCKSIZE || *sizehintp > NCIO_MAXBLOCKSIZE)
  {
    /* Use default */
    *sizehintp = blksize(fd);
  }
  else
  {
    *sizehintp = M_RNDUP(*sizehintp);
  }

  status = ncio_ffio_init2(nciop, sizehintp);
  if(status != ENOERR)
    goto unwind_open;

  if(igetsz != 0)
  {
    status = nciop->get(nciop,
        igeto, igetsz,
                          0,
                          igetvpp);
    if(status != ENOERR)
      goto unwind_open;
  }

  *nciopp = nciop;
  return ENOERR;

unwind_open:
  (void) ffclose(fd);
  /*FALLTHRU*/
unwind_new:
  ncio_free(nciop);
  return status;
}


int 
ncio_close(ncio *nciop, int doUnlink)
{
  int status = ENOERR;

  if(nciop == NULL)
    return EINVAL;

  status = nciop->sync(nciop);

  (void) ffclose(nciop->fd);
  
  if(doUnlink)
    (void) unlink(nciop->path);

  ncio_free(nciop);

  return status;
}
