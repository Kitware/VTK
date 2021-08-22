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
 *		H5P package.  Including this header means that the source file
 *		is part of the H5P package.
 */
#ifndef H5Pmodule_H
#define H5Pmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5P_MODULE
#define H5_MY_PKG      H5P
#define H5_MY_PKG_ERR  H5E_PLIST
#define H5_MY_PKG_INIT YES

/**\defgroup H5P H5P
 * \brief Property List Interface
 *
 * \details The HDF5 Property List Interface provides a mechanism to take
 *          advantage of more powerful or unusual features in HDF5.
 *
 *          HDF5 objects have properties or characteristics associated with
 *          them, and there are default properties that handle the most
 *          common needs. These default properties can be modified using the
 *          HDF5 Property List Interface. For example, the data storage
 *          layout property of a dataset is contiguous by default. For better
 *          performance, the layout can be modified to be chunked or chunked
 *          and compressed.
 *
 * \todo Describe concisely what the functions in this module are about.
 *
 * \defgroup GPLO General Property List Operations
 * \ingroup H5P
 * \defgroup GPLOA General Property List Operations (Advanced)
 * \ingroup H5P
 * \defgroup FCPL File Creation Properties
 * \ingroup H5P
 * \defgroup FAPL File Access Properties
 * \ingroup H5P
 * \defgroup GCPL Group Creation Properties
 * \ingroup H5P
 * \defgroup ALCAPL Attribute and Link Creation Properties
 * \ingroup H5P
 * \defgroup LAPL Link Access Properties
 * \ingroup H5P
 * \defgroup DCPL Dataset Creation Properties
 * \ingroup H5P
 * \defgroup DAPL Dataset Access Properties
 * \ingroup H5P
 * \defgroup DXPL Dataset Transfer Properties
 * \ingroup H5P
 * \defgroup OCPL Object Creation Properties
 * \ingroup H5P
 * \defgroup OCPPL Object Copy Properties
 * \ingroup H5P
 * \defgroup GACPL General Access Properties
 * \ingroup H5P
 * \defgroup MAPL Map Access Properties
 * \ingroup H5P
 */

#endif /* H5Pmodule_H */
