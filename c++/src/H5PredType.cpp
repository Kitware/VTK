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
#include "H5Library.h"
#include "H5PredType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:	PredType overloaded constructor
///\brief	Creates a PredType object using the id of an existing
///		predefined datatype.
///\param	predtype_id - IN: Id of a predefined datatype
// Description
// 		This constructor creates a PredType object by copying
//		the provided HDF5 predefined datatype.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
PredType::PredType( const hid_t predtype_id ) : AtomType( predtype_id )
{
    id = H5Tcopy(predtype_id);
}

//--------------------------------------------------------------------------
// Function:	PredType default constructor
///\brief	Default constructor: Creates a stub predefined datatype
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
PredType::PredType() : AtomType() {}

//--------------------------------------------------------------------------
// Function:	PredType copy constructor
///\brief	Copy constructor: makes a copy of the original PredType object.
///\param	original - IN: PredType instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
PredType::PredType( const PredType& original ) : AtomType( original ) {}

const PredType PredType::NotAtexit;	// only for atexit/global dest. problem

// Definition of pre-defined types
const PredType PredType::C_S1( H5T_C_S1 );
const PredType PredType::FORTRAN_S1( H5T_FORTRAN_S1 );

const PredType PredType::STD_I8BE( H5T_STD_I8BE );
const PredType PredType::STD_I8LE( H5T_STD_I8LE );
const PredType PredType::STD_I16BE( H5T_STD_I16BE );
const PredType PredType::STD_I16LE( H5T_STD_I16LE );
const PredType PredType::STD_I32BE( H5T_STD_I32BE );
const PredType PredType::STD_I32LE( H5T_STD_I32LE );
const PredType PredType::STD_I64BE( H5T_STD_I64BE );
const PredType PredType::STD_I64LE( H5T_STD_I64LE );
const PredType PredType::STD_U8BE( H5T_STD_U8BE );
const PredType PredType::STD_U8LE( H5T_STD_U8LE );
const PredType PredType::STD_U16BE( H5T_STD_U16BE );
const PredType PredType::STD_U16LE( H5T_STD_U16LE );
const PredType PredType::STD_U32BE( H5T_STD_U32BE );
const PredType PredType::STD_U32LE( H5T_STD_U32LE );
const PredType PredType::STD_U64BE( H5T_STD_U64BE );
const PredType PredType::STD_U64LE( H5T_STD_U64LE );
const PredType PredType::STD_B8BE( H5T_STD_B8BE );
const PredType PredType::STD_B8LE( H5T_STD_B8LE );

const PredType PredType::STD_B16BE( H5T_STD_B16BE );
const PredType PredType::STD_B16LE( H5T_STD_B16LE );
const PredType PredType::STD_B32BE( H5T_STD_B32BE );
const PredType PredType::STD_B32LE( H5T_STD_B32LE );
const PredType PredType::STD_B64BE( H5T_STD_B64BE );
const PredType PredType::STD_B64LE( H5T_STD_B64LE );
const PredType PredType::STD_REF_OBJ( H5T_STD_REF_OBJ );
const PredType PredType::STD_REF_DSETREG( H5T_STD_REF_DSETREG );

const PredType PredType::IEEE_F32BE( H5T_IEEE_F32BE );
const PredType PredType::IEEE_F32LE( H5T_IEEE_F32LE );
const PredType PredType::IEEE_F64BE( H5T_IEEE_F64BE );
const PredType PredType::IEEE_F64LE( H5T_IEEE_F64LE );

const PredType PredType::UNIX_D32BE( H5T_UNIX_D32BE );
const PredType PredType::UNIX_D32LE( H5T_UNIX_D32LE );
const PredType PredType::UNIX_D64BE( H5T_UNIX_D64BE );
const PredType PredType::UNIX_D64LE( H5T_UNIX_D64LE );

const PredType PredType::INTEL_I8( H5T_INTEL_I8 );
const PredType PredType::INTEL_I16( H5T_INTEL_I16 );
const PredType PredType::INTEL_I32( H5T_INTEL_I32 );
const PredType PredType::INTEL_I64( H5T_INTEL_I64 );
const PredType PredType::INTEL_U8( H5T_INTEL_U8 );
const PredType PredType::INTEL_U16( H5T_INTEL_U16 );
const PredType PredType::INTEL_U32( H5T_INTEL_U32 );
const PredType PredType::INTEL_U64( H5T_INTEL_U64 );
const PredType PredType::INTEL_B8( H5T_INTEL_B8 );
const PredType PredType::INTEL_B16( H5T_INTEL_B16 );
const PredType PredType::INTEL_B32( H5T_INTEL_B32 );
const PredType PredType::INTEL_B64( H5T_INTEL_B64 );
const PredType PredType::INTEL_F32( H5T_INTEL_F32 );
const PredType PredType::INTEL_F64( H5T_INTEL_F64 );

