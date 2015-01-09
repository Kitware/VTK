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
#define XDMF_ARRAY_IN  0
#define XDMF_ARRAY_OUT  1

namespace xdmf2
{

template<class ArrayType, class ValueType>
void XdmfArrayCopy(
  ArrayType* ArrayPointer, XdmfInt64 ArrayStride,
  ValueType* ValuePointer, XdmfInt64 ValueStride,
  XdmfInt32 Direction, XdmfInt64 NumberOfValues)
{
  XdmfInt64   i; 
  ArrayType *ap; 

  i = NumberOfValues; 
  ap = ArrayPointer; 
  if( Direction == XDMF_ARRAY_IN ) { 
    while(i--){ 
      *ap = (ArrayType)*ValuePointer; 
      ap += ArrayStride; 
      ValuePointer += ValueStride; 
    } 
  } else { 
    while(i--){ 
      *ValuePointer = (ValueType)*ap; 
      ap += ArrayStride; 
      ValuePointer += ValueStride; 
    } 
  } 
}


#define XDMF_ARRAY_COPY(\
    ArrayPointer, ArrayType, ArrayStride, \
    ValuePointer, ValueType, ValueStride, Direction, NumberOfValues  ) \
    switch(ArrayType) { \
      case XDMF_UINT8_TYPE : \
        XdmfArrayCopy((XdmfUInt8*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_UINT16_TYPE : \
        XdmfArrayCopy((XdmfUInt16*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_UINT32_TYPE : \
        XdmfArrayCopy((XdmfUInt32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_INT8_TYPE : \
        XdmfArrayCopy((XdmfInt8*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_INT16_TYPE : \
        XdmfArrayCopy((XdmfInt16*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_INT32_TYPE : \
        XdmfArrayCopy((XdmfInt32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_INT64_TYPE : \
        XdmfArrayCopy((XdmfInt64*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_FLOAT32_TYPE : \
        XdmfArrayCopy((XdmfFloat32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      case XDMF_FLOAT64_TYPE : \
        XdmfArrayCopy((XdmfFloat64*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues); \
        break; \
      default : \
        this->CopyCompound(ArrayPointer, ArrayType, ArrayStride, \
          ValuePointer, ValueType, ValueStride, \
          Direction, NumberOfValues ); \
        break; \
      }

class XdmfArrayAddTag{};
template<class Val1, class Val2>
void XdmfArrayOperator(Val1& v1, const Val2& v2, XdmfArrayAddTag*)
{ v1 += (Val1)v2; }

class XdmfArraySubtractTag{};
template<class Val1, class Val2>
void XdmfArrayOperator(Val1& v1, const Val2& v2, XdmfArraySubtractTag*)
{ v1 -= (Val1)v2; }

class XdmfArrayMultiplyTag{};
template<class Val1, class Val2>
void XdmfArrayOperator(Val1& v1, const Val2& v2, XdmfArrayMultiplyTag*)
{ v1 *= (Val1)v2; }

class XdmfArrayDivideTag{};
template<class Val1, class Val2>
void XdmfArrayOperator(Val1& v1, const Val2& v2, XdmfArrayDivideTag*)
{ v1 /= (Val1)v2; }

template<class ArrayType, class ValueType, class OperatorTag>
void XdmfArrayOperate(
  ArrayType* ArrayPointer, XdmfInt64 ArrayStride,
  ValueType* ValuePointer, XdmfInt64 ValueStride,
  XdmfInt32 Direction, XdmfInt64 NumberOfValues, OperatorTag*)
{
  XdmfInt64   i; 
  ArrayType *ap; 

  i = NumberOfValues; 
  ap = ArrayPointer; 
  if( Direction == XDMF_ARRAY_IN ) { 
    while(i--){ 
      XdmfArrayOperator(*ap, *ValuePointer, (OperatorTag*)0); 
      ap += ArrayStride; 
      ValuePointer += ValueStride; 
    } 
  } else { 
    while(i--){ 
      XdmfArrayOperator(*ValuePointer, *ap, (OperatorTag*)0); 
      ap += ArrayStride; 
      ValuePointer += ValueStride; 
    } 
  } 
}


#define XDMF_ARRAY_OPERATE(OPERATOR, ArrayPointer, ArrayType, ArrayStride, ValuePointer, ValueType, ValueStride, Direction, NumberOfValues  ) \
    switch(ArrayType) { \
      case XDMF_UINT8_TYPE : \
        XdmfArrayOperate((XdmfUInt8*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_UINT16_TYPE : \
        XdmfArrayOperate((XdmfUInt16*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_UINT32_TYPE : \
        XdmfArrayOperate((XdmfUInt32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_INT8_TYPE : \
        XdmfArrayOperate((XdmfInt8*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_INT16_TYPE : \
        XdmfArrayOperate((XdmfInt16*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_INT32_TYPE : \
        XdmfArrayOperate((XdmfInt32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_INT64_TYPE : \
        XdmfArrayOperate((XdmfInt64*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_FLOAT32_TYPE : \
        XdmfArrayOperate((XdmfFloat32*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      case XDMF_FLOAT64_TYPE : \
        XdmfArrayOperate((XdmfFloat64*)ArrayPointer, ArrayStride, ValuePointer, ValueStride, Direction, NumberOfValues, (OPERATOR*)0); \
        break; \
      default : \
        XdmfErrorMessage("Can't Assign Values to Compound Type"); \
        break; \
      }
}
