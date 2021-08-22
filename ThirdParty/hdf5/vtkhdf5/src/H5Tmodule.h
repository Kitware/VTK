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
 * Programmer:	Quincey Koziol
 *		Saturday, September 12, 2015
 *
 * Purpose:	This file contains declarations which define macros for the
 *		H5T package.  Including this header means that the source file
 *		is part of the H5T package.
 */
#ifndef H5Tmodule_H
#define H5Tmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5T_MODULE
#define H5_MY_PKG      H5T
#define H5_MY_PKG_ERR  H5E_DATATYPE
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5T H5T
 * \brief Datatype Interface
 * \todo Describe concisely what the functions in this module are about.
 *
 * \defgroup ARRAY Array Datatypes
 * \ingroup H5T
 * \defgroup ATOM Atomic Datatypes
 * \ingroup H5T
 * \defgroup COMPOUND Compound Datatypes
 * \ingroup H5T
 * \defgroup CONV Conversion Function
 * \ingroup H5T
 * \defgroup ENUM Enumeration Datatypes
 * \ingroup H5T
 * \defgroup OPAQUE Opaque Datatypes
 * \ingroup H5T
 * \defgroup VLEN Variable-length Sequence Datatypes
 * \ingroup H5T
 *
 * \defgroup PDT Predefined Datatypes
 * \ingroup H5T
 * \details What is a predefined HDF5 datatype?
 * \todo Fill in the blanks!
 *
 * \defgroup PDTCPU By CPU
 * \ingroup PDT
 * \details CPU-specific datatypes
 * \defgroup PDTALPHA DEC Alpha
 * \ingroup PDTCPU
 * \defgroup PDTX86 AMD & INTEL
 * \ingroup PDTCPU
 * \defgroup PDTMIPS SGI MIPS
 * \ingroup PDTCPU
 *
 * \defgroup PDTIEEE IEEE
 * \ingroup PDT
 * \details The IEEE floating point types in big- and little-endian byte orders.
 *
 * \defgroup PDTSTD Standard Datatypes
 * \ingroup PDT
 * \details These are "standard" types. For instance, signed (2's complement)
 *          and unsigned integers of various sizes in big- and little-endian
 *          byte orders.
 *
 * \defgroup PDTUNIX UNIX-specific Datatypes
 * \ingroup PDT
 * \details Types which are particular to Unix.
 * \todo Fill in the blanks!
 *
 * \defgroup PDTNAT Native Datatypes
 * \ingroup PDT
 * \details These are the datatypes detected during library \Emph{compilation}
 *          by \c H5detect(). Their names differ from other HDF5 datatype names
 *          as follows:
 *          \li Instead of a class name, precision and byte order as the last
 *              component, they have a C-like type name.
 *          \li If the type begins with \c U then it is the unsigned version of
 *              the integer type; other integer types are signed.
 *          \li The datatype \c LLONG corresponds C's \Code{long long} and
 *              \c LDOUBLE is \Code{long double}. These types might be the same
 *              as \c LONG and \c DOUBLE, respectively.
 * \defgroup PDTC9x C9x Integer Datatypes
 * \ingroup PDTNAT
 * \details C9x integer types
 * \todo Fill in the blanks!
 *
 * \defgroup PDTS Strings
 * \ingroup PDT
 *
 */

#endif /* H5Tmodule_H */
