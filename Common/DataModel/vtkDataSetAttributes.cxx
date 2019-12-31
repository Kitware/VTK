/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetAttributes.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkAssume.h"
#include "vtkCell.h"
#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"
#include "vtkStructuredExtent.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include <vector>

vtkStandardNewMacro(vtkDataSetAttributes);
//--------------------------------------------------------------------------
const char vtkDataSetAttributes::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][19] = {
  "Scalars",
  "Vectors",
  "Normals",
  "TCoords",
  "Tensors",
  "GlobalIds",
  "PedigreeIds",
  "EdgeFlag",
  "Tangents",
  "RationalWeights",
  "HigherOrderDegrees",
};

const char vtkDataSetAttributes::LongAttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][42] = {
  "vtkDataSetAttributes::SCALARS",
  "vtkDataSetAttributes::VECTORS",
  "vtkDataSetAttributes::NORMALS",
  "vtkDataSetAttributes::TCOORDS",
  "vtkDataSetAttributes::TENSORS",
  "vtkDataSetAttributes::GLOBALIDS",
  "vtkDataSetAttributes::PEDIGREEIDS",
  "vtkDataSetAttributes::EDGEFLAG",
  "vtkDataSetAttributes::TANGENTS",
  "vtkDataSetAttributes::RATIONALWEIGHTS",
  "vtkDataSetAttributes::HIGHERORDERDEGREES",
};

//--------------------------------------------------------------------------
// Construct object with copying turned on for all data.
vtkDataSetAttributes::vtkDataSetAttributes()
{
  int attributeType;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    this->AttributeIndices[attributeType] = -1;
    this->CopyAttributeFlags[COPYTUPLE][attributeType] = 1;
    this->CopyAttributeFlags[INTERPOLATE][attributeType] = 1;
    this->CopyAttributeFlags[PASSDATA][attributeType] = 1;
  }

  // Global IDs should not be interpolated because they are labels, not "numbers"
  // Global IDs should not be copied either, unless doing so preserves meaning.
  // Passing through is usually OK because it is 1:1.
  this->CopyAttributeFlags[COPYTUPLE][GLOBALIDS] = 0;
  this->CopyAttributeFlags[INTERPOLATE][GLOBALIDS] = 0;

  // Pedigree IDs should not be interpolated because they are labels, not "numbers"
  // Pedigree IDs may be copied since they do not require 1:1 mapping.
  this->CopyAttributeFlags[INTERPOLATE][PEDIGREEIDS] = 0;

  this->TargetIndices = nullptr;
}

//--------------------------------------------------------------------------
// Destructor for the vtkDataSetAttributes objects.
vtkDataSetAttributes::~vtkDataSetAttributes()
{
  this->Initialize();
  delete[] this->TargetIndices;
  this->TargetIndices = nullptr;
}

//--------------------------------------------------------------------------
// Turn on copying of all data.
void vtkDataSetAttributes::CopyAllOn(int ctype)
{
  this->vtkFieldData::CopyAllOn();
  this->SetCopyScalars(1, ctype);
  this->SetCopyVectors(1, ctype);
  this->SetCopyNormals(1, ctype);
  this->SetCopyTCoords(1, ctype);
  this->SetCopyTensors(1, ctype);
  this->SetCopyGlobalIds(1, ctype);
  this->SetCopyPedigreeIds(1, ctype);
  this->SetCopyTangents(1, ctype);
  this->SetCopyRationalWeights(1, ctype);
  this->SetCopyHigherOrderDegrees(1, ctype);
}

//--------------------------------------------------------------------------
// Turn off copying of all data.
void vtkDataSetAttributes::CopyAllOff(int ctype)
{
  this->vtkFieldData::CopyAllOff();
  this->SetCopyScalars(0, ctype);
  this->SetCopyVectors(0, ctype);
  this->SetCopyNormals(0, ctype);
  this->SetCopyTCoords(0, ctype);
  this->SetCopyTensors(0, ctype);
  this->SetCopyGlobalIds(0, ctype);
  this->SetCopyPedigreeIds(0, ctype);
  this->SetCopyTangents(0, ctype);
  this->SetCopyRationalWeights(0, ctype);
  this->SetCopyHigherOrderDegrees(0, ctype);
}

//--------------------------------------------------------------------------
// Deep copy of data (i.e., create new data arrays and
// copy from input data). Note that attribute data is
// not copied.
void vtkDataSetAttributes::DeepCopy(vtkFieldData* fd)
{
  this->Initialize(); // free up memory

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
  // If the source is a vtkDataSetAttributes
  if (dsa)
  {
    int numArrays = fd->GetNumberOfArrays();
    int attributeType, i;
    vtkAbstractArray *data, *newData;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    for (i = 0; i < numArrays; i++)
    {
      data = fd->GetAbstractArray(i);
      newData = data->NewInstance(); // instantiate same type of object
      newData->DeepCopy(data);
      newData->SetName(data->GetName());
      this->AddArray(newData);
      newData->Delete();
    }
    // Copy the copy flags
    for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
    {
      // If an array is an attribute in the source, then mark it as a attribute
      // in the clone as well.
      this->AttributeIndices[attributeType] = dsa->AttributeIndices[attributeType];

      this->CopyAttributeFlags[COPYTUPLE][attributeType] =
        dsa->CopyAttributeFlags[COPYTUPLE][attributeType];
      this->CopyAttributeFlags[INTERPOLATE][attributeType] =
        dsa->CopyAttributeFlags[INTERPOLATE][attributeType];
      this->CopyAttributeFlags[PASSDATA][attributeType] =
        dsa->CopyAttributeFlags[PASSDATA][attributeType];
    }
    this->CopyFlags(dsa);
  }
  // If the source is field data, do a field data copy
  else
  {
    this->vtkFieldData::DeepCopy(fd);
  }
}

