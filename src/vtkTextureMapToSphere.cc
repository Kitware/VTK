/*=========================================================================

  Program:   Visualization Library
  Module:    vtkTextureMapToSphere.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does 
not apply to the related textbook "The Visualization Toolkit" ISBN 
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute
this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. Additionally, the 
authors grant permission to modify this software and its documentation for 
any purpose, provided that such modifications are not distributed without
the explicit consent of the authors and that existing copyright notices are 
retained in all copies.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.

=========================================================================*/
#include "vtkTextureMapToSphere.hh"
#include "vtkMath.hh"

// Description:
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
  vtkFloatTCoords *newTCoords;
  vtkDataSet *input=this->Input;
  int numPts=input->GetNumberOfPoints();
  int ptId;
  float *x, rho, r, tc[2], phi, thetaX, thetaY;
  vtkMath math;

  vtkDebugMacro(<<"Generating Spherical Texture Coordinates");

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

  //loop over all points computing spherical coordinates
  newTCoords = new vtkFloatTCoords(numPts,2);
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = input->GetPoint(ptId);
    rho = sqrt((double)math.Distance2BetweenPoints(x,this->Center));
    phi = acos((double)((x[2]-this->Center[2])/rho));
    tc[1] = phi / math.Pi();

    r = rho * sin((double)phi);
    thetaX = acos ((double)(x[0]-this->Center[0])/r);
    thetaY = asin ((double)(x[1]-this->Center[1])/r);

    if ( this->PreventSeam )
      {
      tc[0] = thetaX / math.Pi();
      }
    else
      {
      tc[0] = thetaX / (2.0*math.Pi());
      if ( thetaY < 0.0 )
        {
        tc[0] = 1.0 - tc[0];
        }
      }

    newTCoords->InsertTCoord(ptId,tc);
    }

  this->Output->GetPointData()->CopyTCoordsOff();
  this->Output->GetPointData()->PassData(this->Input->GetPointData());

  this->Output->GetPointData()->SetTCoords(newTCoords);
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

