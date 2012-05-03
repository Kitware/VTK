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
#include "H5CommonFG.h"
#include "H5DataType.h"
#include "H5AbstractDs.h"
#include "H5DxferProp.h"
#include "H5DataSpace.h"
#include "H5AtomType.h"
#include "H5IntType.h"
#include "H5DataSet.h"
#include "H5PredType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif


//--------------------------------------------------------------------------
// Function:	IntType default constructor
///\brief	Default constructor: Creates a stub integer datatype
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::IntType() {}

//--------------------------------------------------------------------------
// Function:	IntType copy constructor
///\brief	Copy constructor: makes a copy of the original IntType object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::IntType( const IntType& original ) : AtomType( original ) {}

//--------------------------------------------------------------------------
// Function:	IntType overloaded constructor
///\brief	Creates a integer type using a predefined type
///\param	pred_type - IN: Predefined datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::IntType( const PredType& pred_type ) : AtomType()
{
   // use DataType::copy to make a copy of this predefined type
   copy( pred_type );
}

//--------------------------------------------------------------------------
// Function:	IntType overloaded constructor
///\brief	Creates an integer datatype using the id of an existing
///		datatype.
///\param	existing_id - IN: Id of an existing datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::IntType( const hid_t existing_id ) : AtomType( existing_id ) {}

//--------------------------------------------------------------------------
// Function:	IntType overloaded constructor
///\brief	Gets the integer datatype of the specified dataset.
///\param	dataset - IN: Dataset that this integer datatype associates with
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::IntType( const DataSet& dataset ) : AtomType()
{
   // Calls C function H5Dget_type to get the id of the datatype
   id = H5Dget_type( dataset.getId() );

   if( id < 0 )
   {
      throw DataSetIException("IntType constructor", "H5Dget_type failed");
   }
}

//--------------------------------------------------------------------------
// Function:	IntType::getSign
///\brief	Retrieves the sign type for an integer type.
///\return	Valid sign type
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_sign_t IntType::getSign() const
{
   H5T_sign_t type_sign = H5Tget_sign( id );  // C routine

   // Returns a valid sign type if no errors
   if( type_sign == H5T_SGN_ERROR )
   {
      throw DataTypeIException("IntType::getSign",
		"H5Tget_sign failed - returned H5T_SGN_ERROR for the sign type");
   }
   return( type_sign );
}

//--------------------------------------------------------------------------
// Function:	IntType::getSign
///\brief	Sets the sign property for an integer type.
///\param	sign - IN: Sign type
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void IntType::setSign( H5T_sign_t sign ) const
{
   // Call C routine to set the sign property
   herr_t ret_value = H5Tset_sign( id, sign );
   if( ret_value < 0 )
   {
      throw DataTypeIException("IntType::setSign", "H5Tset_sign failed");
   }
}

//--------------------------------------------------------------------------
// Function:	IntType destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType::~IntType() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
