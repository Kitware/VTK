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
#include "H5AbstractDs.h"
#include "H5DxferProp.h"
#include "H5DataSpace.h"
#include "H5DcreatProp.h"
#include "H5CommonFG.h"
#include "H5DataType.h"
#include "H5DataSet.h"
#include "H5AtomType.h"
#include "H5IntType.h"
#include "H5EnumType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	EnumType default constructor
///\brief	Default constructor: Creates a stub datatype
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType() : DataType() {}

//--------------------------------------------------------------------------
// Function:	EnumType overloaded constructor
///\brief	Creates an EnumType object using the id of an existing datatype.
///\param	existing_id - IN: Id of an existing datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType( const hid_t existing_id ) : DataType( existing_id ) {}

//--------------------------------------------------------------------------
// Function:	EnumType copy constructor
///\brief	Copy constructor: makes a copy of the original EnumType object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType( const EnumType& original ) : DataType( original ) {}

//--------------------------------------------------------------------------
// Function:	EnumType overloaded constructor
///\brief	Creates an empty enumeration datatype given a size, in bytes.
///\param	size - IN: Number of bytes in the datatype to create
///\exception	H5::DataTypeIException
// Description
// 		The DataType constructor calls the C API H5Tcreate to create
// 		the enum datatype.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType( size_t size ) : DataType( H5T_ENUM, size ) {}

//--------------------------------------------------------------------------
// Function:	EnumType overloaded constructor
///\brief	Gets the enum datatype of the specified dataset.
///\param	dataset - IN: Dataset that this enum datatype associates with
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType( const DataSet& dataset ) : DataType()
{
   // Calls C function H5Dget_type to get the id of the datatype
   id = H5Dget_type( dataset.getId() );

   // If the datatype id is not valid, throw an exception
   if( id < 0 )
   {
      throw DataSetIException("EnumType constructor", "H5Dget_type failed");
   }
}

//--------------------------------------------------------------------------
// Function:	EnumType overloaded constructor
///\brief	Creates a new enum datatype based on an integer datatype.
///\param	data_type - IN: Base datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::EnumType( const IntType& data_type ) : DataType()
{
   // Calls C function H5Tenum_create to get the id of the datatype
   id = H5Tenum_create( data_type.getId() );

   // If the datatype id is not valid, throw an exception
   if( id < 0 )
   {
      throw DataSetIException("EnumType constructor", "H5Tenum_create failed");
   }
}

//--------------------------------------------------------------------------
// Function:	EnumType::insert
///\brief	Inserts a new member to this enumeration datatype.
///\param	name  - IN: Name of the new member
///\param	value - IN: Pointer to the value of the new member
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void EnumType::insert( const char* name, void *value ) const
{
   // Calls C routine H5Tenum_insert to insert the new enum datatype member.
   herr_t ret_value = H5Tenum_insert( id, name, value );
   if( ret_value < 0 )
   {
      throw DataTypeIException("EnumType::insert", "H5Tenum_insert failed");
   }
}

//--------------------------------------------------------------------------
// Function:	EnumType::insert
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void EnumType::insert( const H5std_string& name, void *value ) const
{
    insert( name.c_str(), value );
}

//--------------------------------------------------------------------------
// Function:	EnumType::nameOf
///\brief	Returns the symbol name corresponding to a specified member
///		of this enumeration datatype.
///\param	value - IN: Pointer to the value of the enum datatype
///\param	size  - IN: Size for the name
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string EnumType::nameOf( void *value, size_t size ) const
{
   char* name_C = new char[size+1];  // temporary C-string for C API

   // Calls C routine H5Tenum_nameof to get the name of the specified enum type
   herr_t ret_value = H5Tenum_nameof( id, value, name_C, size );

   // If H5Tenum_nameof returns a negative value, raise an exception,
   if( ret_value < 0 )
   {
      throw DataTypeIException("EnumType::nameOf", "H5Tenum_nameof failed");
   }
   // otherwise, create the string to hold the datatype name and return it
   H5std_string name = H5std_string(name_C);
   delete []name_C;
   return( name );
}

//--------------------------------------------------------------------------
// Function:	EnumType::valueOf
///\brief	Retrieves the value corresponding to a member of this
///		enumeration datatype, given the member's name.
///\param	name  -  IN: Name of the queried member
///\param	value - OUT: Pointer to the retrieved value
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void EnumType::valueOf( const char* name, void *value ) const
{
   // Calls C routine H5Tenum_valueof to get the enum datatype value
   herr_t ret_value = H5Tenum_valueof( id, name, value );
   if( ret_value < 0 )
   {
      throw DataTypeIException("EnumType::valueOf", "H5Tenum_valueof failed");
   }
}

//--------------------------------------------------------------------------
// Function:	EnumType::valueOf
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of
///		argument \a name.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void EnumType::valueOf( const H5std_string& name, void *value ) const
{
    valueOf( name.c_str(), value );
}

//--------------------------------------------------------------------------
// Function:	EnumType::getMemberIndex
///\brief	Returns the index of a member in this enumeration datatype.
///\param	name - IN: Name of the queried member
///\return	Index of the member if it exists.  Index will have the value
///		between 0 and \c N-1, where \c N is the value returned by the
///		member function \c EnumType::getNmembers.
///\exception	H5::DataTypeIException
// Programmer   Binh-Minh Ribler - May 16, 2002
//--------------------------------------------------------------------------
int EnumType::getMemberIndex(const char *name) const
{
   int member_index = H5Tget_member_index(id, name);
   if( member_index < 0 )
   {
      throw DataTypeIException("EnumType::getMemberIndex",
                "H5Tget_member_index returns negative value");
   }
   return( member_index );
}

//--------------------------------------------------------------------------
// Function:	EnumType::getMemberIndex
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in the type of
///		argument \a name.
// Programmer   Binh-Minh Ribler - May 16, 2002
//--------------------------------------------------------------------------
int EnumType::getMemberIndex(const H5std_string& name) const
{
    return(EnumType::getMemberIndex(name.c_str()));
}

//--------------------------------------------------------------------------
// Function:	EnumType::getNmembers
///\brief	Returns the number of members in this enumeration datatype.
///\return	Number of members
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
int EnumType::getNmembers() const
{
   int num_members = H5Tget_nmembers( id );
   if( num_members < 0 )
   {
      throw DataTypeIException("EnumType::getNmembers",
                "H5Tget_nmembers returns negative number of members");
   }
   return( num_members );
}

//--------------------------------------------------------------------------
// Function:	EnumType::getMemberValue
///\brief	Retrieves the value of a member in this enumeration datatype,
///		given the member's index.
///\param	memb_no - IN: Index of the queried member
///\param	value   - OUT: Pointer to the retrieved value
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void EnumType::getMemberValue( unsigned memb_no, void *value ) const
{
   // Call C routine H5Tget_member_value to get the datatype member's value
   hid_t ret_value = H5Tget_member_value( id, memb_no, value );
   if( ret_value < 0 )
   {
      throw DataTypeIException("EnumType::getMemberValue", "H5Tget_member_value failed");
   }
}

//--------------------------------------------------------------------------
// Function:	EnumType destructor
///\brief	Properly terminates access to this enum datatype.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType::~EnumType() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
