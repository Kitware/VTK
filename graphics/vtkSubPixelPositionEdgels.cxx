/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkSubPixelPositionEdgels.h"
#include "vtkImageRegion.h"

vtkSubPixelPositionEdgels::vtkSubPixelPositionEdgels()
{
  this->Gradient = NULL;
}

void vtkSubPixelPositionEdgels::Execute()
{
  vtkImageRegion *region = new vtkImageRegion;
  int regionExtent[8];
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int numPts=input->GetNumberOfPoints();
  vtkFloatPoints *newPts;
  vtkPoints *inPts;
  vtkCellArray *inLines=input->GetLines();
  int nptsin, *idsin;
  vtkPolyData *output = this->GetOutput();
  int j;
  float result[3];
  float *pnt;
  
  vtkDebugMacro(<<"SubPixelPositioning Edgels");

  if (numPts < 1 || (inPts=input->GetPoints()) == NULL)
    {
    vtkErrorMacro(<<"No data to fit!");
    return;
    }

  newPts = new vtkFloatPoints;

  // Fill in image information.
  this->Gradient->UpdateImageInformation(region);
  region->SetAxes4d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, 
		    VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_Z_AXIS);
  
  // get the input region
  region->GetImageExtent4d(regionExtent);
  region->SetExtent4d(regionExtent);

  this->Gradient->UpdateRegion(region);
  if ( ! region->IsAllocated())
    {
    vtkErrorMacro(<< "Execute: Could not get region.");
    return;
    }

  // chekc data type for float
  if (region->GetDataType() != VTK_IMAGE_FLOAT)
    {
    vtkImageRegion *temp = region;
    
    vtkWarningMacro(<<"Converting non float image data to float");
    
    region = new vtkImageRegion;
    region->SetDataType(VTK_IMAGE_FLOAT);
    region->SetExtent(temp->GetExtent());
    region->CopyRegionData(temp);
    temp->Delete();
    }
    
  // loop over all the segments
  for(inLines->InitTraversal(); inLines->GetNextCell(nptsin, idsin);)
    {
    // move the current point based on its gradient neighbors
    for (j = 0; j < nptsin; j++)
      {
      pnt = inPts->GetPoint(idsin[j]);
      this->Move(region,(int)(pnt[0]+0.5),(int)(pnt[1]+0.5),
		 result, (int)(pnt[2]+0.5));
      newPts->InsertNextPoint(result);
      }
    }
  
  output->GetPointData()->PassData(input->GetPointData());
  output->SetPoints(newPts);
  output->SetLines(inLines);
  newPts->Delete();
  region->Delete();
}

void vtkSubPixelPositionEdgels::Move(vtkImageRegion *region, 
				     int x, int y,
				     float *result, int z)
{
  int *extent = region->GetExtent2d();
  float vec[2];
  float val1, val2, val3, val4;
  float mix,mag;
  float dist;
  float a,b,c,root;
  float *imgData, *imgPtr;
  int    imgIncX, imgIncY, imgIncVec;

  if (x <= extent[0] || y <= extent[2] || x >= extent[1] || y >= extent[3])
    {
    result[0] = x;
    result[1] = y;
    result[2] = z;
    }
  else 
    {
    // do the non maximal suppression at this pixel
    // first get the orientation
    imgData = (float *)region->GetScalarPointer4d(0,0,0,z);
    region->GetIncrements3d(imgIncX, imgIncY, imgIncVec);

    imgPtr = imgData + x*imgIncX + y*imgIncY;
    vec[0] = *(imgPtr+ imgIncVec);
    vec[1] = *(imgPtr + 2*imgIncVec);
    mag = *imgPtr;
    
    if (vec[1] && vec[0])
      {
      mix = fabs(atan2(vec[1],vec[0]));
      }
    else
      {
      mix = 0;
      }
    
    if (mix > 3.1415926/2.0)
      {
      mix -= 3.1415926/2.0;
      }
    if (mix > 3.1415926/4.0)
      {
      mix = 3.1415926/2.0 - mix;
      }
    
    mix = sin(mix)*1.4142;
    if (fabs(vec[0]) < fabs(vec[1]))
      {
      if (vec[1]*vec[0] > 0)
	{
	val1 = *(imgPtr + imgIncY);
	val2 = *(imgPtr + imgIncX + imgIncY);
	val3 = *(imgPtr - imgIncY);
	val4 = *(imgPtr - imgIncX - imgIncY);
	}
      else
	{
	val1 = *(imgPtr + imgIncY);
	val2 = *(imgPtr - imgIncX + imgIncY);
	val3 = *(imgPtr - imgIncY);
	val4 = *(imgPtr + imgIncX - imgIncY);
	}
      }
    else
      {
      if (vec[0]*vec[1] > 0)
	{
	val1 = *(imgPtr + imgIncX);
	val2 = *(imgPtr + imgIncX + imgIncY);
	val3 = *(imgPtr - imgIncX);
	val4 = *(imgPtr - imgIncX - imgIncY);
	}
      else
	{
	val1 = *(imgPtr - imgIncX);
	val2 = *(imgPtr - imgIncX + imgIncY);
	val3 = *(imgPtr + imgIncX);
	val4 = *(imgPtr + imgIncX - imgIncY);
	}
      }
    val1 = (1.0 - mix)*val1 + mix*val2;
    val3 = (1.0 - mix)*val3 + mix*val4;

    result[0] = x;
    result[1] = y;
    result[2] = z;

    // swap val1 and val3 if necc
    if (vec[1] < 0)
      {
      val2 = val3;
      val3 = val1;
      val1 = val2;
      }
    
    // now fit to a parabola and find max
    dist = 1.0 + mix*0.414;
    c = mag;
    b = (val1 - val3)/(2.0*dist);
    a = (val1 - mag - b*dist)/(dist*dist);
    root = -0.5*b/a;
    result[0] += vec[0]*root;
    result[1] += vec[1]*root;
    }
}


// Description:
// Override update method because execution can branch two ways 
// (Input and GradMaps)
void vtkSubPixelPositionEdgels::Update()
{
  // make sure input is available
  if (this->Input == NULL || this->Gradient == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
  this->Gradient->GetPipelineMTime() > this->ExecuteTime || 
  this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize();
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}





