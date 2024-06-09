// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // For fast paths
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayPrivate.txx"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericDataArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkSOADataArrayTemplate.h" // For fast paths
#ifdef VTK_USE_SCALED_SOA_ARRAYS
#include "vtkScaledSOADataArrayTemplate.h" // For fast paths
#endif
#include "vtkSMPTools.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm> // for min(), max()
#include <vector>

namespace
{

template <typename InfoType, typename KeyType>
bool hasValidKey(InfoType info, KeyType key, double range[2])
{
  if (info->Has(key))
  {
    info->Get(key, range);
    return true;
  }
  return false;
}

template <typename InfoType, typename KeyType, typename ComponentKeyType>
bool hasValidKey(InfoType info, KeyType key, ComponentKeyType ckey, double range[2], int comp)
{
  if (info->Has(key))
  {
    info->Get(key)->GetInformationObject(comp)->Get(ckey, range);
    return true;
  }
  return false;
}

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN

vtkInformationKeyRestrictedMacro(vtkDataArray, COMPONENT_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataArray, L2_NORM_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataArray, L2_NORM_FINITE_RANGE, DoubleVector, 2);
vtkInformationKeyMacro(vtkDataArray, UNITS_LABEL, String);

//------------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray()
{
  this->LookupTable = nullptr;
  this->Range[0] = 0;
  this->Range[1] = 0;
  this->FiniteRange[0] = 0;
  this->FiniteRange[1] = 0;
}

//------------------------------------------------------------------------------
vtkDataArray::~vtkDataArray()
{
  if (this->LookupTable)
  {
    this->LookupTable->Delete();
  }
  this->SetName(nullptr);
}

//------------------------------------------------------------------------------
void vtkDataArray::DeepCopy(vtkAbstractArray* aa)
{
  if (aa == nullptr)
  {
    return;
  }

  vtkDataArray* da = vtkDataArray::FastDownCast(aa);
  if (da == nullptr)
  {
    vtkErrorMacro(<< "Input array is not a vtkDataArray (" << aa->GetClassName() << ")");
    return;
  }

  this->DeepCopy(da);
}

//------------------------------------------------------------------------------
void vtkDataArray::ShallowCopy(vtkDataArray* other)
{
  // Deep copy by default. Subclasses may override this behavior.
  this->DeepCopy(other);
}

//------------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType i, const float* source)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
  {
    this->SetComponent(i, c, static_cast<double>(source[c]));
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType i, const double* source)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
  {
    this->SetComponent(i, c, source[c]);
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple(
  vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source)
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

//------------------------------------------------------------------------------
vtkIdType vtkDataArray::InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  vtkIdType tupleIdx = this->GetNumberOfTuples();
  this->InsertTuple(tupleIdx, srcTupleIdx, source);
  return tupleIdx;
}

//------------------------------------------------------------------------------
// These can be overridden for more efficiency
double vtkDataArray::GetComponent(vtkIdType tupleIdx, int compIdx)
{
  double *tuple = new double[this->NumberOfComponents], c;

  this->GetTuple(tupleIdx, tuple);
  c = tuple[compIdx];
  delete[] tuple;

  return c;
}

//------------------------------------------------------------------------------
void vtkDataArray::SetComponent(vtkIdType tupleIdx, int compIdx, double value)
{
  double* tuple = new double[this->NumberOfComponents];

  if (tupleIdx < this->GetNumberOfTuples())
  {
    this->GetTuple(tupleIdx, tuple);
  }
  else
  {
    for (int k = 0; k < this->NumberOfComponents; k++)
    {
      tuple[k] = 0.0;
    }
  }

  tuple[compIdx] = value;
  this->SetTuple(tupleIdx, tuple);

  delete[] tuple;
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertComponent(vtkIdType tupleIdx, int compIdx, double value)
{
  double* tuple = new double[this->NumberOfComponents];

  if (tupleIdx < this->GetNumberOfTuples())
  {
    this->GetTuple(tupleIdx, tuple);
  }
  else
  {
    for (int k = 0; k < this->NumberOfComponents; k++)
    {
      tuple[k] = 0.0;
    }
  }

  tuple[compIdx] = value;
  this->InsertTuple(tupleIdx, tuple);

  delete[] tuple;
}

//------------------------------------------------------------------------------
void vtkDataArray::GetData(
  vtkIdType tupleMin, vtkIdType tupleMax, int compMin, int compMax, vtkDoubleArray* data)
{
  int i;
  vtkIdType j;
  int numComp = this->GetNumberOfComponents();
  double* tuple = new double[numComp];
  double* ptr = data->WritePointer(0, (tupleMax - tupleMin + 1) * (compMax - compMin + 1));

  for (j = tupleMin; j <= tupleMax; j++)
  {
    this->GetTuple(j, tuple);
    for (i = compMin; i <= compMax; i++)
    {
      *ptr++ = tuple[i];
    }
  }
  delete[] tuple;
}

//------------------------------------------------------------------------------
void vtkDataArray::GetIntegerTuple(vtkIdType tupleIdx, vtkTypeInt64* tuple)
{
  static VTK_THREAD_LOCAL std::vector<double> data;
  int numComp = this->GetNumberOfComponents();
  data.resize(numComp);
  this->GetTuple(tupleIdx, data.data());
  for (int ii = 0; ii < numComp; ++ii)
  {
    tuple[ii] = static_cast<vtkTypeInt64>(data[ii]);
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::SetIntegerTuple(vtkIdType tupleIdx, vtkTypeInt64* tuple)
{
  static VTK_THREAD_LOCAL std::vector<double> data;
  int numComp = this->GetNumberOfComponents();
  data.resize(numComp);
  for (int ii = 0; ii < numComp; ++ii)
  {
    data[ii] = static_cast<double>(tuple[ii]);
  }
  this->SetTuple(tupleIdx, data.data());
}

//------------------------------------------------------------------------------
void vtkDataArray::GetUnsignedTuple(vtkIdType tupleIdx, vtkTypeUInt64* tuple)
{
  static VTK_THREAD_LOCAL std::vector<double> data;
  int numComp = this->GetNumberOfComponents();
  data.resize(numComp);
  this->GetTuple(tupleIdx, data.data());
  for (int ii = 0; ii < numComp; ++ii)
  {
    tuple[ii] = static_cast<vtkTypeUInt64>(data[ii]);
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::SetUnsignedTuple(vtkIdType tupleIdx, vtkTypeUInt64* tuple)
{
  static VTK_THREAD_LOCAL std::vector<double> data;
  int numComp = this->GetNumberOfComponents();
  data.resize(numComp);
  for (int ii = 0; ii < numComp; ++ii)
  {
    data[ii] = static_cast<double>(tuple[ii]);
  }
  this->SetTuple(tupleIdx, data.data());
}

//------------------------------------------------------------------------------
void vtkDataArray::CreateDefaultLookupTable()
{
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  this->LookupTable = vtkLookupTable::New();
  // make sure it is built
  // otherwise problems with InsertScalar trying to map through
  // non built lut
  this->LookupTable->Build();
}

//------------------------------------------------------------------------------
void vtkDataArray::SetLookupTable(vtkLookupTable* lut)
{
  if (this->LookupTable != lut)
  {
    if (this->LookupTable)
    {
      this->LookupTable->UnRegister(this);
    }
    this->LookupTable = lut;
    if (this->LookupTable)
    {
      this->LookupTable->Register(this);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkDataArray::GetTupleN(vtkIdType i, int n)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != n)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != " << n);
  }
  return this->GetTuple(i);
}

//------------------------------------------------------------------------------
double vtkDataArray::GetTuple1(vtkIdType i)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 1");
  }
  return *(this->GetTuple(i));
}

//------------------------------------------------------------------------------
double* vtkDataArray::GetTuple2(vtkIdType i)
{
  return this->GetTupleN(i, 2);
}
//------------------------------------------------------------------------------
double* vtkDataArray::GetTuple3(vtkIdType i)
{
  return this->GetTupleN(i, 3);
}
//------------------------------------------------------------------------------
double* vtkDataArray::GetTuple4(vtkIdType i)
{
  return this->GetTupleN(i, 4);
}
//------------------------------------------------------------------------------
double* vtkDataArray::GetTuple6(vtkIdType i)
{
  return this->GetTupleN(i, 6);
}
//------------------------------------------------------------------------------
double* vtkDataArray::GetTuple9(vtkIdType i)
{
  return this->GetTupleN(i, 9);
}

//------------------------------------------------------------------------------
void vtkDataArray::SetTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 1");
  }
  this->SetTuple(i, &value);
}
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->SetTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple3(vtkIdType i, double val0, double val1, double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->SetTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple4(vtkIdType i, double val0, double val1, double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->SetTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple6(
  vtkIdType i, double val0, double val1, double val2, double val3, double val4, double val5)
{
  double tuple[6];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 6)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 6");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  this->SetTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple9(vtkIdType i, double val0, double val1, double val2, double val3,
  double val4, double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 9");
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

//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 1");
  }
  this->InsertTuple(i, &value);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple3(vtkIdType i, double val0, double val1, double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertTuple(i, tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple4(vtkIdType i, double val0, double val1, double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertTuple(i, tuple);
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple6(
  vtkIdType i, double val0, double val1, double val2, double val3, double val4, double val5)
{
  if (this->NumberOfComponents != 6)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
      << this->NumberOfComponents << " != 6");
  }
  double tuple[6] = { val0, val1, val2, val3, val4, val5 };
  this->InsertTuple(i, tuple);
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertTuple9(vtkIdType i, double val0, double val1, double val2, double val3,
  double val4, double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 9");
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

//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple1(double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 1");
  }
  this->InsertNextTuple(&value);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple2(double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 2");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertNextTuple(tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple3(double val0, double val1, double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 3");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertNextTuple(tuple);
}
//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple4(double val0, double val1, double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 4");
  }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertNextTuple(tuple);
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple6(
  double val0, double val1, double val2, double val3, double val4, double val5)
{
  if (this->NumberOfComponents != 6)
  {
    vtkErrorMacro("The number of components do not match the number requested: "
      << this->NumberOfComponents << " != 6");
  }

  double tuple[6] = { val0, val1, val2, val3, val4, val5 };
  this->InsertNextTuple(tuple);
}

//------------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple9(double val0, double val1, double val2, double val3, double val4,
  double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
  {
    vtkErrorMacro(
      "The number of components do not match the number requested: " << numComp << " != 9");
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

//------------------------------------------------------------------------------
unsigned long vtkDataArray::GetActualMemorySize() const
{
  vtkIdType numPrims;
  double size;
  // The allocated array may be larger than the number of primitives used.
  // numPrims = this->GetNumberOfTuples() * this->GetNumberOfComponents();
  numPrims = this->GetSize();

  size = vtkDataArray::GetDataTypeSize(this->GetDataType());

  // kibibytes
  return static_cast<unsigned long>(ceil((size * static_cast<double>(numPrims)) / 1024.0));
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkDataArray::FillComponent(int compIdx, double value)
{
  if (compIdx < 0 || compIdx >= this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << compIdx << " is not in [0, "
                  << this->GetNumberOfComponents() << ")");
    return;
  }

  // Xcode 8.2 calls GetNumberOfTuples() after each iteration.
  // Prevent this by storing the result in a local variable.
  vtkIdType numberOfTuples = this->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numberOfTuples; i++)
  {
    this->SetComponent(i, compIdx, value);
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::Fill(double value)
{
  for (int i = 0; i < this->GetNumberOfComponents(); ++i)
  {
    this->FillComponent(i, value);
  }
}

//------------------------------------------------------------------------------
double vtkDataArray::GetMaxNorm()
{
  vtkIdType i;
  double norm, maxNorm;
  int nComponents = this->GetNumberOfComponents();

  maxNorm = 0.0;
  for (i = 0; i < this->GetNumberOfTuples(); i++)
  {
    norm = vtkMath::Norm(this->GetTuple(i), nComponents);
    if (norm > maxNorm)
    {
      maxNorm = norm;
    }
  }

  return maxNorm;
}

//------------------------------------------------------------------------------
int vtkDataArray::CopyInformation(vtkInformation* infoFrom, vtkTypeBool deep)
{
  // Copy everything + give base classes a chance to
  // Exclude keys which they don't want copied.
  this->Superclass::CopyInformation(infoFrom, deep);

  // Remove any keys we own that are not to be copied here.
  vtkInformation* myInfo = this->GetInformation();
  // Range:
  if (myInfo->Has(L2_NORM_RANGE()))
  {
    myInfo->Remove(L2_NORM_RANGE());
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkDataArray::ComputeFiniteRange(double range[2], int comp)
{
  this->ComputeFiniteRange(range, comp, nullptr);
}

//------------------------------------------------------------------------------
void vtkDataArray::ComputeFiniteRange(
  double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  // this method needs a large refactoring to be way easier to read

  if (comp >= this->NumberOfComponents)
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
  if (comp < 0)
  {
    if (ghosts)
    {
      this->ComputeFiniteVectorRange(range, ghosts, ghostsToSkip);
      return;
    }
    rkey = L2_NORM_FINITE_RANGE();
    // hasValidKey will update range to the cached value if it exists.
    if (!hasValidKey(info, rkey, range))
    {
      this->ComputeFiniteVectorRange(range);
      info->Set(rkey, range, 2);
    }
    return;
  }
  else
  {
    std::vector<double> allCompRanges(this->NumberOfComponents * 2);
    if (ghosts)
    {
      if (this->ComputeFiniteScalarRange(allCompRanges.data(), ghosts, ghostsToSkip))
      {
        range[0] = allCompRanges[comp * 2];
        range[1] = allCompRanges[(comp * 2) + 1];
      }
      return;
    }
    rkey = COMPONENT_RANGE();

    // hasValidKey will update range to the cached value if it exists.
    if (!hasValidKey(info, PER_FINITE_COMPONENT(), rkey, range, comp))
    {
      const bool computed = this->ComputeFiniteScalarRange(allCompRanges.data());
      if (computed)
      {
        // construct the keys and add them to the info object
        vtkInformationVector* infoVec = vtkInformationVector::New();
        info->Set(PER_FINITE_COMPONENT(), infoVec);

        infoVec->SetNumberOfInformationObjects(this->NumberOfComponents);
        for (int i = 0; i < this->NumberOfComponents; ++i)
        {
          infoVec->GetInformationObject(i)->Set(rkey, allCompRanges.data() + (i * 2), 2);
        }
        infoVec->FastDelete();

        // update the range passed in since we have a valid range.
        range[0] = allCompRanges[comp * 2];
        range[1] = allCompRanges[(comp * 2) + 1];
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::ComputeRange(double range[2], int comp)
{
  this->ComputeRange(range, comp, nullptr);
}

//------------------------------------------------------------------------------
void vtkDataArray::ComputeRange(
  double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  // this method needs a large refactoring to be way easier to read

  if (comp >= this->NumberOfComponents)
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
  if (comp < 0)
  {
    if (ghosts)
    {
      this->ComputeVectorRange(range, ghosts, ghostsToSkip);
      return;
    }
    rkey = L2_NORM_RANGE();
    // hasValidKey will update range to the cached value if it exists.
    if (!hasValidKey(info, rkey, range))
    {
      this->ComputeVectorRange(range);
      info->Set(rkey, range, 2);
    }
    return;
  }
  else
  {
    std::vector<double> allCompRanges(this->NumberOfComponents * 2);
    if (ghosts)
    {
      if (this->ComputeScalarRange(allCompRanges.data(), ghosts, ghostsToSkip))
      {
        range[0] = allCompRanges[comp * 2];
        range[1] = allCompRanges[(comp * 2) + 1];
      }
      return;
    }
    rkey = COMPONENT_RANGE();

    // hasValidKey will update range to the cached value if it exists.
    if (!hasValidKey(info, PER_COMPONENT(), rkey, range, comp))
    {
      const bool computed = this->ComputeScalarRange(allCompRanges.data());
      if (computed)
      {
        // construct the keys and add them to the info object
        vtkInformationVector* infoVec = vtkInformationVector::New();
        info->Set(PER_COMPONENT(), infoVec);

        infoVec->SetNumberOfInformationObjects(this->NumberOfComponents);
        for (int i = 0; i < this->NumberOfComponents; ++i)
        {
          infoVec->GetInformationObject(i)->Set(rkey, allCompRanges.data() + (i * 2), 2);
        }
        infoVec->FastDelete();

        // update the range passed in since we have a valid range.
        range[0] = allCompRanges[comp * 2];
        range[1] = allCompRanges[(comp * 2) + 1];
      }
    }
  }
}

//------------------------------------------------------------------------------
// call modified on superclass
void vtkDataArray::Modified()
{
  if (this->HasInformation())
  {
    // Clear key-value pairs that are now out of date.
    vtkInformation* info = this->GetInformation();
    info->Remove(L2_NORM_RANGE());
    info->Remove(L2_NORM_FINITE_RANGE());
  }
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(double range[2])
{
  vtkDataArray::GetDataTypeRange(this->GetDataType(), range);
}

//------------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetDataType());
}

//------------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetDataType());
}

//------------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(int type, double range[2])
{
  range[0] = vtkDataArray::GetDataTypeMin(type);
  range[1] = vtkDataArray::GetDataTypeMax(type);
}

//------------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin(int type)
{
  switch (type)
  {
    case VTK_BIT:
      return static_cast<double>(VTK_BIT_MIN);
    case VTK_SIGNED_CHAR:
      return static_cast<double>(VTK_SIGNED_CHAR_MIN);
    case VTK_UNSIGNED_CHAR:
      return static_cast<double>(VTK_UNSIGNED_CHAR_MIN);
    case VTK_CHAR:
      return static_cast<double>(VTK_CHAR_MIN);
    case VTK_UNSIGNED_SHORT:
      return static_cast<double>(VTK_UNSIGNED_SHORT_MIN);
    case VTK_SHORT:
      return static_cast<double>(VTK_SHORT_MIN);
    case VTK_UNSIGNED_INT:
      return static_cast<double>(VTK_UNSIGNED_INT_MIN);
    case VTK_INT:
      return static_cast<double>(VTK_INT_MIN);
    case VTK_UNSIGNED_LONG:
      return static_cast<double>(VTK_UNSIGNED_LONG_MIN);
    case VTK_LONG:
      return static_cast<double>(VTK_LONG_MIN);
    case VTK_UNSIGNED_LONG_LONG:
      return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MIN);
    case VTK_LONG_LONG:
      return static_cast<double>(VTK_LONG_LONG_MIN);
    case VTK_FLOAT:
      return static_cast<double>(VTK_FLOAT_MIN);
    case VTK_DOUBLE:
      return static_cast<double>(VTK_DOUBLE_MIN);
    case VTK_ID_TYPE:
      return static_cast<double>(VTK_ID_MIN);
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax(int type)
{
  switch (type)
  {
    case VTK_BIT:
      return static_cast<double>(VTK_BIT_MAX);
    case VTK_SIGNED_CHAR:
      return static_cast<double>(VTK_SIGNED_CHAR_MAX);
    case VTK_UNSIGNED_CHAR:
      return static_cast<double>(VTK_UNSIGNED_CHAR_MAX);
    case VTK_CHAR:
      return static_cast<double>(VTK_CHAR_MAX);
    case VTK_UNSIGNED_SHORT:
      return static_cast<double>(VTK_UNSIGNED_SHORT_MAX);
    case VTK_SHORT:
      return static_cast<double>(VTK_SHORT_MAX);
    case VTK_UNSIGNED_INT:
      return static_cast<double>(VTK_UNSIGNED_INT_MAX);
    case VTK_INT:
      return static_cast<double>(VTK_INT_MAX);
    case VTK_UNSIGNED_LONG:
      return static_cast<double>(VTK_UNSIGNED_LONG_MAX);
    case VTK_LONG:
      return static_cast<double>(VTK_LONG_MAX);
    case VTK_UNSIGNED_LONG_LONG:
      return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MAX);
    case VTK_LONG_LONG:
      return static_cast<double>(VTK_LONG_LONG_MAX);
    case VTK_FLOAT:
      return static_cast<double>(VTK_FLOAT_MAX);
    case VTK_DOUBLE:
      return static_cast<double>(VTK_DOUBLE_MAX);
    case VTK_ID_TYPE:
      return static_cast<double>(VTK_ID_MAX);
    default:
      return 1;
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::RemoveLastTuple()
{
  if (this->GetNumberOfTuples() > 0)
  {
    this->Resize(this->GetNumberOfTuples() - 1);
  }
}

//------------------------------------------------------------------------------
void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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
  if (this->LookupTable)
  {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LookupTable: (none)\n";
  }
}
VTK_ABI_NAMESPACE_END
