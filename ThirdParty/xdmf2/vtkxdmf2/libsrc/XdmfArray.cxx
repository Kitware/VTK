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
#include "XdmfArray.h"
#include "XdmfArrayCopyMacro.h"

#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif


static XdmfLength  GlobalTimeCntr = 0;
// static  ostrstream  *StringOutput = NULL;

class XdmfArrayListClass
{
public:
  XdmfArrayListClass()
    {
    this->ListLength = 0;
    this->ListIndex = 0;
    this->List = 0;
    }
  ~XdmfArrayListClass();

  static XdmfArrayListClass *Instance();
  XdmfArrayList* AddArray();
  void RemoveArray(XdmfArray*);

  XdmfLength GetNumberOfElements() 
    {
    return this->ListIndex;
    }

  XdmfLength  ListLength;
  XdmfArrayList  *List;

private:
  XdmfLength  ListIndex;
  static XdmfArrayListClass  *XdmfArrayListClassInstance;
};

XdmfArrayListClass *
XdmfArrayListClass::Instance(){
    if(XdmfArrayListClassInstance == 0){
        // cout << "Creating XdmfArrayListClassInstance" << endl;
        XdmfArrayListClassInstance = new XdmfArrayListClass;
    }
    // cout << "Return Instance = " << XdmfArrayListClassInstance << endl;
    return(XdmfArrayListClassInstance);
}

XdmfArrayListClass::~XdmfArrayListClass()
{


  if ( this->List )
    {
    while( this->ListIndex > 0 )
      {
      // cout << "Before this->ListIndex = " << this->ListIndex << endl;
      // cout << "Delete Array #" << i++ << endl;
      delete this->List[this->ListIndex-1].Array;
      // cout << "After this->ListIndex = " << this->ListIndex << endl;
      }
    delete [] this->List;
    this->List = 0;
    }
  if ( XdmfArrayListClassInstance ) 
    {
    delete XdmfArrayListClassInstance;
    }
}

XdmfArrayList* XdmfArrayListClass::AddArray()
{
XdmfArrayList* res = 0;
if( this->ListIndex >= this->ListLength ){
  XdmfArrayList  *NewList = new XdmfArrayList[ this->ListLength + 32];
  if( this->List != NULL ){
    memcpy( NewList, this->List, this->ListLength * sizeof( XdmfArrayList ));
    delete [] this->List;
    }
  this->List = NewList;
  this->ListLength += 32;
  }
res = this->List + this->ListIndex;
this->ListIndex++;
return res;
}

void XdmfArrayListClass::RemoveArray(XdmfArray* array)
{
  XdmfLength   i;
for( i = 0 ; i < this->ListLength ; i++ ){
  if ( this->List[ i ].Array == array ){
    memmove( &this->List[ i ],
      &this->List[ i + 1 ],
      MAX(0,(this->ListLength - i - 1)) * sizeof( XdmfArrayList ) );      
    this->ListIndex--;
    break;
    }
  }
}

// static XdmfArrayListClass XDMFArrayList;
XdmfArrayListClass *XdmfArrayListClass::XdmfArrayListClassInstance;

XdmfArray *
TagNameToArray( XdmfString TagName ) {

char    c;
XdmfInt64  i, Id;
istrstream   Tag(TagName, strlen(TagName));
XdmfArrayListClass  *XDMFArrayList = XdmfArrayListClass::Instance();

Tag >> c;
if( c != '_' ) {
  XdmfErrorMessage("Invalid Array Tag Name: " << TagName );
  return( NULL );
  }
#ifdef ICE_HAVE_64BIT_STREAMS
Tag >> Id;
#else
{
  double d;
  Tag >> d;
  Id = (XdmfInt64)d;
}
#endif

for( i = 0 ; i < XDMFArrayList->ListLength ; i++ ){
  if ( XDMFArrayList->List[ i ].timecntr == Id ){
    return( XDMFArrayList->List[ i ].Array );
    }
  }
XdmfErrorMessage("No Array found with Tag Name: " << TagName );
return( NULL );
}