//--------------------------------------------------------------------------
// Shallow copy of data (i.e., use reference counting).
void vtkDataSetAttributes::ShallowCopy(vtkFieldData* fd)
{
  this->Initialize(); // free up memory

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
  // If the source is a vtkDataSetAttributes
  if (dsa)
  {
    int numArrays = fd->GetNumberOfArrays();
    int attributeType, i;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    this->NumberOfActiveArrays = 0;
    for (i = 0; i < numArrays; i++)
    {
      this->NumberOfActiveArrays++;
      this->SetArray(i, fd->GetAbstractArray(i));
    }

    // Copy the copy flags
    for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
    {
      // If an array is an attribute in the source, then mark it as a attribute
      // in the clone as well.
      this->AttributeIndices[attributeType] = dsa->AttributeIndices[attributeType];

      this->CopyAttributeFlags[COPYTUPLE][attributeType] =
        dsa->CopyAttributeFlags[COPYTUPLE][attributeType];
      this->CopyAttributeFlags[INTERPOLATE][attributeType] =
        dsa->CopyAttributeFlags[INTERPOLATE][attributeType];
      this->CopyAttributeFlags[PASSDATA][attributeType] =
        dsa->CopyAttributeFlags[PASSDATA][attributeType];
    }
    this->CopyFlags(dsa);
  }
  // If the source is field data, do a field data copy
  else
  {
    this->vtkFieldData::ShallowCopy(fd);
  }
}

//--------------------------------------------------------------------------
// Initialize all of the object's data to nullptr
void vtkDataSetAttributes::InitializeFields()
{
  this->vtkFieldData::InitializeFields();

  int attributeType;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    this->AttributeIndices[attributeType] = -1;
    this->CopyAttributeFlags[COPYTUPLE][attributeType] = 1;
    this->CopyAttributeFlags[INTERPOLATE][attributeType] = 1;
    this->CopyAttributeFlags[PASSDATA][attributeType] = 1;
  }
  this->CopyAttributeFlags[COPYTUPLE][GLOBALIDS] = 0;
  this->CopyAttributeFlags[INTERPOLATE][GLOBALIDS] = 0;

  this->CopyAttributeFlags[INTERPOLATE][PEDIGREEIDS] = 0;
}

//--------------------------------------------------------------------------
// Initialize all of the object's data to nullptr
void vtkDataSetAttributes::Initialize()
{
  //
  // We don't modify ourselves because the "ReleaseData" methods depend upon
  // no modification when initialized.
  //

  // Call superclass' Initialize()
  this->vtkFieldData::Initialize();
  //
  // Free up any memory
  // And don't forget to reset the attribute copy flags.
  int attributeType;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    this->AttributeIndices[attributeType] = -1;
    this->CopyAttributeFlags[COPYTUPLE][attributeType] = 1;
    this->CopyAttributeFlags[INTERPOLATE][attributeType] = 1;
    this->CopyAttributeFlags[PASSDATA][attributeType] = 1;
  }
  this->CopyAttributeFlags[COPYTUPLE][GLOBALIDS] = 0;
  this->CopyAttributeFlags[INTERPOLATE][GLOBALIDS] = 0;

  this->CopyAttributeFlags[INTERPOLATE][PEDIGREEIDS] = 0;
}

//--------------------------------------------------------------------------
// This method is used to determine which arrays
// will be copied to this object
vtkFieldData::BasicIterator vtkDataSetAttributes::ComputeRequiredArrays(
  vtkDataSetAttributes* pd, int ctype)
{
  if ((ctype < COPYTUPLE) || (ctype > PASSDATA))
  {
    vtkErrorMacro("Must call compute required with COPYTUPLE, INTERPOLATE or PASSDATA");
    ctype = COPYTUPLE;
  }

  // We need to do some juggling to find the number of arrays
  // which will be passed.

  // First, find the number of arrays to be copied because they
  // are in the list of _fields_ to be copied (and the actual data
  // pointer is non-nullptr). Also, we keep those indices in a list.
  int* copyFlags = new int[pd->GetNumberOfArrays()];
  int index, i, numArrays = 0;
  for (i = 0; i < pd->GetNumberOfArrays(); i++)
  {
    const char* arrayName = pd->GetArrayName(i);
    // If there is no blocker for the given array
    // and both CopyAllOff and CopyOn for that array are not true
    if ((this->GetFlag(arrayName) != 0) &&
      !(this->DoCopyAllOff && (this->GetFlag(arrayName) != 1)) && pd->GetAbstractArray(i))
    {
      // Cannot interpolate idtype arrays
      if (ctype != INTERPOLATE || pd->GetAbstractArray(i)->GetDataType() != VTK_ID_TYPE)
      {
        copyFlags[numArrays] = i;
        numArrays++;
      }
    }
  }

  // Next, we check the arrays to be copied because they are one of
  // the _attributes_ to be copied (and the data array in non-nullptr).
  // We make sure that we don't count anything twice.
  int alreadyCopied;
  int attributeType, j;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    index = pd->AttributeIndices[attributeType];
    int flag = this->GetFlag(pd->GetArrayName(index));
    // If this attribute is to be copied
    if (this->CopyAttributeFlags[ctype][attributeType] && flag)
    {
      // Find out if it is also in the list of fields to be copied
      // Since attributes can only be vtkDataArray, we use GetArray() call.
      if (pd->GetArray(index))
      {
        alreadyCopied = 0;
        for (i = 0; i < numArrays; i++)
        {
          if (index == copyFlags[i])
          {
            alreadyCopied = 1;
          }
        }
        // If not, increment the number of arrays to be copied.
        if (!alreadyCopied)
        {
          // Cannot interpolate idtype arrays
          if (ctype != INTERPOLATE || pd->GetArray(index)->GetDataType() != VTK_ID_TYPE)
          {
            copyFlags[numArrays] = index;
            numArrays++;
          }
        }
      }
    }
    // If it is not to be copied and it is in the list (from the
    // previous pass), remove it
    else
    {
      for (i = 0; i < numArrays; i++)
      {
        if (index == copyFlags[i])
        {
          for (j = i; j < numArrays - 1; j++)
          {
            copyFlags[j] = copyFlags[j + 1];
          }
          numArrays--;
          i--;
        }
      }
    }
  }

  vtkFieldData::BasicIterator it(copyFlags, numArrays);
  delete[] copyFlags;
  return it;
}

