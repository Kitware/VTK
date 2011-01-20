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

#ifdef H5_VMS
#include <iostream>
#endif /*H5_VMS*/

#include <string>
#include "H5Include.h"
#include "H5Exception.h"
#include "H5Library.h"
#include "H5IdComponent.h"
#include "H5DataSpace.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	IdComponent overloaded constructor
///\brief	Creates an IdComponent object using the id of an existing object.
///\param	h5_id - IN: Id of an existing object
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IdComponent::IdComponent(const hid_t h5_id) {}

//--------------------------------------------------------------------------
// Function:	IdComponent copy constructor
///\brief	Copy constructor: makes a copy of the original IdComponent object.
///\param	original - IN: IdComponent instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IdComponent::IdComponent( const IdComponent& original ) {}

//--------------------------------------------------------------------------
// Function:	IdComponent::incRefCount
///\brief	Increment reference counter for a given id.
// Programmer	Binh-Minh Ribler - May 2005
//--------------------------------------------------------------------------
void IdComponent::incRefCount(const hid_t obj_id) const
{
    if (p_valid_id(obj_id))
	if (H5Iinc_ref(obj_id) < 0)
	    throw IdComponentException(inMemFunc("incRefCount"), "incrementing object ref count failed");
}

//--------------------------------------------------------------------------
// Function:	IdComponent::incRefCount
///\brief	Increment reference counter for the id of this object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void IdComponent::incRefCount() const
{
    incRefCount(getId());
}

//--------------------------------------------------------------------------
// Function:	IdComponent::decRefCount
///\brief	Decrement reference counter for a given id.
// Programmer	Binh-Minh Ribler - May 2005
// Modification:
//		Added the check for ref counter to give a little more info
//		on why H5Idec_ref fails in some cases - BMR 5/19/2005
//--------------------------------------------------------------------------
void IdComponent::decRefCount(const hid_t obj_id) const
{
    if (p_valid_id(obj_id))
	if (H5Idec_ref(obj_id) < 0)
	    if (H5Iget_ref(obj_id) <= 0)
		throw IdComponentException(inMemFunc("decRefCount"),
					"object ref count is 0 or negative");
	    else
		throw IdComponentException(inMemFunc("decRefCount"),
					"decrementing object ref count failed");
}

//--------------------------------------------------------------------------
// Function:	IdComponent::decRefCount
///\brief	Decrement reference counter for the id of this object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void IdComponent::decRefCount() const
{
    decRefCount(getId());
}

//--------------------------------------------------------------------------
// Function:	IdComponent::getCounter
///\brief	Returns the reference counter for a given id.
///\return	Reference count
// Programmer	Binh-Minh Ribler - May 2005
//--------------------------------------------------------------------------
int IdComponent::getCounter(const hid_t obj_id) const
{
    int counter = 0;
    if (p_valid_id(obj_id))
    {
	counter = H5Iget_ref(obj_id);
	if (counter < 0)
	    throw IdComponentException(inMemFunc("incRefCount"), "getting object ref count failed - negative");
    }
    return (counter);
}

//--------------------------------------------------------------------------
// Function:	IdComponent::getCounter
///\brief	Returns the reference counter for the id of this object.
///\return	Reference count
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int IdComponent::getCounter() const
{
    return (getCounter(getId()));
}

//--------------------------------------------------------------------------
// Function:	hdfObjectType
///\brief	Given an id, returns the type of the object.
///\return	a valid HDF object type, which may be one of the following:
///		\li \c H5I_FILE
///		\li \c H5I_GROUP
///		\li \c H5I_DATATYPE
///		\li \c H5I_DATASPACE
///		\li \c H5I_DATASET
///		\li \c H5I_ATTR
///		\li or \c H5I_BADID, if no valid type can be determined or the
///				input object id is invalid.
// Programmer   Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
H5I_type_t IdComponent::getHDFObjType(const hid_t obj_id)
{
    H5I_type_t id_type = H5Iget_type(obj_id);
    if (id_type <= H5I_BADID || id_type >= H5I_NTYPES)
	return H5I_BADID; // invalid
    else
	return id_type; // valid type
}

