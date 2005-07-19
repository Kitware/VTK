/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *  See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */
/*#define USE_CWRITE 1 *//*define to use _cwrite instead of write*/

#ifdef SGI64
#define lseek lseek64
#endif

#ifdef __PARAGON__
#include <nx.h>
 static int netcdf_local_is_file_pfs = 0;
#endif

#include "ncconfig.h"
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#ifndef ENOERR
#define ENOERR 0
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#ifdef _MSC_VER /* Microsoft Compilers */
#include <io.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#include "ncio.h"
#include "fbits.h"
#include "rnd.h"

/* #define INSTRUMENT 1 */
#if INSTRUMENT /* debugging */
#undef NDEBUG
#include <stdio.h>
#include "instr.h"
#endif

#undef MIN  /* system may define MIN somewhere and complain */
#define MIN(mm,nn) (((mm) < (nn)) ? (mm) : (nn))

#if 0 /* !defined(NDEBUG) && !defined(X_ALIGN) */
#define  X_ALIGN 4
#else
#undef X_ALIGN
#endif

/*
 * Define the following for debugging.
 */
/* #define ALWAYS_NC_SHARE 1 */

/* Begin OS */
#ifdef __PARAGON__
/* #define ALIGN32 32 */
#ifdef __PUMAGON__
#define POSIXIO_DEFAULT_PAGESIZE (256*1024)
#else
#define POSIXIO_DEFAULT_PAGESIZE (256*1024)
#endif
#ifndef HAVE_ST_BLKSIZE
#define HAVE_ST_BLKSIZE 1
#endif
#if defined(_SC_PAGE_SIZE)
#undef _SC_PAGE_SIZE
#endif
#endif /* __PARAGON__ */
#ifndef POSIXIO_DEFAULT_PAGESIZE
#define POSIXIO_DEFAULT_PAGESIZE 4096
#endif
/*
 * What is the system pagesize?
 */
static size_t
pagesize(void)
{
/* Hmm, aren't standards great? */
#if defined(_SC_PAGE_SIZE) && !defined(_SC_PAGESIZE)
#define _SC_PAGESIZE _SC_PAGE_SIZE
#endif

#ifdef _SC_PAGESIZE
  {
    const long pgsz = sysconf(_SC_PAGESIZE);
    if(pgsz > 0)
      return (size_t) pgsz;
    /* else, silent in the face of error */
  }
#elif defined(HAVE_GETPAGESIZE)
  return (size_t) getpagesize();
#endif
  return (size_t) POSIXIO_DEFAULT_PAGESIZE;
}

/*
 * What is the preferred I/O block size?
 */
static size_t
blksize(int fd)
{
#if defined(HAVE_ST_BLKSIZE)
  struct stat sb;
  if (fstat(fd, &sb) > -1)
  {
#ifdef __PARAGON__
             netcdf_local_is_file_pfs = 0;
             if(sb.st_blksize >= 256*1024)
             {
                netcdf_local_is_file_pfs = 1;
             }
             if(sb.st_blksize < POSIXIO_DEFAULT_PAGESIZE)
             {
                return (size_t) POSIXIO_DEFAULT_PAGESIZE;
             }
             else
             {
    return (size_t) sb.st_blksize;
             }
#else
    return (size_t) sb.st_blksize;
#endif
  }
  /* else, silent in the face of error */
#endif
  return (size_t) 2 * pagesize();
}


/*
 * Sortof like ftruncate, except won't make the
 * file shorter.
 */
static int
fgrow(const int fd, const off_t len)
{
  struct stat sb;
  if (fstat(fd, &sb) < 0)
    return errno;
  if (len < sb.st_size)
    return ENOERR;
#if defined(HAVE_FTRUNCATE)
  if (ftruncate(fd, len) < 0)
    return errno;
#else
  {
    const long dumb = 0;
      /* cache current position */
    const off_t pos = lseek(fd, 0, SEEK_CUR);
    if(pos < 0)
      return errno;
    if (lseek(fd, len-sizeof(dumb), SEEK_SET) < 0)
      return errno;

    if(write(fd, &dumb, sizeof(dumb)) < 0)
      return errno;
    if (lseek(fd, pos, SEEK_SET) < 0)
      return errno;
  }
#endif /* HAVE_FTRUNCATE */
  /* else */
  return ENOERR;
}