//--------------------------------------------------------------------------
// Pass entire arrays of input data through to output. Obey the "copy"
// flags.
void vtkDataSetAttributes::PassData(vtkFieldData* fd)
{
  if (!fd)
  {
    return;
  }

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);

  if (dsa)
  {
    // Create an iterator to iterate over the fields which will
    // be passed, i.e. fields which are either:
    // 1> in the list of _fields_ to be copied or
    // 2> in the list of _attributes_ to be copied.
    // Note that nullptr data arrays are not copied

    vtkFieldData::BasicIterator it = this->ComputeRequiredArrays(dsa, PASSDATA);

    if (it.GetListSize() > this->NumberOfArrays)
    {
      this->AllocateArrays(it.GetListSize());
    }
    if (it.GetListSize() == 0)
    {
      return;
    }

    // Since we are replacing, remove old attributes
    int attributeType; // will change//
    for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
    {
      if (this->CopyAttributeFlags[PASSDATA][attributeType])
      {
        this->RemoveArray(this->AttributeIndices[attributeType]);
        this->AttributeIndices[attributeType] = -1;
      }
    }

    int i, arrayIndex;
    for (i = it.BeginIndex(); !it.End(); i = it.NextIndex())
    {
      arrayIndex = this->AddArray(dsa->GetAbstractArray(i));
      // If necessary, make the array an attribute
      if (((attributeType = dsa->IsArrayAnAttribute(i)) != -1) &&
        this->CopyAttributeFlags[PASSDATA][attributeType])
      {
        this->SetActiveAttribute(arrayIndex, attributeType);
      }
    }
  }
  else
  {
    this->vtkFieldData::PassData(fd);
  }
}

//----------------------------------------------------------------------------
namespace
{
struct CopyStructuredDataWorker
{
  const int* OutExt;
  const int* InExt;

  CopyStructuredDataWorker(const int* outExt, const int* inExt)
    : OutExt(outExt)
    , InExt(inExt)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* dstArray, Array2T* srcArray)
  {
    // Give the compiler a hand -- allow optimizations that require both arrays
    // to have the same stride.
    VTK_ASSUME(srcArray->GetNumberOfComponents() == dstArray->GetNumberOfComponents());

    // Create some tuple ranges to simplify optimized copying:
    const auto srcTuples = vtk::DataArrayTupleRange(srcArray);
    auto dstTuples = vtk::DataArrayTupleRange(dstArray);

    if (vtkStructuredExtent::Smaller(this->OutExt, this->InExt))
    {
      // get outExt relative to the inExt to keep the logic simple. This assumes
      // that outExt is a subset of the inExt.
      const int relOutExt[6] = {
        this->OutExt[0] - this->InExt[0],
        this->OutExt[1] - this->InExt[0],
        this->OutExt[2] - this->InExt[2],
        this->OutExt[3] - this->InExt[2],
        this->OutExt[4] - this->InExt[4],
        this->OutExt[5] - this->InExt[4],
      };

      const int dims[3] = {
        this->InExt[1] - this->InExt[0] + 1,
        this->InExt[3] - this->InExt[2] + 1,
        this->InExt[5] - this->InExt[4] + 1,
      };

      auto dstTupleIter = dstTuples.begin();
      for (int outz = relOutExt[4]; outz <= relOutExt[5]; ++outz)
      {
        const vtkIdType zfactor = static_cast<vtkIdType>(outz) * dims[1];
        for (int outy = relOutExt[2]; outy <= relOutExt[3]; ++outy)
        {
          const vtkIdType yfactor = (zfactor + outy) * dims[0];
          for (int outx = relOutExt[0]; outx <= relOutExt[1]; ++outx)
          {
            const vtkIdType inTupleIdx = yfactor + outx;
            *dstTupleIter++ = srcTuples[inTupleIdx];
          }
        }
      }
    }
    else
    {
      int writeExt[6];
      memcpy(writeExt, this->OutExt, 6 * sizeof(int));
      vtkStructuredExtent::Clamp(writeExt, this->InExt);

      const vtkIdType inDims[3] = { this->InExt[1] - this->InExt[0] + 1,
        this->InExt[3] - this->InExt[2] + 1, this->InExt[5] - this->InExt[4] + 1 };
      const vtkIdType outDims[3] = { this->OutExt[1] - this->OutExt[0] + 1,
        this->OutExt[3] - this->OutExt[2] + 1, this->OutExt[5] - this->OutExt[4] + 1 };

      for (int idz = writeExt[4]; idz <= writeExt[5]; ++idz)
      {
        const vtkIdType inTupleId1 = (idz - this->InExt[4]) * inDims[0] * inDims[1];
        const vtkIdType outTupleId1 = (idz - this->OutExt[4]) * outDims[0] * outDims[1];
        for (int idy = writeExt[2]; idy <= writeExt[3]; ++idy)
        {
          const vtkIdType inTupleId2 = inTupleId1 + (idy - this->InExt[2]) * inDims[0];
          const vtkIdType outTupleId2 = outTupleId1 + (idy - this->OutExt[2]) * outDims[0];
          for (int idx = writeExt[0]; idx <= writeExt[1]; ++idx)
          {
            const vtkIdType inTupleIdx = inTupleId2 + idx - this->InExt[0];
            const vtkIdType outTupleIdx = outTupleId2 + idx - this->OutExt[0];

            dstTuples[outTupleIdx] = srcTuples[inTupleIdx];
          }
        }
      }
    }

    dstArray->DataChanged();
  }
};

