/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToSphere.cxx
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
#include "vtkTextureMapToSphere.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkTextureMapToSphere* vtkTextureMapToSphere::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTextureMapToSphere");
  if(ret)
    {
    return (vtkTextureMapToSphere*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTextureMapToSphere;
}

// Create object with Center (0,0,0) and the PreventSeam ivar is set to true. The 
// sphere center is automatically computed.
vtkTextureMapToSphere::vtkTextureMapToSphere()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->AutomaticSphereGeneration = 1;
  this->PreventSeam = 1;
}

void vtkTextureMapToSphere::Execute()
{
  vtkTCoords *newTCoords;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType ptId;
  float *x, rho, r, tc[2], phi=0.0, thetaX, thetaY;
  double diff, PiOverTwo=vtkMath::Pi()/2.0;

  vtkDebugMacro(<<"Generating Spherical Texture Coordinates");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"Can't generate texture coordinates without points");
    return;
    }

  if ( this->AutomaticSphereGeneration )
    {
    this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      x = input->GetPoint(ptId);
      this->Center[0] += x[0];
      this->Center[1] += x[1];
      this->Center[2] += x[2];
      }
    this->Center[0] /= numPts;
    this->Center[1] /= numPts;
    this->Center[2] /= numPts;

    vtkDebugMacro(<<"Center computed as: (" << this->Center[0] <<", "
                  << this->Center[1] <<", " << this->Center[2] <<")");
    }

  //loop over all points computing spherical coordinates. Only tricky part
  //is keeping track of singularities/numerical problems.
  newTCoords = vtkTCoords::New();
  newTCoords->SetNumberOfTCoords(numPts);
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = input->GetPoint(ptId);
    rho = sqrt((double)vtkMath::Distance2BetweenPoints(x,this->Center));
    if ( rho != 0.0 )
      {
      // watch for truncation problems
      if ( fabs((diff=x[2]-this->Center[2])) > rho )
        {
        phi = 0.0;
        if ( diff > 0.0 )
	  {
	  tc[1] = 0.0;
	  }
        else
	  {
	  tc[1] = 1.0;
	  }
        }
      else
        {
        phi = acos((double)(diff/rho));
        tc[1] = phi / vtkMath::Pi();
        }
      }
    else
      {
      tc[1] = 0.0;
      }

    r = rho * sin((double)phi);
    if ( r != 0.0 )
      {
      // watch for truncation problems
      if ( fabs((diff=x[0]-this->Center[0])) > r )
        {
        if ( diff > 0.0 )
	  {
	  thetaX = 0.0;
	  }
        else
	  {
	  thetaX = vtkMath::Pi();
	  }
        }
      else
        {
        thetaX = acos ((double)diff/r);
        }

      if ( fabs((diff=x[1]-this->Center[1])) > r )
        {
        if ( diff > 0.0 )
	  {
	  thetaY = PiOverTwo;
	  }
        else
	  {
	  thetaY = -PiOverTwo;
	  }
        }
      else
        {
        thetaY = asin ((double)diff/r);
        }
      }
    else
      {
      thetaX = thetaY = 0.0;
      }

    if ( this->PreventSeam )
      {
      tc[0] = thetaX / vtkMath::Pi();
      }
    else
      {
      tc[0] = thetaX / (2.0*vtkMath::Pi());
      if ( thetaY < 0.0 )
        {
        tc[0] = 1.0 - tc[0];
        }
      }

    newTCoords->SetTCoord(ptId,tc);
    }

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkTextureMapToSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Automatic Sphere Generation: " << 
                  (this->AutomaticSphereGeneration ? "On\n" : "Off\n");
  os << indent << "Prevent Seam: " << 
                  (this->PreventSeam ? "On\n" : "Off\n");
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";
}