/* End OS */
/* Begin px */

static int
px_pgout(ncio *const nciop, 
  off_t const offset,  const size_t extent,
  void *const vp, off_t *posp)
{
#ifdef X_ALIGN
  assert(offset % X_ALIGN == 0);
#endif

  assert(*posp == OFF_NONE || *posp == lseek(nciop->fd, 0, SEEK_CUR));

  if(*posp != offset)
  {
    if(lseek(nciop->fd, offset, SEEK_SET) != offset)
    {
      return errno;
    }
    *posp = offset;
  }
#ifdef __PARAGON__
        if(nciop->is_file_pfs == 1)
        {
    if(_cwrite(nciop->fd, vp, extent) != (ssize_t) extent)
    {
    return errno;
    }
        }
        else
        {
    if(write(nciop->fd, vp, extent) != (ssize_t) extent)
    {
    return errno;
    }
        }
        
#else
  if(write(nciop->fd, vp, extent) != (ssize_t) extent)
  {
    return errno;
  }
#endif
  *posp += extent;

  return ENOERR;
}


static int
px_pgin(ncio *const nciop,
  off_t const offset, const size_t extent,
  void *const vp, size_t *nreadp, off_t *posp)
{
  int status;
  ssize_t nread;

#ifdef X_ALIGN
  assert(offset % X_ALIGN == 0);
  assert(extent % X_ALIGN == 0);
#endif

  assert(*posp == OFF_NONE || *posp == lseek(nciop->fd, 0, SEEK_CUR));

  if(*posp != offset)
  {
    if(lseek(nciop->fd, offset, SEEK_SET) != offset)
    {
      status = errno;
      return status;
    }
    *posp = offset;
  }

  errno = 0;
#ifdef __PARAGON__
        if(nciop->is_file_pfs == 1)
        {
    nread = _cread(nciop->fd, vp, extent);
        }
        else
        {
    nread = read(nciop->fd, vp, extent);
        }
#else
  nread = read(nciop->fd, vp, extent);
#endif
  if(nread != (ssize_t) extent)
  {
    status = errno;
#ifdef __PARAGON__
                if(status == ERDEOF){status=ENOERR;}
#endif
    if(nread == -1 || status != ENOERR)
      return status;
    /* else it's okay we read less than asked for */
    (void) memset((char *)vp + nread, 0, (ssize_t)extent - nread);
  }
  *nreadp = nread;
  *posp += nread;

  return ENOERR;
}


typedef struct ncio_px {
  size_t blksz;
  off_t pos;
  /* buffer */
  off_t  bf_offset; 
  size_t  bf_extent;
  size_t  bf_cnt;
  void  *bf_base;
  int  bf_rflags;
  int  bf_refcount;
  /* chain for double buffering in px_move */
  struct ncio_px *slave;
} ncio_px;


/*ARGSUSED*/
static int
px_rel(ncio_px *const pxp, off_t offset, int rflags)
{
  assert(pxp->bf_offset <= offset
     && offset < pxp->bf_offset + (off_t) pxp->bf_extent);
  assert(pIf(fIsSet(rflags, RGN_MODIFIED),
    fIsSet(pxp->bf_rflags, RGN_WRITE)));

  if(fIsSet(rflags, RGN_MODIFIED))
  {
    fSet(pxp->bf_rflags, RGN_MODIFIED);
  }
  pxp->bf_refcount--;

  return ENOERR;
}

static int
ncio_px_rel(ncio *const nciop, off_t offset, int rflags)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;

  if(fIsSet(rflags, RGN_MODIFIED) && !fIsSet(nciop->ioflags, NC_WRITE))
    return EPERM; /* attempt to write readonly file */

  return px_rel(pxp, offset, rflags);
}

