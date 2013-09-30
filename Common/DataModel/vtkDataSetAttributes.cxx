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

#include "vtkArrayIteratorIncludes.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"

#include <vector>

namespace
{
  // pair.first it used to indicate if pair.second is valid.
  typedef  std::vector<std::pair<bool, vtkStdString> > vtkInternalComponentNameBase;
}

class vtkDataSetAttributes::vtkInternalComponentNames : public vtkInternalComponentNameBase {};

vtkStandardNewMacro(vtkDataSetAttributes);

//--------------------------------------------------------------------------
const char vtkDataSetAttributes
::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][12] =
{ "Scalars",
  "Vectors",
  "Normals",
  "TCoords",
  "Tensors",
  "GlobalIds",
  "PedigreeIds",
  "EdgeFlag"
};

const char vtkDataSetAttributes
::LongAttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][35] =
{ "vtkDataSetAttributes::SCALARS",
  "vtkDataSetAttributes::VECTORS",
  "vtkDataSetAttributes::NORMALS",
  "vtkDataSetAttributes::TCOORDS",
  "vtkDataSetAttributes::TENSORS",
  "vtkDataSetAttributes::GLOBALIDS",
  "vtkDataSetAttributes::PEDIGREEIDS",
  "vtkDataSetAttributes::EDGEFLAG"
};

//--------------------------------------------------------------------------
// Construct object with copying turned on for all data.
vtkDataSetAttributes::vtkDataSetAttributes()
{
  int attributeType;
  for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    this->AttributeIndices[attributeType] = -1;
    this->CopyAttributeFlags[COPYTUPLE][attributeType] = 1;
    this->CopyAttributeFlags[INTERPOLATE][attributeType] = 1;
    this->CopyAttributeFlags[PASSDATA][attributeType] = 1;
    }

  //Global IDs should not be interpolated because they are labels, not "numbers"
  //Global IDs should not be copied either, unless doing so preserves meaning.
  //Passing through is ussually OK because it is 1:1.
  this->CopyAttributeFlags[COPYTUPLE][GLOBALIDS] = 0;
  this->CopyAttributeFlags[INTERPOLATE][GLOBALIDS] = 0;

  //Pedigree IDs should not be interpolated because they are labels, not "numbers"
  //Pedigree IDs may be copied since they do not require 1:1 mapping.
  this->CopyAttributeFlags[INTERPOLATE][PEDIGREEIDS] = 0;

  this->TargetIndices=0;

}

//--------------------------------------------------------------------------
// Destructor for the vtkDataSetAttributes objects.
vtkDataSetAttributes::~vtkDataSetAttributes()
{
  this->Initialize();
  delete[] this->TargetIndices;
  this->TargetIndices = 0;
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
}