//----------------------------------------------------------------------------
// Handle vtkAbstractArrays that aren't vtkDataArrays.
template <class iterT>
void vtkDataSetAttributesCopyValues(iterT* destIter, const int* outExt, vtkIdType outIncs[3],
  iterT* srcIter, const int* inExt, vtkIdType inIncs[3])
{
  int data_type_size = srcIter->GetArray()->GetDataTypeSize();
  vtkIdType rowLength = outIncs[1];
  unsigned char* inPtr;
  unsigned char* outPtr;
  unsigned char* inZPtr;
  unsigned char* outZPtr;

  // Get the starting input pointer.
  inZPtr = static_cast<unsigned char*>(srcIter->GetArray()->GetVoidPointer(0));
  // Shift to the start of the subextent.
  inZPtr += (outExt[0] - inExt[0]) * inIncs[0] * data_type_size +
    (outExt[2] - inExt[2]) * inIncs[1] * data_type_size +
    (outExt[4] - inExt[4]) * inIncs[2] * data_type_size;

  // Get output pointer.
  outZPtr = static_cast<unsigned char*>(destIter->GetArray()->GetVoidPointer(0));

  // Loop over z axis.
  for (int zIdx = outExt[4]; zIdx <= outExt[5]; ++zIdx)
  {
    inPtr = inZPtr;
    outPtr = outZPtr;
    for (int yIdx = outExt[2]; yIdx <= outExt[3]; ++yIdx)
    {
      memcpy(outPtr, inPtr, rowLength * data_type_size);
      inPtr += inIncs[1] * data_type_size;
      outPtr += outIncs[1] * data_type_size;
    }
    inZPtr += inIncs[2] * data_type_size;
    outZPtr += outIncs[2] * data_type_size;
  }
}

//----------------------------------------------------------------------------
// Specialize for vtkStringArray.
template <>
void vtkDataSetAttributesCopyValues(vtkArrayIteratorTemplate<vtkStdString>* destIter,
  const int* outExt, vtkIdType outIncs[3], vtkArrayIteratorTemplate<vtkStdString>* srcIter,
  const int* inExt, vtkIdType inIncs[3])
{
  vtkIdType inZIndex = (outExt[0] - inExt[0]) * inIncs[0] + (outExt[2] - inExt[2]) * inIncs[1] +
    (outExt[4] - inExt[4]) * inIncs[2];

  vtkIdType outZIndex = 0;
  vtkIdType rowLength = outIncs[1];

  for (int zIdx = outExt[4]; zIdx <= outExt[5]; ++zIdx)
  {
    vtkIdType inIndex = inZIndex;
    vtkIdType outIndex = outZIndex;
    for (int yIdx = outExt[2]; yIdx <= outExt[3]; ++yIdx)
    {
      for (int xIdx = 0; xIdx < rowLength; ++xIdx)
      {
        destIter->GetValue(outIndex + xIdx) = srcIter->GetValue(inIndex + xIdx);
      }
      inIndex += inIncs[1];
      outIndex += outIncs[1];
    }
    inZIndex += inIncs[2];
    outZIndex += outIncs[2];
  }
}

} // end anon namespace

//----------------------------------------------------------------------------
// This is used in the imaging pipeline for copying arrays.
// CopyAllocate needs to be called before this method.
void vtkDataSetAttributes::CopyStructuredData(
  vtkDataSetAttributes* fromPd, const int* inExt, const int* outExt, bool setSize)
{
  int i;

  for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    vtkAbstractArray* inArray = fromPd->Data[i];
    vtkAbstractArray* outArray = this->Data[this->TargetIndices[i]];
    vtkIdType inIncs[3];
    vtkIdType outIncs[3];
    vtkIdType zIdx;

    // Compute increments
    inIncs[0] = inArray->GetNumberOfComponents();
    inIncs[1] = inIncs[0] * (inExt[1] - inExt[0] + 1);
    inIncs[2] = inIncs[1] * (inExt[3] - inExt[2] + 1);
    outIncs[0] = inIncs[0];
    outIncs[1] = outIncs[0] * (outExt[1] - outExt[0] + 1);
    outIncs[2] = outIncs[1] * (outExt[3] - outExt[2] + 1);

    // Make sure the input extents match the actual array lengths.
    zIdx = inIncs[2] / inIncs[0] * (inExt[5] - inExt[4] + 1);
    if (inArray->GetNumberOfTuples() != zIdx)
    {
      vtkErrorMacro("Input extent (" << inExt[0] << ", " << inExt[1] << ", " << inExt[2] << ", "
                                     << inExt[3] << ", " << inExt[4] << ", " << inExt[5]
                                     << ") does not match array length: " << zIdx);
      // Skip copying this array.
      continue;
    }
    // Make sure the output extents match the actual array lengths.
    zIdx = outIncs[2] / outIncs[0] * (outExt[5] - outExt[4] + 1);
    if (outArray->GetNumberOfTuples() != zIdx && setSize)
    {
      // The "CopyAllocate" method only sets the size, not the number of tuples.
      outArray->SetNumberOfTuples(zIdx);
    }

    // We get very little performance improvement from this, but we'll leave the
    // legacy code around until we've done through benchmarking.
    vtkDataArray* inDA = vtkArrayDownCast<vtkDataArray>(inArray);
    vtkDataArray* outDA = vtkArrayDownCast<vtkDataArray>(outArray);
    if (!inDA || !outDA) // String array, etc
    {
      vtkArrayIterator* srcIter = inArray->NewIterator();
      vtkArrayIterator* destIter = outArray->NewIterator();
      switch (inArray->GetDataType())
      {
        vtkArrayIteratorTemplateMacro(vtkDataSetAttributesCopyValues(static_cast<VTK_TT*>(destIter),
          outExt, outIncs, static_cast<VTK_TT*>(srcIter), inExt, inIncs));
      }
      srcIter->Delete();
      destIter->Delete();
    }
    else
    {
      CopyStructuredDataWorker worker(outExt, inExt);
      if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(outDA, inDA, worker))
      {
        // Fallback to vtkDataArray API (e.g. vtkBitArray):
        worker(outDA, inDA);
      }
    }
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetupForCopy(vtkDataSetAttributes* pd)
{
  this->InternalCopyAllocate(pd, COPYTUPLE, 0, 0, false, false);
}

