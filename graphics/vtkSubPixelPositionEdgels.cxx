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
#include "vtkMath.h"
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
  vtkFloatNormals *newNormals;
  vtkPoints *inPts;
  vtkVectors *inVectors;
  int ptId;
  vtkPolyData *output = this->GetOutput();
  float *MapData;
  float pnt[3];
  int *dimensions;
  float result[3], resultNormal[3];
  float *spacing, *origin;
  
  vtkDebugMacro(<<"SubPixelPositioning Edgels");

  if ( numPts < 1 || (inPts=input->GetPoints()) == NULL )
    {
    vtkErrorMacro(<<"No data to fit!");
    return;
    }

  newPts = vtkFloatPoints::New();
  newNormals = vtkFloatNormals::New();
  
  dimensions = this->GradMaps->GetDimensions();
  spacing = this->GradMaps->GetSpacing();
  origin = this->GradMaps->GetOrigin();
  MapData = ((vtkFloatScalars *)((this->GradMaps->GetPointData())->GetScalars()))->GetPointer(0);
  inVectors = this->GradMaps->GetPointData()->GetVectors();

  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < inPts->GetNumberOfPoints(); ptId++)
    {
    inPts->GetPoint(ptId,pnt);
    pnt[0] = (pnt[0] - origin[0])/spacing[0];
    pnt[1] = (pnt[1] - origin[1])/spacing[1];
    pnt[2] = (pnt[2] - origin[2])/spacing[2];
    this->Move(dimensions[0],dimensions[1],dimensions[2],
	       (int)(pnt[0]+0.5),(int)(pnt[1]+0.5),MapData,
	       inVectors, result, (int)(pnt[2]+0.5), spacing,
	       resultNormal);
    result[0] = result[0]*spacing[0] + origin[0];
    result[1] = result[1]*spacing[1] + origin[1];
    result[2] = result[2]*spacing[2] + origin[2];
    newPts->InsertNextPoint(result);
    newNormals->InsertNextNormal(resultNormal);
    }
  
  output->CopyStructure(input);
  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->SetNormals(newNormals);
  output->SetPoints(newPts);
  newPts->Delete();
  newNormals->Delete();
}

