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
  this->Input = NULL;
  this->WholeImage = 1;
  this->InputMemoryLimit = 50000000;  // A very big image indeed.
  this->SetSplitOrder(VTK_IMAGE_TIME_AXIS, VTK_IMAGE_Z_AXIS,
		      VTK_IMAGE_Y_AXIS, VTK_IMAGE_X_AXIS);
  this->Coordinate3 = 0;
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  int *extent;
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "WholeImage: " << this->WholeImage << "\n";
  os << indent << "Coordinate3: " << this->Coordinate3 << "\n";
  extent = this->Region.GetExtent();
  os << indent << "Extent: (" << extent[0] << ", " << extent[1] << ", "
     << extent[2] << ", " << extent[3] << ", " 
     << extent[4] << ", " << extent[5] << ")\n";
  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit << "\n";
  os << indent << "SplitOrder: (";
  os << vtkImageAxisNameMacro(this->SplitOrder[0]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[1]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[2]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[3]) << ")\n";
}



//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetSplitOrder(int *axes, int num)
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
void vtkImageToStructuredPoints::GetSplitOrder(int *axes, int num)
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
void vtkImageToStructuredPoints::Update()
{
  int execute;
  
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  execute = this->Input->GetPipelineMTime() > this->ExecuteTime
    || this->GetMTime() > this->ExecuteTime 
    || this->Region.GetMTime() > this->ExecuteTime;
    
  if (execute)
    {
    vtkDebugMacro(<< "Update: Condition satisfied, executeTime = " 
                  << this->ExecuteTime
                  << ", modifiedTime = " << this->GetMTime() 
                  << ", input MTime = " << this->Input->GetPipelineMTime()
                  << ", released = " << this->Output->GetDataReleased());
    
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}


void vtkImageToStructuredPoints::Execute()
{
  vtkImageRegion *region = new vtkImageRegion;
  int regionExtent[8];
  int *dataExtent;
  int *extent, dim[3];
  float aspectRatio[3] = {1.0, 1.0, 1.0};
  float origin[3];
  vtkStructuredPoints *output = this->GetOutput();
  
  // error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Execute:Please specify an input!");
    return;
    }

  // Set the coordinate system of the region
  region->SetAxes(this->Region.GetAxes());
  
  // Fill in image information.
  this->Input->UpdateImageInformation(region);

  // Determine the extent of the region we are converting
  if (this->WholeImage)
    {
    region->GetImageExtent(regionExtent, 4);
    if (this->Coordinate3<regionExtent[6] || this->Coordinate3>regionExtent[7])
      {
      vtkWarningMacro(<< "Coordinate3 = " << this->Coordinate3 
              << ", is not in extent [" << regionExtent[6] << ", "
              << regionExtent[7] << "]. Using value "<< regionExtent[6]);
      this->Coordinate3 = regionExtent[6];
      }
    }
  else
    {
    this->Region.GetExtent(regionExtent, 4);
    }
  // make sure last axis has only one sample.
  regionExtent[6] = regionExtent[7] = this->Coordinate3;
  region->SetExtent(regionExtent, 4);

  // Update the data for the region.
  if ( region->GetMemorySize() < this->InputMemoryLimit)
    {
    this->Input->UpdateRegion(region);
    }
  if ( ! region->IsAllocated())
    {
    // We need to stream
    region->SetDataType(this->Input->GetDataType());
    region->Allocate();
    if ( ! region->IsAllocated())
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
    region->SetExtent(regionExtent, 4);
    region->CopyRegionData(temp);
    temp->Delete();
    }
  
  // setup the structured points with the scalars
  extent = region->GetExtent();
  region->GetAspectRatio(aspectRatio, 3);
  region->GetOrigin(origin, 3);
  origin[0] += (float)(extent[0]) * aspectRatio[0]; 
  origin[1] += (float)(extent[2]) * aspectRatio[1]; 
  origin[2] += (float)(extent[4]) * aspectRatio[2];
  dim[0] = extent[1] - extent[0] + 1;
  dim[1] = extent[3] - extent[2] + 1;
  dim[2] = extent[5] - extent[4] + 1;
  output->SetDimensions(dim);
  output->SetAspectRatio(aspectRatio);
  output->SetOrigin(origin);
  output->GetPointData()->SetScalars(region->GetData()->GetScalars());

  // delete the temporary structures
  region->Delete();
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
  int saveExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int splitExtent[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int splitAxisIdx;
  int min, max;
  vtkImageRegion *inRegion;
  
  // Save the  region state to restore later
  outRegion->GetExtent(saveExtent);
  outRegion->GetAxes(saveAxes);
  
  // change to split order coordinat system to make spliting easier.
  outRegion->SetAxes(this->SplitOrder);
  
  // Split output into two pieces and update separately.
  inRegion = new vtkImageRegion;
  inRegion->SetAxes(this->SplitOrder);
  outRegion->GetExtent(splitExtent);

  splitAxisIdx = 0;
  while (splitExtent[splitAxisIdx * 2] == splitExtent[splitAxisIdx * 2 + 1])
    {
    ++splitAxisIdx;
    if (splitAxisIdx >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "SplitExecute: Cannot split one pixel.");
      inRegion->Delete();
      outRegion->SetAxes(saveAxes);
      outRegion->SetExtent(saveExtent);
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
    this->Input->UpdateRegion(inRegion);
    }
  if (inRegion->IsAllocated())
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
      outRegion->SetAxes(saveAxes);
      outRegion->SetExtent(saveExtent);
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
    this->Input->UpdateRegion(inRegion);
    }
  if (inRegion->IsAllocated())
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
      outRegion->SetAxes(saveAxes);
      outRegion->SetExtent(saveExtent);
      return 0;
      }
    }
  
  // Clean up
  inRegion->Delete();
  outRegion->SetAxes(saveAxes);
  outRegion->SetExtent(saveExtent);
  return 1;
}

  
