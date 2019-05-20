/** \file dfile.c

File create and open functions

These functions end up calling functions in one of the dispatch layers
(netCDF-4, dap server, etc).

Copyright 2010 University Corporation for Atmospheric
Research/Unidata. See COPYRIGHT file for more info.
*/

#include "config.h"
#include <stdlib.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include "ncdispatch.h"
#include "netcdf_mem.h"
#include "ncwinpath.h"

extern int NC_initialized;
extern int NC_finalized;

/** \defgroup datasets NetCDF File and Data I/O

NetCDF opens datasets as files or remote access URLs.

A netCDF dataset that has not yet been opened can only be referred to
by its dataset name. Once a netCDF dataset is opened, it is referred
to by a netCDF ID, which is a small non-negative integer returned when
you create or open the dataset. A netCDF ID is much like a file
descriptor in C or a logical unit number in FORTRAN. In any single
program, the netCDF IDs of distinct open netCDF datasets are
distinct. A single netCDF dataset may be opened multiple times and
will then have multiple distinct netCDF IDs; however at most one of
the open instances of a single netCDF dataset should permit
writing. When an open netCDF dataset is closed, the ID is no longer
associated with a netCDF dataset.

Functions that deal with the netCDF library include:
- Get version of library.
- Get error message corresponding to a returned error code.

The operations supported on a netCDF dataset as a single object are:
- Create, given dataset name and whether to overwrite or not.
- Open for access, given dataset name and read or write intent.
- Put into define mode, to add dimensions, variables, or attributes.
- Take out of define mode, checking consistency of additions.
- Close, writing to disk if required.
- Inquire about the number of dimensions, number of variables,
number of global attributes, and ID of the unlimited dimension, if
any.
- Synchronize to disk to make sure it is current.
- Set and unset nofill mode for optimized sequential writes.
- After a summary of conventions used in describing the netCDF
interfaces, the rest of this chapter presents a detailed description
of the interfaces for these operations.
*/


/*!
  Interpret the magic number found in the header of a netCDF file.

  This function interprets the magic number/string contained in the header of a netCDF file and sets the appropriate NC_FORMATX flags.

  @param[in] magic Pointer to a character array with the magic number block.
  @param[out] model Pointer to an integer to hold the corresponding netCDF type.
  @param[out] version Pointer to an integer to hold the corresponding netCDF version.
  @param[in] use_parallel 1 if using parallel, 0 if not.
  @return Returns an error code or 0 on success.

\internal
\ingroup datasets

*/
static int
NC_interpret_magic_number(char* magic, int* model, int* version, int use_parallel)
{
    int status = NC_NOERR;
    /* Look at the magic number */
    /* Ignore the first byte for HDF */
#ifdef USE_NETCDF4
    if(magic[1] == 'H' && magic[2] == 'D' && magic[3] == 'F') {
	*model = NC_FORMATX_NC4;
	*version = 5;
#ifdef USE_HDF4
    } else if(magic[0] == '\016' && magic[1] == '\003'
              && magic[2] == '\023' && magic[3] == '\001') {
	*model = NC_FORMATX_NC4;
	*version = 4;
#endif
    } else
#endif
    if(magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F') {
        if(magic[3] == '\001') {
            *version = 1; /* netcdf classic version 1 */
	    *model = NC_FORMATX_NC3;
         } else if(magic[3] == '\002') {
            *version = 2; /* netcdf classic version 2 */
	    *model = NC_FORMATX_NC3;
#ifdef USE_CDF5
        } else if(magic[3] == '\005') {
          *version = 5; /* cdf5 (including pnetcdf) file */
	    *model = NC_FORMATX_NC3;
#endif
     } else
	    {status = NC_ENOTNC; goto done;}
     } else
        {status = NC_ENOTNC; goto done;}
done:
     return status;
}

/*!
Given an existing file, figure out its format
and return that format value (NC_FORMATX_XXX)
in model arg. Assume any path conversion was
already performed at a higher level.
*/
static int
NC_check_file_type(const char *path, int flags, void *parameters,
		   int* model, int* version)
{
   char magic[MAGIC_NUMBER_LEN];
   int status = NC_NOERR;
   int diskless = ((flags & NC_DISKLESS) == NC_DISKLESS);
   int use_parallel = ((flags & NC_MPIIO) == NC_MPIIO);
   int inmemory = (diskless && ((flags & NC_INMEMORY) == NC_INMEMORY));

   *model = 0;

    if(inmemory)  {
	NC_MEM_INFO* meminfo = (NC_MEM_INFO*)parameters;
	if(meminfo == NULL || meminfo->size < MAGIC_NUMBER_LEN)
	    {status = NC_EDISKLESS; goto done;}
	memcpy(magic,meminfo->memory,MAGIC_NUMBER_LEN);
    } else {/* presumably a real file */
       /* Get the 4-byte magic from the beginning of the file. Don't use posix
        * for parallel, use the MPI functions instead. */

#ifdef USE_PARALLEL
	if (use_parallel) {
	    MPI_File fh;
	    MPI_Status mstatus;
	    int retval;
	    MPI_Comm comm = MPI_COMM_WORLD;
	    MPI_Info info = MPI_INFO_NULL;

	    if(parameters != NULL) {
	        comm = ((NC_MPI_INFO*)parameters)->comm;
		info = ((NC_MPI_INFO*)parameters)->info;
	    }
	    if((retval = MPI_File_open(comm,(char*)path,MPI_MODE_RDONLY,info,
				       &fh)) != MPI_SUCCESS)
		{status = NC_EPARINIT; goto done;}
	    if((retval = MPI_File_read(fh, magic, MAGIC_NUMBER_LEN, MPI_CHAR,
				 &mstatus)) != MPI_SUCCESS)
		{status = NC_EPARINIT; goto done;}
	    if((retval = MPI_File_close(&fh)) != MPI_SUCCESS)
		{status = NC_EPARINIT; goto done;}
	} else
#endif /* USE_PARALLEL */
	{
	    FILE *fp;
	    size_t i;
#ifdef HAVE_FILE_LENGTH_I64
          __int64 file_len = 0;
#else
          struct stat st;
#endif
          if(path == NULL || strlen(path)==0)
		{status = NC_EINVAL; goto done;}

	  if (!(fp = fopen(path, "r")))
		{status = errno; goto done;}

#ifdef HAVE_SYS_STAT_H
	  /* The file must be at least MAGIC_NUMBER_LEN in size,
	       or otherwise the following fread will exhibit unexpected
  	       behavior. */

          /* Windows and fstat have some issues, this will work around that. */
#ifdef HAVE_FILE_LENGTH_I64
          if((file_len = _filelengthi64(fileno(fp))) < 0) {
            fclose(fp);
            status = errno;
            goto done;
          }

          if(file_len < MAGIC_NUMBER_LEN) {
            fclose(fp);
            status = NC_ENOTNC;
            goto done;
          }
#else
	  { int fno = fileno(fp);
	    if(!(fstat(fno,&st) == 0)) {
	        fclose(fp);
	        status = errno;
	        goto done;
	    }
	    if(st.st_size < MAGIC_NUMBER_LEN) {
              fclose(fp);
              status = NC_ENOTNC;
              goto done;
	    }
	  }
#endif //HAVE_FILE_LENGTH_I64

#endif //HAVE_SYS_STAT_H

	    i = fread(magic, MAGIC_NUMBER_LEN, 1, fp);
	    fclose(fp);
	    if(i == 0)
		{status = NC_ENOTNC; goto done;}
	    if(i != 1)
		{status = errno; goto done;}
	}
    } /* !inmemory */

    /* Look at the magic number */
    status = NC_interpret_magic_number(magic,model,version,use_parallel);

done:
   return status;
}

