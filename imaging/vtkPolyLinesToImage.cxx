/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLinesToImage.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include <string.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkPolyLinesToImage.h"
#include "vtkColorScalars.h"

//----------------------------------------------------------------------------
vtkPolyLinesToImage::vtkPolyLinesToImage()
{
  this->Input = NULL;
  this->Paint = new vtkImageCanvasSource2D;

  // invalid
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = -1;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = -1;
  this->WholeExtent[4] = this->WholeExtent[5] = 0;
  this->WholeExtent[6] = this->WholeExtent[7] = 0;
  
  this->Spacing[0] = 1.0;
  this->Spacing[1] = 1.0;
  this->Spacing[2] = 1.0;
  this->Spacing[3] = 1.0;
  
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
  this->Origin[3] = 0.0;
  
  this->SetOutputScalarType(VTK_SHORT);
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}

//----------------------------------------------------------------------------
vtkPolyLinesToImage::~vtkPolyLinesToImage()
{
  this->Paint->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyLinesToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkPolyLinesToImage::SetWholeExtent(int min0,int max0, int min1,int max1)
{
  this->WholeExtent[0] = min0;
  this->WholeExtent[1] = max0;
  this->WholeExtent[2] = min1;
  this->WholeExtent[3] = max1;
}


  
  
  
  
//----------------------------------------------------------------------------
void vtkPolyLinesToImage::UpdateInput()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...");
    return;
    }

  // This will cause an update if the pipeline has been changed.
  this->Input->Update();
  
  // If the input has been released.  Force it to update.
  if ( this->Input->GetDataReleased() )
    {
    this->Input->ForceUpdate();
    }

}


//----------------------------------------------------------------------------
void vtkPolyLinesToImage::Update()
{
  // Make sure input is up to date
  this->UpdateInput();
  
  this->vtkImageSource::Update();
  
  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkPolyLinesToImage::UpdateImageInformation()
{
  // Make sure input is up to date
  this->UpdateInput();
  
  // Make sure image information is update
  this->ExecuteImageInformation();
  
  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}


//----------------------------------------------------------------------------
unsigned long vtkPolyLinesToImage::GetPipelineMTime()
{
  unsigned long time, temp;
  
  time = this->GetMTime();

  if ( this->Input )
    {
    // This will cause an update if the pipeline has been changed.
    this->Input->Update();
    temp = this->Input->GetMTime();
    if (temp > time)
      time = temp;
    }
  
  return time;
}




//----------------------------------------------------------------------------
void vtkPolyLinesToImage::Execute(vtkImageRegion *region)
{
  int idx, idx2;
  vtkPolyData *input = (vtkPolyData *)this->Input;
  vtkPoints *pts = input->GetPoints(); 
  int numLines = input->GetNumberOfLines();
  vtkCellArray *lines = input->GetLines();
  int numCellPts;
  int *cellPts;
  float *pt1, *pt2;
  
  region->Fill(255.0);
  this->Paint->SetImageRegion(region);
  this->Paint->SetDrawColor(0.0); 

  lines->InitTraversal();
  for (idx = 0; idx < numLines; ++idx)
    {
    lines->GetNextCell(numCellPts, cellPts);
    if (numCellPts > 1)
      {
      pt1 = pts->GetPoint(cellPts[0]);
      for (idx2 = 1; idx2 < numCellPts; ++idx2)
	{
	// Get the second point defining a line segment
	pt2 = pts->GetPoint(cellPts[idx2]);
	// convert into image pixel coordinates and draw
	this->Paint->DrawSegment(
			 (int)((pt1[0]-this->Origin[0])/this->Spacing[0]), 
			 (int)((pt1[1]-this->Origin[1])/this->Spacing[1]), 
			 (int)((pt2[0]-this->Origin[0])/this->Spacing[0]), 
			 (int)((pt2[1]-this->Origin[1])/this->Spacing[1]));
	pt1 = pt2;
	}
      }
    }
}


//----------------------------------------------------------------------------
void vtkPolyLinesToImage::ExecuteImageInformation()
{
  // If extent has not been set, compute a default
  if (this->WholeExtent[0] > this->WholeExtent[1])
    {
    float *bounds = this->Input->GetBounds();
    this->WholeExtent[0] = (int)((bounds[0]-this->Origin[0])/this->Spacing[0]);
    this->WholeExtent[1] = (int)((bounds[1]-this->Origin[0])/this->Spacing[0]);
    this->WholeExtent[2] = (int)((bounds[2]-this->Origin[1])/this->Spacing[1]);
    this->WholeExtent[3] = (int)((bounds[3]-this->Origin[1])/this->Spacing[1]);
    }
  
  this->Output->SetWholeExtent(this->WholeExtent);
  this->Output->SetSpacing(this->Spacing);
  this->Output->SetOrigin(this->Origin);
}











