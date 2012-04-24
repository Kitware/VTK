/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef OLD_HEADER_FILENAME
#include <iostream.h>
#else
#include <iostream>
#endif
#include <string>

#include "H5Include.h"
#include "H5Exception.h"
#include "H5IdComponent.h"
#include "H5PropList.h"
#include "H5Object.h"
#include "H5FaccProp.h"
#include "H5FcreatProp.h"
#include "H5DxferProp.h"
#include "H5DcreatProp.h"
#include "H5CommonFG.h"
#include "H5Group.h"
#include "H5AbstractDs.h"
#include "H5DataSpace.h"
#include "H5DataSet.h"
#include "H5File.h"
#include "H5Alltypes.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#ifndef H5_NO_STD
    using std::cerr;
    using std::endl;
#endif  // H5_NO_STD
#endif

//--------------------------------------------------------------------------
// Function	H5File default constructor
///\brief	Default constructor: creates a stub H5File object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5File::H5File() : IdComponent(), id(0) {}

//--------------------------------------------------------------------------
// Function:	H5File overloaded constructor
///\brief	Creates or opens an HDF5 file depending on the parameter flags.
///\param	name         - IN: Name of the file
///\param	flags        - IN: File access flags
///\param	create_plist - IN: File creation property list, used when
///		modifying default file meta-data.  Default to
///		FileCreatPropList::DEFAULT
///\param	access_plist - IN: File access property list.  Default to
///		FileCreatPropList::DEFAULT
///\par Description
///		Valid values of \a flags include:
///		\li \c H5F_ACC_TRUNC - Truncate file, if it already exists,
///				       erasing all data previously stored in
///				       the file.
///		\li \c H5F_ACC_EXCL - Fail if file already exists.
///			\c H5F_ACC_TRUNC and \c H5F_ACC_EXCL are mutually exclusive
///		\li \c H5F_ACC_DEBUG - print debug information. This flag is
///			used only by HDF5 library developers; it is neither
///			tested nor supported for use in applications.
///\par
///		For info on file creation in the case of an already-open file,
///		please refer to the \b Special \b case section in the C layer
///		Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5File::H5File( const char* name, unsigned int flags, const FileCreatPropList& create_plist, const FileAccPropList& access_plist ) : IdComponent(0)
{
   p_get_file(name, flags, create_plist, access_plist);
}

//--------------------------------------------------------------------------
// Function:	H5File overloaded constructor
///\brief	This is another overloaded constructor.  It differs from the
///		above constructor only in the type of the \a name argument.
///\param	name - IN: Name of the file - \c H5std_string
///\param	flags - IN: File access flags
///\param	create_plist - IN: File creation property list, used when
///		modifying default file meta-data.  Default to
///		FileCreatPropList::DEFAULT
///\param	access_plist - IN: File access property list.  Default to
///		FileCreatPropList::DEFAULT
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5File::H5File( const H5std_string& name, unsigned int flags, const FileCreatPropList& create_plist, const FileAccPropList& access_plist ) : IdComponent(0)
{
   p_get_file(name.c_str(), flags, create_plist, access_plist);
}

//--------------------------------------------------------------------------
// This function is private and contains common code between the
// constructors taking a string or a char*
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5File::p_get_file(const char* name, unsigned int flags, const FileCreatPropList& create_plist, const FileAccPropList& access_plist)
{
    // These bits only set for creation, so if any of them are set,
    // create the file.
    if( flags & (H5F_ACC_CREAT|H5F_ACC_EXCL|H5F_ACC_TRUNC|H5F_ACC_DEBUG))
    {
	hid_t create_plist_id = create_plist.getId();
	hid_t access_plist_id = access_plist.getId();
	id = H5Fcreate( name, flags, create_plist_id, access_plist_id );
	if( id < 0 )  // throw an exception when open/create fail
	{
	    throw FileIException("H5File constructor", "H5Fcreate failed");
	}
    }
    // Open the file if none of the bits above are set.
    else
    {
	hid_t access_plist_id = access_plist.getId();
	id = H5Fopen( name, flags, access_plist_id );
	if( id < 0 )  // throw an exception when open/create fail
	{
	    throw FileIException("H5File constructor", "H5Fopen failed");
	}
    }
}