/**  \ingroup datasets
Create a new netCDF file.

This function creates a new netCDF dataset, returning a netCDF ID that
can subsequently be used to refer to the netCDF dataset in other
netCDF function calls. The new netCDF dataset opened for write access
and placed in define mode, ready for you to add dimensions, variables,
and attributes.

\param path The file name of the new netCDF dataset.

\param cmode The creation mode flag. The following flags are available:
  NC_NOCLOBBER (do not overwrite existing file),
  NC_SHARE (limit write caching - netcdf classic files only),
  NC_64BIT_OFFSET (create 64-bit offset file),
  NC_64BIT_DATA (Alias NC_CDF5) (create CDF-5 file),
  NC_NETCDF4 (create netCDF-4/HDF5 file),
  NC_CLASSIC_MODEL (enforce netCDF classic mode on netCDF-4/HDF5 files),
  NC_DISKLESS (store data only in memory),
  NC_MMAP (use MMAP for NC_DISKLESS),
  and NC_WRITE.
  See discussion below.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

<h2>The cmode Flag</h2>

The cmode flag is used to control the type of file created, and some
aspects of how it may be used.

Setting NC_NOCLOBBER means you do not want to clobber (overwrite) an
existing dataset; an error (NC_EEXIST) is returned if the specified
dataset already exists.

The NC_SHARE flag is appropriate when one process may be writing the
dataset and one or more other processes reading the dataset
concurrently; it means that dataset accesses are not buffered and
caching is limited. Since the buffering scheme is optimized for
sequential access, programs that do not access data sequentially may
see some performance improvement by setting the NC_SHARE flag. This
flag is ignored for netCDF-4 files.

Setting NC_64BIT_OFFSET causes netCDF to create a 64-bit offset format
file, instead of a netCDF classic format file. The 64-bit offset
format imposes far fewer restrictions on very large (i.e. over 2 GB)
data files. See Large File Support.

Setting NC_64BIT_DATA (Alias NC_CDF5) causes netCDF to create a CDF-5
file format that supports large files (i.e. over 2GB) and large
variables (over 2B array elements.). See Large File Support.

Note that the flag NC_PNETCDF also exists as the combination of
NC_CDF5 or'd with NC_MPIIO to indicate that the pnetcdf library
should be used.

A zero value (defined for convenience as NC_CLOBBER) specifies the
default behavior: overwrite any existing dataset with the same file
name and buffer and cache accesses for efficiency. The dataset will be
in netCDF classic format. See NetCDF Classic Format Limitations.

Setting NC_NETCDF4 causes netCDF to create a HDF5/NetCDF-4 file.

Setting NC_CLASSIC_MODEL causes netCDF to enforce the classic data
model in this file. (This only has effect for netCDF-4/HDF5 files, as
classic and 64-bit offset files always use the classic model.) When
used with NC_NETCDF4, this flag ensures that the resulting
netCDF-4/HDF5 file may never contain any new constructs from the
enhanced data model. That is, it cannot contain groups, user defined
types, multiple unlimited dimensions, or new atomic types. The
advantage of this restriction is that such files are guaranteed to
work with existing netCDF software.

Setting NC_DISKLESS causes netCDF to create the file only in memory.
This allows for the use of files that have no long term purpose. Note that
with one exception, the in-memory file is destroyed upon calling
nc_close. If, however, the flag combination (NC_DISKLESS|NC_WRITE)
is used, then at close, the contents of the memory file will be
made persistent in the file path that was specified in the nc_create
call. If NC_DISKLESS is going to be used for creating a large classic file,
it behooves one to use either nc__create or nc_create_mp and specify
an appropriately large value of the initialsz parameter to avoid
to many extensions to the in-memory space for the file.
This flag applies to files in classic format and to file in extended
format (netcdf-4).

Normally, NC_DISKLESS allocates space in the heap for
storing the in-memory file. If, however, the ./configure
flags --enable-mmap is used, and the additional mode flag
NC_MMAP is specified, then the file will be created using
the operating system MMAP facility.
This flag only applies to files in classic format. Extended
format (netcdf-4) files will ignore the NC_MMAP flag.

Using NC_MMAP for nc_create is
only included for completeness vis-a-vis nc_open. The
ability to use MMAP is of limited use for nc_create because
nc_create is going to create the file in memory anyway.
Closing a MMAP'd file will be slightly faster, but not significantly.

Note that nc_create(path,cmode,ncidp) is equivalent to the invocation of
nc__create(path,cmode,NC_SIZEHINT_DEFAULT,NULL,ncidp).

\returns ::NC_NOERR No error.

\returns ::NC_ENOMEM System out of memory.

\returns ::NC_EHDFERR HDF5 error (netCDF-4 files only).

\returns ::NC_EFILEMETA Error writing netCDF-4 file-level metadata in
HDF5 file. (netCDF-4 files only).

\returns ::NC_EDISKLESS if there was an error in creating the
in-memory file.

\note When creating a netCDF-4 file HDF5 error reporting is turned
off, if it is on. This doesn't stop the HDF5 error stack from
recording the errors, it simply stops their display to the user
through stderr.

<h1>Examples</h1>

In this example we create a netCDF dataset named foo.nc; we want the
dataset to be created in the current directory only if a dataset with
that name does not already exist:

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo.nc", NC_NOCLOBBER, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

In this example we create a netCDF dataset named foo_large.nc. It will
be in the 64-bit offset format.

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo_large.nc", NC_NOCLOBBER|NC_64BIT_OFFSET, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

In this example we create a netCDF dataset named foo_HDF5.nc. It will
be in the HDF5 format.

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo_HDF5.nc", NC_NOCLOBBER|NC_NETCDF4, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

In this example we create a netCDF dataset named
foo_HDF5_classic.nc. It will be in the HDF5 format, but will not allow
the use of any netCDF-4 advanced features. That is, it will conform to
the classic netCDF-3 data model.

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo_HDF5_classic.nc", NC_NOCLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

In this example we create a in-memory netCDF classic dataset named
diskless.nc whose content will be lost when nc_close() is called.

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("diskless.nc", NC_DISKLESS, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

In this example we create a in-memory netCDF classic dataset named
diskless.nc and specify that it should be made persistent
in a file named diskless.nc when nc_close() is called.

@code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("diskless.nc", NC_DISKLESS|NC_WRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
@endcode

A variant of nc_create(), nc__create() (note the double underscore) allows
users to specify two tuning parameters for the file that it is
creating.  */
int
nc_create(const char *path, int cmode, int *ncidp)
{
   return nc__create(path,cmode,NC_SIZEHINT_DEFAULT,NULL,ncidp);
}