static int
px_get(ncio *const nciop, ncio_px *const pxp,
    off_t offset, size_t extent,
    int rflags,
    void **const vpp)
{
  int status = ENOERR;

  const off_t blkoffset = _RNDDOWN(offset, (off_t)pxp->blksz);
  size_t diff = (size_t)(offset - blkoffset);
  size_t blkextent = _RNDUP(diff + extent, pxp->blksz);
  
  assert(extent != 0);
  assert(offset >= 0); /* sanity check */

  if(2 * pxp->blksz < blkextent)
    return E2BIG; /* TODO: temporary kludge */
  if(pxp->bf_offset == OFF_NONE)
  {
    /* Uninitialized */
    if(pxp->bf_base == NULL)
    {
      assert(pxp->bf_extent == 0);
      assert(blkextent <= 2 * pxp->blksz);
#ifndef ALIGN32
      pxp->bf_base = malloc(2 * pxp->blksz );
#else
      pxp->bf_base = malloc(2 * pxp->blksz + ALIGN32);
      pxp->bf_base = (char *)((uint)((char *)pxp->bf_base + ALIGN32 - 1) & ~(ALIGN32 - 1));
#endif
      if(pxp->bf_base == NULL)
        return ENOMEM;
    }
    goto pgin;
  }
  /* else */
  assert(blkextent <= 2 * pxp->blksz);

  if(blkoffset == pxp->bf_offset)
  {
    /* hit */
     if(blkextent > pxp->bf_extent) 
    {
      /* page in upper */
      void *const middle =
         (void *)((char *)pxp->bf_base + pxp->blksz);
      assert(pxp->bf_extent == pxp->blksz);
      status = px_pgin(nciop,
         pxp->bf_offset + (off_t)pxp->blksz,
         pxp->blksz,
         middle,
         &pxp->bf_cnt,
         &pxp->pos);
      if(status != ENOERR)
        return status;
      pxp->bf_extent = 2 * pxp->blksz;
      pxp->bf_cnt += pxp->blksz;
    }
    goto done;
  }
  /* else */

  if(pxp->bf_extent > pxp->blksz
     && blkoffset == pxp->bf_offset + pxp->blksz)
  {
    /* hit in upper half */
    if(blkextent == pxp->blksz)
    {
      /* all in upper half, no fault needed */
      diff += pxp->blksz;
      goto done;
    }
    /* else */
    if(pxp->bf_cnt > pxp->blksz)
    {
      /* data in upper half */
      void *const middle =
        (void *)((char *)pxp->bf_base + pxp->blksz);
      assert(pxp->bf_extent == 2 * pxp->blksz);
      if(fIsSet(pxp->bf_rflags, RGN_MODIFIED))
      {
        /* page out lower half */
        assert(pxp->bf_refcount <= 0);
        status = px_pgout(nciop,
          pxp->bf_offset,
          pxp->blksz,
          pxp->bf_base,
          &pxp->pos);
        if(status != ENOERR)
          return status;
      }
      pxp->bf_cnt -= pxp->blksz;
      /* copy upper half into lower half */
      (void) memcpy(pxp->bf_base, middle, pxp->bf_cnt);
    }
    pxp->bf_offset = blkoffset;
    /* pxp->bf_extent = pxp->blksz; */

     assert(blkextent == 2 * pxp->blksz);
    {
      /* page in upper */
      void *const middle =
         (void *)((char *)pxp->bf_base + pxp->blksz);
      status = px_pgin(nciop,
         pxp->bf_offset + (off_t)pxp->blksz,
         pxp->blksz,
         middle,
         &pxp->bf_cnt,
         &pxp->pos);
      if(status != ENOERR)
        return status;
      pxp->bf_extent = 2 * pxp->blksz;
      pxp->bf_cnt += pxp->blksz;
    }
    goto done;
  }
  /* else */

  if(blkoffset == pxp->bf_offset - pxp->blksz)
  {
    /* wants the page below */
    void *const middle =
      (void *)((char *)pxp->bf_base + pxp->blksz);
    size_t upper_cnt = 0;
    if(pxp->bf_cnt > pxp->blksz)
    {
      /* data in upper half */
      assert(pxp->bf_extent == 2 * pxp->blksz);
      if(fIsSet(pxp->bf_rflags, RGN_MODIFIED))
      {
        /* page out upper half */
        assert(pxp->bf_refcount <= 0);
        status = px_pgout(nciop,
          pxp->bf_offset + (off_t)pxp->blksz,
          pxp->bf_cnt - pxp->blksz,
          middle,
          &pxp->pos);
        if(status != ENOERR)
          return status;
      }
      pxp->bf_cnt = pxp->blksz;
      pxp->bf_extent = pxp->blksz;
    }
    if(pxp->bf_cnt > 0)
    {
      /* copy lower half into upper half */
      (void) memcpy(middle, pxp->bf_base, pxp->blksz);
      upper_cnt = pxp->bf_cnt;
    }
    /* read page below into lower half */
    status = px_pgin(nciop,
       blkoffset,
       pxp->blksz,
       pxp->bf_base,
       &pxp->bf_cnt,
       &pxp->pos);
    if(status != ENOERR)
      return status;
    pxp->bf_offset = blkoffset;
    if(upper_cnt != 0)
    {
      pxp->bf_extent = 2 * pxp->blksz;
      pxp->bf_cnt = pxp->blksz + upper_cnt;
    }
    else
    {
      pxp->bf_extent = pxp->blksz;
    }
    goto done;
  }
  /* else */

  /* no overlap */
  if(fIsSet(pxp->bf_rflags, RGN_MODIFIED))
  {
    assert(pxp->bf_refcount <= 0);
    status = px_pgout(nciop,
      pxp->bf_offset,
      pxp->bf_cnt,
      pxp->bf_base,
      &pxp->pos);
    if(status != ENOERR)
      return status;
    pxp->bf_rflags = 0;
  }

pgin:
  status = px_pgin(nciop,
     blkoffset,
     blkextent,
     pxp->bf_base,
     &pxp->bf_cnt,
     &pxp->pos);
  if(status != ENOERR)
    return status;
   pxp->bf_offset = blkoffset;
   pxp->bf_extent = blkextent;

done:
  extent += diff;
  if(pxp->bf_cnt < extent)
    pxp->bf_cnt = extent;
  assert(pxp->bf_cnt <= pxp->bf_extent);

  pxp->bf_rflags |= rflags;
  pxp->bf_refcount++;

  *vpp = (char *)pxp->bf_base + diff;
  return ENOERR;
}

