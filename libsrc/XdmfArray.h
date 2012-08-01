/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfArray_h
#define __XdmfArray_h

#include "XdmfDataDesc.h"

#define XDMF_LONGEST_LENGTH     (XdmfInt64)~0
#define XDMF_ARRAY_TAG_LENGTH  80

//! Self Describing Data Structure
/*!
This is the SuperClass for All XDMF Arrays.
XDMF Arrays allow the user to set and query the
number of elements and will automatically re-allocate
enough space. You can also set the pointer directly.

XdmfArray is a self-describing data structure. It is derived from
XdmfDataDesc so it understands the number type and the \b SHAPE
of the data. An XdmfArray is a homogeneous array (each element
of the array is the same type). Access to HDF5 files is
accomplished directly thru XdmfArrays by the XdmfHDF class.
*/

#ifndef SWIG
#include <stdlib.h>
#endif

class XDMF_EXPORT XdmfArray : public XdmfDataDesc {

public:
  XdmfArray();
#ifndef SWIG
  XdmfArray( XdmfInt32 NumberType );
  XdmfArray( XdmfInt32 NumberType, XdmfLength Length );
#endif
  ~XdmfArray();

  XdmfConstString GetClassName( void ) { return ( "XdmfArray" ) ; };

//! Get the \b NAME of the array
  XdmfString    GetTagName( void );
//! Get the length ( in bytes ) of the current data array
  XdmfLength GetCoreLength( ) { return ( this->GetElementSize() * this->GetNumberOfElements() ) ; } ;

  XdmfInt32  Allocate( void );

//! Allow for automatic allocation of data buffer
  XdmfSetValueMacro(AllowAllocate, XdmfBoolean);
  XdmfGetValueMacro(AllowAllocate, XdmfBoolean);

//! Overloaded SetShape to Allocate space
  XdmfInt32       SetShape( XdmfInt32 Rank, XdmfInt64 *Dimensions );
  XdmfInt32       SetShapeFromString( XdmfConstString Dimensions );
  XdmfInt32       SetShapeFromSelection( XdmfDataDesc *DataDesc);
  XdmfInt32  SetNumberOfElements( XdmfInt64 Length ) { 
        return( this->SetShape( 1, &Length ) );
        };

//! Reshape without changing (allocating) number of elements
  XdmfInt32       ReformFromString( XdmfConstString Dimensions );
  XdmfInt32       ReformFromSelection( XdmfDataDesc *DataDesc);


#ifndef SWIG
  XdmfInt32  Reform( XdmfInt32 Rank, XdmfInt64 *Dimensions );
  XdmfInt32  Reform( XdmfDataDesc *DataDesc );
  XdmfInt32  CopyShape( hid_t DataSpace );
#endif
  XdmfInt32  CopyShape( XdmfDataDesc *DataDesc );

//! Get the undelying data for fast access \b CAUTION !!
  XdmfPointer  GetDataPointer( XdmfInt64 Index  = 0 );

  void    SetDataPointer( XdmfPointer Pointer ){
        if( this->DataIsMine && this->DataPointer ){
          free( this->DataPointer );
          }
        this->DataPointer = Pointer;
        this->DataIsMine = 0;
        }

  void    Reset( XdmfInt32 Free=0 ){
        // XdmfInt64 Length = 1;
        if( Free && this->DataIsMine && this->DataPointer ){
          free( this->DataPointer );
          }
        this->DataPointer = 0;
        this->DataIsMine = 1;
        // this->Reform(1, &Length);
        }

/*! Methods to Set Values of Elements
*/
  XdmfInt32  SetValueFromFloat64( XdmfInt64 Index, XdmfFloat64 Value );
  XdmfInt32  SetValueFromInt64( XdmfInt64 Index, XdmfInt64 Value );


XdmfInt32 SetValues( XdmfInt64 Index, XdmfArray *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ValuesStart = 0,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

#ifndef SWIG
/*! The Following Methods are NOT directly available via SWIG
*/
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfUInt8 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfUInt16 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfUInt32 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfInt8 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfInt16 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfInt32 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfInt64 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfFloat32 Value );
  XdmfInt32  SetValue( XdmfInt64 Index, XdmfFloat64 Value );