/*!
Create a netCDF file with some extra parameters controlling classic
file cacheing.

Like nc_create(), this function creates a netCDF file.

\param path The file name of the new netCDF dataset.

\param cmode The creation mode flag, the same as in nc_create().

\param initialsz On some systems, and with custom I/O layers, it may
be advantageous to set the size of the output file at creation
time. This parameter sets the initial size of the file at creation
time. This only applies to classic and 64-bit offset files.
The special value NC_SIZEHINT_DEFAULT (which is the value 0),
lets the netcdf library choose a suitable initial size.

\param chunksizehintp A pointer to the chunk size hint,
which controls a space versus time tradeoff, memory
allocated in the netcdf library versus number of system
calls. Because of internal requirements, the value may not
be set to exactly the value requested. The actual value
chosen is returned by reference. Using a NULL pointer or
having the pointer point to the value NC_SIZEHINT_DEFAULT
causes the library to choose a default. How the system
chooses the default depends on the system. On many systems,
the "preferred I/O block size" is available from the stat()
system call, struct stat member st_blksize. If this is
available it is used. Lacking that, twice the system
pagesize is used. Lacking a call to discover the system
pagesize, we just set default bufrsize to 8192. The bufrsize
is a property of a given open netcdf descriptor ncid, it is
not a persistent property of the netcdf dataset. This only
applies to classic and 64-bit offset files.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

\note This function uses the same return codes as the nc_create()
function.

<h1>Examples</h1>

In this example we create a netCDF dataset named foo_large.nc; we want
the dataset to be created in the current directory only if a dataset
with that name does not already exist. We also specify that bufrsize
and initial size for the file.

\code
#include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
     int intialsz = 2048;
     int *bufrsize;
        ...
     *bufrsize = 1024;
     status = nc__create("foo.nc", NC_NOCLOBBER, initialsz, bufrsize, &ncid);
     if (status != NC_NOERR) handle_error(status);
\endcode

\ingroup datasets

*/
int
nc__create(const char *path, int cmode, size_t initialsz,
	   size_t *chunksizehintp, int *ncidp)
{
   return NC_create(path, cmode, initialsz, 0,
		    chunksizehintp, 0, NULL, ncidp);

}
/**
\internal

\deprecated This function was used in the old days with the Cray at
NCAR. The Cray is long gone, and this call is supported only for
backward compatibility.

 */
int
nc__create_mp(const char *path, int cmode, size_t initialsz,
	      int basepe, size_t *chunksizehintp, int *ncidp)
{
   return NC_create(path, cmode, initialsz, basepe,
		    chunksizehintp, 0, NULL, ncidp);
}

/** \ingroup datasets
Open an existing netCDF file.

This function opens an existing netCDF dataset for access. It
determines the underlying file format automatically. Use the same call
to open a netCDF classic, 64-bit offset, or netCDF-4 file.

\param path File name for netCDF dataset to be opened. When DAP
support is enabled, then the path may be an OPeNDAP URL rather than a
file path.

\param mode The mode flag may include NC_WRITE (for read/write
access) and NC_SHARE (see below) and NC_DISKLESS (see below).

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

<h2>Open Mode</h2>

A zero value (or ::NC_NOWRITE) specifies the default behavior: open the
dataset with read-only access, buffering and caching accesses for
efficiency.

Otherwise, the open mode is ::NC_WRITE, ::NC_SHARE, or
::NC_WRITE|::NC_SHARE. Setting the ::NC_WRITE flag opens the dataset with
read-write access. ("Writing" means any kind of change to the dataset,
including appending or changing data, adding or renaming dimensions,
variables, and attributes, or deleting attributes.)

The NC_SHARE flag is only used for netCDF classic and 64-bit offset
files. It is appropriate when one process may be writing the dataset
and one or more other processes reading the dataset concurrently; it
means that dataset accesses are not buffered and caching is
limited. Since the buffering scheme is optimized for sequential
access, programs that do not access data sequentially may see some
performance improvement by setting the NC_SHARE flag.

This procedure may also be invoked with the NC_DISKLESS flag
set in the mode argument if the file to be opened is a
classic format file.  For nc_open(), this flag applies only
to files in classic format.  If the file is of type
NC_NETCDF4, then the NC_DISKLESS flag will be ignored.

If NC_DISKLESS is specified, then the whole file is read completely into
memory. In effect this creates an in-memory cache of the file.
If the mode flag also specifies NC_WRITE, then the in-memory cache
will be re-written to the disk file when nc_close() is called.
For some kinds of manipulations, having the in-memory cache can
speed up file processing. But in simple cases, non-cached
processing may actually be faster than using cached processing.
You will need to experiment to determine if the in-memory caching
is worthwhile for your application.

Normally, NC_DISKLESS allocates space in the heap for
storing the in-memory file. If, however, the ./configure
flags --enable-mmap is used, and the additional mode flag
NC_MMAP is specified, then the file will be opened using
the operating system MMAP facility.
This flag only applies to files in classic format. Extended
format (netcdf-4) files will ignore the NC_MMAP flag.

In most cases, using MMAP provides no advantage
for just NC_DISKLESS. The one case where using MMAP is an
advantage is when a file is to be opened and only a small portion
of its data is to be read and/or written.
In this scenario, MMAP will cause only the accessed data to be
retrieved from disk. Without MMAP, NC_DISKLESS will read the whole
file into memory on nc_open. Thus, MMAP will provide some performance
improvement in this case.

It is not necessary to pass any information about the format of the
file being opened. The file type will be detected automatically by the
netCDF library.

If a the path is a DAP URL, then the open mode is read-only.
Setting NC_WRITE will be ignored.

As of version 4.3.1.2, multiple calls to nc_open with the same
path will return the same ncid value.

\note When opening a netCDF-4 file HDF5 error reporting is turned off,
if it is on. This doesn't stop the HDF5 error stack from recording the
errors, it simply stops their display to the user through stderr.

nc_open()returns the value NC_NOERR if no errors occurred. Otherwise,
the returned status indicates an error. Possible causes of errors
include:

Note that nc_open(path,cmode,ncidp) is equivalent to the invocation of
nc__open(path,cmode,NC_SIZEHINT_DEFAULT,NULL,ncidp).

\returns ::NC_NOERR No error.

\returns ::NC_ENOMEM Out of memory.

\returns ::NC_EHDFERR HDF5 error. (NetCDF-4 files only.)

\returns ::NC_EDIMMETA Error in netCDF-4 dimension metadata. (NetCDF-4 files only.)

<h1>Examples</h1>

Here is an example using nc_open()to open an existing netCDF dataset
named foo.nc for read-only, non-shared access:

@code
#include <netcdf.h>
   ...
int status = NC_NOERR;
int ncid;
   ...
status = nc_open("foo.nc", 0, &ncid);
if (status != NC_NOERR) handle_error(status);
@endcode
*/
int
nc_open(const char *path, int mode, int *ncidp)
{
   return NC_open(path, mode, 0, NULL, 0, NULL, ncidp);
}

/** \ingroup datasets
Open a netCDF file with extra performance parameters for the classic
library.

\param path File name for netCDF dataset to be opened. When DAP
support is enabled, then the path may be an OPeNDAP URL rather than a
file path.

\param mode The mode flag may include NC_WRITE (for read/write
access) and NC_SHARE as in nc_open().

\param chunksizehintp A size hint for the classic library. Only
applies to classic and 64-bit offset files. See below for more
information.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

<h1>The chunksizehintp Parameter</h1>

The argument referenced by bufrsizehintp controls a space versus time
tradeoff, memory allocated in the netcdf library versus number of
system calls.

Because of internal requirements, the value may not be set to exactly
the value requested. The actual value chosen is returned by reference.

Using a NULL pointer or having the pointer point to the value
NC_SIZEHINT_DEFAULT causes the library to choose a default.
How the system chooses the default depends on the system. On
many systems, the "preferred I/O block size" is available from the
stat() system call, struct stat member st_blksize. If this is
available it is used. Lacking that, twice the system pagesize is used.

Lacking a call to discover the system pagesize, we just set default
bufrsize to 8192.

The bufrsize is a property of a given open netcdf descriptor ncid, it
is not a persistent property of the netcdf dataset.


\returns ::NC_NOERR No error.

\returns ::NC_ENOMEM Out of memory.

\returns ::NC_EHDFERR HDF5 error. (NetCDF-4 files only.)

\returns ::NC_EDIMMETA Error in netCDF-4 dimension metadata. (NetCDF-4
files only.)

*/
int
nc__open(const char *path, int mode,
	 size_t *chunksizehintp, int *ncidp)
{
   /* this API is for non-parallel access: TODO check for illegal cmode
    * flags, such as NC_PNETCDF, NC_MPIIO, or NC_MPIPOSIX, before entering
    * NC_open()? Note nc_open_par() also calls NC_open().
    */
   return NC_open(path, mode, 0, chunksizehintp, 0,
		  NULL, ncidp);
}

