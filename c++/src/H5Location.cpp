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
#include "H5Location.h"
#include "H5Object.h"
#include "H5DcreatProp.h"
#include "H5DxferProp.h"
#include "H5FaccProp.h"
#include "H5FcreatProp.h"
#include "H5CommonFG.h"
#include "H5DataType.h"
#include "H5DataSpace.h"
#include "H5AbstractDs.h"
#include "H5File.h"
#include "H5DataSet.h"
#include "H5Attribute.h"
#include "H5private.h"		// for HDmemset

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// userAttrOpWrpr simply interfaces between the user's function and the
// C library function H5Aiterate2; used to resolve the different prototype
// problem.  May be moved to Iterator later.
extern "C" herr_t userAttrOpWrpr(hid_t loc_id, const char *attr_name,
    const H5A_info_t *ainfo, void *op_data)
{
   H5std_string s_attr_name = H5std_string( attr_name );
#ifdef NO_STATIC_CAST
   UserData4Aiterate* myData = (UserData4Aiterate *) op_data;
#else
   UserData4Aiterate* myData = static_cast <UserData4Aiterate *> (op_data);
#endif
   myData->op( *myData->location, s_attr_name, myData->opData );
   return 0;
}

//--------------------------------------------------------------------------
// Function:	H5Location default constructor (protected)
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Location::H5Location() : IdComponent() {}

//--------------------------------------------------------------------------
// Function:	H5Location overloaded constructor (protected)
// Purpose	Creates an H5Location object using the id of an existing HDF5
//		object.
// Parameters	object_id - IN: Id of an existing HDF5 object
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Location::H5Location(const hid_t object_id) : IdComponent(object_id) {}

//--------------------------------------------------------------------------
// Function:	H5Location copy constructor
///\brief	Copy constructor: makes a copy of the original H5Location
///		instance.
///\param	original - IN: H5Location instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Location::H5Location( const H5Location& original ) : IdComponent( original ) {}

