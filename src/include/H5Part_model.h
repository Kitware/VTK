/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef H5PART_MODEL
#define H5PART_MODEL

#include "h5core/h5_log.h"
#include "h5core/h5u_model.h"

/**
   \addtogroup h5part_model
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
   !   _                   _          
   !  (_)_ __   __ _ _   _(_)_ __ ___ 
   !  | | '_ \ / _` | | | | | '__/ _ \
   !  | | | | | (_| | |_| | | | |  __/
   !  |_|_| |_|\__, |\__,_|_|_|  \___|
   !              |_|
   !
*/

/**
  Get the number of datasets that are stored at the current
  step/iteration.

  \return   number of datasets in current step/iteration
  \return   \c H5_FAILURE on error
*/
static inline h5_ssize_t
H5PartGetNumDatasets (
	const h5_file_t f		///< [in]  file handle
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_get_num_datasets(f));
}

/**
  Query the name of a dataset given by it's index in the current
  step/iteration.

  If the number of datasets is \c n, the range of \c _index is \c 0 to \c n-1.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartGetDatasetName (
	const h5_file_t f,           	///< [in]  file handle
	const h5_id_t idx,      	///< [in]  index of the dataset
	char* const name,             	///< [out] name of dataset
	const h5_size_t len     	///< [in]  size of buffer \c name
	) {
	H5_API_ENTER (h5_err_t, 
		       "f=%p, "
		       "idx=%lld, "
		       "name='%p', len=%llu, ",
                      (h5_file_p)f,
		       (long long)idx,
		       name, (unsigned long long)len);
	H5_API_RETURN (
		h5u_get_dataset_info_by_idx(
			f,
			idx,
			name, len,
			NULL, NULL));
}

/**
  Gets the name, type and number of elements of a dataset based on its
  index in the current step/iteration.

  Type is one of the following values:

  - \c H5_FLOAT64_T (for \c h5_float64_t)
  - \c H5_FLOAT32_T (for \c h5_float32_t)
  - \c H5_INT64_T (for \c h5_int64_t)
  - \c H5_INT32_T (for \c h5_int32_t)

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5PartGetNumDatasets()
  \see H5PartGetDatasetInfoByName()
*/
static inline h5_err_t
H5PartGetDatasetInfo (
	const h5_file_t f,           	///< [in]  file handle
	const h5_id_t idx,      	///< [in]  index of the dataset
	char* const name,             	///< [out] name of dataset
	const h5_size_t len_name,       ///< [in]  size of buffer \c name
	h5_int64_t* type,       	///< [out] type of data in dataset
	h5_size_t* nelems        	///< [out] number of elements
	) {
	H5_API_ENTER (h5_int64_t, 
		      "f=%p, "
		      "idx=%lld, "
		      "name='%p', len_name=%llu, "
		      "type=%p, nelems=%p",
		      (h5_file_p)f,
		      (long long)idx,
		      name, (long long unsigned)len_name,
		      type, nelems);
	H5_API_RETURN (
		h5u_get_dataset_info_by_idx (
			f,
			idx,
			name, len_name,
			type, nelems));
}
/**
  Determines whether a dataset with given name exists in current
  step/iteration.

  \return      true (value \c >0) if step/iteration exists
  \return      false (\c 0) if step/iteration does not exist
  \return      \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartHasDataset (
	const h5_file_t f,           	///< [in]  file handle
	const char* const name         	///< [in]  name of dataset
	) {
	H5_API_ENTER (h5_int64_t, 
		      "f=%p, name='%s'",
		      (h5_file_p)f, name);
	H5_API_RETURN (h5u_has_dataset (f, name));
}

/**
  Gets the type and number of elements of a dataset based on its
  name in the current step/iteration.

  Type is one of the following values:

  - \c H5_FLOAT64_T (for \c h5_float64_t)
  - \c H5_FLOAT32_T (for \c h5_float32_t)
  - \c H5_INT64_T (for \c h5_int64_t)
  - \c H5_INT32_T (for \c h5_int32_t)

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5PartHasDataset()
  \see H5PartGetDatasetInfo()
*/
static inline h5_err_t
H5PartGetDatasetInfoByName (
	const h5_file_t f,           	///< [in]  file handle
	const char* const name,         ///< [in]  name of dataset
	h5_int64_t* type,       	///< [out] type of data in dataset
	h5_size_t* nelems        	///< [out] number of elements
	) {
	H5_API_ENTER (h5_int64_t, 
		      "f=%p, "
		      "name='%s', "
		      "type=%p, nelems=%p",
		      (h5_file_p)f,
		      name,
		      type, nelems);
	H5_API_RETURN (
		h5u_get_dataset_info_by_name (
			f,
			name,
			type, nelems));
}

