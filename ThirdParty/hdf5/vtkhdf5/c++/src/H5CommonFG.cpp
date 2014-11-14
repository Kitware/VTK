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
#include "H5private.h"		// for HDstrcpy

// There are a few comments that are common to most of the functions
// defined in this file so they are listed here.
// - getLocId is called by all functions, that call a C API, to get
//   the location id, which can be either a file id or a group id.
//   This function is pure virtual and it's up to H5File and Group
//   to call the right getId() - although, as the structure of the
//   library at this time, getId() is basically the IdComponent::getId()
// - when a failure returned by the C API, the functions will call
//   throwException, which is a pure virtual function and is implemented
//   by H5File to throw a FileIException and by Group to throw a
//   GroupIException.
// December 2000

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	CommonFG::createGroup
///\brief	Creates a new group at this location which can be a file
///		or another group.
///\param	name  - IN: Name of the group to create
///\param	size_hint - IN: Indicates the number of bytes to reserve for
///		the names that will appear in the group
///\return	Group instance
///\exception	H5::FileIException or H5::GroupIException
///\par Description
///		The optional \a size_hint specifies how much file space to
///		reserve for storing the names that will appear in this new
///		group. If a non-positive value is provided for the \a size_hint
///		then a default size is chosen.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group CommonFG::createGroup( const char* name, size_t size_hint ) const
{
   // Group creation property list for size_hint
   hid_t gcpl_id = 0;

   // Set the local heap size hint
   if(!(size_hint == (size_t)-1 || size_hint == 0)) {

       // If the creation of the property list failed, throw an exception
       if((gcpl_id = H5Pcreate(H5P_GROUP_CREATE)) < 0)
          throwException("createGroup", "H5Pcreate failed");

       if( H5Pset_local_heap_size_hint(gcpl_id, size_hint) < 0) {
          H5Pclose(gcpl_id);
          throwException("createGroup", "H5Pset_local_heap_size failed");
       }
    }

   // Call C routine H5Gcreate2 to create the named group, giving the
   // location id which can be a file id or a group id
   hid_t group_id = H5Gcreate2( getLocId(), name, H5P_DEFAULT, gcpl_id, H5P_DEFAULT );

   // Close the group creation property list, if necessary
   if(gcpl_id > 0)
       H5Pclose(gcpl_id);

   // If the creation of the group failed, throw an exception
   if( group_id < 0 )
      throwException("createGroup", "H5Gcreate2 failed");

   // No failure, create and return the Group object
   Group group( group_id );
   return( group );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::createGroup
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group CommonFG::createGroup( const H5std_string& name, size_t size_hint ) const
{
   return( createGroup( name.c_str(), size_hint ));
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openGroup
///\brief	Opens an existing group in a location which can be a file
///		or another group.
///\param	name  - IN: Name of the group to open
///\return	Group instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group CommonFG::openGroup( const char* name ) const
{
   // Call C routine H5Gopen2 to open the named group, giving the
   // location id which can be a file id or a group id
   hid_t group_id = H5Gopen2( getLocId(), name, H5P_DEFAULT );

   // If the opening of the group failed, throw an exception
   if( group_id < 0 )
      throwException("openGroup", "H5Gopen2 failed");

   // No failure, create and return the Group object
   Group group( group_id );
   return( group );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openGroup
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group CommonFG::openGroup( const H5std_string& name ) const
{
   return( openGroup( name.c_str() ));
}

//--------------------------------------------------------------------------
// Function:	CommonFG::createDataSet
///\brief	Creates a new dataset at this location.
///\param	name  - IN: Name of the dataset to create
///\param	data_type - IN: Datatype of the dataset
///\param	data_space - IN: Dataspace for the dataset
///\param	create_plist - IN: Creation properly list for the dataset
///\return	DataSet instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSet CommonFG::createDataSet( const char* name, const DataType& data_type, const DataSpace& data_space, const DSetCreatPropList& create_plist ) const
{
   // Obtain identifiers for C API
   hid_t type_id = data_type.getId();
   hid_t space_id = data_space.getId();
   hid_t create_plist_id = create_plist.getId();

   // Call C routine H5Dcreate2 to create the named dataset
   hid_t dataset_id = H5Dcreate2( getLocId(), name, type_id, space_id, H5P_DEFAULT, create_plist_id, H5P_DEFAULT );

   // If the creation of the dataset failed, throw an exception
   if( dataset_id < 0 )
      throwException("createDataSet", "H5Dcreate2 failed");

   // No failure, create and return the DataSet object
   DataSet dataset( dataset_id );
   return( dataset );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::createDataSet
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSet CommonFG::createDataSet( const H5std_string& name, const DataType& data_type, const DataSpace& data_space, const DSetCreatPropList& create_plist ) const
{
   return( createDataSet( name.c_str(), data_type, data_space, create_plist ));
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openDataSet
///\brief	Opens an existing dataset at this location.
///\param	name  - IN: Name of the dataset to open
///\return	DataSet instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSet CommonFG::openDataSet( const char* name ) const
{
   // Call C function H5Dopen2 to open the specified dataset, giving
   // the location id and the dataset's name
   hid_t dataset_id = H5Dopen2( getLocId(), name, H5P_DEFAULT );

   // If the dataset's opening failed, throw an exception
   if(dataset_id < 0)
      throwException("openDataSet", "H5Dopen2 failed");

   // No failure, create and return the DataSet object
   DataSet dataset( dataset_id );
   return( dataset );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openDataSet
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSet CommonFG::openDataSet( const H5std_string& name ) const
{
   return( openDataSet( name.c_str() ));
}

//--------------------------------------------------------------------------
// Function:	CommonFG::link
///\brief	Creates a link of the specified type from \a new_name to
///		\a curr_name.
///\param	link_type  - IN: Link type; possible values are
///		\li \c H5G_LINK_HARD
///		\li \c H5G_LINK_SOFT
///\param	curr_name - IN: Name of the existing object if link is a hard
///		link; can be anything for the soft link
///\param	new_name - IN: New name for the object
///\exception	H5::FileIException or H5::GroupIException
///\par Description
///		Note that both names are interpreted relative to the
///		specified location.
///		For information on creating hard link and soft link, please
///		refer to the C layer Reference Manual at:
/// http://hdfgroup.org/HDF5/doc/RM/RM_H5L.html#Link-CreateHard and
/// http://hdfgroup.org/HDF5/doc/RM/RM_H5L.html#Link-CreateSoft
// Programmer	Binh-Minh Ribler - 2000
// Modification
//	2007: QAK modified to use H5L APIs - BMR
//--------------------------------------------------------------------------
void CommonFG::link( H5L_type_t link_type, const char* curr_name, const char* new_name ) const
{
    herr_t ret_value = -1;

    switch(link_type) {
        case H5L_TYPE_HARD:
            ret_value = H5Lcreate_hard( getLocId(), curr_name, H5L_SAME_LOC, new_name, H5P_DEFAULT, H5P_DEFAULT );
            break;

        case H5L_TYPE_SOFT:
            ret_value = H5Lcreate_soft( curr_name, getLocId(), new_name, H5P_DEFAULT, H5P_DEFAULT );
            break;

        default:
            throwException("link", "unknown link type");
            break;
    } /* end switch */

   if( ret_value < 0 )
      throwException("link", "creating link failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::link
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a curr_name and \a new_name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::link( H5L_type_t link_type, const H5std_string& curr_name, const H5std_string& new_name ) const
{
   link( link_type, curr_name.c_str(), new_name.c_str() );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::unlink
///\brief	Removes the specified name at this location.
///\param	name  - IN: Name of the object to be removed
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
// Modification
//	2007: QAK modified to use H5L APIs - BMR
//--------------------------------------------------------------------------
void CommonFG::unlink( const char* name ) const
{
   herr_t ret_value = H5Ldelete( getLocId(), name, H5P_DEFAULT );
   if( ret_value < 0 )
      throwException("unlink", "H5Ldelete failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::unlink
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::unlink( const H5std_string& name ) const
{
   unlink( name.c_str() );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::move
///\brief	Renames an object at this location.
///\param	src - IN: Object's original name
///\param	dst - IN: Object's new name
///\exception	H5::FileIException or H5::GroupIException
///\note
///		Exercise care in moving groups as it is possible to render
///		data in a file inaccessible with Group::move. Please refer
///		to the Group Interface in the HDF5 User's Guide for details at:
/// http://www.hdfgroup.org/HDF5/doc/UG/UG_frame09Groups.html
// Programmer	Binh-Minh Ribler - 2000
// Modification
//	2007: QAK modified to use H5L APIs - BMR
//--------------------------------------------------------------------------
void CommonFG::move( const char* src, const char* dst ) const
{
   herr_t ret_value = H5Lmove( getLocId(), src, H5L_SAME_LOC, dst, H5P_DEFAULT, H5P_DEFAULT );
   if( ret_value < 0 )
      throwException("move", "H5Lmove failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::move
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a src and \a dst.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::move( const H5std_string& src, const H5std_string& dst ) const
{
   move( src.c_str(), dst.c_str() );
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:	CommonFG::getObjinfo
///\brief	Returns information about an object.
///\param	name  - IN: Name of the object
///\param	follow_link - IN: Link flag
///\param	statbuf - OUT: Buffer to return information about the object
///\exception	H5::FileIException or H5::GroupIException
///\par Description
///		For more information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-GetObjinfo
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::getObjinfo( const char* name, hbool_t follow_link, H5G_stat_t& statbuf ) const
{
   herr_t ret_value = H5Gget_objinfo( getLocId(), name, follow_link, &statbuf );
   if( ret_value < 0 )
      throwException("getObjinfo", "H5Gget_objinfo failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjinfo
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::getObjinfo( const H5std_string& name, hbool_t follow_link, H5G_stat_t& statbuf ) const
{
   getObjinfo( name.c_str(), follow_link, statbuf );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjinfo
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above functions in that it doesn't have
///		the paramemter \a follow_link.
// Programmer	Binh-Minh Ribler - Nov, 2005
// Note: need to modify to use H5Oget_info and H5Lget_info - BMR
//--------------------------------------------------------------------------
void CommonFG::getObjinfo( const char* name, H5G_stat_t& statbuf ) const
{
   herr_t ret_value = H5Gget_objinfo( getLocId(), name, 0, &statbuf );
   if( ret_value < 0 )
      throwException("getObjinfo", "H5Gget_objinfo failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjinfo
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - Nov, 2005
//--------------------------------------------------------------------------
void CommonFG::getObjinfo( const H5std_string& name, H5G_stat_t& statbuf ) const
{
   getObjinfo( name.c_str(), statbuf );
}
#endif /* H5_NO_DEPRECATED_SYMBOLS */

//--------------------------------------------------------------------------
// Function:	CommonFG::getLinkval
///\brief	Returns the name of the object that the symbolic link points to.
///\param	name  - IN: Symbolic link to the object
///\param	size - IN: Maximum number of characters of value to be returned
///\return	Name of the object
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string CommonFG::getLinkval( const char* name, size_t size ) const
{
    H5L_info_t linkinfo;
    char *value_C;	// value in C string
    size_t val_size = size;
    H5std_string value = "";
    herr_t ret_value;

    // if user doesn't provide buffer size, determine it
    if (size == 0)
    {
	ret_value = H5Lget_info(getLocId(), name, &linkinfo, H5P_DEFAULT);
	if( ret_value < 0 )
	    throwException("getLinkval", "H5Lget_info to find buffer size failed");

	val_size = linkinfo.u.val_size;
    }

    // if link has value, retrieve the value, otherwise, return null string
    if (val_size > 0)
    {
	value_C = new char[val_size+1];  // temporary C-string for C API
	HDmemset(value_C, 0, val_size+1); // clear buffer

	ret_value = H5Lget_val(getLocId(), name, value_C, val_size, H5P_DEFAULT);
	if( ret_value < 0 )
	    throwException("getLinkval", "H5Lget_val failed");

	value = H5std_string(value_C);
	delete []value_C;
    }
    return(value);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getLinkval
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string CommonFG::getLinkval( const H5std_string& name, size_t size ) const
{
   return( getLinkval( name.c_str(), size ));
}

//--------------------------------------------------------------------------
// Function:	CommonFG::mount
///\brief	Mounts the file \a child onto this group.
///\param	name  - IN: Name of the group
///\param	child - IN: File to mount
///\param	plist - IN: Property list to use
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2014 (original 2000)
//--------------------------------------------------------------------------
void CommonFG::mount(const char* name, const H5File& child, const PropList& plist ) const
{
   // Obtain identifiers for C API
   hid_t plist_id = plist.getId();
   hid_t child_id = child.getId();

   // Call C routine H5Fmount to do the mouting
   herr_t ret_value = H5Fmount( getLocId(), name, child_id, plist_id );

   // Raise exception if H5Fmount returns negative value
   if( ret_value < 0 )
      throwException("mount", "H5Fmount failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::mount
///\brief	This is an overloaded member function, kept for backward
///		compatibility.  It differs from the above function in that it
///		misses const's.  This wrapper will be removed in future release.
///\param	name  - IN: Name of the group
///\param	child - IN: File to mount
///\param	plist - IN: Property list to use
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::mount(const char* name, H5File& child, PropList& plist) const
{
   mount(name, (const H5File)child, (const PropList)plist);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::mount
///\brief	This is an overloaded member function, provided for convenience.
///		It takes an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::mount(const H5std_string& name, const H5File& child, const PropList& plist) const
{
   mount(name.c_str(), child, plist);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::mount
///\brief	This is an overloaded member function, kept for backward
///		compatibility.  It differs from the above function in that it
///		misses const's.  This wrapper will be removed in future release.
// Programmer	Binh-Minh Ribler - 2014
//--------------------------------------------------------------------------
void CommonFG::mount(const H5std_string& name, H5File& child, PropList& plist) const
{
   mount(name.c_str(), (const H5File)child, (const PropList)plist);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::unmount
///\brief	Unmounts the specified file.
///\param	name  - IN: Name of the file to unmount
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::unmount( const char* name ) const
{
   // Call C routine H5Fmount to do the mouting
   herr_t ret_value = H5Funmount( getLocId(), name );

   // Raise exception if H5Funmount returns negative value
   if( ret_value < 0 )
      throwException("unmount", "H5Funmount failed");
}

//--------------------------------------------------------------------------
// Function:	CommonFG::unmount
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void CommonFG::unmount( const H5std_string& name ) const
{
   unmount( name.c_str() );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openDataType
///\brief	Opens the named generic datatype at this location.
///\param	name  - IN: Name of the datatype to open
///\return	DataType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType CommonFG::openDataType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openDataType", "H5Topen2 failed");

   // No failure, create and return the DataType object
   DataType data_type(type_id);
   return(data_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openDataType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType CommonFG::openDataType( const H5std_string& name ) const
{
   return( openDataType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openArrayType
///\brief	Opens the named array datatype at this location.
///\param	name  - IN: Name of the array datatype to open
///\return	ArrayType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
ArrayType CommonFG::openArrayType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openArrayType", "H5Topen2 failed");

   // No failure, create and return the ArrayType object
   ArrayType array_type (type_id);
   return(array_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openArrayType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
ArrayType CommonFG::openArrayType( const H5std_string& name ) const
{
   return( openArrayType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openCompType
///\brief	Opens the named compound datatype at this location.
///\param	name  - IN: Name of the compound datatype to open
///\return	CompType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
CompType CommonFG::openCompType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openCompType", "H5Topen2 failed");

   // No failure, create and return the CompType object
   CompType comp_type(type_id);
   return(comp_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openCompType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
CompType CommonFG::openCompType( const H5std_string& name ) const
{
   return( openCompType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openEnumType
///\brief	Opens the named enumeration datatype at this location.
///\param	name  - IN: Name of the enumeration datatype to open
///\return	EnumType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType CommonFG::openEnumType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openEnumType", "H5Topen2 failed");

   // No failure, create and return the EnumType object
   EnumType enum_type(type_id);
   return(enum_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openEnumType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType CommonFG::openEnumType( const H5std_string& name ) const
{
   return( openEnumType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openIntType
///\brief	Opens the named integer datatype at this location.
///\param	name  - IN: Name of the integer datatype to open
///\return	IntType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType CommonFG::openIntType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openIntType", "H5Topen2 failed");

   // No failure, create and return the IntType object
   IntType int_type(type_id);
   return(int_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openIntType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType CommonFG::openIntType( const H5std_string& name ) const
{
   return( openIntType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openFloatType
///\brief	Opens the named floating-point datatype at this location.
///\param	name  - IN: Name of the floating-point datatype to open
///\return	FloatType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType CommonFG::openFloatType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openFloatType", "H5Topen2 failed");

   // No failure, create and return the FloatType object
   FloatType float_type(type_id);
   return(float_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openFloatType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType CommonFG::openFloatType( const H5std_string& name ) const
{
   return( openFloatType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openStrType
///\brief	Opens the named string datatype at this location.
///\param	name  - IN: Name of the string datatype to open
///\return	StrType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
StrType CommonFG::openStrType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openStrType", "H5Topen2 failed");

   // No failure, create and return the StrType object
   StrType str_type(type_id);
   return(str_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openStrType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
StrType CommonFG::openStrType( const H5std_string& name ) const
{
   return( openStrType( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openVarLenType
///\brief	Opens the named variable length datatype at this location.
///\param	name  - IN: Name of the variable length datatype to open
///\return	VarLenType instance
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
VarLenType CommonFG::openVarLenType( const char* name ) const
{
   // Call C function H5Topen2 to open the named datatype in this group,
   // given either the file or group id
   hid_t type_id = H5Topen2(getLocId(), name, H5P_DEFAULT);

   // If the datatype's opening failed, throw an exception
   if( type_id < 0 )
      throwException("openVarLenType", "H5Topen2 failed");

   // No failure, create and return the VarLenType object
   VarLenType varlen_type(type_id);
   return(varlen_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::openVarLenType
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
VarLenType CommonFG::openVarLenType( const H5std_string& name ) const
{
   return( openVarLenType( name.c_str()) );
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:	CommonFG::iterateElems
///\brief	Iterates a user's function over the entries of a group.
///\param	name    - IN    : Name of group to iterate over
///\param	idx     - IN/OUT: Starting (IN) and ending (OUT) entry indices
///\param	op      - IN    : User's function to operate on each entry
///\param	op_data - IN/OUT: Data associated with the operation
///\return	The return value of the first operator that returns non-zero,
///		or zero if all members were processed with no operator
///		returning non-zero.
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int CommonFG::iterateElems( const char* name, int *idx, H5G_iterate_t op , void* op_data )
{
   int ret_value = H5Giterate( getLocId(), name, idx, op, op_data );
   if( ret_value < 0 )
   {
      throwException("iterateElems", "H5Giterate failed");
   }
   return( ret_value );
}

//--------------------------------------------------------------------------
// Function:	CommonFG::iterateElems
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int CommonFG::iterateElems( const H5std_string& name, int *idx, H5G_iterate_t op , void* op_data )
{
   return( iterateElems( name.c_str(), idx, op, op_data ));
}
#endif /* H5_NO_DEPRECATED_SYMBOLS */

//--------------------------------------------------------------------------
// Function:	CommonFG::getNumObjs
///\brief	Returns the number of objects in this group.
///\return	Number of objects
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - January, 2003
//--------------------------------------------------------------------------
hsize_t CommonFG::getNumObjs() const
{
   H5G_info_t 		ginfo;                  /* Group information */

   herr_t ret_value = H5Gget_info(getLocId(), &ginfo);
   if(ret_value < 0)
      throwException("getNumObjs", "H5Gget_info failed");
   return (ginfo.nlinks);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjnameByIdx
///\brief	Returns the name of an object in this group, given the
///		object's index.
///\param	idx  -     IN: Transient index of the object
///\return	Object name
///\exception	H5::FileIException or H5::GroupIException
///\par Description
///		The value of idx can be any nonnegative number less than the
///		total number of objects in the group, which is returned by
///		the function \c CommonFG::getNumObjs.  Note that this is a
///		transient index; thus, an object may have a different index
///		each time the group is opened.
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
H5std_string CommonFG::getObjnameByIdx(hsize_t idx) const
{
    // call H5Lget_name_by_idx with name as NULL to get its length
    ssize_t name_len = H5Lget_name_by_idx(getLocId(), ".", H5_INDEX_NAME, H5_ITER_INC, idx, NULL, 0, H5P_DEFAULT);
    if(name_len < 0)
      throwException("getObjnameByIdx", "H5Lget_name_by_idx failed");

    // now, allocate C buffer to get the name
    char* name_C = new char[name_len+1];
    HDmemset(name_C, 0, name_len+1); // clear buffer

    name_len = H5Lget_name_by_idx(getLocId(), ".", H5_INDEX_NAME, H5_ITER_INC, idx, name_C, name_len+1, H5P_DEFAULT);

    // clean up and return the string
    H5std_string name = H5std_string(name_C);
    delete []name_C;
    return (name);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjnameByIdx
///\brief	Retrieves the name of an object in this group, given the
///		object's index.
///\param	idx  -     IN: Transient index of the object
///\param	name - IN/OUT: Retrieved name of the object
///\param	size -     IN: Length to retrieve
///\return	Actual size of the object name or 0, if object has no name
///\exception	H5::FileIException or H5::GroupIException
///\par Description
///		The value of idx can be any nonnegative number less than the
///		total number of objects in the group, which is returned by
///		the function \c CommonFG::getNumObjs.  Note that this is a
///		transient index; thus, an object may have a different index
///		each time the group is opened.
// Programmer	Binh-Minh Ribler - January, 2003
//--------------------------------------------------------------------------
ssize_t CommonFG::getObjnameByIdx(hsize_t idx, char* name, size_t size) const
{
   ssize_t name_len = H5Lget_name_by_idx(getLocId(), ".", H5_INDEX_NAME, H5_ITER_INC, idx, name, size, H5P_DEFAULT);
   if(name_len < 0)
      throwException("getObjnameByIdx", "H5Lget_name_by_idx failed");

   return (name_len);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjnameByIdx
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - January, 2003
//--------------------------------------------------------------------------
ssize_t CommonFG::getObjnameByIdx(hsize_t idx, H5std_string& name, size_t size) const
{
   char* name_C = new char[size+1]; // temporary C-string for object name
   HDmemset(name_C, 0, size+1); // clear buffer

   ssize_t name_len = getObjnameByIdx(idx, name_C, size+1);
   if(name_len < 0)
      throwException("getObjnameByIdx", "H5Lget_name_by_idx failed");

   name = H5std_string(name_C);
   delete []name_C;
   return (name_len);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::childObjType
///\brief	Returns the type of an object in this file/group, given the
///		object's name.
///\param	objname - IN: Name of the object
///\return	Object type, which can have the following values for group,
///		dataset, and named datatype
///		\li \c H5O_TYPE_GROUP
///		\li \c H5O_TYPE_DATASET
///		\li \c H5O_TYPE_NAMED_DATATYPE
///		Refer to the C API documentation for more details:
///		http://www.hdfgroup.org/HDF5/doc/RM/RM_H5O.html#Object-GetInfo
///\exception	H5::FileIException or H5::GroupIException
///		Exception will be thrown when:
///		- an error returned by the C API
///		- object type is not one of the valid values above
// Programmer	Binh-Minh Ribler - April, 2014
//--------------------------------------------------------------------------
H5O_type_t CommonFG::childObjType(const char* objname) const
{
    H5O_info_t objinfo;
    H5O_type_t objtype = H5O_TYPE_UNKNOWN;

    // Use C API to get information of the object
    herr_t ret_value = H5Oget_info_by_name(getLocId(), objname, &objinfo, H5P_DEFAULT);

    // Throw exception if C API returns failure
    if (ret_value < 0)
	throwException("childObjType", "H5Oget_info_by_name failed");
    // Return a valid type or throw an exception for unknown type
    else
      switch (objinfo.type)
      {
	case H5O_TYPE_GROUP:
	case H5O_TYPE_DATASET:
	case H5O_TYPE_NAMED_DATATYPE:
	    objtype = objinfo.type;
	    break;
	default:
	    throwException("childObjType", "Unknown type of object");
      }
    return(objtype);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::childObjType
///\brief	This is an overloaded member function, provided for convenience.
///		It takes an \a H5std_string for the object's name.
///\brief	Returns the type of an object in this group, given the
///		object's name.
///\param	objname - IN: Name of the object (H5std_string&)
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - April, 2014
//--------------------------------------------------------------------------
H5O_type_t CommonFG::childObjType(const H5std_string& objname) const
{
    // Use overloaded function
    H5O_type_t objtype = childObjType(objname.c_str());
    return(objtype);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::childObjType
///\brief	Returns the type of an object in this file/group, given the
///		object's index and its type and order.
///\param	index - IN: Position of the object
///\param	index_type - IN: Type of the index, default to H5_INDEX_NAME
///\param	order - IN: Traversing order, default to H5_ITER_INC
///\param	objname - IN: Name of the object, default to "."
///\return	Object type, which can have the following values for group,
///		dataset, and named datatype
///		\li \c H5O_TYPE_GROUP
///		\li \c H5O_TYPE_DATASET
///		\li \c H5O_TYPE_NAMED_DATATYPE
///		Refer to the C API documentation for more details:
///		http://www.hdfgroup.org/HDF5/doc/RM/RM_H5O.html#Object-GetInfo
///\exception	H5::FileIException or H5::GroupIException
///		Exception will be thrown when:
///		- an error returned by the C API
///		- object type is not one of the valid values above
// Developer's Notes:
//	- this overload uses H5Oget_info_by_idx instead of H5Oget_info_by_name
//	  like the previous childObjType()
//	- index is the required argument so, first
//	- objname is last because it's more likely the location is already
//	  fully specified
//	- Leave property list out for now because C API is not using it, it
//	  can be added later when needed.
// Programmer	Binh-Minh Ribler - April, 2014
//--------------------------------------------------------------------------
H5O_type_t CommonFG::childObjType(hsize_t index, H5_index_t index_type, H5_iter_order_t order, const char* objname) const
{
    herr_t ret_value;
    H5O_info_t objinfo;
    H5O_type_t objtype = H5O_TYPE_UNKNOWN;

    // Use C API to get information of the object
    ret_value = H5Oget_info_by_idx(getLocId(), objname, index_type, order, index, &objinfo, H5P_DEFAULT);

    // Throw exception if C API returns failure
    if (ret_value < 0)
	throwException("childObjType", "H5Oget_info_by_idx failed");
    // Return a valid type or throw an exception for unknown type
    else
      switch (objinfo.type)
      {
	case H5O_TYPE_GROUP:
	case H5O_TYPE_DATASET:
	case H5O_TYPE_NAMED_DATATYPE:
	    objtype = objinfo.type;
	    break;
	default:
	    throwException("childObjType", "Unknown type of object");
      }
    return(objtype);
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	CommonFG::getObjTypeByIdx
///\brief	Returns the type of an object in this group, given the
///		object's index.
///\param	idx - IN: Transient index of the object
///\return	Object type
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - January, 2003
//--------------------------------------------------------------------------
H5G_obj_t CommonFG::getObjTypeByIdx(hsize_t idx) const
{
   H5G_obj_t obj_type = H5Gget_objtype_by_idx(getLocId(), idx);
   if (obj_type == H5G_UNKNOWN)
      throwException("getObjTypeByIdx", "H5Gget_objtype_by_idx failed");

   return (obj_type);
}

//--------------------------------------------------------------------------
// Function:	CommonFG::getObjTypeByIdx
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function because it also provides
///		the returned object type in text (char*)
///\param	idx       - IN: Transient index of the object
///\param	type_name - OUT: Object type in text
///\return	Object type
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - May, 2010
//--------------------------------------------------------------------------
H5G_obj_t CommonFG::getObjTypeByIdx(hsize_t idx, char* type_name) const
{
   H5G_obj_t obj_type = H5Gget_objtype_by_idx(getLocId(), idx);
   switch (obj_type)
   {
	case H5G_LINK: HDstrcpy(type_name, "symbolic link"); break;
	case H5G_GROUP: HDstrcpy(type_name, "group"); break;
	case H5G_DATASET: HDstrcpy(type_name, "dataset"); break;
	case H5G_TYPE: HDstrcpy(type_name, "datatype"); break;
	case H5G_UNKNOWN:
	default:
	   throwException("getObjTypeByIdx", "H5Gget_objtype_by_idx failed");
   }
   return (obj_type);
}
//--------------------------------------------------------------------------
// Function:	CommonFG::getObjTypeByIdx
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function because it also provides
///		the returned object type in text (H5std_string&)
///\param	idx       - IN: Transient index of the object
///\param	type_name - OUT: Object type in text
///\return	Object type
///\exception	H5::FileIException or H5::GroupIException
// Programmer	Binh-Minh Ribler - January, 2003
//--------------------------------------------------------------------------
H5G_obj_t CommonFG::getObjTypeByIdx(hsize_t idx, H5std_string& type_name) const
{
   H5G_obj_t obj_type = H5Gget_objtype_by_idx(getLocId(), idx);
   switch (obj_type)
   {
	case H5G_LINK: type_name = H5std_string("symbolic link"); break;
	case H5G_GROUP: type_name = H5std_string("group"); break;
	case H5G_DATASET: type_name = H5std_string("dataset"); break;
	case H5G_TYPE: type_name = H5std_string("datatype"); break;
	case H5G_UNKNOWN:
	default:
	   throwException("getObjTypeByIdx", "H5Gget_objtype_by_idx failed");
   }
   return (obj_type);
}
#endif // DOXYGEN_SHOULD_SKIP_THIS
#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	CommonFG default constructor
///\brief	Default constructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
CommonFG::CommonFG() {}

//--------------------------------------------------------------------------
// Function:	CommonFG destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
CommonFG::~CommonFG() {}
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef H5_NO_NAMESPACE
}
#endif