//--------------------------------------------------------------------------
// Allocates point data for point-by-point (or cell-by-cell) copy operation.
// If sze=0, then use the input DataSetAttributes to create (i.e., find
// initial size of) new objects; otherwise use the sze variable.
void vtkDataSetAttributes::InternalCopyAllocate(vtkDataSetAttributes* pd, int ctype, vtkIdType sze,
  vtkIdType ext, int shallowCopyArrays, bool createNewArrays)
{
  vtkAbstractArray* newAA;
  int i;

  // Create various point data depending upon input
  //
  if (!pd)
  {
    return;
  }

  if ((ctype < COPYTUPLE) || (ctype > PASSDATA))
  {
    return;
  }

  this->RequiredArrays = this->ComputeRequiredArrays(pd, ctype);
  if (this->RequiredArrays.GetListSize() == 0)
  {
    return;
  }
  delete[] this->TargetIndices;
  this->TargetIndices = new int[pd->GetNumberOfArrays()];
  for (i = 0; i < pd->GetNumberOfArrays(); i++)
  {
    this->TargetIndices[i] = -1;
  }

  vtkAbstractArray* aa = nullptr;
  // If we are not copying on self
  if ((pd != this) && createNewArrays)
  {
    int attributeType;

    for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
         i = this->RequiredArrays.NextIndex())
    {
      // Create all required arrays
      aa = pd->GetAbstractArray(i);
      if (shallowCopyArrays)
      {
        newAA = aa;
      }
      else
      {
        newAA = aa->NewInstance();
        newAA->SetNumberOfComponents(aa->GetNumberOfComponents());
        newAA->CopyComponentNames(aa);
        newAA->SetName(aa->GetName());
        if (aa->HasInformation())
        {
          newAA->CopyInformation(aa->GetInformation(), /*deep=*/1);
        }
        if (sze > 0)
        {
          newAA->Allocate(sze * aa->GetNumberOfComponents(), ext);
        }
        else
        {
          newAA->Allocate(aa->GetNumberOfTuples());
        }
        vtkDataArray* newDA = vtkArrayDownCast<vtkDataArray>(newAA);
        if (newDA)
        {
          vtkDataArray* da = vtkArrayDownCast<vtkDataArray>(aa);
          newDA->SetLookupTable(da->GetLookupTable());
        }
      }
      this->TargetIndices[i] = this->AddArray(newAA);
      // If necessary, make the array an attribute
      if (((attributeType = pd->IsArrayAnAttribute(i)) != -1) &&
        this->CopyAttributeFlags[ctype][attributeType])
      {
        this->CopyAttributeFlags[ctype][attributeType] =
          pd->CopyAttributeFlags[ctype][attributeType];
        this->SetActiveAttribute(this->TargetIndices[i], attributeType);
      }
      if (!shallowCopyArrays)
      {
        newAA->Delete();
      }
    }
  }
  else if (pd == this)
  {
    // If copying on self, resize the arrays and initialize
    // TargetIndices
    for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
         i = this->RequiredArrays.NextIndex())
    {
      aa = pd->GetAbstractArray(i);
      aa->Resize(sze);
      this->TargetIndices[i] = i;
    }
  }
  else
  {
    // All we are asked to do is create a mapping.
    // Here we assume that arrays are the same and ordered
    // the same way.
    for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
         i = this->RequiredArrays.NextIndex())
    {
      this->TargetIndices[i] = i;
    }
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::RemoveArray(int index)
{
  if ((index < 0) || (index >= this->NumberOfActiveArrays))
  {
    return;
  }
  this->Superclass::RemoveArray(index);
  int attributeType;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    if (this->AttributeIndices[attributeType] == index)
    {
      this->AttributeIndices[attributeType] = -1;
    }
    else if (this->AttributeIndices[attributeType] > index)
    {
      this->AttributeIndices[attributeType]--;
    }
  }
}

