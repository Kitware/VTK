/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.cxx
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
#include "vtkGraymap.h"
#include "vtkAGraymap.h"
#include "vtkPixmap.h"
#include "vtkAPixmap.h"
#include "vtkImageCache.h"
#include "vtkImageToStructuredPoints.h"


//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  this->ScalarInput = NULL;
  this->VectorInput = NULL;
  this->WholeImage = 1;
  this->InputMemoryLimit = 500000;  // A very big image indeed (in kB).
  this->SetSplitOrder(VTK_IMAGE_TIME_AXIS, VTK_IMAGE_Z_AXIS,
		      VTK_IMAGE_Y_AXIS, VTK_IMAGE_X_AXIS);
  // For specifying extents in an intuitive way.
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "ScalarInput: (" << this->ScalarInput << ")\n";
  os << indent << "VectorInput: (" << this->VectorInput << ")\n";
  os << indent << "WholeImage: " << this->WholeImage << "\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] 
     << ", " << this->Extent[4] << ", " << this->Extent[5] 
     << ", " << this->Extent[6] << ", " << this->Extent[7] << ")\n";
  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit << "\n";
  os << indent << "SplitOrder: (";
  os << vtkImageAxisNameMacro(this->SplitOrder[0]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[1]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[2]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[3]) << ")\n";
}



