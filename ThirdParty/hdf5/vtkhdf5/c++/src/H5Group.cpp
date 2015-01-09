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
#include "H5AbstractDs.h"
#include "H5FaccProp.h"
#include "H5FcreatProp.h"
#include "H5DcreatProp.h"
#include "H5DxferProp.h"
#include "H5DataSpace.h"
#include "H5DataSet.h"
#include "H5CommonFG.h"
#include "H5Attribute.h"
#include "H5Group.h"
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
// Function:	Group default constructor
///\brief	Default constructor: creates a stub Group.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group::Group() : H5Object(), id(0) {}

//--------------------------------------------------------------------------
// Function:	Group copy constructor
///\brief	Copy constructor: makes a copy of the original Group object.
///\param	original - IN: Original group to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group::Group(const Group& original) : H5Object(original)
{
    id = original.getId();
    incRefCount(); // increment number of references to this id
}

//--------------------------------------------------------------------------
// Function:	Group::getLocId
///\brief	Returns the id of this group.
///\return	Id of this group
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hid_t Group::getLocId() const
{
   return( getId() );
}

//--------------------------------------------------------------------------
// Function:	Group overloaded constructor
///\brief	Creates a Group object using the id of an existing group.
///\param	existing_id - IN: Id of an existing group
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Group::Group(const hid_t existing_id) : H5Object()
{
    id = existing_id;
}

//--------------------------------------------------------------------------
// Function:	Group overload constructor - dereference
///\brief	Given a reference, ref, to an hdf5 group, creates a Group object
///\param	loc - IN: Specifying location referenced object is in
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type - default to H5R_OBJECT
///\exception	H5::ReferenceException
///\par Description
///		\c obj can be DataSet, Group, or named DataType, that
///		is a datatype that has been named by DataType::commit.
// Programmer	Binh-Minh Ribler - Oct, 2006
//--------------------------------------------------------------------------
Group::Group(const H5Location& loc, const void* ref, H5R_type_t ref_type) : H5Object(), id(0)
{
    id = H5Location::p_dereference(loc.getId(), ref, ref_type, "constructor - by dereference");
}

//--------------------------------------------------------------------------
// Function:	Group overload constructor - dereference
///\brief	Given a reference, ref, to an hdf5 group, creates a Group object
///\param	attr - IN: Specifying location where the referenced object is in
///\param	ref - IN: Reference pointer
///\param	ref_type - IN: Reference type - default to H5R_OBJECT
///\exception	H5::ReferenceException
// Programmer	Binh-Minh Ribler - Oct, 2006
//--------------------------------------------------------------------------
Group::Group(const Attribute& attr, const void* ref, H5R_type_t ref_type) : H5Object(), id(0)
{
    id = H5Location::p_dereference(attr.getId(), ref, ref_type, "constructor - by dereference");
}

//--------------------------------------------------------------------------
// Function:    Group::getId
///\brief	Get the id of this group
///\return	Group identifier
// Modification:
//      May 2008 - BMR
//		Class hierarchy is revised to address bugzilla 1068.  Class
//		AbstractDS and Attribute are moved out of H5Object.  In
//		addition, member IdComponent::id is moved into subclasses, and
//		IdComponent::getId now becomes pure virtual function.
// Programmer   Binh-Minh Ribler - May, 2008
//--------------------------------------------------------------------------
hid_t Group::getId() const
{
   return(id);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:    Group::p_setId
///\brief       Sets the identifier of this object to a new value.
///
///\exception   H5::IdComponentException when the attempt to close the HDF5
///		object fails
// Description:
//		The underlaying reference counting in the C library ensures
//		that the current valid id of this object is properly closed.
//		Then the object's id is reset to the new id.
// Programmer   Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Group::p_setId(const hid_t new_id)
{
    // handling references to this old id
    try {
        close();
    }
    catch (Exception close_error) {
        throw GroupIException("Group::p_setId", close_error.getDetailMsg());
    }
   // reset object's id to the given id
   id = new_id;
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	Group::close
///\brief	Closes this group.
///
///\exception	H5::GroupIException
// Programmer	Binh-Minh Ribler - Mar 9, 2005
//--------------------------------------------------------------------------
void Group::close()
{
    if (p_valid_id(id))
    {
	herr_t ret_value = H5Gclose( id );
	if( ret_value < 0 )
	{
	    throw GroupIException("Group::close", "H5Gclose failed");
	}
	// reset the id
	id = 0;
    }
}

//--------------------------------------------------------------------------
// Function:	Group::throwException
///\brief	Throws H5::GroupIException.
///\param	func_name - Name of the function where failure occurs
///\param	msg       - Message describing the failure
///\exception	H5::GroupIException
// Description
//		This function is used in CommonFG implementation so that
//		proper exception can be thrown for file or group.  The
//		argument func_name is a member of CommonFG and "Group::"
//		will be inserted to indicate the function called is an
//		implementation of Group.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Group::throwException(const H5std_string& func_name, const H5std_string& msg) const
{
   H5std_string full_name = func_name;
   full_name.insert(0, "Group::");
   throw GroupIException(full_name, msg);
}

//--------------------------------------------------------------------------
// Function:	Group destructor
///\brief	Properly terminates access to this group.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Feb 20, 2005
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
Group::~Group()
{
    try {
	close();
    }
    catch (Exception close_error) {
	cerr << "Group::~Group - " << close_error.getDetailMsg() << endl;
    }
}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
