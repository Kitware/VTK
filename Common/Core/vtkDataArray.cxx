/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayDispatch.h"
#include "vtkAOSDataArrayTemplate.h" // For fast paths
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataArrayPrivate.txx"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkLookupTable.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkSOADataArrayTemplate.h" // For fast paths
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm> // for min(), max()

namespace {

//--------Copy tuples from src to dest------------------------------------------
struct DeepCopyWorker
{
  // AoS --> AoS same-type specialization:
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType> *src,
                  vtkAOSDataArrayTemplate<ValueType> *dst)
  {
    std::copy(src->Begin(), src->End(), dst->Begin());
  }

  // SoA --> SoA same-type specialization:
  template <typename ValueType>
  void operator()(vtkSOADataArrayTemplate<ValueType> *src,
                  vtkSOADataArrayTemplate<ValueType> *dst)
  {
    vtkIdType numTuples = src->GetNumberOfTuples();
    for (int comp; comp < src->GetNumberOfComponents(); ++comp)
    {
      ValueType *srcBegin = src->GetComponentArrayPointer(comp);
      ValueType *srcEnd = srcBegin + numTuples;
      ValueType *dstBegin = dst->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }

  // Generic implementation:
  template <typename Array1T, typename Array2T>
  void operator()(Array1T *src, Array2T *dst)
  {
    vtkDataArrayAccessor<Array1T> s(src);
    vtkDataArrayAccessor<Array2T> d(dst);

    typedef typename vtkDataArrayAccessor<Array2T>::APIType DestType;

    vtkIdType tuples = src->GetNumberOfTuples();
    int comps = src->GetNumberOfComponents();

    for (vtkIdType t = 0; t < tuples; ++t)
    {
      for (int c = 0; c < comps; ++c)
      {
        d.Set(t, c, static_cast<DestType>(s.Get(t, c)));
      }
    }
  }
};

//------------InterpolateTuple workers------------------------------------------
struct InterpolateMultiTupleWorker
{
  vtkIdType DestTuple;
  vtkIdType *TupleIds;
  vtkIdType NumTuples;
  double *Weights;

  InterpolateMultiTupleWorker(vtkIdType destTuple, vtkIdType *tupleIds,
                              vtkIdType numTuples, double *weights)
    : DestTuple(destTuple), TupleIds(tupleIds), NumTuples(numTuples),
      Weights(weights)
  {}


  template <typename Array1T, typename Array2T>
  void operator()(Array1T *src, Array2T *dst)
  {
    vtkDataArrayAccessor<Array1T> s(src);
    vtkDataArrayAccessor<Array2T> d(dst);

    typedef typename vtkDataArrayAccessor<Array2T>::APIType DestType;

    int numComp = src->GetNumberOfComponents();

    for (int c = 0; c < numComp; ++c)
    {
      double val = 0.;
      for (vtkIdType tupleId = 0; tupleId < this->NumTuples; ++tupleId)
      {
        vtkIdType t = this->TupleIds[tupleId];
        double weight = this->Weights[tupleId];
        val += weight * static_cast<double>(s.Get(t, c));
      }
      DestType valT;
      vtkMath::RoundDoubleToIntegralIfNecessary(val, &valT);
      d.Insert(this->DestTuple, c, valT);
    }
  }
};

struct InterpolateTupleWorker
{
  vtkIdType SrcTuple1;
  vtkIdType SrcTuple2;
  vtkIdType DstTuple;
  double Weight;

  InterpolateTupleWorker(vtkIdType srcTuple1, vtkIdType srcTuple2,
                         vtkIdType dstTuple, double weight)
    : SrcTuple1(srcTuple1), SrcTuple2(srcTuple2), DstTuple(dstTuple),
      Weight(weight)
  {}

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T *src1, Array2T *src2, Array3T *dst)
  {
    vtkDataArrayAccessor<Array1T> s1(src1);
    vtkDataArrayAccessor<Array2T> s2(src2);
    vtkDataArrayAccessor<Array3T> d(dst);

    typedef typename vtkDataArrayAccessor<Array3T>::APIType DestType;

    const int numComps = dst->GetNumberOfComponents();
    const double oneMinusT = 1. - this->Weight;
    double val;
    DestType valT;

    for (int c = 0; c < numComps; ++c)
    {
      val = s1.Get(this->SrcTuple1, c) * oneMinusT +
            s2.Get(this->SrcTuple2, c) * this->Weight;
      vtkMath::RoundDoubleToIntegralIfNecessary(val, &valT);
      d.Insert(this->DstTuple, c, valT);
    }
  }
};

//-----------------GetTuples (id list)------------------------------------------
struct GetTuplesFromListWorker
{
  vtkIdList *Ids;

  GetTuplesFromListWorker(vtkIdList *ids) : Ids(ids) {}

