/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Private definitions for HDF5 Subfiling VFD
 */

#ifndef H5FDsubfiling_priv_H
#define H5FDsubfiling_priv_H

/********************/
/* Standard Headers */
/********************/

#include <stdatomic.h>

/**************/
/* H5 Headers */
/**************/

#include "H5private.h"     /* Generic Functions                        */
#include "H5CXprivate.h"   /* API Contexts                             */
#include "H5Dprivate.h"    /* Datasets                                 */
#include "H5Eprivate.h"    /* Error handling                           */
#include "H5FDsubfiling.h" /* Subfiling VFD                            */
#include "H5FDioc.h"       /* IOC VFD                                  */
#include "H5Iprivate.h"    /* IDs                                      */
#include "H5MMprivate.h"   /* Memory management                        */
#include "H5Pprivate.h"    /* Property lists                           */

#include "H5subfiling_common.h"

#define DRIVER_INFO_MESSAGE_MAX_INFO   65536
#define DRIVER_INFO_MESSAGE_MAX_LENGTH 65552 /* MAX_INFO + sizeof(info_header_t) */

typedef struct _info_header { /* Header for a driver info message */
    uint8_t version;
    uint8_t unused_1;
    uint8_t unused_2;
    uint8_t unused_3;    /* Actual info message length, but  */
    int32_t info_length; /* CANNOT exceed 64k (65552) bytes  */
    char    vfd_key[8];  /* 's' 'u' 'b' 'f' 'i' 'l' 'i' 'n'  */
} info_header_t;

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5FD__subfiling__truncate_sub_files(hid_t context_id, int64_t logical_file_eof, MPI_Comm comm);
H5_DLL herr_t H5FD__subfiling__get_real_eof(hid_t context_id, int64_t *logical_eof_ptr);

#ifdef __cplusplus
}
#endif

#endif /* H5FDsubfiling_priv_H */