void vtkSubPixelPositionEdgels::Move(int xdim, int ydim, int zdim,
				     int x, int y,
				     float *img, vtkVectors *inVecs, 
				     float *result, int z, float *spacing,
				     float *resultNormal)
{
  int ypos, zpos;
  float vec[3];
  float valn, valp;
  float mag;
  float a,b,c,root;
  float xp, yp, zp;
  float xn, yn, zn;
  int xi, yi, zi, i;
  
  zpos = z*xdim*ydim;

  ypos = y*xdim;
  
  // handle the 2d case
  if (zdim < 2)
    {
    if (x < 1 || y < 1 || x == (xdim-1) || y == (ydim -1))
      {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      }
    else 
      {
      // first get the orientation
      inVecs->GetVector(x+ypos,vec);
      vec[0] = vec[0]*spacing[0];
      vec[1] = vec[1]*spacing[1];
      vec[2] = 0;
      vtkMath::Normalize(vec);
      mag = img[x+ypos];
      
      // compute the sample points
      xp = (float)x + vec[0];
      yp = (float)y + vec[1];
      xn = (float)x - vec[0];
      yn = (float)y - vec[1];
      
      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      valp = 
	img[xi +xdim*yi]*(1.0 -xp +xi)*(1.0 -yp +yi) +
	img[1 +xi + xdim*yi]*(xp -xi)*(1.0 -yp +yi) +
	img[xi +xdim*(yi +1)]*(1.0 -xp +xi)*(yp -yi) +
	img[1 + xi + xdim*(yi +1)]*(xp -xi)*(yp -yi);
      
      xi = (int)xn;
      yi = (int)yn;
      valn = 
	img[xi +xdim*yi]*(1.0 -xn +xi)*(1.0 -yn +yi) +
	img[1 + xi +xdim*yi]*(xn -xi)*(1.0 -yn +yi) +
	img[xi + xdim*(yi +1)]*(1.0 -xn +xi)*(yn -yi) +
	img[1 + xi + xdim*(yi +1)]*(xn -xi)*(yn -yi);
      
      result[0] = x;
      result[1] = y;
      result[2] = z;
      
      // now fit to a parabola and find max
      c = mag;
      b = (valp - valn)/2.0;
      a = (valp - c - b);
      root = -0.5*b/a;
      if (root > 1.0) root = 1.0;
      if (root < -1.0) root = -1.0;
      result[0] += vec[0]*root;
      result[1] += vec[1]*root;
      
      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      zn = result[2];
      for (i = 0; i < 3; i++)
	{
	resultNormal[i] = 
	  inVecs->GetVector(xi + xdim*yi)[i] * (1.0 -xn +xi)*(1.0 -yn +yi) +
	  inVecs->GetVector(1 + xi + xdim*yi)[i] * (xn -xi)*(1.0 -yn +yi) +
	  inVecs->GetVector(xi + xdim*(yi +1))[i] * (1.0 -xn +xi)*(yn -yi) +
	  inVecs->GetVector(1 + xi + xdim*(yi +1))[i] * (xn -xi)*(yn -yi);
	}
      vtkMath::Normalize(resultNormal);
      }
    }
  else
    {
    if (x < 1 || y < 1 || z < 1 || 
	x == (xdim-1) || y == (ydim -1) || z == (zdim -1))
      {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      }
    else 
      {
      // first get the orientation
      inVecs->GetVector(x+ypos+zpos,vec);
      vec[0] = vec[0]*spacing[0];
      vec[1] = vec[1]*spacing[1];
      vec[2] = vec[2]*spacing[2];
      vtkMath::Normalize(vec);
      mag = img[x+ypos+zpos];
      
      // compute the sample points
      xp = (float)x + vec[0];
      yp = (float)y + vec[1];
      zp = (float)z + vec[2];
      xn = (float)x - vec[0];
      yn = (float)y - vec[1];
      zn = (float)z - vec[2];
      
      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      zi = (int)zp;
      valp = 
	img[xi +xdim*(yi +zi*ydim)]*(1.0 -xp +xi)*(1.0 -yp +yi)*(1.0 -zp +zi) +
	img[1 +xi + xdim*(yi + zi*ydim)]*(xp -xi)*(1.0 -yp +yi)*(1.0 -zp +zi) +
	img[xi +xdim*(yi +1 + zi*ydim)]*(1.0 -xp +xi)*(yp -yi)*(1.0 -zp +zi) +
	img[1 + xi + xdim*(yi +1 +zi*ydim)]*(xp -xi)*(yp -yi)*(1.0 -zp +zi) +
	img[xi +xdim*(yi + (zi+1)*ydim)]*(1.0 -xp +xi)*(1.0 -yp +yi)*(zp -zi) +
	img[1 + xi + xdim*(yi + (zi+1)*ydim)]*(xp -xi)*(1.0 -yp +yi)*(zp -zi) +
	img[xi + xdim*(yi +1 + (zi+1)*ydim)]*(1.0 -xp +xi)*(yp -yi)*(zp -zi) +
	img[1 + xi + xdim*(yi +1 +(zi+1)*ydim)]*(xp -xi)*(yp -yi)*(zp -zi);
      
      xi = (int)xn;
      yi = (int)yn;
      zi = (int)zn;
      valn = 
	img[xi +xdim*(yi +zi*ydim)]*(1.0 -xn +xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	img[1 + xi +xdim*(yi + zi*ydim)]*(xn -xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	img[xi + xdim*(yi +1 + zi*ydim)]*(1.0 -xn +xi)*(yn -yi)*(1.0 -zn +zi) +
	img[1 + xi + xdim*(yi +1 +zi*ydim)]*(xn -xi)*(yn -yi)*(1.0 -zn +zi) +
	img[xi +xdim*(yi + (zi+1)*ydim)]*(1.0 -xn +xi)*(1.0 -yn +yi)*(zn -zi) +
	img[1 + xi + xdim*(yi + (zi+1)*ydim)]*(xn -xi)*(1.0 -yn +yi)*(zn -zi) +
	img[xi + xdim*(yi +1 + (zi+1)*ydim)]*(1.0 -xn +xi)*(yn -yi)*(zn -zi) +
	img[1 + xi + xdim*(yi +1 +(zi+1)*ydim)]*(xn -xi)*(yn -yi)*(zn -zi);
      
      result[0] = x;
      result[1] = y;
      result[2] = z;
      
      // now fit to a parabola and find max
      c = mag;
      b = (valp - valn)/2.0;
      a = (valp - c - b);
      root = -0.5*b/a;
      if (root > 1.0) root = 1.0;
      if (root < -1.0) root = -1.0;
      result[0] += vec[0]*root;
      result[1] += vec[1]*root;
      result[2] += vec[2]*root;
      
      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      zn = result[2];
      for (i = 0; i < 3; i++)
	{
	resultNormal[i] = 
	  inVecs->GetVector(xi + xdim*(yi + zi*ydim))[i] * 
	  (1.0 -xn +xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	  inVecs->GetVector(1 + xi + xdim*(yi + zi*ydim))[i] *
	  (xn -xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	  inVecs->GetVector(xi + xdim*(yi +1 + zi*ydim))[i] *
	  (1.0 -xn +xi)*(yn -yi)*(1.0 -zn +zi) +
	  inVecs->GetVector(1 + xi + xdim*(yi +1 +zi*ydim))[i] *
	  (xn -xi)*(yn -yi)*(1.0 -zn +zi) +
	  inVecs->GetVector(xi + xdim*(yi + (zi+1)*ydim))[i] *
	  (1.0 -xn +xi)*(1.0 -yn +yi)*(zn -zi) +
	  inVecs->GetVector(1 + xi + xdim*(yi + (zi+1)*ydim))[i] * 
	  (xn -xi)*(1.0 -yn +yi)*(zn -zi) +
	  inVecs->GetVector(xi + xdim*(yi +1 + (zi+1)*ydim))[i] *
	  (1.0 -xn +xi)*(yn -yi)*(zn -zi) +
	  inVecs->GetVector(1 + xi + xdim*(yi +1 +(zi+1)*ydim))[i] *
	  (xn -xi)*(yn -yi)*(zn -zi);
	}
      vtkMath::Normalize(resultNormal);
      }
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