/** \ingroup datasets
Open a netCDF file with the contents taken from a block of memory.

\param path Must be non-null, but otherwise only used to set the dataset name.

\param mode the mode flags; Note that this procedure uses a limited set of flags because it forcibly sets NC_NOWRITE|NC_DISKLESS|NC_INMEMORY.

\param size The length of the block of memory being passed.

\param memory Pointer to the block of memory containing the contents
of a netcdf file.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

\returns ::NC_NOERR No error.

\returns ::NC_ENOMEM Out of memory.

\returns ::NC_EDISKLESS diskless io is not enabled for fails.

\returns ::NC_EINVAL, etc. other errors also returned by nc_open.

<h1>Examples</h1>

Here is an example using nc_open_mem() to open an existing netCDF dataset
named foo.nc for read-only, non-shared access. It differs from the nc_open()
example in that it assumes the contents of foo.nc have been read into memory.

@code
#include <netcdf.h>
#include <netcdf_mem.h>
   ...
int status = NC_NOERR;
int ncid;
size_t size;
void* memory;
   ...
size = <compute file size of foo.nc in bytes>;
memory = malloc(size);
   ...
status = nc_open_mem("foo.nc", 0, size, memory, &ncid);
if (status != NC_NOERR) handle_error(status);
@endcode
*/
int
nc_open_mem(const char* path, int mode, size_t size, void* memory, int* ncidp)
{
#ifdef USE_DISKLESS
    NC_MEM_INFO meminfo;

    /* Sanity checks */
    if(memory == NULL || size < MAGIC_NUMBER_LEN || path == NULL)
 	return NC_EINVAL;
    if(mode & (NC_WRITE|NC_MPIIO|NC_MPIPOSIX|NC_MMAP))
	return NC_EINVAL;
    mode |= (NC_INMEMORY|NC_DISKLESS);
    meminfo.size = size;
    meminfo.memory = memory;
    return NC_open(path, mode, 0, NULL, 0, &meminfo, ncidp);
#else
    return NC_EDISKLESS;
#endif
}

/**
\internal

\deprecated This function was used in the old days with the Cray at
NCAR. The Cray is long gone, and this call is supported only for
backward compatibility.

 */
int
nc__open_mp(const char *path, int mode, int basepe,
	    size_t *chunksizehintp, int *ncidp)
{
   return NC_open(path, mode, basepe, chunksizehintp,
		  0, NULL, ncidp);
}

/** \ingroup datasets
Get the file pathname (or the opendap URL) which was used to
open/create the ncid's file.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\param pathlen Pointer where length of path will be returned. Ignored
if NULL.

\param path Pointer where path name will be copied. Space must already
be allocated. Ignored if NULL.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.
*/
int
nc_inq_path(int ncid, size_t *pathlen, char *path)
{
   NC* ncp;
   int stat = NC_NOERR;
   if ((stat = NC_check_id(ncid, &ncp)))
      return stat;
   if(ncp->path == NULL) {
	if(pathlen) *pathlen = 0;
	if(path) path[0] = '\0';
   } else {
       if (pathlen) *pathlen = strlen(ncp->path);
       if (path) strcpy(path, ncp->path);
   }
   return stat;
}

/** \ingroup datasets
Put open netcdf dataset into define mode

The function nc_redef puts an open netCDF dataset into define mode, so
dimensions, variables, and attributes can be added or renamed and
attributes can be deleted.

For netCDF-4 files (i.e. files created with NC_NETCDF4 in the cmode in
their call to nc_create()), it is not necessary to call nc_redef()
unless the file was also created with NC_STRICT_NC3. For straight-up
netCDF-4 files, nc_redef() is called automatically, as needed.

For all netCDF-4 files, the root ncid must be used. This is the ncid
returned by nc_open() and nc_create(), and points to the root of the
hierarchy tree for netCDF-4 files.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Bad ncid.

\returns ::NC_EBADGRPID The ncid must refer to the root group of the
file, that is, the group returned by nc_open() or nc_create().

\returns ::NC_EINDEFINE Already in define mode.

\returns ::NC_EPERM File is read-only.

<h1>Example</h1>

Here is an example using nc_redef to open an existing netCDF dataset
named foo.nc and put it into define mode:

\code
#include <netcdf.h>
   ...
int status = NC_NOERR;
int ncid;
   ...
status = nc_open("foo.nc", NC_WRITE, &ncid);
if (status != NC_NOERR) handle_error(status);
   ...
status = nc_redef(ncid);
if (status != NC_NOERR) handle_error(status);
\endcode
 */
int
nc_redef(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->redef(ncid);
}

/** \ingroup datasets
Leave define mode

The function nc_enddef() takes an open netCDF dataset out of define
mode. The changes made to the netCDF dataset while it was in define
mode are checked and committed to disk if no problems
occurred. Non-record variables may be initialized to a "fill value" as
well with nc_set_fill(). The netCDF dataset is then placed in data
mode, so variable data can be read or written.

It's not necessary to call nc_enddef() for netCDF-4 files. With netCDF-4
files, nc_enddef() is called when needed by the netcdf-4 library. User
calls to nc_enddef() for netCDF-4 files still flush the metadata to
disk.

This call may involve copying data under some circumstances. For a
more extensive discussion see File Structure and Performance.

For netCDF-4/HDF5 format files there are some variable settings (the
compression, endianness, fletcher32 error correction, and fill value)
which must be set (if they are going to be set at all) between the
nc_def_var() and the next nc_enddef(). Once the nc_enddef() is called,
these settings can no longer be changed for a variable.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

If you use a group id (in a netCDF-4/HDF5 file), the enddef
will apply to the entire file. That means the enddef will not just end
define mode in one group, but in the entire file.

\returns ::NC_NOERR no error

\returns ::NC_EBADID Invalid ncid passed.

<h1>Example</h1>

Here is an example using nc_enddef() to finish the definitions of a new
netCDF dataset named foo.nc and put it into data mode:

\code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo.nc", NC_NOCLOBBER, &ncid);
     if (status != NC_NOERR) handle_error(status);

        ...  create dimensions, variables, attributes

     status = nc_enddef(ncid);
     if (status != NC_NOERR) handle_error(status);
\endcode
 */
int
nc_enddef(int ncid)
{
   int status = NC_NOERR;
   NC *ncp;
   status = NC_check_id(ncid, &ncp);
   if(status != NC_NOERR) return status;
   return ncp->dispatch->_enddef(ncid,0,1,0,1);
}