static int
ncio_px_get(ncio *const nciop, 
    off_t offset, size_t extent,
    int rflags,
    void **const vpp)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;
  
  if(fIsSet(rflags, RGN_WRITE) && !fIsSet(nciop->ioflags, NC_WRITE))
    return EPERM; /* attempt to write readonly file */

  /* reclaim space used in move */
  if(pxp->slave != NULL)
  {
    if(pxp->slave->bf_base != NULL)
    {
      free(pxp->slave->bf_base);
      pxp->slave->bf_base = NULL;
      pxp->slave->bf_extent = 0;
      pxp->slave->bf_offset = OFF_NONE;
    }
    free(pxp->slave);
    pxp->slave = NULL;
  }
  return px_get(nciop, pxp, offset, extent, rflags, vpp);
}


/* ARGSUSED */
static int
px_double_buffer(ncio *const nciop, off_t to, off_t from,
      size_t nbytes, int rflags)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;
  int status = ENOERR;
  void *src;
  void *dest;
  
#if 0
fprintf(stderr, "double_buffr %ld %ld %ld\n",
     (long)to, (long)from, (long)nbytes);
#endif
  status = px_get(nciop, pxp, to, nbytes, RGN_WRITE,
      &dest);
  if(status != ENOERR)
    return status;

  if(pxp->slave == NULL)
  {
    pxp->slave = (ncio_px *) malloc(sizeof(ncio_px));
    if(pxp->slave == NULL)
      return ENOMEM;

    pxp->slave->blksz = pxp->blksz;
    /* pos done below */
    pxp->slave->bf_offset = pxp->bf_offset; 
    pxp->slave->bf_extent = pxp->bf_extent;
    pxp->slave->bf_cnt = pxp->bf_cnt;
#ifndef ALIGN32
    pxp->slave->bf_base = malloc(2 * pxp->blksz);
#else
    pxp->slave->bf_base = malloc(2 * pxp->blksz + ALIGN32);
    pxp->slave->bf_base = (char *)((uint)((char *)pxp->slave->bf_base + ALIGN32-1) & ~(ALIGN32 - 1));
#endif
    if(pxp->slave->bf_base == NULL)
      return ENOMEM;
    (void) memcpy(pxp->slave->bf_base, pxp->bf_base,
       pxp->bf_extent);
    pxp->slave->bf_rflags = 0;
    pxp->slave->bf_refcount = 0;
    pxp->slave->slave = NULL;
  }
  
  pxp->slave->pos = pxp->pos;
  status = px_get(nciop, pxp->slave, from, nbytes, 0,
      &src);
  if(status != ENOERR)
    return status;
  if(pxp->pos != pxp->slave->pos)
  {
    /* position changed, sync */
    pxp->pos = pxp->slave->pos;
  }

  (void) memcpy(dest, src, nbytes);

  (void)px_rel(pxp->slave, from, 0);
  (void)px_rel(pxp, to, RGN_MODIFIED);
  
  return status;
}

