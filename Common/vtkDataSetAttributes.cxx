/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkDataSetAttributes.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkVoidArray.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
char vtkDataSetAttributes::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10] =
  { "Scalars",
    "Vectors",
    "Normals",
    "TCoords",
    "Tensors" };

vtkDataSetAttributes* vtkDataSetAttributes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataSetAttributes");
  if(ret)
    {
    return (vtkDataSetAttributes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetAttributes;
}

// Construct object with copying turned on for all data.
vtkDataSetAttributes::vtkDataSetAttributes()
{
  for(int attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    this->AttributeIndices[attributeType] = -1;
    this->CopyAttributeFlags[attributeType] = 1;
    this->Attributes[attributeType] = 0;
    }
  this->TargetIndices=0;
}

// Destructor for the vtkDataSetAttributes objects.
vtkDataSetAttributes::~vtkDataSetAttributes()
{
  this->Initialize();
  delete[] this->TargetIndices;
  this->TargetIndices = 0;
}


// Turn on copying of all data.
void vtkDataSetAttributes::CopyAllOn()
{
  this->CopyScalarsOn();
  this->CopyVectorsOn();
  this->CopyNormalsOn();
  this->CopyTCoordsOn();
  this->CopyTensorsOn();
  this->DoCopyAllOn = 1;
  this->DoCopyAllOff = 0;
}

// Turn off copying of all data.
void vtkDataSetAttributes::CopyAllOff()
{
  this->CopyScalarsOff();
  this->CopyVectorsOff();
  this->CopyNormalsOff();
  this->CopyTCoordsOff();
  this->CopyTensorsOff();
  this->DoCopyAllOn = 0;
  this->DoCopyAllOff = 1;
}

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
    int attributeType;
    vtkDataArray *data, *newData;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    for ( int i=0; i < numArrays; i++ )
      {
      data = fd->GetArray(i);
      newData = data->MakeObject(); //instantiate same type of object
      newData->DeepCopy(data);
      newData->SetName(data->GetName());
      if ((attributeType=dsa->IsArrayAnAttribute(i)) != -1)
	{
	// If this array is an attribute in the source, make it so
	// in the target as well.
	this->SetAttribute(newData, attributeType);
	}
      else
	{
	this->AddArray(newData);
	}
      newData->Delete();
      }
    // Copy the copy flags
    for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
      {
      this->CopyAttributeFlags[attributeType] = 
	dsa->CopyAttributeFlags[attributeType];
      }
    this->CopyFlags(dsa);
    }
  // If the source is field data, do a field data copy
  else
    {
    this->vtkFieldData::DeepCopy(fd);
    }
	  
}

// Shallow copy of data (i.e., use reference counting).
void vtkDataSetAttributes::ShallowCopy(vtkFieldData *fd)
{
  this->Initialize(); //free up memory

  vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
  // If the source is a vtkDataSetAttributes
  if (dsa)
    {
    int numArrays = fd->GetNumberOfArrays();
    int attributeType;

    // Allocate space for numArrays
    this->AllocateArrays(numArrays);
    this->NumberOfActiveArrays = 0;
    for ( int i=0; i < numArrays; i++ )
      {
      this->NumberOfActiveArrays++;
      this->SetArray(i, fd->GetArray(i));
      if ((attributeType=dsa->IsArrayAnAttribute(i)) != -1)
	{
	// If this array is an attribute in the source, make it so
	// in the target as well.
	this->SetActiveAttribute(i, attributeType);
	// Copy the attribute data too
        this->Attributes[attributeType] = dsa->Attributes[attributeType];
        if (this->Attributes[attributeType])
          {
          this->Attributes[attributeType]->Register(this);
          }
	}
      }
    // Copy the copy flags
    for(attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
      {
      this->CopyAttributeFlags[attributeType] = 
	dsa->CopyAttributeFlags[attributeType];
      }
    this->CopyFlags(dsa);
    }
  // If the source is field data, do a field data copy
  else
    {
    this->vtkFieldData::ShallowCopy(fd);
    }
}

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
//
  for(int attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    if (this->Attributes[attributeType])
      {
      this->Attributes[attributeType]->UnRegister(this);
      this->Attributes[attributeType] = 0;
      }
    this->AttributeIndices[attributeType] = -1;
    }
}

