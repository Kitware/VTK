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
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}




//----------------------------------------------------------------------------
// Description:
// This filter executes if it or a previous filter has been modified or
// if its data has been released and it is forced to update.
void vtkImageToStructuredPoints::ConditionalUpdate(int forced)
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
    || this->Region.GetMTime() > this->ExecuteTime 
    || (forced && this->Output->GetDataReleased());
  
  if (execute)
    {
    vtkDebugMacro(<< "ConditionalUpdate: Condition satisfied, forced = "
                  << forced << ", executeTime = " << this->ExecuteTime
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

//----------------------------------------------------------------------------
// Description:
// To keep the old update method working.
void vtkImageToStructuredPoints::Update()
{
  this->ConditionalUpdate(0);
}


void vtkImageToStructuredPoints::Execute()
{
  vtkImageRegion *region = new vtkImageRegion;
  int regionBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int dataBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
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

  // get the input region
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
  
  this->Input->UpdateRegion(region);
  if ( ! region->IsAllocated())
    {
    vtkErrorMacro(<< "Execute: Could not get region.");
    return;
    }

  // If data is not the same size as the region, we need to reformat.
  // Assume that relativeCoordinates == absoluteCoordinates.
  region->GetData()->GetBounds(dataBounds);
  if (dataBounds[0] != regionBounds[0] || dataBounds[1] != regionBounds[1] ||
      dataBounds[2] != regionBounds[2] || dataBounds[3] != regionBounds[3] ||
      dataBounds[4] != regionBounds[4] || dataBounds[5] != regionBounds[5] ||
      dataBounds[6] != regionBounds[6] || dataBounds[7] != regionBounds[7] ||
      dataBounds[8] != regionBounds[8] || dataBounds[9] != regionBounds[9])
    {
    region = this->ReformatRegion(region);
    }
  
  // setup the structured points with the scalars
  bounds = region->GetBounds3d();
  origin[0] = (float)(bounds[0]); 
  origin[1] = (float)(bounds[2]); 
  origin[2] = (float)(bounds[4]);
  dim[0] = bounds[1] - bounds[0] + 1;
  dim[1] = bounds[3] - bounds[2] + 1;
  dim[2] = bounds[5] - bounds[4] + 1;
  output->SetDimensions(dim);
  output->SetAspectRatio(aspectRatio);
  output->SetOrigin(origin);
  output->GetPointData()->SetScalars(region->GetData()->GetScalars());

  // delete the temporary structures
  //region->Delete();
}



// Copy the region data to scalar data
template <class T>
void vtkImageToStructuredPointsReformatRegion(vtkImageToStructuredPoints *self,
				      vtkImageRegion *inRegion, T *inPtr,
				      vtkImageRegion *outRegion, T *outPtr)
{
  int idx0, idx1, idx2;
  int min0, max0,  min1, max1,  min2, max2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1, *outPtr2;
  
  
  self = self;
  
  // set up variables to march through data
  inRegion->GetBounds3d(min0, max0, min1, max1, min2, max2);
  inRegion->GetIncrements3d(inInc0, inInc1, inInc2);
  outRegion->GetIncrements3d(outInc0, outInc1, outInc2);

  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	
	// Copy the pixel
	*outPtr0 = *inPtr0;
	
	inPtr0 += inInc0;
	outPtr0 += outInc0;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}




// Description:
// This method duplicates a region.  The intent is to have data
// with the same dimensions as the region.  The input region is deleted.
vtkImageRegion *
vtkImageToStructuredPoints::ReformatRegion(vtkImageRegion *inRegion)
{
  vtkImageRegion *outRegion = new vtkImageRegion;
  void *inPtr, *outPtr;

  outRegion->SetDataType(inRegion->GetDataType());
  outRegion->SetBounds3d(inRegion->GetBounds3d());
  outRegion->Allocate();
  inPtr = inRegion->GetVoidPointer3d();
  outPtr = outRegion->GetVoidPointer3d();
  
  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageToStructuredPointsReformatRegion(this,
				       inRegion, (float *)(inPtr), 
				       outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImageToStructuredPointsReformatRegion(this,
				       inRegion, (int *)(inPtr), 
				       outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageToStructuredPointsReformatRegion(this,
				       inRegion, (short *)(inPtr), 
				       outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageToStructuredPointsReformatRegion(this,
				       inRegion, (unsigned short *)(inPtr), 
				       outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageToStructuredPointsReformatRegion(this,
				       inRegion, (unsigned char *)(inPtr), 
				       outRegion, (unsigned char *)(outPtr));
      break;
    }
  inRegion->Delete();
  
  return outRegion;
}