/**
  Set the number of items/particles for the current step/iteration.
  After you call this subroutine, all subsequent 
  operations will assume this number of particles will be written.

  For the parallel library, the \c nparticles value is the number of
  particles that the \e individual task will write. You can use
  a different value on different tasks.
  This function uses an \c MPI_Allgather
  call to aggregate each tasks number of particles and determine
  the appropiate offsets. Because of the use of this MPI collective,
  it is advisable to call this function as
  few times as possible when running at large concurrency.

  This function assumes that your particles' data fields are in stored in
  contiguous 1D arrays.
  For instance, the fields \e x and \e y for your particles are stored
  in separate arrays \c x[] and \c y[].
  
  If instead you store your particles as tuples, so that the values
  are arranged \f$ x_1,y_1,x_2,y_2\f$... than you need to setup striding
  (in this case with value 2) using \ref H5PartSetNumParticlesStrided.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
 */
static inline h5_err_t
H5PartSetNumItems (
	const h5_file_t f,		///< [in]  file handle.
	h5_size_t num_items               ///< [in]  Number of elements.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, num_items=%llu",
		      (h5_file_p)f, (long long unsigned)num_items);
	h5_size_t stride = 1;
	H5_API_RETURN (h5u_set_num_items (f, num_items, stride));
}

/**
  \see H5PartSetNumItems()
*/
static inline h5_err_t
H5PartSetNumParticles (
	const h5_file_t f,		///< [in]  file handle.
	h5_size_t nparticles            ///< [in]  Number of particles.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, nparticles=%llu",
		      (h5_file_p)f, (long long unsigned)nparticles);
	h5_size_t stride = 1;
	H5_API_RETURN (h5u_set_num_items (f, nparticles, stride));
}

/**
  This function returns the number of particles in this processor's view,
  if a view has been set.

  If not, it returns the total number of particles across all processors
  from the last \ref H5PartSetNumParticles() call.

  If you have neither set the number of particles
  nor set a view, then this returns the total number of
  particles in the first data set of the current step/iteration.
  Note that H5Part assumes that all data sets within a given step/iteration
  have the same number of particles (although the number particles can
  vary across steps/iteration).
  
  If none of these conditions are met, an error is thrown.

  \return	number of elements in datasets in current step/iteration.
  \return       \c H5_FAILURE on error.
 */
	static inline h5_ssize_t
H5PartGetNumItems (
	const h5_file_t f		///< [in]  file handle.
	) {
	H5_API_ENTER (h5_ssize_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_get_num_items (f));
}

/**
  \see H5PartGetNumItems()
*/
static inline h5_ssize_t
H5PartGetNumParticles (
	const h5_file_t f		///< [in]  file handle.
	) {
	H5_API_ENTER (h5_ssize_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_get_num_items (f));
}