// This method is used to determine which arrays
// will be copied to this object after PassData or PassNoReplaceData
vtkFieldData::BasicIterator  vtkDataSetAttributes::ComputeRequiredArrays(
  vtkDataSetAttributes* pd)
{
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
	 pd->GetArray(i))
      {
      copyFlags[numArrays] = i;
      numArrays++;
      }
    }

  // Next, we check the arrays to be copied because they are one of
  // the _attributes_ to be copied (and the data array in non-NULL). 
  // We make sure that we don't count anything twice.
  int alreadyCopied;
  for(int attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    index = pd->AttributeIndices[attributeType];
    // If this attribute is to be copied
    if (this->CopyAttributeFlags[attributeType])
      {
      // Find out if it is also in the list of fields to be copied
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
          copyFlags[numArrays] = index;
          numArrays++;
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
          for(int j=i; j<numArrays-1; j++)
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
    vtkFieldData::BasicIterator it = this->ComputeRequiredArrays(dsa);
    
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
      if (this->CopyAttributeFlags[attributeType])
	{
	this->RemoveArray(this->AttributeIndices[attributeType]);
	this->AttributeIndices[attributeType] = -1;
	}
      }
    
    int i, arrayIndex;
    for(i=it.BeginIndex(); !it.End(); i=it.NextIndex())
      {
      arrayIndex = this->AddArray(dsa->GetArray(i));
      // If necessary, make the array an attribute
      if ( ((attributeType = dsa->IsArrayAnAttribute(i)) != -1 ) && 
	   this->CopyAttributeFlags[attributeType] )
	{
	this->SetActiveAttribute(arrayIndex, attributeType);
	// Also pass the attribute data
	this->Attributes[attributeType] = dsa->Attributes[attributeType];
	if (this->Attributes[attributeType])
	  {
	  this->Attributes[attributeType]->Register(this);
	  }
	}
      }
    }
  else
    {
    this->vtkFieldData::PassData(fd);
    }
}

// Allocates point data for point-by-point (or cell-by-cell) copy operation.  
// If sze=0, then use the input DataSetAttributes to create (i.e., find 
// initial size of) new objects; otherwise use the sze variable.
void vtkDataSetAttributes::CopyAllocate(vtkDataSetAttributes* pd, 
					vtkIdType sze, vtkIdType ext)
{
  vtkDataArray* newDA;
  int i;

  // If we are not copying on self
  if ( pd != this )
    {
    this->Initialize();
    }

  // Create various point data depending upon input
  //
  if ( !pd )
    {
    return;
    }

  this->RequiredArrays = this->ComputeRequiredArrays(pd);
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

  vtkDataArray* da=0;
  // If we are not copying on self
  if ( pd != this )
    {
    int attributeType;

    for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End(); 
        i=this->RequiredArrays.NextIndex())
      {
      // Create all required arrays
      da = pd->GetArray(i);
      newDA = da->MakeObject();
      newDA->SetName(da->GetName());
      if ( sze > 0 )
        {
        newDA->Allocate(sze*da->GetNumberOfComponents(),ext);
        }
      else
        {
        newDA->Allocate(da->GetNumberOfTuples());
        }
      newDA->SetLookupTable(da->GetLookupTable());
      this->TargetIndices[i] = this->AddArray(newDA);
      // If necessary, make the array an attribute
      if ( ((attributeType = pd->IsArrayAnAttribute(i)) != -1 ) && 
	   this->CopyAttributeFlags[attributeType] )
	{
	this->SetActiveAttribute(this->TargetIndices[i], attributeType);
	}
      newDA->Delete();
      }
    }
  else
    {
    // If copying on self, resize the arrays and initialize
    // TargetIndices
    for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End(); 
        i=this->RequiredArrays.NextIndex())
      {
      da = pd->GetArray(i);
      da->Resize(sze);
      this->TargetIndices[i] = i;
      }
    }
}

void vtkDataSetAttributes::RemoveArray(int index)
{
  if ( (index<0) || (index>=this->NumberOfActiveArrays))
    {
    return;
    }
  this->vtkFieldData::RemoveArray(index);
  int attributeType;
  for(attributeType = 0; attributeType < NUM_ATTRIBUTES; attributeType++)
    {
    if (this->AttributeIndices[attributeType] == index)
      {
      this->AttributeIndices[attributeType] = -1;
      // UnRegister the exiting vtkAttributeData and set it to NULL
      if (this->Attributes[attributeType])
        {
        this->Attributes[attributeType]->UnRegister(this);
        }
      this->Attributes[attributeType] = 0;
      }
    else if (this->AttributeIndices[attributeType] > index)
      {
      this->AttributeIndices[attributeType]--;
      }
    }
}

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

// Initialize point interpolation method.
void vtkDataSetAttributes::InterpolateAllocate(vtkDataSetAttributes* pd,
                                               vtkIdType sze, vtkIdType ext)
{
  this->CopyAllocate(pd, sze, ext);
}

// Interpolate data from points and interpolation weights. Make sure that the 
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(vtkDataSetAttributes *fromPd, 
                                            vtkIdType toId, vtkIdList *ptIds, 
                                            float *weights)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End(); 
      i=this->RequiredArrays.NextIndex())
    {
    this->InterpolateTuple(fromPd->Data[i], 
                           this->Data[this->TargetIndices[i]], 
                           toId, ptIds, weights);
    }
}