static int
ncio_px_move(ncio *const nciop, off_t to, off_t from,
      size_t nbytes, int rflags)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;
  int status = ENOERR;
  off_t lower;  
  off_t upper;
  char *base;
  size_t diff;
  size_t extent;

  if(to == from)
    return ENOERR; /* NOOP */
  
  if(fIsSet(rflags, RGN_WRITE) && !fIsSet(nciop->ioflags, NC_WRITE))
    return EPERM; /* attempt to write readonly file */

  rflags &= RGN_NOLOCK; /* filter unwanted flags */

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
  diff = (size_t)(upper - lower);
  extent = diff + nbytes;

  if(extent > pxp->blksz)
  {
    size_t remaining = nbytes;
    for(;;)
    {
      size_t loopextent = MIN(remaining, pxp->blksz);

      status = px_double_buffer(nciop, to, from,
           loopextent, rflags) ;
      if(status != ENOERR)
        return status;
      remaining -= loopextent;

      if(remaining == 0)
        break; /* normal loop exit */
      to += loopextent;
      from += loopextent;
    }
    return ENOERR;
  }
  
#if 0
fprintf(stderr, "ncio_px_move %ld %ld %ld %ld %ld\n",
     (long)to, (long)from, (long)nbytes, (long)lower, (long)extent);
#endif
  status = px_get(nciop, pxp, lower, extent, RGN_WRITE|rflags,
      (void **)&base);

  if(status != ENOERR)
    return status;

  if(to > from)
    (void) memmove(base + diff, base, nbytes); 
  else
    (void) memmove(base, base + diff, nbytes); 
    
  (void) px_rel(pxp, lower, RGN_MODIFIED);

  return status;
}


static int
ncio_px_sync(ncio *const nciop)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;
  int status = ENOERR;
  if(fIsSet(pxp->bf_rflags, RGN_MODIFIED))
  {
    assert(pxp->bf_refcount <= 0);
    status = px_pgout(nciop, pxp->bf_offset,
      pxp->bf_cnt,
      pxp->bf_base, &pxp->pos);
    if(status != ENOERR)
      return status;
    pxp->bf_rflags = 0;
  }
  return status;
}

static void
ncio_px_free(void *const pvt)
{
  ncio_px *const pxp = (ncio_px *)pvt;
  if(pxp == NULL)
    return;

  if(pxp->slave != NULL)
  {
    if(pxp->slave->bf_base != NULL)
    {
      free(pxp->slave->bf_base);
      pxp->slave->bf_base = NULL;
      pxp->slave->bf_extent = 0;
      pxp->slave->bf_offset = OFF_NONE;
    }
    free(pxp->slave);
    pxp->slave = NULL;
  }
    
  if(pxp->bf_base != NULL)
  {
    free(pxp->bf_base);
    pxp->bf_base = NULL;
    pxp->bf_extent = 0;
    pxp->bf_offset = OFF_NONE;
  }
}


static int
ncio_px_init2(ncio *const nciop, size_t *sizehintp, int isNew)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;
  const size_t bufsz = 2 * *sizehintp;

  assert(nciop->fd >= 0);

  pxp->blksz = *sizehintp;

  assert(pxp->bf_base == NULL);

  /* this is separate allocation because it may grow */
#ifndef ALIGN32
  pxp->bf_base = malloc(bufsz);
#else
  pxp->bf_base = malloc(bufsz + ALIGN32);
  pxp->bf_base = (char *)((uint)((char *)pxp->bf_base + ALIGN32 - 1) & ~(ALIGN32 - 1));
