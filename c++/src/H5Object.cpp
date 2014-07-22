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
#include "H5private.h"		// for HDmemset

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	H5Object default constructor (protected)
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object() : H5Location() {}

//--------------------------------------------------------------------------
// Function:	H5Object overloaded constructor (protected)
// Purpose	Creates an H5Object object using the id of an existing HDF5
// 		object.
// Parameters	object_id - IN: Id of an existing HDF5 object
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object( const hid_t object_id ) : H5Location( object_id ) {}

//--------------------------------------------------------------------------
// Function:    getObjName
///\brief       Given an id, returns the type of the object.
///\return      The name of the object
// Programmer   Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
ssize_t H5Object::getObjName(char *obj_name, size_t buf_size) const
{
    // H5Iget_name will get buf_size-1 chars of the name to null terminate it
    ssize_t name_size = H5Iget_name(getId(), obj_name, buf_size);

    // If H5Iget_name returns a negative value, raise an exception
    if (name_size < 0)
    {
        throw Exception(inMemFunc("getObjName"), "H5Iget_name failed");
    }
    else if (name_size == 0)
    {
        throw Exception(inMemFunc("getObjName"), "Object must have a name, but name length is 0");
    }
    // Return length of the name
    return(name_size);
}

//--------------------------------------------------------------------------
// Function:    H5Object::getObjName
///\brief       Returns the name of this object as an \a H5std_string.
///\return      Name of the object
///\exception   H5::Exception
// Programmer   Binh-Minh Ribler - Mar, 2014
// Modification
//--------------------------------------------------------------------------
H5std_string H5Object::getObjName() const
{
    H5std_string obj_name(""); // object name to return

    // Preliminary call to get the size of the object name
    ssize_t name_size = H5Iget_name(getId(), NULL, (size_t)0);

    // If H5Iget_name failed, throw exception
    if (name_size < 0)
    {
        throw Exception(inMemFunc("getObjName"), "H5Iget_name failed");
    }
    else if (name_size == 0)
    {
        throw Exception(inMemFunc("getObjName"), "Object must have a name, but name length is 0");
    }
    // Object's name exists, retrieve it
    else if (name_size > 0)
    {
        char* name_C = new char[name_size+1];  // temporary C-string
        HDmemset(name_C, 0, name_size+1); // clear buffer

        // Use overloaded function
        name_size = getObjName(name_C, name_size+1);

        // Convert the C object name to return
        obj_name = name_C;

        // Clean up resource
        delete []name_C;
    }
    // Return object's name
    return(obj_name);
}

//--------------------------------------------------------------------------
// Function:    H5Object::getObjName
///\brief       Gets the name of this object, returning its length.
///\param       obj_name - OUT: Buffer for the name string as \a H5std_string
///\param       len  -  IN: Desired length of the name, default to 0
///\return      Actual length of the object name
///\exception   H5::Exception
///\par Description
///             This function retrieves the object's name as an std string.
///             buf_size can specify a specific length or default to 0, in
///             which case the entire name will be retrieved.
// Programmer   Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
ssize_t H5Object::getObjName(H5std_string& obj_name, size_t len) const
{
    ssize_t name_size = 0;

    // If no length is provided, get the entire object name
    if (len == 0)
    {
        obj_name = getObjName();
        name_size = obj_name.length();
    }
    // If length is provided, get that number of characters in name
    else
    {
        char* name_C = new char[len+1];  // temporary C-string
        HDmemset(name_C, 0, len+1); // clear buffer

        // Use overloaded function
        name_size = getObjName(name_C, len+1);

        // Convert the C object name to return
        obj_name = name_C;

        // Clean up resource
        delete []name_C;
    }
    // Otherwise, keep obj_name intact

    // Return name size
    return(name_size);
}

//--------------------------------------------------------------------------
// Function:	H5Object copy constructor
///\brief	Copy constructor: makes a copy of the original H5Object
///		instance.
///\param	original - IN: H5Object instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::H5Object( const H5Object& original ) : H5Location( original ) {}

//--------------------------------------------------------------------------
// Function:	H5Object destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5Object::~H5Object() {}
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