//--------------------------------------------------------------------------
// Copy the attribute data from one id to another. Make sure CopyAllocate() has
// been invoked before using this method.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes* fromPd, vtkIdType fromId, vtkIdType toId)
{
  int i;
  for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    this->CopyTuple(fromPd->Data[i], this->Data[this->TargetIndices[i]], fromId, toId);
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyData(
  vtkDataSetAttributes* fromPd, vtkIdList* fromIds, vtkIdList* toIds)
{
  int i;
  for (i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    this->CopyTuples(fromPd->Data[i], this->Data[this->TargetIndices[i]], fromIds, toIds);
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyData(
  vtkDataSetAttributes* fromPd, vtkIdType dstStart, vtkIdType n, vtkIdType srcStart)
{
  for (int i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    this->CopyTuples(fromPd->Data[i], this->Data[this->TargetIndices[i]], dstStart, n, srcStart);
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyAllocate(
  vtkDataSetAttributes* pd, vtkIdType sze, vtkIdType ext, int shallowCopyArrays)
{
  this->InternalCopyAllocate(pd, COPYTUPLE, sze, ext, shallowCopyArrays);
}

// Initialize point interpolation method.
void vtkDataSetAttributes::InterpolateAllocate(
  vtkDataSetAttributes* pd, vtkIdType sze, vtkIdType ext, int shallowCopyArrays)
{
  this->InternalCopyAllocate(pd, INTERPOLATE, sze, ext, shallowCopyArrays);
}

//--------------------------------------------------------------------------
// Interpolate data from points and interpolation weights. Make sure that the
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(
  vtkDataSetAttributes* fromPd, vtkIdType toId, vtkIdList* ptIds, double* weights)
{
  for (int i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    vtkAbstractArray* fromArray = fromPd->Data[i];
    vtkAbstractArray* toArray = this->Data[this->TargetIndices[i]];

    // check if the destination array needs nearest neighbor interpolation
    int attributeIndex = this->IsArrayAnAttribute(this->TargetIndices[i]);
    if (attributeIndex != -1 && this->CopyAttributeFlags[INTERPOLATE][attributeIndex] == 2)
    {
      vtkIdType numIds = ptIds->GetNumberOfIds();
      vtkIdType maxId = ptIds->GetId(0);
      vtkIdType maxWeight = 0.;
      for (int j = 0; j < numIds; j++)
      {
        if (weights[j] > maxWeight)
        {
          maxWeight = weights[j];
          maxId = ptIds->GetId(j);
        }
      }
      toArray->InsertTuple(toId, maxId, fromArray);
    }
    else
    {
      toArray->InterpolateTuple(toId, ptIds, fromArray, weights);
    }
  }
}

//--------------------------------------------------------------------------
// Interpolate data from the two points p1,p2 (forming an edge) and an
// interpolation factor, t, along the edge. The weight ranges from (0,1),
// with t=0 located at p1. Make sure that the method InterpolateAllocate()
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateEdge(
  vtkDataSetAttributes* fromPd, vtkIdType toId, vtkIdType p1, vtkIdType p2, double t)
{
  for (int i = this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
       i = this->RequiredArrays.NextIndex())
  {
    vtkAbstractArray* fromArray = fromPd->Data[i];
    vtkAbstractArray* toArray = this->Data[this->TargetIndices[i]];

    // check if the destination array needs nearest neighbor interpolation
    int attributeIndex = this->IsArrayAnAttribute(this->TargetIndices[i]);
    if (attributeIndex != -1 && this->CopyAttributeFlags[INTERPOLATE][attributeIndex] == 2)
    {
      if (t < .5)
      {
        toArray->InsertTuple(toId, p1, fromArray);
      }
      else
      {
        toArray->InsertTuple(toId, p2, fromArray);
      }
    }
    else
    {
      toArray->InterpolateTuple(toId, p1, fromArray, p2, fromArray, t);
    }
  }
}

//--------------------------------------------------------------------------
// Interpolate data from the two points p1,p2 (forming an edge) and an
// interpolation factor, t, along the edge. The weight ranges from (0,1),
// with t=0 located at p1. Make sure that the method InterpolateAllocate()
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateTime(
  vtkDataSetAttributes* from1, vtkDataSetAttributes* from2, vtkIdType id, double t)
{
  for (int attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    // If this attribute is to be copied
    if (this->CopyAttributeFlags[INTERPOLATE][attributeType])
    {
      if (from1->GetAttribute(attributeType) && from2->GetAttribute(attributeType))
      {
        vtkAbstractArray* toArray = this->GetAttribute(attributeType);
        // check if the destination array needs nearest neighbor interpolation
        if (this->CopyAttributeFlags[INTERPOLATE][attributeType] == 2)
        {
          if (t < .5)
          {
            toArray->InsertTuple(id, id, from1->GetAttribute(attributeType));
          }
          else
          {
            toArray->InsertTuple(id, id, from2->GetAttribute(attributeType));
          }
        }
        else
        {
          toArray->InterpolateTuple(
            id, id, from1->GetAttribute(attributeType), id, from2->GetAttribute(attributeType), t);
        }
      }
    }
  }
}

//--------------------------------------------------------------------------
// Copy a tuple of data from one data array to another. This method (and
// following ones) assume that the fromData and toData objects are of the
// same type, and have the same number of components. This is true if you
// invoke CopyAllocate() or InterpolateAllocate().
void vtkDataSetAttributes::CopyTuple(
  vtkAbstractArray* fromData, vtkAbstractArray* toData, vtkIdType fromId, vtkIdType toId)
{
  toData->InsertTuple(toId, fromId, fromData);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyTuples(
  vtkAbstractArray* fromData, vtkAbstractArray* toData, vtkIdList* fromIds, vtkIdList* toIds)
{
  toData->InsertTuples(toIds, fromIds, fromData);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyTuples(vtkAbstractArray* fromData, vtkAbstractArray* toData,
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart)
{
  toData->InsertTuples(dstStart, n, srcStart, fromData);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetScalars(vtkDataArray* da)
{
  return this->SetAttribute(da, SCALARS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveScalars(const char* name)
{
  return this->SetActiveAttribute(name, SCALARS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveAttribute(const char* name, int attributeType)
{
  int index;
  this->GetAbstractArray(name, index);
  return this->SetActiveAttribute(index, attributeType);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetScalars()
{
  return this->GetAttribute(SCALARS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetVectors(vtkDataArray* da)
{
  return this->SetAttribute(da, VECTORS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveVectors(const char* name)
{
  return this->SetActiveAttribute(name, VECTORS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetVectors()
{
  return this->GetAttribute(VECTORS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetNormals(vtkDataArray* da)
{
  return this->SetAttribute(da, NORMALS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveNormals(const char* name)
{
  return this->SetActiveAttribute(name, NORMALS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetNormals()
{
  return this->GetAttribute(NORMALS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetTangents(vtkDataArray* da)
{
  return this->SetAttribute(da, TANGENTS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveTangents(const char* name)
{
  return this->SetActiveAttribute(name, TANGENTS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTangents()
{
  return this->GetAttribute(TANGENTS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetTCoords(vtkDataArray* da)
{
  return this->SetAttribute(da, TCOORDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveTCoords(const char* name)
{
  return this->SetActiveAttribute(name, TCOORDS);
}
//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTCoords()
{
  return this->GetAttribute(TCOORDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetTensors(vtkDataArray* da)
{
  return this->SetAttribute(da, TENSORS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveTensors(const char* name)
{
  return this->SetActiveAttribute(name, TENSORS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTensors()
{
  return this->GetAttribute(TENSORS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetGlobalIds(vtkDataArray* da)
{
  return this->SetAttribute(da, GLOBALIDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveGlobalIds(const char* name)
{
  return this->SetActiveAttribute(name, GLOBALIDS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetGlobalIds()
{
  return this->GetAttribute(GLOBALIDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetPedigreeIds(vtkAbstractArray* aa)
{
  return this->SetAttribute(aa, PEDIGREEIDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActivePedigreeIds(const char* name)
{
  return this->SetActiveAttribute(name, PEDIGREEIDS);
}

//--------------------------------------------------------------------------
vtkAbstractArray* vtkDataSetAttributes::GetPedigreeIds()
{
  return this->GetAbstractAttribute(PEDIGREEIDS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetRationalWeights(vtkDataArray* da)
{
  return this->SetAttribute(da, RATIONALWEIGHTS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveRationalWeights(const char* name)
{
  return this->SetActiveAttribute(name, RATIONALWEIGHTS);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetRationalWeights()
{
  return this->GetAttribute(RATIONALWEIGHTS);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetHigherOrderDegrees(vtkDataArray* da)
{
  return this->SetAttribute(da, HIGHERORDERDEGREES);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveHigherOrderDegrees(const char* name)
{
  return this->SetActiveAttribute(name, HIGHERORDERDEGREES);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetHigherOrderDegrees()
{
  return this->GetAttribute(HIGHERORDERDEGREES);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetScalars(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetScalars();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetVectors(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetVectors();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetNormals(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetNormals();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTangents(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetTangents();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTCoords(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetTCoords();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTensors(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetTensors();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetGlobalIds(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetGlobalIds();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkAbstractArray* vtkDataSetAttributes::GetPedigreeIds(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetPedigreeIds();
  }
  return this->GetAbstractArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetRationalWeights(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetRationalWeights();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetHigherOrderDegrees(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    return this->GetHigherOrderDegrees();
  }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveAttribute(int index, int attributeType)
{
  if ((index >= 0) && (index < this->GetNumberOfArrays()))
  {
    if (attributeType != PEDIGREEIDS)
    {
      vtkDataArray* darray = vtkArrayDownCast<vtkDataArray>(this->Data[index]);
      if (!darray)
      {
        vtkWarningMacro("Can not set attribute "
          << vtkDataSetAttributes::AttributeNames[attributeType]
          << ". Only vtkDataArray subclasses can be set as active attributes.");
        return -1;
      }
      if (!this->CheckNumberOfComponents(darray, attributeType))
      {
        vtkWarningMacro("Can not set attribute "
          << vtkDataSetAttributes::AttributeNames[attributeType]
          << ". Incorrect number of components.");
        return -1;
      }
    }

    this->AttributeIndices[attributeType] = index;
    this->Modified();
    return index;
  }
  else if (index == -1)
  {
    this->AttributeIndices[attributeType] = index;
    this->Modified();
  }

  return -1;
}

//--------------------------------------------------------------------------
const int
  vtkDataSetAttributes ::NumberOfAttributeComponents[vtkDataSetAttributes::NUM_ATTRIBUTES] = { 0, 3,
    3, 3, 9, 1, 1, 1, 3, 1, 3 };

//--------------------------------------------------------------------------
// Scalars set to NOLIMIT
const int vtkDataSetAttributes ::AttributeLimits[vtkDataSetAttributes::NUM_ATTRIBUTES] = { NOLIMIT,
  EXACT, EXACT, MAX, EXACT, EXACT, EXACT, EXACT, EXACT, EXACT, EXACT };

//--------------------------------------------------------------------------
int vtkDataSetAttributes::CheckNumberOfComponents(vtkAbstractArray* aa, int attributeType)
{
  int numComp = aa->GetNumberOfComponents();

  if (vtkDataSetAttributes::AttributeLimits[attributeType] == MAX)
  {
    if (numComp > vtkDataSetAttributes::NumberOfAttributeComponents[attributeType])
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
  else if (vtkDataSetAttributes::AttributeLimits[attributeType] == EXACT)
  {
    if (numComp == vtkDataSetAttributes::NumberOfAttributeComponents[attributeType] ||
      (numComp == 6 && attributeType == TENSORS)) // TENSORS6 support
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else if (vtkDataSetAttributes::AttributeLimits[attributeType] == NOLIMIT)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetAttribute(int attributeType)
{
  int index = this->AttributeIndices[attributeType];
  if (index == -1)
  {
    return nullptr;
  }
  else
  {
    return vtkArrayDownCast<vtkDataArray>(this->Data[index]);
  }
}

//--------------------------------------------------------------------------
vtkAbstractArray* vtkDataSetAttributes::GetAbstractAttribute(int attributeType)
{
  int index = this->AttributeIndices[attributeType];
  if (index == -1)
  {
    return nullptr;
  }
  else
  {
    return this->Data[index];
  }
}

//--------------------------------------------------------------------------
// This method lets the user add an array and make it the current
// scalars, vectors etc... (this is determined by the attribute type
// which is an enum defined vtkDataSetAttributes)
int vtkDataSetAttributes::SetAttribute(vtkAbstractArray* aa, int attributeType)
{
  if (aa && attributeType != PEDIGREEIDS && !vtkArrayDownCast<vtkDataArray>(aa))
  {
    vtkWarningMacro("Can not set attribute "
      << vtkDataSetAttributes::AttributeNames[attributeType]
      << ". This attribute must be a subclass of vtkDataArray.");
    return -1;
  }
  if (aa && !this->CheckNumberOfComponents(aa, attributeType))
  {
    vtkWarningMacro("Can not set attribute " << vtkDataSetAttributes::AttributeNames[attributeType]
                                             << ". Incorrect number of components.");
    return -1;
  }

  int currentAttribute = this->AttributeIndices[attributeType];

  // If there is an existing attribute, replace it
  if ((currentAttribute >= 0) && (currentAttribute < this->GetNumberOfArrays()))
  {
    if (this->GetAbstractArray(currentAttribute) == aa)
    {
      return currentAttribute;
    }
    this->RemoveArray(currentAttribute);
  }

  if (aa)
  {
    // Add the array
    currentAttribute = this->AddArray(aa);
    this->AttributeIndices[attributeType] = currentAttribute;
  }
  else
  {
    this->AttributeIndices[attributeType] = -1; // attribute of this type doesn't exist
  }
  this->Modified();
  return this->AttributeIndices[attributeType];
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // Print the copy flags
  int i;
  os << indent << "Copy Tuple Flags: ( ";
  for (i = 0; i < NUM_ATTRIBUTES; i++)
  {
    os << this->CopyAttributeFlags[COPYTUPLE][i] << " ";
  }
  os << ")" << endl;
  os << indent << "Interpolate Flags: ( ";
  for (i = 0; i < NUM_ATTRIBUTES; i++)
  {
    os << this->CopyAttributeFlags[INTERPOLATE][i] << " ";
  }
  os << ")" << endl;
  os << indent << "Pass Through Flags: ( ";
  for (i = 0; i < NUM_ATTRIBUTES; i++)
  {
    os << this->CopyAttributeFlags[PASSDATA][i] << " ";
  }
  os << ")" << endl;

  // Now print the various attributes
  vtkAbstractArray* aa;
  int attributeType;
  for (attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
  {
    os << indent << vtkDataSetAttributes::AttributeNames[attributeType] << ": ";
    if ((aa = this->GetAbstractAttribute(attributeType)))
    {
      os << endl;
      aa->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << "(none)" << endl;
    }
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::GetAttributeIndices(int* indexArray)
{
  int i;
  for (i = 0; i < NUM_ATTRIBUTES; i++)
  {
    indexArray[i] = this->AttributeIndices[i];
  }
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::IsArrayAnAttribute(int idx)
{
  int i;
  for (i = 0; i < NUM_ATTRIBUTES; i++)
  {
    if (idx == this->AttributeIndices[i])
    {
      return i;
    }
  }
  return -1;
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyAttribute(int index, int value, int ctype)
{
  if (index < 0 || ctype < 0 || index >= vtkDataSetAttributes::NUM_ATTRIBUTES ||
    ctype > vtkDataSetAttributes::ALLCOPY)
  {
    vtkErrorMacro("Cannot set copy attribute for attribute type "
      << index << " and copy operation " << ctype << ". These values are out of range.");
    return;
  }

  if (ctype == vtkDataSetAttributes::ALLCOPY)
  {
    int t;
    for (t = COPYTUPLE; t < vtkDataSetAttributes::ALLCOPY; t++)
    {
      if (this->CopyAttributeFlags[t][index] != value)
      {
        this->CopyAttributeFlags[t][index] = value;
        this->Modified();
      }
    }
  }
  else
  {
    if (this->CopyAttributeFlags[ctype][index] != value)
    {
      this->CopyAttributeFlags[ctype][index] = value;
      this->Modified();
    }
  }
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyAttribute(int index, int ctype)
{
  if (index < 0 || ctype < 0 || index >= vtkDataSetAttributes::NUM_ATTRIBUTES ||
    ctype > vtkDataSetAttributes::ALLCOPY)
  {
    vtkWarningMacro("Cannot get copy attribute for attribute type "
      << index << " and copy operation " << ctype << ". These values are out of range.");
    return -1;
  }
  else if (ctype == vtkDataSetAttributes::ALLCOPY)
  {
    return (this->CopyAttributeFlags[COPYTUPLE][index] &&
      this->CopyAttributeFlags[INTERPOLATE][index] && this->CopyAttributeFlags[PASSDATA][index]);
  }
  else
  {
    return this->CopyAttributeFlags[ctype][index];
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyScalars(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(SCALARS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyScalars(int ctype)
{
  return this->GetCopyAttribute(SCALARS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyVectors(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(VECTORS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyVectors(int ctype)
{
  return this->GetCopyAttribute(VECTORS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyNormals(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(NORMALS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyNormals(int ctype)
{
  return this->GetCopyAttribute(NORMALS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyTangents(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(TANGENTS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyTangents(int ctype)
{
  return this->GetCopyAttribute(TANGENTS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyTCoords(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(TCOORDS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyTCoords(int ctype)
{
  return this->GetCopyAttribute(TCOORDS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyTensors(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(TENSORS, i, ctype);
}
//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyTensors(int ctype)
{
  return this->GetCopyAttribute(TENSORS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyGlobalIds(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(GLOBALIDS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyGlobalIds(int ctype)
{
  return this->GetCopyAttribute(GLOBALIDS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyPedigreeIds(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(PEDIGREEIDS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyPedigreeIds(int ctype)
{
  return this->GetCopyAttribute(PEDIGREEIDS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyRationalWeights(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(RATIONALWEIGHTS, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyRationalWeights(int ctype)
{
  return this->GetCopyAttribute(RATIONALWEIGHTS, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyHigherOrderDegrees(vtkTypeBool i, int ctype)
{
  this->SetCopyAttribute(HIGHERORDERDEGREES, i, ctype);
}

//--------------------------------------------------------------------------
vtkTypeBool vtkDataSetAttributes::GetCopyHigherOrderDegrees(int ctype)
{
  return this->GetCopyAttribute(HIGHERORDERDEGREES, ctype);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyAllocate(
  vtkDataSetAttributes::FieldList& list, vtkIdType sze, vtkIdType ext)
{
  list.CopyAllocate(this, COPYTUPLE, sze, ext);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::InterpolateAllocate(
  vtkDataSetAttributes::FieldList& list, vtkIdType sze, vtkIdType ext)
{
  list.CopyAllocate(this, INTERPOLATE, sze, ext);
}

//--------------------------------------------------------------------------
// Description:
// A special form of CopyData() to be used with FieldLists. Use it when you are
// copying data from a set of vtkDataSetAttributes. Make sure that you have
// called the special form of CopyAllocate that accepts FieldLists.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes::FieldList& list,
  vtkDataSetAttributes* fromDSA, int idx, vtkIdType fromId, vtkIdType toId)
{
  list.CopyData(idx, fromDSA, fromId, this, toId);
}

//--------------------------------------------------------------------------
// Description:
// A special form of CopyData() to be used with FieldLists. Use it when you are
// copying data from a set of vtkDataSetAttributes. Make sure that you have
// called the special form of CopyAllocate that accepts FieldLists.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes::FieldList& list,
  vtkDataSetAttributes* fromDSA, int idx, vtkIdType dstStart, vtkIdType n, vtkIdType srcStart)
{
  list.CopyData(idx, fromDSA, srcStart, n, this, dstStart);
}

//--------------------------------------------------------------------------
// Interpolate data from points and interpolation weights. Make sure that the
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(vtkDataSetAttributes::FieldList& list,
  vtkDataSetAttributes* fromPd, int idx, vtkIdType toId, vtkIdList* ptIds, double* weights)
{
  list.InterpolatePoint(idx, fromPd, ptIds, weights, this, toId);
}

//--------------------------------------------------------------------------
const char* vtkDataSetAttributes::GetAttributeTypeAsString(int attributeType)
{
  if (attributeType < 0 || attributeType >= NUM_ATTRIBUTES)
  {
    vtkGenericWarningMacro("Bad attribute type: " << attributeType << ".");
    return nullptr;
  }
  return vtkDataSetAttributes::AttributeNames[attributeType];
}

//--------------------------------------------------------------------------
const char* vtkDataSetAttributes::GetLongAttributeTypeAsString(int attributeType)
{
  if (attributeType < 0 || attributeType >= NUM_ATTRIBUTES)
  {
    vtkGenericWarningMacro("Bad attribute type: " << attributeType << ".");
    return nullptr;
  }
  return vtkDataSetAttributes::LongAttributeNames[attributeType];
}