#endif
  if(pxp->bf_base == NULL)
    return ENOMEM;
  /* else */
  pxp->bf_cnt = 0;
  if(isNew)
  {
    /* save a read */
    pxp->pos = 0;
    pxp->bf_offset = 0;
    pxp->bf_extent = bufsz;
    (void) memset(pxp->bf_base, 0, pxp->bf_extent);
  }
  return ENOERR;
}


static void
ncio_px_init(ncio *const nciop)
{
  ncio_px *const pxp = (ncio_px *)nciop->pvt;

  *((ncio_relfunc **)&nciop->rel) = ncio_px_rel; /* cast away const */
  *((ncio_getfunc **)&nciop->get) = ncio_px_get; /* cast away const */
  *((ncio_movefunc **)&nciop->move) = ncio_px_move; /* cast away const */
  *((ncio_syncfunc **)&nciop->sync) = ncio_px_sync; /* cast away const */
  *((ncio_freefunc **)&nciop->free) = ncio_px_free; /* cast away const */

  pxp->blksz = 0;
  pxp->pos = -1;
  pxp->bf_offset = OFF_NONE;
  pxp->bf_extent = 0;
  pxp->bf_rflags = 0;
  pxp->bf_refcount = 0;
  pxp->bf_base = NULL;
  pxp->slave = NULL;

}

/* Begin spx */

typedef struct ncio_spx {
  off_t pos;
  /* buffer */
  off_t  bf_offset; 
  size_t  bf_extent;
  size_t  bf_cnt;
  void  *bf_base;
} ncio_spx;


/*ARGSUSED*/
static int
ncio_spx_rel(ncio *const nciop, off_t offset, int rflags)
{
  ncio_spx *const pxp = (ncio_spx *)nciop->pvt;
  int status = ENOERR;

  assert(pxp->bf_offset <= offset);
  assert(pxp->bf_cnt != 0);
  assert(pxp->bf_cnt <= pxp->bf_extent);
#ifdef X_ALIGN
  assert(offset < pxp->bf_offset + X_ALIGN);
  assert(pxp->bf_cnt % X_ALIGN == 0 );
#endif

  if(fIsSet(rflags, RGN_MODIFIED))
  {
    if(!fIsSet(nciop->ioflags, NC_WRITE))
      return EPERM; /* attempt to write readonly file */

    status = px_pgout(nciop, pxp->bf_offset,
      pxp->bf_cnt,
      pxp->bf_base, &pxp->pos);
    /* if error, invalidate buffer anyway */
  }
  pxp->bf_offset = OFF_NONE;
  pxp->bf_cnt = 0;
  return status;
}


static int
ncio_spx_get(ncio *const nciop,
    off_t offset, size_t extent,
    int rflags,
    void **const vpp)
{
  ncio_spx *const pxp = (ncio_spx *)nciop->pvt;
  int status = ENOERR;
#ifdef X_ALIGN
  size_t rem;
#endif
  
  if(fIsSet(rflags, RGN_WRITE) && !fIsSet(nciop->ioflags, NC_WRITE))
    return EPERM; /* attempt to write readonly file */

  assert(extent != 0);

  assert(pxp->bf_cnt == 0);

#ifdef X_ALIGN
  rem = (size_t)(offset % X_ALIGN);
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

  if(pxp->bf_extent < extent)
  {
    if(pxp->bf_base != NULL)
    {
      free(pxp->bf_base);
      pxp->bf_base = NULL;
      pxp->bf_extent = 0;
    }
    assert(pxp->bf_extent == 0);
    pxp->bf_base = malloc(extent);
    if(pxp->bf_base == NULL)
      return ENOMEM;
    pxp->bf_extent = extent;
  }

  status = px_pgin(nciop, offset,
     extent,
     pxp->bf_base,
     &pxp->bf_cnt, &pxp->pos);
  if(status != ENOERR)
    return status;

  pxp->bf_offset = offset;

  if(pxp->bf_cnt < extent)
    pxp->bf_cnt = extent;

#ifdef X_ALIGN
  *vpp = (char *)pxp->bf_base + rem;
#else
  *vpp = pxp->bf_base;
#endif
  return ENOERR;
}


