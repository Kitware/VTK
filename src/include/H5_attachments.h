/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_ATTACHMENTS_H
#define __H5_ATTACHMENTS_H

#include "h5core/h5_log.h"
#include "h5core/h5_model.h"
#include "h5core/h5_attachments.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   \addtogroup h5_attach
   @{
*/

/**
  Return number of attached files.

  \return   number of attachments.
  \return   \c H5_FAILURE on error.
*/
static inline h5_ssize_t
H5GetNumAttachments (
	const h5_file_t f               ///< [in]  file handle.
	) {
	H5_API_ENTER (h5_ssize_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_num_attachments (f));
}

/**
  Get name and size of attachment given by index \c idx. Return the file name
  in \c fname and the original size in \c fsize.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetAttachmentInfoByIdx (      	
	const h5_file_t f,              ///< [in]  file handle.
	const h5_size_t idx,		///< [in]  index of attachment to be queried.
	char* const fname,		///< [out] original file name.
	h5_size_t len_fname,		///< [in]  max length of file name.
	h5_size_t* const fsize		///< [out] size of original file.
	) {
	H5_API_ENTER (h5_err_t,
		      "idx=%llu, fname=%p, len_fname=%llu, fsize=%p",
		      (long long unsigned)idx,
		      fname, (long long unsigned)len_fname,
		      fsize);
	H5_API_RETURN (h5_get_attachment_info_by_idx (
			       f, idx, fname, len_fname, fsize));
}

/**
  Query whether a particular attachment exists in the file.

  \return      true (value \c >0) if attachment exists
  \return      false (\c 0) if attachment does not exist
  \return      \c H5_FAILURE on error
*/
static inline h5_err_t
H5HasAttachment (
	const h5_file_t f,              ///< [in]  file handle.
	const char* const fname		///< [in]  original file name.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s'",
                      (h5_file_p)f, fname);
	H5_API_RETURN (h5_has_attachment (f, fname));
}

/**
  Get size of attached file with name \c fname.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetAttachmentInfoByName (
	const h5_file_t f,              ///< [in]  file handle.
	char* const fname,		///< [in]  original file name.
	h5_size_t* const fsize		///< [out] size of original file.
	) {
	H5_API_ENTER (h5_err_t, "fname='%s', fsize=%p", fname, fsize);
	H5_API_RETURN (h5_get_attachment_info_by_name (
			       f, fname, fsize));
}

/**
  Attach file \c fname.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5AddAttachment (
	const h5_file_t f,		///< [in]  file handle.
	const char* fname		///< [in]  name of file to attach.
	) {
	H5_API_ENTER (h5_err_t, "fname='%s'", fname);
	H5_API_RETURN (h5_add_attachment (f, fname));
}

/**
  Get attachment \c fname from H5hut file and write it to disk in
  the current working directory.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetAttachment (
	const h5_file_t f,              ///< [in]  file handle.
	const char* fname               ///< [in]  name of attachment.
	) {
	H5_API_ENTER (h5_err_t, "fname='%s'", fname);
	H5_API_RETURN (h5_get_attachment (f, fname));
}

/**
  Delete attachment \c fname.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5DeleteAttachment (
	const h5_file_t f,              ///< [in]  file handle.
	const char* const fname         ///< [in]  name of attachment.
	) {
	H5_API_ENTER (h5_err_t, "fname='%s'", fname);
	H5_API_RETURN (h5_delete_attachment (f, fname));
}
/**
   @}
*/
	
#ifdef __cplusplus
}
#endif

#endif