#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Location::createAttribute
///\brief	Creates an attribute for a group, dataset, or named datatype.
///\param	name - IN: Name of the attribute
///\param	data_type - IN: Datatype for the attribute
///\param	data_space - IN: Dataspace for the attribute - only simple
///		dataspaces are allowed at this time
///\param	create_plist - IN: Creation property list - default to
///		PropList::DEFAULT
///\return	Attribute instance
///\exception	H5::AttributeIException
///\par Description
///		The attribute name specified in \a name must be unique.
///		Attempting to create an attribute with the same name as an
///		existing attribute will raise an exception, leaving the
///		pre-existing attribute intact. To overwrite an existing
///		attribute with a new attribute of the same name, first
///		delete the existing one with \c H5Location::removeAttr, then
///		recreate it with this function.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Location::createAttribute( const char* name, const DataType& data_type, const DataSpace& data_space, const PropList& create_plist ) const
{
   hid_t type_id = data_type.getId();
   hid_t space_id = data_space.getId();
   hid_t plist_id = create_plist.getId();
   hid_t attr_id = H5Acreate2(getId(), name, type_id, space_id, plist_id, H5P_DEFAULT );

   // If the attribute id is valid, create and return the Attribute object
   if( attr_id > 0 )
   {
      Attribute attr( attr_id );
      return( attr );
   }
   else
      throw AttributeIException(inMemFunc("createAttribute"), "H5Acreate2 failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::createAttribute
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Location::createAttribute( const H5std_string& name, const DataType& data_type, const DataSpace& data_space, const PropList& create_plist ) const
{
   return( createAttribute( name.c_str(), data_type, data_space, create_plist ));
}

//--------------------------------------------------------------------------
// Function:	H5Location::openAttribute
///\brief	Opens an attribute given its name.
///\param	name - IN: Name of the attribute
///\return	Attribute instance
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Location::openAttribute( const char* name ) const
{
   hid_t attr_id = H5Aopen(getId(), name, H5P_DEFAULT);
   if( attr_id > 0 )
   {
      Attribute attr( attr_id );
      return( attr );
   }
   else
   {
      throw AttributeIException(inMemFunc("openAttribute"), "H5Aopen failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::openAttribute
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Location::openAttribute( const H5std_string& name ) const
{
   return( openAttribute( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	H5Location::openAttribute
///\brief	Opens an attribute given its index.
///\param	idx - IN: Index of the attribute, a 0-based, non-negative integer
///\return	Attribute instance
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Location::openAttribute( const unsigned int idx ) const
{
   hid_t attr_id = H5Aopen_by_idx(getId(), ".", H5_INDEX_CRT_ORDER,
			H5_ITER_INC, (hsize_t)idx, H5P_DEFAULT, H5P_DEFAULT);
   if( attr_id > 0 )
   {
      Attribute attr( attr_id );
      return( attr );
   }
   else
   {
      throw AttributeIException(inMemFunc("openAttribute"), "H5Aopen_by_idx failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::iterateAttrs
///\brief	Iterates a user's function over all the attributes of an H5
///		object, which may be a group, dataset or named datatype.
///\param	user_op - IN: User's function to operate on each attribute
///\param	_idx - IN/OUT: Starting (IN) and ending (OUT) attribute indices
///\param	op_data - IN: User's data to pass to user's operator function
///\return	Returned value of the last operator if it was non-zero, or
///		zero if all attributes were processed
///\exception	H5::AttributeIException
///\par Description
///		The signature of user_op is
///		void (*)(H5::H5Location&, H5std_string, void*).
///		For information, please refer to the C layer Reference Manual
///		at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Iterate
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int H5Location::iterateAttrs( attr_operator_t user_op, unsigned *_idx, void *op_data )
{
   // store the user's function and data
   UserData4Aiterate* userData = new UserData4Aiterate;
   userData->opData = op_data;
   userData->op = user_op;
   userData->location = this;

   // call the C library routine H5Aiterate2 to iterate the attributes
   hsize_t idx = _idx ? (hsize_t)*_idx : 0;
   int ret_value = H5Aiterate2(getId(), H5_INDEX_NAME, H5_ITER_INC, &idx,
			userAttrOpWrpr, (void *) userData);

   // release memory
   delete userData;

   if( ret_value >= 0 ) {
      /* Pass back update index value to calling code */
      if (_idx)
	 *_idx = (unsigned)idx;

      return( ret_value );
   }
   else  // raise exception when H5Aiterate returns a negative value
      throw AttributeIException(inMemFunc("iterateAttrs"), "H5Aiterate2 failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::getNumAttrs
///\brief	Returns the number of attributes attached to this HDF5 object.
///\return	Number of attributes
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int H5Location::getNumAttrs() const
{
   H5O_info_t oinfo;    /* Object info */

   if(H5Oget_info(getId(), &oinfo) < 0)
      throw AttributeIException(inMemFunc("getNumAttrs"), "H5Oget_info failed");
   else
      return( (int)oinfo.num_attrs );
}

//--------------------------------------------------------------------------
// Function:	H5Location::attrExists
///\brief	Checks whether the named attribute exists at this location.
///\param	name - IN: Name of the attribute to be queried
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2013
//--------------------------------------------------------------------------
bool H5Location::attrExists(const char* name) const
{
   // Call C routine H5Aexists to determine whether an attribute exists
   // at this location, which could be specified by a file, group, dataset,
   // or named datatype.
   herr_t ret_value = H5Aexists(getId(), name);
   if( ret_value > 0 )
      return true;
   else if(ret_value == 0)
      return false;
   else // Raise exception when H5Aexists returns a negative value
      throw AttributeIException(inMemFunc("attrExists"), "H5Aexists failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::attrExists
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool H5Location::attrExists(const H5std_string& name) const
{
   return(attrExists(name.c_str()));
}

//--------------------------------------------------------------------------
// Function:	H5Location::removeAttr
///\brief	Removes the named attribute from this object.
///\param	name - IN: Name of the attribute to be removed
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Location::removeAttr( const char* name ) const
{
   herr_t ret_value = H5Adelete(getId(), name);
   if( ret_value < 0 )
      throw AttributeIException(inMemFunc("removeAttr"), "H5Adelete failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::removeAttr
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Location::removeAttr( const H5std_string& name ) const
{
   removeAttr( name.c_str() );
}

//--------------------------------------------------------------------------
// Function:	H5Location::renameAttr
///\brief	Renames the named attribute from this object.
///\param	oldname - IN: Name of the attribute to be renamed
///\param	newname - IN: New name ame of the attribute
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
void H5Location::renameAttr(const char* oldname, const char* newname) const
{
   herr_t ret_value = H5Arename(getId(), oldname, newname);
   if (ret_value < 0)
      throw AttributeIException(inMemFunc("renameAttr"), "H5Arename failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::renameAttr
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for the names.
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
void H5Location::renameAttr(const H5std_string& oldname, const H5std_string& newname) const
{
   renameAttr (oldname.c_str(), newname.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Location::flush
///\brief	Flushes all buffers associated with a location to disk.
///\param	scope - IN: Specifies the scope of the flushing action,
///		which can be either of these values:
///		\li \c H5F_SCOPE_GLOBAL - Flushes the entire virtual file
///		\li \c H5F_SCOPE_LOCAL - Flushes only the specified file
///\exception	H5::Exception
///\par Description
///		This location is used to identify the file to be flushed.
// Programmer	Binh-Minh Ribler - 2012
// Modification
//	Sep 2012 - BMR
//		Moved from H5File/H5Object
//--------------------------------------------------------------------------
void H5Location::flush(H5F_scope_t scope) const
{
   herr_t ret_value = H5Fflush(getId(), scope);
   if( ret_value < 0 )
   {
      throw LocationException(inMemFunc("flush"), "H5Fflush failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::getFileName
///\brief	Gets the name of the file, in which this HDF5 object belongs.
///\return	File name
///\exception	H5::LocationException
// Programmer	Binh-Minh Ribler - Jul, 2004
//--------------------------------------------------------------------------
H5std_string H5Location::getFileName() const
{
   try {
      return(p_get_file_name());
   }
   catch (LocationException E) {
      throw FileIException(inMemFunc("getFileName"), E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::setComment
///\brief	Sets or resets the comment for an object specified by its name.
///\param	name  - IN: Name of the object
///\param	comment - IN: New comment
///\exception	H5::LocationException
///\par	Description
///		If \a comment is an empty string or a null pointer, the comment
///		message is removed from the object.
///		Comments should be relatively short, null-terminated, ASCII
///		strings.  They can be attached to any object that has an
///		object header, e.g., data sets, groups, named data types,
///		and data spaces, but not symbolic links.
// Programmer	Binh-Minh Ribler - 2000 (moved from CommonFG, Sep 2013)
// Modification
//	2007: QAK modified to use H5O APIs; however the first parameter is
//		no longer just file or group, this function should be moved
//		to another class to accommodate attribute, dataset, and named
//		datatype. - BMR
//--------------------------------------------------------------------------
void H5Location::setComment(const char* name, const char* comment) const
{
   herr_t ret_value = H5Oset_comment_by_name(getId(), name, comment, H5P_DEFAULT);
   if( ret_value < 0 )
      throw LocationException(inMemFunc("setComment"), "H5Oset_comment_by_name failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::setComment
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name and \a comment.
// Programmer	Binh-Minh Ribler - 2000 (moved from CommonFG, Sep 2013)
//--------------------------------------------------------------------------
void H5Location::setComment(const H5std_string& name, const H5std_string& comment) const
{
   setComment(name.c_str(), comment.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Location::setComment
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it doesn't take
///		an object name.
// Programmer	Binh-Minh Ribler - Sep 2013
// Modification
//--------------------------------------------------------------------------
void H5Location::setComment(const char* comment) const
{
   herr_t ret_value = H5Oset_comment_by_name(getId(), ".", comment, H5P_DEFAULT);
   if( ret_value < 0 )
      throw LocationException(inMemFunc("setComment"), "H5Oset_comment_by_name failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::setComment
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a comment.
// Programmer	Binh-Minh Ribler - Sep 2013
//--------------------------------------------------------------------------
void H5Location::setComment(const H5std_string& comment) const
{
   setComment(comment.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Location::removeComment
///\brief	Removes the comment from an object specified by its name.
///\param	name  - IN: Name of the object
///\exception	H5::LocationException
// Programmer	Binh-Minh Ribler - May 2005 (moved from CommonFG, Sep 2013)
//	2007: QAK modified to use H5O APIs; however the first parameter is
//		no longer just file or group, this function should be moved
//		to another class to accommodate attribute, dataset, and named
//		datatype. - BMR
//--------------------------------------------------------------------------
void H5Location::removeComment(const char* name) const
{
   herr_t ret_value = H5Oset_comment_by_name(getId(), name, NULL, H5P_DEFAULT);
   if( ret_value < 0 )
      throw LocationException(inMemFunc("removeComment"), "H5Oset_comment_by_name failed");
}

//--------------------------------------------------------------------------
// Function:	H5Location::removeComment
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - May 2005 (moved from CommonFG, Sep 2013)
//--------------------------------------------------------------------------
void H5Location::removeComment(const H5std_string& name) const
{
   removeComment (name.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Location::getComment
///\brief	Retrieves the comment for this location, returning its length.
///\param	name     - IN: Name of the object
///\param	buf_size - IN: Length of the comment to retrieve
///\param	comment  - OUT: Retrieved comment
///\return	Actual length of the comment
///\exception	H5::LocationException
///\par Description
///		This function retrieves \a buf_size characters of the comment
///		including the null terminator.  Thus, if the actual length
///		of the comment is more than buf_size-1, the retrieved comment
///		will be truncated to accommodate the null terminator.
// Programmer	Binh-Minh Ribler - Mar 2014
//--------------------------------------------------------------------------
ssize_t H5Location::getComment(const char* name, size_t buf_size, char* comment) const
{
    // H5Oget_comment_by_name will get buf_size chars of the comment including
    // the null terminator
    ssize_t comment_len;
    comment_len = H5Oget_comment_by_name(getId(), name, comment, buf_size, H5P_DEFAULT);

    // If H5Oget_comment_by_name returns a negative value, raise an exception
    if (comment_len < 0)
    {
        throw LocationException("H5Location::getComment", "H5Oget_comment_by_name failed");
    }
    // If the comment is longer than the provided buffer size, the C library
    // will not null terminate it
    if ((size_t)comment_len >= buf_size)
	comment[buf_size-1] = '\0';

    // Return the actual comment length, which might be different from buf_size
    return(comment_len);
}

//--------------------------------------------------------------------------
// Function:	H5Location::getComment
///\brief	Returns the comment as \a string for this location,
///		returning its length.
///\param	name     - IN: Name of the object
///\param	buf_size - IN: Length of the comment to retrieve, default to 0
///\return	Comment string
///\exception	H5::LocationException
// Programmer	Binh-Minh Ribler - 2000 (moved from CommonFG, Sep 2013)
//--------------------------------------------------------------------------
H5std_string H5Location::getComment(const char* name, size_t buf_size) const
{
    // Initialize string to "", so that if there is no comment, the returned
    // string will be empty
    H5std_string comment("");

    // Preliminary call to get the comment's length
    ssize_t comment_len = H5Oget_comment_by_name(getId(), name, NULL, (size_t)0, H5P_DEFAULT);

    // If H5Oget_comment_by_name returns a negative value, raise an exception
    if (comment_len < 0)
    {
        throw LocationException("H5Location::getComment", "H5Oget_comment_by_name failed");
    }

    // If comment exists, calls C routine again to get it
    else if (comment_len > 0)
    {
	size_t tmp_len = buf_size;

	// If buffer size is not provided, use comment length
	if (tmp_len == 0)
	    tmp_len = comment_len;

	// Temporary buffer for char* comment
	char* comment_C = new char[tmp_len+1];
	HDmemset(comment_C, 0, tmp_len+1); // clear buffer

	// Used overloaded function
	ssize_t comment_len = getComment(name, tmp_len+1, comment_C);
	if (comment_len < 0)
	{
	    throw LocationException("H5Location::getComment", "H5Oget_comment_by_name failed");
	}

	// Convert the C comment to return
	comment = comment_C;

	// Clean up resource
	delete []comment_C;
    }

    // Return the string comment
    return(comment);
}

//--------------------------------------------------------------------------
// Function:	H5Location::getComment
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000 (moved from CommonFG, Sep 2013)
//--------------------------------------------------------------------------
H5std_string H5Location::getComment(const H5std_string& name, size_t buf_size) const
{
    return(getComment(name.c_str(), buf_size));
}
#ifndef DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Location::p_reference (protected)
// Purpose	Creates a reference to an HDF5 object or a dataset region.
// Parameters
//		name - IN: Name of the object to be referenced
//		dataspace - IN: Dataspace with selection
//		ref_type - IN: Type of reference; default to \c H5R_DATASET_REGION
// Exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Location::p_reference(void* ref, const char* name, hid_t space_id, H5R_type_t ref_type) const
{
   herr_t ret_value = H5Rcreate(ref, getId(), name, ref_type, space_id);
   if (ret_value < 0)
   {
      throw ReferenceException(inMemFunc("reference"), "H5Rcreate failed");
   }
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Location::reference
///\brief	Creates a reference to an HDF5 object or a dataset region.
///\param	ref - IN: Reference pointer
///\param	name - IN: Name of the object to be referenced
///\param	dataspace - IN: Dataspace with selection
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference.
///		\li \c H5R_DATASET_REGION - Reference is a dataset region
///			reference. (default)
///\exception	H5::ReferenceException
///\note	This method is more suitable for a dataset region reference.
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Location::reference(void* ref, const char* name, const DataSpace& dataspace, H5R_type_t ref_type) const
{
   try {
      p_reference(ref, name, dataspace.getId(), ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException(inMemFunc("reference"), E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::reference
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for \a name.
///\param	ref - IN: Reference pointer
///\param	name - IN: Name of the object to be referenced
///\param	dataspace - IN: Dataspace with selection
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference.
///		\li \c H5R_DATASET_REGION - Reference is a dataset region
///			reference. (default)
///\exception	H5::ReferenceException
///\note	This method is more suitable for a dataset region reference.
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Location::reference(void* ref, const H5std_string& name, const DataSpace& dataspace, H5R_type_t ref_type) const
{
   try {
      p_reference(ref, name.c_str(), dataspace.getId(), ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException(inMemFunc("reference"), E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::reference
///\brief	This is an overloaded function, provided for your convenience.
///		It differs from the above function in that it does not take
///		a DataSpace object and the reference type must be specified.
///\param	ref - IN: Reference pointer
///\param	name - IN: Name of the object to be referenced
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference (default)
///		\li \c H5R_DATASET_REGION - Reference is a dataset region
///\exception	H5::ReferenceException
///\note	This method is more suitable for an object reference.
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Location::reference(void* ref, const char* name, H5R_type_t ref_type) const
{
   try {
      p_reference(ref, name, -1, ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException(inMemFunc("reference"), E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	H5Location::reference
///\brief	This is an overloaded function, provided for your convenience.
///		It differs from the above function in that it takes an
///		\c H5std_string for the object's name.
///\param	ref - IN: Reference pointer
///\param	name - IN: Name of the object to be referenced - \c H5std_string
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference (default)
///		\li \c H5R_DATASET_REGION - Reference is a dataset region
///\note	This method is more suitable for an object reference.
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Location::reference(void* ref, const H5std_string& name, H5R_type_t ref_type) const
{
   reference(ref, name.c_str(), ref_type);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	H5Location::p_dereference (protected)
// Purpose	Dereference a ref into an hdf5 object.
// Parameters
//		loc_id - IN: An hdf5 identifier specifying the location of the
//			 referenced object
//		ref - IN: Reference pointer
//		ref_type - IN: Reference type
// Exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
// Modification
//	May 2008 - BMR
//		Moved from IdComponent.
//--------------------------------------------------------------------------
hid_t H5Location::p_dereference(hid_t loc_id, const void* ref, H5R_type_t ref_type, const char* from_func)
{
   hid_t temp_id = H5Rdereference(loc_id, ref_type, ref);
   if (temp_id < 0)
   {
      throw ReferenceException(inMemFunc(from_func), "H5Rdereference failed");
   }

   return(temp_id);
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Location::dereference
///\brief	Dereferences a reference into an HDF5 object, given an HDF5 object.
///\param	loc - IN: Location of the referenced object
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type
///\param	plist - IN: Property list - default to PropList::DEFAULT
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
// Modification
//	May, 2008
//		Corrected missing parameters. - BMR
//--------------------------------------------------------------------------
void H5Location::dereference(const H5Location& loc, const void* ref, H5R_type_t ref_type)
{
   p_setId(p_dereference(loc.getId(), ref, ref_type, "dereference"));
}

//--------------------------------------------------------------------------
// Function:	H5Location::dereference
///\brief	Dereferences a reference into an HDF5 object, given an attribute.
///\param	attr - IN: Attribute specifying the location of the referenced object
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type
///\param	plist - IN: Property list - default to PropList::DEFAULT
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
// Modification
//	May, 2008
//		Corrected missing parameters. - BMR
//--------------------------------------------------------------------------
void H5Location::dereference(const Attribute& attr, const void* ref, H5R_type_t ref_type)
{
   p_setId(p_dereference(attr.getId(), ref, ref_type, "dereference"));
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:	H5Location::getObjType
///\brief	Retrieves the type of object that an object reference points to.
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT - Reference is an object reference.
///		\li \c H5R_DATASET_REGION - Reference is a dataset region reference.
///\param	ref      - IN: Reference to query
///\return	An object type, which can be one of the following:
///		\li \c H5G_UNKNOWN  - A failure occurs. (-1)
///		\li \c H5G_GROUP  - Object is a group.
///		\li \c H5G_DATASET - Object is a dataset.
///		\li \c H5G_TYPE Object - is a named datatype
///		\li \c H5G_LINK  - Object is a symbolic link.
///		\li \c H5G_UDLINK  - Object is a user-defined link.
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
// Modification
//	Sep 2012: Moved up from H5File, Group, DataSet, and DataType
//--------------------------------------------------------------------------
H5G_obj_t H5Location::getObjType(void *ref, H5R_type_t ref_type) const
{
   try {
      return(p_get_obj_type(ref, ref_type));
   }
   catch (ReferenceException E) {
      throw ReferenceException(inMemFunc("getObjType"), E.getDetailMsg());
   }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	H5Location::p_get_obj_type (protected)
// Purpose	Retrieves the type of object that an object reference points to.
// Parameters
//		ref      - IN: Reference to query
//		ref_type - IN: Type of reference to query
// Return	An object type, which can be one of the following:
//			H5G_UNKNOWN \tFailure occurs (-1)
//			H5G_GROUP \tObject is a group.
//			H5G_DATASET \tObject is a dataset.
//			H5G_TYPE Object \tis a named datatype.
//			H5G_LINK \tObject is a symbolic link.
//			H5G_UDLINK \tObject is a user-defined link.
// Exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
H5G_obj_t H5Location::p_get_obj_type(void *ref, H5R_type_t ref_type) const
{
   H5G_obj_t obj_type = H5Rget_obj_type1(getId(), ref_type, ref);

   if (obj_type == H5G_UNKNOWN)
   {
      throw ReferenceException(inMemFunc("getObjType"), "H5Rget_obj_type1 failed");
   }
   return(obj_type);
}
#endif // DOXYGEN_SHOULD_SKIP_THIS
#endif /* H5_NO_DEPRECATED_SYMBOLS */

//--------------------------------------------------------------------------
// Function:	H5Location::getRefObjType
///\brief	Retrieves the type of object that an object reference points to.
///\param	ref      - IN: Reference to query
///\param	ref_type - IN: Type of reference to query, valid values are:
///		\li \c H5R_OBJECT         - Reference is an object reference.
///		\li \c H5R_DATASET_REGION - Reference is a dataset region reference.
///\return	An object type, which can be one of the following:
///		\li \c H5O_TYPE_UNKNOWN	- Unknown object type (-1)
///		\li \c H5O_TYPE_GROUP	- Object is a group
///		\li \c H5O_TYPE_DATASET	- Object is a dataset
///		\li \c H5O_TYPE_NAMED_DATATYPE - Object is a named datatype
///		\li \c H5O_TYPE_NTYPES	- Number of different object types
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
H5O_type_t H5Location::getRefObjType(void *ref, H5R_type_t ref_type) const
{
   try {
      return(p_get_ref_obj_type(ref, ref_type));
   }
   catch (ReferenceException E) {
      throw ReferenceException(inMemFunc("getRefObjType"), E.getDetailMsg());
   }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	H5Location::p_get_ref_obj_type (protected)
// Purpose	Retrieves the type of object that an object reference points to.
// Parameters
//		ref      - IN: Reference to query
//		ref_type - IN: Type of reference to query
// Return	An object type, which can be one of the following:
//			H5O_TYPE_UNKNOWN	- Unknown object type (-1)
//			H5O_TYPE_GROUP		- Object is a group
//			H5O_TYPE_DATASET	- Object is a dataset
//			H5O_TYPE_NAMED_DATATYPE - Object is a named datatype
//			H5O_TYPE_NTYPES		- Number of object types
// Exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
H5O_type_t H5Location::p_get_ref_obj_type(void *ref, H5R_type_t ref_type) const
{
   H5O_type_t obj_type = H5O_TYPE_UNKNOWN;
   herr_t ret_value = H5Rget_obj_type2(getId(), ref_type, ref, &obj_type);
   if (ret_value < 0)
   {
      throw ReferenceException(inMemFunc("getRefObjType"), "H5Rget_obj_type2 failed");
   }
   if (obj_type == H5O_TYPE_UNKNOWN || obj_type >= H5O_TYPE_NTYPES)
   {
      throw ReferenceException(inMemFunc("getRefObjType"), "H5Rget_obj_type2 returned invalid type");
   }
   return(obj_type);
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Location::getRegion
///\brief	Retrieves a dataspace with the region pointed to selected.
///\param	ref	 - IN: Reference to get region of
///\param	ref_type - IN: Type of reference to get region of - default
//				to H5R_DATASET_REGION
///\return	DataSpace object
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
DataSpace H5Location::getRegion(void *ref, H5R_type_t ref_type) const
{
   hid_t space_id = H5Rget_region(getId(), ref_type, ref);
   if (space_id < 0)
   {
      throw ReferenceException(inMemFunc("getRegion"), "H5Rget_region failed");
   }
   try {
      DataSpace dataspace(space_id);
      return(dataspace);
   }
   catch (DataSpaceIException E) {
      throw ReferenceException(inMemFunc("getRegion"), E.getDetailMsg());
   }
}


//--------------------------------------------------------------------------
// Function:	H5Location destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Location::~H5Location() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