/**
  Set the number of particles for the current step/iteration.
  After you call this subroutine, all subsequent 
  operations will assume this number of particles will be written.

  For the parallel library, the \c nparticles value is the number of
  particles that the \e individual task will write. You can use
  a different value on different tasks.
  This function uses an \c MPI_Allgather
  call to aggregate each tasks number of particles and determine
  the appropiate offsets. Because of the use of this MPI collective,
  it is advisable to call this function as
  few times as possible when running at large concurrency.

  This function assumes that your particles' data fields are
  stored tuples. For instance, the fields \e x and \e y of your
  particles are arranged \f$x_1,y_1,x_2,y_2\f$... in a single data
  array. In this example, the stride value would be 2.
  
  If you instead have a separate array for each fields,
  such as \c x[] and \c y[],
  use \ref H5PartSetNumParticles().

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartSetNumParticlesStrided (
	const h5_file_t f,              ///< [in]  file handle.
	h5_size_t num_items,            ///< [in]  number of elements.
	h5_size_t stride                ///< [in]  stride value (e.g. number
                                        ///<       of fields in the particle array).
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, num_items=%llu, stride=%llu",
		      (h5_file_p)f, (long long unsigned)num_items,
		      (long long unsigned)stride);
	H5_API_RETURN (h5u_set_num_items (f, num_items, stride));
}

/**
  Define the chunk \c size and enables chunking in the underlying
  HDF5 layer.

  Note that this policy wastes some disk space, but can improve read and write
  performance depending on the access pattern.

  On parallel filesystems that are sensitive to write alignment (e.g. lustre)
  it is recommended to set a reasonable chunk size when using the MPI-POSIX
  or MPI-IO independent VFDs (see \ref H5OpenFile()).

  For more details about chunking, please read the HDF5 documentation.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartSetChunkSize (
	const h5_file_t f,              ///< [in]  file handle.
	h5_size_t size                  ///< [in]  chunk size.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, size=%llu",
                      (h5_file_p)f, (long long unsigned)size);
	H5_API_RETURN (h5u_set_chunk (f, size));
}


/**
  Reset the view.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartResetView (
 	const h5_file_t f		///< [in]  file handle.
	) {
	H5_API_ENTER (h5_ssize_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_reset_view (f));
}

/**
  Check whether a view has been set, either automatically with
  \ref H5PartSetNumParticles() or manually with \ref H5PartSetView
  or \ref H5PartSetViewIndices.

  \return      true (value \c >0) if step/iteration exists
  \return      false (\c 0) if step/iteration does not exist
  \return      \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartHasView (
 	const h5_file_t f		///< [in]  file handle.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_has_view (f));
}

/**
  For parallel I/O or for subsetting operations on the datafile,
  this function allows you to define a subset of the total
  particle dataset to operate on.
  The concept of "view" works for both serial
  and for parallel I/O.  The "view" will remain in effect until a new view
  is set, or the number of particles in a dataset changes, or the view is
  "unset" by calling \c H5PartSetView(file,-1,-1);

  Before you set a view, \ref H5PartGetNumItems will return the
  total number of particles in the current step/iteration (even for
  the parallel reads).  However, after you set a view, it will
  return the number of particles contained in the view.

  The range is \e inclusive: the end value is the last index of the
  data.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartSetView (
	const h5_file_t f,		///< [in]  file handle.
	h5_int64_t start,               ///< [in]  start particle.
	h5_int64_t end                  ///< [in]  end particle.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, start=%lld, end=%lld",
		      (h5_file_p)f, (long long)start, (long long)end);
	H5_API_RETURN (h5u_set_view (f, start, end));
}

/**
  For parallel I/O or for subsetting operations on the datafile,
  this function allows you to define a subset of the total
  dataset to operate on by specifying a list of indices.
  The concept of "view" works for both serial
  and for parallel I/O.  The "view" will remain in effect until a new view
  is set, or the number of particles in a dataset changes, or the view is
  "unset" by calling \c H5PartSetViewIndices(NULL,0);

  When you perform a read or write on a view consisting of indices, it
  is assumed that your buffer is \b unpacked, meaning that there is room
  for all the intermediate values (which will not be touched by the read
  or write).

  Before you set a view, the \c H5PartGetNumItems() will return the
  total number of particles in the current step/iteration (even for
  the parallel reads).  However, after you set a view, it will return
  the number of particles contained in the view.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5PartSetViewIndices (
	const h5_file_t f,		///< [in]  file handle.
	const h5_size_t* indices,	///< [in]  list of indices.
	h5_size_t nelems		///< [in]  size of list.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, indices=%p, nelems=%llu",
		      (h5_file_p)f, indices, (long long unsigned)nelems);
	H5_API_RETURN (h5u_set_view_indices (f, indices, nelems));
}

/**
   Allows you to query the current view. Start and End
   will be \c -1 if there is no current view established.
   Use \c H5PartHasView() to see if the view is smaller than the
   total dataset.

   \return	number of elements in the view
   \return      \c H5_FAILURE on error
*/
static inline h5_int64_t
H5PartGetView (
	const h5_file_t f,		///< [in]  file handle.
	h5_int64_t* start,		///< [out] start particle.
	h5_int64_t* end			///< [out] end particle.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, start=%p, end=%p",
		      (h5_file_p)f, start, end);
	H5_API_RETURN (h5u_get_view (f, start, end));
}

/**
  If it is too tedious to manually set the start and end coordinates
  for a view, \c H5SetCanonicalView() will automatically select an
  appropriate domain decomposition of the data arrays for the degree
  of parallelism and set the "view" accordingly.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_int64_t
H5PartSetCanonicalView (
	const h5_file_t f		///< [in]  file handle.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5u_set_canonical_view (f));
}

#ifdef __cplusplus
}
#endif

///< @}
#endif