/** \ingroup datasets
Leave define mode with performance tuning

The function nc__enddef takes an open netCDF dataset out of define
mode. The changes made to the netCDF dataset while it was in define
mode are checked and committed to disk if no problems
occurred. Non-record variables may be initialized to a "fill value" as
well with nc_set_fill(). The netCDF dataset is then placed in data mode,
so variable data can be read or written.

This call may involve copying data under some circumstances. For a
more extensive discussion see File Structure and Performance.

\warning This function exposes internals of the netcdf version 1 file
format. Users should use nc_enddef() in most circumstances. This
function may not be available on future netcdf implementations.

The classic netcdf file format has three sections, the "header"
section, the data section for fixed size variables, and the data
section for variables which have an unlimited dimension (record
variables).

The header begins at the beginning of the file. The index (offset) of
the beginning of the other two sections is contained in the
header. Typically, there is no space between the sections. This causes
copying overhead to accrue if one wishes to change the size of the
sections, as may happen when changing names of things, text attribute
values, adding attributes or adding variables. Also, for buffered i/o,
there may be advantages to aligning sections in certain ways.

The minfree parameters allow one to control costs of future calls to
nc_redef, nc_enddef() by requesting that minfree bytes be available at
the end of the section.

The align parameters allow one to set the alignment of the beginning
of the corresponding sections. The beginning of the section is rounded
up to an index which is a multiple of the align parameter. The flag
value ALIGN_CHUNK tells the library to use the bufrsize (see above) as
the align parameter. It has nothing to do with the chunking
(multidimensional tiling) features of netCDF-4.

The file format requires mod 4 alignment, so the align parameters are
silently rounded up to multiples of 4. The usual call,

\code
     nc_enddef(ncid);
\endcode

is equivalent to

\code
     nc__enddef(ncid, 0, 4, 0, 4);
\endcode

The file format does not contain a "record size" value, this is
calculated from the sizes of the record variables. This unfortunate
fact prevents us from providing minfree and alignment control of the
"records" in a netcdf file. If you add a variable which has an
unlimited dimension, the third section will always be copied with the
new variable added.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\param h_minfree Sets the pad at the end of the "header" section.

\param v_align Controls the alignment of the beginning of the data
section for fixed size variables.

\param v_minfree Sets the pad at the end of the data section for fixed
size variables.

\param r_align Controls the alignment of the beginning of the data
section for variables which have an unlimited dimension (record
variables).

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.

 */
int
nc__enddef(int ncid, size_t h_minfree, size_t v_align, size_t v_minfree,
	   size_t r_align)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->_enddef(ncid,h_minfree,v_align,v_minfree,r_align);
}

/** \ingroup datasets
Synchronize an open netcdf dataset to disk

The function nc_sync() offers a way to synchronize the disk copy of a
netCDF dataset with in-memory buffers. There are two reasons you might
want to synchronize after writes:
- To minimize data loss in case of abnormal termination, or
- To make data available to other processes for reading immediately
  after it is written. But note that a process that already had the
  dataset open for reading would not see the number of records
  increase when the writing process calls nc_sync(); to accomplish this,
  the reading process must call nc_sync.

This function is backward-compatible with previous versions of the
netCDF library. The intent was to allow sharing of a netCDF dataset
among multiple readers and one writer, by having the writer call
nc_sync() after writing and the readers call nc_sync() before each
read. For a writer, this flushes buffers to disk. For a reader, it
makes sure that the next read will be from disk rather than from
previously cached buffers, so that the reader will see changes made by
the writing process (e.g., the number of records written) without
having to close and reopen the dataset. If you are only accessing a
small amount of data, it can be expensive in computer resources to
always synchronize to disk after every write, since you are giving up
the benefits of buffering.

An easier way to accomplish sharing (and what is now recommended) is
to have the writer and readers open the dataset with the NC_SHARE
flag, and then it will not be necessary to call nc_sync() at
all. However, the nc_sync() function still provides finer granularity
than the NC_SHARE flag, if only a few netCDF accesses need to be
synchronized among processes.

It is important to note that changes to the ancillary data, such as
attribute values, are not propagated automatically by use of the
NC_SHARE flag. Use of the nc_sync() function is still required for this
purpose.

Sharing datasets when the writer enters define mode to change the data
schema requires extra care. In previous releases, after the writer
left define mode, the readers were left looking at an old copy of the
dataset, since the changes were made to a new copy. The only way
readers could see the changes was by closing and reopening the
dataset. Now the changes are made in place, but readers have no
knowledge that their internal tables are now inconsistent with the new
dataset schema. If netCDF datasets are shared across redefinition,
some mechanism external to the netCDF library must be provided that
prevents access by readers during redefinition and causes the readers
to call nc_sync before any subsequent access.

When calling nc_sync(), the netCDF dataset must be in data mode. A
netCDF dataset in define mode is synchronized to disk only when
nc_enddef() is called. A process that is reading a netCDF dataset that
another process is writing may call nc_sync to get updated with the
changes made to the data by the writing process (e.g., the number of
records written), without having to close and reopen the dataset.

Data is automatically synchronized to disk when a netCDF dataset is
closed, or whenever you leave define mode.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.
 */
int
nc_sync(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->sync(ncid);
}

/** \ingroup datasets
No longer necessary for user to invoke manually.


\warning Users no longer need to call this function since it is called
automatically by nc_close() in case the dataset is in define mode and
something goes wrong with committing the changes. The function
nc_abort() just closes the netCDF dataset, if not in define mode. If
the dataset is being created and is still in define mode, the dataset
is deleted. If define mode was entered by a call to nc_redef(), the
netCDF dataset is restored to its state before definition mode was
entered and the dataset is closed.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\returns ::NC_NOERR No error.

<h1>Example</h1>

Here is an example using nc_abort to back out of redefinitions of a
dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int ncid, status, latid;
        ...
     status = nc_open("foo.nc", NC_WRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_redef(ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_def_dim(ncid, "lat", 18L, &latid);
     if (status != NC_NOERR) {
        handle_error(status);
        status = nc_abort(ncid);
        if (status != NC_NOERR) handle_error(status);
     }
\endcode

 */
int
nc_abort(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;

#ifdef USE_REFCOUNT
   /* What to do if refcount > 0? */
   /* currently, forcibly abort */
   ncp->refcount = 0;
#endif

   stat = ncp->dispatch->abort(ncid);
   del_from_NCList(ncp);
   free_NC(ncp);
   return stat;
}

/** \ingroup datasets
Close an open netCDF dataset

If the dataset in define mode, nc_enddef() will be called before
closing. (In this case, if nc_enddef() returns an error, nc_abort() will
automatically be called to restore the dataset to the consistent state
before define mode was last entered.) After an open netCDF dataset is
closed, its netCDF ID may be reassigned to the next netCDF dataset
that is opened or created.

\param ncid NetCDF ID, from a previous call to nc_open() or nc_create().

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid id passed.

\returns ::NC_EBADGRPID ncid did not contain the root group id of this
file. (NetCDF-4 only).

<h1>Example</h1>

Here is an example using nc_close to finish the definitions of a new
netCDF dataset named foo.nc and release its netCDF ID:

\code
     #include <netcdf.h>
        ...
     int status = NC_NOERR;
     int ncid;
        ...
     status = nc_create("foo.nc", NC_NOCLOBBER, &ncid);
     if (status != NC_NOERR) handle_error(status);

        ...   create dimensions, variables, attributes

     status = nc_close(ncid);
     if (status != NC_NOERR) handle_error(status);
\endcode

 */
int
nc_close(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;

#ifdef USE_REFCOUNT
   ncp->refcount--;
   if(ncp->refcount <= 0)
#endif
   {

	   stat = ncp->dispatch->close(ncid);
       /* Remove from the nc list */
       del_from_NCList(ncp);
       free_NC(ncp);
   }
   return stat;
}

