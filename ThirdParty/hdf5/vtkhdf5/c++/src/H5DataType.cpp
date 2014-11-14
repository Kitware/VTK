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
#include "H5DataSpace.h"
#include "H5Object.h"
#include "H5FaccProp.h"
#include "H5FcreatProp.h"
#include "H5DcreatProp.h"
#include "H5DxferProp.h"
#include "H5CommonFG.h"
#include "H5DataType.h"
#include "H5AtomType.h"
#include "H5PredType.h"
#include "H5private.h"
#include "H5AbstractDs.h"
#include "H5DataSet.h"
#include "H5File.h"
#include "H5Attribute.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#ifndef H5_NO_STD
    using std::cerr;
    using std::endl;
#endif  // H5_NO_STD
#endif

//--------------------------------------------------------------------------
// Function:	DataType default constructor
///\brief	Default constructor: Creates a stub datatype
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType::DataType() : H5Object(), id(0) {}

//--------------------------------------------------------------------------
// Function:	DataType overloaded constructor
///\brief	Creates a datatype using an existing datatype's id
///\param	existing_id - IN: Id of the existing datatype
// Description
//		Constructor creates a copy of an existing DataType using
//		its id.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//	Dec, 2005
//		Removed second argument, "predefined", after changing to the
//		new ref counting mechanism that relies on C's ref counting.
//--------------------------------------------------------------------------
DataType::DataType(const hid_t existing_id) : H5Object()
{
    id = existing_id;
}