//--------------------------------------------------------------------------
// Function:	H5File copy constructor
///\brief	Copy constructor: makes a copy of the original
///		H5File object.
///\param	original - IN: H5File instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5File::H5File(const H5File& original) : IdComponent(original)
{
    id = original.getId();
    incRefCount(); // increment number of references to this id
}

//--------------------------------------------------------------------------
// Function:	H5File::flush
///\brief	Flushes all buffers associated with a file to disk.
///\param	scope - IN: Specifies the scope of the flushing action,
///		which can be either of these values:
///		\li \c H5F_SCOPE_GLOBAL - Flushes the entire virtual file
///		\li \c H5F_SCOPE_LOCAL - Flushes only the specified file
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - Dec. 2005
//--------------------------------------------------------------------------
void H5File::flush(H5F_scope_t scope) const
{
   herr_t ret_value = H5Fflush( id, scope );
   if( ret_value < 0 )
   {
      throw FileIException("H5File::flush", "H5Fflush failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::isHdf5
///\brief	Determines whether a file in HDF5 format.
///\param	name - IN: Name of the file
///\return	true if the file is in HDF5 format, and false, otherwise
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool H5File::isHdf5(const char* name)
{
   // Calls C routine H5Fis_hdf5 to determine whether the file is in
   // HDF5 format.  It returns positive value, 0, or negative value
   htri_t ret_value = H5Fis_hdf5( name );
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else // Raise exception when H5Fis_hdf5 returns a negative value
   {
      throw FileIException("H5File::isHdf5", "H5Fis_hdf5 returned negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::isHdf5
///\brief	This is an overloaded member function, provided for convenience.
///		It takes an \c H5std_string for \a name.
///\param	name - IN: Name of the file - \c H5std_string
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool H5File::isHdf5(const H5std_string& name )
{
   return( isHdf5( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	openFile
///\brief	Opens an HDF5 file
///\param	name         - IN: Name of the file
///\param	flags        - IN: File access flags
///\param	access_plist - IN: File access property list.  Default to
///		FileCreatPropList::DEFAULT
///\par Description
///		Valid values of \a flags include:
///		H5F_ACC_RDWR:   Open with read/write access. If the file is
///				currently open for read-only access then it
///				will be reopened. Absence of this flag
///				implies read-only access.
///
///		H5F_ACC_RDONLY: Open with read only access. - default
///
// Programmer	Binh-Minh Ribler - Oct, 2005
//--------------------------------------------------------------------------
void H5File::openFile(const char* name, unsigned int flags, const FileAccPropList& access_plist)
{
    hid_t access_plist_id = access_plist.getId();
    id = H5Fopen (name, flags, access_plist_id);
    if (id < 0)  // throw an exception when open fails
    {
	throw FileIException("H5File::openFile", "H5Fopen failed");
    }
}

//--------------------------------------------------------------------------
// Function:	H5File::openFile
///\brief	This is an overloaded member function, provided for convenience.
///		It takes an \c H5std_string for \a name.
///\param	name         - IN: Name of the file - \c H5std_string
///\param	flags        - IN: File access flags
///\param	access_plist - IN: File access property list.  Default to
///		FileAccPropList::DEFAULT
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5File::openFile(const H5std_string& name, unsigned int flags, const FileAccPropList& access_plist)
{
    openFile(name.c_str(), flags, access_plist);
}

//--------------------------------------------------------------------------
// Function:	H5File::reOpen
///\brief	Reopens this file.
///
///\exception	H5::FileIException
// Description
//		If this object has represented another HDF5 file, the previous
//		HDF5 file need to be closed first.
// Programmer	Binh-Minh Ribler - 2000
// Note:        This wrapper doesn't seem right regarding the 'id' and should
//              be investigated.  BMR - 2/20/2005
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Feb 20, 2005
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
void H5File::reOpen()
{
    try {
        close();
    }
    catch (Exception close_error) {
        throw FileIException("H5File::reOpen", close_error.getDetailMsg());
    }

   // call C routine to reopen the file - Note: not sure about this
   // does id need to be closed later?  which id to be the parameter?
   id = H5Freopen( id );
   if( id < 0 ) // Raise exception when H5Freopen returns a neg value
      throw FileIException("H5File::reOpen", "H5Freopen failed");
}

//--------------------------------------------------------------------------
// Function:	H5File::reopen
///\brief	Reopens this file.
///
///\exception	H5::FileIException
///\par Description
///		This function will be replaced by the above function \c reOpen
///		in future releases.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5File::reopen()
{
   H5File::reOpen();
}

//--------------------------------------------------------------------------
// Function:	H5File::getCreatePlist
///\brief	Returns the creation property list of this file
///\return	FileCreatPropList object
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileCreatPropList H5File::getCreatePlist() const
{
   hid_t create_plist_id = H5Fget_create_plist( id );

   // if H5Fget_create_plist returns a valid id, create and return
   // the FileCreatPropList object for this property list
   if( create_plist_id > 0 )
   {
      FileCreatPropList create_plist( create_plist_id );
      return( create_plist );
   }
   else
   {
      throw FileIException("H5File::getCreatePlist", "H5Fget_create_plist failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getAccessPlist
///\brief	Returns the access property list of this file
///\return	FileAccPropList object
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileAccPropList H5File::getAccessPlist() const
{
   hid_t access_plist_id = H5Fget_access_plist( id );

   // if H5Fget_access_plist returns a valid id, create and return
   // the FileAccPropList object for this property list
   if( access_plist_id > 0 )
   {
      FileAccPropList access_plist( access_plist_id );
      return access_plist;
   }
   else // Raise an exception
   {
      throw FileIException("H5File::getAccessPlist", "H5Fget_access_plist failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getFreeSpace
///\brief	Returns the amount of free space in the file.
///\return	Amount of free space
///\exception	H5::FileIException
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
hssize_t H5File::getFreeSpace() const
{
   hssize_t free_space = H5Fget_freespace(id);
   if( free_space < 0 )
   {
      throw FileIException("H5File::getFreeSpace", "H5Fget_freespace failed");
   }
   return (free_space);
}


//--------------------------------------------------------------------------
// Function:	H5File::getObjCount
///\brief	Returns the number of opened object IDs (files, datasets,
///		groups and datatypes) in the same file.
///\param	types - Type of object to retrieve the count
///\return	Number of opened object IDs
///\exception	H5::FileIException
///\par Description
///		The valid values for \a types include:
///		\li \c H5F_OBJ_FILE	- Files only
///		\li \c H5F_OBJ_DATASET	- Datasets only
///		\li \c H5F_OBJ_GROUP	- Groups only
///		\li \c H5F_OBJ_DATATYPE	- Named datatypes only
///		\li \c H5F_OBJ_ATTR	- Attributes only
///		\li \c H5F_OBJ_ALL    - All of the above, i.e., \c H5F_OBJ_FILE
///					| \c H5F_OBJ_DATASET | \c H5F_OBJ_GROUP
///					| \c H5F_OBJ_DATATYPE | \c H5F_OBJ_ATTR
///\par
/// Multiple object types can be combined with the logical OR operator (|).
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ssize_t H5File::getObjCount(unsigned types) const
{
   ssize_t num_objs = H5Fget_obj_count(id, types);
   if( num_objs < 0 )
   {
      throw FileIException("H5File::getObjCount", "H5Fget_obj_count failed");
   }
   return (num_objs);
}

//--------------------------------------------------------------------------
// Function:	H5File::getObjCount
///\brief	This is an overloaded member function, provided for convenience.
///		It takes no parameter and returns the object count of all
///		object types.
///\return	Number of opened object IDs
///\exception	H5::FileIException
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ssize_t H5File::getObjCount() const
{
   ssize_t num_objs = H5Fget_obj_count(id, H5F_OBJ_ALL);
   if( num_objs < 0 )
   {
      throw FileIException("H5File::getObjCount", "H5Fget_obj_count failed");
   }
   return (num_objs);
}

//--------------------------------------------------------------------------
// Function:	H5File::getObjIDs
///\brief	Retrieves a list of opened object IDs (files, datasets,
///		groups and datatypes) in the same file.
///\param	types    - Type of object to retrieve the count
///\param	max_objs - Maximum number of object identifiers to place
///			   into obj_id_list.
///\param	oid_list - List of open object identifiers
///\exception	H5::FileIException
///\par Description
///		The valid values for \a types include:
///		\li \c H5F_OBJ_FILE	- Files only
///		\li \c H5F_OBJ_DATASET	- Datasets only
///		\li \c H5F_OBJ_GROUP	- Groups only
///		\li \c H5F_OBJ_DATATYPE	- Named datatypes only
///		\li \c H5F_OBJ_ATTR	- Attributes only
///		\li \c H5F_OBJ_ALL    - All of the above, i.e., \c H5F_OBJ_FILE
///					| \c H5F_OBJ_DATASET | \c H5F_OBJ_GROUP
///					| \c H5F_OBJ_DATATYPE | \c H5F_OBJ_ATTR
///\par
/// Multiple object types can be combined with the logical OR operator (|).
//
// Notes: will do the overload for this one after hearing from Quincey???
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
void H5File::getObjIDs(unsigned types, size_t max_objs, hid_t *oid_list) const
{
   ssize_t ret_value = H5Fget_obj_ids(id, types, max_objs, oid_list);
   if( ret_value < 0 )
   {
      throw FileIException("H5File::getObjIDs", "H5Fget_obj_ids failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getVFDHandle
///\brief	Returns the pointer to the file handle of the low-level file
///		driver.
///\param	fapl        - File access property list
///\param	file_handle - Pointer to the file handle being used by
///			      the low-level virtual file driver
///\exception	H5::FileIException
///\par Description
///		For the \c FAMILY or \c MULTI drivers, \a fapl should be
///		defined through the property list functions:
///		\c FileAccPropList::setFamilyOffset for the \c FAMILY driver
///		and \c FileAccPropList::setMultiType for the \c MULTI driver.
///
///		The obtained file handle is dynamic and is valid only while
///		the file remains open; it will be invalid if the file is
///		closed and reopened or opened during a subsequent session.
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
void H5File::getVFDHandle(FileAccPropList& fapl, void **file_handle) const
{
   hid_t fapl_id = fapl.getId();
   herr_t ret_value = H5Fget_vfd_handle(id, fapl_id, file_handle);
   if( ret_value < 0 )
   {
      throw FileIException("H5File::getVFDHandle", "H5Fget_vfd_handle failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getVFDHandle
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in what arguments it
///		accepts.
///\param	file_handle - Pointer to the file handle being used by
///			      the low-level virtual file driver
///\exception	H5::FileIException
// Programmer   Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
void H5File::getVFDHandle(void **file_handle) const
{
   herr_t ret_value = H5Fget_vfd_handle(id, H5P_DEFAULT, file_handle);
   if( ret_value < 0 )
   {
      throw FileIException("H5File::getVFDHandle", "H5Fget_vfd_handle failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getFileName
///\brief	Gets the name of this file.
///\return	File name
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - Jul, 2004
//--------------------------------------------------------------------------
H5std_string H5File::getFileName() const
{
   try {
      return(p_get_file_name());
   }
   catch (IdComponentException E) {
      throw FileIException("H5File::getFileName", E.getDetailMsg());
   }
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:	H5File::getObjType
///\brief	Retrieves the type of object that an object reference points to.
///\param	ref      - IN: Reference to query
///\param	ref_type - IN: Type of reference, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference.
///		\li \c H5R_DATASET_REGION - Reference is a dataset region reference.
///\return	Object type, which can be one of the following:
///		\li \c H5G_LINK    - Object is a symbolic link.
///		\li \c H5G_GROUP   - Object is a group.
///		\li \c H5G_DATASET - Object is a dataset.
///		\li \c H5G_TYPE    - Object is a named datatype
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
H5G_obj_t H5File::getObjType(void *ref, H5R_type_t ref_type) const
{
   try {
      return(p_get_obj_type(ref, ref_type));
   }
   catch (IdComponentException E) {
      throw FileIException("H5File::getObjType", E.getDetailMsg());
   }
}
#endif /* H5_NO_DEPRECATED_SYMBOLS */

//--------------------------------------------------------------------------
// Function:	H5File::getRegion
///\brief	Retrieves a dataspace with the region pointed to selected.
///\param	ref      - IN: Reference to get region of
///\param	ref_type - IN: Type of reference to get region of - default
///\return	DataSpace instance
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
DataSpace H5File::getRegion(void *ref, H5R_type_t ref_type) const
{
   try {
      DataSpace dataspace(p_get_region(ref, ref_type));
      return(dataspace);
   }
   catch (IdComponentException E) {
      throw FileIException("H5File::getRegion", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	H5File::getFileSize
///\brief	Returns the file size of the HDF5 file.
///\return	File size
///\exception	H5::FileIException
///\par Description
///		This function is called after an existing file is opened in
///		order to learn the true size of the underlying file.
// Programmer   Raymond Lu - June 24, 2004
//--------------------------------------------------------------------------
hsize_t H5File::getFileSize() const
{
   hsize_t file_size;
   herr_t ret_value = H5Fget_filesize(id, &file_size);
   if (ret_value < 0)
   {
      throw FileIException("H5File::getFileSize", "H5Fget_filesize failed");
   }
   return (file_size);
}

//--------------------------------------------------------------------------
// Function:    H5File::p_reference (protected)
// Purpose      Creates a reference to an HDF5 object or a dataset region.
// Parameters
//              name - IN: Name of the object to be referenced
//              dataspace - IN: Dataspace with selection
//              ref_type - IN: Type of reference; default to \c H5R_DATASET_REGION
// Exception    H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5File::p_reference(void* ref, const char* name, hid_t space_id, H5R_type_t ref_type) const
{
   herr_t ret_value = H5Rcreate(ref, getId(), name, ref_type, space_id);
   if (ret_value < 0)
   {
      throw IdComponentException("", "H5Rcreate failed");
   }
}

//--------------------------------------------------------------------------
// Function:    H5File::reference
///\brief       Creates a reference to an HDF5 object or a dataset region.
///\param       ref - IN: Reference pointer
///\param       name - IN: Name of the object to be referenced
///\param       dataspace - IN: Dataspace with selection
///\param       ref_type - IN: Type of reference to query, valid values are:
///             \li \c H5R_OBJECT         - Reference is an object reference.
///             \li \c H5R_DATASET_REGION - Reference is a dataset region
///                     reference. - this is the default
///\exception   H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5File::reference(void* ref, const char* name, const DataSpace& dataspace, H5R_type_t ref_type) const
{
   try {
      p_reference(ref, name, dataspace.getId(), ref_type);
   }
   catch (IdComponentException E) {
      throw IdComponentException("H5File::reference", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:    H5File::reference
///\brief       This is an overloaded function, provided for your convenience.
///             It differs from the above function in that it only creates
///             a reference to an HDF5 object, not to a dataset region.
///\param       ref - IN: Reference pointer
///\param       name - IN: Name of the object to be referenced - \c char pointer
///\exception   H5::IdComponentException
///\par Description
//              This function passes H5R_OBJECT and -1 to the protected
//              function for it to pass to the C API H5Rcreate
//              to create a reference to the named object.
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5File::reference(void* ref, const char* name) const
{
   try {
      p_reference(ref, name, -1, H5R_OBJECT);
   }
   catch (IdComponentException E) {
      throw IdComponentException("H5File::reference", E.getDetailMsg());
   }
}
//--------------------------------------------------------------------------
// Function:    H5File::reference
///\brief       This is an overloaded function, provided for your convenience.
///             It differs from the above function in that it takes an
///             \c H5std_string for the object's name.
///\param       ref - IN: Reference pointer
///\param       name - IN: Name of the object to be referenced - \c H5std_string
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5File::reference(void* ref, const H5std_string& name) const
{
   reference(ref, name.c_str());
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:    H5File::p_get_obj_type (protected)
// Purpose      Retrieves the type of object that an object reference points to.
// Parameters
//              ref      - IN: Reference to query
//              ref_type - IN: Type of reference to query
// Return       An object type, which can be one of the following:
//                      H5G_LINK Object is a symbolic link.
//                      H5G_GROUP Object is a group.
//                      H5G_DATASET   Object is a dataset.
//                      H5G_TYPE Object is a named datatype
// Exception    H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
H5G_obj_t H5File::p_get_obj_type(void *ref, H5R_type_t ref_type) const
{
   H5G_obj_t obj_type = H5Rget_obj_type1(getId(), ref_type, ref);

   if (obj_type == H5G_UNKNOWN)
   {
      throw IdComponentException("", "H5Rget_obj_type failed");
   }
   return(obj_type);
}
#endif /* H5_NO_DEPRECATED_SYMBOLS */


//--------------------------------------------------------------------------
// Function:    H5File::p_get_region (protected)
// Purpose      Retrieves a dataspace with the region pointed to selected.
// Parameters
//              ref_type - IN: Type of reference to get region of - default
//                              to H5R_DATASET_REGION
//              ref      - IN: Reference to get region of
// Return       Dataspace id
// Exception    H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
hid_t H5File::p_get_region(void *ref, H5R_type_t ref_type) const
{
   hid_t space_id = H5Rget_region(getId(), ref_type, ref);
   if (space_id < 0)
   {
      throw IdComponentException("", "H5Rget_region failed");
   }
   return(space_id);
}

//--------------------------------------------------------------------------
// Function:	H5File::getLocId
// Purpose:	Get the id of this file
// Description
//		This function is a redefinition of CommonFG::getLocId.  It
//		is used by CommonFG member functions to get the file id.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hid_t H5File::getLocId() const
{
   return( getId() );
}

//--------------------------------------------------------------------------
// Function:    H5File::getId
// Purpose:     Get the id of this attribute
// Modification:
//      May 2008 - BMR
//              Class hierarchy is revised to address bugzilla 1068.  Class
//              AbstractDS and Attribute are moved out of H5Object.  In
//              addition, member IdComponent::id is moved into subclasses, and
//              IdComponent::getId now becomes pure virtual function.
// Programmer   Binh-Minh Ribler - May, 2008
//--------------------------------------------------------------------------
hid_t H5File::getId() const
{
   return(id);
}

//--------------------------------------------------------------------------
// Function:    H5File::p_setId
///\brief       Sets the identifier of this object to a new value.
///
///\exception   H5::IdComponentException when the attempt to close the HDF5
///             object fails
// Description:
//              The underlaying reference counting in the C library ensures
//              that the current valid id of this object is properly closed.
//              Then the object's id is reset to the new id.
// Programmer   Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5File::p_setId(const hid_t new_id)
{
    // handling references to this old id
    try {
        close();
    }
    catch (Exception E) {
        throw FileIException("H5File::p_setId", E.getDetailMsg());
    }
   // reset object's id to the given id
   id = new_id;
}

//--------------------------------------------------------------------------
// Function:	H5File::close
///\brief	Closes this HDF5 file.
///
///\exception	H5::FileIException
// Programmer	Binh-Minh Ribler - Mar 9, 2005
//--------------------------------------------------------------------------
void H5File::close()
{
    if (p_valid_id(id))
    {
	herr_t ret_value = H5Fclose( id );
	if( ret_value < 0 )
	{
	    throw FileIException("H5File::close", "H5Fclose failed");
	}
	// reset the id when the file that it represents is no longer
	// referenced
	if (getCounter() == 0)
	    id = 0;
    }
}

//--------------------------------------------------------------------------
// Function:	H5File::throwException
///\brief	Throws file exception - initially implemented for CommonFG
///\param	func_name - Name of the function where failure occurs
///\param	msg       - Message describing the failure
///\exception	H5::FileIException
// Description
// 		This function is used in CommonFG implementation so that
//		proper exception can be thrown for file or group.  The
//		argument func_name is a member of CommonFG and "H5File::"
//		will be inserted to indicate the function called is an
//		implementation of H5File.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5File::throwException(const H5std_string& func_name, const H5std_string& msg) const
{
   H5std_string full_name = func_name;
   full_name.insert(0, "H5File::");
   throw FileIException(full_name, msg);
}

//--------------------------------------------------------------------------
// Function:	H5File destructor
///\brief	Properly terminates access to this file.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Feb 20, 2005
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
H5File::~H5File()
{
    try {
	close();
    } catch (Exception close_error) {
	cerr << "H5File::~H5File - " << close_error.getDetailMsg() << endl;
    }
}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