#if 0
/*ARGSUSED*/
static int
strategy(ncio *const nciop, off_t to, off_t offset,
      size_t extent, int rflags)
{
  static ncio_spx pxp[1];
  int status = ENOERR;
#ifdef X_ALIGN
  size_t rem;
#endif
  
  assert(extent != 0);
#if INSTRUMENT
fprintf(stderr, "strategy %ld at %ld to %ld\n",
   (long)extent, (long)offset, (long)to);
#endif


#ifdef X_ALIGN
  rem = (size_t)(offset % X_ALIGN);
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

  if(pxp->bf_extent < extent)
  {
    if(pxp->bf_base != NULL)
    {
      free(pxp->bf_base);
      pxp->bf_base = NULL;
      pxp->bf_extent = 0;
    }
    assert(pxp->bf_extent == 0);
    pxp->bf_base = malloc(extent);
    if(pxp->bf_base == NULL)
      return ENOMEM;
    pxp->bf_extent = extent;
  }

  status = px_pgin(nciop, offset,
     extent,
     pxp->bf_base,
     &pxp->bf_cnt, &pxp->pos);
  if(status != ENOERR)
    return status;

  pxp->bf_offset = to; /* TODO: XALIGN */
  
  if(pxp->bf_cnt < extent)
    pxp->bf_cnt = extent;

  status = px_pgout(nciop, pxp->bf_offset,
    pxp->bf_cnt,
    pxp->bf_base, &pxp->pos);
  /* if error, invalidate buffer anyway */
  pxp->bf_offset = OFF_NONE;
  pxp->bf_cnt = 0;
  return status;
}
#endif

static int
ncio_spx_move(ncio *const nciop, off_t to, off_t from,
      size_t nbytes, int rflags)
{
  int status = ENOERR;
  off_t lower = from;  
  off_t upper = to;
  char *base;
  size_t diff = (size_t)(upper - lower);
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

  diff = (size_t)(upper - lower);
  extent = diff + nbytes;

  status = ncio_spx_get(nciop, lower, extent, RGN_WRITE|rflags,
      (void **)&base);

  if(status != ENOERR)
    return status;

  if(to > from)
    (void) memmove(base + diff, base, nbytes); 
  else
    (void) memmove(base, base + diff, nbytes); 
    
  (void) ncio_spx_rel(nciop, lower, RGN_MODIFIED);

  return status;
}


/*ARGSUSED*/
static int
ncio_spx_sync(ncio *const nciop)
{
  /* NOOP */
  return ENOERR;
}

static void
ncio_spx_free(void *const pvt)
{
  ncio_spx *const pxp = (ncio_spx *)pvt;
  if(pxp == NULL)
    return;

  if(pxp->bf_base != NULL)
  {
    free(pxp->bf_base);
    pxp->bf_base = NULL;
    pxp->bf_offset = OFF_NONE;
    pxp->bf_extent = 0;
    pxp->bf_cnt = 0;
  }
}


static int
ncio_spx_init2(ncio *const nciop, const size_t *const sizehintp)
{
  ncio_spx *const pxp = (ncio_spx *)nciop->pvt;

  assert(nciop->fd >= 0);

  pxp->bf_extent = *sizehintp;

  assert(pxp->bf_base == NULL);

  /* this is separate allocation because it may grow */
  pxp->bf_base = malloc(pxp->bf_extent);
  if(pxp->bf_base == NULL)
  {
    pxp->bf_extent = 0;
    return ENOMEM;
  }
  /* else */
  return ENOERR;
}