/** \ingroup datasets
Change the fill-value mode to improve write performance.

This function is intended for advanced usage, to optimize writes under
some circumstances described below. The function nc_set_fill() sets the
fill mode for a netCDF dataset open for writing and returns the
current fill mode in a return parameter. The fill mode can be
specified as either ::NC_FILL or ::NC_NOFILL. The default behavior
corresponding to ::NC_FILL is that data is pre-filled with fill values,
that is fill values are written when you create non-record variables
or when you write a value beyond data that has not yet been
written. This makes it possible to detect attempts to read data before
it was written. For more information on the use of fill values see
Fill Values. For information about how to define your own fill values
see Attribute Conventions.

The behavior corresponding to ::NC_NOFILL overrides the default behavior
of prefilling data with fill values. This can be used to enhance
performance, because it avoids the duplicate writes that occur when
the netCDF library writes fill values that are later overwritten with
data.

A value indicating which mode the netCDF dataset was already in is
returned. You can use this value to temporarily change the fill mode
of an open netCDF dataset and then restore it to the previous mode.

After you turn on ::NC_NOFILL mode for an open netCDF dataset, you must
be certain to write valid data in all the positions that will later be
read. Note that nofill mode is only a transient property of a netCDF
dataset open for writing: if you close and reopen the dataset, it will
revert to the default behavior. You can also revert to the default
behavior by calling nc_set_fill() again to explicitly set the fill mode
to ::NC_FILL.

There are three situations where it is advantageous to set nofill
mode:
- Creating and initializing a netCDF dataset. In this case, you should
  set nofill mode before calling nc_enddef() and then write completely
  all non-record variables and the initial records of all the record
  variables you want to initialize.
- Extending an existing record-oriented netCDF dataset. Set nofill
  mode after opening the dataset for writing, then append the
  additional records to the dataset completely, leaving no intervening
  unwritten records.
- Adding new variables that you are going to initialize to an existing
  netCDF dataset. Set nofill mode before calling nc_enddef() then write
  all the new variables completely.

If the netCDF dataset has an unlimited dimension and the last record
was written while in nofill mode, then the dataset may be shorter than
if nofill mode was not set, but this will be completely transparent if
you access the data only through the netCDF interfaces.

The use of this feature may not be available (or even needed) in
future releases. Programmers are cautioned against heavy reliance upon
this feature.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\param fillmode Desired fill mode for the dataset, either ::NC_NOFILL or
::NC_FILL.

\param old_modep Pointer to location for returned current fill mode of
the dataset before this call, either ::NC_NOFILL or ::NC_FILL.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID The specified netCDF ID does not refer to an open
netCDF dataset.

\returns ::NC_EPERM The specified netCDF ID refers to a dataset open for
read-only access.

\returns ::NC_EINVAL The fill mode argument is neither ::NC_NOFILL nor
::NC_FILL.

<h1>Example</h1>

Here is an example using nc_set_fill() to set nofill mode for subsequent
writes of a netCDF dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int ncid, status, old_fill_mode;
        ...
     status = nc_open("foo.nc", NC_WRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);

        ...     write data with default prefilling behavior

     status = nc_set_fill(ncid, ::NC_NOFILL, &old_fill_mode);
     if (status != NC_NOERR) handle_error(status);

        ...    write data with no prefilling
\endcode
 */
int
nc_set_fill(int ncid, int fillmode, int *old_modep)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->set_fill(ncid,fillmode,old_modep);
}

/**
\internal

\deprecated This function was used in the old days with the Cray at
NCAR. The Cray is long gone, and this call is supported only for
backward compatibility.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.
 */
int
nc_inq_base_pe(int ncid, int *pe)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_base_pe(ncid,pe);
}

/**
\internal

\deprecated This function was used in the old days with the Cray at
NCAR. The Cray is long gone, and this call is supported only for
backward compatibility.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.
 */
int
nc_set_base_pe(int ncid, int pe)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->set_base_pe(ncid,pe);
}

/** \ingroup datasets
Inquire about the binary format of a netCDF file
as presented by the API.

This function returns the (rarely needed) format version.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\param formatp Pointer to location for returned format version, one of
NC_FORMAT_CLASSIC, NC_FORMAT_64BIT_OFFSET, NC_FORMAT_CDF5, NC_FORMAT_NETCDF4,
NC_FORMAT_NETCDF4_CLASSIC.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.

 */
int
nc_inq_format(int ncid, int *formatp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_format(ncid,formatp);
}

/** \ingroup datasets
Obtain more detailed (vis-a-vis nc_inq_format)
format information about an open dataset.

Note that the netcdf API will present the file
as if it had the format specified by nc_inq_format.
The true file format, however, may not even be
a netcdf file; it might be DAP, HDF4, or PNETCDF,
for example. This function returns that true file type.
It also returns the effective mode for the file.

\param ncid NetCDF ID, from a previous call to nc_open() or
nc_create().

\param formatp Pointer to location for returned true format.

\param modep Pointer to location for returned mode flags.

Refer to the actual list in the file netcdf.h to see the
currently defined set.

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.

 */
int
nc_inq_format_extended(int ncid, int *formatp, int *modep)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_format_extended(ncid,formatp,modep);
}

/**\ingroup datasets
Inquire about a file or group.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param ndimsp Pointer to location for returned number of dimensions
defined for this netCDF dataset. Ignored if NULL.

\param nvarsp Pointer to location for returned number of variables
defined for this netCDF dataset. Ignored if NULL.

\param nattsp Pointer to location for returned number of global
attributes defined for this netCDF dataset. Ignored if NULL.

\param unlimdimidp Pointer to location for returned ID of the
unlimited dimension, if there is one for this netCDF dataset. If no
unlimited length dimension has been defined, -1 is returned. Ignored
if NULL.  If there are multiple unlimited dimensions (possible only
for netCDF-4 files), only a pointer to the first is returned, for
backward compatibility.  If you want them all, use nc_inq_unlimids().

\returns ::NC_NOERR No error.

\returns ::NC_EBADID Invalid ncid passed.

<h1>Example</h1>

Here is an example using nc_inq to find out about a netCDF dataset
named foo.nc:

\code
     #include <netcdf.h>
        ...
     int status, ncid, ndims, nvars, ngatts, unlimdimid;
        ...
     status = nc_open("foo.nc", NC_NOWRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
     if (status != NC_NOERR) handle_error(status);
\endcode
 */
int
nc_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq(ncid,ndimsp,nvarsp,nattsp,unlimdimidp);
}

int
nc_inq_nvars(int ncid, int *nvarsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq(ncid, NULL, nvarsp, NULL, NULL);
}

/**\ingroup datasets
Inquire about a type.

Given an ncid and a typeid, get the information about a type. This
function will work on any type, including atomic and any user defined
type, whether compound, opaque, enumeration, or variable length array.

For even more information about a user defined type nc_inq_user_type().

\param ncid The ncid for the group containing the type (ignored for
atomic types).

\param xtype The typeid for this type, as returned by nc_def_compound,
nc_def_opaque, nc_def_enum, nc_def_vlen, or nc_inq_var, or as found in
netcdf.h in the list of atomic types (NC_CHAR, NC_INT, etc.).

\param name If non-NULL, the name of the user defined type will be
copied here. It will be NC_MAX_NAME bytes or less. For atomic types,
the type name from CDL will be given.

\param size If non-NULL, the (in-memory) size of the type in bytes
will be copied here. VLEN type size is the size of nc_vlen_t. String
size is returned as the size of a character pointer. The size may be
used to malloc space for the data, no matter what the type.

\returns ::NC_NOERR No error.

\returns ::NC_EBADTYPE Bad typeid.

\returns ::NC_ENOTNC4 Seeking a user-defined type in a netCDF-3 file.

\returns ::NC_ESTRICTNC3 Seeking a user-defined type in a netCDF-4 file
for which classic model has been turned on.

\returns ::NC_EBADGRPID Bad group ID in ncid.

\returns ::NC_EBADID Type ID not found.

\returns ::NC_EHDFERR An error was reported by the HDF5 layer.

<h1>Example</h1>

This example is from the test program tst_enums.c, and it uses all the
possible inquiry functions on an enum type.

\code
           if (nc_inq_user_type(ncid, typeids[0], name_in, &base_size_in, &base_nc_type_in,
                                &nfields_in, &class_in)) ERR;
           if (strcmp(name_in, TYPE_NAME) || base_size_in != sizeof(int) ||
               base_nc_type_in != NC_INT || nfields_in != NUM_MEMBERS || class_in != NC_ENUM) ERR;
           if (nc_inq_type(ncid, typeids[0], name_in, &base_size_in)) ERR;
           if (strcmp(name_in, TYPE_NAME) || base_size_in != sizeof(int)) ERR;
           if (nc_inq_enum(ncid, typeids[0], name_in, &base_nc_type, &base_size_in, &num_members)) ERR;
           if (strcmp(name_in, TYPE_NAME) || base_nc_type != NC_INT || num_members != NUM_MEMBERS) ERR;
           for (i = 0; i < NUM_MEMBERS; i++)
           {
              if (nc_inq_enum_member(ncid, typeid, i, name_in, &value_in)) ERR;
              if (strcmp(name_in, member_name[i]) || value_in != member_value[i]) ERR;
              if (nc_inq_enum_ident(ncid, typeid, member_value[i], name_in)) ERR;
              if (strcmp(name_in, member_name[i])) ERR;
           }

           if (nc_close(ncid)) ERR;
\endcode
 */
