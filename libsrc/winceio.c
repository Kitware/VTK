/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: winceio.c,v 1.2 2010/05/04 17:30:04 dmh Exp $ */
/* Dennis Heimbigner 2010-3-04 */


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>	/* DEBUG */

#ifndef _WIN32_WCE
#include <errno.h>
#else
#define EPERM  NC_EPERM
#define ENOMEM NC_ENOMEM
#define EINVAL NC_EINVAL
#define EIO    NC_EINVAL
#define EEXIST NC_EEXIST
#endif

#ifndef NC_NOERR
#define NC_NOERR 0
#endif

#include <string.h>
#include <stdio.h>

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

#define DEFAULTBLKSIZE 32768

static FILE* descriptors[1024];
static int fdmax = 1; /* never use zero */

/* Begin OS */

/*
 * What is the preferred I/O block size?
 * (This becomes the default *sizehint == ncp->chunk in the higher layers.)
 * TODO: What is the the best answer here?
 */
static size_t
blksize(int fd)
{
	return (size_t) DEFAULTBLKSIZE;
}

/*
 * Sortof like ftruncate, except won't make the
 * file shorter.
 */
static int
fgrow(FILE* f, const off_t len)
{
    int status = NC_NOERR;
    long pos = ftell(f);
    long size;
    pos = ftell(f);
    status = fseek(f,0,SEEK_END);
    if(ferror(f)) return EIO;
    size = ftell(f);
    status = fseek(f,pos,SEEK_SET);
    if(ferror(f)) return EIO;
    if(len < size) return NC_NOERR;
    else {
	const long dumb = 0;
	status = fseek(f, len-sizeof(dumb), SEEK_SET);
        if(ferror(f)) return EIO;
	fwrite((const void *)&dumb, 1, sizeof(dumb), f);
	if(ferror(f)) return EIO;
	status = fseek(f, pos, SEEK_SET);
        if(ferror(f)) return EIO;
    }
    return NC_NOERR;
}


/*
 * Sortof like ftruncate, except won't make the file shorter.  Differs
 * from fgrow by only writing one byte at designated seek position, if
 * needed.
 */
static int
fgrow2(FILE* f, const off_t len)
{
    int status = NC_NOERR;
    long pos = ftell(f);
    long size;
    pos = ftell(f);
    status = fseek(f,0,SEEK_END);
    if(ferror(f)) return EIO;
    size = ftell(f);
    status = fseek(f,pos,SEEK_SET);
    if(ferror(f)) return EIO;
    if(len < size) return NC_NOERR;
    else {
	const char dumb = 0;
	status = fseek(f, len-sizeof(dumb), SEEK_SET);
        if(ferror(f)) return EIO;
	fwrite((const void *)&dumb, 1, sizeof(dumb), f);
	if(ferror(f)) return EIO;
	status = fseek(f, pos, SEEK_SET);
	if(ferror(f)) return EIO;
    }
    return NC_NOERR;
}
/* End OS */
/* Begin ffio */

static int
fileio_pgout(ncio *const nciop,
	off_t const offset,  const size_t extent,
	const void *const vp, off_t *posp)
{
       int status = NC_NOERR;
       FILE* f = descriptors[nciop->fd];

#ifdef X_ALIGN
	assert(offset % X_ALIGN == 0);
	assert(extent % X_ALIGN == 0);
#endif

	if(*posp != offset)
	{
		status = fseek(f, offset, SEEK_SET);
                if(ferror(f)) return EIO;
		*posp = offset;
	}
	fwrite(vp,1,extent,f);
        if(ferror(f)) return EIO;
	*posp += extent;
	return NC_NOERR;
}

static int
fileio_pgin(ncio *const nciop,
	off_t const offset, const size_t extent,
	void *const vp, size_t *nreadp, off_t *posp)
{
	int status = NC_NOERR;
	ssize_t nread;
	int count;

        FILE* f = descriptors[nciop->fd];

#ifdef X_ALIGN
	assert(offset % X_ALIGN == 0);
	assert(extent % X_ALIGN == 0);
#endif

	if(*posp != offset)
	{
		status = fseek(f, offset, SEEK_SET);
                if(ferror(f)) return EIO;
		*posp = offset;
	}

	nread = fread(vp,1,extent,f);
        if(ferror(f)) return EIO;
	*nreadp = nread;
	*posp += nread;
	return NC_NOERR;
}

/* */

typedef struct ncio_ffio {
	off_t pos;
	/* buffer */
	off_t	bf_offset;
	size_t	bf_extent;
	size_t	bf_cnt;
	void	*bf_base;
} ncio_ffio;

