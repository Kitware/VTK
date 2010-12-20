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
#include "H5FloatType.h"
#include "H5DataSet.h"
#include "H5PredType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	FloatType default constructor
///\brief	Default constructor: Creates a stub floating-point datatype
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::FloatType() {}

//--------------------------------------------------------------------------
// Function:	FloatType overloaded constructor
///\brief	Creates a floating-point datatype using a predefined type.
///\param	pred_type - IN: Predefined datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::FloatType( const PredType& pred_type ) : AtomType()
{
   // use DataType::copy to make a copy of this predefined type
   copy( pred_type );
}

//--------------------------------------------------------------------------
// Function:	FloatType overloaded constructor
///\brief	Creates an FloatType object using the id of an existing
///		datatype.
///\param	existing_id - IN: Id of an existing datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::FloatType( const hid_t existing_id ) : AtomType( existing_id ) {}

//--------------------------------------------------------------------------
// Function:	FloatType copy constructor
///\brief	Copy constructor: makes a copy of the original FloatType object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::FloatType( const FloatType&  original ) : AtomType( original ){}

//--------------------------------------------------------------------------
// Function:	EnumType overloaded constructor
///\brief	Gets the floating-point datatype of the specified dataset
///\param	dataset - IN: Dataset that this floating-point datatype
///		associates with
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::FloatType( const DataSet& dataset ) : AtomType()
{
   // Calls C function H5Dget_type to get the id of the datatype
   id = H5Dget_type( dataset.getId() );

   if( id < 0 )
   {
      throw DataSetIException("FloatType constructor", "H5Dget_type failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType::getFields
///\brief	Retrieves floating point datatype bit field information.
///\param	spos  - OUT: Retrieved floating-point sign bit
///\param	epos  - OUT: Retrieved exponent bit-position
///\param	esize - OUT: Retrieved size of exponent, in bits
///\param	mpos  - OUT: Retrieved mantissa bit-position
///\param	msize - OUT: Retrieved size of mantissa, in bits
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FloatType::getFields( size_t& spos, size_t& epos, size_t& esize, size_t& mpos, size_t& msize ) const
{
   herr_t ret_value = H5Tget_fields( id, &spos, &epos, &esize, &mpos, &msize );
   if( ret_value < 0 )
   {
      throw DataTypeIException("FloatType::getFields", "H5Tget_fields failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType::setFields
///\brief	Sets locations and sizes of floating point bit fields.
///\param	spos  - OUT: Sign position, i.e., the bit offset of the
///		floating-point sign bit.
///\param	epos  - OUT: Exponent bit position
///\param	esize - OUT: Size of exponent, in bits
///\param	mpos  - OUT: Mantissa bit-position
///\param	msize - OUT: Size of mantissa, in bits
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FloatType::setFields( size_t spos, size_t epos, size_t esize, size_t mpos, size_t msize ) const
{
   herr_t ret_value = H5Tset_fields( id, spos, epos, esize, mpos, msize );
   if( ret_value < 0 )
   {
      throw DataTypeIException("FloatType::setFields", "H5Tset_fields failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType::getEbias
///\brief	Retrieves the exponent bias of a floating-point type.
///\return	Exponent bias
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
size_t FloatType::getEbias() const
{
   size_t ebias = H5Tget_ebias( id );
   // Returns the bias if successful
   if( ebias == 0 )
   {
      throw DataTypeIException("FloatType::getEbias", "H5Tget_ebias failed - returned exponent bias as 0");
   }
   return( ebias );
}

//--------------------------------------------------------------------------
// Function:	FloatType::setEbias
///\brief	Sets the exponent bias of a floating-point type.
///\param	ebias - Exponent bias value
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FloatType::setEbias( size_t ebias ) const
{
   herr_t ret_value = H5Tset_ebias( id, ebias );
   if( ret_value < 0 )
   {
      throw DataTypeIException("FloatType::setEbias", "H5Tset_ebias failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType::getNorm
///\brief	Retrieves mantissa normalization of a floating-point datatype.
///\param	norm_string - OUT: Text string of the normalization type
///\return	Valid normalization type, which can be:
///		\li \c H5T_NORM_IMPLIED (0) - MSB of mantissa is not stored
///		\li \c H5T_NORM_MSBSET (1) - MSB of mantissa is always 1
///		\li \c H5T_NORM_NONE (2) - Mantissa is not normalized
///\exception	H5::DataTypeIException
///\par Description
///		For your convenience, this function also provides the text
///		string of the returned normalization type, via parameter
///		\a norm_string.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_norm_t FloatType::getNorm( H5std_string& norm_string ) const
{
   H5T_norm_t norm = H5Tget_norm( id );  // C routine
   // Returns a valid normalization type if successful
   if( norm == H5T_NORM_ERROR )
   {
      throw DataTypeIException("FloatType::getNorm", "H5Tget_norm failed - returned H5T_NORM_ERROR");
   }
   if( norm == H5T_NORM_IMPLIED )
      norm_string = "H5T_NORM_IMPLIED (0)";
   else if( norm == H5T_NORM_MSBSET )
      norm_string = "H5T_NORM_MSBSET (1)";
   else if( norm == H5T_NORM_NONE )
      norm_string = "H5T_NORM_NONE (2)";
   return( norm );
}

//--------------------------------------------------------------------------
// Function:	FloatType::setNorm
///\brief	Sets the mantissa normalization of a floating-point datatype.
///\param	norm - IN: Mantissa normalization type
///\exception	H5::DataTypeIException
///\par Description
///		Valid values for normalization type include:
///		\li \c H5T_NORM_IMPLIED (0) - MSB of mantissa is not stored
///		\li \c H5T_NORM_MSBSET (1) - MSB of mantissa is always 1
///		\li \c H5T_NORM_NONE (2) - Mantissa is not normalized
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FloatType::setNorm( H5T_norm_t norm ) const
{
   herr_t ret_value = H5Tset_norm( id, norm );
   if( ret_value < 0 )
   {
      throw DataTypeIException("FloatType::setNorm", "H5Tset_norm failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType::getInpad
///\brief	Retrieves the internal padding type for unused bits in
///		this floating-point datatypes.
///\return	Internal padding type, which can be:
///		\li \c H5T_PAD_ZERO (0) - Set background to zeros
///		\li \c H5T_PAD_ONE (1) - Set background to ones
///		\li \c H5T_PAD_BACKGROUND (2) - Leave background alone
///\exception	H5::DataTypeIException
///\par Description
///		For your convenience, this function also provides the text
///		string of the returned internal padding type, via parameter
///		\a pad_string.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_pad_t FloatType::getInpad( H5std_string& pad_string ) const
{
   H5T_pad_t pad_type = H5Tget_inpad( id );
   // Returns a valid padding type if successful
   if( pad_type == H5T_PAD_ERROR )
   {
      throw DataTypeIException("FloatType::getInpad", "H5Tget_inpad failed - returned H5T_PAD_ERROR");
   }
   if( pad_type == H5T_PAD_ZERO )
      pad_string = "H5T_PAD_ZERO (0)";
   else if( pad_type == H5T_PAD_ONE )
      pad_string = "H5T_PAD_ONE (1)";
   else if( pad_type == H5T_PAD_BACKGROUND )
      pad_string = "H5T_PAD_BACKGROUD (2)";
   return( pad_type );
}

//--------------------------------------------------------------------------
// Function:	FloatType::setInpad
///\brief	Fills unused internal floating point bits.
///\param	inpad - IN: Internal padding type
///\exception	H5::DataTypeIException
///\par Description
///		If any internal bits of a floating point type are unused
///		(that is, those significant bits which are not part of the
///		sign, exponent, or mantissa), then they will be filled
///		according to the padding value provided by \a inpad.
///\par
///		Valid values for normalization type include:
///		\li \c H5T_PAD_ZERO (0) - Set background to zeros
///		\li \c H5T_PAD_ONE (1) - Set background to ones
///		\li \c H5T_PAD_BACKGROUND (2) - Leave background alone
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FloatType::setInpad( H5T_pad_t inpad ) const
{
   herr_t ret_value = H5Tset_inpad( id, inpad );
   if( ret_value < 0 )
   {
      throw DataTypeIException("FloatType::setInpad", "H5Tset_inpad failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FloatType destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType::~FloatType() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