//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetSplitOrder(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetSplitOrder: " << num << "is to many axes.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  this->NumberOfSplitAxes = num;
  for (idx = 0; idx < num; ++idx)
    {
    this->SplitOrder[idx] = axes[idx];
    }
  
}
//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::GetSplitOrder(int num, int *axes)
{
  int idx;

  if (num > this->NumberOfSplitAxes)
    {
    vtkWarningMacro(<< "GetSplitOrder: Only returning "
      << this->NumberOfSplitAxes << " of requested " << num << " axes");
    num = this->NumberOfSplitAxes;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->SplitOrder[idx];
    }
  
}

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetAxes(int num, int *axes)
{
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetAxes: " << num << "is to many axes.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  vtkImageRegion::CompleteUnspecifiedAxes(num, axes, this->Axes);
  // If Axes is going to survive, we need to permute the extent variable.
}
//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::GetAxes(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetAxes: Requesting too many axes");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetExtent: " << num << "is to large.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  this->WholeImageOff();
  for (idx = 0; idx < num*2; ++idx)
    {
    this->Extent[idx] = extent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::GetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetExtent: Requesting too large");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Update()
{
  unsigned long inputMTime = 0;
  unsigned temp;
  
  if (this->ScalarInput)
    {
    inputMTime = this->ScalarInput->GetPipelineMTime();
    }
  if (this->VectorInput)
    {
    temp = this->VectorInput->GetPipelineMTime();
    // max
    inputMTime = (temp > inputMTime) ? temp : inputMTime;
    }

  if ((inputMTime > this->ExecuteTime) || this->GetMTime() > this->ExecuteTime)
    {
    vtkDebugMacro(<< "Update: Condition satisfied, executeTime = " 
    << this->ExecuteTime
    << ", modifiedTime = " << this->GetMTime() 
    << ", input MTime = " << inputMTime
    << ", released = " << this->Output->GetDataReleased());
    
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Execute()
{
  vtkImageRegion *region = vtkImageRegion::New();
  vtkScalars *scalars = NULL;
  vtkVectors *vectors = NULL;
  int regionExtent[10];
  int *extent, dim[5];
  float spacing[4] = {0.0, 1.0, 1.0, 1.0};
  float origin[4];
  vtkStructuredPoints *output = this->GetOutput();
  
  
  // Get the size of the whole image.
  if (this->ScalarInput)
    {
    this->ScalarInput->UpdateImageInformation(region);
    }
  else if (this->VectorInput)
    {
    this->VectorInput->UpdateImageInformation(region);
    }
  else
    {
    vtkErrorMacro(<< "Execute: No Inputs!");
    region->Delete();
    return;
    }
  
  // Whole image or region of image?
  if (this->WholeImage)
    {
    region->SetAxes(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS,
		    VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
    region->GetImageExtent(5, regionExtent);
    // Take the lowest time sample.
    regionExtent[9] = regionExtent[8];
    // Components can olny have up to four values. (APixmap)
    if (regionExtent[1] - regionExtent[0] + 1 > 4)
      {
      regionExtent[1] = regionExtent[0] + 3;
      }
    region->SetExtent(4, regionExtent);    
    }
  else
    {
    // Copy extent from local ivar.
    region->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
    region->SetExtent(5, this->Extent);
    region->GetExtent(5, regionExtent);
    }

  // Get scalars and vectors
  if (this->ScalarInput)
    {
    scalars = this->ScalarExecute(region);
    }
  if (this->VectorInput)
    {
    // we don't want to delete the region yet, so use another.
    vectors = this->VectorExecute(region);
    }
  
  // setup the structured points
  region->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
  extent = region->GetExtent();
  region->GetSpacing(3, spacing);
  region->GetOrigin(3, origin);
  origin[0] += (float)(extent[0]) * spacing[0]; 
  origin[1] += (float)(extent[2]) * spacing[1]; 
  origin[2] += (float)(extent[4]) * spacing[2];
  dim[0] = extent[1] - extent[0] + 1;
  dim[1] = extent[3] - extent[2] + 1;
  dim[2] = extent[5] - extent[4] + 1;
  output->SetDimensions(dim);
  output->SetSpacing(spacing);
  output->SetOrigin(origin);
  output->GetPointData()->SetScalars(scalars);
  output->GetPointData()->SetVectors(vectors);
  
  // delete the temporary structures
  region->Delete();
  if (scalars)
    {
    scalars->Delete();
    }
  if (vectors)
    {
    vectors->Delete();
    }
}




//============================================================================
// Scalar stuff
//============================================================================


//----------------------------------------------------------------------------
// Returns scalars which must be deleted.
// Region has no data when method returns.
vtkScalars *vtkImageToStructuredPoints::ScalarExecute(vtkImageRegion *region)
{
  vtkScalars *scalars;
  int dim, min, max;
  int colorScalarsFlag = 0;
  
  // Update the data for the region.
  if ( region->GetExtentMemorySize() < this->InputMemoryLimit)
    {
    this->ScalarInput->Update(region);
    }
  if ( ! region->AreScalarsAllocated())
    {
    // We need to stream
    region->SetScalarType(this->ScalarInput->GetScalarType());

    // This streaming algorithm is suspect, it should be tested.
    // !!!!!!!!!!!!!!!!!!
    // It expects the source to fill in a previously allocated region.
    region->AllocateScalars();
    if ( ! region->AreScalarsAllocated())
      {
      vtkErrorMacro(<< "Execute: Could not allocate region.");
      return NULL;
      }
    if ( ! this->ScalarSplitExecute(region))
      {
      vtkErrorMacro(<< "Execute: Streaming Failed.");
      return NULL;
      }
    }
  
  // If we have more than one component then we want color
  // scalars. Also if we have one component and it is 
  // unsigned char then use color scalars.
  // The first axis is components.
  region->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  dim = max - min + 1;
  if ((dim > 1)|| (region->GetScalarType() == VTK_UNSIGNED_CHAR))
    {
    colorScalarsFlag = 1;
    }
  
  // If data is not the same size as the region, we need to reformat.
  // This may involve a copy.
  scalars = this->ReformatRegionData(region, colorScalarsFlag);

  // If required convert to color scalars with no copy
  if (colorScalarsFlag)
    {
    scalars = this->CreateColorScalars(scalars, dim);
    }
  
  region->ReleaseData();
  return scalars;
}

//----------------------------------------------------------------------------
// Description:
// This function is for streaming.  It divides a region into two pieces,
// An executes each one.  SplitOrder is used to determine which axis
// to split first.  The default values are:
// the TIME axis is tried first then ZYX and COMPONENT axes are tried.
int vtkImageToStructuredPoints::ScalarSplitExecute(vtkImageRegion *outRegion)
{
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  int saveExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int splitExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int splitAxisIdx;
  int min, max;
  vtkImageRegion *inRegion;
  
  // Save the  region state to restore later
  outRegion->GetExtent(saveExtent);
  outRegion->GetAxes(saveAxes);
  
  // change to split order coordinat system to make spliting easier.
  outRegion->SetAxes(this->NumberOfSplitAxes, this->SplitOrder);
  
  // Split output into two pieces and update separately.
  inRegion = vtkImageRegion::New();
  inRegion->SetAxes(this->NumberOfSplitAxes, this->SplitOrder);
  outRegion->GetExtent(splitExtent);

  splitAxisIdx = 0;
  while (splitExtent[splitAxisIdx * 2] == splitExtent[splitAxisIdx * 2 + 1])
    {
    ++splitAxisIdx;
    if (splitAxisIdx >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "ScalarSplitExecute: Cannot split one pixel.");
      inRegion->Delete();
      outRegion->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
      outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
      return 0;
      }
    }
  min = splitExtent[splitAxisIdx * 2];
  max = splitExtent[splitAxisIdx * 2 + 1];
  // lower part
  splitExtent[splitAxisIdx * 2 + 1] = (min + max) / 2;
  inRegion->SetExtent(splitExtent);
  outRegion->SetExtent(splitExtent);
  if ( inRegion->GetExtentMemorySize() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: extent: " <<
         splitExtent[0] << ", " << splitExtent[1] << ", " <<
         splitExtent[2] << ", " << splitExtent[3] << ", " <<
         splitExtent[4] << ", " << splitExtent[5]);
    this->ScalarInput->Update(inRegion);
    }
  if (inRegion->AreScalarsAllocated())
    {
    outRegion->CopyRegionData(inRegion);
    inRegion->ReleaseData();
    }
  else
    {
    if ( ! this->ScalarSplitExecute(outRegion))
      {
      vtkErrorMacro(<< "ScalarSplitExecute: Split failed.");
      inRegion->Delete();
      outRegion->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
      outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
      return 0;
      }
    }
  // upper part
  splitExtent[splitAxisIdx * 2] = splitExtent[splitAxisIdx * 2 + 1] + 1;
  splitExtent[splitAxisIdx * 2 + 1] = max;
  inRegion->SetExtent(splitExtent);
  outRegion->SetExtent(splitExtent);
  if ( inRegion->GetExtentMemorySize() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: extent are: " <<
         splitExtent[0] << ", " << splitExtent[1] << ", " <<
         splitExtent[2] << ", " << splitExtent[3] << ", " <<
         splitExtent[4] << ", " << splitExtent[5]);
    this->ScalarInput->Update(inRegion);
    }
  if (inRegion->AreScalarsAllocated())
    {
    outRegion->CopyRegionData(inRegion);
    inRegion->ReleaseData();
    }
  else
    {
    if ( ! this->ScalarSplitExecute(outRegion))
      {
      vtkErrorMacro(<< "ScalarSplitExecute: Split failed.");
      inRegion->Delete();
      outRegion->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
      outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
      return 0;
      }
    }
  
  // Clean up
  inRegion->Delete();
  outRegion->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
  outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
  return 1;
}




//----------------------------------------------------------------------------
// The user wants color scalars. Copy image region to color scalars.
vtkColorScalars *
vtkImageToStructuredPoints::CreateColorScalars(vtkScalars *scalars, int dim)
{
  vtkColorScalars *colorScalars;
  vtkUnsignedCharScalars *charScalars;

  // Get the unsigned char scalars to convert
  if (strcmp(scalars->GetClassName(), "vtkUnsignedCharScalars") != 0)
    {
    vtkErrorMacro("CreateColorScalars: ScalarType needs to be unsigned char");
    return NULL;
    }
  charScalars = (vtkUnsignedCharScalars *)(scalars);
  
  switch (dim)
    {
    case 1:
      colorScalars = vtkGraymap::New();
      break;
    case 2:
      colorScalars = vtkAGraymap::New();
      break;
    case 3:
      colorScalars = vtkPixmap::New();
      break;
    case 4:
      colorScalars = vtkAPixmap::New();
      break;
    default:
      vtkErrorMacro("Do not know how to convert dimension " << dim
		    << " to color.");
      return NULL;
    }
  
  // transfer the Array (reference counted).
  colorScalars->SetS(charScalars->GetS());
  charScalars->UnRegister(this);
  
  return colorScalars;
}

  


//============================================================================
// Vector stuff
//============================================================================



//----------------------------------------------------------------------------
// Templated function for copying image into vectors.
template <class T>
static void 
vtkImageToStructuredPointsCopyVectors(vtkImageToStructuredPoints *self, 
				      vtkImageRegion *region, T *inPtr,
				      vtkFloatVectors *vectors)

{
  int idx0, idx1, idx2, idx3;
  float *outPtr;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  int inInc0, inInc1, inInc2, inInc3;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int remainder;
  
  
  // Avoid compiler warnings
  self = self;
  
  // Get information to march through region.
  region->GetAxisExtent(VTK_IMAGE_X_AXIS, min0, max0);
  region->GetAxisExtent(VTK_IMAGE_Y_AXIS, min1, max1);
  region->GetAxisExtent(VTK_IMAGE_Z_AXIS, min2, max2);
  region->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min3, max3);
  region->GetAxisIncrements(VTK_IMAGE_X_AXIS, inInc0);
  region->GetAxisIncrements(VTK_IMAGE_Y_AXIS, inInc1);
  region->GetAxisIncrements(VTK_IMAGE_Z_AXIS, inInc2);
  region->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, inInc3);
  // Previous checks made sure 3 or less elements in components.
  // must fill all three vector elements.
  remainder = 2 - (max3 - min3);
  
  // Loop and copy
  outPtr = vectors->GetPointer(0);
  inPtr2 = inPtr;
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      inPtr0 = inPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{

	// Copy vector
	inPtr3 = inPtr0;
	for (idx3 = min3; idx3 <= max3; ++idx3)
	  {
	  *outPtr++ = *inPtr3;
	  inPtr3 += inInc3;
	  }
	// pad the remaining elements with 0.
	for (idx3 = 0; idx3 < remainder; ++idx3)
	  {
	  *outPtr++ = 0.0;
	  }

	inPtr0 += inInc0;
	}
      inPtr1 += inInc1;
      }
    inPtr2 += inInc2;
    }
}

  
//----------------------------------------------------------------------------
// This assumes that the vectors are stored in the component axis of the image.
// Returned vectors must be deleted.  On return region has no data.
vtkVectors *vtkImageToStructuredPoints::VectorExecute(vtkImageRegion *region)
{
  void *ptr;
  vtkFloatVectors *vectors;
  int min, max, numberOfVectors;
  int *extent;
  
  // compute how many vectors we have.
  region->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
  extent = region->GetExtent();
  numberOfVectors = (extent[1] - extent[0] + 1) 
    * (extent[3] - extent[2] + 1)
    * (extent[5] - extent[4] + 1);
  
  // Determine the extent of the component axis.
  this->VectorInput->UpdateImageInformation(region);
  region->GetAxisImageExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  // Vectors can have at most 3 dimensions.
  if (max - min > 2)
    {
    max = min + 2;
    }
  region->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  
  // No Streaming Yet
  
  // Update the data for the region.
  this->VectorInput->Update(region);

  // Copy the scalars into  vectors
  vectors = vtkFloatVectors::New();
  vectors->Allocate(numberOfVectors);
  ptr = region->GetScalarPointer();
  
  // WE SHOULD REALLY TRY TO COPY BY REFERENCE FIRST
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageToStructuredPointsCopyVectors(this, region, (float *)(ptr),
					    vectors);
      break;
    case VTK_INT:
      vtkImageToStructuredPointsCopyVectors(this, region, (int *)(ptr),
					    vectors);
      break;
    case VTK_SHORT:
      vtkImageToStructuredPointsCopyVectors(this, region, (short *)(ptr),
					    vectors);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageToStructuredPointsCopyVectors(this, region,
					    (unsigned short *)(ptr), vectors);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageToStructuredPointsCopyVectors(this, region,
					    (unsigned char *)(ptr), vectors);
      break;
    default:
      vtkErrorMacro(<< "ExecuteVectors::Cannot handle type");
      vectors->Delete();
      return NULL;
    }
  
  region->ReleaseData();
  
  return vectors;
}



