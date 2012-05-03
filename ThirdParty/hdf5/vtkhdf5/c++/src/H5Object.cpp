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
   myData->op( *myData->object, s_attr_name, myData->opData );
   return 0;
}

//--------------------------------------------------------------------------
// Function:	H5Object default constructor (protected)
// Description
//		The id is set by IdComponent() but subclass constructor will
//		set it to a valid HDF5 id.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object() : IdComponent(0) {}

//--------------------------------------------------------------------------
// Function:	H5Object overloaded constructor (protected)
// Purpose	Creates an H5Object object using the id of an existing HDF5
// 		object.
// Parameters	object_id - IN: Id of an existing HDF5 object
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object( const hid_t object_id ) : IdComponent( object_id ) {}

#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Object copy constructor
///\brief	Copy constructor: makes a copy of the original H5Object
///		instance.
///\param	original - IN: H5Object instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object( const H5Object& original ) : IdComponent( original ) {}

//--------------------------------------------------------------------------
// Function:	H5Object::createAttribute
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
///		delete the existing one with \c H5Object::removeAttr, then
///		recreate it with this function.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Object::createAttribute( const char* name, const DataType& data_type, const DataSpace& data_space, const PropList& create_plist ) const
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
// Function:	H5Object::createAttribute
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Object::createAttribute( const H5std_string& name, const DataType& data_type, const DataSpace& data_space, const PropList& create_plist ) const
{
   return( createAttribute( name.c_str(), data_type, data_space, create_plist ));
}

//--------------------------------------------------------------------------
// Function:	H5Object::openAttribute
///\brief	Opens an attribute given its name.
///\param	name - IN: Name of the attribute
///\return	Attribute instance
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Object::openAttribute( const char* name ) const
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
// Function:	H5Object::openAttribute
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Object::openAttribute( const H5std_string& name ) const
{
   return( openAttribute( name.c_str()) );
}