static void
ncio_spx_init(ncio *const nciop)
{
  ncio_spx *const pxp = (ncio_spx *)nciop->pvt;

  *((ncio_relfunc **)&nciop->rel) = ncio_spx_rel; /* cast away const */
  *((ncio_getfunc **)&nciop->get) = ncio_spx_get; /* cast away const */
  *((ncio_movefunc **)&nciop->move) = ncio_spx_move; /* cast away const */
  *((ncio_syncfunc **)&nciop->sync) = ncio_spx_sync; /* cast away const */
  *((ncio_freefunc **)&nciop->free) = ncio_spx_free; /* cast away const */

  pxp->pos = -1;
  pxp->bf_offset = OFF_NONE;
  pxp->bf_extent = 0;
  pxp->bf_cnt = 0;
  pxp->bf_base = NULL;
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
    sz_ncio_pvt = sizeof(ncio_spx);
  else
    sz_ncio_pvt = sizeof(ncio_px);

  nciop = (ncio *) malloc(sz_ncio + sz_path + sz_ncio_pvt);
  if(nciop == NULL)
    return NULL;
  
  nciop->ioflags = ioflags;
  *((int *)&nciop->fd) = -1; /* cast away const */

  nciop->path = (char *) ((char *)nciop + sz_ncio);
  (void) strcpy((char *)nciop->path, path); /* cast away const */

        /* cast away const */
  *((void **)&nciop->pvt) = (void *)(nciop->path + sz_path);

  if(fIsSet(ioflags, NC_SHARE))
    ncio_spx_init(nciop);
  else
    ncio_px_init(nciop);

  return nciop;
}


/* Public below this point */

#define NCIO_MINBLOCKSIZE 256
#define NCIO_MAXBLOCKSIZE 268435456 /* sanity check, about X_SIZE_T_MAX/8 */

#if defined( S_IRUSR ) && defined( S_IRGRP ) && defined( S_IWGRP ) && defined( S_IROTH ) && defined( S_IWOTH )
#define NC_DEFAULT_CREAT_MODE \
        (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) /* 0666 */

#else
#define NC_DEFAULT_CREAT_MODE 0666
#endif

int
ncio_create(const char *path, int ioflags,
  size_t initialsz,
  off_t igeto, size_t igetsz, size_t *sizehintp,
  ncio **nciopp, void **const igetvpp)
{
  ncio *nciop;
  int oflags = (O_RDWR|O_CREAT);
  int fd;
  int status;

  if(initialsz < (size_t)igeto + igetsz)
    initialsz = (size_t)igeto + igetsz;

  fSet(ioflags, NC_WRITE);

  if(path == NULL || *path == 0)
    return EINVAL;

  nciop = ncio_new(path, ioflags);
  if(nciop == NULL)
    return ENOMEM;

  if(fIsSet(ioflags, NC_NOCLOBBER))
    fSet(oflags, O_EXCL);
  else
    fSet(oflags, O_TRUNC);
#ifdef O_BINARY
  fSet(oflags, O_BINARY);
#endif
#ifdef vms
  fd = open(path, oflags, NC_DEFAULT_CREAT_MODE, "ctx=stm");
#else
  /* Should we mess with the mode based on NC_SHARE ?? */
  fd = open(path, oflags, NC_DEFAULT_CREAT_MODE);
#endif
#if 0
  (void) fprintf(stderr, "ncio_create(): path=\"%s\"\n", path);
  (void) fprintf(stderr, "ncio_create(): oflags=0x%x\n", oflags);
#endif
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
#ifdef __PARAGON__
        *((int *)&nciop->is_file_pfs) = netcdf_local_is_file_pfs;
        netcdf_local_is_file_pfs = 0;
#endif

  if(fIsSet(nciop->ioflags, NC_SHARE))
    status = ncio_spx_init2(nciop, sizehintp);
  else
    status = ncio_px_init2(nciop, sizehintp, 1);

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
  (void) close(fd);
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
  int oflags = fIsSet(ioflags, NC_WRITE) ? O_RDWR : O_RDONLY;
  int fd;
  int status;

  if(path == NULL || *path == 0)
    return EINVAL;

  nciop = ncio_new(path, ioflags);
  if(nciop == NULL)
    return ENOMEM;

#ifdef O_BINARY
  fSet(oflags, O_BINARY);
#endif
#ifdef vms
  fd = open(path, oflags, 0, "ctx=stm");
#else
  fd = open(path, oflags, 0);
#endif
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

  if(fIsSet(nciop->ioflags, NC_SHARE))
    status = ncio_spx_init2(nciop, sizehintp);
  else
    status = ncio_px_init2(nciop, sizehintp, 0);

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
  (void) close(fd);
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

  (void) close(nciop->fd);
  
  if(doUnlink)
    (void) unlink(nciop->path);

  ncio_free(nciop);

  return status;
}