//--------------------------------------------------------------------------
// Function:	IdComponent::operator=
///\brief	Assignment operator.
///\param	rhs - IN: Reference to the existing object
///\return	Reference to IdComponent instance
///\exception	H5::IdComponentException when attempt to close the HDF5
///		object fails
// Description
//		First, close the current valid id of this object.  Then
//		copy the id from rhs to this object, and increment the
//		reference counter of the id to indicate that another object
//		is referencing that id.
// Modification
//	2010/5/9 - BMR
//		Removed close() and incRefCount() because setId/p_setId takes
//		care of close() and setId takes care incRefCount().
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IdComponent& IdComponent::operator=( const IdComponent& rhs )
{
    if (this != &rhs)
    {
	// handling references to this id
  	try {
	    setId(rhs.getId());
	    // Note: a = b, so there are two objects with the same hdf5 id
	    // that's why incRefCount is needed, and it is called by setId
	}
	catch (Exception close_error) {
	    throw FileIException(inMemFunc("operator="), close_error.getDetailMsg());
	}
    }
    return *this;
}

//--------------------------------------------------------------------------
// Function:	IdComponent::setId
///\brief	Sets the identifier of this object to a new value.
///
///\exception	H5::IdComponentException when the attempt to close the HDF5
///		object fails
// Description:
//		p_setId ensures that the current valid id of this object is
//		properly closed before resetting the object's id to the new id.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//	2008/7/23 - BMR
//		Changed all subclasses' setId to p_setId and put back setId
//		here.  p_setId is used in the library where the id provided
//		by a C API passed on to user's application in the form of a
//		C++ API object, which will be destroyed properly, and so
//		p_setId does not call incRefCount.  On the other hand, the
//		public version setId is used by other applications, in which
//		the id passed to setId already has a reference count, so setId
//		must call incRefCount.
//--------------------------------------------------------------------------
void IdComponent::setId(const hid_t new_id)
{
   // set to new_id
   p_setId(new_id);

   // increment the reference counter of the new id
   incRefCount();
}

//--------------------------------------------------------------------------
// Function:	IdComponent destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IdComponent::~IdComponent() {}

//
// Implementation of protected functions for HDF5 Reference Interface
// and miscelaneous helpers.
//

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	IdComponent::inMemFunc
///\brief	Makes and returns string "<class-name>::<func_name>"
///\param	func_name - Name of the function where failure occurs
// Description
///		Concatenates the class name of this object with the
///		passed-in function name to create a string that indicates
///		where the failure occurs.  The class-name is provided by
///		fromClass().  This string will be used by a base class when
///		an exception is thrown.
// Programmer	Binh-Minh Ribler - Aug 6, 2005
//--------------------------------------------------------------------------
H5std_string IdComponent::inMemFunc(const char* func_name) const
{
#ifdef H5_VMS
   H5std_string full_name = fromClass();
   full_name.append("::");
   full_name.append(func_name);
#else
   H5std_string full_name = func_name;
   full_name.insert(0, "::");
   full_name.insert(0, fromClass());
#endif /*H5_VMS*/
   return (full_name);
}

//--------------------------------------------------------------------------
// Function:	IdComponent default constructor - private
///\brief	Default constructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IdComponent::IdComponent() {}

//--------------------------------------------------------------------------
// Function:	IdComponent::p_get_file_name (protected)
// Purpose:	Gets the name of the file, in which this object belongs.
// Exception:	H5::IdComponentException
// Description:
// 		This function is protected so that the user applications can
// 		only have access to its code via allowable classes, namely,
// 		H5File and H5Object subclasses.
// Programmer	Binh-Minh Ribler - Jul, 2004
//--------------------------------------------------------------------------
H5std_string IdComponent::p_get_file_name() const
{
   hid_t temp_id = getId();

   // Preliminary call to H5Fget_name to get the length of the file name
   ssize_t name_size = H5Fget_name(temp_id, NULL, 0);

   // If H5Aget_name returns a negative value, raise an exception,
   if( name_size < 0 )
   {
      throw IdComponentException("", "H5Fget_name failed");
   }

   // Call H5Fget_name again to get the actual file name
   char* name_C = new char[name_size+1];  // temporary C-string for C API
   name_size = H5Fget_name(temp_id, name_C, name_size+1);

   // Check for failure again
   if( name_size < 0 )
   {
      throw IdComponentException("", "H5Fget_name failed");
   }

   // Convert the C file name and return
   H5std_string file_name(name_C);
   delete []name_C;
   return(file_name);
}

//
// Local functions used in this class
//

//--------------------------------------------------------------------------
// Function:	p_valid_id
// Purpose:	Verifies that the given id is a valid id so it can be passed
//		into an H5I C function.
// Return	true if id is valid, false, otherwise
// Programmer	Binh-Minh Ribler - May, 2005
//--------------------------------------------------------------------------
bool IdComponent::p_valid_id(const hid_t obj_id)
{
    H5I_type_t id_type = H5Iget_type(obj_id);
    if (id_type <= H5I_BADID || id_type >= H5I_NTYPES)
	return false;
    else
	return true;
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef H5_NO_NAMESPACE
}
#endif
