/*=========================================================================


  Module:    vtkImageData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkImageData.h"
#include "vtkScalars.h"


//----------------------------------------------------------------------------
// Construct an instance of vtkImageData with no data.
vtkImageData::vtkImageData()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = 0;
    this->Extent[idx*2] = 0;
    this->Extent[idx*2 + 1] = 0;
    }
  
  this->ScalarType = VTK_VOID;
  this->NumberOfScalarComponents = 0; 
}


//----------------------------------------------------------------------------
void vtkImageData::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkStructuredPoints::PrintSelf(os,indent);

  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";

  os << indent << "Increments: (" << this->Increments[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->Increments[idx];
    }
  os << ")\n";

  os << indent << "NumberOfScalarComponents: " << 
    this->NumberOfScalarComponents << endl;
  os << indent << "ScalarType: " << this->ScalarType << endl;
  
}


//----------------------------------------------------------------------------
void vtkImageData::SetNumberOfScalarComponents(int num)
{
  int modified = 0;
  
  if (this->NumberOfScalarComponents != num)
    {
    this->NumberOfScalarComponents = num;
    modified = 1;
    }
  
  if (modified)
    {
    this->Modified();
    this->ComputeIncrements();
    }
}

void vtkImageData::GetExtent(int &x1, int &x2, int &y1, int &y2, 
			     int &z1, int &z2)
{
  int *ext;
  ext = this->GetExtent();
  x1 = ext[0];
  x2 = ext[1];
  y1 = ext[2];
  y2 = ext[3];
  z1 = ext[4];
  z2 = ext[5];
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
  this->SetExtent(ext);
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int *extent)
{
  int idx, modified = 0;
  vtkDebugMacro(<< "SetExtent: ...");
  
  // Copy the input
  for (idx = 0; idx < 3; ++idx)
    {
    if (this->Extent[idx*2] != extent[idx*2])
      {
      this->Extent[idx*2] = extent[idx*2];
      modified = 1;
      }
    if (this->Extent[idx*2 + 1] != extent[idx*2 + 1])
      {
      this->Extent[idx*2 + 1] = extent[idx*2 + 1];
      modified = 1;
      }
    }

  if (modified)
    {
    this->SetDimensions(this->Extent[1] - this->Extent[0] + 1,
			this->Extent[3] - this->Extent[2] + 1,
			this->Extent[5] - this->Extent[4] + 1);
    this->Modified();
    this->ComputeIncrements();
    // if the extent has changed and we have scalars we should free them
    // because they are invalid
    this->PointData->Initialize();
    }
}


//----------------------------------------------------------------------------
void vtkImageData::GetContinuousIncrements(int extent[6], int &incX,
					   int &incY, int &incZ)
{
  int e0, e1, e2, e3;
  
  incX = 0;

  e0 = extent[0];
  if (e0 < this->Extent[0])
    {
    e0 = this->Extent[0];
    }
  e1 = extent[1];
  if (e1 > this->Extent[1])
    {
    e1 = this->Extent[1];
    }
  e2 = extent[2];
  if (e2 < this->Extent[2])
    {
    e2 = this->Extent[2];
    }
  e3 = extent[3];
  if (e3 > this->Extent[3])
    {
    e3 = this->Extent[3];
    }
  
  incY = this->Increments[1] - (e1 - e0 + 1)*this->Increments[0];
  incZ = this->Increments[2] - (e3 - e2 + 1)*this->Increments[1];
}


//----------------------------------------------------------------------------
// This method computes the increments from the MemoryOrder and the extent.
void vtkImageData::ComputeIncrements()
{
  int idx;
  int inc = this->NumberOfScalarComponents;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (this->Extent[idx*2+1] - this->Extent[idx*2] + 1);
    }
}




//----------------------------------------------------------------------------
float vtkImageData::GetScalarComponentAsFloat(int x, int y, int z, int comp)
{
  void *ptr;
  
  if (comp >= this->NumberOfScalarComponents || comp < 0)
    {
    vtkErrorMacro("Bad component index " << comp);
    return 0.0;
    }
  
  ptr = this->GetScalarPointer(x, y, z);
  
  if (ptr == NULL)
    {
    // error message will be generated by get scalar pointer
    return 0.0;
    }
  
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      return *(((float *)ptr) + comp);
    case VTK_INT:
      return (float)(*(((int *)ptr) + comp));
    case VTK_SHORT:
      return (float)(*(((short *)ptr) + comp));
    case VTK_UNSIGNED_SHORT:
      return (float)(*(((unsigned short *)ptr) + comp));
    case VTK_UNSIGNED_CHAR:
      return (float)(*(((unsigned char *)ptr) + comp));
    }

  vtkErrorMacro("Unknown Scalar type");
  return 0.0;
}


//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int x, int y, int z)
{
  int tmp[3];
  tmp[0] = x;
  tmp[1] = y;
  tmp[2] = z;
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointerForExtent(int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int coordinates[3])
{
  vtkScalars *scalars;
  int idx;
    
  // Make sure the scalars have been allocated.
  scalars = this->PointData->GetScalars();
  if (scalars == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    scalars = this->PointData->GetScalars();
    }
  
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < 3; ++idx)
    {
    if (coordinates[idx] < this->Extent[idx*2] ||
	coordinates[idx] > this->Extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinates[0] << ", " 
      << coordinates[1] << ", "
      << coordinates[2] << ") not in memory.\n Current extent= ("
      << this->Extent[0] << ", " << this->Extent[1] << ", "
      << this->Extent[2] << ", " << this->Extent[3] << ", "
      << this->Extent[4] << ", " << this->Extent[5] << ")");
      return NULL;
      }
    }
  
  // compute the index of the vector.
  idx = ((coordinates[0] - this->Extent[0]) * this->Increments[0]
	 + (coordinates[1] - this->Extent[2]) * this->Increments[1]
	 + (coordinates[2] - this->Extent[4]) * this->Increments[2]);
  
  return scalars->GetVoidPointer(idx);
}


//----------------------------------------------------------------------------
// This Method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetScalarPointer()
{
  if (this->PointData->GetScalars() == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    }
  return this->PointData->GetScalars()->GetVoidPointer(0);
}

int vtkImageData::GetScalarType()
{
  vtkScalars *tmp;
  
  // if we have scalars make sure the type matches our ivar
  tmp = this->GetPointData()->GetScalars();
  if (tmp && tmp->GetDataType() != this->ScalarType)
    {
    vtkWarningMacro("ScalarType does not match current scalars!");
    }
  
  return this->ScalarType;
}

void vtkImageData::SetScalarType(int t)
{
  vtkScalars *tmp;

  // if we have scalars make sure they match
  tmp = this->GetPointData()->GetScalars();
  if (tmp && tmp->GetDataType() != t)
    {
    // free old scalars
    this->PointData->Initialize();
    }
  
  if (t != this->ScalarType)
    {
    this->Modified();
    this->ScalarType = t;
    vtkDebugMacro("Setting ScalarType to " << t << " !");
    }
}


//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars()
{
  vtkScalars *scalars;
  
  // if the scalar type has not been set then we have a problem
  if (this->ScalarType == VTK_VOID)
    {
    vtkWarningMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
    }
  
  // if we currently have scalars then just adjust the size
  if (this->PointData->GetScalars()) 
    {
    this->PointData->GetScalars()->SetNumberOfComponents(this->NumberOfScalarComponents);
    this->PointData->GetScalars()->
      SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
			 (this->Extent[3] - this->Extent[2] + 1)*
			 (this->Extent[5] - this->Extent[4] + 1));
    return;
    }
  
  // otherwise delete the old data (if any) 
  if (this->PointData->GetScalars())
    {
    this->PointData->SetScalars(NULL);
    }
  
  // allocate the new scalars
  scalars = vtkScalars::New();
  scalars->SetDataType(this->ScalarType);
  scalars->SetNumberOfComponents(this->NumberOfScalarComponents);
  this->PointData->SetScalars(scalars);
  scalars->Delete();
  
  // allocate enough memory
  this->PointData->GetScalars()->
    SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
		       (this->Extent[3] - this->Extent[2] + 1)*
		       (this->Extent[5] - this->Extent[4] + 1));
}


int vtkImageData::GetScalarSize()
{
  // allocate the new scalars
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      return sizeof(float);
    case VTK_INT:
      return sizeof(int);
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      return 2;
    case VTK_UNSIGNED_CHAR:
      return 1;
    }
  
  return 1;
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageDataCastExecute(vtkImageData *inData, IT *inPtr,
				    vtkImageData *outData, OT *outPtr,
				    int outExt[6])
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	*outPtr = (OT)(*inPtr);
	outPtr++;
	inPtr++;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}



//----------------------------------------------------------------------------
template <class T>
static void vtkImageDataCastExecute(vtkImageData *inData, T *inPtr,
				    vtkImageData *outData, int outExt[6])
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  switch (outData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageDataCastExecute(inData, (T *)(inPtr), 
			      outData, (float *)(outPtr),outExt);
      break;
    case VTK_INT:
      vtkImageDataCastExecute(inData, (T *)(inPtr), 
			      outData, (int *)(outPtr),outExt); 
      break;
    case VTK_SHORT:
      vtkImageDataCastExecute(inData, (T *)(inPtr), 
			      outData, (short *)(outPtr),outExt);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDataCastExecute(inData, (T *)(inPtr), 
			      outData, (unsigned short *)(outPtr),outExt); 
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDataCastExecute(inData, (T *)(inPtr), 
			      outData, (unsigned char *)(outPtr),outExt); 
      break;
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageData::CopyAndCastFrom(vtkImageData *inData, int extent[6])
{
  void *inPtr = inData->GetScalarPointerForExtent(extent);
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageDataCastExecute(inData, (float *)(inPtr), 
			      this, extent);
      break;
    case VTK_INT:
      vtkImageDataCastExecute(inData, (int *)(inPtr), 
			      this, extent);
      break;
    case VTK_SHORT:
      vtkImageDataCastExecute(inData, (short *)(inPtr), 
			      this, extent);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDataCastExecute(inData, (unsigned short *)(inPtr), 
			      this, extent);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDataCastExecute(inData, (unsigned char *)(inPtr), 
			      this, extent);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMin()
{
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      return (double)(VTK_FLOAT_MIN);
    case VTK_INT:
      return (double)(VTK_INT_MIN);
    case VTK_SHORT:
      return (double)(VTK_SHORT_MIN);
    case VTK_UNSIGNED_SHORT:
      return (double)(0.0);
    case VTK_UNSIGNED_CHAR:
      return (double)(0.0);
    default:
      vtkErrorMacro("Cannot handle scalar type " << this->ScalarType);
      return 0.0;
    }
}


//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMax()
{
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      return (double)(VTK_FLOAT_MAX);
    case VTK_INT:
      return (double)(VTK_INT_MAX);
    case VTK_SHORT:
      return (double)(VTK_SHORT_MAX);
    case VTK_UNSIGNED_SHORT:
      return (double)(VTK_UNSIGNED_SHORT_MAX);
    case VTK_UNSIGNED_CHAR:
      return (double)(VTK_UNSIGNED_CHAR_MAX);
    default:
      vtkErrorMacro("Cannot handle scalar type " << this->ScalarType);
      return 0.0;
    }
}

