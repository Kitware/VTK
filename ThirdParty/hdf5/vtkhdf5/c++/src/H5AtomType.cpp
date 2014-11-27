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
#include "H5AtomType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	AtomType default constructor [protected]
// Purpose	Default constructor: creates a stub atomic datatype.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AtomType::AtomType() : DataType() {}

//--------------------------------------------------------------------------
// Function:	AtomType overloaded constructor [protected]
// Purpose	Creates an AtomType object using an existing id.
// Parameter	existing_id - IN: Id of an existing datatype
// Exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AtomType::AtomType( const hid_t existing_id ) : DataType( existing_id ) {}

//--------------------------------------------------------------------------
// Function:	AtomType copy constructor
///\brief	Copy constructor: makes a copy of the original AtomType object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AtomType::AtomType( const AtomType& original ) : DataType( original ) {}
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	AtomType::setSize
///\brief	Sets the total size for an atomic datatype.
///\param	size - IN: Size to set
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::setSize( size_t size ) const
{
   // Call C routine H5Tset_size to set the total size
   herr_t ret_value = H5Tset_size( id, size );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setSize"), "H5Tset_size failed");
   }
}

//--------------------------------------------------------------------------
// Function:	AtomType::getOrder
///\brief	Returns the byte order of an atomic datatype.
///\return	Byte order, which can be:
///		\li \c H5T_ORDER_LE
///		\li \c H5T_ORDER_BE
///		\li \c H5T_ORDER_VAX
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Mar, 2005
//--------------------------------------------------------------------------
H5T_order_t AtomType::getOrder() const
{
   // Call C routine to get the byte ordering
   H5T_order_t type_order = H5Tget_order( id );

   // return a byte order constant if successful
   if( type_order == H5T_ORDER_ERROR )
   {
      throw DataTypeIException(inMemFunc("getOrder"),
		"H5Tget_order returns H5T_ORDER_ERROR");
   }
   return( type_order );
}

//--------------------------------------------------------------------------
// Function:	AtomType::getOrder
///\brief	This is an overloaded member function, provided for convenience.
///		It takes a reference to a \c H5std_string for the buffer that
///		provide the text description of the returned byte order.
///		The text description can be either of the following:
///		"Little endian byte ordering (0)";
///		"Big endian byte ordering (1)";
///		"VAX mixed byte ordering (2)";
///\param	order_string - OUT: Text description of the returned byte order
///\return	Byte order, which can be:
///		\li \c H5T_ORDER_LE
///		\li \c H5T_ORDER_BE
///		\li \c H5T_ORDER_VAX
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_order_t AtomType::getOrder( H5std_string& order_string ) const
{
   // Call the overloaded to get the type order without text
   H5T_order_t type_order = getOrder();

   // Then provide the text and return the type order
   if( type_order == H5T_ORDER_LE )
      order_string = "Little endian byte ordering (0)";
   else if( type_order == H5T_ORDER_BE )
      order_string = "Big endian byte ordering (1)";
   else if( type_order == H5T_ORDER_VAX )
      order_string = "VAX mixed byte ordering (2)";
   return( type_order );
}

//--------------------------------------------------------------------------
// Function:	AtomType::setOrder
///\brief	Sets the byte ordering of an atomic datatype.
///\param	order - IN: Byte ordering constant, which can be:
///		\li \c H5T_ORDER_LE
///		\li \c H5T_ORDER_BE
///		\li \c H5T_ORDER_VAX
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::setOrder( H5T_order_t order ) const
{
   // Call C routine to set the byte ordering
   herr_t ret_value = H5Tset_order( id, order );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setOrder"), "H5Tset_order failed");
   }
}

