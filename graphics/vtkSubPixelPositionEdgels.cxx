/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1997 Ken Martin, Will Schroeder, Bill Lorensen.

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

vtkSubPixelPositionEdgels::vtkSubPixelPositionEdgels()
{
  this->GradMaps = NULL;
}

void vtkSubPixelPositionEdgels::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int numPts=input->GetNumberOfPoints();
  vtkFloatPoints *newPts;
  vtkPoints *inPts;
  vtkVectors *inVectors;
  vtkCellArray *inLines=input->GetLines();
  int nptsin, *idsin;
  vtkPolyData *output = this->GetOutput();
  int j;
  float *MapData, *CurrMap;
  float *pnt;
  int *dimensions;
  float result[3];
  
  vtkDebugMacro(<<"SubPixelPositioning Edgels");

  if ( numPts < 1 || (inPts=input->GetPoints()) == NULL )
    {
    vtkErrorMacro(<<"No data to fit!");
    return;
    }

  newPts = new vtkFloatPoints;

  dimensions = this->GradMaps->GetDimensions();
  MapData = ((vtkFloatScalars *)((this->GradMaps->GetPointData())->GetScalars()))->GetPtr(0);
  inVectors = this->GradMaps->GetPointData()->GetVectors();

  // loop over all the segments
  for(inLines->InitTraversal(); inLines->GetNextCell(nptsin, idsin);)
    {
    // move the current point based on its gradient neighbors
    for (j = 0; j < nptsin; j++)
      {
      pnt = inPts->GetPoint(idsin[j]);
      CurrMap = MapData + dimensions[0]*dimensions[1]*((int)(pnt[2]+0.5));
      this->Move(dimensions[0],dimensions[1],
		 (int)(pnt[0]+0.5),(int)(pnt[1]+0.5),CurrMap,
		 inVectors, result, (int)(pnt[2]+0.5));
      newPts->InsertNextPoint(result);
      }
    }
  
  output->GetPointData()->PassData(input->GetPointData());
  output->SetPoints(newPts);
  output->SetLines(inLines);
  newPts->Delete();
}

void vtkSubPixelPositionEdgels::Move(int xdim, int ydim, int x, int y,
				     float *img, vtkVectors *inVecs, 
				     float *result, int z)
{
  int ypos, zpos;
  float *vec;
  float val1, val2, val3, val4;
  float mix,mag;
  float dist;
  float a,b,c,root;
  
  zpos = z*xdim*ydim;

  ypos = y*xdim;
  if (x == 0 || y == 0 || x == (xdim-1) || y == (ydim -1))
    {
    result[0] = x;
    result[1] = y;
    result[2] = z;
    }
  else 
    {
    // do the non maximal suppression at this pixel
    // first get the orientation
    vec = inVecs->GetVector(x+ypos+zpos);
    mag = img[x+ypos];
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
	val1 = img[x + (y+1)*xdim];
	val2 = img[x + 1 + (y+1)*xdim];
	val3 = img[x + (y-1)*xdim];
	val4 = img[x - 1 + (y-1)*xdim];
	}
      else
	{
	val1 = img[x + (y+1)*xdim];
	val2 = img[x - 1 + (y+1)*xdim];
	val3 = img[x + (y-1)*xdim];
	val4 = img[x + 1 + (y-1)*xdim];
	}
      }
    else
      {
      if (vec[0]*vec[1] > 0)
	{
	val1 = img[x + 1 + ypos];
	val2 = img[x + 1 + (y+1)*xdim];
	val3 = img[x - 1 + ypos];
	val4 = img[x - 1 + (y-1)*xdim];
	}
      else
	{
	val1 = img[x - 1 + ypos];
	val2 = img[x - 1 + (y+1)*xdim];
	val3 = img[x + 1 + ypos];
	val4 = img[x + 1 + (y-1)*xdim];
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
  if (this->Input == NULL || this->GradMaps == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->GradMaps->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
  this->GradMaps->GetMTime() > this->ExecuteTime || 
  this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize();
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->GradMaps->ShouldIReleaseData() ) this->GradMaps->ReleaseData();
}

