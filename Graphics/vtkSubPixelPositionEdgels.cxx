/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMath.h"
#include "vtkSubPixelPositionEdgels.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSubPixelPositionEdgels* vtkSubPixelPositionEdgels::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSubPixelPositionEdgels");
  if(ret)
    {
    return (vtkSubPixelPositionEdgels*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSubPixelPositionEdgels;
}

vtkSubPixelPositionEdgels::vtkSubPixelPositionEdgels()
{
  this->TargetFlag = 0;
  this->TargetValue = 0.0;
}

vtkSubPixelPositionEdgels::~vtkSubPixelPositionEdgels()
{
}

void vtkSubPixelPositionEdgels::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkPoints *newPts;
  vtkFloatArray *newNormals;
  vtkPoints *inPts;
  vtkDataArray *inVectors;
  vtkIdType ptId;
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

  newPts = vtkPoints::New();
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  
  dimensions = this->GetGradMaps()->GetDimensions();
  spacing = this->GetGradMaps()->GetSpacing();
  origin = this->GetGradMaps()->GetOrigin();
  MapData = ((vtkFloatArray *)(this->GetGradMaps()->GetPointData())
	     ->GetScalars()->GetData())->GetPointer(0);
  inVectors = this->GetGradMaps()->GetPointData()->GetActiveVectors();

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
    newNormals->InsertNextTuple(resultNormal);
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
				     float *img, vtkDataArray *inVecs, 
				     float *result, int z, float *spacing,
				     float *resultNormal)
{
  int ypos;
  vtkIdType zpos;
  float vec[3];
  float valn, valp;
  float mag;
  float a,b,c;
  float xp, yp, zp;
  float xn, yn, zn;
  int xi, yi, zi, i;
  
  zpos = z*xdim*ydim;

  ypos = y*xdim;
  
  // handle the 2d case
  if (zdim < 2)
    {
    if (x < 1 || y < 1 || x == (xdim-2) || y == (ydim -2))
      {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      for (i = 0; i < 3; i++)
	{
	resultNormal[i] = 
	  inVecs->GetTuple(x + xdim*y)[i];
	}
      vtkMath::Normalize(resultNormal);
      }
    else 
      {
      // first get the orientation
      inVecs->GetTuple(x+ypos,vec);
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
      // assign the root to c because MSVC5.0 optimizer has problems with this
      // function
      c = -0.5*b/a;
      if (c > 1.0)
	{
	c = 1.0;
	}
      if (c < -1.0)
	{
	c = -1.0;
	}
      result[0] += vec[0]*c;
      result[1] += vec[1]*c;
      
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
	  inVecs->GetTuple(xi + xdim*yi)[i] * (1.0 -xn +xi)*(1.0 -yn +yi) +
	  inVecs->GetTuple(1 + xi + xdim*yi)[i] * (xn -xi)*(1.0 -yn +yi) +
	  inVecs->GetTuple(xi + xdim*(yi +1))[i] * (1.0 -xn +xi)*(yn -yi) +
	  inVecs->GetTuple(1 + xi + xdim*(yi +1))[i] * (xn -xi)*(yn -yi);
	}
      vtkMath::Normalize(resultNormal);
      }
    }
  else
    {
    if (x < 1 || y < 1 || z < 1 || 
	x == (xdim-2) || y == (ydim -2) || z == (zdim -2))
      {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      for (i = 0; i < 3; i++)
	{
	resultNormal[i] = 
	  inVecs->GetTuple(x + xdim*y + xdim*ydim*z)[i];
	}
      vtkMath::Normalize(resultNormal);
      }
    else 
      {
      // first get the orientation
      inVecs->GetTuple(x+ypos+zpos,vec);
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

      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valp = 
	img[xi +xdim*(yi +zi*ydim)]*(1.0 -xp +xi)*(1.0 -yp +yi)*(1.0 -zp +zi);
      valp +=
	img[1 +xi + xdim*(yi + zi*ydim)]*(xp -xi)*(1.0 -yp +yi)*(1.0 -zp +zi);
      valp +=
	img[xi +xdim*(yi +1 + zi*ydim)]*(1.0 -xp +xi)*(yp -yi)*(1.0 -zp +zi);
      valp +=
	img[1 + xi + xdim*(yi +1 +zi*ydim)]*(xp -xi)*(yp -yi)*(1.0 -zp +zi);
      valp +=
	img[xi +xdim*(yi + (zi+1)*ydim)]*(1.0 -xp +xi)*(1.0 -yp +yi)*(zp -zi);
      valp +=
	img[1 + xi + xdim*(yi + (zi+1)*ydim)]*(xp -xi)*(1.0 -yp +yi)*(zp -zi);
      valp +=
	img[xi + xdim*(yi +1 + (zi+1)*ydim)]*(1.0 -xp +xi)*(yp -yi)*(zp -zi);
      valp +=
	img[1 + xi + xdim*(yi +1 +(zi+1)*ydim)]*(xp -xi)*(yp -yi)*(zp -zi);
      
      xi = (int)xn;
      yi = (int)yn;
      zi = (int)zn;
      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valn = 
	img[xi +xdim*(yi +zi*ydim)]*(1.0 -xn +xi)*(1.0 -yn +yi)*(1.0 -zn +zi);
      valn +=
	img[1 + xi +xdim*(yi + zi*ydim)]*(xn -xi)*(1.0 -yn +yi)*(1.0 -zn +zi);
      valn +=
	img[xi + xdim*(yi +1 + zi*ydim)]*(1.0 -xn +xi)*(yn -yi)*(1.0 -zn +zi);
      valn +=
	img[1 + xi + xdim*(yi +1 +zi*ydim)]*(xn -xi)*(yn -yi)*(1.0 -zn +zi);
      valn +=
	img[xi +xdim*(yi + (zi+1)*ydim)]*(1.0 -xn +xi)*(1.0 -yn +yi)*(zn -zi);
      valn +=
	img[1 + xi + xdim*(yi + (zi+1)*ydim)]*(xn -xi)*(1.0 -yn +yi)*(zn -zi);
      valn +=
	img[xi + xdim*(yi +1 + (zi+1)*ydim)]*(1.0 -xn +xi)*(yn -yi)*(zn -zi);
      valn +=
	img[1 + xi + xdim*(yi +1 +(zi+1)*ydim)]*(xn -xi)*(yn -yi)*(zn -zi);
      
      result[0] = x;
      result[1] = y;
      result[2] = z;

      if (this->TargetFlag)
	{
	// For target, do a simple linear interpolation to avoid binomial.
	c = mag;
	if (c == this->TargetValue)
	  {
	  c = 0.0;
	  }
	else if ((this->TargetValue < c && valp < c) ||
		 (this->TargetValue > c && valp > c))
	  {
	  c = (this->TargetValue - c) / (valp - c);
	  }
	else if ((this->TargetValue < c && valn < c) ||
		 (this->TargetValue < c && valn > c))
	  {
	  c = (this->TargetValue - c) / (c - valn);
	  }
	else
	  {
	  c = 0.0;
	  }
	}
      else 
	{
	// now fit to a parabola and find max
	c = mag;
	b = (valp - valn)/2.0;
	a = (valp - c - b);
	
	//assign the root to c because MSVC5.0 optimizer has problems with this
	// function
	c = -0.5*b/a;
	}
      
      if (c > 1.0) 
        {
        c = 1.0;
        }
      if (c < -1.0) 
        {
        c = -1.0;
        }
      result[0] = result[0] + vec[0]*c;
      result[1] = result[1] + vec[1]*c;
      result[2] = result[2] + vec[2]*c;

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
	  inVecs->GetTuple(xi + xdim*(yi + zi*ydim))[i] * 
	  (1.0 -xn +xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	  inVecs->GetTuple(1 + xi + xdim*(yi + zi*ydim))[i] *
	  (xn -xi)*(1.0 -yn +yi)*(1.0 -zn +zi) +
	  inVecs->GetTuple(xi + xdim*(yi +1 + zi*ydim))[i] *
	  (1.0 -xn +xi)*(yn -yi)*(1.0 -zn +zi) +
	  inVecs->GetTuple(1 + xi + xdim*(yi +1 +zi*ydim))[i] *
	  (xn -xi)*(yn -yi)*(1.0 -zn +zi) +
	  inVecs->GetTuple(xi + xdim*(yi + (zi+1)*ydim))[i] *
	  (1.0 -xn +xi)*(1.0 -yn +yi)*(zn -zi) +
	  inVecs->GetTuple(1 + xi + xdim*(yi + (zi+1)*ydim))[i] * 
	  (xn -xi)*(1.0 -yn +yi)*(zn -zi) +
	  inVecs->GetTuple(xi + xdim*(yi +1 + (zi+1)*ydim))[i] *
	  (1.0 -xn +xi)*(yn -yi)*(zn -zi) +
	  inVecs->GetTuple(1 + xi + xdim*(yi +1 +(zi+1)*ydim))[i] *
	  (xn -xi)*(yn -yi)*(zn -zi);
	}
      vtkMath::Normalize(resultNormal);

      }
    }
}

void vtkSubPixelPositionEdgels::SetGradMaps(vtkStructuredPoints *gm)
{
  this->vtkProcessObject::SetNthInput(1, gm);
}

vtkStructuredPoints *vtkSubPixelPositionEdgels::GetGradMaps()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return (vtkStructuredPoints*)(this->Inputs[1]);
}

// Print the state of the class.
void vtkSubPixelPositionEdgels::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if ( this->GetGradMaps() )
    {
    os << indent << "Gradient Data: " << this->GetGradMaps() << "\n";
    }
  else
    {
    os << indent << "Gradient Data: (none)\n";
    }
  
  os << indent << "TargetFlag: " << this->TargetFlag << endl;
  os << indent << "TargetValue: " << this->TargetValue << endl;
}
