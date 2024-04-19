/*!
  \defgroup h5multiblock_c_api H5MultiBlock C API

  This package includes C code for writing and reading 3D datasets intended
  for distributed memory applications. The underlying file format is H5Part,
  a simple data storage schema and API derived from HDF5. In particular, we
  use the H5Block subset of H5Part for managing 3D field data. To achieve large
  contiguous reads and writes, the datasets use HDF5's chunking mechanism to
  reorder the layout on disk such that local subfields (or "blocks") of the
  3D field are stored contiguously (as opposed to storing an entire column
  contiguously, as in the Fortran column-major layout for an array). We refer
  to this chunked layout as "multiblock" and the column-major layout as
  "uniblock." Multiblocked datasets exhibit the following constraints:

  * All blocks must be the same size (a requirement of the HDF5 chunking
    mechanism).

  * The block dimensions must divide the field dimensions, to prevent fringe
    data that will lower performance by interfering with contiguous I/O
    operations.

  * For load-balancing purposes, the total number of blocks that the field
    is decomposed into must be a multiple of the number of nodes in the
    distributed system. (For ease of implementation, we require that the
    number of blocks equal the number of nodes.)

*/

/*!
  \ingroup h5multiblock_c_api
  \defgroup h5multiblock_model	Setting up the Data Model
*/  

/*!
  \ingroup h5multiblock_c_api
  \defgroup h5multiblock_data	Reading and Writing Datasets
*/  

/*!
  \ingroup h5multiblock_c_api
  \defgroup h5multiblock_attrib	Reading and Writing Attributes
*/


/*!
  \internal

  \defgroup h5multiblock_static H5MultiBlock Static
*/

/*!
  \internal

  \defgroup h5multiblock_private H5MultiBlock Private
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vtk_hdf5.h>
#include "H5Part.h"
#include "H5PartErrors.h"
#include "H5PartPrivate.h"

#include "H5Block.h"
#include "H5BlockTypes.h"
#include "H5BlockErrors.h"
#include "H5BlockPrivate.h"

#include "H5MultiBlock.h"
#include "H5MultiBlockTypes.h"
#include "H5MultiBlockErrors.h"
#include "H5MultiBlockPrivate.h"

#ifdef PARALLEL_IO

#define HALO_EXCHANGE_METHOD _halo_exchange_vectors

/*******************************************************************************
* Static functions
*******************************************************************************/

/*!
  \ingroup h5multiblock_static

  \internal

  Check whether \c f points to a valid file handle.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_H5MultiBlock_file_is_valid (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	if ( f == NULL )
		return H5PART_ERR_BADFD;
	if ( f->file == 0 )
		return H5PART_ERR_BADFD;
	if ( f->block == NULL )
		return H5PART_ERR_BADFD;
	if ( f->multiblock == NULL )
		return H5PART_ERR_BADFD;
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Gets the block decomposition and offsets.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_get_decomp_and_offsets (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	h5part_int64_t i, j, k;
	h5part_int64_t nblocks;

	struct H5MultiBlockStruct *mb = f->multiblock;

	mb->decomp[0] = mb->field_dims[0] / mb->block_dims[0];
	mb->decomp[1] = mb->field_dims[1] / mb->block_dims[1];
	mb->decomp[2] = mb->field_dims[2] / mb->block_dims[2];

	if (f->myproc == 0) {
		_H5Part_print_info ("Block decomposition: (%ld,%ld,%ld)",
				mb->decomp[2],
				mb->decomp[1],
				mb->decomp[0] );
	}

	_H5Part_print_debug ("PROC[%d]: Block decomposition: (%ld,%ld,%ld)",
			f->myproc,
			mb->decomp[2],
			mb->decomp[1],
			mb->decomp[0] );

        int rank = (f->myproc + mb->proc_shift) % f->nprocs;
	k = rank % mb->decomp[2];
	j = (rank / mb->decomp[2]) % mb->decomp[1];
	i = rank / (mb->decomp[2] * mb->decomp[1]);

	/* keep track of blocks that border the edges of the field */
	if (i == 0)			mb->field_edges |= H5MB_EDGE_Z0;
	if (i == mb->decomp[0] - 1)	mb->field_edges |= H5MB_EDGE_Z1;
	if (j == 0)			mb->field_edges |= H5MB_EDGE_Y0;
	if (j == mb->decomp[1] - 1)	mb->field_edges |= H5MB_EDGE_Y1;
	if (k == 0)			mb->field_edges |= H5MB_EDGE_X0;
	if (k == mb->decomp[2] - 1)	mb->field_edges |= H5MB_EDGE_X1;

	mb->offsets[0] = i * mb->block_dims[0];
	mb->offsets[1] = j * mb->block_dims[1];
	mb->offsets[2] = k * mb->block_dims[2];

	_H5Part_print_debug ("PROC[%d]: Block offsets: (%ld,%ld,%ld)",
			f->myproc,
			mb->offsets[2],
			mb->offsets[1],
			mb->offsets[0] );

	nblocks = mb->decomp[0] * mb->decomp[1] * mb->decomp[2];

	if (f->myproc == 0) {
		_H5Part_print_info ("Number of blocks: %ld", nblocks);
	}

	if (nblocks != f->nprocs) {
		return HANDLE_H5PART_BLOCK_DECOMP_ERR;
	}

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Calculates an integer \c divisor of \c m that is >= the \c n root of \c m.
  
  Used for finding block decompositions for an arbitrary number of processors.

  \return \c divisor