  template <typename Array1T, typename Array2T>
  void operator()(Array1T *src, Array2T *dst)
  {
    vtkDataArrayAccessor<Array1T> s(src);
    vtkDataArrayAccessor<Array2T> d(dst);

    typedef typename vtkDataArrayAccessor<Array2T>::APIType DestType;

    int numComps = src->GetNumberOfComponents();
    vtkIdType *srcTuple = this->Ids->GetPointer(0);
    vtkIdType *srcTupleEnd = this->Ids->GetPointer(Ids->GetNumberOfIds());
    vtkIdType dstTuple = 0;

    while (srcTuple != srcTupleEnd)
    {
      for (int c = 0; c < numComps; ++c)
      {
        d.Set(dstTuple, c, static_cast<DestType>(s.Get(*srcTuple, c)));
      }
      ++srcTuple;
      ++dstTuple;
    }
  }
};

//-----------------GetTuples (tuple range)--------------------------------------
struct GetTuplesRangeWorker
{
  vtkIdType Start;
  vtkIdType End; // Note that End is inclusive.

  GetTuplesRangeWorker(vtkIdType start, vtkIdType end)
    : Start(start), End(end)
  {}

  template <typename Array1T, typename Array2T>
  void operator()(Array1T *src, Array2T *dst)
  {
    vtkDataArrayAccessor<Array1T> s(src);
    vtkDataArrayAccessor<Array2T> d(dst);

    typedef typename vtkDataArrayAccessor<Array2T>::APIType DestType;

    int numComps = src->GetNumberOfComponents();
    for (vtkIdType srcT = this->Start, dstT = 0;
         srcT <= this->End;
         ++srcT, ++dstT)
    {
      for (int c = 0; c < numComps; ++c)
      {
        d.Set(dstT, c, static_cast<DestType>(s.Get(srcT, c)));
      }
    }
  }
};

//----------------SetTuple (from array)-----------------------------------------
struct SetTupleArrayWorker
{
  vtkIdType SrcTuple;
  vtkIdType DstTuple;

  SetTupleArrayWorker(vtkIdType srcTuple, vtkIdType dstTuple)
    : SrcTuple(srcTuple), DstTuple(dstTuple)
  {}

  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT *src, DstArrayT *dst)
  {
    vtkDataArrayAccessor<SrcArrayT> s(src);
    vtkDataArrayAccessor<DstArrayT> d(dst);

    typedef typename vtkDataArrayAccessor<DstArrayT>::APIType DestType;

    int numComps = src->GetNumberOfComponents();
    for (int c = 0; c < numComps; ++c)
    {
      d.Set(this->DstTuple, c, static_cast<DestType>(s.Get(this->SrcTuple, c)));
    }
  }
};

//----------------SetTuples (from array+vtkIdList)------------------------------
struct SetTuplesIdListWorker
{
  vtkIdList *SrcTuples;
  vtkIdList *DstTuples;

  SetTuplesIdListWorker(vtkIdList *srcTuples, vtkIdList *dstTuples)
    : SrcTuples(srcTuples), DstTuples(dstTuples)
  {}

  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT *src, DstArrayT *dst)
  {
    vtkDataArrayAccessor<SrcArrayT> s(src);
    vtkDataArrayAccessor<DstArrayT> d(dst);

    typedef typename vtkDataArrayAccessor<DstArrayT>::APIType DestType;

    vtkIdType numTuples = this->SrcTuples->GetNumberOfIds();
    int numComps = src->GetNumberOfComponents();
    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      vtkIdType srcT = this->SrcTuples->GetId(t);
      vtkIdType dstT = this->DstTuples->GetId(t);
      for (int c = 0; c < numComps; ++c)
      {
        d.Set(dstT, c, static_cast<DestType>(s.Get(srcT, c)));
      }
    }
  }
};

//----------------SetTuples (from array+range)----------------------------------
struct SetTuplesRangeWorker
{
  vtkIdType SrcStartTuple;
  vtkIdType DstStartTuple;
  vtkIdType NumTuples;

  SetTuplesRangeWorker(vtkIdType srcStartTuple, vtkIdType dstStartTuple,
                       vtkIdType numTuples)
    : SrcStartTuple(srcStartTuple), DstStartTuple(dstStartTuple),
      NumTuples(numTuples)
  {}

  // Generic implementation. We perform the obvious optimizations for AOS/SOA
  // in the derived class implementations.
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT *src, DstArrayT *dst)
  {
    vtkDataArrayAccessor<SrcArrayT> s(src);
    vtkDataArrayAccessor<DstArrayT> d(dst);

    typedef typename vtkDataArrayAccessor<DstArrayT>::APIType DestType;

    int numComps = src->GetNumberOfComponents();
    vtkIdType srcT = this->SrcStartTuple;
    vtkIdType srcTEnd = srcT + this->NumTuples;
    vtkIdType dstT = this->DstStartTuple;

    while (srcT < srcTEnd)
    {
      for (vtkIdType t = 0; t < this->NumTuples; ++t)
      {
        for (int c = 0; c < numComps; ++c)
        {
          d.Set(dstT, c, static_cast<DestType>(s.Get(srcT, c)));
        }
      }
      ++srcT;
      ++dstT;
    }
  }
};