XdmfArray *
GetNextOlderArray( XdmfLength Age, XdmfLength *AgeOfArray ) {
XdmfLength i;
XdmfArrayListClass  *XDMFArrayList = XdmfArrayListClass::Instance();

for( i = 0 ; i < XDMFArrayList->GetNumberOfElements(); i++ ){
  if( XDMFArrayList->List[ i ].timecntr > Age ) {
    if( AgeOfArray != NULL ){
      *AgeOfArray = XDMFArrayList->List[ i ].timecntr;
      }
    return( XDMFArrayList->List[ i ].Array );
    }
}
return( NULL );
}

XdmfLength
GetCurrentArrayTime( void ) {

return (GlobalTimeCntr);
}

extern void
PrintAllXdmfArrays() {
XdmfLength i;
XdmfArrayListClass  *XDMFArrayList = XdmfArrayListClass::Instance();

for( i = 0 ; i < XDMFArrayList->GetNumberOfElements(); i++ ){
  cerr << "XdmfArray " << XDMF_64BIT_CAST i << '\n';
  cerr << "   NumberType " << XDMFArrayList->List[ i ].Array->GetNumberTypeAsString() << '\n';
  cerr << "   Time = " << XDMF_64BIT_CAST XDMFArrayList->List[ i ].timecntr << '\n';
  cerr << "   Size = " << XDMF_64BIT_CAST XDMFArrayList->List[ i ].Array->GetNumberOfElements() << '\n';
  }
}

void 
XdmfArray::AddArrayToList( void ) {
XdmfArrayListClass  *XDMFArrayList = XdmfArrayListClass::Instance();

ostrstream   Tag(this->TagName, XDMF_ARRAY_TAG_LENGTH);
GlobalTimeCntr++;
XdmfArrayList* array = XDMFArrayList->AddArray();
array->name = NULL;
array->timecntr = GlobalTimeCntr;
array->Array = this;
Tag << "_" << XDMF_64BIT_CAST GlobalTimeCntr << "_XdmfArray" << ends;
}

XdmfArray::XdmfArray() {
  XdmfDebug("XdmfArray Constructor");
  this->AllowAllocate = 1;
  this->DataPointer = NULL;
  this->HeavyDataSetName = NULL;
  this->DataIsMine = 1;
  this->AddArrayToList();
 }

XdmfArray::XdmfArray( XdmfInt32 numberType ) {
  XdmfDebug("XdmfArray Constructor");
  this->AllowAllocate = 1;
  this->DataPointer = NULL;
  this->DataIsMine = 1;
  this->SetNumberType( numberType );
  this->AddArrayToList();
 }

XdmfArray::XdmfArray( XdmfInt32 numberType, XdmfLength Length ) {
  XdmfDebug("XdmfArray Constructor");
  this->AllowAllocate = 1;
  this->DataPointer = NULL;
  this->DataIsMine = 1;
  this->SetNumberType( numberType );
  this->SetNumberOfElements( Length );
  this->AddArrayToList();
 }

XdmfArray::~XdmfArray() {
XdmfArrayListClass  *XDMFArrayList = XdmfArrayListClass::Instance();
  XdmfDebug("XdmfArray Destructor");
  if( ( this->DataIsMine ) && ( this->DataPointer != NULL ) ) {
    XdmfDebug(" Deleteing Data Array " << this->DataPointer );
    // delete [] this->DataPointer;
    free( this->DataPointer );
    XdmfDebug(" Done Deleteing Data Array " << this->DataPointer );
    this->DataPointer = NULL;
  } else {
    XdmfDebug("Can't Delete Array : Data Pointer is not mine");
  }
  XdmfDebug(" Remove From Array List  " << this );
  XDMFArrayList->RemoveArray(this);
  XdmfDebug(" Done Remove From Array List  " << this );
}

XdmfString
XdmfArray::GetTagName( void ){
return( this->TagName );
}


/*
XdmfPointer XdmfArray::MemCopy( XdmfLength StartOffset,
    XdmfLength NumberOfElemnts,
    XdmfPointer DataPointer,
    XdmfLength Stride ) {
  XdmfPointer RetVal;

if( Stride == 1 ) {
  RetVal = memcpy( this->GetVoidPointer( StartOffset ),
      DataPointer,
      this->Precision * NumberOfElemnts );
} else {
  XdmfLength   i;
  XdmfInt8  *ptr;

  RetVal = this->GetVoidPointer( StartOffset );
  ptr = ( XdmfInt8 *)DataPointer;
  for( i = 0 ; i < NumberOfElemnts ; i++ ){
    memcpy( this->GetVoidPointer( StartOffset ),
      ptr,
      this->Precision );
    StartOffset += Stride;
    ptr += this->Precision;
    }
}

return( RetVal );
}
*/