//--------------------------------------------------------------------------
// Function:	H5Object::openAttribute
///\brief	Opens an attribute given its index.
///\param	idx - IN: Index of the attribute, a 0-based, non-negative integer
///\return	Attribute instance
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Attribute H5Object::openAttribute( const unsigned int idx ) const
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
// Function:	H5Object::iterateAttrs
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
///		void (*)(H5::H5Object&, std::string, void*).
///		For information, please refer to the C layer Reference Manual
///		at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Iterate
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int H5Object::iterateAttrs( attr_operator_t user_op, unsigned *_idx, void *op_data )
{
   // store the user's function and data
   UserData4Aiterate* userData = new UserData4Aiterate;
   userData->opData = op_data;
   userData->op = user_op;
   userData->object = this;

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
// Function:	H5Object::getNumAttrs
///\brief	Returns the number of attributes attached to this HDF5 object.
///\return	Number of attributes
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int H5Object::getNumAttrs() const
{
   H5O_info_t oinfo;    /* Object info */

   if(H5Oget_info(getId(), &oinfo) < 0)
      throw AttributeIException(inMemFunc("getNumAttrs"), "H5Oget_info failed");
   else
      return( (int)oinfo.num_attrs );
}

//--------------------------------------------------------------------------
// Function:	H5Object::removeAttr
///\brief	Removes the named attribute from this object.
///\param	name - IN: Name of the attribute to be removed
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Object::removeAttr( const char* name ) const
{
   herr_t ret_value = H5Adelete(getId(), name);
   if( ret_value < 0 )
      throw AttributeIException(inMemFunc("removeAttr"), "H5Adelete failed");
}

//--------------------------------------------------------------------------
// Function:	H5Object::removeAttr
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Object::removeAttr( const H5std_string& name ) const
{
   removeAttr( name.c_str() );
}

//--------------------------------------------------------------------------
// Function:	H5Object::renameAttr
///\brief	Renames the named attribute from this object.
///\param	oldname - IN: Name of the attribute to be renamed
///\param	newname - IN: New name ame of the attribute
///\exception	H5::AttributeIException
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
void H5Object::renameAttr(const char* oldname, const char* newname) const
{
   herr_t ret_value = H5Arename(getId(), oldname, newname);
   if (ret_value < 0)
      throw AttributeIException(inMemFunc("renameAttr"), "H5Arename failed");
}

//--------------------------------------------------------------------------
// Function:	H5Object::renameAttr
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function in that it takes
///		a reference to an \c H5std_string for the names.
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
void H5Object::renameAttr(const H5std_string& oldname, const H5std_string& newname) const
{
   renameAttr (oldname.c_str(), newname.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Object::flush
///\brief	Flushes all buffers associated with a file to disk.
///\param	scope - IN: Specifies the scope of the flushing action,
///		which can be either of these values:
///		\li \c H5F_SCOPE_GLOBAL - Flushes the entire virtual file
///		\li \c H5F_SCOPE_LOCAL - Flushes only the specified file
///\exception	H5::AttributeIException
///\par Description
///		This object is used to identify the file to be flushed.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Object::flush(H5F_scope_t scope) const
{
   herr_t ret_value = H5Fflush(getId(), scope);
   if( ret_value < 0 )
   {
      throw FileIException(inMemFunc("flush"), "H5Fflush failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Object::getFileName
///\brief	Gets the name of the file, in which this HDF5 object belongs.
///\return	File name
///\exception	H5::IdComponentException
// Programmer	Binh-Minh Ribler - Jul, 2004
//--------------------------------------------------------------------------
H5std_string H5Object::getFileName() const
{
   try {
      return(p_get_file_name());
   }
   catch (IdComponentException E) {
      throw FileIException(inMemFunc("getFileName"), E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:    H5Object::p_reference (protected)
// Purpose      Creates a reference to an HDF5 object or a dataset region.
// Parameters
//              name - IN: Name of the object to be referenced
//              dataspace - IN: Dataspace with selection
//              ref_type - IN: Type of reference; default to \c H5R_DATASET_REGION
// Exception    H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Object::p_reference(void* ref, const char* name, hid_t space_id, H5R_type_t ref_type) const
{
   herr_t ret_value = H5Rcreate(ref, getId(), name, ref_type, space_id);
   if (ret_value < 0)
   {
      throw IdComponentException("", "H5Rcreate failed");
   }
}

//--------------------------------------------------------------------------
// Function:    H5Object::reference
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
void H5Object::reference(void* ref, const char* name, const DataSpace& dataspace, H5R_type_t ref_type) const
{
   try {
      p_reference(ref, name, dataspace.getId(), ref_type);
   }
   catch (IdComponentException E) {
      throw IdComponentException("H5Object::reference", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:    H5Object::reference
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
void H5Object::reference(void* ref, const char* name) const
{
   try {
      p_reference(ref, name, -1, H5R_OBJECT);
   }
   catch (IdComponentException E) {
      throw IdComponentException("H5Object::reference", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:    H5Object::reference
///\brief       This is an overloaded function, provided for your convenience.
///             It differs from the above function in that it takes an
///             \c H5std_string for the object's name.
///\param       ref - IN: Reference pointer
///\param       name - IN: Name of the object to be referenced - \c H5std_string
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Object::reference(void* ref, const H5std_string& name) const
{
   reference(ref, name.c_str());
}

//--------------------------------------------------------------------------
// Function:	H5Object::p_dereference (protected)
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
hid_t H5Object::p_dereference(hid_t loc_id, const void* ref, H5R_type_t ref_type)
{
   hid_t temp_id;
   temp_id = H5Rdereference(loc_id, ref_type, ref);
   if (temp_id < 0)
   {
      throw ReferenceException("", "H5Rdereference failed");
   }

   // No failure, set id to the object
   return(temp_id);
}

//--------------------------------------------------------------------------
// Function:	H5Object::dereference
///\brief	Dereferences a reference into an HDF5 object, given an HDF5 object.
///\param	obj - IN: Object specifying the location of the referenced object
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type
///\exception	H5::ReferenceException
// Programmer   Binh-Minh Ribler - Oct, 2006
// Modification
//	May, 2008
//		Corrected missing parameters. - BMR
//--------------------------------------------------------------------------
void H5Object::dereference(H5Object& obj, const void* ref, H5R_type_t ref_type)
{
   hid_t temp_id;
   try {
      temp_id = p_dereference(obj.getId(), ref, ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException("H5Object::dereference - located by object", E.getDetailMsg());
   }
   p_setId(temp_id);
}

//--------------------------------------------------------------------------
// Function:	H5Object::dereference
///\brief	Dereferences a reference into an HDF5 object, given an HDF5 file.
///\param	h5file - IN: HDF5 file specifying the location of the referenced object
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type
///\exception	H5::ReferenceException
// Programmer   Binh-Minh Ribler - Oct, 2006
// Modification
//	May, 2008
//		Corrected missing parameters. - BMR
//--------------------------------------------------------------------------
void H5Object::dereference(H5File& h5file, const void* ref, H5R_type_t ref_type)
{
   hid_t temp_id;
   try {
      temp_id = p_dereference(h5file.getId(), ref, ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException("H5Object::dereference - located by file", E.getDetailMsg());
   }
   p_setId(temp_id);
}

//--------------------------------------------------------------------------
// Function:	H5Object::dereference
///\brief	Dereferences a reference into an HDF5 object, given an attribute.
///\param	attr - IN: Attribute specifying the location of the referenced object
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type
///\exception	H5::ReferenceException
// Programmer   Binh-Minh Ribler - Oct, 2006
// Modification
//	May, 2008
//		Corrected missing parameters. - BMR
//--------------------------------------------------------------------------
void H5Object::dereference(Attribute& attr, const void* ref, H5R_type_t ref_type)
{
   hid_t temp_id;
   try {
      temp_id = p_dereference(attr.getId(), ref, ref_type);
   }
   catch (ReferenceException E) {
      throw ReferenceException("H5Object::dereference - located by attribute", E.getDetailMsg());
   }
   p_setId(temp_id);
}

#ifndef H5_NO_DEPRECATED_SYMBOLS
//--------------------------------------------------------------------------
// Function:    H5Object::p_get_obj_type (protected)
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
H5G_obj_t H5Object::p_get_obj_type(void *ref, H5R_type_t ref_type) const
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
// Function:    H5Object::p_get_region (protected)
// Purpose      Retrieves a dataspace with the region pointed to selected.
// Parameters
//              ref_type - IN: Type of reference to get region of - default
//                              to H5R_DATASET_REGION
//              ref      - IN: Reference to get region of
// Return       Dataspace id
// Exception    H5::IdComponentException
// Programmer   Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
hid_t H5Object::p_get_region(void *ref, H5R_type_t ref_type) const
{
   hid_t space_id = H5Rget_region(getId(), ref_type, ref);
   if (space_id < 0)
   {
      throw IdComponentException("", "H5Rget_region failed");
   }
   return(space_id);
}


//--------------------------------------------------------------------------
// Function:	H5Object destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::~H5Object() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
