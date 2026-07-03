/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:	The public header file for the "I/O concentrator" driver.
 * This provides a similar functionality to that of the subfiling driver
 * but introduces the necessary file access functionality via a multi-
 * threading MPI service
 */

#ifndef H5FDioc_H
#define H5FDioc_H

/* Public header files */
#include "H5FDpublic.h" /* File drivers             */

#ifdef H5_HAVE_IOC_VFD

/**
 * \def H5FD_IOC
 * Macro that returns the identifier for the #H5FD_IOC driver. \hid_t{file driver}
 *
 * \since 1.14.0
 */
#define H5FD_IOC (H5OPEN H5FD_IOC_id_g)
#else
#define H5FD_IOC (H5I_INVALID_HID)
#endif

/**
 * \def H5FD_IOC_NAME
 * The canonical name for the #H5FD_IOC driver
 *
 * \since 1.14.0
 */
#define H5FD_IOC_NAME "ioc"

#ifdef H5_HAVE_IOC_VFD

#ifndef H5FD_IOC_FAPL_MAGIC
/**
 * \def H5FD_IOC_CURR_FAPL_VERSION
 * The version number of the H5FD_ioc_config_t configuration
 * structure for the #H5FD_IOC driver
 */
#define H5FD_IOC_CURR_FAPL_VERSION 1
/**
 * \def H5FD_IOC_FAPL_MAGIC
 * Unique number used to distinguish the #H5FD_IOC driver from other HDF5 file drivers
 */
#define H5FD_IOC_FAPL_MAGIC 0xFED21331
#endif

/**
 * \def H5FD_IOC_DEFAULT_THREAD_POOL_SIZE
 * The default number of I/O concentrator worker threads
 */
#define H5FD_IOC_DEFAULT_THREAD_POOL_SIZE 4

/*
 * Environment variables interpreted by the IOC VFD
 */

/**
 * \def H5FD_IOC_THREAD_POOL_SIZE
 * Macro for name of the environment variable that controls/overrides
 * the number of I/O concentrator worker threads
 *
 * The value set for this environment variable is interpreted as an
 * int value and must be > 0.
 */
#define H5FD_IOC_THREAD_POOL_SIZE "H5FD_IOC_THREAD_POOL_SIZE"

//! <!-- [H5FD_ioc_config_t_snip] -->
/**
 * \struct H5FD_ioc_config_t
 * \brief Configuration structure for H5Pset_fapl_ioc() / H5Pget_fapl_ioc()
 *
 * \details H5FD_ioc_config_t is a public structure that is used to pass
 *          configuration data to the #H5FD_IOC driver via a File Access
 *          Property List. A pointer to an instance of this structure is
 *          a parameter to H5Pset_fapl_ioc() and H5Pget_fapl_ioc().
 *
 * \var uint32_t H5FD_ioc_config_t::magic
 *      A somewhat unique number which distinguishes the #H5FD_IOC driver
 *      from other drivers. Used in combination with a version number, it
 *      can help to validate a user-generated File Access Property List.
 *      This field should be set to #H5FD_IOC_FAPL_MAGIC.
 *
 * \var uint32_t H5FD_ioc_config_t::version
 *      Version number of the H5FD_ioc_config_t structure. Any instance passed
 *      to H5Pset_fapl_ioc() / H5Pget_fapl_ioc() must have a recognized version
 *      number or an error will be raised. Currently, this field should be set
 *      to #H5FD_IOC_CURR_FAPL_VERSION.
 *
 * \var int32_t H5FD_ioc_config_t::thread_pool_size
 *      The number of I/O concentrator worker threads to use.
 *
 *      This value can also be set or adjusted with the #H5FD_IOC_THREAD_POOL_SIZE
 *      environment variable.
 *
 */
typedef struct H5FD_ioc_config_t {
    uint32_t magic;            /* Must be set to H5FD_IOC_FAPL_MAGIC */
    uint32_t version;          /* Must be set to H5FD_IOC_CURR_FAPL_VERSION */
    int32_t  thread_pool_size; /* Number of I/O concentrator worker threads to use */
} H5FD_ioc_config_t;
//! <!-- [H5FD_ioc_config_t_snip] -->

#ifdef __cplusplus
extern "C" {
#endif

/** @private
 *
 * \brief ID for the IOC VFD
 */
H5_DLLVAR hid_t H5FD_IOC_id_g;

/**
 * \ingroup FAPL
 *
 * \brief Modifies the specified File Access Property List to use the #H5FD_IOC driver
 *
 * \fapl_id
 * \param[in] vfd_config Pointer to #H5FD_IOC driver configuration structure. May be NULL.
 * \returns \herr_t
 *
 * \details H5Pset_fapl_ioc() modifies the File Access Property List to use the
 *          #H5FD_IOC driver.
 *
 *          The #H5FD_IOC driver is a reference implementation of an "I/O concentrator"
 *          file driver that works in conjunction with the #H5FD_SUBFILING driver and
 *          provides the I/O backend for servicing I/O requests to subfiles.
 *
 *          Typically, an HDF5 application won't need to call this routine directly.
 *          The #H5FD_IOC driver is usually set up as a side effect of an HDF5 application
 *          using the #H5FD_SUBFILING driver, but this routine is provided in case the
 *          application wishes to manually configure the #H5FD_IOC driver.
 *
 * \note The \p vfd_config parameter may be NULL. In this case, the driver will
 *       be setup with default settings. Note that in this case, it is assumed
 *       the parent #H5FD_SUBFILING driver was also setup with default settings.
 *       If the two drivers differ in configuration settings, application behavior
 *       may not be as expected.
 *
 * \since 1.14.0
 *
 */
H5_DLL herr_t H5Pset_fapl_ioc(hid_t fapl_id, H5FD_ioc_config_t *vfd_config);
/**
 * \ingroup FAPL
 *
 * \brief Queries a File Access Property List for #H5FD_IOC file driver properties
 *
 * \fapl_id
 * \param[out] config_out Pointer to H5FD_ioc_config_t structure through which the
 *                        #H5FD_IOC file driver properties will be returned.
 *
 * \returns \herr_t
 *
 * \details H5Pget_fapl_ioc() queries the specified File Access Property List for
 *          #H5FD_IOC driver properties as set by H5Pset_fapl_ioc(). If the #H5FD_IOC
 *          driver has not been set on the File Access Property List, a default
 *          configuration is returned. An HDF5 application may use this functionality
 *          to manually configure the #H5FD_IOC driver by calling H5Pget_fapl_ioc()
 *          on a newly-created File Access Property List, adjusting the default
 *          values and then calling H5Pset_fapl_ioc() with the configured
 *          H5FD_ioc_config_t structure.
 *
 * \since 1.14.0
 *
 */
H5_DLL herr_t H5Pget_fapl_ioc(hid_t fapl_id, H5FD_ioc_config_t *config_out);

#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_IOC_VFD */

#endif