int
nc_inq_type(int ncid, nc_type xtype, char *name, size_t *size)
{
   NC* ncp;
   int stat;

   /* Do a quick triage on xtype */
   if(xtype <= NC_NAT) return NC_EBADTYPE;
   /* For compatibility, we need to allow inq about
      atomic types, even if ncid is ill-defined */
   if(xtype <= ATOMICTYPEMAX4) {
      if(name) strncpy(name,NC_atomictypename(xtype),NC_MAX_NAME);
      if(size) *size = NC_atomictypelen(xtype);
      return NC_NOERR;
   }
   /* Apparently asking about a user defined type, so we need
      a valid ncid */
   stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) /* bad ncid */
      return NC_EBADTYPE;
   /* have good ncid */
   return ncp->dispatch->inq_type(ncid,xtype,name,size);
}

/**
\internal
\ingroup dispatch

Create a file, calling the appropriate dispatch create call.

For create, we have the following pieces of information to use to
determine the dispatch table:
- path
- cmode

\param path0 The file name of the new netCDF dataset.

\param cmode The creation mode flag, the same as in nc_create().

\param initialsz This parameter sets the initial size of the file at creation
time. This only applies to classic and 64-bit offset files.

\param basepe Deprecated parameter from the Cray days.

\param chunksizehintp A pointer to the chunk size hint. This only
applies to classic and 64-bit offset files.

\param useparallel Non-zero if parallel I/O is to be used on this
file.

\param parameters Pointer to MPI comm and info.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

\returns ::NC_NOERR No error.
*/
int
NC_create(const char *path0, int cmode, size_t initialsz,
	  int basepe, size_t *chunksizehintp, int useparallel,
	  void* parameters, int *ncidp)
{
   int stat = NC_NOERR;
   NC* ncp = NULL;
   NC_Dispatch* dispatcher = NULL;
   /* Need three pieces of information for now */
   int model = NC_FORMATX_UNDEFINED; /* one of the NC_FORMATX values */
   int isurl = 0;   /* dap or cdmremote or neither */
   int xcmode = 0; /* for implied cmode flags */
   char* path = NULL;

   TRACE(nc_create);
   if(path0 == NULL)
	return NC_EINVAL;
   /* Initialize the dispatch table. The function pointers in the
    * dispatch table will depend on how netCDF was built
    * (with/without netCDF-4, DAP, CDMREMOTE). */
   if(!NC_initialized)
   {
      if ((stat = nc_initialize()))
	 return stat;
   }

#ifndef USE_DISKLESS
   cmode &= (~ NC_DISKLESS); /* Force off */
#endif

#ifdef WINPATH
   /* Need to do path conversion */
   path = NCpathcvt(path0);
fprintf(stderr,"XXX: path0=%s path=%s\n",path0,path); fflush(stderr);
#else
   path = nulldup(path0);
#endif

#ifdef USE_REFCOUNT
   /* If this path is already open, then fail */
   ncp = find_in_NCList_by_name(path);
   if(ncp != NULL) {
	nullfree(path);
	return NC_ENFILE;
   }
#endif

    {
	char* newpath = NULL;
        model = NC_urlmodel(path,cmode,&newpath);
        isurl = (model != 0);
        if(isurl) {
	    nullfree(path);
	    path = newpath;
	}
    }

   /* Look to the incoming cmode for hints */
   if(model == NC_FORMATX_UNDEFINED) {
#ifdef USE_NETCDF4
      if((cmode & NC_NETCDF4) == NC_NETCDF4)
	model = NC_FORMATX_NC4;
      else
#endif
#ifdef USE_PNETCDF
      /* pnetcdf is used for parallel io on CDF-1, CDF-2, and CDF-5 */
      if((cmode & NC_MPIIO) == NC_MPIIO)
	model = NC_FORMATX_PNETCDF;
      else
#endif
	{}
    }
    if(model == NC_FORMATX_UNDEFINED) {
      /* Check default format (not formatx) */
      int format = nc_get_default_format();
      switch (format) {
#ifdef USE_NETCDF4
	 case NC_FORMAT_NETCDF4:
	    xcmode |= NC_NETCDF4;
	    model = NC_FORMATX_NC4;
	    break;
	 case NC_FORMAT_NETCDF4_CLASSIC:
	    xcmode |= NC_CLASSIC_MODEL;
	    model = NC_FORMATX_NC4;
	    break;
#endif
#ifdef USE_CDF5
	 case NC_FORMAT_CDF5:
	    xcmode |= NC_64BIT_DATA;
	    model = NC_FORMATX_NC3;
	    break;
#endif
      case NC_FORMAT_64BIT_OFFSET:
	    xcmode |= NC_64BIT_OFFSET;
	    model = NC_FORMATX_NC3;
	    break;
	 case NC_FORMAT_CLASSIC:
	    model = NC_FORMATX_NC3;
	    break;
	 default:
	    model = NC_FORMATX_NC3;
	    break;
      }
   }

   /* Add inferred flags */
   cmode |= xcmode;

   /* Clean up illegal combinations */
   if((cmode & (NC_64BIT_OFFSET|NC_64BIT_DATA)) == (NC_64BIT_OFFSET|NC_64BIT_DATA))
	cmode &= ~(NC_64BIT_OFFSET); /*NC_64BIT_DATA=>NC_64BIT_OFFSET*/

   if((cmode & NC_MPIIO) && (cmode & NC_MPIPOSIX))
      return  NC_EINVAL;

   if (dispatcher == NULL)
   {

      /* Figure out what dispatcher to use */
#ifdef USE_NETCDF4
      if(model == (NC_FORMATX_NC4))
 	dispatcher = NC4_dispatch_table;
      else
#endif /*USE_NETCDF4*/
#ifdef USE_PNETCDF
      if(model == (NC_FORMATX_PNETCDF))
	dispatcher = NCP_dispatch_table;
      else
#endif
      if(model == (NC_FORMATX_NC3))
 	dispatcher = NC3_dispatch_table;
      else
	 return NC_ENOTNC;
   }

   /* Create the NC* instance and insert its dispatcher */
   stat = new_NC(dispatcher,path,cmode,&ncp);
   nullfree(path); path = NULL; /* no longer needed */

   if(stat) return stat;

   /* Add to list of known open files and define ext_ncid */
   add_to_NCList(ncp);

#ifdef USE_REFCOUNT
   /* bump the refcount */
   ncp->refcount++;
#endif

   /* Assume create will fill in remaining ncp fields */
   if ((stat = dispatcher->create(ncp->path, cmode, initialsz, basepe, chunksizehintp,
				   useparallel, parameters, dispatcher, ncp))) {
	del_from_NCList(ncp); /* oh well */
	free_NC(ncp);
     } else {
       if(ncidp)*ncidp = ncp->ext_ncid;
     }
   return stat;
}

