/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
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
  int regionBounds[8];
  int *dataBounds;
  int *bounds, dim[3];
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

  // Determine the bounds of the region we are converting
  if (this->WholeImage)
    {
    region->GetImageBounds3d(regionBounds);
    }
  else
    {
    this->Region.GetBounds3d(regionBounds);
    }
  regionBounds[6] = regionBounds[7] = this->Region.GetDefaultCoordinate3();
  region->SetBounds4d(regionBounds);

  // Update the data for the region.
  if ( region->GetVolume() < this->InputMemoryLimit)
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
  dataBounds = region->GetData()->GetBounds();
  if (dataBounds[0] != regionBounds[0] || dataBounds[1] != regionBounds[1] ||
      dataBounds[2] != regionBounds[2] || dataBounds[3] != regionBounds[3] ||
      dataBounds[4] != regionBounds[4] || dataBounds[5] != regionBounds[5] ||
      dataBounds[6] != regionBounds[6] || dataBounds[7] != regionBounds[7])
    {
    vtkImageRegion *temp = region;
    region = new vtkImageRegion;
    region->SetBounds4d(regionBounds);
    region->CopyRegionData(temp);
    temp->Delete();
    }
  
  // setup the structured points with the scalars
  bounds = region->GetBounds3d();
  region->GetAspectRatio3d(aspectRatio);
  origin[0] = (float)(bounds[0]) * aspectRatio[0]; 
  origin[1] = (float)(bounds[2]) * aspectRatio[1]; 
  origin[2] = (float)(bounds[4]) * aspectRatio[2];
  dim[0] = bounds[1] - bounds[0] + 1;
  dim[1] = bounds[3] - bounds[2] + 1;
  dim[2] = bounds[5] - bounds[4] + 1;
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
// An executes each one.  The spliting logic is hardwired.  First
// the TIME axis is split then ZYX and COMPONENT axes are split.
int vtkImageToStructuredPoints::SplitExecute(vtkImageRegion *outRegion)
{
  int saveBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int splitBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int splitAxisIdx = 0;
  int splitAxis;
  int *splitAxes;
  int min, max;
  vtkImageRegion *inRegion;
  
  // Save the  region state to restore later
  outRegion->GetBounds(saveBounds);
  
  // Split output into two pieces and update separately.
  inRegion = new vtkImageRegion;
  splitAxes = this->SplitOrder.GetAxes();
  vtkDebugMacro ("Split order is: " << splitAxes[0] << ", " 
				    << splitAxes[1] << ", " 
				    << splitAxes[2] << ", " 
				    << splitAxes[3]);
  outRegion->GetBounds(splitBounds);
  vtkDebugMacro (<<"Initial split region bounds are: " <<
         splitBounds[0] << ", " <<
         splitBounds[1] << ", " <<
         splitBounds[2] << ", " <<
         splitBounds[3] << ", " <<
         splitBounds[4] << ", " <<
         splitBounds[5] << ", " <<
         splitBounds[6] << ", " <<
         splitBounds[7]);

  splitAxis = splitAxes[splitAxisIdx];
  while (splitBounds[splitAxis * 2] == splitBounds[splitAxis * 2 + 1])
    {
    ++splitAxisIdx;
    if (splitAxisIdx >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "SplitExecute: Cannot split one pixel.");
      return 0;
      }
    splitAxis = splitAxes[splitAxisIdx];
    }
  min = splitBounds[splitAxis * 2];
  max = splitBounds[splitAxis * 2 + 1];
  // lower part
  splitBounds[splitAxis * 2 + 1] = (min + max) / 2;
  inRegion->SetBounds(splitBounds);
  outRegion->SetBounds(splitBounds);
  if ( inRegion->GetVolume() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: bounds are: " <<
         splitBounds[0] << ", " <<
         splitBounds[1] << ", " <<
         splitBounds[2] << ", " <<
         splitBounds[3] << ", " <<
         splitBounds[4] << ", " <<
         splitBounds[5]);
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
      return 0;
      }
    }
  // upper part
  splitBounds[splitAxis * 2] = splitBounds[splitAxis * 2 + 1] + 1;
  splitBounds[splitAxis * 2 + 1] = max;
  inRegion->SetBounds(splitBounds);
  outRegion->SetBounds(splitBounds);
  if ( inRegion->GetVolume() < this->InputMemoryLimit)
    {
    vtkDebugMacro (<<"Updating Split Region: bounds are: " <<
         splitBounds[0] << ", " <<
         splitBounds[1] << ", " <<
         splitBounds[2] << ", " <<
         splitBounds[3] << ", " <<
         splitBounds[4] << ", " <<
         splitBounds[5]);
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
      return 0;
      }
    }
  
  // Clean up
  inRegion->Delete();
  outRegion->SetBounds(saveBounds);
}

  
