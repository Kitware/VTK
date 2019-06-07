/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef _NCSTDIO_H_
#define _NCSTDIO_H_

typedef struct ncstdio ncstdio;	/* forward reference */
/*
 * netcdf i/o abstraction
 */
struct ncstdio {
    int ioflags; /* make visible for fIsSet macro access*/
    /* Internal state of the stdio dispatcher */
    void* state;
    /* dispatch functions; never called directly by any higher-level code */
    struct ncstdio_ops {
        int (*read)(ncstdio*,void*,const size_t,size_t*);
        int (*write)(ncstdio*,const void*,const size_t,size_t*);
        int (*free)(ncstdio*);
        int (*close)(ncstdio*,int);
        int (*flush)(ncstdio*)
	int (*seek)(ncstdio*,off_t);
	int (*sync)(ncstdio*);
	int (*uid)(ncstdio*,int*);
    } ops;
};

extern int 
ncstdio_close(ncstdio* ncstdiop, int deletefile);

extern int
ncstdio_flush(ncstdio* ncstdiop);

extern int
ncstdio_seek(ncstdio* ncstdiop, off_t pos);

extern int
ncstdio_sync(ncstdio* ncstdiop);

extern int
ncstdio_read(ncstdio* ncstdiop, void* memory, const size_t size, size_t* actual);

extern int
ncstdio_write(ncstdio* ncstdiop, const void* memory, const size_t size, size_t* actual);

extern int
ncstdio_uid(ncstdio* ncstdiop,int*);

/* export all known ncstdio implementation create/open procedures */
extern int ncFile_create(const char *path, int ioflags, ncstdio** filepp);
extern int ncFile_open(const char *path, int ioflags, ncstdio** filepp);

extern int ncMemory_create(const char *path, int ioflags, ncstdio** filepp);
extern int ncMemory_open(const char *path, int ioflags, ncstdio** filepp);

#endif /* _NCSTDIO_H_* /