/**
\internal
\ingroup dispatch

Open a netCDF file (or remote dataset) calling the appropriate
dispatch function.

For open, we have the following pieces of information to use to determine the dispatch table.
- table specified by override
- path
- cmode
- the contents of the file (if it exists), basically checking its magic number.

\returns ::NC_NOERR No error.
*/
int
NC_open(const char *path0, int cmode,
	int basepe, size_t *chunksizehintp,
        int useparallel, void* parameters,
        int *ncidp)
{
   int stat = NC_NOERR;
   NC* ncp = NULL;
   NC_Dispatch* dispatcher = NULL;
   int inmemory = 0;
   int diskless = 0;
   /* Need pieces of information for now to decide model*/
   int model = 0;
   int isurl = 0;
   int version = 0;
   int flags = 0;
   char* path = NULL;

   TRACE(nc_open);
   if(!NC_initialized) {
      stat = nc_initialize();
      if(stat) return stat;
   }

   /* Attempt to do file path conversion: note that this will do
      nothing if path is a 'file:...' url, so it will need to be
      repeated in protocol code: libdap2 and libdap4
    */

#ifndef USE_DISKLESS
   /* Clean up cmode */
   cmode &= (~ NC_DISKLESS);
#endif

   inmemory = ((cmode & NC_INMEMORY) == NC_INMEMORY);
   diskless = ((cmode & NC_DISKLESS) == NC_DISKLESS);


#ifdef WINPATH
   path = NCpathcvt(path0);
#else
   path = nulldup(path0);
#endif

#ifdef USE_REFCOUNT
   /* If this path is already open, then bump the refcount and return it */
   ncp = find_in_NCList_by_name(path);
   if(ncp != NULL) {
	nullfree(path);
	ncp->refcount++;
	if(ncidp) *ncidp = ncp->ext_ncid;
	return NC_NOERR;
   }
#endif

   if(!inmemory) {
	char* newpath = NULL;
        model = NC_urlmodel(path,cmode,&newpath);
        isurl = (model != 0);
	if(isurl) {
	    nullfree(path);
	    path = newpath;
	} else
	    nullfree(newpath);
    }
    if(model == 0) {
	version = 0;
	/* Try to find dataset type */
	if(useparallel) flags |= NC_MPIIO;
	if(inmemory) flags |= NC_INMEMORY;
	if(diskless) flags |= NC_DISKLESS;
	stat = NC_check_file_type(path,flags,parameters,&model,&version);
        if(stat == NC_NOERR) {
   	if(model == 0)
	    return NC_ENOTNC;
	} else /* presumably not a netcdf file */
	    return stat;
    }

   if(model == 0) {
	fprintf(stderr,"Model == 0\n");
	return NC_ENOTNC;
   }

   /* Force flag consistentcy */
   if(model == NC_FORMATX_NC4 || model == NC_FORMATX_DAP4)
      cmode |= NC_NETCDF4;
   else if(model == NC_FORMATX_DAP2) {
      cmode &= ~NC_NETCDF4;
      cmode &= ~NC_PNETCDF;
      cmode &= ~NC_64BIT_OFFSET;
   } else if(model == NC_FORMATX_NC3) {
      cmode &= ~NC_NETCDF4; /* must be netcdf-3 (CDF-1, CDF-2, CDF-5) */
      /* User may want to open file using the pnetcdf library */
      if(cmode & NC_PNETCDF) {
         /* dispatch is determined by cmode, rather than file format */
         model = NC_FORMATX_PNETCDF;
      }
      /* For opening an existing file, flags NC_64BIT_OFFSET and NC_64BIT_DATA
       * will be ignored, as the file is already in either CDF-1, 2, or 5
       * format. However, below we add the file format info to cmode so the
       * internal netcdf file open subroutine knows what file format to open.
       * The mode will be saved in ncp->mode, to be used by
       * nc_inq_format_extended() to report the file format.
       * See NC3_inq_format_extended() in libsrc/nc3internal.c for example.
       */
      if(version == 2) cmode |= NC_64BIT_OFFSET;
      else if(version == 5) {
        cmode |= NC_64BIT_DATA;
        cmode &= ~(NC_64BIT_OFFSET); /*NC_64BIT_DATA=>NC_64BIT_OFFSET*/
      }
   } else if(model == NC_FORMATX_PNETCDF) {
     cmode &= ~(NC_NETCDF4|NC_64BIT_OFFSET);
     cmode |= NC_64BIT_DATA;
   }

   if((cmode & NC_MPIIO && cmode & NC_MPIPOSIX))
     return  NC_EINVAL;

   /* override any other table choice */
   if(dispatcher != NULL) goto havetable;

   /* Figure out what dispatcher to use */
#if defined(ENABLE_DAP)
   if(model == (NC_FORMATX_DAP2))
	dispatcher = NCD2_dispatch_table;
   else
#endif
#if defined(ENABLE_DAP4)
   if(model == (NC_FORMATX_DAP4))
	dispatcher = NCD4_dispatch_table;
   else
#endif
#if  defined(USE_PNETCDF)
   if(model == (NC_FORMATX_PNETCDF))
	dispatcher = NCP_dispatch_table;
   else
#endif
#if defined(USE_NETCDF4)
   if(model == (NC_FORMATX_NC4))
	dispatcher = NC4_dispatch_table;
   else
#endif
   if(model == (NC_FORMATX_NC3))
	dispatcher = NC3_dispatch_table;
   else
      return  NC_ENOTNC;

havetable:

   if(dispatcher == NULL)
	return NC_ENOTNC;

   /* Create the NC* instance and insert its dispatcher */
   stat = new_NC(dispatcher,path,cmode,&ncp);
   nullfree(path); path = NULL; /* no longer need path */
   if(stat) return stat;

   /* Add to list of known open files */
   add_to_NCList(ncp);

#ifdef USE_REFCOUNT
   /* bump the refcount */
   ncp->refcount++;
#endif

   /* Assume open will fill in remaining ncp fields */
   stat = dispatcher->open(ncp->path, cmode, basepe, chunksizehintp,
			   useparallel, parameters, dispatcher, ncp);
   if(stat == NC_NOERR) {
     if(ncidp) *ncidp = ncp->ext_ncid;
   } else {
	del_from_NCList(ncp);
	free_NC(ncp);
   }
   return stat;
}

/*Provide an internal function for generating pseudo file descriptors
  for systems that are not file based (e.g. dap, memio).
*/

/* Static counter for pseudo file descriptors (incremented) */
static int pseudofd = 0;

/* Create a pseudo file descriptor that does not
   overlap real file descriptors
*/
int
nc__pseudofd(void)
{
    if(pseudofd == 0)  {
        int maxfd = 32767; /* default */
#ifdef HAVE_GETRLIMIT
        struct rlimit rl;
        if(getrlimit(RLIMIT_NOFILE,&rl) == 0) {
	    if(rl.rlim_max != RLIM_INFINITY)
	        maxfd = (int)rl.rlim_max;
	    if(rl.rlim_cur != RLIM_INFINITY)
	        maxfd = (int)rl.rlim_cur;
	}
	pseudofd = maxfd+1;
#endif
    }
    return pseudofd++;
}