const PredType PredType::ALPHA_I8( H5T_ALPHA_I8 );
const PredType PredType::ALPHA_I16( H5T_ALPHA_I16 );
const PredType PredType::ALPHA_I32( H5T_ALPHA_I32 );
const PredType PredType::ALPHA_I64( H5T_ALPHA_I64 );
const PredType PredType::ALPHA_U8( H5T_ALPHA_U8 );
const PredType PredType::ALPHA_U16( H5T_ALPHA_U16 );
const PredType PredType::ALPHA_U32( H5T_ALPHA_U32 );
const PredType PredType::ALPHA_U64( H5T_ALPHA_U64 );
const PredType PredType::ALPHA_B8( H5T_ALPHA_B8 );
const PredType PredType::ALPHA_B16( H5T_ALPHA_B16 );
const PredType PredType::ALPHA_B32( H5T_ALPHA_B32 );
const PredType PredType::ALPHA_B64( H5T_ALPHA_B64 );
const PredType PredType::ALPHA_F32( H5T_ALPHA_F32 );
const PredType PredType::ALPHA_F64( H5T_ALPHA_F64 );

const PredType PredType::MIPS_I8( H5T_MIPS_I8 );
const PredType PredType::MIPS_I16( H5T_MIPS_I16 );
const PredType PredType::MIPS_I32( H5T_MIPS_I32 );
const PredType PredType::MIPS_I64( H5T_MIPS_I64 );
const PredType PredType::MIPS_U8( H5T_MIPS_U8 );
const PredType PredType::MIPS_U16( H5T_MIPS_U16 );
const PredType PredType::MIPS_U32( H5T_MIPS_U32 );
const PredType PredType::MIPS_U64( H5T_MIPS_U64 );
const PredType PredType::MIPS_B8( H5T_MIPS_B8 );
const PredType PredType::MIPS_B16( H5T_MIPS_B16 );
const PredType PredType::MIPS_B32( H5T_MIPS_B32 );
const PredType PredType::MIPS_B64( H5T_MIPS_B64 );
const PredType PredType::MIPS_F32( H5T_MIPS_F32 );
const PredType PredType::MIPS_F64( H5T_MIPS_F64 );

const PredType PredType::NATIVE_CHAR( H5T_NATIVE_CHAR );
const PredType PredType::NATIVE_INT( H5T_NATIVE_INT );
const PredType PredType::NATIVE_FLOAT( H5T_NATIVE_FLOAT );
const PredType PredType::NATIVE_SCHAR( H5T_NATIVE_SCHAR );
const PredType PredType::NATIVE_UCHAR( H5T_NATIVE_UCHAR );
const PredType PredType::NATIVE_SHORT( H5T_NATIVE_SHORT );
const PredType PredType::NATIVE_USHORT( H5T_NATIVE_USHORT );
const PredType PredType::NATIVE_UINT( H5T_NATIVE_UINT );
const PredType PredType::NATIVE_LONG( H5T_NATIVE_LONG );
const PredType PredType::NATIVE_ULONG( H5T_NATIVE_ULONG );
const PredType PredType::NATIVE_LLONG( H5T_NATIVE_LLONG );
const PredType PredType::NATIVE_ULLONG( H5T_NATIVE_ULLONG );
const PredType PredType::NATIVE_DOUBLE( H5T_NATIVE_DOUBLE );
#if H5_SIZEOF_LONG_DOUBLE !=0
const PredType PredType::NATIVE_LDOUBLE( H5T_NATIVE_LDOUBLE );
#endif
const PredType PredType::NATIVE_B8( H5T_NATIVE_B8 );
const PredType PredType::NATIVE_B16( H5T_NATIVE_B16 );
const PredType PredType::NATIVE_B32( H5T_NATIVE_B32 );
const PredType PredType::NATIVE_B64( H5T_NATIVE_B64 );
const PredType PredType::NATIVE_OPAQUE( H5T_NATIVE_OPAQUE );
const PredType PredType::NATIVE_HSIZE( H5T_NATIVE_HSIZE );
const PredType PredType::NATIVE_HSSIZE( H5T_NATIVE_HSSIZE );
const PredType PredType::NATIVE_HERR( H5T_NATIVE_HERR );
const PredType PredType::NATIVE_HBOOL( H5T_NATIVE_HBOOL );

const PredType PredType::NATIVE_INT8( H5T_NATIVE_INT8 );
const PredType PredType::NATIVE_UINT8( H5T_NATIVE_UINT8 );
const PredType PredType::NATIVE_INT16( H5T_NATIVE_INT16 );
const PredType PredType::NATIVE_UINT16( H5T_NATIVE_UINT16 );
const PredType PredType::NATIVE_INT32( H5T_NATIVE_INT32 );
const PredType PredType::NATIVE_UINT32( H5T_NATIVE_UINT32 );
const PredType PredType::NATIVE_INT64( H5T_NATIVE_INT64 );
const PredType PredType::NATIVE_UINT64( H5T_NATIVE_UINT64 );