XdmfInt32 XdmfArray::Allocate( void ){
  XdmfDebug("Request Allocating " <<
    XDMF_64BIT_CAST (this->GetNumberOfElements() *  this->GetElementSize()) <<
    " Bytes");
  if(!this->AllowAllocate){
      XdmfDebug("AllowAllocate is Off");
      return(XDMF_SUCCESS);
  }
  if( this->DataIsMine ) {
    XdmfDebug("Data  " << XDMF_64BIT_CAST this->DataPointer << " is Mine");
    if( this->DataPointer ) {
      this->DataPointer = (XdmfInt8 *)realloc( this->DataPointer, this->GetNumberOfElements() *  this->GetElementSize());
    } else {
      this->DataPointer = (XdmfInt8 *)malloc( this->GetNumberOfElements() *  this->GetElementSize());
    }
    if( this->DataPointer == NULL ) {
      XdmfDebug("Allocation Failed");
      perror(" Alloc :" );
#ifndef _WIN32      
      XdmfDebug("End == " << sbrk(0)  );
#endif
      }
  }
  XdmfDebug("Data Pointer = " << this->DataPointer );
  if( this->DataPointer == NULL ) {
    XdmfDebug("Allocation Failed");
    return( XDMF_FAIL );
    }
  XdmfDebug("Allocation Succeeded");
  return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::Reform( XdmfInt32 rank, XdmfInt64 *Dimensions ) {

  XdmfDebug("Reform Shape");
  XdmfDataDesc::SetShape( rank, Dimensions );  
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::ReformFromSelection( XdmfDataDesc *DataDesc) {

    XdmfDebug("Reform from Selection");

    if(DataDesc->GetSelectionType() == XDMF_SELECTALL){
        return(this->Reform(DataDesc));
    }
    if( DataDesc->GetSelectionType() == XDMF_HYPERSLAB ){
        XdmfInt32  rank;
        XdmfInt64  start[ XDMF_MAX_DIMENSION ];
        XdmfInt64  stride[ XDMF_MAX_DIMENSION ];
        XdmfInt64  count[ XDMF_MAX_DIMENSION ];

        // Select the HyperSlab from HDF5
        XdmfDebug("Reform from Hyperslab");
        rank = DataDesc->GetHyperSlab( start, stride, count );
        this->Reform(rank, count);
        this->SelectAll();
    } else {
        XdmfInt64  NumberOfCoordinates;
        // XdmfInt64  *Coordinates;


        // Select Parametric Coordinates from HDF5
        XdmfDebug("Reform from Coordinates");
        NumberOfCoordinates = DataDesc->GetSelectionSize();
        XdmfDataDesc::SetNumberOfElements(NumberOfCoordinates);
        this->SelectAll();
        // delete Coordinates;
        }
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetShapeFromSelection( XdmfDataDesc *DataDesc) {
    this->ReformFromSelection(DataDesc);
    if(this->Allocate() != XDMF_SUCCESS){
        return(XDMF_FAIL);
    }
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetShape( XdmfInt32 rank, XdmfInt64 *Dimensions ) {

  XdmfDebug("Setting Shape and Allocating Memory");
  XdmfDataDesc::SetShape( rank, Dimensions );  
  if(this->Allocate() != XDMF_SUCCESS){
      return(XDMF_FAIL);
  }
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfArray::CopyShape( hid_t dataSpace ){
  XdmfDebug("Setting Shape and Allocating Memory");
  XdmfDataDesc::CopyShape( dataSpace );  
  if(this->Allocate() != XDMF_SUCCESS){
      return(XDMF_FAIL);
  }
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfArray::CopyShape( XdmfDataDesc *DataDesc ){
  XdmfDebug("Setting Shape and Allocating Memory");
  XdmfDataDesc::CopyShape( DataDesc );  
  if(this->Allocate() != XDMF_SUCCESS){
      return(XDMF_FAIL);
  }
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfArray::Reform( XdmfDataDesc *DataDesc ){
  XdmfDebug("Setting Shape");
  XdmfDataDesc::CopyShape( DataDesc );  
  return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetShapeFromString( XdmfConstString Dimensions ) {

  XdmfDebug("Setting Shape and Allocating Memory");
  XdmfDataDesc::SetShapeFromString( Dimensions );
  if(this->Allocate() != XDMF_SUCCESS){
      return(XDMF_FAIL);
  }
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::ReformFromString( XdmfConstString Dimensions ) {

  XdmfDebug("Setting Shape and Allocating Memory");
  XdmfDataDesc::SetShapeFromString( Dimensions );

return( XDMF_SUCCESS );
}


XdmfPointer XdmfArray::GetDataPointer( XdmfInt64 Index ) {

XdmfByte  *Pointer;
Pointer = ( XdmfByte *)this->DataPointer;
Pointer += (this->GetElementSize() * Index );
return( (XdmfPointer)Pointer );
}

XdmfInt32 XdmfArray::SetValueFromInt64( XdmfInt64 Index, XdmfInt64 Value ) {
return( this->SetValue( Index, Value ) );
}

XdmfInt32 XdmfArray::SetValueFromFloat64( XdmfInt64 Index, XdmfFloat64 Value ) {
return( this->SetValue( Index, Value ) );
}

XdmfInt32 XdmfArray::CopyCompound( XdmfPointer ArrayPointer,
      XdmfInt32 /*ArrayType*/,
      XdmfInt64 /*ArrayStride*/,
      XdmfPointer ValuePointer,
      XdmfInt32  ValueType,
      XdmfInt64 ValueStride,
      XdmfInt32 Direction,
      XdmfInt64 NumberOfValues ) {

XdmfFloat64  *TmpPtr, *TmpArray;
XdmfInt64  Length, NumberOfMembers, MemberIndex, MemberLength;
XdmfInt32  MemberType;
XdmfByte  *Ptr;


XdmfDebug("Coping " << XDMF_64BIT_CAST NumberOfValues << " Direction = " << Direction );
if( Direction == XDMF_ARRAY_IN ){

  TmpArray = new XdmfFloat64[ NumberOfValues ];

  // Copy Values To TmpArray
  TmpPtr = TmpArray;
  Ptr = (XdmfByte *)ValuePointer;
  XDMF_ARRAY_COPY( Ptr, ValueType, ValueStride,
      TmpPtr, XDMF_FLOAT64_TYPE, 1,
       XDMF_ARRAY_OUT, NumberOfValues );        

  // Copy TmpArray to Array
  Ptr = (XdmfByte *)ArrayPointer;
  TmpPtr = TmpArray;
  MemberIndex = 0;
  NumberOfMembers = this->GetNumberOfMembers();
  while( NumberOfValues ){
    MemberType = this->GetMemberType( MemberIndex );
    MemberLength = this->GetMemberLength( MemberIndex );
    XDMF_ARRAY_COPY( Ptr, MemberType, 1,
      TmpPtr, XDMF_FLOAT64_TYPE, 1,
      XDMF_ARRAY_IN, MemberLength);
    Ptr += this->GetMemberSize( MemberIndex );
    NumberOfValues -= MemberLength;
    MemberIndex++;
    if( MemberIndex >= NumberOfMembers ) {
      MemberIndex = 0;
      }
  }
} else {
  TmpArray = new XdmfFloat64[ NumberOfValues ];
  // Copy Array to TmpArray
  Ptr = (XdmfByte *)ArrayPointer;
  TmpPtr = TmpArray;
  MemberIndex = 0;
  NumberOfMembers = this->GetNumberOfMembers();
  Length = NumberOfValues;
  XdmfDebug("Copying " << XDMF_64BIT_CAST NumberOfValues << " Out");
  while( NumberOfValues ){
    MemberType = this->GetMemberType( MemberIndex );
    MemberLength = this->GetMemberLength( MemberIndex );
    XDMF_ARRAY_COPY( Ptr, MemberType, 1,
      TmpPtr, XDMF_FLOAT64_TYPE, 1,
      XDMF_ARRAY_OUT, MemberLength);
    Ptr += this->GetMemberSize( MemberIndex );
    NumberOfValues -= MemberLength;
    MemberIndex++;
    if( MemberIndex >= NumberOfMembers ) {
      MemberIndex = 0;
      }
  }
  // Copy TmpArray to Values
  TmpPtr = TmpArray;
  Ptr = (XdmfByte *)ValuePointer;
  XDMF_ARRAY_COPY( Ptr, ValueType, ValueStride,
      TmpPtr, XDMF_FLOAT64_TYPE, 1,
       XDMF_ARRAY_IN, Length);        

}

delete TmpArray;
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfInt64 Value ) {

XdmfPointer  ArrayPointer;
XdmfInt64  *vp;

ArrayPointer = this->GetDataPointer(Index);
vp = &Value;
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_INT64_TYPE, 1,
    XDMF_ARRAY_IN, 1);
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfFloat64 Value ) {

XdmfPointer  ArrayPointer;
XdmfFloat64  *vp;

ArrayPointer = this->GetDataPointer(Index);
vp = &Value;
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, 1);
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfArray *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ValuesStart,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) 
{
  switch(Values->GetNumberType())
  {
    case(XDMF_INT8_TYPE):
      return this->SetValues(Index, (XdmfInt8*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_INT16_TYPE):
      return this->SetValues(Index, (XdmfInt16*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_INT32_TYPE):
      return this->SetValues(Index, (XdmfInt32*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_INT64_TYPE):
      return this->SetValues(Index, (XdmfInt64*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_FLOAT32_TYPE):
      return this->SetValues(Index, (XdmfFloat32*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_FLOAT64_TYPE):
      return this->SetValues(Index, (XdmfFloat64*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_UINT8_TYPE):
      return this->SetValues(Index, (XdmfUInt8*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_UINT16_TYPE):
      return this->SetValues(Index, (XdmfUInt16*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    case(XDMF_UINT32_TYPE):
      return this->SetValues(Index, (XdmfUInt32*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
    default:
      return this->SetValues(Index, (XdmfFloat64*)Values->GetDataPointer(ValuesStart), NumberOfValues, ArrayStride, ValuesStride);
  }
  return XDMF_FAIL;
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfConstString Values,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;
XdmfInt64    count = 0, i = 0, NumberOfValues;
char* NewValues = new char [ strlen(Values) + 1 ];
strcpy(NewValues, Values);
istrstream   ist(NewValues, strlen( NewValues) );
istrstream   counter(NewValues, strlen( NewValues) );
XdmfFloat64  dummy, *ValueArray, *vp;

while( counter >> dummy ) count++;
NumberOfValues = count;
vp = ValueArray = new XdmfFloat64[ count + 1 ];
while( ist >> dummy ) ValueArray[i++] = dummy;
ArrayPointer = this->GetDataPointer(Index);
delete [] NewValues;
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    vp, XDMF_FLOAT64_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
delete [] ValueArray;
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfUInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT8_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfUInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT16_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfUInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT32_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT8_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT16_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT32_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT64_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_FLOAT32_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::SetValues( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
if( ArrayPointer == NULL ){
  this->SetNumberOfElements( NumberOfValues + Index );
  ArrayPointer = this->GetDataPointer(Index);
  }
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_FLOAT64_TYPE, ValuesStride,
    XDMF_ARRAY_IN, NumberOfValues );
return( XDMF_SUCCESS );
}


XdmfFloat64 XdmfArray::GetValueAsFloat64( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfFloat64  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}

XdmfFloat32 XdmfArray::GetValueAsFloat32( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfFloat32  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_FLOAT32_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}

XdmfInt64 XdmfArray::GetValueAsInt64( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfInt64  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_INT64_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}

XdmfInt32 XdmfArray::GetValueAsInt32( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfInt32  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_INT32_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}

XdmfInt16 XdmfArray::GetValueAsInt16( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfInt16  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_INT16_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}

XdmfInt8 XdmfArray::GetValueAsInt8( XdmfInt64 Index ) {

XdmfPointer  ArrayPointer;
XdmfInt8  Value, *vp = &Value;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    vp, XDMF_INT8_TYPE, 1,
    XDMF_ARRAY_OUT, 1);
return( Value );
}


XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfUInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT8_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfUInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT16_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfUInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_UINT32_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfInt8 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT8_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfInt16 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT16_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfInt32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT32_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfInt64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_INT64_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfFloat32 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_FLOAT32_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfInt32 XdmfArray::GetValues( XdmfInt64 Index, XdmfFloat64 *Values,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride,
        XdmfInt64 ValuesStride ) {
XdmfPointer  ArrayPointer;

ArrayPointer = this->GetDataPointer(Index);
XdmfDebug("Getting " << XDMF_64BIT_CAST NumberOfValues << " From Pointer = " << ArrayPointer << " to " << Values );
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), ArrayStride,
    Values, XDMF_FLOAT64_TYPE, ValuesStride,
    XDMF_ARRAY_OUT, NumberOfValues );
return( XDMF_SUCCESS );
}

XdmfString  XdmfArray::GetValues( XdmfInt64 Index,
        XdmfInt64 NumberOfValues,
        XdmfInt64 ArrayStride ) {

XdmfInt64  i = 0, MemberLength;
XdmfInt64  *IntValues;
XdmfFloat64 * FloatValues;
ostrstream      StringOutput;
XdmfString Ptr;
static XdmfString ReturnString = NULL;



if( NumberOfValues == 0 ){
  NumberOfValues = this->GetNumberOfElements() - Index;
  }
// NumberOfValues -= 1;
if( this->GetNumberType() == XDMF_COMPOUND_TYPE ){
  XdmfDebug("Array is Compound, increasing value of NumberOfValues " << XDMF_64BIT_CAST NumberOfValues );
  MemberLength = 0;
  for( i = 0 ; i < this->GetNumberOfMembers() ; i++ ){
    MemberLength += this->GetMemberLength(i);
    }
  NumberOfValues *= MemberLength;
  XdmfDebug("New NumberOfValues  = " << XDMF_64BIT_CAST NumberOfValues );
}

if( this->GetNumberType() == XDMF_INT8_TYPE || this->GetNumberType() == XDMF_INT16_TYPE || this->GetNumberType() == XDMF_INT32_TYPE || this->GetNumberType() == XDMF_INT64_TYPE || this->GetNumberType() == XDMF_UINT8_TYPE || this->GetNumberType() == XDMF_UINT16_TYPE || this->GetNumberType() == XDMF_UINT32_TYPE) {
  IntValues = new XdmfInt64[ NumberOfValues + 10 ];
  this->GetValues( Index, IntValues, NumberOfValues, ArrayStride, 1 );
  i = 0;
  while( NumberOfValues-- ) {
    StringOutput << (long)IntValues[i++] << " ";
    }
  delete [] IntValues;
}
else {
  FloatValues = new XdmfFloat64[ NumberOfValues + 10];
  this->GetValues( Index, FloatValues, NumberOfValues, ArrayStride, 1 );
  i=0;
  while( NumberOfValues-- ) {
    StringOutput << (double)FloatValues[i++] << " ";
  }
  delete [] FloatValues;
}

StringOutput << ends;
Ptr = StringOutput.str();
if ( ReturnString != NULL ) delete [] ReturnString;
ReturnString = new char[ strlen( Ptr ) + 2 ];
strcpy( ReturnString, Ptr );
delete [] Ptr;
return( ReturnString );
}

XdmfInt32  XdmfArray::Generate( XdmfFloat64 StartValue,
      XdmfFloat64 EndValue,
      XdmfInt64 StartIndex,
      XdmfInt64 EndIndex ){

XdmfFloat64  *Values, *vp, Current, Delta;
XdmfInt64  i, Length;

if( EndIndex == StartIndex ) EndIndex = this->GetNumberOfElements() - 1;
Length = EndIndex - StartIndex;
vp = Values = new XdmfFloat64[ Length  + 1];
Current = StartValue;
Delta = ( EndValue - StartValue ) / Length;
Length++;
for( i = 0 ; i < Length ; i++ ) {
  *vp++ = Current;
  Current += Delta;
  }
this->SetValues( StartIndex, Values, Length, 1, 1);
delete [] Values;
return( XDMF_SUCCESS );
}

XdmfArray &
XdmfArray::operator=( XdmfArray &Array ){

XdmfInt64  Length;
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp, *Values;

Length = MIN( this->GetNumberOfElements(), Array.GetNumberOfElements() );
Vp = Values = new XdmfFloat64[ Length + 10 ];
Array.GetValues( 0, Values, Length );
ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, Length );
delete [] Values;
return( *this );
}

XdmfArray &
XdmfArray::operator=( XdmfFloat64 Value ){

XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp = &Value;

ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_COPY( ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 0,
    XDMF_ARRAY_IN, this->GetNumberOfElements());
return( *this );
}

XdmfArray &
XdmfArray::operator+( XdmfArray &Array ){
XdmfInt64  Length;
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp, *Values;

Length = MIN( this->GetNumberOfElements(), Array.GetNumberOfElements() );
Vp = Values = new XdmfFloat64[ Length + 10 ];
Array.GetValues( 0, Values, Length );
ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayAddTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, Length );
delete [] Values;
return( *this );
}

XdmfArray &
XdmfArray::operator+( XdmfFloat64 Value ){
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp = &Value;

ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayAddTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 0,
    XDMF_ARRAY_IN, this->GetNumberOfElements());
return( *this );
}

XdmfArray &
XdmfArray::operator-( XdmfArray &Array ){
XdmfInt64  Length;
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp, *Values;

Length = MIN( this->GetNumberOfElements(), Array.GetNumberOfElements() );
Vp = Values = new XdmfFloat64[ Length + 10 ];
Array.GetValues( 0, Values, Length );
ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArraySubtractTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, Length );
delete [] Values;
return( *this );
}

XdmfArray &
XdmfArray::operator-( XdmfFloat64 Value ){
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp = &Value;

ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArraySubtractTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 0,
    XDMF_ARRAY_IN, this->GetNumberOfElements());
return( *this );
}

XdmfArray &
XdmfArray::operator*( XdmfArray &Array ){
XdmfInt64  Length;
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp, *Values;

Length = MIN( this->GetNumberOfElements(), Array.GetNumberOfElements() );
Vp = Values = new XdmfFloat64[ Length + 10 ];
Array.GetValues( 0, Values, Length );
ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayMultiplyTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, Length );
delete [] Values;
return( *this );
}

XdmfArray &
XdmfArray::operator*( XdmfFloat64 Value ){
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp = &Value;

ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayMultiplyTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 0,
    XDMF_ARRAY_IN, this->GetNumberOfElements());
return( *this );
}

XdmfArray &
XdmfArray::operator/( XdmfArray &Array ){
XdmfInt64  Length;
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp, *Values;

Length = MIN( this->GetNumberOfElements(), Array.GetNumberOfElements() );
Vp = Values = new XdmfFloat64[ Length + 10 ];
Array.GetValues( 0, Values, Length );
ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayDivideTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 1,
    XDMF_ARRAY_IN, Length );
delete [] Values;
return( *this );
}

XdmfArray &
XdmfArray::operator/( XdmfFloat64 Value ){
XdmfPointer  ArrayPointer;
XdmfFloat64  *Vp = &Value;

ArrayPointer = this->GetDataPointer();
XDMF_ARRAY_OPERATE( XdmfArrayDivideTag, ArrayPointer, this->GetNumberType(), 1,
    Vp, XDMF_FLOAT64_TYPE, 0,
    XDMF_ARRAY_IN, this->GetNumberOfElements());
return( *this );
}

XdmfArray *
XdmfArray::Clone( XdmfArray *Indexes ){

XdmfLength  i, Length, Size;
XdmfByte  *Ptr, *Source, *Target;
XdmfArray  *NewArray = new XdmfArray();
XdmfInt64  *IVals;


Length = Indexes->GetNumberOfElements();
IVals = new XdmfInt64[ Length + 10 ];
Indexes->GetValues( 0, IVals, Length );
NewArray->SetNumberType( this->GetNumberType() );
NewArray->SetNumberOfElements( Length );
Source = ( XdmfByte *)this->GetDataPointer();
Target = ( XdmfByte *)NewArray->GetDataPointer(0);
Size = this->GetElementSize();
for( i = 0 ; i < Length ; i++ ){
  Ptr = Source + ( Size * IVals[ i ] );
  memcpy( Target, Ptr, Size ); 
  Target += Size;
}
delete [] IVals;
return( NewArray );
}

XdmfArray *
XdmfArray::Clone( XdmfLength start, XdmfLength End ) {

XdmfLength  Length;
XdmfArray  *NewArray = new XdmfArray();


if( ( start == 0 ) && ( End == 0 )){
  NewArray->CopyType( this );
  NewArray->CopyShape( this );
  Length = this->GetNumberOfElements();
  End = Length - 1;
} else {
  if( End <= start ) End = this->GetNumberOfElements() - start - 1;
  Length = End - start + 1;
  NewArray->SetNumberType( this->GetNumberType() );
  NewArray->SetNumberOfElements( Length );
}
memcpy( NewArray->GetDataPointer(0),
  this->GetDataPointer( start ),
  Length * this->GetElementSize() );
return( NewArray );
}

XdmfArray *
XdmfArray::Reference( XdmfLength start, XdmfLength End ) {

XdmfLength  Length;
XdmfArray  *NewArray = new XdmfArray();


if( End <= start ) End = this->GetNumberOfElements() - start - 1;
Length = End - start + 1;
NewArray->SetNumberType( this->GetNumberType() );
NewArray->SetDataPointer( this->GetDataPointer( start ) );
NewArray->SetNumberOfElements( Length );
return( NewArray );
}


XdmfFloat64
XdmfArray::GetMean( void ) {
XdmfLength  i, Length;
XdmfFloat64  Value, *Data;

Length = this->GetNumberOfElements();
Data = new XdmfFloat64[ Length + 10 ];
this->GetValues( 0, Data, Length);
Value = 0;
for( i = 0 ; i < Length ; i++ ){
  Value += Data[i];
  }
delete [] Data;
Value /= Length;
return( Value );
}

XdmfFloat64
XdmfArray::GetMaxAsFloat64( void ) {
XdmfLength  i, Length;
XdmfFloat64  Value, *Data;

Length = this->GetNumberOfElements();
Data = new XdmfFloat64[ Length ];
this->GetValues( 0, Data, Length);
Value = Data[0];
for( i = 0 ; i < Length ; i++ ){
  if ( Data[i] > Value ) {
    Value =  Data[i];
    }
  }
delete [] Data;
return( Value );
}

XdmfFloat64
XdmfArray::GetMinAsFloat64( void ) {
XdmfLength  i, Length;
XdmfFloat64  Value, *Data;

Length = this->GetNumberOfElements();
Data = new XdmfFloat64[ Length ];
this->GetValues( 0, Data, Length);
Value = Data[0];
for( i = 0 ; i < Length ; i++ ){
  if ( Data[i] < Value ) {
    Value =  Data[i];
    }
  }
delete [] Data;
return( Value );
}

XdmfInt64
XdmfArray::GetMinAsInt64( void ) {
XdmfLength  i, Length;
XdmfInt64  Value, *Data;

Length = this->GetNumberOfElements();
Data = new XdmfInt64[ Length ];
this->GetValues( 0, Data, Length);
Value = Data[0];
for( i = 0 ; i < Length ; i++ ){
  if ( Data[i] < Value ) {
    Value =  Data[i];
    }
  }
delete [] Data;
return( Value );
}

XdmfInt64
XdmfArray::GetMaxAsInt64( void ) {
XdmfLength  i, Length;
XdmfInt64  Value, *Data;

Length = this->GetNumberOfElements();
Data = new XdmfInt64[ Length ];
this->GetValues( 0, Data, Length);
Value = Data[0];
for( i = 0 ; i < Length ; i++ ){
  if ( Data[i] > Value ) { 
    Value =  Data[i];
    }
  }
delete [] Data;
return( Value );
}

XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfUInt8 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}
XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfUInt16 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}
XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfUInt32 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}
XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfInt8 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}
XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfInt16 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}
XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfInt32 Value ) {
return(this->SetValueFromInt64( Index, Value ));
}

XdmfInt32 XdmfArray::SetValue( XdmfInt64 Index, XdmfFloat32 Value ) {
return(this->SetValueFromFloat64( Index, Value ));
}

istrstream& ICE_READ_STREAM64(istrstream& istr, XDMF_64_INT& i)
{
#if defined( ICE_HAVE_64BIT_STREAMS )
istr >>i;
#else
  double d = 0;
  istr >> d;
  i = (XDMF_64_INT)d;
#endif
return istr;
}