// Interpolate data from the two points p1,p2 (forming an edge) and an 
// interpolation factor, t, along the edge. The weight ranges from (0,1), 
// with t=0 located at p1. Make sure that the method InterpolateAllocate() 
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateEdge(vtkDataSetAttributes *fromPd, 
                                           vtkIdType toId, vtkIdType p1,
                                           vtkIdType p2, float t)
{
  int i;
  for(i=this->RequiredArrays.BeginIndex(); !this->RequiredArrays.End(); 
      i=this->RequiredArrays.NextIndex())
    {
    this->InterpolateTuple(fromPd->Data[i], 
                           this->Data[this->TargetIndices[i]], 
                           toId, p1, p2, t);
    }
}

// Interpolate data from the two points p1,p2 (forming an edge) and an 
// interpolation factor, t, along the edge. The weight ranges from (0,1), 
// with t=0 located at p1. Make sure that the method InterpolateAllocate() 
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateTime(vtkDataSetAttributes *from1,
                                           vtkDataSetAttributes *from2,
                                           vtkIdType id, float t)
{
  for(int attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    // If this attribute is to be copied
    if (this->CopyAttributeFlags[attributeType])
      {
      this->InterpolateTuple(from1->GetActiveAttribute(attributeType), 
                             from2->GetActiveAttribute(attributeType),
                             this->GetActiveAttribute(attributeType), id, t);
      }
    }
}