//----------------------------------------------------------------------------
// Description: This method returns scalars with the correct type and format.
vtkScalars *
vtkImageToStructuredPoints::ReformatRegionData(vtkImageRegion *region, 
					       int colorScalarsFlag)
{
  int *regionExtent;
  int *dataExtent;
  int *dataAxes;
  int reformat = 0;
  vtkImageRegion *temp = NULL;
  vtkScalars *scalars;
  
  dataAxes = region->GetMemoryOrder();
  dataExtent = region->GetData()->GetExtent();
  // Since we are going to compare extent, axes should be the same.
  region->SetAxes(VTK_IMAGE_DIMENSIONS, dataAxes);
  regionExtent = region->GetExtent();
  
  // Determine if we need to copy the data
  if (colorScalarsFlag)
    {
    // Color scalars need to be unsigned char  
    if ((region->GetScalarType() != VTK_UNSIGNED_CHAR))
      {
      vtkDebugMacro("ReformatRegion: Wrong ScalarType " 
		    << vtkImageScalarTypeNameMacro(region->GetScalarType()));
      reformat = 1;
      }
    
    // Check to see if data order needs to be changed.
    // This is more tricky than it seems because axes can colapse.
    // This is naive way of checking
    if (dataAxes[0] != VTK_IMAGE_COMPONENT_AXIS || 
	dataAxes[1] != VTK_IMAGE_X_AXIS || dataAxes[2] != VTK_IMAGE_Y_AXIS ||
	dataAxes[3] != VTK_IMAGE_Z_AXIS)
      {
      vtkDebugMacro("ReformatRegion: Wrong MemoryOrder " 
		    << vtkImageAxisNameMacro(dataAxes[0]) << ", "
		    << vtkImageAxisNameMacro(dataAxes[1]) << ", "
		    << vtkImageAxisNameMacro(dataAxes[2]) << ", "
		    << vtkImageAxisNameMacro(dataAxes[3]));
      reformat = 1;
      }
    }
  else
    {
    // Check to see if data order needs to be changed.
    // This is more tricky than it seems because axes can colapse.
    // This is naive way of checking
    if (dataAxes[0] != VTK_IMAGE_X_AXIS || dataAxes[1] != VTK_IMAGE_Y_AXIS ||
	dataAxes[2] != VTK_IMAGE_Z_AXIS)
      {
      // Put a special case for the most common condition
      if ((dataAxes[0] == VTK_IMAGE_COMPONENT_AXIS &&
	   dataAxes[1] == VTK_IMAGE_X_AXIS &&
	   dataAxes[2] == VTK_IMAGE_Y_AXIS &&
	   dataAxes[3] == VTK_IMAGE_Z_AXIS) && 
	  (dataExtent[0] == dataExtent[1]))
	{
	// reformat does not change
	}
      else
	{
	vtkDebugMacro("ReformatRegion: Wrong MemoryOrder " 
		      << vtkImageAxisNameMacro(dataAxes[0]) << ", "
		      << vtkImageAxisNameMacro(dataAxes[1]) << ", "
		      << vtkImageAxisNameMacro(dataAxes[2]));
	reformat = 1;
	}
      }
    }
  
  // Since we are comparing all five axes, 
  // the order of the axis makes no difference.
  if (dataExtent[0] != regionExtent[0] || dataExtent[1] != regionExtent[1] ||
      dataExtent[2] != regionExtent[2] || dataExtent[3] != regionExtent[3] ||
      dataExtent[4] != regionExtent[4] || dataExtent[5] != regionExtent[5] ||
      dataExtent[6] != regionExtent[6] || dataExtent[7] != regionExtent[7] ||
      dataExtent[8] != regionExtent[8] || dataExtent[9] != regionExtent[9])
    {
    vtkDebugMacro("ReformatRegion: Wrong data Extent (" 
		  << dataExtent[0] << ", " << dataExtent[1] << ", "
		  << dataExtent[2] << ", " << dataExtent[3] << ", "
		  << dataExtent[4] << ", " << dataExtent[5] << ", "
		  << dataExtent[6] << ", " << dataExtent[7] << ")");
    reformat = 1;
    }
  
  if (reformat)
    {
    vtkDebugMacro("Reformatting region");
    if (colorScalarsFlag)
      {
      temp = vtkImageRegion::New();
      temp->SetScalarType(VTK_UNSIGNED_CHAR);
      temp->SetAxes(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS, 
		    VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
      region->SetAxes(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS, 
		    VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
      temp->SetMemoryOrder(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS, 
			 VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
      temp->SetExtent(4, region->GetExtent());
      temp->CopyRegionData(region);
      scalars = temp->GetData()->GetPointData()->GetScalars();
      scalars->Register(this);
      temp->Delete();
      }
    else
      {
      temp = vtkImageRegion::New();
      temp->SetScalarType(region->GetScalarType());
      temp->SetMemoryOrder(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS);
      temp->SetExtent(3, regionExtent);
      temp->CopyRegionData(region);
      scalars = temp->GetData()->GetPointData()->GetScalars();
      scalars->Register(this);
      temp->Delete();
      }
    }
  else
    { // no copy needed,
    scalars = region->GetData()->GetPointData()->GetScalars();
    scalars->Register(this);
    }
  
  return scalars;
}