  XdmfInt32  SetValues( XdmfInt64 Index, XdmfUInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfUInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfUInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfUInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfUInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfUInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

  XdmfInt32  GetValues( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );
#endif
  XdmfInt32  SetValues( XdmfInt64 Index, XdmfConstString Values,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 );

/*!
  Convenience Functions so they're "wrapped" properly
*/

  XdmfInt32  GetValuesAsInt8( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return ( GetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };
  XdmfInt32  SetValuesFromInt8( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return( SetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };


  XdmfInt32  GetValuesAsInt32( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return ( GetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };
  XdmfInt32  SetValuesFromInt32( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return( SetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };


  XdmfInt32  GetValuesAsInt64( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return ( GetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };
  XdmfInt32  SetValuesFromInt64( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return( SetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };


  XdmfInt32  GetValuesAsFloat32( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return ( GetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };
  XdmfInt32  SetValuesFromFloat32( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return( SetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };


  XdmfInt32  GetValuesAsFloat64( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return ( GetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };
  XdmfInt32  SetValuesFromFloat64( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride = 1,
        XdmfInt64 ValuesStride = 1 ) {
    return( SetValues( Index, Values, NumberOfValues, ArrayStride, ValuesStride));
        };

#ifndef SWIG
  XdmfArray  & operator=( XdmfArray &Array );
  XdmfArray  & operator=( XdmfFloat64 Value );
  XdmfArray  & operator+( XdmfArray &Array );
  XdmfArray  & operator+( XdmfFloat64 Value );
  XdmfArray  & operator-( XdmfArray &Array );
  XdmfArray  & operator-( XdmfFloat64 Value );
  XdmfArray  & operator*( XdmfArray &Array );
  XdmfArray  & operator*( XdmfFloat64 Value );
  XdmfArray  & operator/( XdmfArray &Array );
  XdmfArray  & operator/( XdmfFloat64 Value );

  XdmfArray  & operator+=( XdmfArray &Array ){
        *this = *this + Array;
        return( *this );
        };
  XdmfArray  & operator+=( XdmfFloat64 Value ){
        *this = *this + Value;
        return( *this );
        };
  XdmfArray  & operator-=( XdmfArray &Array ){
        *this = *this - Array;
        return( *this );
        };
  XdmfArray  & operator-=( XdmfFloat64 Value ){
        *this = *this - Value;
        return( *this );
        };
  XdmfArray  & operator*=( XdmfArray &Array ){
        *this = *this * Array;
        return( *this );
        };
  XdmfArray  & operator*=( XdmfFloat64 Value ){
        *this = *this * Value;
        return( *this );
        };
  XdmfArray  & operator/=( XdmfArray &Array ){
        *this = *this / Array;
        return( *this );
        };
  XdmfArray  & operator/=( XdmfFloat64 Value ){
        *this = *this / Value;
        return( *this );
        };


  XdmfInt32 CopyCompound( XdmfPointer ArrayPointer,
      XdmfInt32 ArrayType,
      XdmfInt64 ArrayStride,
      XdmfPointer ValuePointer,
      XdmfInt32  ValueType,
      XdmfInt64 ValueStride,
      XdmfInt32 Direction,
      XdmfInt64 NumberOfValues );
#endif
/*! Methods to Get Values of Elements
*/
  XdmfFloat64  GetValueAsFloat64( XdmfInt64 Index );
  XdmfFloat32  GetValueAsFloat32( XdmfInt64 Index );
  XdmfInt64  GetValueAsInt64( XdmfInt64 Index );
  XdmfInt32  GetValueAsInt32( XdmfInt64 Index );
  XdmfInt16  GetValueAsInt16( XdmfInt64 Index );
  XdmfInt8  GetValueAsInt8( XdmfInt64 Index );
  XdmfString  GetValues( XdmfInt64 Index = 0,
        XdmfInt64 NumberOfValues = 0,
        XdmfInt64 ArrayStride = 1);



  XdmfInt32  Generate( XdmfFloat64 StartValue,
        XdmfFloat64 EndValue,
        XdmfInt64 StartIndex = 0,
        XdmfInt64 EndIndex = 0 );


//! Make Exact Copy of Existing Array
#ifndef SWIG
  XdmfArray *Clone( XdmfArray *Indexes );
#endif
//! Make Exact Copy of Existing Array
  XdmfArray *Clone( XdmfLength Start = 0, XdmfLength End = 0);
//! Get Reference to Section of Array
  XdmfArray *Reference( XdmfLength Start = 0 , XdmfLength End = 0);

//! Get Max as a Float
  XdmfFloat64 GetMaxAsFloat64( void );
//! Get Min as a Float
  XdmfFloat64 GetMinAsFloat64( void );

//! Get Max  as a Int
  XdmfInt64 GetMaxAsInt64( void );
//! Get Min  as a Int
  XdmfInt64 GetMinAsInt64( void );

//! Get Mean
  XdmfFloat64 GetMean( void );

//! Blindly copy in chars
  void SetDataFromChars( XdmfString String ) {
    this->SetNumberOfElements( strlen( String ) + 1 );
    strcpy((XdmfString)this->GetDataPointer(), String );
    };
//! Return as if its a XdmfString 
  XdmfString GetDataPointerAsCharPointer( void ) {
    return( (XdmfString)this->GetDataPointer() );
    };

//! Copy Data From One Array to Another
//  XdmfPointer MemCopy( XdmfLength StartOffset,
//    XdmfLength NumberOfElemnts,
//    XdmfPointer DataPointer,
//    XdmfLength Stride = 1 );

//! Copy Data From a Scripting Variable
//  XdmfInt32 CopyFromScriptVariable( XdmfScriptVariablePointer Pointer,
//    XdmfLength StartOffset,
//    XdmfLength NumberOfElements ) {
//
//    XdmfDebug("Copy " << NumberOfElements <<
//      " to " << this->GetVoidPointer( StartOffset ) <<
//      " from " << Pointer );
//    memcpy( this->GetVoidPointer( StartOffset ),
//        Pointer,
//        this->Precision * NumberOfElements );  
//    return ( XDMF_SUCCESS );
//    };
//! Copy Data To a Scripting Variable
//  XdmfInt32 CopyToScriptVariable( XdmfScriptVariablePointer Pointer,
//    XdmfLength StartOffset,
//    XdmfLength NumberOfElements ) {
//
//    XdmfDebug("Copy " << NumberOfElements <<
//      " from " << this->GetVoidPointer( StartOffset ) <<
//      " to " << Pointer );
//    memcpy( Pointer, this->GetVoidPointer( StartOffset ),
//        this->Precision * NumberOfElements );  
//    return ( XDMF_SUCCESS );
//    };

protected:
  XdmfPointer  DataPointer;
  XdmfBoolean  AllowAllocate;
  XdmfBoolean  DataIsMine;
  char    TagName[XDMF_ARRAY_TAG_LENGTH];

  void    AddArrayToList( void );
};

#ifndef DOXYGEN_SKIP
typedef struct {
  char    *name;
  XdmfLength  timecntr;
  XdmfArray  *Array;
  } XdmfArrayList;

XDMF_EXPORT  XdmfArray  *TagNameToArray( XdmfString TagName );
XDMF_EXPORT  void    PrintAllXdmfArrays( void );
XDMF_EXPORT  XdmfArray  *GetNextOlderArray( XdmfLength Age, XdmfLength *AgeOfArray = NULL  );
XDMF_EXPORT  XdmfLength  GetCurrentArrayTime( void );
#endif /* DOXYGEN_SKIP */

#endif // __XdmfArray_h