*/
static int
_nth_root_int_divisor (const int m, const int n)
{
	int i, root;
	double p;

	p = 1.0 / (double)n;
	root = (int) ceil ( pow ( (double)m, p ) );
	for (i=root; i<=m; i++)
	{
		if (m % i == 0) return i;
	}

	return i;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Allocates a block using the dimensions read from the file and the halo
  radii specified by the user.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_alloc_block (
	const H5PartFile *f,		/*!< IN: file handle */
	char **data,			/*!< IN/OUT: buffer to alloc */
	hid_t type			/*!< IN: HDF5 datatype of buffer */
	) {

	size_t typesize;
	h5part_int64_t nelems;

	struct H5MultiBlockStruct *mb = f->multiblock;

	/* size of datatype */
	typesize = H5Tget_size ( type );

	/* number of elements */
	nelems = mb->halo_dims[0] * mb->halo_dims[1] * mb->halo_dims[2];

	*data = (char*) malloc ( nelems * typesize );
	if ( ! *data ) return HANDLE_H5PART_NOMEM_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Takes a contiguous buffer of data and rearranges it in place to add padding
  with \c radius layers. Assumes that \c buffer is already large enough to
  perform this operation.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_pad_block (
	const H5PartFile *f,		/*!< IN: file handle */
	char *data,			/*!< IN/OUT: buffer to pad */
	hid_t type			/*!< IN: HDF5 datatype of buffer */
	) {

	size_t typesize;
	h5part_int64_t j, k;
	h5part_int64_t iDst, iSrc;
	h5part_int64_t xSize, xySize;
	h5part_int64_t hxSize, hxySize;
	h5part_int64_t hxInset;

	struct H5MultiBlockStruct *mb = f->multiblock;

	/* size of datatype */
	typesize = H5Tget_size ( type );

	/* size of row in original block */
	xSize = mb->block_dims[2] * typesize;

	/* size of slab in original block */
	xySize = xSize * mb->block_dims[1];

	/* size of row/slab with halo regions */
	hxSize = mb->halo_dims[2] * typesize;
	hxySize = hxSize * mb->halo_dims[1];

	/* inset of row in halo region */
	hxInset = mb->halo_radii[2] * typesize;
	
	for (k=(mb->block_dims[0]-1);k>=0;k--)
	{
		for (j=(mb->block_dims[1]-1);j>=0;j--)
		{
			iSrc =	k*xySize + j*xSize;

			iDst =	(k + mb->halo_radii[0]) * hxySize +
				(j + mb->halo_radii[1]) * hxSize +
				hxInset;

			memcpy ( data + iDst, data + iSrc, xSize );
		}
	}

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Zeroes the padding of a block. This routine uses the 0 value for the specific datatype
  (int, float, etc.) so that it is safe even with floating point implementations that do
  not use all bytes zero to represent 0.F.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_zero_block (
	const H5PartFile *f,		/*!< IN: file handle */
	char *data,			/*!< IN/OUT: buffer to pad */
	hid_t type			/*!< IN: HDF5 datatype of buffer */
	) {

	h5part_int64_t i, j, k;
        h5part_int64_t offset;
	size_t typesize;
	char zeroval[8];
	h5part_int64_t xySize, xSize;

	struct H5MultiBlockStruct *mb = f->multiblock;

	typesize = H5Tget_size ( type );

	if (type == H5T_NATIVE_DOUBLE) {
		typesize = sizeof (h5part_float64_t);
		h5part_float64_t zero = 0.0;
		memcpy ( zeroval, &zero, typesize );
	} else if (type == H5T_NATIVE_FLOAT) {
		typesize = sizeof (h5part_float32_t);
		h5part_float32_t zero = 0.F;
		memcpy ( zeroval, &zero, typesize );
	} else if (type == H5T_NATIVE_INT64) {
		typesize = sizeof (h5part_int64_t);
		h5part_int64_t zero = 0;
		memcpy ( zeroval, &zero, typesize );
	} else if (type == H5T_NATIVE_INT32) {
		typesize = sizeof (h5part_int32_t);
		h5part_int32_t zero = 0;
		memcpy ( zeroval, &zero, typesize );
	}

	xSize = mb->halo_dims[2] * typesize;
	xySize = xSize * mb->halo_dims[1];

#define ZERO_SLAB(I0,J0,K0,I1,J1,K1,X,Y,Z) \
	for (i=I0; i<I1; i++) { \
		for (j=J0; j<J1; j++) { \
			for (k=K0; k<K1; k++) { \
				offset = i*X + j*Y + k*Z; \
				memcpy ( data + offset, zeroval, typesize ); \
			} \
		} \
	}

        ZERO_SLAB (	0, 0, 0,
			mb->halo_radii[0], mb->halo_dims[1], mb->halo_dims[2],
			xySize, xSize, typesize );
        ZERO_SLAB (	0, 0, 0,
			mb->halo_radii[1], mb->halo_dims[0], mb->halo_dims[2],
			xSize, xySize, typesize );
	ZERO_SLAB (	0, 0, 0,
			mb->halo_radii[2], mb->halo_dims[0], mb->halo_dims[1],
			typesize, xySize, xSize );
	ZERO_SLAB (	mb->halo_dims[0] - mb->halo_radii[0], 0, 0,
			mb->halo_dims[0], mb->halo_dims[1], mb->halo_dims[2],
			xySize, xSize, typesize );
	ZERO_SLAB (	mb->halo_dims[1] - mb->halo_radii[1], 0, 0,
			mb->halo_dims[1], mb->halo_dims[0], mb->halo_dims[2],
			xSize, xySize, typesize );
	ZERO_SLAB (	mb->halo_dims[2] - mb->halo_radii[2], 0, 0,
			mb->halo_dims[2], mb->halo_dims[0], mb->halo_dims[1],
			typesize, xySize, xSize );

	return H5PART_SUCCESS;
}

static h5part_int64_t
_halo_exchange_vector (
	const H5PartFile *f,		/*!< IN: file handle */
	char *data,			/*!< IN/OUT: local buffer */
	MPI_Datatype *halo_vector,
	int send_offset,
	int recv_offset,
	char send_only_edge,
	char recv_only_edge,
	int proc_spacing
	) {

	int ret;
	MPI_Status status;

	struct H5MultiBlockStruct *mb = f->multiblock;

	if ( mb->field_edges & recv_only_edge ) {
		ret = MPI_Recv (
				data + recv_offset,
				1,
				*halo_vector,
				f->myproc - proc_spacing,
				f->myproc, // use destination proc as tag
				f->comm,
				&status );
	} else if ( mb->field_edges & send_only_edge ) {
		ret = MPI_Send (
				data + send_offset,
				1,
				*halo_vector,
				f->myproc + proc_spacing,
				f->myproc + proc_spacing,
				f->comm );
	} else {
		ret = MPI_Sendrecv (
				data + send_offset,
				1,
				*halo_vector,
				f->myproc + proc_spacing,
				f->myproc + proc_spacing,
				data + recv_offset,
				1,
				*halo_vector,
				f->myproc - proc_spacing,
				f->myproc,
				f->comm,
				&status );
	}

	if ( ret != MPI_SUCCESS ) return HANDLE_MPI_SENDRECV_ERR;

	return H5PART_SUCCESS;
}

static h5part_int64_t
_halo_exchange_buffer (
	const H5PartFile *f,		/*!< IN: file handle */
	char *send_buffer,
	char *recv_buffer,
	int bufsize,
	char send_only_edge,
	char recv_only_edge,
	int proc_spacing
	) {

	int ret;
	MPI_Status status;

	struct H5MultiBlockStruct *mb = f->multiblock;

	if ( mb->field_edges & recv_only_edge ) {
		ret = MPI_Recv (
				recv_buffer,
				bufsize,
				MPI_BYTE,
				f->myproc - proc_spacing,
				f->myproc, // use destination proc as tag
				f->comm,
				&status );
	} else if ( mb->field_edges & send_only_edge ) {
		ret = MPI_Send (
				send_buffer,
				bufsize,
				MPI_BYTE,
				f->myproc + proc_spacing,
				f->myproc + proc_spacing,
				f->comm );
	} else {
		ret = MPI_Sendrecv (
				send_buffer,
				bufsize,
				MPI_BYTE,
				f->myproc + proc_spacing,
				f->myproc + proc_spacing,
				recv_buffer,
				bufsize,
				MPI_BYTE,
				f->myproc - proc_spacing,
				f->myproc,
				f->comm,
				&status );
	}

	if ( ret != MPI_SUCCESS ) return HANDLE_MPI_SENDRECV_ERR;

	return H5PART_SUCCESS;
}

static void
_halo_buffer_pack (
	const char *data,		/*!< IN: local block */
	char *buffer,			/*!< OUT: packed buffer */
	h5part_int64_t offset,
	h5part_int64_t count,
	h5part_int64_t blocklen,
	h5part_int64_t stride
	) {

	h5part_int64_t i;
	h5part_int64_t dst_offset, src_offset;

        dst_offset = 0;
	src_offset = offset;

	for (i=0; i<count; i++)
	{
		memcpy (	buffer + dst_offset,
				data + src_offset,
				blocklen );
		dst_offset += blocklen;
		src_offset += stride;
	}
}

static void
_halo_buffer_unpack (
	char *data,			/*!< OUT: local block */
	const char *buffer,		/*!< IN: packed buffer */
	h5part_int64_t offset,
	h5part_int64_t count,
	h5part_int64_t blocklen,
	h5part_int64_t stride
	) {

	h5part_int64_t i;
	h5part_int64_t dst_offset, src_offset;

	dst_offset = offset;
        src_offset = 0;

	for (i=0; i<count; i++)
	{
		memcpy (	data + dst_offset,
				buffer + src_offset,
				blocklen );
		dst_offset += stride;
		src_offset += blocklen;
	}
}

static h5part_int64_t
_halo_exchange_buffers (
	const H5PartFile *f,		/*!< IN: file handle */
	char *data,			/*!< IN/OUT: local buffer */
	h5part_int64_t count,
	h5part_int64_t blocklen,
	h5part_int64_t stride,
	int proc_spacing,
	const char *edges
	) {

	char *send_buffer;
	char *recv_buffer;
	int ibufsize;
	h5part_int64_t bufsize;
	h5part_int64_t herr;

	struct H5MultiBlockStruct *mb = f->multiblock;

	if ( f->myproc == 0 ) _H5Part_print_info (
			"Using halo exchange method _halo_exchange_buffers");

	bufsize = count * blocklen;
	ibufsize = (int)bufsize;
	if ( (h5part_int64_t)ibufsize != bufsize ) return HANDLE_MPI_INT64_ERR;

	send_buffer = (char*) malloc ( bufsize );
	if ( send_buffer == NULL ) return HANDLE_H5PART_NOMEM_ERR;

	recv_buffer = (char*) malloc ( bufsize );
	if ( recv_buffer == NULL ) return HANDLE_H5PART_NOMEM_ERR;

	/* forward */
	_halo_buffer_pack ( data, send_buffer, stride - 2*blocklen,
						count, blocklen, stride );

	herr = _halo_exchange_buffer ( f, send_buffer, recv_buffer,
				ibufsize, edges[0], edges[1], proc_spacing);
	if ( herr != H5PART_SUCCESS ) return herr;

	if ( ! (mb->field_edges & edges[0]) ) {
		_halo_buffer_unpack ( data, recv_buffer, 0,
						count, blocklen, stride );
	}

	/* backward */
	_halo_buffer_pack ( data, send_buffer, blocklen,
						count, blocklen, stride );

	herr = _halo_exchange_buffer ( f, send_buffer, recv_buffer,
				ibufsize, edges[1], edges[0], -proc_spacing);
	if ( herr != H5PART_SUCCESS ) return herr;

	if ( ! (mb->field_edges & edges[1]) ) {
		_halo_buffer_unpack ( data, recv_buffer, stride - blocklen,
						count, blocklen, stride );
	}

	free ( send_buffer );
	free ( recv_buffer );

	return H5PART_SUCCESS;	
}

static h5part_int64_t
_halo_exchange_vectors (
	const H5PartFile *f,
	char *data,
	h5part_int64_t count,
	h5part_int64_t blocklen,
	h5part_int64_t stride,
	int proc_spacing,
	const char *edges
	) {

	int ret;
	int icount, iblocklen, istride;
	h5part_int64_t herr;
	MPI_Datatype halo_vector;

	if ( f->myproc == 0 ) _H5Part_print_info (
			"Using halo exchange method _halo_exchange_vectors");

	icount = (int)count;
	if ( (h5part_int64_t)icount != count ) return HANDLE_MPI_INT64_ERR;

	iblocklen = (int)blocklen;
	if ( (h5part_int64_t)iblocklen != blocklen ) return HANDLE_MPI_INT64_ERR;

	istride = (int)stride;
	if ( (h5part_int64_t)istride != stride ) return HANDLE_MPI_INT64_ERR;

	ret = MPI_Type_vector (	icount, iblocklen, istride,
						MPI_BYTE, &halo_vector);
	if (ret != MPI_SUCCESS) return HANDLE_MPI_TYPE_ERR;

	ret = MPI_Type_commit ( &halo_vector );
	if (ret != MPI_SUCCESS) return HANDLE_MPI_TYPE_ERR;

	/* forward */
	herr = _halo_exchange_vector ( f, data, &halo_vector,
					stride - 2*blocklen,
					0,
					edges[0], edges[1], proc_spacing );
	if (herr != H5PART_SUCCESS) return herr;

	/* backward */
	herr = _halo_exchange_vector ( f, data, &halo_vector,
					blocklen,
					stride - blocklen,
					edges[1], edges[0], -proc_spacing );
	if (herr != H5PART_SUCCESS) return herr;

	ret = MPI_Type_free ( &halo_vector );
	if (ret != MPI_SUCCESS) return HANDLE_MPI_TYPE_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_static

  \internal

  Exchanges halo regions among neighboring blocks using MPI.

  \return	H5PART_SUCCESS or error code
*/
static h5part_int64_t
_halo_exchange (
	const H5PartFile *f,		/*!< IN: file handle */
	char *data,			/*!< IN/OUT: local buffer */
	hid_t type			/*!< IN: HDF5 datatype of buffer */
	) {

	size_t typesize;
	char edges[2];
	int proc_spacing;
	h5part_int64_t count, blocklen, stride;
	h5part_int64_t herr;

	struct H5MultiBlockStruct *mb = f->multiblock;

	typesize = H5Tget_size (type);

	/* xy-slab
	 * the best case: the entire x-dimension by y-dimension by z halo
	 * radius slab can be transferred contiguosly */
	if ( mb->decomp[0] > 1 ) {
		count    = 1;
		blocklen =   mb->halo_dims[2] * mb->halo_dims[1]
		           * mb->halo_radii[0] * typesize;
    		stride   =   mb->halo_dims[2] * mb->halo_dims[1] 
		           * mb->halo_dims[0]  * typesize;
		/* jump by an xy-slab to get the next z block */
		proc_spacing = mb->decomp[2] * mb->decomp[1];
		edges[0] = H5MB_EDGE_Z0;
		edges[1] = H5MB_EDGE_Z1;
		herr = HALO_EXCHANGE_METHOD ( f, data,
						count, blocklen, stride,
						proc_spacing, edges);
		if ( herr != H5PART_SUCCESS ) return herr;
	}
	
	/* xz-slab
	 * the second best case: a rectangle of x-dimension by y halo radius
	 * can be transferred contiguously */
	if ( mb->decomp[1] > 1 ) {
		count    = mb->halo_dims[0];
		blocklen = mb->halo_dims[2] * mb->halo_radii[1] * typesize;
		stride   = mb->halo_dims[2] * mb->halo_dims[1]  * typesize;
		/* jump by an x-row to get the next y block */
		proc_spacing = mb->decomp[2];
		edges[0] = H5MB_EDGE_Y0;
		edges[1] = H5MB_EDGE_Y1;
		herr = HALO_EXCHANGE_METHOD ( f, data,
						count, blocklen, stride,
						proc_spacing, edges);
		if ( herr != H5PART_SUCCESS ) return herr;
	}

	/* yz-slab
	 * the worst case: only small rows with length = x halo radius */
	if ( mb->decomp[2] > 1 ) {
		count    = mb->halo_dims[1]  * mb->halo_dims[0];
		blocklen = mb->halo_radii[2] * typesize;
		stride   = mb->halo_dims[2]  * typesize;
		/* blocks are contiguous in the x direction */
		proc_spacing = 1;
		edges[0] = H5MB_EDGE_X0;
		edges[1] = H5MB_EDGE_X1;
		herr = HALO_EXCHANGE_METHOD ( f, data,
						count, blocklen, stride,
						proc_spacing, edges);
		if ( herr != H5PART_SUCCESS ) return herr;
	}

	return H5PART_SUCCESS;
}

/*******************************************************************************
* Private functions
*******************************************************************************/

/*!
  \ingroup h5multiblock_private

  \internal

  Initialize H5MultiBlock internal structure.

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5MultiBlock_init (
	H5PartFile *f			/*!< IN: file handle */
	) {

	struct H5MultiBlockStruct *mb;

	BLOCK_INIT( f );

	if (f->multiblock != NULL) return H5PART_SUCCESS;

	f->multiblock =
		(struct H5MultiBlockStruct*) malloc( sizeof (*f->multiblock) );
	if ( f->multiblock == NULL ) {
		return HANDLE_H5PART_NOMEM_ERR;
	}
	mb = f->multiblock;

	mb->halo_radii[0] = 0;
	mb->halo_radii[1] = 0;
	mb->halo_radii[2] = 0;

	mb->field_edges = 0;
	mb->halo = 0;
	mb->read = 0;
	mb->have_decomp = 0;
        mb->proc_shift = 0;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_private

  \internal

  Free H5MultiBlock internal struct;

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5MultiBlock_close (
	H5PartFile *f		/*!< IN: file handle */
	) {

	free ( f->multiblock );
	f->multiblock = NULL;
	f->close_multiblock = NULL;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_private

  \internal

  Read a multiblock field \c name into the buffer starting at \c data
  using the current time-step and the block decomposition and dimensions
  defined in the file.

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5MultiBlock_read_data (
	H5PartFile *f,			/*!< IN: file handle */
	const char *field_name,		/*!< IN: Name of field */
	char **data,			/*!< OUT: ptr to read buffer ptr*/
	hid_t type			/*!< IN: HDF5 datatype of buffer */
	) {

	MULTIBLOCK_INIT( f );

	hid_t dataset_id;
	hid_t dataspace_id;
	int rank;
	h5part_int64_t herr;
	
	struct H5BlockStruct *b = f->block;
	struct H5MultiBlockStruct *mb = f->multiblock;

	char * const fname = _H5Part_get_funcname();

	herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	dataset_id = H5Dopen ( b->field_group_id, "0"
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( "0" );

	/* read block dimensions from field attribute */
	herr = _H5Part_read_attrib (
		b->field_group_id,
		H5MULTIBLOCK_ATTR_NAME,
		mb->block_dims );
	if ( herr < 0 ) return herr;

	if ( f->myproc == 0 ) {
		_H5Part_print_info ("Block dimensions: (%ld,%ld,%ld)", 
				mb->block_dims[2],
				mb->block_dims[1],
				mb->block_dims[0] );
	}

	_H5Part_print_debug ("PROC[%d]: Block dimensions: (%ld,%ld,%ld)",
			f->myproc,
			mb->block_dims[2],
			mb->block_dims[1],
			mb->block_dims[0] );

	mb->halo_dims[0] = mb->block_dims[0] + 2*mb->halo_radii[0];
	mb->halo_dims[1] = mb->block_dims[1] + 2*mb->halo_radii[1];
	mb->halo_dims[2] = mb->block_dims[2] + 2*mb->halo_radii[2];

 	dataspace_id = H5Dget_space ( dataset_id );
	if ( dataspace_id < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

	rank = H5Sget_simple_extent_dims (
			dataspace_id,
			(hsize_t*)mb->field_dims,
			NULL );
	if ( rank < 0 )  return HANDLE_H5S_GET_SIMPLE_EXTENT_DIMS_ERR;
	if ( rank != 3 ) return HANDLE_H5PART_DATASET_RANK_ERR ( rank, 3 );

#ifdef H5MB_TIMING
	_H5Part_print_info ( "timing[%d]: alloc: %.7f", f->myproc, MPI_Wtime() );
#endif
	herr = _alloc_block ( f, data, type );
	if ( herr < 0 ) return herr;
#ifdef H5MB_TIMING
	_H5Part_print_info ( "timing[%d]: read: %.7f", f->myproc, MPI_Wtime() );
#endif

	herr = _get_decomp_and_offsets ( f );
	if ( herr < 0 ) return herr;

	mb->have_decomp = 1;

	/* shortcut: use the H5Block layout; indices have to be inverted, though, since
	 * the API exposes Fortran ordering and all internal data uses C ordering */
	herr = H5BlockDefine3DFieldLayout ( f,
		mb->offsets[2], mb->offsets[2] + mb->block_dims[2] - 1,
		mb->offsets[1], mb->offsets[1] + mb->block_dims[1] - 1,
		mb->offsets[0], mb->offsets[0] + mb->block_dims[0] - 1);
	if ( herr < 0 ) return herr;

	_H5Part_set_funcname ( fname );

	herr = _H5Block_select_hyperslab_for_reading ( f, dataset_id );
	if ( herr < 0 ) return herr;

	herr = _H5Part_start_throttle( f );
	if (herr < 0) return herr;

	herr = H5Dread ( 
		dataset_id,
		type,
		f->block->memshape,
		f->block->diskshape,
		f->xfer_prop,
		*data );
	if ( herr < 0 ) return HANDLE_H5D_READ_ERR ( field_name, f->timestep );

	herr = _H5Part_end_throttle( f );
	if (herr < 0) return herr;

	herr = H5Dclose ( dataset_id );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	if ( mb->halo ) {
#ifdef H5MB_TIMING
		_H5Part_print_info ( "timing[%d]: pad: %.7f", f->myproc, MPI_Wtime() );
#endif
		_pad_block ( f, *data, type );
#ifdef H5MB_TIMING
		_H5Part_print_info ( "timing[%d]: zero: %.7f", f->myproc, MPI_Wtime() );
#endif
		_zero_block (f, *data, type );
#ifdef H5MB_TIMING
		_H5Part_print_info ( "timing[%d]: halo: %.7f", f->myproc, MPI_Wtime() );
#endif
		herr = _halo_exchange ( f, *data, type );
		if ( herr != H5PART_SUCCESS ) return herr;
#ifdef H5MB_TIMING
		_H5Part_print_info ( "timing[%d]: end: %.7f", f->myproc, MPI_Wtime() );
#endif
	}

	mb->read = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_private

  \internal

  Write a multiblock field \c name from the buffer starting at \c data
  to the current time-step using the defined block decomposition and dimensions.

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5MultiBlock_write_data (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,       /*!< IN: name of dataset to write */
	const void* data,	/*!< IN: data buffer */
	hid_t type		/*!< IN: HDF5 datatype */
	) {

	MULTIBLOCK_INIT( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );
	CHECK_DECOMP ( f );

	hid_t dataset;
	h5part_int64_t herr;

	struct H5BlockStruct *b = f->block;
	struct H5MultiBlockStruct *mb = f->multiblock;

	char * const fname = _H5Part_get_funcname();

	/* shortcut: use the H5Block layout; indices have to be inverted, though, since
	 * the API exposes Fortran ordering and all internal data uses C ordering */
	herr = H5BlockDefine3DFieldLayout ( f,
		mb->offsets[2], mb->offsets[2] + mb->block_dims[2] - 1,
		mb->offsets[1], mb->offsets[1] + mb->block_dims[1] - 1,
		mb->offsets[0], mb->offsets[0] + mb->block_dims[0] - 1);
	if ( herr < 0 ) return herr;

	herr = H5BlockDefine3DChunkDims( f,
			mb->block_dims[2],
			mb->block_dims[1],
			mb->block_dims[0]);
	if ( herr < 0 ) return herr;

	_H5Part_set_funcname ( fname );

	herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	dataset = H5Dcreate (
		b->field_group_id,
		"0",
		type,
		b->shape, 
#ifndef H5_USE_16_API
		H5P_DEFAULT,
		b->create_prop,
		H5P_DEFAULT
#else
		b->create_prop
#endif
		);
	if ( dataset < 0 ) return HANDLE_H5D_CREATE_ERR ( name, f->timestep );

	herr = _H5Part_start_throttle( f );
	if (herr < 0) return herr;

	herr = H5Dwrite ( 
		dataset,
		type,
		b->memshape,
		b->diskshape,
		f->xfer_prop,
		data );
	if ( herr < 0 ) return HANDLE_H5D_WRITE_ERR ( name, f->timestep );

	herr = _H5Part_end_throttle( f );
	if (herr < 0) return herr;

	herr = H5Dclose ( dataset );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	/* write out the block dimensions to a special field attribute */
	herr = _H5Part_write_attrib (
		f->block->field_group_id,
		H5MULTIBLOCK_ATTR_NAME,
		H5T_NATIVE_INT64,
		mb->block_dims,
		3 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}


/*******************************************************************************
* Public API
*******************************************************************************/

/*!
  \ingroup h5multiblock_model

  Define the radius for halo exchanges between the blocks. Blocks on the edges
  of the field will be padded with zero values out to the radius.

  \return \c H5PART_SUCCESS on success
*/
h5part_int64_t
H5MultiBlock3dDefineRadius (
	H5PartFile *f,		/* IN: file handle */
	const h5part_int64_t r	/* IN: radius for i, j and k directions*/
	) {

	SET_FNAME ( "H5MultiBlock3dDefineRadius" );
	MULTIBLOCK_INIT( f );

	struct H5MultiBlockStruct *mb = f->multiblock;

	mb->halo_radii[0] = 
	mb->halo_radii[1] =
	mb->halo_radii[2] = r;

	mb->halo = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Define the radii for halo exchanges between the blocks. Blocks on the edges
  of the field will be padded with zero values out to the radius.

  Different radii can be set for each direction.

  \return \c H5PART_SUCCESS on success
*/
h5part_int64_t
H5MultiBlock3dDefineRadii (
	H5PartFile *f,			/* IN: file handle */
	const h5part_int64_t ri,	/* IN: radius for i direction */
	const h5part_int64_t rj,	/* IN: radius for j direction */
	const h5part_int64_t rk		/* IN: radius for k direction */
	) {

	SET_FNAME ( "H5MultiBlock3dDefineRadii" );
	MULTIBLOCK_INIT( f );

	struct H5MultiBlockStruct *mb = f->multiblock;

	mb->halo_radii[0] = rk;
	mb->halo_radii[1] = rj;
	mb->halo_radii[2] = ri;

	mb->halo = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Define the field and block dimensions for writing.

  \return \c H5PART_SUCCESS on success
  	  \c H5PART_ERR_INVAL if block dims do not divide field dims
*/
h5part_int64_t
H5MultiBlock3dDefineDims (
	H5PartFile *f,				/* IN: file handle */
	const h5part_int64_t *field_dims,	/* IN: field dimensions */
	const h5part_int64_t *block_dims	/* IN: block dimensions */
	) {

	SET_FNAME ( "H5MultiBlock3dDefineDims" );
	MULTIBLOCK_INIT ( f );

	struct H5MultiBlockStruct *mb = f->multiblock;

	mb->field_dims[0] = field_dims[2];
	mb->field_dims[1] = field_dims[1];
	mb->field_dims[2] = field_dims[0];

	mb->block_dims[0] = block_dims[2];
	mb->block_dims[1] = block_dims[1];
	mb->block_dims[2] = block_dims[0];

	h5part_int64_t herr = _get_decomp_and_offsets ( f );
	if ( herr < 0 ) return H5PART_ERR_INVAL;

	mb->have_decomp = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Returns the field dimensions of the last field that was read.

  \return \c H5PART_SUCCESS on success<br>
  	  \c H5PART_ERR_INVAL if no field has been read yet

*/
h5part_int64_t
H5MultiBlock3dGetFieldDims(
	H5PartFile *f,
	h5part_int64_t *dims
	) {

	SET_FNAME ( "H5MultiBlock3dGetFieldDims" );
	MULTIBLOCK_INIT ( f );

	struct H5MultiBlockStruct *mb = f->multiblock;

	if ( ! mb->read ) return H5PART_ERR_INVAL;

	dims[0] = mb->field_dims[2];
	dims[1] = mb->field_dims[1];
	dims[2] = mb->field_dims[0];

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Returns the block dimensions of the last field that was read.

  \return \c H5PART_SUCCESS on success<br>
  	  \c H5PART_ERR_INVAL if no field has been read yet

*/
h5part_int64_t
H5MultiBlock3dGetBlockDims(
	H5PartFile *f,	  	/* IN: file handle */
	const char *field_name, /* IN: field name */
	h5part_int64_t *dims    /* OUT: block dimensions */
	) {

	SET_FNAME ( "H5MultiBlock3dGetBlockDims" );
	MULTIBLOCK_INIT ( f );

	struct H5MultiBlockStruct *mb = f->multiblock;

	if ( ! mb->read ) return H5PART_ERR_INVAL;

	dims[0] = mb->block_dims[2];
	dims[1] = mb->block_dims[1];
	dims[2] = mb->block_dims[0];

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Return the offsets for the block belonging to processor \c proc.

  \return \c H5PART_SUCCESS on success<br>
	  \c H5PART_ERR_INVAL if proc is invalid
*/
h5part_int64_t
H5MultiBlock3dGetOffsetsOfProc (
	H5PartFile *f,			/* IN: file handle */
	const h5part_int64_t proc,	/* IN: processor number */
	h5part_int64_t *offsets		/* OUT: 3d array of offsets */
	) { 

	SET_FNAME ( "H5MultiBlock3dGetOffsetsOfProc" );
	MULTIBLOCK_INIT ( f );

	if ( ( proc < 0 ) || ( proc >= f->nprocs ) )
		return H5PART_ERR_INVAL;

	struct H5MultiBlockStruct *mb = f->multiblock;

	offsets[0] = mb->offsets[2];
	offsets[1] = mb->offsets[1];
	offsets[2] = mb->offsets[0];

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Finds a 3D block decomposition for an arbitrary number of processors
  \c nprocs.

  \return \c H5PART_SUCCESS on success<br>
	  \c H5PART_ERR_INVAL if the decomp doesn't have \c nprocs blocks
*/
h5part_int64_t
H5MultiBlock3dCalculateDecomp (
	const int nprocs,	/*!< IN: number of processors/blocks */
	h5part_int64_t *decomp	/*!< OUT: 3D block decomposition */
	) {

	decomp[0] = _nth_root_int_divisor (nprocs, 3);
	decomp[1] = _nth_root_int_divisor (nprocs / decomp[0], 2);
	decomp[2] = nprocs / decomp[0] / decomp[1];

	if (decomp[0] * decomp[1] * decomp[2] != nprocs) {
		return H5PART_ERR_INVAL;
	}

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Frees a \c block that was allocated during a read.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5MultiBlockFree (
	void *block	/*!<  IN: block that was allocated during a read */
	) {

	free (block);

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_model

  Shifts the assignment of procs to blocks within the field.

  This is useful mainly for I/O benchmarking, in order to defeat the write
  cache when reading in data that was just written out.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5MultiBlockShiftProcs (
	H5PartFile *f,			/* IN: file handle */
	const int shift			/* IN: shift amount (non-negative) */
	) {

	SET_FNAME ( "H5MultiBlockShiftProcs" );
	MULTIBLOCK_INIT ( f );

	if ( shift < 0 ) return H5PART_ERR_INVAL;

	struct H5MultiBlockStruct *mb = f->multiblock;

	mb->proc_shift = shift;

	return H5PART_SUCCESS;
}

#endif // PARALLEL_IO

