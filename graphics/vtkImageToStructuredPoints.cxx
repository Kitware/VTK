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
#include "vtkImageToStructuredPoints.h"



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  int idx;
  
  this->ScalarInput = NULL;
  this->VectorInput = NULL;
  this->WholeImage = 1;
  this->InputMemoryLimit = 50000000;  // A very big image indeed.
  this->SetSplitOrder(VTK_IMAGE_TIME_AXIS, VTK_IMAGE_Z_AXIS,
		      VTK_IMAGE_Y_AXIS, VTK_IMAGE_X_AXIS);
  this->Coordinate3 = 0;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = idx;
    }
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
  os << indent << "Coordinate3: " << this->Coordinate3 << "\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] << ", " 
     << this->Extent[4] << ", " << this->Extent[5] << ")\n";
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
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetAxes: " << num << "is to many axes.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  for (idx = 0; idx < num; ++idx)
    {
    this->Axes[idx] = axes[idx];
    }
  
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
  vtkImageRegion *region = new vtkImageRegion;
  int regionExtent[8];
  int *dataExtent;
  int *extent, dim[3];
  float aspectRatio[3] = {1.0, 1.0, 1.0};
  float origin[3];
  vtkStructuredPoints *output = this->GetOutput();
  
  // Set the coordinate system of the region
  region->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  
  // Fill in scalar information.
  if (this->ScalarInput)
    {
    this->ScalarInput->UpdateImageInformation(region);
    
    // Determine the extent of the region we are converting
    if (this->WholeImage)
      {
      region->GetImageExtent(4, regionExtent);
      if (this->Coordinate3<regionExtent[6]||this->Coordinate3>regionExtent[7])
	{
	vtkWarningMacro(<< "Coordinate3 = " << this->Coordinate3 
	<< ", is not in extent [" << regionExtent[6] << ", "
	<< regionExtent[7] << "]. Using value "<< regionExtent[6]);
	this->Coordinate3 = regionExtent[6];
	}
      regionExtent[6] = regionExtent[7] = this->Coordinate3;
      region->SetExtent(4, regionExtent);    
      }
    else
      {
      this->Extent[6] = this->Extent[7] = this->Coordinate3;
      region->SetExtent(4, this->Extent);
      region->GetExtent(4, regionExtent);
      }

    // Update the data for the region.
    if ( region->GetMemorySize() < this->InputMemoryLimit)
      {
      this->ScalarInput->UpdateRegion(region);
      }
    if ( ! region->AreScalarsAllocated())
      {
      // We need to stream
      region->SetScalarType(this->ScalarInput->GetScalarType());
      region->AllocateScalars();
      if ( ! region->AreScalarsAllocated())
	{
	vtkErrorMacro(<< "Execute: Could not allocate region.");
	return;
	}
      if ( ! this->SplitExecute(region))
	{
	vtkErrorMacro(<< "Execute: Streaming Failed.");
	return;
	}
      }
    
    // If data is not the same size as the region, we need to reformat.
    // Assume that relativeCoordinates == absoluteCoordinates.
    dataExtent = region->GetData()->GetExtent();
    if (dataExtent[0] != regionExtent[0] || dataExtent[1] != regionExtent[1] ||
	dataExtent[2] != regionExtent[2] || dataExtent[3] != regionExtent[3] ||
	dataExtent[4] != regionExtent[4] || dataExtent[5] != regionExtent[5] ||
	dataExtent[6] != regionExtent[6] || dataExtent[7] != regionExtent[7])
      {
      vtkImageRegion *temp = region;
      region = new vtkImageRegion;
      region->SetExtent(4, regionExtent);
      region->CopyRegionData(temp);
      temp->Delete();
      }
    
    // setup the structured points with the scalars
    extent = region->GetExtent();
    region->GetAspectRatio(3, aspectRatio);
    region->GetOrigin(3, origin);
    origin[0] += (float)(extent[0]) * aspectRatio[0]; 
    origin[1] += (float)(extent[2]) * aspectRatio[1]; 
    origin[2] += (float)(extent[4]) * aspectRatio[2];
    dim[0] = extent[1] - extent[0] + 1;
    dim[1] = extent[3] - extent[2] + 1;
    dim[2] = extent[5] - extent[4] + 1;
    output->SetDimensions(dim);
    output->SetAspectRatio(aspectRatio);
    output->SetOrigin(origin);
    output->GetPointData()->SetScalars(
		       region->GetData()->GetPointData()->GetScalars());
    
    // delete the temporary structures
    region->Delete();
    }
}




//----------------------------------------------------------------------------
// Description:
// This function is for streaming.  It divides a region into two pieces,
// An executes each one.  SplitOrder is used to determine which axis
// to split first.  The default values are:
// the TIME axis is tried first then ZYX and COMPONENT axes are tried.
int vtkImageToStructuredPoints::SplitExecute(vtkImageRegion *outRegion)
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
  inRegion = new vtkImageRegion;
  inRegion->SetAxes(this->NumberOfSplitAxes, this->SplitOrder);
  outRegion->GetExtent(splitExtent);

  splitAxisIdx = 0;
  while (splitExtent[splitAxisIdx * 2] == splitExtent[splitAxisIdx * 2 + 1])
    {
    ++splitAxisIdx;
    if (splitAxisIdx >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "SplitExecute: Cannot split one pixel.");
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
  if ( inRegion->GetMemorySize() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: extent: " <<
         splitExtent[0] << ", " <<
         splitExtent[1] << ", " <<
         splitExtent[2] << ", " <<
         splitExtent[3] << ", " <<
         splitExtent[4] << ", " <<
         splitExtent[5]);
    this->ScalarInput->UpdateRegion(inRegion);
    }
  if (inRegion->AreScalarsAllocated())
    {
    outRegion->CopyRegionData(inRegion);
    inRegion->ReleaseData();
    }
  else
    {
    if ( ! this->SplitExecute(outRegion))
      {
      vtkErrorMacro(<< "SplitExecute: Split failed.");
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
  if ( inRegion->GetMemorySize() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: extent are: " <<
         splitExtent[0] << ", " <<
         splitExtent[1] << ", " <<
         splitExtent[2] << ", " <<
         splitExtent[3] << ", " <<
         splitExtent[4] << ", " <<
         splitExtent[5]);
    this->ScalarInput->UpdateRegion(inRegion);
    }
  if (inRegion->AreScalarsAllocated())
    {
    outRegion->CopyRegionData(inRegion);
    inRegion->ReleaseData();
    }
  else
    {
    if ( ! this->SplitExecute(outRegion))
      {
      vtkErrorMacro(<< "SplitExecute: Split failed.");
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


  
