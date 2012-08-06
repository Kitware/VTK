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
#include "XdmfHDFSupport.h"

XdmfConstString
XdmfTypeToClassString( XdmfInt32 XdmfType ) {
  switch( XdmfType ){
    case XDMF_INT8_TYPE :
      return( "Char");
    case XDMF_UINT8_TYPE :
      return( "UChar");
    case XDMF_INT16_TYPE :
      return( "Short");
    case XDMF_UINT16_TYPE :
      return( "UShort");
    case XDMF_UINT32_TYPE :
      return "UInt";
    case XDMF_INT32_TYPE :
    case XDMF_INT64_TYPE :
      return( "Int");
    case XDMF_FLOAT32_TYPE :
    case XDMF_FLOAT64_TYPE :
      return( "Float" );
    default :
      break;
    }
return( "Compound" );
}

XdmfInt32
StringToXdmfType( XdmfConstString TypeName ) {

if( STRCASECMP( TypeName, "XDMF_UINT8_TYPE" ) == 0 ) return( XDMF_UINT8_TYPE );
if( STRCASECMP( TypeName, "XDMF_UINT16_TYPE" ) == 0 ) return( XDMF_UINT16_TYPE );
if( STRCASECMP( TypeName, "XDMF_UINT32_TYPE" ) == 0 ) return( XDMF_UINT32_TYPE );
if( STRCASECMP( TypeName, "XDMF_INT8_TYPE" ) == 0 ) return( XDMF_INT8_TYPE );
if( STRCASECMP( TypeName, "XDMF_INT16_TYPE" ) == 0 ) return( XDMF_INT16_TYPE );
if( STRCASECMP( TypeName, "XDMF_INT32_TYPE" ) == 0 ) return( XDMF_INT32_TYPE );
if( STRCASECMP( TypeName, "XDMF_INT64_TYPE" ) == 0 ) return( XDMF_INT64_TYPE );
if( STRCASECMP( TypeName, "XDMF_FLOAT32_TYPE" ) == 0 ) return( XDMF_FLOAT32_TYPE );
if( STRCASECMP( TypeName, "XDMF_FLOAT64_TYPE" ) == 0 ) return( XDMF_FLOAT64_TYPE );
if( STRCASECMP( TypeName, "XDMF_COMPOUND_TYPE" ) == 0 ) return( XDMF_COMPOUND_TYPE );
return( XDMF_FAIL );
}

XdmfConstString
XdmfTypeToString( XdmfInt32 XdmfType ) {
  switch( XdmfType ){
    case XDMF_UINT8_TYPE :
      return( "XDMF_UINT8_TYPE" );
    case XDMF_UINT16_TYPE :
      return( "XDMF_UINT16_TYPE" );
    case XDMF_UINT32_TYPE :
      return( "XDMF_UINT32_TYPE" );
    case XDMF_INT8_TYPE :
      return( "XDMF_INT8_TYPE" );
    case XDMF_INT16_TYPE :
      return( "XDMF_INT16_TYPE" );
    case XDMF_INT32_TYPE :
      return( "XDMF_INT32_TYPE" );
    case XDMF_INT64_TYPE :
      return( "XDMF_INT64_TYPE");
    case XDMF_FLOAT32_TYPE :
      return( "XDMF_FLOAT32_TYPE" );
    case XDMF_FLOAT64_TYPE :
      return( "XDMF_FLOAT64_TYPE" );
    default :
      break;
    }
return( "XDMF_COMPOUND_TYPE" );
}

hid_t
XdmfTypeToHDF5Type( XdmfInt32 XdmfType ) {
  switch( XdmfType ){
    case XDMF_UINT8_TYPE :
      return( H5T_NATIVE_UINT8 );
    case XDMF_UINT16_TYPE :
      return( H5T_NATIVE_UINT16 );
    case XDMF_UINT32_TYPE :
      return( H5T_NATIVE_UINT32 );
    case XDMF_INT8_TYPE :
      return( H5T_NATIVE_INT8 );
    case XDMF_INT16_TYPE :
      return( H5T_NATIVE_INT16 );
    case XDMF_INT32_TYPE :
      return( H5T_NATIVE_INT32 );
    case XDMF_INT64_TYPE :
      return( H5T_NATIVE_INT64 );
    case XDMF_FLOAT32_TYPE :
      return( H5T_NATIVE_FLOAT );
    case XDMF_FLOAT64_TYPE :
      return( H5T_NATIVE_DOUBLE );
    default :
      break;
    }
return( H5T_COMPOUND );
}

XdmfInt32
HDF5TypeToXdmfType( hid_t HDF5Type ) {
  switch( H5Tget_class( HDF5Type ) ) {
    case H5T_INTEGER :
      if ( H5Tget_sign(HDF5Type) )
        {
        switch( H5Tget_size( HDF5Type ) ) 
          {
        case 1 :
          return( XDMF_INT8_TYPE );
        case 2 :
          return( XDMF_INT16_TYPE );
        case 4 :
          return( XDMF_INT32_TYPE );
        case 8 :
          return( XDMF_INT64_TYPE );
        default :
          break;
          }
        }
      else
        {
        switch( H5Tget_size( HDF5Type ) ) 
          {
        case 1 :
          return( XDMF_UINT8_TYPE );
        case 2 :
          return( XDMF_UINT16_TYPE );
        case 4 :
          return( XDMF_UINT32_TYPE );
        default :
          break;
          }
        }
      break;
    case H5T_FLOAT :
      switch( H5Tget_size( HDF5Type ) ) {
        case 4 :
          return( XDMF_FLOAT32_TYPE );
        case 8 :
          return( XDMF_FLOAT64_TYPE );
        default :
          break;
        }
      break;
    default :
      return( XDMF_COMPOUND_TYPE );
      break;
    }
return( XDMF_FAIL );
}

