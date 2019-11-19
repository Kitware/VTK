/*
  System dependend definitions
*/

#ifndef _H5PART_TYPES_H_
#define _H5PART_TYPES_H_

#ifdef   WIN32
typedef __int64			int64_t;
#endif /* WIN32 */

typedef int64_t			h5part_int64_t;
typedef int			h5part_int32_t;
typedef double			h5part_float64_t;
typedef float			h5part_float32_t;
typedef h5part_int64_t (*h5part_error_handler)( const char*, const h5part_int64_t, const char*,...)
#ifdef __GNUC__
__attribute__ ((format (printf, 3, 4)))
#endif
 ;

#if defined(PARALLEL_IO) || defined(H5_HAVE_PARALLEL)
typedef MPI_Comm		H5_Comm;
#else
typedef unsigned long		H5_Comm;
#endif

#define H5PART_STEPNAME_LEN	64
#define H5PART_DATANAME_LEN	64

struct H5BlockFile;

/**
   \struct H5PartFile

   This is an essentially opaque datastructure that
   acts as the filehandle for all practical purposes.
   It is created by H5PartOpenFile<xx>() and destroyed by
   H5PartCloseFile().  
*/
VTKH5PART_EXPORT
struct H5PartFile {
	hid_t	file;
	char	groupname_step[H5PART_STEPNAME_LEN];
	int	stepno_width;
	int	empty;
       
	char flags;

	h5part_int64_t timestep;
	hsize_t nparticles;

	hid_t timegroup;
	hid_t shape;
	hid_t xfer_prop;
	hid_t access_prop;
	hid_t dcreate_prop;
	hid_t fcreate_prop;

	/* the dataspace on disk for the current view */
	hid_t diskshape;
	/* the dataspace in memory for the current view */
	hid_t memshape;

	h5part_int64_t viewstart; /* -1 if no view is available: A "view" looks */
	h5part_int64_t viewend;   /* at a subset of the data. */
	char viewindexed; /* flag for an indexed view */
  
	/**
	   the number of particles in each processor.
	   With respect to the "VIEW", these numbers
	   can be regarded as non-overlapping subsections
	   of the particle array stored in the file.
	   So they can be used to compute the offset of
	   the view for each processor
	*/
	h5part_int64_t *pnparticles;

	/**
	   Number of processors
	*/
	int nprocs;
	
	/**
	   The index of the processor this process is running on.
	*/
	int myproc;

	/**
	   MPI communicator
	*/
	H5_Comm comm;

	int throttle;

	struct H5BlockStruct *block;
	h5part_int64_t (*close_block)(struct H5PartFile *f);

#ifdef PARALLEL_IO
	struct H5MultiBlockStruct *multiblock;
	h5part_int64_t (*close_multiblock)(struct H5PartFile *f);
#endif
};

typedef struct H5PartFile H5PartFile;

#ifdef IPL_XT3
# define SEEK_END 2 
#endif

#endif