//--------------------------------------------------------------------------
// Function:	DataType overloaded constructor
///\brief	Creates a object given its class and size
///\param	type_class - IN: Class of datatype to create
///\param	size       - IN: Number of bytes in the datatype to create
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType::DataType( const H5T_class_t type_class, size_t size ) : H5Object()
{
   // Call C routine to create the new datatype
   id = H5Tcreate( type_class, size );
   if( id < 0 )
   {
      throw DataTypeIException("DataType constructor", "H5Tcreate failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType overload constructor - dereference
///\brief	Given a reference, ref, to an hdf5 group, creates a
///		DataType object
///\param       loc - IN: Location referenced object is in
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type - default to H5R_OBJECT
///\param	plist - IN: Property list - default to PropList::DEFAULT
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
// Modification
//	Jul, 2008
//		Added for application convenience.
//--------------------------------------------------------------------------
DataType::DataType(const H5Location& loc, const void* ref, H5R_type_t ref_type) : H5Object(), id(0)
{
    id = H5Location::p_dereference(loc.getId(), ref, ref_type, "constructor - by dereference");
}

//--------------------------------------------------------------------------
// Function:	DataType overload constructor - dereference
///\brief	Given a reference, ref, to an hdf5 group, creates a
///		DataType object
///\param       attr - IN: Specifying location where the referenced object is in
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type - default to H5R_OBJECT
///\param	plist - IN: Property list - default to PropList::DEFAULT
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
// Modification
//	Jul, 2008
//		Added for application convenience.
//--------------------------------------------------------------------------
DataType::DataType(const Attribute& attr, const void* ref, H5R_type_t ref_type) : H5Object(), id(0)
{
    id = H5Location::p_dereference(attr.getId(), ref, ref_type, "constructor - by dereference");
}

//--------------------------------------------------------------------------
// Function:	DataType copy constructor
///\brief	Copy constructor: makes a copy of the original DataType object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType::DataType(const DataType& original) : H5Object(original)
{
    id = original.getId();
    incRefCount(); // increment number of references to this id
}

//--------------------------------------------------------------------------
// Function:	DataType::copy
///\brief	Copies an existing datatype to this datatype object
///\param	like_type - IN: Datatype to be copied
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Jun 1, 2004
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
void DataType::copy( const DataType& like_type )
{
    // close the current data type before copying like_type to this object
    try {
	close();
    }
    catch (Exception close_error) {
	throw DataTypeIException(inMemFunc("copy"), close_error.getDetailMsg());
    }

    // call C routine to copy the datatype
    id = H5Tcopy( like_type.getId() );
    if( id < 0 )
	throw DataTypeIException(inMemFunc("copy"), "H5Tcopy failed");
}

//--------------------------------------------------------------------------
// Function:	DataType::copy
///\brief	Copies the datatype of the given dataset to this datatype object
///\param	dset - IN: Dataset
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Jan, 2007
///\par Description
///		The resulted dataset will be transient and modifiable.
//--------------------------------------------------------------------------
void DataType::copy(const DataSet& dset)
{
    // close the current data type before copying dset's datatype to this object
    try {
	close();
    }
    catch (Exception close_error) {
	throw DataTypeIException(inMemFunc("copy"), close_error.getDetailMsg());
    }

    // call C routine to copy the datatype
    id = H5Tcopy( dset.getId() );
    if( id < 0 )
	throw DataTypeIException(inMemFunc("copy"), "H5Tcopy failed");
}

//--------------------------------------------------------------------------
// Function:	DataType::operator=
///\brief	Assignment operator
///\param	rhs - IN: Reference to the existing datatype
///\return	Reference to DataType instance
///\exception	H5::DataTypeIException
// Description
// 		Makes a copy of the type on the right hand side and stores
//		the new id in the left hand side object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType& DataType::operator=( const DataType& rhs )
{
    if (this != &rhs)
	copy(rhs);
    return(*this);
}

//--------------------------------------------------------------------------
// Function:	DataType::operator==
///\brief	Compares this DataType against the given one to determines
///		whether the two objects refer to the same actual datatype.
///\param	compared_type - IN: Reference to the datatype to compare
///\return	true if the datatypes are equal, and false, otherwise.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DataType::operator==(const DataType& compared_type ) const
{
   // Call C routine H5Tequal to determines whether two datatype
   // identifiers refer to the same datatype
   htri_t ret_value = H5Tequal( id, compared_type.getId() );
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else
   {
      throw DataTypeIException(inMemFunc("operator=="), "H5Tequal returns negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::p_commit (private)
//\brief	Commits a transient datatype to a file, creating a new
//		named datatype
//\param	loc_id - IN: The id of either a file, group, dataset, named
//			 datatype, or attribute.
//\param	name - IN: Name of the datatype
//\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
// Modification:
//		Copied from DataType::commit and made into private function
//		to be commonly used by several overloads of DataType::commit.
//		BMR - Jan, 2007
//--------------------------------------------------------------------------
void DataType::p_commit(hid_t loc_id, const char* name)
{
   // Call C routine to commit the transient datatype
   herr_t ret_value = H5Tcommit2(loc_id, name, id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
   if( ret_value < 0 )
      throw DataTypeIException(inMemFunc("p_commit"), "H5Tcommit2 failed");
}

//--------------------------------------------------------------------------
// Function:	DataType::commit
///\brief	Commits a transient datatype to a file, creating a new
///		named datatype
///\param	loc - IN: A location (file, dataset, datatype, or group)
///\param	name - IN: Name of the datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::commit(const H5Location& loc, const char* name)
{
   p_commit(loc.getId(), name);
}

//--------------------------------------------------------------------------
// Function:	DataType::commit
///\brief	This is an overloaded member function, kept for backward
///		compatibility.  It differs from the above function in that it
///		misses const's.  This wrapper will be removed in future release.
///\param	loc - IN: A location (file, dataset, datatype, or group)
///\param	name - IN: Name of the datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Jan, 2007
//--------------------------------------------------------------------------
void DataType::commit(H5Location& loc, const char* name)
{
   p_commit(loc.getId(), name);
}

//--------------------------------------------------------------------------
// Function:	DataType::commit
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of the
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::commit(const H5Location& loc, const H5std_string& name)
{
   p_commit(loc.getId(), name.c_str());
}

//--------------------------------------------------------------------------
// Function:	DataType::commit
///\brief	This is an overloaded member function, kept for backward
///		compatibility.  It differs from the above function in that it
///		misses const's.  This wrapper will be removed in future release.
///\param	loc - IN: A location (file, dataset, datatype, or group)
///\param	name - IN: Name of the datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Jan, 2007
//--------------------------------------------------------------------------
void DataType::commit(H5Location& loc, const H5std_string& name)
{
   p_commit(loc.getId(), name.c_str());
}

//--------------------------------------------------------------------------
// Function:	DataType::committed
///\brief	Determines whether a datatype is a named type or a
///		transient type.
///\return	\c true if the datatype is a named type, and \c false,
///		otherwise.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DataType::committed() const
{
   // Call C function to determine if a datatype is a named one
   htri_t committed = H5Tcommitted( id );
   if( committed > 0 )
      return true;
   else if( committed == 0 )
      return false;
   else
   {
      throw DataTypeIException(inMemFunc("committed"), "H5Tcommitted return negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::find
///\brief	Finds a conversion function that can handle a conversion
///		from this datatype to the specified datatype, \a dest.
///\param	dest   - IN: Destination datatype
///\param	pcdata - IN: Pointer to type conversion data
///\return	Pointer to a suitable conversion function
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_conv_t DataType::find( const DataType& dest, H5T_cdata_t **pcdata ) const
{
   // Call C routine to find the conversion function
   H5T_conv_t func = H5Tfind( id, dest.getId(), pcdata );
   if( func == NULL )
   {
      throw DataTypeIException(inMemFunc("find"), "H5Tfind returns a NULL function");
   }
   return( func );
}

//--------------------------------------------------------------------------
// Function:	DataType::convert
///\brief	Converts data from this datatype to the specified datatypes.
///\param	dest       - IN: Destination datatype
///\param	nelmts     - IN: Size of array \a buf
///\param	buf        - IN/OUT: Array containing pre- and post-conversion
///				values
///\param	background - IN: Optional backgroud buffer
///\param	plist - IN: Property list - default to PropList::DEFAULT
///\return	Pointer to a suitable conversion function
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::convert( const DataType& dest, size_t nelmts, void *buf, void *background, const PropList& plist ) const
{
   // Get identifiers for C API
   hid_t dest_id = dest.getId();
   hid_t plist_id = plist.getId();

   // Call C routine H5Tconvert to convert the data
   herr_t ret_value;
   ret_value = H5Tconvert( id, dest_id, nelmts, buf, background, plist_id );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("convert"), "H5Tconvert failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::lock
///\brief	Locks a datatype, making it read-only and non-destructible.
///
///\exception	H5::DataTypeIException
///\par Descrition
///		This is normally done by the library for predefined data
///		types so the application doesn't inadvertently change or
///		delete a predefined type.
///
///		Once a data type is locked it can never be unlocked unless
///		the entire library is closed.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::lock() const
{
   // Call C routine to lock the datatype
   herr_t ret_value = H5Tlock( id );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("lock"), "H5Tlock failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::getClass
///\brief	Returns the datatype class identifier.
///\return	Datatype class identifier
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_class_t DataType::getClass() const
{
   H5T_class_t type_class = H5Tget_class( id );

   // Return datatype class identifier if successful
   if( type_class == H5T_NO_CLASS )
   {
      throw DataTypeIException(inMemFunc("getClass"), "H5Tget_class returns H5T_NO_CLASS");
   }
   return( type_class );
}

//--------------------------------------------------------------------------
// Function:	DataType::getSize
///\brief	Returns the size of a datatype.
///\return	Datatype size in bytes
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
size_t DataType::getSize() const
{
   // Call C routine to get the datatype size
   size_t type_size = H5Tget_size( id );
   if( type_size <= 0 ) // valid data types are never zero size
   {
      throw DataTypeIException(inMemFunc("getSize"), "H5Tget_size returns invalid datatype size");
   }
   return( type_size );
}

//--------------------------------------------------------------------------
// Function:	DataType::getSuper
///\brief	Returns the base datatype from which a datatype is derived.
///\return	DataType object
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType DataType::getSuper() const
{
   // Call C routine to get the base datatype from which the specified
   // datatype is derived.
   hid_t base_type_id = H5Tget_super( id );

   // If H5Tget_super returns a valid datatype id, create and return
   // the base type, otherwise, raise exception
   if( base_type_id > 0 )
   {
      DataType base_type( base_type_id );
      return( base_type );
   }
   else
   {
      throw DataTypeIException(inMemFunc("getSuper"), "H5Tget_super failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::registerFunc
///\brief	Registers the specified conversion function.
///\param	pers - IN: Conversion option
///			\li \c H5T_PERS_HARD for hard conversion functions
///			\li \c H5T_PERS_SOFT for soft conversion functions.
///\param	name - IN: Name displayed in diagnostic output.
///\param	dest - IN: Destination datatype.
///\param	func - IN: Function to convert between source and
///		destination datatypes.
///\exception	H5::DataTypeIException
///\par Description
///		For more information, please see:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-Register
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::registerFunc( H5T_pers_t pers, const char* name, const DataType& dest, H5T_conv_t func ) const
{
   hid_t dest_id = dest.getId();  // get id of the destination datatype

   // Call C routine H5Tregister to register the conversion function
   herr_t ret_value = H5Tregister( pers, name, id, dest_id, func );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("registerFunc"), "H5Tregister failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::registerFunc
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of the
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::registerFunc( H5T_pers_t pers, const H5std_string& name, const DataType& dest, H5T_conv_t func ) const
{
   registerFunc( pers, name.c_str(), dest, func );
}

//--------------------------------------------------------------------------
// Function:	DataType::unregister
///\brief	Removes a conversion function from all conversion paths.
///\param	pers - IN: Conversion option
///			\li \c H5T_PERS_HARD for hard conversion functions
///			\li \c H5T_PERS_SOFT for soft conversion functions.
///\param	name - IN: Name displayed in diagnostic output.
///\param	dest - IN: Destination datatype.
///\param	func - IN: Function to convert between source and
///		destination datatypes.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::unregister( H5T_pers_t pers, const char* name, const DataType& dest, H5T_conv_t func ) const
{
   hid_t dest_id = dest.getId();  // get id of the dest datatype for C API

   // Call C routine H5Tunregister to remove the conversion function
   herr_t ret_value = H5Tunregister( pers, name, id, dest_id, func );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("unregister"), "H5Tunregister failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::unregister
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of the
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::unregister( H5T_pers_t pers, const H5std_string& name, const DataType& dest, H5T_conv_t func ) const
{
   unregister( pers, name.c_str(), dest, func );
}

//--------------------------------------------------------------------------
// Function:	DataType::setTag
///\brief	Tags an opaque datatype.
///\param	tag - IN: Descriptive ASCII string with which the opaque
///		datatype is to be tagged.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::setTag( const char* tag ) const
{
   // Call C routine H5Tset_tag to tag an opaque datatype.
   herr_t ret_value = H5Tset_tag( id, tag );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setTag"), "H5Tset_tag failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::setTag
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of the
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataType::setTag( const H5std_string& tag ) const
{
   setTag( tag.c_str());
}

//--------------------------------------------------------------------------
// Function:	DataType::getTag
///\brief	Gets the tag associated with an opaque datatype.
///\return	Tag associated with the opaque datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string DataType::getTag() const
{
    char* tag_Cstr = H5Tget_tag( id );

    // if the tag C-string returned is not NULL, convert it to C++ string
    // and return it, otherwise, raise an exception
    if( tag_Cstr != NULL )
    {
	H5std_string tag = H5std_string(tag_Cstr); // C string to string object
	H5free_memory(tag_Cstr); // free the C string
	return (tag); // return the tag
    }
    else
    {
	throw DataTypeIException(inMemFunc("getTag"), "H5Tget_tag returns NULL for tag");
    }
}

//--------------------------------------------------------------------------
// Function:	DataType::detectClass
///\brief	Checks whether a datatype contains (or is) a certain type of
///		datatype.
///\return	true if this datatype contains or is the specified type,
///		and false, otherwise.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
bool DataType::detectClass(H5T_class_t cls) const
{
   htri_t ret_value = H5Tdetect_class(id, cls);
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else
   {
      throw DataTypeIException(inMemFunc("detectClass"),
				"H5Tdetect_class returns negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DataType::isVariableStr
///\brief	Check whether this datatype is a variable-length string.
///\return	true if this datatype is a variable-length string, and
///		false, otherwise.
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
bool DataType::isVariableStr() const
{
   htri_t is_varlen_str = H5Tis_variable_str(id);
   if( is_varlen_str == 1 )
      return true;
   else if( is_varlen_str == 0 )
      return false;
   else
   {
      throw DataTypeIException(inMemFunc("isVariableStr"),
				"H5Tis_variable_str returns negative value");
   }
}

//--------------------------------------------------------------------------
// Function:    DataType::getId
///\brief       Get the id of this datatype
///\return      Datatype identifier
// Modification:
//      May 2008 - BMR
//              Class hierarchy is revised to address bugzilla 1068.  Class
//              AbstractDS and Attribute are moved out of H5Object.  In
//              addition, member IdComponent::id is moved into subclasses, and
//              IdComponent::getId now becomes pure virtual function.
// Programmer   Binh-Minh Ribler - May, 2008
//--------------------------------------------------------------------------
hid_t DataType::getId() const
{
   return(id);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:    DataType::p_setId
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
void DataType::p_setId(const hid_t new_id)
{
    // handling references to this old id
    try {
        close();
    }
    catch (Exception close_error) {
        throw DataTypeIException(inMemFunc("p_setId"), close_error.getDetailMsg());
    }
   // reset object's id to the given id
   id = new_id;
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	DataType::close
///\brief	Closes the datatype if it is not a predefined type.
///
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Mar 9, 2005
//--------------------------------------------------------------------------
void DataType::close()
{
    if (p_valid_id(id))
    {
	herr_t ret_value = H5Tclose(id);
	if( ret_value < 0 )
	{
	    throw DataTypeIException(inMemFunc("close"), "H5Tclose failed");
	}
	// reset the id
	id = 0;
    }
}

//--------------------------------------------------------------------------
// Function:	DataType destructor
///\brief	Properly terminates access to this datatype.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		  library ID reference counting mechanism - BMR, Jun 1, 2004
//		- Replaced decRefCount with close() to let the C library
//		  handle the reference counting - BMR, Jun 1, 2006
//		- Added the use of H5CPP_EXITED to terminate the HDF5 library
//		  and elimiate previous memory leaks.  See comments in the
//		  header file "H5PredType.h" for details. - BMR, Mar 30, 2012
//--------------------------------------------------------------------------
DataType::~DataType()
{
    try
    {
	/* If this is the object AtExit, terminate the HDF5 library.  This is
	   to eliminate memory leaks due to the library being re-initiated
	   (after the program has ended) and not re-terminated. */
	if (id == H5CPP_EXITED)
	{
	    herr_t ret_value = H5close();
	    if (ret_value == FAIL)
		throw DataTypeIException(inMemFunc("~DataType - "), "H5close failed");
	}
	// Close the HDF5 datatype
	else
	    close();
    }
    catch (Exception close_error) {
	cerr << inMemFunc("~DataType - ") << close_error.getDetailMsg() << endl;
    }
}
#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