// Copy a tuple of data from one data array to another. This method (and
// following ones) assume that the fromData and toData objects are of the
// same type, and have the same number of components. This is true if you
// invoke CopyAllocate() or InterpolateAllocate().
void vtkDataSetAttributes::CopyTuple(vtkDataArray *fromData,
                                     vtkDataArray *toData, vtkIdType fromId,
                                     vtkIdType toId)
{
  int i;
  int numComp=fromData->GetNumberOfComponents();

  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        to->InsertValue(toId+i, from->GetValue(fromId+i));
        }
      }
      break;

    case VTK_CHAR:
      {
      char *to=((vtkCharArray *)toData)->WritePointer(toId*numComp,numComp);
      char *from=((vtkCharArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(toId*numComp,numComp);
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *to=((vtkShortArray *)toData)->WritePointer(toId*numComp,numComp);
      short *from=((vtkShortArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(toId*numComp,numComp);
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_INT:
      {
      int *to=((vtkIntArray *)toData)->WritePointer(toId*numComp,numComp);
      int *from=((vtkIntArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(toId*numComp,numComp);
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_LONG:
      {
      long *to=((vtkLongArray *)toData)->WritePointer(toId*numComp,numComp);
      long *from=((vtkLongArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(toId*numComp,numComp);
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *to=((vtkFloatArray *)toData)->WritePointer(toId*numComp,numComp);
      float *from=((vtkFloatArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *to=((vtkDoubleArray *)toData)->WritePointer(toId*numComp,numComp);
      double *from=((vtkDoubleArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_ID_TYPE:
      {
      vtkIdType *to=((vtkIdTypeArray *)toData)->WritePointer(toId*numComp,numComp);
      vtkIdType *from=((vtkIdTypeArray *)fromData)->GetPointer(fromId*numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData, 
                                            vtkDataArray *toData, 
                                            vtkIdType toId, vtkIdList *ptIds, 
                                            float *weights)
{
  int numComp=fromData->GetNumberOfComponents(), i;
  vtkIdType j, numIds=ptIds->GetNumberOfIds();
  vtkIdType *ids=ptIds->GetPointer(0);
  vtkIdType idx=toId*numComp;
  double c;
  
  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from->GetValue(ids[j]*numComp+i);
          }
        to->InsertValue(idx+i, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *from=((vtkCharArray *)fromData)->GetPointer(0);
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      short *from=((vtkShortArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      int *from=((vtkIntArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      long *from=((vtkLongArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*(float)from[ids[j]*numComp+i];
          }
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*(float)from[ids[j]*numComp+i];
          }
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      float *from=((vtkFloatArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0.0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      double *from=((vtkDoubleArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0.0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (double) c;
        }
      }
      break;

    case VTK_ID_TYPE:
      {
      vtkIdType *to=((vtkIdTypeArray *)toData)->WritePointer(idx,numComp);
      vtkIdType *from=((vtkIdTypeArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*(float)from[ids[j]*numComp+i];
          }
        *to++ = (vtkIdType) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData,
                                            vtkDataArray *toData,
                                            vtkIdType toId, vtkIdType id1,
                                            vtkIdType id2, float t)
{
  int i, numComp=fromData->GetNumberOfComponents();
  vtkIdType idx=toId*numComp;
  vtkIdType idx1=id1*numComp, idx2=id2*numComp;
  float c;
  
  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        c = from->GetValue(idx1+i)+ t * (from->GetValue(idx2+i) - from->GetValue(idx1+i));
        to->InsertValue(idx+i, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      char *from=((vtkCharArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      short *from=((vtkShortArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      int *from=((vtkIntArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      long *from=((vtkLongArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = (float)from[idx1+i] + t * (float)(from[idx2+i] - from[idx1+i]);
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = (float)from[idx1+i] + t * (float)(from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      float *from=((vtkFloatArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = (1.0 - t) * from[idx1+i] + t * from[idx2+i];
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      double *from=((vtkDoubleArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = (1.0 - t) * from[idx1+i] + t * from[idx2+i];
        *to++ = (double) c;
        }
      }
      break;

    case VTK_ID_TYPE:
      {
      vtkIdType *to=((vtkIdTypeArray *)toData)->WritePointer(idx,numComp);
      vtkIdType *from=((vtkIdTypeArray *)fromData)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        c = (float)from[idx1+i] + t * (float)(from[idx2+i] - from[idx1+i]);
        *to++ = (vtkIdType) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData1,
                                            vtkDataArray *fromData2, 
                                            vtkDataArray *toData, vtkIdType id,
                                            float t)
{
  int i, numComp=fromData1->GetNumberOfComponents();
  vtkIdType idx=id*numComp, ii;
  float c;

  switch (fromData1->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from1=(vtkBitArray *)fromData1;
      vtkBitArray *from2=(vtkBitArray *)fromData2;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1->GetValue(ii)+ t * (from2->GetValue(ii) - from1->GetValue(ii));
        to->InsertValue(ii, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      char *from1=((vtkCharArray *)fromData1)->GetPointer(0);
      char *from2=((vtkCharArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      unsigned char *from1=((vtkUnsignedCharArray *)fromData1)->GetPointer(0);
      unsigned char *from2=((vtkUnsignedCharArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      short *from1=((vtkShortArray *)fromData1)->GetPointer(0);
      short *from2=((vtkShortArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      unsigned short *from1=((vtkUnsignedShortArray *)fromData1)->GetPointer(0);
      unsigned short *from2=((vtkUnsignedShortArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      int *from1=((vtkIntArray *)fromData1)->GetPointer(0);
      int *from2=((vtkIntArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      unsigned int *from1=((vtkUnsignedIntArray *)fromData1)->GetPointer(0);
      unsigned int *from2=((vtkUnsignedIntArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      long *from1=((vtkLongArray *)fromData1)->GetPointer(0);
      long *from2=((vtkLongArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (float)from1[ii] + t * (float)(from2[ii] - from1[ii]);
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      unsigned long *from1=((vtkUnsignedLongArray *)fromData1)->GetPointer(0);
      unsigned long *from2=((vtkUnsignedLongArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (float)from1[ii] + t * (float)(from2[ii] - from1[ii]);
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      float *from1=((vtkFloatArray *)fromData1)->GetPointer(0);
      float *from2=((vtkFloatArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (1.0 - t) * from1[ii] + t * from2[ii];
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      double *from1=((vtkDoubleArray *)fromData1)->GetPointer(0);
      double *from2=((vtkDoubleArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (1.0 - t) * from1[ii] + t * from2[ii];
        *to++ = (double) c;
        }
      }
      break;

    case VTK_ID_TYPE:
      {
      vtkIdType *to=((vtkIdTypeArray *)toData)->WritePointer(idx,numComp);
      vtkIdType *from1=((vtkIdTypeArray *)fromData1)->GetPointer(0);
      vtkIdType *from2=((vtkIdTypeArray *)fromData2)->GetPointer(0);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (float)from1[ii] + t * (float)(from2[ii] - from1[ii]);
        *to++ = (vtkIdType) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during interpolation!");
    }
}

int vtkDataSetAttributes::SetScalars(vtkDataArray* da) 
{ 
  return this->SetAttribute(da, SCALARS); 
}

void vtkDataSetAttributes::SetScalars(vtkScalars* scalars)
{ 
  this->SetAttributeData(scalars, SCALARS); 
}

int vtkDataSetAttributes::SetActiveScalars(const char* name)
{ 
  return this->SetActiveAttribute(name, SCALARS); 
}

int vtkDataSetAttributes::SetActiveAttribute(const char* name,
                                             int attributeType)
{
  int index; 
  this->GetArray(name, index);
  return this->SetActiveAttribute(index, attributeType);
}

vtkDataArray* vtkDataSetAttributes::GetActiveScalars() 
{ 
  return this->GetActiveAttribute(SCALARS); 
}

int vtkDataSetAttributes::SetVectors(vtkDataArray* da) 
{ 
return this->SetAttribute(da, VECTORS); 
}

void vtkDataSetAttributes::SetVectors(vtkVectors* vectors)
{ 
  this->SetAttributeData(vectors, VECTORS); 
}

int vtkDataSetAttributes::SetActiveVectors(const char* name)
{ 
  return this->SetActiveAttribute(name, VECTORS); 
}

vtkDataArray* vtkDataSetAttributes::GetActiveVectors() 
{ 
  return this->GetActiveAttribute(VECTORS); 
}

int vtkDataSetAttributes::SetNormals(vtkDataArray* da) 
{ 
  return this->SetAttribute(da, NORMALS); 
}

void vtkDataSetAttributes::SetNormals(vtkNormals* normals)
{ 
  this->SetAttributeData(normals, NORMALS); 
}

int vtkDataSetAttributes::SetActiveNormals(const char* name)
{ 
  return this->SetActiveAttribute(name, NORMALS); 
}

vtkDataArray* vtkDataSetAttributes::GetActiveNormals() 
{ 
  return this->GetActiveAttribute(NORMALS); 
}

int vtkDataSetAttributes::SetTCoords(vtkDataArray* da) 
{ 
  return this->SetAttribute(da, TCOORDS); 
}

void vtkDataSetAttributes::SetTCoords(vtkTCoords* tcoords)
{ 
  this->SetAttributeData(tcoords, TCOORDS); 
}

int vtkDataSetAttributes::SetActiveTCoords(const char* name)
{ 
  return this->SetActiveAttribute(name, TCOORDS); 
}
vtkDataArray* vtkDataSetAttributes::GetActiveTCoords() 
{ 
  return this->GetActiveAttribute(TCOORDS); 
}

int vtkDataSetAttributes::SetTensors(vtkDataArray* da) 
{ 
  return this->SetAttribute(da, TENSORS); 
}

void vtkDataSetAttributes::SetTensors(vtkTensors* Tensors)
{ 
  this->SetAttributeData(Tensors, TENSORS); 
}

int vtkDataSetAttributes::SetActiveTensors(const char* name)
{ 
  return this->SetActiveAttribute(name, TENSORS); 
}

vtkDataArray* vtkDataSetAttributes::GetActiveTensors() 
{ 
  return this->GetActiveAttribute(TENSORS); 
}

int vtkDataSetAttributes::SetActiveAttribute(int index, int attributeType)
{
  if ( (index >= 0) && (index < this->GetNumberOfArrays()))
    {
    if (!this->CheckNumberOfComponents(this->Data[index], attributeType))
      {
      vtkWarningMacro("Can not set attribute " 
		      << vtkDataSetAttributes::AttributeNames[attributeType]
		      << ". Incorrect number of components.");
      return -1;
      }
    this->AttributeIndices[attributeType] = index;

    // UnRegister the exiting vtkAttributeData and set it to NULL
    if (this->Attributes[attributeType])
      {
      this->Attributes[attributeType]->UnRegister(this);
      }
    this->Attributes[attributeType] = 0;
    this->Modified();
    return index;
    }
  else
    {
    return -1;
    }
}

    // Scalars set to NOLIMIT and 1024 (limit is arbitrary, so make it big.)
int vtkDataSetAttributes::NumberOfAttributeComponents[vtkDataSetAttributes::NUM_ATTRIBUTES]
= { 0,
    3,
    3,
    3,
    9};

int vtkDataSetAttributes::AttributeLimits[vtkDataSetAttributes::NUM_ATTRIBUTES]
= { NOLIMIT,
    EXACT,
    EXACT,
    MAX,
    EXACT };

int vtkDataSetAttributes::CheckNumberOfComponents(vtkDataArray* da,
						  int attributeType)
{
  int numComp = da->GetNumberOfComponents();
  
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

vtkDataArray* vtkDataSetAttributes::GetActiveAttribute(int attributeType)
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

// This method lets the user add an array and make it the current
// scalars, vectors etc... (this is determined by the attribute type
// which is an enum defined vtkDataSetAttributes)
int vtkDataSetAttributes::SetAttribute(vtkDataArray* da, int attributeType)
{
  if (da && !this->CheckNumberOfComponents(da, attributeType))
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
    if (this->GetArray(currentAttribute) == da)
      {
      return currentAttribute;
      }
    this->RemoveArray(currentAttribute);
    }

  if (da)
    {
    // Add the array
    currentAttribute = this->AddArray(da);
    this->AttributeIndices[attributeType] = currentAttribute;
    }
  else
    {
    this->AttributeIndices[attributeType] = -1; //attribute of this type doesn't exist
    }
  
  // UnRegister the exiting vtkAttributeData and set it to NULL
  if (this->Attributes[attributeType])
    {
    this->Attributes[attributeType]->UnRegister(this);
    }
  this->Attributes[attributeType] = 0;

  this->Modified();
  return this->AttributeIndices[attributeType];
}

// This method does two things: 1> Get the data array inside
// the attribute data and add it to the list of arrays and
// make it correspond to the attribute (i.e. scalars, vectors etc)
// given by attribute type, 2> keep a pointer to the attribute
// data in the pointer array Attributes[]
void vtkDataSetAttributes::SetAttributeData(vtkAttributeData* newAtt,
                                            int attributeType)
{
  if ( newAtt != this->Attributes[attributeType] )
    {
    if(newAtt)
      {
      this->SetAttribute(newAtt->GetData(), attributeType);
      this->Attributes[attributeType] = newAtt;
      if (newAtt)
        {
        newAtt->Register(this);
        }
      }
    else
      {
      this->SetAttribute(0, attributeType);
      }
    }
}

vtkAttributeData* vtkDataSetAttributes::GetAttributeData(int attributeType)
{
  if (this->Attributes[attributeType])
    {
    return this->Attributes[attributeType];
    }
  else
    {
    return 0;
    }
}

vtkScalars* vtkDataSetAttributes::GetScalars()
{
  vtkScalars* retVal=0;

  if (this->Attributes[SCALARS] || (this->AttributeIndices[SCALARS] == -1))
    {
    retVal = (vtkScalars*)this->GetAttributeData(SCALARS);
    }
  else
    {
    vtkScalars *s = vtkScalars::New();
    this->Attributes[SCALARS] = s;
    s->Register(this);
    s->Delete();
    this->Attributes[SCALARS]->SetData(this->GetActiveScalars());
    retVal = (vtkScalars*)this->Attributes[SCALARS];
    }

  return retVal;
}

vtkVectors* vtkDataSetAttributes::GetVectors()
{
  if (this->Attributes[VECTORS] || (this->AttributeIndices[VECTORS] == -1))
    {
    return (vtkVectors*)this->GetAttributeData(VECTORS);
    }
  else
    {
    this->Attributes[VECTORS] = vtkVectors::New();
    this->Attributes[VECTORS]->Register(this);
    this->Attributes[VECTORS]->Delete();
    this->Attributes[VECTORS]->SetData(this->GetActiveVectors());
    return (vtkVectors*)this->Attributes[VECTORS];
    }
}

vtkNormals* vtkDataSetAttributes::GetNormals()
{
  if (this->Attributes[NORMALS] || (this->AttributeIndices[NORMALS] == -1))
    {
    return (vtkNormals*)this->GetAttributeData(NORMALS);
    }
  else
    {
    this->Attributes[NORMALS] = vtkNormals::New();
    this->Attributes[NORMALS]->Register(this);
    this->Attributes[NORMALS]->Delete();
    this->Attributes[NORMALS]->SetData(this->GetActiveNormals());
    return (vtkNormals*)this->Attributes[NORMALS];
    }
}

vtkTCoords* vtkDataSetAttributes::GetTCoords()
{
  if (this->Attributes[TCOORDS] || (this->AttributeIndices[TCOORDS] == -1))
    {
    return (vtkTCoords*)this->GetAttributeData(TCOORDS);
    }
  else
    {
    this->Attributes[TCOORDS] = vtkTCoords::New();
    this->Attributes[TCOORDS]->Register(this);
    this->Attributes[TCOORDS]->Delete();
    this->Attributes[TCOORDS]->SetData(this->GetActiveTCoords());
    return (vtkTCoords*)this->Attributes[TCOORDS];
    }
}

vtkTensors* vtkDataSetAttributes::GetTensors()
{
  if (this->Attributes[TENSORS] || (this->AttributeIndices[TENSORS] == -1))
    {
    return (vtkTensors*)this->GetAttributeData(TENSORS);
    }
  else
    {
    this->Attributes[TENSORS] = vtkTensors::New();
    this->Attributes[TENSORS]->Register(this);
    this->Attributes[TENSORS]->Delete();
    this->Attributes[TENSORS]->SetData(this->GetActiveTensors());
    return (vtkTensors*)this->Attributes[TENSORS];
    }
}


void vtkDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFieldData::PrintSelf(os,indent);

  // Print the copy flags
  os << indent << "Copy Flags: ( ";
  for (int i=0; i<NUM_ATTRIBUTES; i++)
    {
    os << this->CopyAttributeFlags[i] << " ";
    }
  os << ")" << endl;
  
  // Now print the various attributes
  vtkDataArray* da;
  int attributeType;
  for (attributeType=0; attributeType<NUM_ATTRIBUTES; attributeType++)
    {
    os << indent << vtkDataSetAttributes::AttributeNames[attributeType]
       << ": ";
    if ( (da=this->GetActiveAttribute(attributeType)) )
      {
      cout << endl;
      da->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << "(none)" << endl;
      }
    }

}

void vtkDataSetAttributes::GetAttributeIndices(int* indexArray)
{
  for(int i=0; i<NUM_ATTRIBUTES; i++)
    {
    indexArray[i] = this->AttributeIndices[i];
    }
}

int vtkDataSetAttributes::IsArrayAnAttribute(int idx)
{
  for (int i=0; i<NUM_ATTRIBUTES; i++)
    {
    if ( idx == this->AttributeIndices[i] )
      {
      return i;
      }
    }
  return -1;
}

void vtkDataSetAttributes::SetCopyAttribute (int index, int value) 
{ 
  if (this->CopyAttributeFlags[ index ] != value) 
    { 
    this->CopyAttributeFlags[ index ] = value; 
    this->Modified(); 
    } 
} 

void vtkDataSetAttributes::SetCopyScalars(int i) 
{ 
  this->SetCopyAttribute(SCALARS, i); 
}
int vtkDataSetAttributes::GetCopyScalars() { 
  return this->CopyAttributeFlags[SCALARS]; 
}

void vtkDataSetAttributes::SetCopyVectors(int i) 
{ 
  this->SetCopyAttribute(VECTORS, i); 
}
int vtkDataSetAttributes::GetCopyVectors() 
{ 
  return this->CopyAttributeFlags[VECTORS]; 
}

void vtkDataSetAttributes::SetCopyNormals(int i) 
{ 
  this->SetCopyAttribute(NORMALS, i); 
}
int vtkDataSetAttributes::GetCopyNormals() 
{ 
  return this->CopyAttributeFlags[NORMALS]; 
}

void vtkDataSetAttributes::SetCopyTCoords(int i) 
{ 
  this->SetCopyAttribute(TCOORDS, i); 
}
int vtkDataSetAttributes::GetCopyTCoords() 
{ 
  return this->CopyAttributeFlags[TCOORDS]; 
}

void vtkDataSetAttributes::SetCopyTensors(int i) 
{ 
  this->SetCopyAttribute(TENSORS, i); 
}
int vtkDataSetAttributes::GetCopyTensors() { 
  return this->CopyAttributeFlags[TENSORS]; 
}

void vtkDataSetAttributes::RemoveArray(const char *name)
{
  int i;
  this->GetArray(name, i);
  this->RemoveArray(i);
}

void vtkDataSetAttributes::CopyAllocate(vtkDataSetAttributes::FieldList& list, 
                                        vtkIdType sze, vtkIdType ext)
{
  vtkDataArray* newDA=0;
  int i;

  // Get rid of any old stuff
  this->Initialize();

  // Allocate attributes if any
  for (i=0; i < list.NumberOfFields; i++)
    {
    if ( list.FieldIndices[i] >= 0 )
      {
      switch (list.FieldTypes[i])
        {
        case VTK_VOID:
          newDA = vtkVoidArray::New();
          break;
        case VTK_BIT:
          newDA = vtkBitArray::New();
          break;
        case VTK_CHAR:
          newDA = vtkCharArray::New();
          break;
        case VTK_UNSIGNED_CHAR:
          newDA = vtkUnsignedCharArray::New();
          break;
        case VTK_SHORT:
          newDA = vtkShortArray::New();
          break;
        case VTK_UNSIGNED_SHORT:
          newDA = vtkUnsignedShortArray::New();
          break;
        case VTK_INT:
          newDA = vtkIntArray::New();
          break;
        case VTK_UNSIGNED_INT:
          newDA = vtkUnsignedIntArray::New();
          break;
        case VTK_LONG:
          newDA = vtkLongArray::New();
          break;
        case VTK_UNSIGNED_LONG:
          newDA = vtkUnsignedLongArray::New();
          break;
        case VTK_FLOAT:
          newDA = vtkFloatArray::New();
          break;
        case VTK_DOUBLE:
          newDA = vtkDoubleArray::New();
          break;
        case VTK_ID_TYPE:
          newDA = vtkIdTypeArray::New();
          break;
        }

      newDA->SetName(list.Fields[i]);
      newDA->SetNumberOfComponents(list.FieldComponents[i]);

      if ( sze > 0 )
        {
        newDA->Allocate(sze,ext);
        }
      else
        {
        newDA->Allocate(list.NumberOfTuples,ext);
        }
      newDA->SetLookupTable(list.LUT[i]);

      // If attribute data, do something extra
      if ( i < NUM_ATTRIBUTES )
        {
        if ( this->CopyAttributeFlags[i] )
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
          list.FieldIndices[i] = this->AddArray(newDA);
          }
        else
          {
          list.FieldIndices[i] = -1;
          }
        }

      newDA->Delete(); //okay, reference counting
      }//data array defined
    }
}

// Description:
// A special form of CopyData() to be used with FieldLists. Use it when you are
// copying data from a set of vtkDataSetAttributes. Make sure that you have
// called the special form of CopyAllocate that accepts FieldLists.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes::FieldList& list, 
                                    vtkDataSetAttributes* fromDSA,
                                    int idx, vtkIdType fromId, vtkIdType toId)
{
  vtkDataArray *fromDA;
  vtkDataArray *toDA;
  
  for (int i=0; i < list.NumberOfFields; i++)
    {
    if ( list.FieldIndices[i] >= 0 )
      {
      toDA = this->GetArray(list.FieldIndices[i]);
      fromDA = fromDSA->GetArray(list.DSAIndices[idx][i]);
      this->CopyTuple(fromDA, toDA, fromId, toId);
      }
    }
}

// FieldList support ---------------------------------------------------------
// To perform intersection of attribute data, use IntializeFieldList() to grab
// an initial vtkDataSetAttributes. Then use IntersectFieldList() to add (and 
// intersect) additional vtkDataSetAttributes.
void vtkDataSetAttributes::FieldList::InitializeFieldList(vtkDataSetAttributes* dsa)
{
  int i, idx;

  this->ClearFields();

  // Allocate space for the arrays plus five attributes
  this->NumberOfFields = dsa->GetNumberOfArrays() + NUM_ATTRIBUTES;
  this->Fields = new char*[this->NumberOfFields];
  this->FieldTypes = new int [this->NumberOfFields];
  this->FieldComponents = new int [this->NumberOfFields];
  this->FieldIndices = new int [this->NumberOfFields];
  this->LUT = new vtkLookupTable* [this->NumberOfFields];
  for(i=0; i < this->NumberOfFields; i++)
    {
    this->Fields[i] = 0;
    this->FieldTypes[i] = -1;
    this->FieldComponents[i] = 0;
    this->FieldIndices[i] = -1;
    }
  this->CurrentInput = 0;
  this->NumberOfTuples = 0;
  
  //there may be no data hence dsa->Data
  for(i=0; dsa->Data && i < dsa->GetNumberOfArrays(); i++) 
    {
    if ( (idx=dsa->IsArrayAnAttribute(i)) >= 0 ) //it's an attribute
      {
      this->FieldIndices[idx] = idx;
      this->SetField(idx, dsa->Data[i]);
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

void vtkDataSetAttributes::FieldList::IntersectFieldList(vtkDataSetAttributes* dsa)
{
  int i;
  vtkDataArray *da;
  
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
      da = dsa->GetActiveAttribute(i);
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
      da = dsa->GetArray(this->Fields[i], index);
      if ((da) && (da->GetDataType() == this->FieldTypes[i]) &&
          (da->GetNumberOfComponents() == this->FieldComponents[i]))
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

int vtkDataSetAttributes::FieldList::IsAttributePresent(int attrType)
{
  return this->FieldIndices[attrType];
}

int vtkDataSetAttributes::FieldList::IsFieldPresent(const char *name)
{
  if ( !name )
    {
    return -1;
    }

  // First five slots are reserved for named attributes
  for (int i=0; i < this->NumberOfFields; i++)
    {
    if ( this->Fields[i] && !strcmp(this->Fields[i],name) )
      {
      return i;
      }
    }

  return -1;
}

vtkDataSetAttributes::FieldList::FieldList(int numInputs)
{
  this->Fields = 0;
  this->FieldTypes = 0;
  this->FieldComponents = 0;
  this->FieldIndices = 0;
  this->NumberOfFields = 0;
  this->LUT = 0;
  this->NumberOfDSAIndices = numInputs;
  this->DSAIndices = new int*[numInputs];
  for (int i=0; i<numInputs; i++)
    {
    this->DSAIndices[i] = 0;
    }
}

vtkDataSetAttributes::FieldList::~FieldList()
{
  this->ClearFields();
  delete [] this->DSAIndices;
  this->DSAIndices = 0;
}

void vtkDataSetAttributes::FieldList::ClearFields()
{
  if ( this->Fields )
    {
    for (int i=0; i<this->NumberOfFields; i++)
      {
      delete [] this->Fields[i];
      this->Fields[i] = 0;
      }
    }
  if ( this->DSAIndices )
    {
    for (int i=0; i<this->NumberOfDSAIndices; i++)
      {
      delete[] this->DSAIndices[i];
      this->DSAIndices[i] = 0;
      }
    }
  delete [] this->LUT;
  this->LUT = 0;
  delete [] this->Fields;
  this->Fields = 0;
  delete [] this->FieldTypes;
  this->FieldTypes = 0;
  delete [] this->FieldComponents;
  this->FieldComponents = 0;
  delete [] this->FieldIndices;
  this->FieldIndices = 0;
  
  this->NumberOfFields = 0;
  this->CurrentInput = 0;
}

void vtkDataSetAttributes::FieldList::SetField(int index, vtkDataArray *da)
{
  const char* name=da->GetName();
  int dataType=da->GetDataType();
  vtkLookupTable *lut=da->GetLookupTable();
  
  if ( this->Fields[index] )
    {
    delete [] this->Fields[index];
    this->Fields[index] = 0;
    }
  
  this->FieldTypes[index] = dataType;
  this->FieldComponents[index] = da->GetNumberOfComponents();
  this->LUT[index] = lut;
  if (name)
    {
    int len = strlen(name);
    if (len > 0)
      {
      this->Fields[index] = new char[len+1];
      strcpy(this->Fields[index], name);
      }
    }
  else
    {
    this->Fields[index] = 0;
    }
    
}

void vtkDataSetAttributes::FieldList::RemoveField(const char *name)
{
  if ( !name )
    {
    return;
    }

  for (int i=NUM_ATTRIBUTES; i < this->NumberOfFields; i++)
    {
    if ( this->Fields[i] && !strcmp(this->Fields[i],name) )
      {
      delete [] this->Fields[i];
      this->Fields[i] = 0;
      this->FieldIndices[i] = -1;
      return;
      }
    }
}
