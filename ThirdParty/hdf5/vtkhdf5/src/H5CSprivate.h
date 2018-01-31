/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  Header file for function stacks, etc.
 */
#ifndef _H5CSprivate_H
#define _H5CSprivate_H

#ifdef NOT_YET
#include "H5CSpublic.h"
#endif /* NOT_YET */

/* Private headers needed by this file */
#include "H5private.h"

/* Forward declarations for structure fields */
struct H5CS_t;
H5_DLL herr_t H5CS_push(const char *func_name);
H5_DLL herr_t H5CS_pop(void);
H5_DLL herr_t H5CS_print_stack(const struct H5CS_t *stack, FILE *stream);
H5_DLL struct H5CS_t *H5CS_copy_stack(void);
H5_DLL herr_t H5CS_close_stack(struct H5CS_t *stack);

#endif /* _H5CSprivate_H */