//--------------------------------------------------------------------------
// Function:	AtomType::getPrecision
///\brief	Returns the precision of an atomic datatype.
///\return	Number of significant bits
///\exception	H5::DataTypeIException
///\par Description
///		The precision is the number of significant bits which,
///		unless padding is present, is 8 times larger than the
///		value returned by \c DataType::getSize().
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
size_t AtomType::getPrecision() const
{
   size_t num_signi_bits = H5Tget_precision( id );  // C routine

   // returns number of significant bits if successful
   if( num_signi_bits == 0 )
   {
      throw DataTypeIException(inMemFunc("getPrecision"),
		"H5Tget_precision returns invalid number of significant bits");
   }
   return( num_signi_bits );
}

//--------------------------------------------------------------------------
// Function:	AtomType::setPrecision
///\brief	Sets the precision of an atomic datatype.
///\param	precision - IN: Number of bits of precision
///\exception	H5::DataTypeIException
///\par Description
///		For information, please see C layer Reference Manuat at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-SetPrecision
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::setPrecision( size_t precision ) const
{
   // Call C routine to set the datatype precision
   herr_t ret_value = H5Tset_precision( id, precision );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setPrecision"), "H5Tset_precision failed");
   }
}

//--------------------------------------------------------------------------
// Function:	AtomType::getOffset
///\brief	Retrieves the bit offset of the first significant bit.
///\return	Offset value
///\exception	H5::DataTypeIException
///\par Description
///		For information, please see C layer Reference Manuat at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-GetOffset
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		12/05/00: due to C API change
//			- return type changed from size_t to int
//			- offset = -1 when failure occurs vs. 0
//--------------------------------------------------------------------------
int AtomType::getOffset() const
{
   int offset = H5Tget_offset( id );  // C routine

   // returns a non-negative offset value if successful
   if( offset == -1 )
   {
      throw DataTypeIException(inMemFunc("getOffset"),
		"H5Tget_offset returns a negative offset value");
   }
   return( offset );
}

//--------------------------------------------------------------------------
// Function:	AtomType::setOffset
///\brief	Sets the bit offset of the first significant bit.
///\param	offset - IN: Offset of first significant bit
///\exception	H5::DataTypeIException
///\par Description
///		For information, please see C layer Reference Manuat at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-SetOffset
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::setOffset( size_t offset ) const
{
   // Call C routine to set the bit offset
   herr_t ret_value = H5Tset_offset( id, offset );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setOffset"), "H5Tset_offset failed");
   }
}

//--------------------------------------------------------------------------
// Function:	AtomType::getPad
///\brief	Retrieves the padding type of the least and most-significant
///		bit padding.
///\param	lsb - OUT: Least-significant bit padding type
///\param	msb - OUT: Most-significant bit padding type
///\exception	H5::DataTypeIException
///\par Description
///		Possible values for \a lsb and \a msb include:
///		\li \c H5T_PAD_ZERO (0) - Set background to zeros.
///		\li \c H5T_PAD_ONE (1) - Set background to ones.
///		\li \c H5T_PAD_BACKGROUND (2) - Leave background alone.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::getPad( H5T_pad_t& lsb, H5T_pad_t& msb ) const
{
   // Call C routine to get the padding type
   herr_t ret_value = H5Tget_pad( id, &lsb, &msb );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("getPad"), "H5Tget_pad failed");
   }
}

//--------------------------------------------------------------------------
// Function:	AtomType::setPad
///\brief	Sets the least and most-significant bits padding types.
///\param	lsb - IN: Least-significant bit padding type
///\param	msb - IN: Most-significant bit padding type
///\exception	H5::DataTypeIException
///\par Description
///		Valid values for \a lsb and \a msb include:
///		\li \c H5T_PAD_ZERO (0) - Set background to zeros.
///		\li \c H5T_PAD_ONE (1) - Set background to ones.
///		\li \c H5T_PAD_BACKGROUND (2) - Leave background alone.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void AtomType::setPad( H5T_pad_t lsb, H5T_pad_t msb ) const
{
   // Call C routine to set the padding type
   herr_t ret_value = H5Tset_pad( id, lsb, msb );
   if( ret_value < 0 )
   {
      throw DataTypeIException(inMemFunc("setPad"), "H5Tset_pad failed");
   }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	AtomType destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AtomType::~AtomType() {}
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