static int
ncio_fileio_rel(ncio *const nciop, off_t offset, int rflags)
{
	int status = NC_NOERR;
        FILE* f = descriptors[nciop->fd];

	ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;

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

		status = fileio_pgout(nciop, ffp->bf_offset,
			ffp->bf_cnt,
			ffp->bf_base, &ffp->pos);
		/* if error, invalidate buffer anyway */
	}
	ffp->bf_offset = OFF_NONE;
	ffp->bf_cnt = 0;
	return status;
}


static int
ncio_fileio_get(ncio *const nciop,
		off_t offset, size_t extent,
		int rflags,
		void **const vpp)
{
	ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;
	int status = NC_NOERR;
        FILE* f = descriptors[nciop->fd];
#ifdef X_ALIGN
	size_t rem;
#endif

	if(fIsSet(rflags, RGN_WRITE) && !fIsSet(nciop->ioflags, NC_WRITE))
		return EPERM; /* attempt to write readonly file */

	assert(extent != 0);
	assert(extent < X_INT_MAX); /* sanity check */

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

	status = fileio_pgin(nciop, offset,
		 extent,
		 ffp->bf_base,
		 &ffp->bf_cnt, &ffp->pos);
	if(status != NC_NOERR)
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
	return NC_NOERR;
}


static int
ncio_fileio_move(ncio *const nciop, off_t to, off_t from,
			size_t nbytes, int rflags)
{
	int status = NC_NOERR;
	off_t lower = from;
	off_t upper = to;
	char *base;
	size_t diff = upper - lower;
	size_t extent = diff + nbytes;
        FILE* f = descriptors[nciop->fd];

	rflags &= RGN_NOLOCK; /* filter unwanted flags */

	if(to == from)
		return NC_NOERR; /* NOOP */

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

	status = ncio_fileio_get(nciop, lower, extent, RGN_WRITE|rflags,
			(void **)&base);

	if(status != NC_NOERR)
		return status;

	if(to > from)
		(void) memmove(base + diff, base, nbytes);
	else
		(void) memmove(base, base + diff, nbytes);

	(void) ncio_fileio_rel(nciop, lower, RGN_MODIFIED);

	return status;
}

static int
ncio_fileio_sync(ncio *const nciop)
{
        FILE* f = descriptors[nciop->fd];
	fflush(f);
	return NC_NOERR;
}

static void
ncio_fileio_free(void *const pvt)
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
ncio_fileio_init2(ncio *const nciop, size_t *sizehintp)
{
        FILE* f = descriptors[nciop->fd];
	ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;

	assert(f != NULL);

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
	return NC_NOERR;
}


static void
ncio_fileio_init(ncio *const nciop)
{
	ncio_ffio *ffp = (ncio_ffio *)nciop->pvt;
p
	*((ncio_relfunc **)&nciop->rel) = ncio_fileio_rel; /* cast away const */
	*((ncio_getfunc **)&nciop->get) = ncio_fileio_get; /* cast away const */
	*((ncio_movefunc **)&nciop->move) = ncio_fileio_move; /* cast away const */
	*((ncio_syncfunc **)&nciop->sync) = ncio_fileio_sync; /* cast away const */
	*((ncio_freefunc **)&nciop->free) = ncio_fileio_free; /* cast away const */

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
		fprintf(stderr, "NC_SHARE not implemented for fileio\n");

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

	ncio_fileio_init(nciop);

	return nciop;
}

/* Public below this point */

/* TODO: Is this reasonable for this platform? */
static const size_t NCIO_MINBLOCKSIZE = 0x100;
static const size_t NCIO_MAXBLOCKSIZE = 0x100000;

