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

/* Purpose: This file contains declarations which define macros for the
 *          H5R package.  Including this header means that the source file
 *          is part of the H5R package.
 */
#ifndef H5Rmodule_H
#define H5Rmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5R_MODULE
#define H5_MY_PKG     H5R
#define H5_MY_PKG_ERR H5E_REFERENCE

/** \page H5R_UG HDF5 References
 *
 * \section sec_reference HDF5 References
 * HDF5 references allow users to reference existing HDF5 objects (file, group, dataset, named datatype, or
 * attribute) as well as selections within datasets.
 *
 * The original API, now deprecated, was extended in order to add the ability to reference attributes as well
 * as objects in external files. Additionally, there were some inherent limitations within the older API that
 * restricted its use with virtual object layer (VOL) connectors, which do not necessarily follow HDF5’s
 * native file format.
 *
 * The newer API introduced a single opaque reference type, which not only has the advantage of hiding the
 * internal representation of references, but it also allows for future extensions to be added more
 * seamlessly.
 *
 * \subsection subsec_reference_intro Introduction
 * The deprecated HDF5 reference API only allowed users to create references to HDF5 objects (groups,
 * datasets) and regions within a dataset. There were some limitations: it defined two separate reference
 * types #hobj_ref_t and #hdset_reg_ref_t; the former directly mapped to an #haddr_t type that did not allow
 * for external references, while the latter mapped to an HDF5 global heap entry, which was specific to native
 * HDF5 and was created and written to the file when the reference was created. This prevented users from
 * creating region references when the file is opened read-only, it was also not suitable for use outside of
 * native HDF5 files. The newer API addressed these limitations by introducing a single abstract #H5R_ref_t
 * type as well as missing reference types such as attribute references and external references (i.e.,
 * references to objects in an external file).
 *
 * \subsection subsec_reference_dep Deprecated API
 * There is no support for attribute references; references are only valid within the
 * container that they reference; the size of the reference types are tied to the definition of an haddr_t or
 * an entry in the file’s global heap, which only exists in native HDF5.
 *
 * \subsubsection subsubsec_reference_limit Limitations
 * \li The #H5Rcreate signature forces users to constantly pass (#H5I_INVALID_HID) as a space_id, in the case
 * where the reference type is not a region reference.
 * \li The size of region
 * references was defined as the size required to encode a global heap ID, this definition forces
 * references to be written to the file at the time of their creation, hence preventing them to be created
 * from a file that is opened read-only (e.g, when creating references to a file that one does not want
 * to/cannot modify).
 *
 * \subsubsection subsubsec_reference_old_API Deprecated Methods
 * The original API before hdf5 1.12.0 is defined below:
 * \code
 *   // Deprecated reference buffer sizes that are kept for backward compatibility
 *   #define H5R_OBJ_REF_BUF_SIZE      sizeof(haddr_t)
 *   #define H5R_DSET_REG_REF_BUF_SIZE (sizeof(haddr_t) + 4)
 *
 *   // Reference types
 *   typedef enum H5R_type_t {
 *       H5R_BADTYPE = (-1), // Invalid Reference Type
 *       H5R_OBJECT,         // Object reference
 *       H5R_DATASET_REGION, // Dataset Region Reference
 *       H5R_MAXTYPE         // Highest type (Invalid as true type)
 *   } H5R_type_t;
 *
 *   // Object reference structure for user's code
 *   // This needs to be large enough to store largest haddr_t on a worst case
 *   // machine (8 bytes currently).
 *   typedef haddr_t hobj_ref_t;
 *
 *   // Dataset Region reference structure for user's code
 *   // (Buffer to store heap ID and index)
 *   // This needs to be large enough to store largest haddr_t in a worst case
 *   // machine (8 bytes currently) plus an int
 *   typedef unsigned char hdset_reg_ref_t[H5R_DSET_REG_REF_BUF_SIZE];
 *
 *   // Prototypes
 *   herr_t H5Rcreate(void *ref, hid_t loc_id, const char *name, H5R_type_t ref_type, hid_t space_id);
 *   hid_t H5Rdereference2(hid_t obj_id, hid_t oapl_id, H5R_type_t ref_type, const void *ref);
 *   hid_t H5Rget_region(hid_t dataset, H5R_type_t ref_type, const void *ref);
 *   herr_t H5Rget_obj_type2(hid_t id, H5R_type_t ref_type, const void *_ref, H5O_type_t *obj_type);
 *   ssize_t H5Rget_name(hid_t loc_id, H5R_type_t ref_type, const void *ref, char *name , size_t size);
 * \endcode
 *
 * \subsection subsec_reference_new New API
 * The current API is defined below:
 * \code
 *   // Deprecated reference buffer sizes that are kept for backward compatibility
 *   #define H5R_OBJ_REF_BUF_SIZE      sizeof(haddr_t)
 *   #define H5R_DSET_REG_REF_BUF_SIZE (sizeof(haddr_t) + 4)
 *
 *   // Default reference buffer size.
 *   #define H5R_REF_BUF_SIZE (64)
 *
 *   // Reference types allowed.
 *   typedef enum {
 *       H5R_BADTYPE         = (-1), // Invalid reference type
 *       H5R_OBJECT1         = 0,    // Backward compatibility (object)
 *       H5R_DATASET_REGION1 = 1,    // Backward compatibility (region)
 *       H5R_OBJECT2         = 2,    // Object reference
 *       H5R_DATASET_REGION2 = 3,    // Region reference
 *       H5R_ATTR            = 4,    // Attribute Reference
 *       H5R_MAXTYPE         = 5     // Highest type (invalid)
 *   } H5R_type_t;
 *
 *   // Deprecated object reference type that is used with deprecated reference APIs.
 *   // This type can only be used with the "native" HDF5 VOL connector.
 *   typedef haddr_t hobj_ref_t;
 *
 *   // Deprecated dataset region reference type that is used with deprecated reference APIs.
 *   // This type can only be used with the "native" HDF5 VOL connector.
 *   typedef struct {
 *       uint8_t __data[H5R_DSET_REG_REF_BUF_SIZE];
 *   } hdset_reg_ref_t;
 *
 *   // Opaque reference type. The same reference type is used for object,
 *   // dataset region and attribute references. This is the type that
 *   // should always be used with the current reference API.
 *   typedef struct {
 *       union {
 *           uint8_t __data[H5R_REF_BUF_SIZE]; // opaque data
 *           int64_t align;                    // ensures alignment
 *       } u;
 *   } H5R_ref_t;
 *
 *   // Constructors
 *   herr_t H5Rcreate_object(hid_t loc_id, const char *name, H5R_ref_t *ref_ptr);
 *   herr_t H5Rcreate_region(hid_t loc_id, const char *name, hid_t space_id, H5R_ref_t *ref_ptr);
 *   herr_t H5Rcreate_attr(hid_t loc_id, const char *name, const char *attr_name, H5R_ref_t *ref_ptr);
 *   herr_t H5Rdestroy(H5R_ref_t *ref_ptr);
 *
 *   // Info
 *   H5R_type_t H5Rget_type(const H5R_ref_t *ref_ptr);
 *   htri_t H5Requal(const H5R_ref_t *ref1_ptr, const H5R_ref_t *ref2_ptr);
 *   herr_t H5Rcopy(const H5R_ref_t *src_ref_ptr, H5R_ref_t *dst_ref_ptr);
 *
 *   // Dereference
 *   hid_t H5Ropen_object(const H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id);
 *   hid_t H5Ropen_region(const H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id);
 *   hid_t H5Ropen_attr(const H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t aapl_id);
 *
 *   // Get type
 *   herr_t H5Rget_obj_type3(const H5R_ref_t *ref_ptr, hid_t rapl_id, H5O_type_t *obj_type);
 *
 *   // Get name
 *   ssize_t H5Rget_file_name(const H5R_ref_t *ref_ptr, char *name, size_t size);
 *   ssize_t H5Rget_obj_name(const H5R_ref_t *ref_ptr, hid_t rapl_id, char *name, size_t size);
 *   ssize_t H5Rget_attr_name(const H5R_ref_t *ref_ptr, char *name, size_t size);
 * \endcode
 *
 * References can be stored and retrieved from a file by invoking the #H5Dwrite and #H5Dread functions
 * with this single predefined type: #H5T_STD_REF.
 *
 * The advantage of a single type is that it becomes easier for users to mix references of different types. It
 * is also more in line with the opaque type now defined for references. Note that when reading references
 * back from a file, the library may, in consequence of this new design, allocate memory for each of these
 * references. To release the memory, one must either call #H5Rdestroy on each of the references or, for
 * convenience, call the new #H5Treclaim function on the buffer that contains the array of references (type
 * can be compound type, array).
 *
 * As mentioned, instead of having separate routines for both vlen and reference types, we unify the existing:
 * \code
 *    herr_t H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t dxpl_id, void *buf);
 * \endcode
 * to
 * \code
 *    herr_t H5Treclaim(hid_t type_id, hid_t space_id, hid_t dxpl_id, void *buf);
 * \endcode
 *
 * \subsection subsec_reference_compat API Compatibility
 * To preserve compatibility with applications and middleware libraries that have been using the existing
 * reference API, we keep the existing #H5Rcreate, #H5Rdereference2, #H5Rget_region,
 * #H5Rget_obj_type2 and #H5Rget_name routines, but moved to the deprecated
 * API list of functions.
 *
 * It is important to note though that these routines only support the original reference types, noted as
 * #H5R_OBJECT1 and #H5R_DATASET_REGION1 respectively. Any other reference type passed to these routines
 * will return an error. For convenience and compatibility with previous versions of the library we define
 * both #H5R_OBJECT and #H5R_DATASET_REGION to map to the original reference types \code
 *   // Versions for compatibility
 *   #define H5R_OBJECT         H5R_OBJECT1
 *   #define H5R_DATASET_REGION H5R_DATASET_REGION1
 * \endcode
 *
 * When creating and accessing references through these deprecated routines, users are still expected to use
 * the datatypes which describe the #hobj_ref_t and #hdset_reg_ref_t types, #H5T_STD_REF_OBJ and
 * #H5T_STD_REF_DSETREG.
 *
 * One important aspect of these changes is to ensure that previously written data can still be readable after
 * those revisions and that new files produced will not create any undefined behavior when used with previous
 * versions of the library. Backward as well as forward compatibility is summarized in the table:
 *
 * <table>
 * <tr>
 * <th>Version</th><th>Old File Format/Old API</th><th>Old File Format/New API</th><th>New File Format/Old
 * API</th><th>New File Format/New API</th>
 * </tr>
 * <tr>
 * <td>&lt; 1.12.0</td><td>No change</td><td>N/A</td><td>Datatype version bump prevents from reading unknown
 * reference types</td><td>N/A</td>
 * </tr>
 * <tr>
 * <td>&ge; 1.12.0</td>
 * <td>Read and write references through old datatypes and use #hobj_ref_t and #hdset_reg_ref_t types</td>
 * <td>Read and write using #H5T_STD_REF to convert to new #H5R_ref_t type</td>
 * <td>Cannot use old API with new reference types</td>
 * <td>Can use opaque #H5R_ref_t type for all reference types</td>
 * </tr>
 * </table>
 *
 * Because previous library versions do not have a way of detecting when new unknown references types are
 * read, we have to increment the global version of the datatypes, so that early detection can be done and the
 * appropriate error is returned to the user. For versions prior to this change, the library will return an
 * error when the datatype encountered has a version number greater than the currently supported version.
 * Also, to prevent datatype version changes in the future, all library branches are now patched to check for
 * unknown reference types.
 *
 * When reading old data with the new library version, one can either keep using the #H5T_STD_REF_OBJ
 * and #H5T_STD_REF_DSETREG datatypes, which can be queried when opening a dataset, for example using
 * #H5Dget_type, or use the #H5T_STD_REF datatype, which will trigger automatic type conversion. The
 * #H5T_STD_REF_OBJ and #H5T_STD_REF_DSETREG datatypes require the use of the respective #hobj_ref_t
 * and #hdset_reg_ref_t types, which can only be used with the old API functions. These types do not embed
 * all the required information to be simply cast to an #H5R_ref_t type. When an #H5R_ref_t type is desired,
 * the #H5T_STD_REF datatype must be used, allowing old reference data to be used with the new API.
 *
 * \subsection subsec_reference_example Usage Examples
 *
 * \subsubsection subsubsec_reference_example_new External References
 * The example below illustrates the use of the new API with files that are opened read-only. Created
 * references to the objects in that file are stored into a separate file, and accessed from that file,
 * without the user explicitly opening the original file that was referenced. \code #include <stdlib.h>
 *
 *   #include "hdf5.h"
 *   #include <assert.h>
 *
 *   #define H5FILE_NAME1 "refer_extern1.h5"
 *   #define H5FILE_NAME2 "refer_extern2.h5"
 *
 *   #define NDIMS 1 // Number of dimensions
 *   #define BUF_SIZE 4 // Size of example buffer
 *   #define NREFS 1 // Number of references
 *
 *   int main(void) {
 *       hid_t file1, dset1, space1;
 *       hsize_t dset1_dims[NDIMS] = { BUF_SIZE };
 *       int dset_buf[BUF_SIZE];
 *
 *       hid_t file2, dset2, space2;
 *       hsize_t dset2_dims[NDIMS] = { NREFS };
 *       H5R_ref_t ref_buf[NREFS] = { 0 };
 *       H5O_type_t obj_type;
 *       int i;
 *
 *       for (i = 0; i < BUF_SIZE; i++)
 *           dset_buf[i] = i;
 *
 *       // Create file with one dataset and close it
 *       file1 = H5Fcreate(H5FILE_NAME1, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
 *       space1 = H5Screate_simple(NDIMS, dset1_dims, NULL);
 *       dset1 = H5Dcreate2(file1, "dataset1", H5T_NATIVE_INT, space1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
 *       H5Dwrite(dset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_buf);
 *       H5Dclose(dset1);
 *       H5Sclose(space1);
 *       H5Fclose(file1);
 *
 *       // Create reference to dataset1 in "refer_extern1.h5"
 *       file1 = H5Fopen(H5FILE_NAME1, H5F_ACC_RDONLY, H5P_DEFAULT);
 *       H5Rcreate_object(file1, "dataset1", &ref_buf[0]);
 *       H5Fclose(file1);
 *
 *       // Store reference in dataset in separate file "refer_extern2.h5"
 *       file2 = H5Fcreate(H5FILE_NAME2, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
 *       space2 = H5Screate_simple(NDIMS, dset2_dims, NULL);
 *       dset2 = H5Dcreate2(file2, "references", H5T_STD_REF, space2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
 *       H5Dwrite(dset2, H5T_STD_REF, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_buf);
 *       H5Dclose(dset2);
 *       H5Sclose(space2);
 *       H5Fclose(file2);
 *       H5Rdestroy(&ref_buf[0]);
 *
 *       // Read reference back from "refer_extern2.h5"
 *       file2 = H5Fopen(H5FILE_NAME2, H5F_ACC_RDONLY, H5P_DEFAULT);
 *       dset2 = H5Dopen2(file2, "references", H5P_DEFAULT);
 *       H5Dread(dset2, H5T_STD_REF, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_buf);
 *       H5Dclose(dset2);
 *       H5Fclose(file2);
 *
 *       // Access reference and read dataset data without opening original file
 *       assert(H5Rget_type((const H5R_ref_t *)&ref_buf[0]) == H5R_OBJECT2);
 *       H5Rget_obj_type3((const H5R_ref_t *)&ref_buf[0], H5P_DEFAULT, &obj_type);
 *       assert(obj_type == H5O_TYPE_DATASET);
 *       dset1 = H5Ropen_object((const H5R_ref_t *)&ref_buf[0], H5P_DEFAULT, H5P_DEFAULT);
 *       H5Dread(dset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_buf);
 *       H5Dclose(dset1);
 *       H5Rdestroy(&ref_buf[0]);
 *
 *       for (i = 0; i < BUF_SIZE; i++)
 *           assert(dset_buf[i] == i);
 *
 *       return 0;
 *   }
 * \endcode
 *
 * \subsubsection subsubsec_reference_example_old Backward Compatibility and New API
 * The example below illustrates the use of the new API with a file that was written using the old-style
 * reference API, showing how one can take advantage of the automatic type conversion from old reference type
 * to new reference type.
 * \code #include <stdlib.h>
 *
 *   #include "hdf5.h"
 *   #include <assert.h>
 *
 *   #define H5FILE_NAME "refer_deprec.h5"
 *
 *   #define NDIMS 1 // Number of dimensions
 *   #define BUF_SIZE 4 // Size of example buffer
 *   #define NREFS 1 // Number of references
 *
 *   int main(void) {
 *       hid_t file1, dset1, space1;
 *       hsize_t dset1_dims[NDIMS] = { BUF_SIZE };
 *       int dset_buf[BUF_SIZE];
 *
 *       hid_t dset2, space2;
 *       hsize_t dset2_dims[NDIMS] = { NREFS };
 *       hobj_ref_t ref_buf[NREFS] = { 0 };
 *       H5R_ref_t new_ref_buf[NREFS] = { 0 };
 *       H5O_type_t obj_type;
 *       int i;
 *
 *       for (i = 0; i < BUF_SIZE; i++)
 *           dset_buf[i] = i;
 *
 *       // Create file with one dataset and close it
 *       file1 = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
 *
 *       space1 = H5Screate_simple(NDIMS, dset1_dims, NULL);
 *       dset1 = H5Dcreate2(file1, "dataset1", H5T_NATIVE_INT, space1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
 *       H5Dwrite(dset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_buf);
 *       H5Dclose(dset1);
 *       H5Sclose(space1);
 *
 *       // Create reference to dataset1 with deprecated API
 *       // (reminder: there is no destroy call for those references)
 *       H5Rcreate(&ref_buf[0], file1, "dataset1", H5R_OBJECT, H5I_INVALID_HID);
 *
 *       // Store reference in separate dataset using deprecated reference type
 *       space2 = H5Screate_simple(NDIMS, dset2_dims, NULL);
 *       dset2 = H5Dcreate2(file1, "references", H5T_STD_REF_OBJ, space2, H5P_DEFAULT, H5P_DEFAULT,
 *                          H5P_DEFAULT);
 *       H5Dwrite(dset2, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_buf); H5Dclose(dset2);
 *       H5Sclose(space2);
 *       H5Fclose(file1);
 *
 *       // Read reference from file using new reference type
 *       file1 = H5Fopen(H5FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);
 *       dset2 = H5Dopen2(file1, "references", H5P_DEFAULT);
 *       H5Dread(dset2, H5T_STD_REF, H5S_ALL, H5S_ALL, H5P_DEFAULT, new_ref_buf);
 *       H5Dclose(dset2);
 *
 *       // Access reference and read dataset data through new API
 *       assert(H5Rget_type((const H5R_ref_t *)&new_ref_buf[0]) == H5R_OBJECT2);
 *       H5Rget_obj_type3((const H5R_ref_t *)&new_ref_buf[0], H5P_DEFAULT, &obj_type);
 *       assert(obj_type == H5O_TYPE_DATASET);
 *       dset1 = H5Ropen_object((const H5R_ref_t *)&new_ref_buf[0], H5P_DEFAULT, H5P_DEFAULT);
 *       H5Dread(dset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_buf);
 *       H5Dclose(dset1);
 *       H5Rdestroy(&new_ref_buf[0]);
 *
 *       for (i = 0; i < BUF_SIZE; i++)
 *           assert(dset_buf[i] == i);
 *       return 0;
 *   }
 * \endcode
 *
 *
 */

/**
 * \defgroup H5R References (H5R)
 *
 * Use the functions in this module to manage HDF5 references. Referents can
 * be HDF5 objects, attributes, and selections on datasets a.k.a. dataset
 * regions.
 *
 */

#endif /* H5Rmodule_H */