// LEAST types
#if H5_SIZEOF_INT_LEAST8_T != 0
const PredType PredType::NATIVE_INT_LEAST8( H5T_NATIVE_INT_LEAST8 );
#endif /* H5_SIZEOF_INT_LEAST8_T */
#if H5_SIZEOF_UINT_LEAST8_T != 0
const PredType PredType::NATIVE_UINT_LEAST8( H5T_NATIVE_UINT_LEAST8 );
#endif /* H5_SIZEOF_UINT_LEAST8_T */

#if H5_SIZEOF_INT_LEAST16_T != 0
const PredType PredType::NATIVE_INT_LEAST16( H5T_NATIVE_INT_LEAST16 );
#endif /* H5_SIZEOF_INT_LEAST16_T */
#if H5_SIZEOF_UINT_LEAST16_T != 0
const PredType PredType::NATIVE_UINT_LEAST16( H5T_NATIVE_UINT_LEAST16 );
#endif /* H5_SIZEOF_UINT_LEAST16_T */

#if H5_SIZEOF_INT_LEAST32_T != 0
const PredType PredType::NATIVE_INT_LEAST32( H5T_NATIVE_INT_LEAST32 );
#endif /* H5_SIZEOF_INT_LEAST32_T */
#if H5_SIZEOF_UINT_LEAST32_T != 0
const PredType PredType::NATIVE_UINT_LEAST32( H5T_NATIVE_UINT_LEAST32 );
#endif /* H5_SIZEOF_UINT_LEAST32_T */

#if H5_SIZEOF_INT_LEAST64_T != 0
const PredType PredType::NATIVE_INT_LEAST64( H5T_NATIVE_INT_LEAST64 );
#endif /* H5_SIZEOF_INT_LEAST64_T */
#if H5_SIZEOF_UINT_LEAST64_T != 0
const PredType PredType::NATIVE_UINT_LEAST64( H5T_NATIVE_UINT_LEAST64 );
#endif /* H5_SIZEOF_UINT_LEAST64_T */

// FAST types
#if H5_SIZEOF_INT_FAST8_T != 0
const PredType PredType::NATIVE_INT_FAST8( H5T_NATIVE_INT_FAST8 );
#endif /* H5_SIZEOF_INT_FAST8_T */
#if H5_SIZEOF_UINT_FAST8_T != 0
const PredType PredType::NATIVE_UINT_FAST8( H5T_NATIVE_UINT_FAST8 );
#endif /* H5_SIZEOF_UINT_FAST8_T */

#if H5_SIZEOF_INT_FAST16_T != 0
const PredType PredType::NATIVE_INT_FAST16( H5T_NATIVE_INT_FAST16 );
#endif /* H5_SIZEOF_INT_FAST16_T */
#if H5_SIZEOF_UINT_FAST16_T != 0
const PredType PredType::NATIVE_UINT_FAST16( H5T_NATIVE_UINT_FAST16 );
#endif /* H5_SIZEOF_UINT_FAST16_T */

#if H5_SIZEOF_INT_FAST32_T != 0
const PredType PredType::NATIVE_INT_FAST32( H5T_NATIVE_INT_FAST32 );
#endif /* H5_SIZEOF_INT_FAST32_T */
#if H5_SIZEOF_UINT_FAST32_T != 0
const PredType PredType::NATIVE_UINT_FAST32( H5T_NATIVE_UINT_FAST32 );
#endif /* H5_SIZEOF_UINT_FAST32_T */

#if H5_SIZEOF_INT_FAST64_T != 0
const PredType PredType::NATIVE_INT_FAST64( H5T_NATIVE_INT_FAST64 );
#endif /* H5_SIZEOF_INT_FAST64_T */
#if H5_SIZEOF_UINT_FAST64_T != 0
const PredType PredType::NATIVE_UINT_FAST64( H5T_NATIVE_UINT_FAST64 );
#endif /* H5_SIZEOF_UINT_FAST64_T */
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	PredType::operator=
///\brief	Assignment operator.
///\param	rhs - IN: Reference to the predefined datatype
///\return	Reference to PredType instance
///\exception	H5::DataTypeIException
// Description
//		Makes a copy of the type on the right hand side and stores
//		the new id in the left hand side object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
PredType& PredType::operator=( const PredType& rhs )
{
    if (this != &rhs)
	copy(rhs);
    return(*this);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// These dummy functions do not inherit from DataType - they'll
// throw an DataTypeIException if invoked.
void PredType::commit( H5File& loc, const char* name )
{
   throw DataTypeIException("PredType::commit", "Error: Attempted to commit a predefined datatype.  Invalid operation!" );
}

void PredType::commit( H5File& loc, const H5std_string& name )
{
   commit( loc, name.c_str());
}

void PredType::commit( H5Object& loc, const char* name )
{
   throw DataTypeIException("PredType::commit", "Error: Attempted to commit a predefined datatype.  Invalid operation!" );
}

void PredType::commit( H5Object& loc, const H5std_string& name )
{
   commit( loc, name.c_str());
}

bool PredType::committed()
{
   throw DataTypeIException("PredType::committed", "Error: Attempting to check for commit status on a predefined datatype." );
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

// Default destructor
//--------------------------------------------------------------------------
// Function:	PredType destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
PredType::~PredType() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