template<typename InfoType, typename KeyType>
bool hasValidKey(InfoType info, KeyType key,
                   vtkMTimeType mtime, double range[2] )
{
  if ( info->Has( key ) )
  {
    if ( mtime <= info->GetMTime() )
    {
      info->Get( key, range );
      return true;
    }
  }
  return false;
}

template<typename InfoType, typename KeyType, typename ComponentKeyType>
bool hasValidKey(InfoType info, KeyType key, ComponentKeyType ckey,
                   vtkMTimeType mtime, double range[2], int comp )
{
  if ( info->Has( key ) )
  {
    if ( mtime <= info->GetMTime() )
    {
      info->Get( key )->GetInformationObject(comp)->Get( ckey, range );
      return true;
    }
  }
  return false;
}

} // end anon namespace

vtkInformationKeyRestrictedMacro(vtkDataArray, COMPONENT_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataArray, L2_NORM_RANGE, DoubleVector, 2);
vtkInformationKeyMacro(vtkDataArray, UNITS_LABEL, String);

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray()
{
  this->LookupTable = NULL;
  this->Range[0] = 0;
  this->Range[1] = 0;
}

//----------------------------------------------------------------------------
vtkDataArray::~vtkDataArray()
{
  if ( this->LookupTable )
  {
    this->LookupTable->Delete();
  }
  this->SetName(0);
}

//----------------------------------------------------------------------------
void vtkDataArray::DeepCopy(vtkAbstractArray* aa)
{
  if ( aa == NULL )
  {
    return;
  }

  vtkDataArray *da = vtkDataArray::FastDownCast(aa);
  if (da == NULL)
  {
    vtkErrorMacro(<< "Input array is not a vtkDataArray ("
                  << aa->GetClassName() << ")");
    return;
  }

  this->DeepCopy(da);
}

//----------------------------------------------------------------------------
//Normally subclasses will do this when the input and output type of the
//DeepCopy are the same. When they are not the same, then we use the
//templated code below.
void vtkDataArray::DeepCopy(vtkDataArray *da)
{
  // Match the behavior of the old AttributeData
  if ( da == NULL )
  {
    return;
  }

  if ( this != da )
  {
    this->Superclass::DeepCopy( da ); // copy Information object

    vtkIdType numTuples = da->GetNumberOfTuples();
    int numComps = da->NumberOfComponents;

    this->SetNumberOfComponents(numComps);
    this->SetNumberOfTuples(numTuples);

    DeepCopyWorker worker;
    if (!vtkArrayDispatch::Dispatch2::Execute(da, this, worker))
    {
      // If dispatch fails, use fallback:
      worker(da, this);
    }

    this->SetLookupTable(0);
    if (da->LookupTable)
    {
      this->LookupTable = da->LookupTable->NewInstance();
      this->LookupTable->DeepCopy(da->LookupTable);
    }
  }

  this->Squeeze();
}

//------------------------------------------------------------------------------
void vtkDataArray::ShallowCopy(vtkDataArray *other)
{
  // Deep copy by default. Subclasses may override this behavior.
  this->DeepCopy(other);
}