int
ncio_create(const char *path, int ioflags,
	size_t initialsz,
	off_t igeto, size_t igetsz, size_t *sizehintp,
	void* parameters,
	ncio **nciopp, void **const igetvpp)
{
	ncio *nciop;
#ifdef _WIN32_WCE
	char* oflags = "bw+"; /*==?(O_RDWR|O_CREAT|O_TRUNC) && binary*/
#else
	char* oflags = "w+"; /*==?(O_RDWR|O_CREAT|O_TRUNC);*/
#endif
	FILE* f;
	int i,fd;
	int status = NC_NOERR;

	if(initialsz < (size_t)igeto + igetsz)
		initialsz = (size_t)igeto + igetsz;

	fSet(ioflags, NC_WRITE);

	if(path == NULL || *path == 0)
		return EINVAL;

	nciop = ncio_new(path, ioflags);
	if(nciop == NULL)
		return ENOMEM;

	if(fIsSet(ioflags, NC_NOCLOBBER)) {
	    /* Since we do not have use of the O_EXCL flag,
               we need to fake it */
#ifdef WINCE
	    f = fopen(path,"rb");
#else
	    f = fopen(path,"r");
#endif
	    if(f != NULL) { /* do not overwrite */
		(void)fclose(f);
		return EEXIST;
	    }
	}

	f = fopen(path, oflags);
	if(f == NULL)
	{
		status = errno;
		goto unwind_new;
	}

	/* Locate an open pseudo file descriptor */
	fd = -1;
	for(i=1;i<fdmax;i++) {if(descriptors[i] == NULL) {fd=i;break;}}
	if(fd < 0) {fd = fdmax; fdmax++;}
	descriptors[fd] = f;

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

	status = ncio_fileio_init2(nciop, sizehintp);
	if(status != NC_NOERR)
		goto unwind_open;

	if(initialsz != 0)
	{
		status = fgrow(f, (off_t)initialsz);
		if(status != NC_NOERR)
			goto unwind_open;
	}

	if(igetsz != 0)
	{
		status = nciop->get(nciop,
				igeto, igetsz,
                        	RGN_WRITE,
                        	igetvpp);
		if(status != NC_NOERR)
			goto unwind_open;
	}

	*nciopp = nciop;
	return NC_NOERR;

unwind_open:
	(void) fclose(descriptors[fd]);
	descriptors[fd] = NULL;
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
	void* parameters,
	ncio **nciopp, void **const igetvpp)
{
	ncio *nciop;
	char* oflags = fIsSet(ioflags, NC_WRITE) ? "r+"
#ifdef WINCE
						 : "rb";
#else
						 : "r";
#endif
	FILE* f;
	int i,fd;
	int status = NC_NOERR;

	if(path == NULL || *path == 0)
		return EINVAL;

	nciop = ncio_new(path, ioflags);
	if(nciop == NULL)
		return ENOMEM;

	f = fopen(path, oflags);

	if(f == NULL)
	{
		status = errno;
		goto unwind_new;
	}

	/* Locate an open pseudo file descriptor */
	fd = -1;
	for(i=1;i<fdmax;i++) {if(descriptors[i] == NULL) {fd=i;break;}}
	if(fd < 0) {fd = fdmax; fdmax++;}
	descriptors[fd] = f;

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

	status = ncio_fileio_init2(nciop, sizehintp);
	if(status != NC_NOERR)
		goto unwind_open;

	if(igetsz != 0)
	{
		status = nciop->get(nciop,
				igeto, igetsz,
                        	0,
                        	igetvpp);
		if(status != NC_NOERR)
			goto unwind_open;
	}

	*nciopp = nciop;
	return NC_NOERR;

unwind_open:
	(void) fclose(descriptors[fd]);
	descriptors[fd] = NULL;
	/*FALLTHRU*/
unwind_new:
	ncio_free(nciop);
	return status;
}


/*
 * Get file size in bytes.
 * Is use of fstatus = fseek() really necessary, or could we use standard fstat() call
 * and get st_size member?
 */
int
ncio_filesize(ncio *nciop, off_t *filesizep)
{
    int status = NC_NOERR;
    off_t filesize, current, reset;
    FILE* f;

    if(nciop == NULL)
	return EINVAL;

    f = descriptors[nciop->fd];

    current = ftell(f);
    status = fseek(f, 0, SEEK_END); /* get size */
    if(ferror(f)) return EIO;
    *filesizep = ftell(f);
    status = fseek(f, current, SEEK_SET); /* reset */
    if(ferror(f)) return EIO;
    return NC_NOERR;
}


/*
 * Sync any changes to disk, then extend file so its size is length.
 * This is only intended to be called before close, if the file is
 * open for writing and the actual size does not match the calculated
 * size, perhaps as the result of having been previously written in
 * NOFILL mode.
 */
int
ncio_pad_length(ncio *nciop, off_t length)
{
	int status = NC_NOERR;
	FILE* f;

	if(nciop == NULL)
		return EINVAL;

	f = descriptors[nciop->fd];

	if(!fIsSet(nciop->ioflags, NC_WRITE))
	        return EPERM; /* attempt to write readonly file */

	status = nciop->sync(nciop);
	if(status != NC_NOERR)
	        return status;

	status = fgrow2(f, length);
	if(status != NC_NOERR)
	        return errno;
	return NC_NOERR;
}


int
ncio_close(ncio *nciop, int doUnlink)
{
	int status = NC_NOERR;
	FILE* f;

	if(nciop == NULL)
		return EINVAL;

	f = descriptors[nciop->fd];

	nciop->sync(nciop);

	(void) fclose(f);

	descriptors[nciop->fd] = NULL;

	if(doUnlink)
		(void) unlink(nciop->path);

	ncio_free(nciop);

	return status;
}
