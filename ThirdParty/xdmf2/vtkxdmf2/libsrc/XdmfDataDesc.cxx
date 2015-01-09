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
#include "XdmfDataDesc.h"

#include <cstring>
#include <stdlib.h>

namespace xdmf2
{

XdmfDataDesc::XdmfDataDesc() {
#ifndef XDMF_MEMORY_DEBUG
  H5dont_atexit();
#endif
  this->DataSpace = H5I_BADID;
  this->DataType = H5Tcopy(H5T_NATIVE_FLOAT);
  this->SelectionType = XDMF_SELECTALL;
  this->NextOffset = 0;
  this->Rank = 1;
  this->Compression = 0;
  this->ShapeString = 0;
  this->HeavyDataSetName = 0;
  this->DsmBuffer = NULL;
}

XdmfDataDesc::~XdmfDataDesc() {
H5E_BEGIN_TRY {
  H5Tclose( this->DataType );
  if(( this->DataSpace != H5S_ALL ) &&
      ( this->DataSpace != H5I_BADID )){
           H5Sclose( this->DataSpace );
    this->DataSpace = H5I_BADID;
  }
} H5E_END_TRY;
  delete [] this->ShapeString;
  delete [] this->HeavyDataSetName;
}

XdmfInt32
XdmfDataDesc::GetHyperSlab( XdmfInt64 *start, XdmfInt64 *stride, XdmfInt64 *count ) {

XdmfInt32  i, rank = this->Rank;

if( this->SelectionType != XDMF_HYPERSLAB ){
  return( XDMF_FAIL );
  }
for( i = 0 ; i < rank ; i++ ){
  if( start ) {
    *start++ = this->Start[i];
    }
  if( stride ) {
    *stride++ = this->Stride[i];
    }
  if( count ) {
    *count++ = this->Count[i];
    }
  }
return( rank );
}

XdmfConstString
XdmfDataDesc::GetHyperSlabAsString( void ){
ostrstream   StringOutput;
XdmfString Ptr;
static XdmfString Result = NULL;
XdmfInt32  i, rank;
XdmfInt64  start[ XDMF_MAX_DIMENSION ];
XdmfInt64  stride[ XDMF_MAX_DIMENSION ];
XdmfInt64  count[ XDMF_MAX_DIMENSION ];

rank = this->GetHyperSlab( start, stride, count );
if( rank == XDMF_FAIL ) {
  return( NULL );
  }
for( i = 0 ; i < rank ; i++ ){
  StringOutput << XDMF_64BIT_CAST start[i] << " ";
  }
for( i = 0 ; i < rank ; i++ ){
  StringOutput << XDMF_64BIT_CAST stride[i] << " ";
  }
for( i = 0 ; i < rank ; i++ ){
  StringOutput << XDMF_64BIT_CAST count[i] << " ";
  }
StringOutput << ends;
Ptr = StringOutput.str();
if( Result != NULL ) delete [] Result;
Result = new char[ strlen( Ptr ) + 2 ];
strcpy( Result, Ptr );
delete [] Ptr;
return( Result );
}

XdmfInt32
XdmfDataDesc::CopySelection( XdmfDataDesc *Desc ){

if( Desc->SelectionType == XDMF_SELECTALL ) { 
  return( XDMF_SUCCESS );
  }
if( Desc->GetSelectionType() == XDMF_HYPERSLAB ){
  XdmfInt64  start[ XDMF_MAX_DIMENSION ];
  XdmfInt64  stride[ XDMF_MAX_DIMENSION ];
  XdmfInt64  count[ XDMF_MAX_DIMENSION ];
  
  this->Rank = Desc->GetHyperSlab( start, stride, count );
  this->SelectHyperSlab( start, stride, count );
} else {
  XdmfInt64  NumberOfCoordinates;
  XdmfInt64  *Coordinates;


  NumberOfCoordinates = Desc->GetSelectionSize();
  Coordinates = Desc->GetCoordinates();
  this->SelectCoordinates( NumberOfCoordinates, Coordinates );
  delete Coordinates;
  }
return( XDMF_SUCCESS );
}


XdmfInt32
XdmfDataDesc::CopyShape( XdmfDataDesc *DataDesc ) {
XdmfInt32  Status;

Status = this->CopyShape( DataDesc->GetDataSpace() );
// Do This Manually
// if( Status == XDMF_SUCCESS ){
//   Status = this->CopySelection( DataDesc );
//    }
return( Status );
};

XdmfInt32
XdmfDataDesc::CopyType( hid_t dataType ) {
if( this->DataType != H5I_BADID ) {
  H5Tclose( this->DataType );
  }
this->DataType = H5Tcopy( dataType );
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDataDesc::CopyShape( hid_t dataSpace ) {

hsize_t      i, HRank;
hsize_t      HDimension[ XDMF_MAX_DIMENSION ];
XdmfInt64    Dimensions[ XDMF_MAX_DIMENSION ];
XdmfInt32    Status;

HRank = H5Sget_simple_extent_ndims( dataSpace );
H5Sget_simple_extent_dims( dataSpace, HDimension, NULL );
for( i = 0 ; i < HRank ; i++ ){
  Dimensions[i] = HDimension[i];
  }
Status =  this->SetShape( HRank, Dimensions );
if( Status > 0 ){
  return( XDMF_SUCCESS );
  }
return( XDMF_FAIL );
}


XdmfInt32
XdmfDataDesc::GetShape( XdmfInt64 *Dimensions ) {
hsize_t      i, HRank;
hsize_t      HDimension[ XDMF_MAX_DIMENSION ];

HRank = H5Sget_simple_extent_ndims( this->DataSpace );
H5Sget_simple_extent_dims( this->DataSpace, HDimension, NULL );
for( i = 0 ; i < HRank ; i++ ){
  Dimensions[i] = HDimension[i];
  }
return( HRank );
}

XdmfConstString
XdmfDataDesc::GetShapeAsString( void ) {
ostrstream   StringOutput;
XdmfInt64  i, rank, Dimensions[ XDMF_MAX_DIMENSION ];

rank = this->GetShape( Dimensions );
for( i = 0 ; i < rank - 1 ; i++ ){
  StringOutput << XDMF_64BIT_CAST Dimensions[i] << " ";
  }
StringOutput << XDMF_64BIT_CAST Dimensions[i] << ends;
this->SetShapeString(StringOutput.str());
StringOutput.rdbuf()->freeze(0);
return( this->ShapeString );
}

XdmfInt32
XdmfDataDesc::SetShape( XdmfInt32 rank, XdmfInt64 *Dimensions ) {
XdmfInt32    i;
hsize_t      HRank;
hsize_t      HDimension[ XDMF_MAX_DIMENSION ];

if( this->Rank <= 0 ) {
  return( XDMF_FAIL );
  }


if( ( this->DataSpace == H5I_BADID )  || ( this->DataSpace == H5S_ALL ) ) {
  XdmfDebug("Createing new HDF5 DataSpace");
  this->DataSpace = H5Screate( H5S_SIMPLE );
  if( this->DataSpace < 0 ) {
    XdmfErrorMessage( "Can't Create  DataSpace" );
    return( H5I_BADID );
    }
} else {
  HRank = H5Sget_simple_extent_ndims( this->DataSpace );
  if( HRank != (hsize_t)rank ){
    XdmfDebug( "Current Rank " << (int)HRank << " Requested Rank " << rank );
    XdmfDebug( "Data Space Rank Change After Creation" );
  // Work around for ?bug? in HDF
        // when you increase rank
  if(( this->DataSpace != H5S_ALL ) &&
      ( this->DataSpace != H5I_BADID )){
  XdmfDebug("Closing Space");
           H5Sclose( this->DataSpace );
    this->DataSpace = H5I_BADID;
        }
  this->DataSpace = H5Screate( H5S_SIMPLE );
  if( this->DataSpace < 0 ) {
    XdmfErrorMessage( "Can't Create  DataSpace" );
    return( H5I_BADID );
    }
  }
}

this->Rank = HRank = rank;
XdmfDebug("Shape : Rank = " << (int)HRank);
for( i = 0 ; i < rank ; i++ ) {
  XdmfDebug("  Dimension[" << i << "] = " << XDMF_64BIT_CAST Dimensions[i] );
  this->Count[i] = this->Dimension[i] = HDimension[i] = Dimensions[i];
  this->Start[i] = 0;
  this->Stride[i] = 1;
  }

H5Sset_extent_simple( this->DataSpace, HRank, HDimension, NULL );
H5Sselect_all( this->DataSpace );

XdmfDebug("Finished Setting Shape");
return( this->DataSpace );
}

XdmfInt32
XdmfDataDesc::SelectAll( void ) {
XdmfInt32 i;

  // Update Rank and Dimensions
  this->GetNumberOfElements();

for( i = 0 ; i < this->Rank ; i++ ) {
  this->Start[i] = 0;
  this->Stride[i] = 1;
  this->Count[i] = this->Dimension[i];
  }
  
  H5Sselect_all( this->DataSpace );
  this->SelectionType = XDMF_SELECTALL;
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDataDesc::SelectHyperSlab(  XdmfInt64 *start, XdmfInt64 *stride, XdmfInt64 *count ) {

XdmfInt32  i;
XdmfInt64  Dimensions[ XDMF_MAX_DIMENSION ];
herr_t    status;

this->GetShape( Dimensions );
for( i = 0 ; i < this->Rank ; i++ ) {
  if( start ) {
    this->Start[i] = start[ i ];  
  } else {
    this->Start[i] = 0;
  }
  if( stride ){
    this->Stride[i] = stride[ i ];  
  } else {
    this->Stride[i] = 1;
  }
  if( count ) {
    this->Count[i] = count[ i ];  
  } else {
    this->Count[i] = (( Dimensions[i] - this->Start[i] - 1 ) / this->Stride[i]) + 1;
  }
  XdmfDebug("Dim[" << i << "] = " << XDMF_64BIT_CAST this->Dimension[i]  << 
    " Start Stride Count = " <<
    XDMF_64BIT_CAST this->Start[i] << " " <<
    XDMF_64BIT_CAST this->Stride[i] << " " <<
    XDMF_64BIT_CAST this->Count[i] );
  }
this->SelectionType = XDMF_HYPERSLAB;
status = H5Sselect_hyperslab( this->DataSpace,
       H5S_SELECT_SET,
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&((H5_VERS_MINOR>6)||((H5_VERS_MINOR==6)&&(H5_VERS_RELEASE>=4))))
       (const hsize_t *)this->Start, (const hsize_t *)this->Stride, (const hsize_t *)this->Count,
#else
       (const hssize_t *)this->Start, (const hsize_t *)this->Stride, (const hsize_t *)this->Count,
#endif
       NULL);
if( status >= 0 ) {
  return( XDMF_SUCCESS );
}
return( XDMF_FAIL );
}

XdmfInt32
XdmfDataDesc::SelectCoordinates(  XdmfInt64 NumberOfElements, XdmfInt64 *Coordinates ){

/*#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&((H5_VERS_MINOR>6)||((H5_VERS_MINOR==6)&&(H5_VERS_RELEASE>=4))))
hsize_t  *HCoordinates;
#else
*/
hssize_t  *HCoordinates;
/* 
#endif
*/

XdmfInt32  status;
XdmfInt64  i, rank = this->Rank;
hssize_t  Length = NumberOfElements * rank;
hssize_t  NElements = Length / MAX( 1, rank );


if( this->Rank <= 0 ) {
        return( XDMF_FAIL );
        }

this->SelectionType = XDMF_COORDINATES;
XdmfDebug(" Selecting " << (int)NElements << " elements" );
HCoordinates = new hssize_t[ Length ];
for( i = 0 ; i < Length ; i++ ){
  HCoordinates[i] = Coordinates[i];
  }
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&((H5_VERS_MINOR>6)||((H5_VERS_MINOR==6)&&(H5_VERS_RELEASE>=4))))
# if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&((H5_VERS_MINOR>6)||((H5_VERS_MINOR==6)&&(H5_VERS_RELEASE>=7))))
status = H5Sselect_elements( this->DataSpace,
        H5S_SELECT_SET,
         NElements,
         ( const hsize_t *)HCoordinates);
# else
status = H5Sselect_elements( this->DataSpace,
        H5S_SELECT_SET,
         NElements,
         ( const hsize_t **)HCoordinates);
# endif
#else
status = H5Sselect_elements( this->DataSpace,
        H5S_SELECT_SET,
         NElements,
         ( const hssize_t **)HCoordinates);
#endif
if( status < 0 ) {
  return( XDMF_FAIL );
}
return ( XDMF_SUCCESS );
}

XdmfInt64
XdmfDataDesc::GetNumberOfElements( void ) {
hsize_t    i, Dimensions[ XDMF_MAX_DIMENSION ];
XdmfInt64  Nelements = 0;
XdmfInt32  rank;

if(this->DataSpace == H5I_BADID) return(0);
this->Rank = rank = H5Sget_simple_extent_ndims(this->DataSpace );
H5Sget_simple_extent_dims( this->DataSpace, Dimensions, NULL );
if(rank) {
  Nelements = this->Dimension[0] = Dimensions[0];
  for( i = 1 ; i < (hsize_t)rank ; i++ ){
    this->Dimension[i] = Dimensions[i];
    Nelements *= Dimensions[i];
    }
}
return( Nelements );
}

XdmfInt64
XdmfDataDesc::GetElementSize( void ) {
  return( H5Tget_size( this->DataType ) );
}

XdmfInt64
XdmfDataDesc::GetSelectionSize( void ) {
  return( H5Sget_select_npoints( this->DataSpace ) );
}

XdmfInt32
XdmfDataDesc::SelectHyperSlabFromString(  XdmfConstString start, XdmfConstString stride, XdmfConstString count ) {
XdmfInt64  i, HStart[XDMF_MAX_DIMENSION], HStride[XDMF_MAX_DIMENSION], HCount[XDMF_MAX_DIMENSION];
istrstream   Start_ist(const_cast<char*>(start), strlen( start ) );
istrstream   stride_ist(const_cast<char*>(stride), strlen( stride ) );
istrstream   Count_ist(const_cast<char*>(count), strlen( count ) );

for( i = 0 ; i < this->Rank ; i++ ){
  if( start ){
    XDMF_READ_STREAM64(Start_ist, HStart[i]);
  } else {
    HStart[i] = 0;
    }
  if( stride ){
    XDMF_READ_STREAM64(stride_ist, HStride[i]);
  } else {
    HStride[i] = 1;
  }
  if( count ){
    XDMF_READ_STREAM64(Count_ist, HCount[i]);
  } else {
    HCount[i] = (this->Dimension[i] - HStart[i]) / HStride[i];
  }
}
return( this->SelectHyperSlab( HStart, HStride, HCount ) );
}

XdmfInt32
XdmfDataDesc::SetShapeFromString( XdmfConstString String ) {
  XdmfLength      i = 0, count = 0;
  char* NewString = new char[ strlen(String) + 1 ];
  strcpy(NewString, String);
  istrstream   ist(NewString, strlen( NewString ) );
  istrstream   counter(NewString, strlen( NewString ) );
  XdmfInt64  dummy;

  while( XDMF_READ_STREAM64(counter, dummy) ) count++;
  this->Rank = count;
  while( XDMF_READ_STREAM64(ist,dummy) ){
          this->Dimension[i] = dummy;
          i++;
          }
  delete [] NewString;
  XdmfDebug("String Contains " << this->Rank << " Dimensions" );
  return( this->SetShape( this->Rank, this->Dimension ) );

}

XdmfInt32
XdmfDataDesc::SelectCoordinatesFromString( XdmfConstString String ) {
  XdmfInt32  Status;
  XdmfLength      i = 0, count = 0;
  istrstream   ist(const_cast<char*>(String), strlen( String ) );
  istrstream   counter(const_cast<char*>(String), strlen( String ) );
  XdmfInt64  dummy, *Coordinates;

  while( XDMF_READ_STREAM64(counter, dummy) ) count++;
  Coordinates = new XdmfInt64[ count + 1 ];
  while( XDMF_READ_STREAM64(ist, dummy) ){
   Coordinates[i] = dummy;
          i++;
          }
  XdmfDebug("String Contains " << XDMF_64BIT_CAST count << " Coordinates" );
  Status = this->SelectCoordinates( count / this->Rank, Coordinates );
  delete [] Coordinates;
  return( Status );

}

XdmfInt32
XdmfDataDesc::SetNumberType( XdmfInt32 numberType, XdmfInt64 CompoundSize ) {

if( this->DataType != H5I_BADID ) {
  H5Tclose( this->DataType );
  }
if( numberType == XDMF_COMPOUND_TYPE ) {
   this->DataType = H5Tcreate( H5T_COMPOUND, CompoundSize );
} else {
  this->DataType =  H5Tcopy( XdmfTypeToHDF5Type( numberType ) );
}
if ( this->DataType < 0 ) {
  XdmfErrorMessage("Error Creating Data Type");
  this->DataType = H5I_BADID;
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDataDesc::SetNumberTypeFromString( XdmfConstString NumberTypeString, XdmfInt64 CompoundSize ) {
XdmfInt32 numberType;

numberType = StringToXdmfType( NumberTypeString );
return( this->SetNumberType( numberType, CompoundSize ) );
}

XdmfInt32
XdmfDataDesc::GetNumberType( void ) {
  return( HDF5TypeToXdmfType( this->DataType ) );
}

XdmfConstString
XdmfDataDesc::GetNumberTypeAsString() {

XdmfInt32  Type;

Type = HDF5TypeToXdmfType( this->DataType );
if( Type > 0 ){
  return( XdmfTypeToString( Type ) );
}
return("UNKNOWN");
}

XdmfInt64
XdmfDataDesc::GetNumberOfMembers( void ) {

return( H5Tget_nmembers( this->DataType ) );
}

XdmfInt32
XdmfDataDesc::GetMemberType( XdmfInt64 Index ) {
XdmfInt32  RetVal;
hid_t    dataType;

if( Index >  ( H5Tget_nmembers( this->DataType ) - 1 ) ){
  XdmfErrorMessage("Index is Greater than Number of Members");
  return( 0 );
  }
dataType =  H5Tget_member_type( this->DataType, Index );
RetVal = HDF5TypeToXdmfType( dataType ); 
if( RetVal == XDMF_COMPOUND_TYPE ) {
  hid_t  Super;

  Super = H5Tget_super( dataType );
  RetVal = HDF5TypeToXdmfType( Super );
}
H5Tclose( dataType );
return( RetVal );
}

XdmfConstString
XdmfDataDesc::GetMemberTypeAsString( XdmfInt64 Index ) {

return( XdmfTypeToString( this->GetMemberType( Index ) ) );
}

XdmfInt64
XdmfDataDesc::GetMemberOffset( XdmfInt64 Index ) {
XdmfInt64  RetVal;

if( Index >  ( H5Tget_nmembers( this->DataType ) - 1 ) ){
  XdmfErrorMessage("Index is Greater than Number of Members");
  return( 0 );
  }
RetVal =  H5Tget_member_offset( this->DataType, Index );
return( RetVal );
}

XdmfInt32
XdmfDataDesc::GetMemberShape( XdmfInt64 Index, XdmfInt64 *Dimensions ) {
XdmfInt32  i, rank, dataType;
hsize_t     Dims[XDMF_MAX_DIMENSION];

if( Index >  ( H5Tget_nmembers( this->DataType ) - 1 ) ){
  XdmfErrorMessage("Index is Greater than Number of Members");
  return( 0 );
  }
dataType =  H5Tget_member_type( this->DataType, Index );
if( HDF5TypeToXdmfType(dataType) == XDMF_COMPOUND_TYPE ) {
  rank = H5Tget_array_ndims( dataType );
  if( rank <= 0 ){
    H5Tclose(dataType);
    return( XDMF_FAIL );
    }
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
  H5Tget_array_dims2(dataType, Dims);
#else
  H5Tget_array_dims(dataType, Dims, NULL );
#endif
  for( i = 0 ; i < rank ; i++ ){
    Dimensions[i] = Dims[i];
  }
} else {
  rank =  1;
  Dimensions[0] = 1;
}
H5Tclose(dataType);
return( rank );
}

XdmfConstString
XdmfDataDesc::GetMemberShapeAsString( XdmfInt64 Index ){

static char    ReturnString[ 80 ];
ostrstream  ReturnStream( ReturnString, 80 );
XdmfInt64  Dimensions[XDMF_MAX_DIMENSION];
XdmfInt32  i, rank;

rank = this->GetMemberShape( Index, Dimensions );
if( rank == XDMF_FAIL ) {
  XdmfErrorMessage("Error Getting Member Shape");
  return( NULL );
  }
ReturnString[0] = '0';
for( i = 0 ; i < rank ; i++ ){
  ReturnStream << XDMF_64BIT_CAST Dimensions[i] << " ";
  }
ReturnStream << ends;
return( ReturnString );
}

XdmfInt64
XdmfDataDesc::GetMemberLength( XdmfInt64 Index ) {
XdmfInt64  Length, Dimensions[XDMF_MAX_DIMENSION];
XdmfInt32  i, rank;

rank = this->GetMemberShape( Index, Dimensions );
if( rank == XDMF_FAIL ) {
  XdmfErrorMessage("Error Getting Member Shape");
  return( 0 );
  }
Length = 1;
for( i = 0 ; i < rank ; i++ ){
  Length *= Dimensions[i];
  }
return( Length );
}

XdmfInt64
XdmfDataDesc::GetMemberSize( XdmfInt64 Index ) {
hsize_t    Length;
XdmfInt64  RetVal = 1;
//hsize_t     Dims[XDMF_MAX_DIMENSION];

if( Index >  ( H5Tget_nmembers( this->DataType ) - 1 ) ){
  XdmfErrorMessage("Index is Greater than Number of Members");
  return( 0 );
  }
hid_t dataType = H5Tget_member_type( this->DataType, Index );
Length =  H5Tget_size(dataType);
H5Tclose(dataType);
if( Length <= 0 ){
  XdmfErrorMessage("Error Getting Length");
  RetVal = XDMF_FAIL;
} else {
  RetVal = Length;
}
return( RetVal );
}

XdmfConstString
XdmfDataDesc::GetMemberName( XdmfInt64 Index ) {

XdmfString Name;
static char  MemberName[256];

if( Index >  ( H5Tget_nmembers( this->DataType ) - 1 ) ){
  XdmfErrorMessage("Index is Greater than Number of Members");
  return( NULL );
  }
Name = H5Tget_member_name( this->DataType, Index );
strcpy( MemberName, Name );
free( Name );
return( MemberName );
}



XdmfInt32
XdmfDataDesc::AddCompoundMemberFromString( XdmfConstString Name,
    XdmfConstString NumberTypeString,
    XdmfConstString Shape,
    XdmfInt64 Offset ){

XdmfInt32  i, rank = 0, numberType;
XdmfInt64  Dim, Dimensions[XDMF_MAX_DIMENSION];
istrstream  istr( const_cast<char*>(Shape), strlen(Shape) );

numberType = StringToXdmfType( NumberTypeString );
i = 0;
while( XDMF_READ_STREAM64(istr, Dim) ){
  rank++;
  Dimensions[i++] = Dim;
  }
return( this->AddCompoundMember( Name, numberType, rank, Dimensions, Offset) );
}

XdmfInt32
XdmfDataDesc::AddCompoundMember( XdmfConstString Name,
    XdmfInt32 numberType,
    XdmfInt32 rank,
    XdmfInt64 *Dimensions,
    XdmfInt64 Offset ){

herr_t    status;
XdmfInt32  i, HNumberType;
XdmfInt64  One = 1;
XdmfInt64  Size;

if( Offset == 0 ){
  Offset = this->NextOffset;
  }
if( Dimensions == NULL ){
  Dimensions = &One;
  }
XdmfDebug("Inserting " << Name << " at Offset " << XDMF_64BIT_CAST Offset << " as type " << XdmfTypeToString( numberType ) );
if( this->GetNumberType() != XDMF_COMPOUND_TYPE ){
  this->SetNumberType( XDMF_COMPOUND_TYPE );
  }
HNumberType = XdmfTypeToHDF5Type( numberType);
Size = H5Tget_size( HNumberType );
if( ( rank == 1 ) && ( *Dimensions == 1 ) ){
  status = H5Tinsert( this->DataType,
    Name,
    Offset,
    HNumberType );
} else {
  hsize_t    Dims[ XDMF_MAX_DIMENSION ];

  for( i = 0; i < rank ; i++ ){
    Dims[i] = Dimensions[i];
    }
  status = H5Tinsert( this->DataType,
    Name,
    Offset,
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
    H5Tarray_create( HNumberType, rank, Dims));
#else
    H5Tarray_create( HNumberType, rank, Dims, NULL ));
#endif
}
if( status < 0 ){
  return( XDMF_FAIL );
  }
for( i = 0 ; i < rank ; i++ ){
  Size *= Dimensions[i];
  }
this->NextOffset += Size;
return( XDMF_SUCCESS );
}

XdmfConstString
XdmfDataDesc::GetSelectionTypeAsString( void ) {

switch( this->SelectionType ) {
  case XDMF_HYPERSLAB :
    return("XDMF_HYPERSLAB");
    break;
  case XDMF_COORDINATES :
    return("XDMF_COORDINATES");
    break;
  default :
    return( "Unknown" );
    break;
  }
}

XdmfInt64 *
XdmfDataDesc::GetCoordinates( XdmfInt64 start, XdmfInt64 Nelements ){

XdmfInt64 i, Total, *Coordinates = NULL;
XdmfInt32 rank = H5Sget_simple_extent_ndims(this->DataSpace );

if( this->SelectionType == XDMF_COORDINATES ){
  hsize_t  *HCoordinates;
  if( Nelements <= 0 ){
    Nelements = H5Sget_select_elem_npoints( this->DataSpace );
  }
  if ( Nelements > 0 ) {
    Total = Nelements * rank;
    HCoordinates = new hsize_t[ Total ];
    Coordinates = new XdmfInt64[ Total ];
    H5Sget_select_elem_pointlist( this->DataSpace, start, Nelements, HCoordinates);
    for( i = 0 ; i < Total ; i++ ){
      Coordinates[i] = HCoordinates[i];
      }
    delete HCoordinates;
    }
}
return( Coordinates );
}

XdmfConstString
XdmfDataDesc::GetCoordinatesAsString( XdmfInt64 start, XdmfInt64 Nelements ){

hsize_t    i;
XdmfInt32 rank = H5Sget_simple_extent_ndims(this->DataSpace );

ostrstream StringOutput;
XdmfString Ptr;
static XdmfString Result = NULL;


if( this->SelectionType == XDMF_COORDINATES ){
  if( Nelements <= 0 ){
    Nelements = H5Sget_select_elem_npoints( this->DataSpace );
  }
  if ( Nelements > 0 ) {
    hsize_t *Coords = new hsize_t[ Nelements * rank ];
    hsize_t j, *Cp = Coords;
    H5Sget_select_elem_pointlist( this->DataSpace, start, Nelements, Coords );
    for( i = 0 ; i < (hsize_t)Nelements ; i++ ){
      for( j = 0 ; j < (hsize_t)rank ; j++ ){
        StringOutput << (int)*Cp++ << " ";
        }
      }  
    delete [] Coords;
    }
}
Ptr = StringOutput.str();
if( Result != NULL ) delete [] Result;
Result = new char[ strlen( Ptr ) + 2 ];
strcpy( Result, Ptr );
delete [] Ptr;
return( Result );
}

void
XdmfDataDesc::Print( void ){
hsize_t  i, Dimensions[ XDMF_MAX_DIMENSION ];
hsize_t  Nelements = 0;
XdmfInt32 rank = 0;
if ( this->DataSpace != H5I_BADID ) {
  rank = H5Sget_simple_extent_ndims(this->DataSpace );
  }

cout << "Rank " << rank << endl;
if ( this->DataSpace != H5I_BADID ) {
  H5Sget_simple_extent_dims( this->DataSpace, Dimensions, NULL );
  }

XdmfInt32 ii;
for( ii = 0 ; ii < rank ; ii++ ) {
  cout << "Dimension[" << (int)ii << "] " << (int)Dimensions[ii] << endl;
}

cout << "Selection Type : " << this->GetSelectionTypeAsString() << endl;
if( this->SelectionType == XDMF_COORDINATES ){
  if ( this->DataSpace != H5I_BADID ) {
    Nelements = H5Sget_select_elem_npoints( this->DataSpace );
    }
  cout << "Selected Elements : " << (int)Nelements << endl;
  if ( Nelements > 0 ) {
    hsize_t *Coords = new hsize_t[ Nelements * rank ];
    hsize_t j, *Cp = Coords;
    H5Sget_select_elem_pointlist( this->DataSpace, 0, Nelements, Coords );
    for( i = 0 ; i < Nelements ; i++ ){
      cout << "Element[" << (int)i << "] ";
      for( j = 0 ; j < (hsize_t)rank ; j++ ){
        cout << " " << (int)*Cp++;
      }
      cout << endl;
    }  
    delete [] Coords;
  }
}
if( this->SelectionType == XDMF_HYPERSLAB ){
  int  k;
  for( k = 0 ; k < rank ; k++ ){
    cout << k << " : Start " << (int)this->Start[k] << " Stride " <<
      (int)this->Stride[k] << " Count " << (int)this->Count[k] << endl;
  }
}
}

void XdmfDataDesc::SetShapeString(XdmfConstString shape)
{
  if ( shape == this->ShapeString || 
    ( shape && this->ShapeString && strcmp(shape, this->ShapeString) == 0 ) )
    {
    return;
    }

  if ( this->ShapeString )
    {
    delete [] this->ShapeString;
    this->ShapeString = 0;
    }
  if ( shape )
    {
    this->ShapeString = new char[ strlen(shape) + 1 ];
    strcpy(this->ShapeString, shape);
    }
}

}