//------------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                            vtkAbstractArray *source)
{
  vtkDataArray *srcDA = vtkDataArray::FastDownCast(source);
  if (!srcDA)
  {
    vtkErrorMacro("Source array must be a vtkDataArray subclass (got "
                  << source->GetClassName() << ").");
    return;
  }

  if (!vtkDataTypesCompare(source->GetDataType(), this->GetDataType()))
  {
    vtkErrorMacro("Type mismatch: Source: " << source->GetDataTypeAsString()
                  << " Dest: " << this->GetDataTypeAsString());
    return;
  }

  if (source->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
                  << source->GetNumberOfComponents() << " Dest: "
                  << this->GetNumberOfComponents());
    return;
  }

  SetTupleArrayWorker worker(srcTupleIdx, dstTupleIdx);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType i, const float *source)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
  {
    this->SetComponent(i, c, static_cast<double>(source[c]));
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType i, const double *source)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
  {
    this->SetComponent(i, c, source[c]);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                               vtkAbstractArray *source)
{
  vtkIdType newSize = (dstTupleIdx + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(dstTupleIdx + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  this->SetTuple(dstTupleIdx, srcTupleIdx, source);
}

//----------------------------------------------------------------------------
vtkIdType vtkDataArray::InsertNextTuple(vtkIdType srcTupleIdx,
                                        vtkAbstractArray *source)
{
  vtkIdType tupleIdx = this->GetNumberOfTuples();
  this->InsertTuple(tupleIdx, srcTupleIdx, source);
  return tupleIdx;
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                                vtkAbstractArray *src)
{
  if (dstIds->GetNumberOfIds() == 0)
  {
    return;
  }
  if (dstIds->GetNumberOfIds() != srcIds->GetNumberOfIds())
  {
    vtkErrorMacro("Mismatched number of tuples ids. Source: "
                  << srcIds->GetNumberOfIds() << " Dest: "
                  << dstIds->GetNumberOfIds());
    return;
  }
  if (!vtkDataTypesCompare(src->GetDataType(), this->GetDataType()))
  {
    vtkErrorMacro("Data type mismatch: Source: " << src->GetDataTypeAsString()
                  << " Dest: " << this->GetDataTypeAsString());
    return;
  }
  if (src->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
                  << src->GetNumberOfComponents() << " Dest: "
                  << this->GetNumberOfComponents());
    return;
  }
  vtkDataArray *srcDA = vtkDataArray::FastDownCast(src);
  if (!srcDA)
  {
    vtkErrorMacro("Source array must be a subclass of vtkDataArray. Got: "
                  << src->GetClassName());
    return;
  }

  vtkIdType maxSrcTupleId = srcIds->GetId(0);
  vtkIdType maxDstTupleId = dstIds->GetId(0);
  for (int i = 0; i < dstIds->GetNumberOfIds(); ++i)
  {
    maxSrcTupleId = std::max(maxSrcTupleId, srcIds->GetId(i));
    maxDstTupleId = std::max(maxDstTupleId, dstIds->GetId(i));
  }

  if (maxSrcTupleId >= src->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
                  << maxSrcTupleId << ", but there are only "
                  << src->GetNumberOfTuples() << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  SetTuplesIdListWorker worker(srcIds, dstIds);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuples(vtkIdType dstStart, vtkIdType n,
                                vtkIdType srcStart, vtkAbstractArray *src)
{
  if (n == 0)
  {
    return;
  }
  if (!vtkDataTypesCompare(src->GetDataType(), this->GetDataType()))
  {
    vtkErrorMacro("Data type mismatch: Source: " << src->GetDataTypeAsString()
                  << " Dest: " << this->GetDataTypeAsString());
    return;
  }
  if (src->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
                  << src->GetNumberOfComponents() << " Dest: "
                  << this->GetNumberOfComponents());
    return;
  }
  vtkDataArray *srcDA = vtkDataArray::FastDownCast(src);
  if (!srcDA)
  {
    vtkErrorMacro("Source array must be a subclass of vtkDataArray. Got: "
                  << src->GetClassName());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= src->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
                  << maxSrcTupleId << ", but there are only "
                  << src->GetNumberOfTuples() << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  SetTuplesRangeWorker worker(srcStart, dstStart, n);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}

//----------------------------------------------------------------------------
// These can be overridden for more efficiency
double vtkDataArray::GetComponent(vtkIdType tupleIdx, int compIdx)
{
  double *tuple=new double[this->NumberOfComponents], c;

  this->GetTuple(tupleIdx,tuple);
  c =  tuple[compIdx];
  delete [] tuple;

  return c;
}

//----------------------------------------------------------------------------
void vtkDataArray::SetComponent(vtkIdType tupleIdx, int compIdx, double value)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( tupleIdx < this->GetNumberOfTuples() )
  {
    this->GetTuple(tupleIdx,tuple);
  }
  else
  {
    for (int k=0; k<this->NumberOfComponents; k++)
    {
      tuple[k] = 0.0;
    }
  }

  tuple[compIdx] = value;
  this->SetTuple(tupleIdx,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertComponent(vtkIdType tupleIdx, int compIdx,
                                   double value)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( tupleIdx < this->GetNumberOfTuples() )
  {
    this->GetTuple(tupleIdx,tuple);
  }
  else
  {
    for (int k=0; k<this->NumberOfComponents; k++)
    {
      tuple[k] = 0.0;
    }
  }

  tuple[compIdx] = value;
  this->InsertTuple(tupleIdx,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                           int compMax, vtkDoubleArray* data)
{
  int i;
  vtkIdType j;
  int numComp=this->GetNumberOfComponents();
  double *tuple=new double[numComp];
  double *ptr=data->WritePointer(0,(tupleMax-tupleMin+1)*(compMax-compMin+1));

  for (j=tupleMin; j <= tupleMax; j++)
  {
    this->GetTuple(j,tuple);
    for (i=compMin; i <= compMax; i++)
    {
      *ptr++ = tuple[i];
    }
  }
  delete [] tuple;
}

//----------------------------------------------------------------------------
// Interpolate array value from other array value given the
// indices and associated interpolation weights.
// This method assumes that the two arrays are of the same time.
void vtkDataArray::InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList *tupleIds,
                                    vtkAbstractArray* source,  double* weights)
{
  if (!vtkDataTypesCompare(this->GetDataType(), source->GetDataType()))
  {
    vtkErrorMacro("Cannot interpolate arrays of different type.");
    return;
  }

  vtkDataArray *da = vtkDataArray::FastDownCast(source);
  if (!da)
  {
    vtkErrorMacro(<<"Source array is not a vtkDataArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (da->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
                  << source->GetNumberOfComponents() << " Dest: "
                  << this->GetNumberOfComponents());
    return;
  }

  vtkIdType numIds=tupleIds->GetNumberOfIds();
  vtkIdType *ids=tupleIds->GetPointer(0);

  bool fallback = da->GetDataType() == VTK_BIT ||
                  this->GetDataType() == VTK_BIT;

  if (!fallback)
  {
    InterpolateMultiTupleWorker worker(dstTupleIdx, ids, numIds, weights);
    // Use fallback if dispatch fails.
    fallback = !vtkArrayDispatch::Dispatch2SameValueType::Execute(da, this,
                                                                  worker);
  }

  // Fallback to a separate implementation that checks vtkDataArray::GetDataType
  // rather than relying on API types, since we'll need to round differently
  // depending on type, and the API type for vtkDataArray is always double.
  if (fallback)
  {
    bool doRound = !(this->GetDataType() == VTK_FLOAT ||
                     this->GetDataType() == VTK_DOUBLE);
    double typeMin = this->GetDataTypeMin();
    double typeMax = this->GetDataTypeMax();

    for (int c = 0; c < numComps; ++c)
    {
      double val = 0.;
      for (vtkIdType j = 0; j < numIds; ++j)
      {
        val += weights[j] * da->GetComponent(ids[j], c);
      }

      // Clamp to data type range:
      val = std::max(val, typeMin);
      val = std::min(val, typeMax);

      // Round for floating point types:
      if (doRound)
      {
        val = std::floor((val >= 0.) ? (val + 0.5) : (val - 0.5));
      }

      this->InsertComponent(dstTupleIdx, c, val);
    }
  }
}

//----------------------------------------------------------------------------
// Interpolate value from the two values, p1 and p2, and an
// interpolation factor, t. The interpolation factor ranges from (0,1),
// with t=0 located at p1. This method assumes that the three arrays are of
// the same type. p1 is value at index id1 in fromArray1, while, p2 is
// value at index id2 in fromArray2.
void vtkDataArray::InterpolateTuple(vtkIdType dstTuple,
  vtkIdType srcTuple1, vtkAbstractArray* source1,
  vtkIdType srcTuple2, vtkAbstractArray* source2, double t)
{
  int type = this->GetDataType();

  if (!vtkDataTypesCompare(type, source1->GetDataType()) ||
      !vtkDataTypesCompare(type, source2->GetDataType()))
  {
    vtkErrorMacro("All arrays to InterpolateValue must be of same type.");
    return;
  }

  if (srcTuple1 >= source1->GetNumberOfTuples())
  {
    vtkErrorMacro("Tuple 1 out of range for provided array. "
                  "Requested tuple: " << srcTuple1 << " "
                  "Tuples: " << source1->GetNumberOfTuples());
    return;
  }

  if (srcTuple2 >= source2->GetNumberOfTuples())
  {
    vtkErrorMacro("Tuple 2 out of range for provided array. "
                  "Requested tuple: " << srcTuple2 << " "
                  "Tuples: " << source2->GetNumberOfTuples());
    return;
  }

  vtkDataArray *src1DA = vtkDataArray::FastDownCast(source1);
  vtkDataArray *src2DA = vtkDataArray::FastDownCast(source2);
  if (!src1DA || !src2DA)
  {
    vtkErrorMacro("Both arrays must be vtkDataArray subclasses.");
    return;
  }

  bool fallback = type == VTK_BIT;

  if (!fallback)
  {
    InterpolateTupleWorker worker(srcTuple1, srcTuple2, dstTuple, t);
    // Use fallback if dispatch fails:
    fallback = !vtkArrayDispatch::Dispatch3SameValueType::Execute(
          src1DA, src2DA, this, worker);
  }

  // Fallback to a separate implementation that checks vtkDataArray::GetDataType
  // rather than relying on API types, since we'll need to round differently
  // depending on type, and the API type for vtkDataArray is always double.
  if (fallback)
  {
    bool doRound = !(this->GetDataType() == VTK_FLOAT ||
                     this->GetDataType() == VTK_DOUBLE);
    double typeMin = this->GetDataTypeMin();
    double typeMax = this->GetDataTypeMax();
    int numComp = source1->GetNumberOfComponents();
    double in1;
    double in2;
    double out;
    for (int c = 0; c < numComp; c++)
    {
      in1 = src1DA->GetComponent(srcTuple1, c);
      in2 = src2DA->GetComponent(srcTuple2, c);
      out = in1 + t * (in2 - in1);
      // Clamp to datatype range:
      out = std::max(out, typeMin);
      out = std::min(out, typeMax);
      // Round if needed:
      if (doRound)
      {
        out = std::floor((out >= 0.) ? (out + 0.5) : (out - 0.5));
      }
      this->InsertComponent(dstTuple, c, out);
    }
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::CreateDefaultLookupTable()
{
  if ( this->LookupTable )
  {
    this->LookupTable->UnRegister(this);
  }
  this->LookupTable = vtkLookupTable::New();
  // make sure it is built
  // otherwise problems with InsertScalar trying to map through
  // non built lut
  this->LookupTable->Build();
}

//----------------------------------------------------------------------------
void vtkDataArray::SetLookupTable(vtkLookupTable* lut)
{
  if ( this->LookupTable != lut )
  {
    if ( this->LookupTable )
    {
      this->LookupTable->UnRegister(this);
    }
    this->LookupTable = lut;
    if ( this->LookupTable )
    {
      this->LookupTable->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTupleN(vtkIdType i, int n)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != n)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != " << n);
  }
  return this->GetTuple(i);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetTuple1(vtkIdType i)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
  }
  return *(this->GetTuple(i));
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple2(vtkIdType i)
{
  return this->GetTupleN(i, 2);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple3(vtkIdType i)
{
  return this->GetTupleN(i, 3);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple4(vtkIdType i)
{
  return this->GetTupleN(i, 4);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple6(vtkIdType i)
{
  return this->GetTupleN(i, 6);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple9(vtkIdType i)
{
  return this->GetTupleN(i, 9);
}

//----------------------------------------------------------------------------
void vtkDataArray::SetTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
  }
  this->SetTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple3(vtkIdType i, double val0, double val1,
                             double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple4(vtkIdType i, double val0, double val1,
                             double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple6(vtkIdType i, double val0, double val1,
                             double val2, double val3,
                             double val4, double val5)
{
  double tuple[6];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 6)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 6");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple9(vtkIdType i, double val0, double val1,
                             double val2, double val3, double val4,
                             double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->SetTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
  }
  this->InsertTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple3(vtkIdType i, double val0, double val1,
                                double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple4(vtkIdType i, double val0, double val1,
                                double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple6(vtkIdType i, double val0, double val1,
                                double val2, double val3, double val4,
                                double val5)
{
  if (this->NumberOfComponents != 6)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << this->NumberOfComponents << " != 6");
  }
  double tuple[6] = {val0, val1, val2, val3, val4, val5};
  this->InsertTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple9(vtkIdType i, double val0, double val1,
                                double val2,  double val3, double val4,
                                double val5, double val6,double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple1(double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
  }
  this->InsertNextTuple(&value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple2(double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple3(double val0, double val1,
                                    double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple4(double val0, double val1,
                                    double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertNextTuple(tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple6(double val0, double val1, double val2,
                                    double val3, double val4, double val5)
{
  if (this->NumberOfComponents != 6)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << this->NumberOfComponents << " != 6");
  }

  double tuple[6] = {val0, val1, val2, val3, val4, val5};
  this->InsertNextTuple(tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple9(double val0, double val1,
                                    double val2,  double val3, double val4,
                                    double val5, double val6,double val7,
                                    double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertNextTuple(tuple);
}

//----------------------------------------------------------------------------
unsigned long vtkDataArray::GetActualMemorySize()
{
  vtkIdType numPrims;
  double size;
  // The allocated array may be larger than the number of primitives used.
  //numPrims = this->GetNumberOfTuples() * this->GetNumberOfComponents();
  numPrims = this->GetSize();

  size = vtkDataArray::GetDataTypeSize(this->GetDataType());

  // kibibytes
  return static_cast<unsigned long>(ceil((size*static_cast<double>(numPrims)
                                           )/1024.0));
}

//----------------------------------------------------------------------------
vtkDataArray* vtkDataArray::CreateDataArray(int dataType)
{
  vtkAbstractArray* aa = vtkAbstractArray::CreateArray(dataType);
  vtkDataArray* da = vtkDataArray::FastDownCast(aa);
  if (!da && aa)
  {
    // Requested array is not a vtkDataArray. Delete the allocated array.
    aa->Delete();
  }
  return da;
}

//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdList *tupleIds, vtkAbstractArray *aa)
{
  vtkDataArray *da = vtkDataArray::FastDownCast(aa);
  if (!da)
  {
    vtkErrorMacro("Input is not a vtkDataArray, but " << aa->GetClassName());
    return;
  }

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: " << this->GetNumberOfComponents() << "\n"
                  "Destination: " << da->GetNumberOfComponents());
    return;
  }

  GetTuplesFromListWorker worker(tupleIds);
  if (!vtkArrayDispatch::Dispatch2::Execute(this, da, worker))
  {
    // Use fallback if dispatch fails.
    worker(this, da);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *aa)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(aa);
  if (!da)
  {
    vtkWarningMacro("Input is not a vtkDataArray.");
    return;
  }

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: " << this->GetNumberOfComponents() << "\n"
                  "Destination: " << da->GetNumberOfComponents());
    return;
  }

  GetTuplesRangeWorker worker(p1, p2);
  if (!vtkArrayDispatch::Dispatch2::Execute(this, da, worker))
  {
    // Use fallback if dispatch fails.
    worker(this, da);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::FillComponent(int compIdx, double value)
{
  if (compIdx < 0 || compIdx >= this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << compIdx << " is not in [0, "
    << this->GetNumberOfComponents() << ")" );
    return;
  }

  vtkIdType i;

  for (i = 0; i < this->GetNumberOfTuples(); i++)
  {
    this->SetComponent(i, compIdx, value);
  }
}


//----------------------------------------------------------------------------
void vtkDataArray::CopyComponent(int dstComponent, vtkDataArray *src,
                                 int srcComponent)
{
  if (this->GetNumberOfTuples() != src->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of tuples in 'from' ("
    << src->GetNumberOfTuples() << ") and 'to' ("
    << this->GetNumberOfTuples() << ") do not match.");
    return;
  }

  if (dstComponent < 0 || dstComponent >= this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << dstComponent
                  << " in 'to' array is not in [0, "
                  << this->GetNumberOfComponents() << ")" );
    return;
  }

  if (srcComponent < 0 || srcComponent >= src->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << srcComponent
                  << " in 'from' array is not in [0, "
                  << src->GetNumberOfComponents() << ")" );
    return;
  }

  vtkIdType i;
  for (i = 0; i < this->GetNumberOfTuples(); i++)
  {
    this->SetComponent(i, dstComponent, src->GetComponent(i, srcComponent));
  }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetMaxNorm()
{
  vtkIdType i;
  double norm, maxNorm;
  int nComponents = this->GetNumberOfComponents();

  maxNorm = 0.0;
  for (i=0; i<this->GetNumberOfTuples(); i++)
  {
    norm = vtkMath::Norm(this->GetTuple(i), nComponents);
    if ( norm > maxNorm )
    {
      maxNorm = norm;
    }
  }

  return maxNorm;
}

//----------------------------------------------------------------------------
int vtkDataArray::CopyInformation(vtkInformation* infoFrom, int deep)
{
  // Copy everything + give base classes a chance to
  // Exclude keys which they don't want copied.
  this->Superclass::CopyInformation(infoFrom,deep);

  // Remove any keys we own that are not to be copied here.
  vtkInformation *myInfo=this->GetInformation();
  // Range:
  if (myInfo->Has( L2_NORM_RANGE() ))
  {
    myInfo->Remove( L2_NORM_RANGE() );
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkDataArray::ComputeRange(double range[2], int comp)
{
  //this method needs a large refactoring to be way easier to read

  if ( comp >= this->NumberOfComponents )
  { // Ignore requests for nonexistent components.
    return;
  }
  // If we got component -1 on a vector array, compute vector magnitude.
  if (comp < 0 && this->NumberOfComponents == 1)
  {
    comp = 0;
  }

  range[0] = vtkTypeTraits<double>::Max();
  range[1] = vtkTypeTraits<double>::Min();

  vtkInformation* info = this->GetInformation();
  vtkInformationDoubleVectorKey* rkey;
  if ( comp < 0 )
  {
    rkey = L2_NORM_RANGE();
    //hasValidKey will update range to the cached value if it exists.
    if( !hasValidKey(info,rkey,this->GetMTime(),range) )
    {

      this->ComputeVectorRange(range);
      info->Set( rkey, range, 2 );
    }
    return;
  }
  else
  {
    rkey = COMPONENT_RANGE();

    //hasValidKey will update range to the cached value if it exists.
    if( !hasValidKey(info, PER_COMPONENT(), rkey,
                       this->GetMTime(), range, comp))
    {
      double* allCompRanges = new double[this->NumberOfComponents*2];
      const bool computed = this->ComputeScalarRange(allCompRanges);
      if(computed)
      {
        //construct the keys and add them to the info object
        vtkInformationVector* infoVec = vtkInformationVector::New();
        info->Set( PER_COMPONENT(), infoVec );

        infoVec->SetNumberOfInformationObjects( this->NumberOfComponents );
        for ( int i = 0; i < this->NumberOfComponents; ++i )
        {
          infoVec->GetInformationObject( i )->Set( rkey,
                                                   allCompRanges+(i*2),
                                                   2 );
        }
        infoVec->FastDelete();

        //update the range passed in since we have a valid range.
        range[0] = allCompRanges[comp*2];
        range[1] = allCompRanges[(comp*2)+1];
      }
      delete[] allCompRanges;
    }
  }
}

namespace
{

// Wrap the DoCompute[Scalar|Vector]Range calls for vtkArrayDispatch:
struct ScalarRangeDispatchWrapper
{
  bool Success;
  double *Range;

  ScalarRangeDispatchWrapper(double *range) : Success(false), Range(range) {}

  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    this->Success = vtkDataArrayPrivate::DoComputeScalarRange(array,
                                                              this->Range);
  }

};

struct VectorRangeDispatchWrapper
{
  bool Success;
  double *Range;

  VectorRangeDispatchWrapper(double *range) : Success(false), Range(range) {}

  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    this->Success = vtkDataArrayPrivate::DoComputeVectorRange(array,
                                                              this->Range);
  }

};

} // end anon namespace

//----------------------------------------------------------------------------
bool vtkDataArray::ComputeScalarRange(double* ranges)
{
  ScalarRangeDispatchWrapper worker(ranges);
  if (!vtkArrayDispatch::Dispatch::Execute(this, worker))
  {
    worker(this);
  }
  return worker.Success;
}

//-----------------------------------------------------------------------------
bool vtkDataArray::ComputeVectorRange(double range[2])
{
  VectorRangeDispatchWrapper worker(range);
  if (!vtkArrayDispatch::Dispatch::Execute(this, worker))
  {
    worker(this);
  }
  return worker.Success;
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(double range[2])
{
  vtkDataArray::GetDataTypeRange(this->GetDataType(), range);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetDataType());
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetDataType());
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(int type, double range[2])
{
  range[0] = vtkDataArray::GetDataTypeMin(type);
  range[1] = vtkDataArray::GetDataTypeMax(type);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin(int type)
{
  switch (type)
  {
    case VTK_BIT:                return static_cast<double>(VTK_BIT_MIN);
    case VTK_SIGNED_CHAR:        return static_cast<double>(VTK_SIGNED_CHAR_MIN);
    case VTK_UNSIGNED_CHAR:      return static_cast<double>(VTK_UNSIGNED_CHAR_MIN);
    case VTK_CHAR:               return static_cast<double>(VTK_CHAR_MIN);
    case VTK_UNSIGNED_SHORT:     return static_cast<double>(VTK_UNSIGNED_SHORT_MIN);
    case VTK_SHORT:              return static_cast<double>(VTK_SHORT_MIN);
    case VTK_UNSIGNED_INT:       return static_cast<double>(VTK_UNSIGNED_INT_MIN);
    case VTK_INT:                return static_cast<double>(VTK_INT_MIN);
    case VTK_UNSIGNED_LONG:      return static_cast<double>(VTK_UNSIGNED_LONG_MIN);
    case VTK_LONG:               return static_cast<double>(VTK_LONG_MIN);
    case VTK_UNSIGNED_LONG_LONG: return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MIN);
    case VTK_LONG_LONG:          return static_cast<double>(VTK_LONG_LONG_MIN);
    case VTK_FLOAT:              return static_cast<double>(VTK_FLOAT_MIN);
    case VTK_DOUBLE:             return static_cast<double>(VTK_DOUBLE_MIN);
    case VTK_ID_TYPE:            return static_cast<double>(VTK_ID_MIN);
    default: return 0;
  }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax(int type)
{
  switch (type)
  {
    case VTK_BIT:                return static_cast<double>(VTK_BIT_MAX);
    case VTK_SIGNED_CHAR:        return static_cast<double>(VTK_SIGNED_CHAR_MAX);
    case VTK_UNSIGNED_CHAR:      return static_cast<double>(VTK_UNSIGNED_CHAR_MAX);
    case VTK_CHAR:               return static_cast<double>(VTK_CHAR_MAX);
    case VTK_UNSIGNED_SHORT:     return static_cast<double>(VTK_UNSIGNED_SHORT_MAX);
    case VTK_SHORT:              return static_cast<double>(VTK_SHORT_MAX);
    case VTK_UNSIGNED_INT:       return static_cast<double>(VTK_UNSIGNED_INT_MAX);
    case VTK_INT:                return static_cast<double>(VTK_INT_MAX);
    case VTK_UNSIGNED_LONG:      return static_cast<double>(VTK_UNSIGNED_LONG_MAX);
    case VTK_LONG:               return static_cast<double>(VTK_LONG_MAX);
    case VTK_UNSIGNED_LONG_LONG: return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MAX);
    case VTK_LONG_LONG:          return static_cast<double>(VTK_LONG_LONG_MAX);
    case VTK_FLOAT:              return static_cast<double>(VTK_FLOAT_MAX);
    case VTK_DOUBLE:             return static_cast<double>(VTK_DOUBLE_MAX);
    case VTK_ID_TYPE:            return static_cast<double>(VTK_ID_MAX);
    default: return 1;
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::RemoveLastTuple()
{
  if (this->GetNumberOfTuples() > 0)
  {
    this->Resize(this->GetNumberOfTuples() - 1);
  }
}

//----------------------------------------------------------------------------
void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  const char* name = this->GetName();
  if (name)
  {
    os << indent << "Name: " << name << "\n";
  }
  else
  {
    os << indent << "Name: (none)\n";
  }
  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  if ( this->LookupTable )
  {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "LookupTable: (none)\n";
  }
}