//--------------------------------------------------------------------------
// Deep copy of data (i.e., create new data arrays and
// copy from input data). Note that attribute data is
// not copied.
void vtkDataSetAttributes::DeepCopy(vtkFieldData *fd)
{
  this->Initialize(); //free up memory

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
  // If the source is a vtkDataSetAttributes
  if (dsa)
    {
    int numArrays = fd->GetNumberOfArrays();
    int attributeType, i;
    vtkAbstractArray *data, *newData;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    for (i=0; i < numArrays; i++ )
      {
      data = fd->GetAbstractArray(i);
      newData = data->NewInstance(); //instantiate same type of object
      newData->DeepCopy(data);
      newData->SetName(data->GetName());
      this->AddArray(newData);
      newData->Delete();
      }
    // Copy the copy flags
    for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
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
void vtkDataSetAttributes::ShallowCopy(vtkFieldData *fd)
{
  this->Initialize(); //free up memory

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
  // If the source is a vtkDataSetAttributes
  if (dsa)
    {
    int numArrays = fd->GetNumberOfArrays();
    int attributeType, i;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    this->NumberOfActiveArrays = 0;
    for (i=0; i < numArrays; i++ )
      {
      this->NumberOfActiveArrays++;
      this->SetArray(i, fd->GetAbstractArray(i));
      }

    // Copy the copy flags
    for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
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
// Initialize all of the object's data to NULL
void vtkDataSetAttributes::InitializeFields()
{
  this->vtkFieldData::InitializeFields();

  int attributeType;
  for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
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
// Initialize all of the object's data to NULL
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
  for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
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
vtkFieldData::BasicIterator  vtkDataSetAttributes::ComputeRequiredArrays(
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
  // pointer is non-NULL). Also, we keep those indices in a list.
  int* copyFlags = new int[pd->GetNumberOfArrays()];
  int index, i, numArrays = 0;
  for(i=0; i<pd->GetNumberOfArrays(); i++)
    {
    const char* arrayName = pd->GetArrayName(i);
    // If there is no blocker for the given array
    // and both CopyAllOff and CopyOn for that array are not true
    if ( (this->GetFlag(arrayName) != 0) &&
         !(this->DoCopyAllOff && (this->GetFlag(arrayName) != 1)) &&
         pd->GetAbstractArray(i))
      {
      // Cannot interpolate idtype arrays
      if (ctype != INTERPOLATE ||
          !pd->GetAbstractArray(i)->IsA("vtkIdTypeArray"))
        {
        copyFlags[numArrays] = i;
        numArrays++;
        }
      }
    }

  // Next, we check the arrays to be copied because they are one of
  // the _attributes_ to be copied (and the data array in non-NULL).
  // We make sure that we don't count anything twice.
  int alreadyCopied;
  int attributeType, j;
  for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
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
        for(i=0; i<numArrays; i++)
          {
          if ( index == copyFlags[i] )
            {
            alreadyCopied = 1;
            }
          }
        // If not, increment the number of arrays to be copied.
        if (!alreadyCopied)
          {
          // Cannot interpolate idtype arrays
          if (ctype != INTERPOLATE ||
              !pd->GetArray(index)->IsA("vtkIdTypeArray"))
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
      for(i=0; i<numArrays; i++)
        {
        if ( index == copyFlags[i] )
          {
          for(j=i; j<numArrays-1; j++)
            {
            copyFlags[j] = copyFlags[j+1];
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
    // Note that NULL data arrays are not copied

    vtkFieldData::BasicIterator it = this->ComputeRequiredArrays(dsa, PASSDATA);

    if ( it.GetListSize() > this->NumberOfArrays )
      {
      this->AllocateArrays(it.GetListSize());
      }
    if (it.GetListSize() == 0)
      {
      return;
      }

    // Since we are replacing, remove old attributes
    int attributeType; //will change//
    for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
      {
      if (this->CopyAttributeFlags[PASSDATA][attributeType])
        {
        this->RemoveArray(this->AttributeIndices[attributeType]);
        this->AttributeIndices[attributeType] = -1;
        }
      }

    int i, arrayIndex;
    for(i=it.BeginIndex(); !it.End(); i=it.NextIndex())
      {
      arrayIndex = this->AddArray(dsa->GetAbstractArray(i));
      // If necessary, make the array an attribute
      if ( ((attributeType = dsa->IsArrayAnAttribute(i)) != -1 ) &&
           this->CopyAttributeFlags[PASSDATA][attributeType] )
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
template <class iterT>
void vtkDataSetAttributesCopyValues(
  iterT* destIter, const int* outExt, vtkIdType outIncs[3],
  iterT* srcIter, const int* inExt, vtkIdType inIncs[3])
{
  // For vtkDataArray subclasses.
  int data_type_size = srcIter->GetArray()->GetDataTypeSize();
  vtkIdType rowLength = outIncs[1];
  unsigned char *inPtr;
  unsigned char *outPtr;
  unsigned char *inZPtr;
  unsigned char *outZPtr;

  // Get the starting input pointer.
  inZPtr = static_cast<unsigned char*>(srcIter->GetArray()->GetVoidPointer(0));
  // Shift to the start of the subextent.
  inZPtr += (outExt[0]-inExt[0])*inIncs[0] * data_type_size +
    (outExt[2] - inExt[2])*inIncs[1] * data_type_size +
    (outExt[4] - inExt[4])*inIncs[2] * data_type_size;

  // Get output pointer.
  outZPtr =
    static_cast<unsigned char*>(destIter->GetArray()->GetVoidPointer(0));

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
VTK_TEMPLATE_SPECIALIZE
void vtkDataSetAttributesCopyValues(
  vtkArrayIteratorTemplate<vtkStdString>* destIter, const int* outExt,
  vtkIdType outIncs[3],
  vtkArrayIteratorTemplate<vtkStdString>* srcIter,
  const int* inExt, vtkIdType inIncs[3])
{
  vtkIdType inZIndex = (outExt[0] - inExt[0])*inIncs[0] +
    (outExt[2] - inExt[2])*inIncs[1] +
    (outExt[4] - inExt[4])*inIncs[2] ;

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

//----------------------------------------------------------------------------
// This is used in the imaging pipeline for copying arrays.
// CopyAllocate needs to be called before this method.
void vtkDataSetAttributes::CopyStructuredData(vtkDataSetAttributes *fromPd,
                                          const int *inExt, const int *outExt)
{
  int i;

  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
      i=this->RequiredArrays.NextIndex())
    {
    vtkAbstractArray *inArray = fromPd->Data[i];
    vtkAbstractArray *outArray = this->Data[this->TargetIndices[i]];
    vtkIdType inIncs[3];
    vtkIdType outIncs[3];
    vtkIdType zIdx;

    // Compute increments
    inIncs[0] = inArray->GetNumberOfComponents();
    inIncs[1] = inIncs[0] * (inExt[1]-inExt[0]+1);
    inIncs[2] = inIncs[1] * (inExt[3]-inExt[2]+1);
    outIncs[0] = inIncs[0];
    outIncs[1] = outIncs[0] * (outExt[1]-outExt[0]+1);
    outIncs[2] = outIncs[1] * (outExt[3]-outExt[2]+1);

    // Make sure the input extents match the actual array lengths.
    zIdx = inIncs[2]/inIncs[0]*(inExt[5]-inExt[4]+1);
    if (inArray->GetNumberOfTuples() != zIdx)
      {
      vtkErrorMacro("Input extent (" << inExt[0] << ", " << inExt[1] << ", "
                    << inExt[2] << ", " << inExt[3] << ", " << inExt[4] << ", "
                    << inExt[5] << ") does not match array length: " << zIdx);
      // Skip copying this array.
      continue;
      }
    // Make sure the output extents match the actual array lengths.
    zIdx = outIncs[2]/outIncs[0]*(outExt[5]-outExt[4]+1);
    if (outArray->GetNumberOfTuples() != zIdx)
      {
      // The "CopyAllocate" method only sets the size, not the number of tuples.
      outArray->SetNumberOfTuples(zIdx);
      }

    vtkArrayIterator* srcIter = inArray->NewIterator();
    vtkArrayIterator* destIter = outArray->NewIterator();

    switch (inArray->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        vtkDataSetAttributesCopyValues(
          static_cast<VTK_TT*>(destIter), outExt, outIncs,
          static_cast<VTK_TT*>(srcIter), inExt, inIncs));
      }
    srcIter->Delete();
    destIter->Delete();
    }
}

//--------------------------------------------------------------------------
// Allocates point data for point-by-point (or cell-by-cell) copy operation.
// If sze=0, then use the input DataSetAttributes to create (i.e., find
// initial size of) new objects; otherwise use the sze variable.
void vtkDataSetAttributes::InternalCopyAllocate(vtkDataSetAttributes* pd,
                                                int ctype,
                                                vtkIdType sze, vtkIdType ext,
                                                int shallowCopyArrays)
{
  vtkAbstractArray* newAA;
  int i;

  // Create various point data depending upon input
  //
  if ( !pd )
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
  for(i=0; i<pd->GetNumberOfArrays(); i++)
    {
    this->TargetIndices[i] = -1;
    }

  vtkAbstractArray* aa=0;
  // If we are not copying on self
  if ( pd != this )
    {
    int attributeType;

    for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
        i=this->RequiredArrays.NextIndex())
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
        newAA->CopyComponentNames( aa );
        newAA->SetName(aa->GetName());
        if (aa->HasInformation())
          {
          newAA->CopyInformation(aa->GetInformation(),/*deep=*/1);
          }
        if ( sze > 0 )
          {
          newAA->Allocate(sze*aa->GetNumberOfComponents(),ext);
          }
        else
          {
          newAA->Allocate(aa->GetNumberOfTuples());
          }
        vtkDataArray* newDA = vtkDataArray::SafeDownCast(newAA);
        if (newDA)
          {
          vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
          newDA->SetLookupTable(da->GetLookupTable());
          }
        }
      this->TargetIndices[i] = this->AddArray(newAA);
      // If necessary, make the array an attribute
      if ( ((attributeType = pd->IsArrayAnAttribute(i)) != -1 ) &&
           this->CopyAttributeFlags[ctype][attributeType] )
        {
        this->SetActiveAttribute(this->TargetIndices[i], attributeType);
        }
      if (!shallowCopyArrays)
        {
        newAA->Delete();
        }
      }
    }
  else
    {
    // If copying on self, resize the arrays and initialize
    // TargetIndices
    for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
        i=this->RequiredArrays.NextIndex())
      {
      aa = pd->GetAbstractArray(i);
      aa->Resize(sze);
      this->TargetIndices[i] = i;
      }
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::RemoveArray(int index)
{
  if ( (index<0) || (index>=this->NumberOfActiveArrays))
    {
    return;
    }
  this->Superclass::RemoveArray(index);
  int attributeType;
  for(attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
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
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes* fromPd,
                                    vtkIdType fromId, vtkIdType toId)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
      i=this->RequiredArrays.NextIndex())
    {
    this->CopyTuple(fromPd->Data[i], this->Data[this->TargetIndices[i]],
                    fromId, toId);
  }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes *fromPd,
                                    vtkIdList *fromIds, vtkIdList *toIds)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
      i=this->RequiredArrays.NextIndex())
    {
    this->CopyTuples(fromPd->Data[i], this->Data[this->TargetIndices[i]],
        fromIds, toIds);
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyAllocate(vtkDataSetAttributes* pd,
                                        vtkIdType sze, vtkIdType ext,
                                        int shallowCopyArrays)
{
  this->InternalCopyAllocate(pd, COPYTUPLE, sze, ext, shallowCopyArrays);
}

// Initialize point interpolation method.
void vtkDataSetAttributes::InterpolateAllocate(vtkDataSetAttributes* pd,
                                               vtkIdType sze, vtkIdType ext,
                                               int shallowCopyArrays)
{
  this->InternalCopyAllocate(pd, INTERPOLATE, sze, ext, shallowCopyArrays);
}

//--------------------------------------------------------------------------
// Interpolate data from points and interpolation weights. Make sure that the
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(vtkDataSetAttributes *fromPd,
                                            vtkIdType toId, vtkIdList *ptIds,
                                            double *weights)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
      i=this->RequiredArrays.NextIndex())
    {
    vtkAbstractArray* fromArray = this->Data[this->TargetIndices[i]];
    fromArray->InterpolateTuple(toId, ptIds, fromPd->Data[i], weights);
    }
}

//--------------------------------------------------------------------------
// Interpolate data from the two points p1,p2 (forming an edge) and an
// interpolation factor, t, along the edge. The weight ranges from (0,1),
// with t=0 located at p1. Make sure that the method InterpolateAllocate()
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateEdge(vtkDataSetAttributes *fromPd,
                                           vtkIdType toId, vtkIdType p1,
                                           vtkIdType p2, double t)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End();
      i=this->RequiredArrays.NextIndex())
    {
    vtkAbstractArray* fromArray = fromPd->Data[i];
    vtkAbstractArray* toArray = this->Data[this->TargetIndices[i]];

    //check if the destination array needs nearest neighbor interpolation
    int attributeIndex = this->IsArrayAnAttribute(this->TargetIndices[i]);
    if (attributeIndex != -1
        &&
        this->CopyAttributeFlags[INTERPOLATE][attributeIndex]==2)
      {
      double bt = (t < 0.5) ? 0.0 : 1.0;
      toArray->InterpolateTuple(toId, p1, fromArray, p2, fromArray, bt);
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
void vtkDataSetAttributes::InterpolateTime(vtkDataSetAttributes *from1,
                                           vtkDataSetAttributes *from2,
                                           vtkIdType id, double t)
{
  int attributeType;
  for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    // If this attribute is to be copied
    if (this->CopyAttributeFlags[INTERPOLATE][attributeType])
      {
      if (from1->GetAttribute(attributeType) &&
          from2->GetAttribute(attributeType))
        {
        vtkAbstractArray* toArray = this->GetAttribute(attributeType);
        //check if the destination array needs nearest neighbor interpolation
        if (this->CopyAttributeFlags[INTERPOLATE][attributeType]==2)
          {
          double bt = (t < 0.5) ? 0.0 : 1.0;
          toArray->InterpolateTuple(id, id, from1->GetAttribute(attributeType),
                                    id, from2->GetAttribute(attributeType), bt);
          }
        else
          {
          toArray->InterpolateTuple(id, id, from1->GetAttribute(attributeType),
                                    id, from2->GetAttribute(attributeType), t);
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
void vtkDataSetAttributes::CopyTuple(vtkAbstractArray *fromData,
                                     vtkAbstractArray *toData, vtkIdType fromId,
                                     vtkIdType toId)
{
  toData->InsertTuple(toId, fromId, fromData);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyTuples(vtkAbstractArray *fromData,
                                      vtkAbstractArray *toData,
                                      vtkIdList *fromIds, vtkIdList *toIds)
{
  toData->InsertTuples(toIds, fromIds, fromData);
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
int vtkDataSetAttributes::SetActiveAttribute(const char* name,
                                             int attributeType)
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
vtkDataArray* vtkDataSetAttributes::GetScalars(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetScalars();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetVectors(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetVectors();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetNormals(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetNormals();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTCoords(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetTCoords();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetTensors(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetTensors();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkDataArray* vtkDataSetAttributes::GetGlobalIds(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetGlobalIds();
    }
  return this->GetArray(name);
}

//--------------------------------------------------------------------------
vtkAbstractArray* vtkDataSetAttributes::GetPedigreeIds(const char* name)
{
  if (name == NULL || name[0] == '\0')
    {
    return this->GetPedigreeIds();
    }
  return this->GetAbstractArray(name);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::SetActiveAttribute(int index, int attributeType)
{
  if ( (index >= 0) && (index < this->GetNumberOfArrays()))
    {
    if (attributeType != PEDIGREEIDS)
      {
      vtkDataArray* darray = vtkDataArray::SafeDownCast(
        this->Data[index]);
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
const int vtkDataSetAttributes
::NumberOfAttributeComponents[vtkDataSetAttributes::NUM_ATTRIBUTES] =
{ 0,
  3,
  3,
  3,
  9,
  1,
  1,
  1};

//--------------------------------------------------------------------------
// Scalars set to NOLIMIT
const int vtkDataSetAttributes
::AttributeLimits[vtkDataSetAttributes::NUM_ATTRIBUTES] =
{ NOLIMIT,
  EXACT,
  EXACT,
  MAX,
  EXACT,
  EXACT,
  EXACT,
  EXACT};

//--------------------------------------------------------------------------
int vtkDataSetAttributes::CheckNumberOfComponents(vtkAbstractArray* aa,
                                                  int attributeType)
{
  int numComp = aa->GetNumberOfComponents();

  if ( vtkDataSetAttributes::AttributeLimits[attributeType] == MAX )
    {
    if ( numComp >
         vtkDataSetAttributes::NumberOfAttributeComponents[attributeType] )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else if ( vtkDataSetAttributes::AttributeLimits[attributeType] == EXACT )
    {
    if ( numComp !=
         vtkDataSetAttributes::NumberOfAttributeComponents[attributeType] )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else if ( vtkDataSetAttributes::AttributeLimits[attributeType] == NOLIMIT )
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
    return 0;
    }
  else
    {
    return vtkDataArray::SafeDownCast(this->Data[index]);
    }
}

//--------------------------------------------------------------------------
vtkAbstractArray* vtkDataSetAttributes::GetAbstractAttribute(int attributeType)
{
  int index = this->AttributeIndices[attributeType];
  if (index == -1)
    {
    return 0;
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
  if (aa && attributeType != PEDIGREEIDS && !vtkDataArray::SafeDownCast(aa))
    {
    vtkWarningMacro("Can not set attribute "
                    << vtkDataSetAttributes::AttributeNames[attributeType]
                    << ". This attribute must be a subclass of vtkDataArray.");
    return -1;
    }
  if (aa && !this->CheckNumberOfComponents(aa, attributeType))
    {
    vtkWarningMacro("Can not set attribute "
                    << vtkDataSetAttributes::AttributeNames[attributeType]
                    << ". Incorrect number of components.");
    return -1;
    }

  int currentAttribute = this->AttributeIndices[attributeType];

  // If there is an existing attribute, replace it
  if ( (currentAttribute >= 0) &&
       (currentAttribute < this->GetNumberOfArrays()) )
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
    this->AttributeIndices[attributeType] = -1; //attribute of this type doesn't exist
    }
  this->Modified();
  return this->AttributeIndices[attributeType];
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // Print the copy flags
  int i;
  os << indent << "Copy Tuple Flags: ( ";
  for (i=0; i<NUM_ATTRIBUTES; i++)
    {
    os << this->CopyAttributeFlags[COPYTUPLE][i] << " ";
    }
  os << ")" << endl;
  os << indent << "Interpolate Flags: ( ";
  for (i=0; i<NUM_ATTRIBUTES; i++)
    {
    os << this->CopyAttributeFlags[INTERPOLATE][i] << " ";
    }
  os << ")" << endl;
  os << indent << "Pass Through Flags: ( ";
  for (i=0; i<NUM_ATTRIBUTES; i++)
    {
    os << this->CopyAttributeFlags[PASSDATA][i] << " ";
    }
  os << ")" << endl;

  // Now print the various attributes
  vtkAbstractArray* aa;
  int attributeType;
  for (attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    os << indent << vtkDataSetAttributes::AttributeNames[attributeType]
       << ": ";
    if ( (aa=this->GetAbstractAttribute(attributeType)) )
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
  for(i=0; i<NUM_ATTRIBUTES; i++)
    {
    indexArray[i] = this->AttributeIndices[i];
    }
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::IsArrayAnAttribute(int idx)
{
  int i;
  for (i=0; i<NUM_ATTRIBUTES; i++)
    {
    if ( idx == this->AttributeIndices[i] )
      {
      return i;
      }
    }
  return -1;
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyAttribute (int index, int value, int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    int t;
    for (t = COPYTUPLE; t < vtkDataSetAttributes::ALLCOPY; t++)
      {
      if (this->CopyAttributeFlags[t][ index ] != value)
        {
        this->CopyAttributeFlags[t][ index ] = value;
        this->Modified();
        }
      }
    }
  else
    {
    if (this->CopyAttributeFlags[ctype][ index ] != value)
      {
      this->CopyAttributeFlags[ctype][ index ] = value;
      this->Modified();
      }
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyScalars(int i, int ctype)
{
  this->SetCopyAttribute(SCALARS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyScalars(int ctype) {
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][SCALARS] &&
      this->CopyAttributeFlags[INTERPOLATE][SCALARS] &&
      this->CopyAttributeFlags[PASSDATA][SCALARS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][SCALARS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyVectors(int i, int ctype)
{
  this->SetCopyAttribute(VECTORS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyVectors(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][VECTORS] &&
      this->CopyAttributeFlags[INTERPOLATE][VECTORS] &&
      this->CopyAttributeFlags[PASSDATA][VECTORS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][VECTORS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyNormals(int i, int ctype)
{
  this->SetCopyAttribute(NORMALS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyNormals(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][NORMALS] &&
      this->CopyAttributeFlags[INTERPOLATE][NORMALS] &&
      this->CopyAttributeFlags[PASSDATA][NORMALS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][NORMALS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyTCoords(int i, int ctype)
{
  this->SetCopyAttribute(TCOORDS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyTCoords(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][TCOORDS] &&
      this->CopyAttributeFlags[INTERPOLATE][TCOORDS] &&
      this->CopyAttributeFlags[PASSDATA][TCOORDS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][TCOORDS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyTensors(int i, int ctype)
{
  this->SetCopyAttribute(TENSORS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyTensors(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][TENSORS] &&
      this->CopyAttributeFlags[INTERPOLATE][TENSORS] &&
      this->CopyAttributeFlags[PASSDATA][TENSORS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][TENSORS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyGlobalIds(int i, int ctype)
{
  this->SetCopyAttribute(GLOBALIDS, i, ctype);
}
//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyGlobalIds(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][GLOBALIDS] &&
      this->CopyAttributeFlags[INTERPOLATE][GLOBALIDS] &&
      this->CopyAttributeFlags[PASSDATA][GLOBALIDS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][GLOBALIDS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::SetCopyPedigreeIds(int i, int ctype)
{
  this->SetCopyAttribute(PEDIGREEIDS, i, ctype);
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::GetCopyPedigreeIds(int ctype)
{
  if (ctype == vtkDataSetAttributes::ALLCOPY)
    {
    return
      this->CopyAttributeFlags[COPYTUPLE][PEDIGREEIDS] &&
      this->CopyAttributeFlags[INTERPOLATE][PEDIGREEIDS] &&
      this->CopyAttributeFlags[PASSDATA][PEDIGREEIDS];
    }
  else
    {
    return
      this->CopyAttributeFlags[ctype][PEDIGREEIDS];
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::RemoveArray(const char *name)
{
  int i;
  this->GetAbstractArray(name, i);
  this->RemoveArray(i);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::CopyAllocate(
  vtkDataSetAttributes::FieldList& list,
  vtkIdType sze, vtkIdType ext)
{
  this->InternalCopyAllocate(list, COPYTUPLE, sze, ext);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::InterpolateAllocate(
  vtkDataSetAttributes::FieldList& list, vtkIdType sze,
  vtkIdType ext)
{
  this->InternalCopyAllocate(list, INTERPOLATE, sze, ext);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::InternalCopyAllocate(
  vtkDataSetAttributes::FieldList& list,
  int ctype,
  vtkIdType sze, vtkIdType ext)
{
  vtkAbstractArray* newAA=0;
  vtkDataArray* newDA=0;
  int i;

  // Allocate attributes if any
  for (i=0; i < list.NumberOfFields; i++)
    {
    if ( list.FieldIndices[i] >= 0 )
      {
      newAA = vtkAbstractArray::CreateArray(list.FieldTypes[i]);
      newAA->SetName(list.Fields[i]);
      newAA->SetNumberOfComponents(list.FieldComponents[i]);

      if ( list.FieldComponentsNames[i] )
        {
        for (unsigned int j=0; j < list.FieldComponentsNames[i]->size(); ++j)
          {
          if (list.FieldComponentsNames[i]->at(j).first)
            {
            newAA->SetComponentName(j,
              list.FieldComponentsNames[i]->at(j).second.c_str());
            }
          }
        }
      if (list.FieldInformation[i])
        {
        newAA->CopyInformation(list.FieldInformation[i],/*deep=*/1);
        }

      if ( sze > 0 )
        {
        newAA->Allocate(sze,ext);
        }
      else
        {
        newAA->Allocate(list.NumberOfTuples,ext);
        }

      if ( (newDA = vtkDataArray::SafeDownCast(newAA)) )
        {
        newDA->SetLookupTable(list.LUT[i]);
        }


      // If attribute data, do something extra
      if ( i < NUM_ATTRIBUTES )
        {
        // since attributes can only be DataArray, newDA must be non-null.
        if ( this->CopyAttributeFlags[ctype][i] && newDA)
          {
          list.FieldIndices[i] = this->AddArray(newDA);
          this->SetActiveAttribute(list.FieldIndices[i], i);
          }
        else
          {
          list.FieldIndices[i] = -1;
          }
        }
      else //check if this field is to be copied
        {
        if ( (this->GetFlag(list.Fields[i]) != 0) &&
             !(this->DoCopyAllOff && (this->GetFlag(list.Fields[i]) != 1)) )
          {
          list.FieldIndices[i] = this->AddArray(newAA);
          }
        else
          {
          list.FieldIndices[i] = -1;
          }
        }

      newAA->Delete(); //okay, reference counting
      }//data array defined
    }
}

//--------------------------------------------------------------------------
// Description:
// A special form of CopyData() to be used with FieldLists. Use it when you are
// copying data from a set of vtkDataSetAttributes. Make sure that you have
// called the special form of CopyAllocate that accepts FieldLists.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes::FieldList& list,
                                    vtkDataSetAttributes* fromDSA,
                                    int idx, vtkIdType fromId, vtkIdType toId)
{
  vtkAbstractArray *fromDA;
  vtkAbstractArray *toDA;

  int i;
  for (i=0; i < list.NumberOfFields; i++)
    {
    if ( list.FieldIndices[i] >= 0 && list.DSAIndices[idx][i] >= 0 )
      {
      toDA = this->GetAbstractArray(list.FieldIndices[i]);
      fromDA = fromDSA->GetAbstractArray(list.DSAIndices[idx][i]);
      this->CopyTuple(fromDA, toDA, fromId, toId);
      }
    }
}

//--------------------------------------------------------------------------
// Interpolate data from points and interpolation weights. Make sure that the
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(
  vtkDataSetAttributes::FieldList& list,
  vtkDataSetAttributes *fromPd,
  int idx,
  vtkIdType toId, vtkIdList *ptIds,
  double *weights)
{
  vtkAbstractArray *fromArray;
  vtkAbstractArray *toArray;

  for (int i=0; i < list.NumberOfFields; i++)
    {
    if ( list.FieldIndices[i] >= 0 && list.DSAIndices[idx][i] >= 0 )
      {
      toArray = this->GetAbstractArray(list.FieldIndices[i]);
      fromArray = fromPd->GetAbstractArray(list.DSAIndices[idx][i]);
      toArray->InterpolateTuple(toId, ptIds, fromArray, weights);
      }
    }
}

//--------------------------------------------------------------------------
const char* vtkDataSetAttributes::GetAttributeTypeAsString(int attributeType)
{
  if (attributeType < 0 || attributeType >= NUM_ATTRIBUTES)
    {
    vtkGenericWarningMacro("Bad attribute type.");
    return NULL;
    }
  return vtkDataSetAttributes::AttributeNames[attributeType];
}

//--------------------------------------------------------------------------
const char* vtkDataSetAttributes::GetLongAttributeTypeAsString(int attributeType)
{
  if (attributeType < 0 || attributeType >= NUM_ATTRIBUTES)
    {
    vtkGenericWarningMacro("Bad attribute type.");
    return NULL;
    }
  return vtkDataSetAttributes::LongAttributeNames[attributeType];
}

//=============================================================================
vtkDataSetAttributes::FieldList::FieldList(int numInputs)
{
  this->Fields = 0;
  this->FieldTypes = 0;
  this->FieldComponents = 0;
  this->FieldComponentsNames = 0;
  this->FieldIndices = 0;
  this->NumberOfFields = 0;
  this->LUT = 0;
  this->FieldInformation = 0;
  this->DSAIndices = 0;
  this->NumberOfDSAIndices = 0;
  //
  if (numInputs)
    {
    this->NumberOfDSAIndices = numInputs;
    this->DSAIndices = new int*[numInputs];
    int i;
    for (i=0; i<numInputs; i++)
      {
      this->DSAIndices[i] = 0;
      }
    }
}

//--------------------------------------------------------------------------
vtkDataSetAttributes::FieldList::~FieldList()
{
  this->ClearFields();
  delete [] this->DSAIndices;
  this->DSAIndices = 0;
}

//----------------------------------------------------------------------------
// To perform intersection of attribute data, use IntializeFieldList() to grab
// an initial vtkDataSetAttributes. Then use IntersectFieldList() to add (and
// intersect) additional vtkDataSetAttributes.
void vtkDataSetAttributes::FieldList::InitializeFieldList(
  vtkDataSetAttributes* dsa)
{
  int i;
  this->ClearFields();
  // Allocate space for the arrays plus five attributes
  this->NumberOfFields = dsa->GetNumberOfArrays() + NUM_ATTRIBUTES;
  this->Fields = new char*[this->NumberOfFields];
  this->FieldTypes = new int [this->NumberOfFields];
  this->FieldComponents = new int [this->NumberOfFields];
  this->FieldComponentsNames =
    new vtkDataSetAttributes::vtkInternalComponentNames*[this->NumberOfFields];
  this->FieldIndices = new int [this->NumberOfFields];
  this->LUT = new vtkLookupTable* [this->NumberOfFields];
  this->FieldInformation = new vtkInformation* [this->NumberOfFields];
  for(i=0; i < this->NumberOfFields; i++)
    {
    this->Fields[i] = 0;
    this->FieldTypes[i] = -1;
    this->FieldComponents[i] = 0;
    this->FieldComponentsNames[i] = 0;
    this->FieldIndices[i] = -1;
    this->LUT[i] = 0;
    this->FieldInformation[i] = 0;
    }
  this->CurrentInput = 0;
  this->NumberOfTuples = 0;

  //there may be no data hence dsa->Data
  for(i=0; dsa->Data && i < dsa->GetNumberOfArrays(); i++)
    {
    int attrType = dsa->IsArrayAnAttribute(i);
    if ( attrType != -1 ) //it's an attribute
      {
      this->FieldIndices[attrType] = i;
      this->SetField(attrType, dsa->Data[i]);
      }
    else
      {
      this->FieldIndices[NUM_ATTRIBUTES+i] = i;
      this->SetField(NUM_ATTRIBUTES+i, dsa->Data[i]);
      }
    }

  // The first dataset is added to the field list
  this->IntersectFieldList(dsa);
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::UnionFieldList(vtkDataSetAttributes* dsa)
{
  vtkAbstractArray* aa;
  vtkDataArray* da;
  // Keep a running total of the number of tuples...might be useful
  // for later allocation.
  if ( (aa=dsa->GetAbstractArray(0)) )
    {
    this->NumberOfTuples += aa->GetNumberOfTuples();
    }

  // unlike Intersection, with Union the the total number of fields may change,
  // so we have to be careful with that.
  std::vector<int> dsaIndices;
  dsaIndices.resize(this->NumberOfFields, -1);

  // Intersect the active attributes. (Even though we are taking a union, we
  // intersect the active attributes).
  int attributeIndices[NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);
  for (int i=0; i < NUM_ATTRIBUTES; i++)
    {
    if (this->FieldIndices[i] >= 0)
      {
      da = dsa->GetAttribute(i);
      if ((da) && (da->GetDataType() == this->FieldTypes[i]) &&
          (da->GetNumberOfComponents() == this->FieldComponents[i]))
        {
        dsaIndices[i] = attributeIndices[i];
        }
      else
        {
        // doh! A attribute is not available in this new dsa. But it was
        // available until now.

        // In InitializeFieldList(), if an array is an active attribute, its
        // information is noted only in the first "set". Now since we are
        // marking it as not-an-attribute, we still don't want to lose the
        // array. So we enable it in the second "set". But all DSAIndices until
        // the CurrentInput referred to this array in it's location in the
        // first "set" so we have to move those as well. That's what's
        // happening here.
        int offset = this->FieldIndices[i];
        this->FieldIndices[NUM_ATTRIBUTES + offset] = this->FieldIndices[i];
        this->Fields[offset+NUM_ATTRIBUTES] = this->Fields[i];
        this->FieldTypes[offset+NUM_ATTRIBUTES] = this->FieldTypes[i];
        this->FieldComponents[offset+NUM_ATTRIBUTES] = this->FieldComponents[i];
        this->FieldComponentsNames[offset+NUM_ATTRIBUTES] = this->FieldComponentsNames[i];
        this->LUT[offset+NUM_ATTRIBUTES] = this->LUT[i];
        this->FieldInformation[offset+NUM_ATTRIBUTES] = this->FieldInformation[i];

        this->FieldIndices[i] = -1; //Attribute not present
        this->Fields[i] = NULL;
        this->FieldTypes[i] = -1;
        this->FieldComponents[i] = 0;
        this->FieldComponentsNames[i] = NULL;
        this->LUT[i] = NULL;
        this->FieldInformation[i] = NULL;

        for (int cc=0; cc < this->CurrentInput && cc < this->NumberOfDSAIndices;
          cc++)
          {
          this->DSAIndices[cc][offset+NUM_ATTRIBUTES] = this->DSAIndices[cc][i];
          this->DSAIndices[cc][i] = -1;
          }
        }
      }
    }

  std::vector<bool> dsaMarkedArrays;
  dsaMarkedArrays.resize(dsa->GetNumberOfArrays(), false);

  // * Try to match the existing fields with those in dsa.
  for (int i = NUM_ATTRIBUTES; i < this->NumberOfFields; i++)
    {
    // FieldIndices should really be a bool.
    if (this->FieldIndices[i] < 0)
      {
      continue;
      }
    int index;
    aa = dsa->GetAbstractArray(this->Fields[i], index);
    if ((aa) && (aa->GetDataType() == this->FieldTypes[i]) &&
      (aa->GetNumberOfComponents() == this->FieldComponents[i]))
      {
      dsaIndices[i] = index;
      dsaMarkedArrays[index] = true;
      }
    }

  // * Now every array in dsaMarkedArrays that has a false, implies that it did not
  // match with any of the existing fields. So those will be appended to the
  // end of the field list.
  std::vector<int> dsaPendingIndices;
  for (size_t cc=0; cc < dsaMarkedArrays.size(); cc++)
    {
    if (dsaMarkedArrays[cc] == false)
      {
      dsaPendingIndices.push_back(static_cast<int>(cc));
      }
    }

  if (dsaPendingIndices.size() != 0)
    {
    size_t old_size = dsaIndices.size();
    size_t new_size = old_size + dsaPendingIndices.size();

    // * If dsaPendingIndices != empty, then we need to grow the num of fields.
    this->GrowBy(static_cast<unsigned int>(dsaPendingIndices.size()));

    dsaIndices.resize(new_size, -1);
    for (size_t cc=0; cc < dsaPendingIndices.size(); cc++)
      {
      this->FieldIndices[old_size+cc] =
        static_cast<int>((old_size+cc) - NUM_ATTRIBUTES);
      this->SetField(static_cast<int>(old_size+cc),
        dsa->GetAbstractArray(dsaPendingIndices[cc]));
      dsaIndices[old_size + cc] = dsaPendingIndices[cc];
      }
    }

  this->DSAIndices[this->CurrentInput] = new int [this->NumberOfFields];
  memcpy(this->DSAIndices[this->CurrentInput], &dsaIndices[0],
    sizeof(int)*this->NumberOfFields);

  this->CurrentInput++;
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::GrowBy(unsigned int delta)
{
  if (delta == 0)
    {
    return;
    }

  int old_size = this->NumberOfFields;
  int new_size = this->NumberOfFields + delta;

  char** newFields = new char*[new_size];
  int* newFieldTypes = new int[new_size];
  int* newFieldComponents = new int [new_size];
  vtkDataSetAttributes::vtkInternalComponentNames** newFieldComponentsNames
    = new vtkDataSetAttributes::vtkInternalComponentNames* [ new_size ];

  int* newFieldIndices = new int [new_size];
  vtkLookupTable** newLUT = new vtkLookupTable* [new_size];
  vtkInformation** newFieldInformation = new vtkInformation* [new_size];

  // copy the old fields.
  for(int i=0; i < old_size; i++)
    {
    if (this->Fields[i])
      {
      newFields[i] = strdup(this->Fields[i]);
      }
    else
      {
      newFields[i] = NULL;
      }
    if ( this->FieldComponentsNames[i] )
      {
      newFieldComponentsNames[i] =
        new vtkDataSetAttributes::vtkInternalComponentNames(
        *this->FieldComponentsNames[i]);
      }
    else
      {
      newFieldComponentsNames[i] = NULL;
      }
    }
  memcpy(newFieldTypes, this->FieldTypes, sizeof(int)*old_size);
  memcpy(newFieldComponents, this->FieldComponents, sizeof(int)*old_size);
  memcpy(newFieldIndices, this->FieldIndices, sizeof(int)*old_size);
  memcpy(newLUT, this->LUT, sizeof(vtkLookupTable*)*old_size);
  memcpy(newFieldInformation, this->FieldInformation,
    sizeof(vtkInformation*)*old_size);

  // initialize the rest.
  for (int i=old_size; i < new_size; i++)
    {
    newFields[i] = NULL;
    newFieldTypes[i] = -1;
    newFieldComponents[i] = 0;
    newFieldIndices[i] = -1;
    newLUT[i] = NULL;
    newFieldInformation[i] = NULL;
    newFieldComponentsNames[i] = NULL;
    }

  int **newDSAIndices = new int*[this->NumberOfDSAIndices];
  for (int cc=0; cc < this->NumberOfDSAIndices; cc++)
    {
    if (this->DSAIndices[cc] != NULL)
      {
      newDSAIndices[cc] = new int[new_size];
      memcpy(newDSAIndices[cc], this->DSAIndices[cc], sizeof(int)*old_size);
      for (int kk=old_size; kk < new_size; kk++)
        {
        newDSAIndices[cc][kk] = -1;
        }
      }
    else
      {
      newDSAIndices[cc] = NULL;
      }
    }

  int currentInput = this->CurrentInput;
  int numberOfDSAIndices = this->NumberOfDSAIndices;

  this->ClearFields();

  this->NumberOfFields = new_size;
  this->NumberOfDSAIndices = numberOfDSAIndices;
  this->CurrentInput = currentInput;
  this->Fields = newFields;
  this->FieldTypes = newFieldTypes;
  this->FieldComponents = newFieldComponents;
  this->FieldComponentsNames = newFieldComponentsNames;
  this->FieldIndices = newFieldIndices;
  this->LUT = newLUT;
  this->FieldInformation = newFieldInformation;
  this->DSAIndices = newDSAIndices;
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::IntersectFieldList(vtkDataSetAttributes* dsa)
{
  int i;
  vtkDataArray *da;
  vtkAbstractArray* aa;

  // Initialize the indices for this dataset
  this->DSAIndices[this->CurrentInput] = new int [this->NumberOfFields];
  for (i=0; i < this->NumberOfFields; i++)
    {
    this->DSAIndices[this->CurrentInput][i]= -1;
    }

  // Keep a running total of the number of tuples...might be useful
  // for later allocation.
  if ( (da=dsa->GetArray(0)) )
    {
    this->NumberOfTuples += da->GetNumberOfTuples();
    }

  // Intersect the attributes
  int attributeIndices[NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);
  for(i=0; i < NUM_ATTRIBUTES; i++)
    {
    if ( this->FieldIndices[i] >= 0 )
      {
      da = dsa->GetAttribute(i);
      if ((da) && (da->GetDataType() == this->FieldTypes[i]) &&
          (da->GetNumberOfComponents() == this->FieldComponents[i]))
        {
        this->DSAIndices[this->CurrentInput][i] = attributeIndices[i];
        }
      else
        {
        this->FieldIndices[i] = -1; //Attribute not present
        }
      }
    }
  // Intersect the fields
  int index;
  for(i=NUM_ATTRIBUTES; i < this->NumberOfFields; i++)
    {
    if (this->FieldIndices[i] >= 0)
      {
      aa = dsa->GetAbstractArray(this->Fields[i], index);
      if ((aa) && (aa->GetDataType() == this->FieldTypes[i]) &&
          (aa->GetNumberOfComponents() == this->FieldComponents[i]))
        {
        this->DSAIndices[this->CurrentInput][i] = index;
        }
      else
        {
        this->FieldIndices[i] = -1; //Field not present
        }
      }
    }

  this->CurrentInput++;
}

//--------------------------------------------------------------------------
int vtkDataSetAttributes::FieldList::IsAttributePresent(int attrType)
{
  return this->FieldIndices[attrType];
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::ClearFields()
{
  int i;
  if ( this->Fields )
    {
    for (i=0; i<this->NumberOfFields; i++)
      {
      delete [] this->Fields[i];
      this->Fields[i] = 0;
      }
    }
  if ( this->DSAIndices )
    {
    for (i=0; i<this->NumberOfDSAIndices; i++)
      {
      delete[] this->DSAIndices[i];
      this->DSAIndices[i] = 0;
      }
    }
  //
  delete [] this->Fields;
  this->Fields = 0;

  delete [] this->FieldInformation;
  this->FieldInformation = 0;

  delete [] this->LUT;
  this->LUT = 0;

  delete [] this->FieldTypes;
  this->FieldTypes = 0;

  delete [] this->FieldComponents;
  this->FieldComponents = 0;

  if ( this->FieldComponentsNames )
    {
    for (i=0; i<this->NumberOfFields; i++)
      {
      delete this->FieldComponentsNames[i];
      }
    delete [] this->FieldComponentsNames;
    this->FieldComponentsNames = 0;
    }

  delete [] this->FieldIndices;
  this->FieldIndices = 0;

  this->NumberOfFields = 0;
  this->CurrentInput = 0;
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::SetField(
        int index,
        vtkAbstractArray *aa)
{
  // Store the field name
  delete [] this->Fields[index];
  this->Fields[index] = 0;
  const char* name=aa->GetName();
  if (name)
    {
    int len = static_cast<int>(strlen(name));
    if (len > 0)
      {
      this->Fields[index] = new char[len+1];
      strcpy(this->Fields[index], name);
      }
    }
  // Store the data type
  this->FieldTypes[index] = aa->GetDataType();

  //we unallocate the names before we update the field components
  //so we unallocate correctly
  if ( this->FieldComponentsNames[index] )
    {
    delete this->FieldComponentsNames[index];
    this->FieldComponentsNames[index] = NULL;
    }

  //store the components names
  int numberOfComponents = aa->GetNumberOfComponents();
  if ( aa->HasAComponentName() )
    {
    this->FieldComponentsNames[index] =
      new vtkDataSetAttributes::vtkInternalComponentNames();
    this->FieldComponentsNames[index]->resize(numberOfComponents,
      std::pair<bool, vtkStdString>(false, vtkStdString()));
    name = NULL;
    for ( vtkIdType i=0; i < numberOfComponents; ++i)
      {
      name = aa->GetComponentName(i);
      if ( name )
        {
        this->FieldComponentsNames[index]->at(i) =
          std::pair<bool, vtkStdString>(true, name);
        name = NULL;
        }
      }
    }

  // Store the components
  this->FieldComponents[index] = numberOfComponents;

  // Store the lookup table
  this->LUT[index]=0;
  if (vtkDataArray::SafeDownCast(aa))
    {
    this->LUT[index]=vtkDataArray::SafeDownCast(aa)->GetLookupTable();
    }
  // Store the information
  this->FieldInformation[index] = 0;
  if (aa->HasInformation())
    {
    this->FieldInformation[index] = aa->GetInformation();
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::RemoveField(const char *name)
{
  if ( !name )
    {
    return;
    }

  int i;
  for (i=NUM_ATTRIBUTES; i < this->NumberOfFields; i++)
    {
    if ( this->Fields[i] && !strcmp(this->Fields[i],name) )
      {
      delete [] this->Fields[i];
      this->Fields[i] = 0;
      this->FieldTypes[i] = -1;
      this->FieldComponents[i] = 0;

      delete this->FieldComponentsNames[i];
      this->FieldComponentsNames[i] = 0;

      this->FieldIndices[i] = -1;
      this->LUT[i] = 0;
      this->FieldInformation[i] = 0;
      return;
      }
    }
}

//--------------------------------------------------------------------------
void vtkDataSetAttributes::FieldList::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "Number of Fields:" << this->NumberOfFields << endl;
  vtkIndent nextIndent=indent.GetNextIndent();
  for (int i=0; i<this->NumberOfFields; ++i)
    {
    os << indent << "Field " << i << " {" << endl
       << nextIndent
       << (this->Fields[i]==0?"NULL":this->Fields[i]) << ", "
       << this->FieldTypes[i] << ", "
       << this->FieldComponents[i] << ", "
       << this->FieldIndices[i] << ", "
       << this->FieldInformation[i]
       << "}" << endl;
    }
}
