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
#include "vtkImageRegion.h"
#include "vtkImageToStructuredPoints.h"


//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->Extent[idx*2] = -VTK_LARGE_INTEGER;
    this->Extent[idx*2+1] = VTK_LARGE_INTEGER;
    }
  // Take first time sample
  this->TimeSlice = -VTK_LARGE_INTEGER;
      
  this->Input = NULL;
  this->InputMemoryLimit = 500000;  // A very big image indeed (in kB).
  this->SetSplitOrder(VTK_IMAGE_TIME_AXIS, VTK_IMAGE_Z_AXIS,
		      VTK_IMAGE_Y_AXIS, VTK_IMAGE_X_AXIS);
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] 
     << ", " << this->Extent[4] << ", " << this->Extent[5] << ")\n";
  os << indent << "TimeSlice: " << this->TimeSlice << "\n";
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
  int idx, modified = 0;

  if (num > 4)
    {
    vtkWarningMacro(<< "SetSplitOrder: " << num << "is to many axes.");
    num = 4;
    }
  
  this->NumberOfSplitAxes = num;
  for (idx = 0; idx < num; ++idx)
    {
    if (this->SplitOrder[idx] != axes[idx])
      {
      this->SplitOrder[idx] = axes[idx];
      modified = 1;
      }
    }
  if (modified)
    {
    this->Modified();
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
void vtkImageToStructuredPoints::SetExtent(int num, int *extent)
{
  int idx, modified = 0;

  if (num > 3)
    {
    vtkWarningMacro(<< "SetExtent: " << num << "is to large.");
    num = 3;
    }
  for (idx = 0; idx < num*2; ++idx)
    {
    if (this->Extent[idx] != extent[idx])
      {
      this->Extent[idx] = extent[idx];
      modified = 1;
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::GetExtent(int num, int *extent)
{
  int idx;

  if (num > 3)
    {
    vtkWarningMacro(<< "GetExtent: Requesting too large");
    num = 3;
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
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Update: Input Not Set!");
    return;
    }
  
  inputMTime = this->Input->GetPipelineMTime();
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

  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Execute()
{
  int extent[8];
  vtkImageRegion *region = NULL;
  vtkScalars *scalars = NULL;
  float spacing[3];
  float origin[3];
  int dim[3];
  vtkStructuredPoints *output = (vtkStructuredPoints *)(this->Output);

  // Get the extent with z axis.
  this->GetExtent(3,extent);
  extent[6] = extent[7] = this->TimeSlice;

  // Fix the size of the cache (for streaming)
  this->Input->SetWholeUpdateExtent(extent);
  
  this->InputSplitUpdate(0);
  region = this->Input->GetScalarRegion();
  
  // region should actually have data!
  // Get the scalars
  scalars = this->GetScalarsFromRegion(region);
    
  // setup the structured points
  region->GetExtent(3, extent);
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
  
  // delete the temporary structures
  region->Delete();
  if ( scalars)
    {
    scalars->Delete();
    }
}



//----------------------------------------------------------------------------
// Description:
// This function is for streaming.  It divides a updateExtent into two pieces,
// and updates each one.  SplitOrder is used to determine which axis
// to split first.  The default values are:
// the TIME axis is tried first then ZYX are tried.
void vtkImageToStructuredPoints::InputSplitUpdate(int splitAxisIdx)
{
  int splitAxis, min, max, mid;
 
  if (splitAxisIdx >= 4)
    {
    vtkWarningMacro("InputSplitUpdate: Can split no more");
    this->Input->Update();
    return;
    }
  
  if (this->Input->GetUpdateExtentMemorySize() < this->InputMemoryLimit)
    {
    this->Input->Update();
    return;
    }
  

  // Get the split axis and its extent.
  splitAxis = this->SplitOrder[splitAxisIdx];
  this->Input->GetAxisUpdateExtent(splitAxis, min, max);
  if (min == max)
    {
    this->InputSplitUpdate(splitAxisIdx+1);
    return;
    }
  
  mid = (min + max) / 2;
  vtkDebugMacro ("Split " << vtkImageAxisNameMacro(splitAxis) << " ("
		 << min << "->" << mid << ") and (" << mid+1 << "->"
		 << max << ")");

  // first half
  this->Input->SetAxisUpdateExtent(splitAxis, min, mid);
  this->InputSplitUpdate(splitAxisIdx);

  // second half
  this->Input->SetAxisUpdateExtent(splitAxis, mid+1, max);
  this->InputSplitUpdate(splitAxisIdx);
  
  // restore original extent
  this->Input->SetAxisUpdateExtent(splitAxis, min, max);
}







//----------------------------------------------------------------------------
// Returns scalars which must be deleted.
// Region has no data when method returns.
vtkScalars *
vtkImageToStructuredPoints::GetScalarsFromRegion(vtkImageRegion *region)
{
  vtkScalars *scalars;
  int dim, min, max;
  int colorScalarsFlag = 0;
  
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

  


//----------------------------------------------------------------------------
// Description: This method returns scalars with the correct type and format.
vtkScalars *
vtkImageToStructuredPoints::ReformatRegionData(vtkImageRegion *region, 
					       int colorScalarsFlag)
{
  int *regionExtent;
  int *dataExtent;
  int reformat = 0;
  vtkImageRegion *temp = NULL;
  vtkScalars *scalars;
  
  dataExtent = region->GetData()->GetExtent();
  // Since we are going to compare extent, axes should be the same.
  region->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
		  VTK_IMAGE_TIME_AXIS, VTK_IMAGE_COMPONENT_AXIS);
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
    }
  
  // Make sure the datas extent is not too big.
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
      temp->SetExtent(4, region->GetExtent());
      temp->CopyRegionData(region);
      scalars = temp->GetData()->GetScalars();
      scalars->Register(this);
      temp->Delete();
      }
    else
      {
      temp = vtkImageRegion::New();
      temp->SetScalarType(region->GetScalarType());
      temp->SetExtent(3, regionExtent);
      temp->CopyRegionData(region);
      scalars = temp->GetData()->GetScalars();
      scalars->Register(this);
      temp->Delete();
      }
    }
  else
    { // no copy needed,
    scalars = region->GetData()->GetScalars();
    scalars->Register(this);
    }
  
  return scalars;
}



