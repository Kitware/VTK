/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToCylinder.cxx
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
#include "vtkTextureMapToCylinder.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkOBBTree.h"

// Description:
// Create object with cylinder axis parallel to z-axis (points (0,0,-0.5) 
// and (0,0,0.5)). The PreventSeam ivar is set to true. The cylinder is 
// automatically generated.
vtkTextureMapToCylinder::vtkTextureMapToCylinder()
{
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.0;
  this->Point1[2] = -0.5;

  this->Point2[0] = 0.0;
  this->Point2[1] = 0.0;
  this->Point2[2] = 0.5;

  this->AutomaticCylinderGeneration = 1;
  this->PreventSeam = 1;
}

void vtkTextureMapToCylinder::Execute()
{
  vtkFloatTCoords *newTCoords;
  vtkDataSet *input=this->Input;
  int numPts=input->GetNumberOfPoints();
  int ptId, i;
  float *x, tc[2], thetaX, thetaY, closest[3], v[3];
  float axis[3], vP[3], vec[3];

  vtkDebugMacro(<<"Generating Cylindrical Texture Coordinates");

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"Can't generate texture coordinates without points");
    return;
    }

  if ( this->AutomaticCylinderGeneration )
    {
    vtkFloatPoints *pts=new vtkFloatPoints(numPts); pts->SetNumberOfPoints(numPts);
    float corner[3], max[3], mid[3], min[3], size[3], l;
    vtkOBBTree OBB;

    for ( ptId=0; ptId < numPts; ptId++ )
      {
      x = input->GetPoint(ptId);
      pts->SetPoint(ptId,x);
      }

    OBB.ComputeOBB(pts,corner,max,mid,min,size);
    pts->Delete();

    for ( i=0; i < 3; i++)
      {
      l = (mid[i] + min[i])/2.0;
      this->Point1[i] = corner[i] + l;
      this->Point2[i] = corner[i] + max[i] + l;
      }

    vtkDebugMacro(<<"Cylinder axis computed as \tPoint1: (" 
                  << this->Point1[0] <<", " << this->Point1[1] <<", " 
                  << this->Point1[2] <<")\n\t\t\t\tPoint2: ("
                  << this->Point2[0] <<", " << this->Point2[1] <<", " 
                  << this->Point2[2] <<")");
    }

  //compute axis which is theta (angle measure) origin
  for ( i=0; i < 3; i++ ) axis[i] = this->Point2[i] - this->Point1[i];
  if ( vtkMath::Norm(axis) == 0.0 )
    {
    vtkErrorMacro(<<"Bad cylinder axis");
    return;
    }

  v[0] = 1.0; v[1] = v[2] = 0.0;
  vtkMath::Cross(axis,v,vP);
  if ( vtkMath::Norm(vP) == 0.0 )
    {//must be prependicular
    v[1] = 1.0; v[0] = v[2] = 0.0;
    vtkMath::Cross(axis,v,vP);
    }
  vtkMath::Cross(vP,axis,vec);
  if ( vtkMath::Normalize(vec) == 0.0 )
    {
    vtkErrorMacro(<<"Bad cylinder axis");
    return;
    }
  newTCoords = new vtkFloatTCoords(numPts,2);

  //loop over all points computing spherical coordinates
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = input->GetPoint(ptId);
    vtkLine::DistanceToLine(x,this->Point1,this->Point2,tc[1],closest);

    for (i=0; i < 3; i++) v[i] = x[i] - closest[i];
    vtkMath::Normalize(v);

    thetaX = acos ((double)vtkMath::Dot(v,vec));
    vtkMath::Cross(vec,v,vP);
    thetaY = vtkMath::Dot(axis,vP); //not really interested in angle, just +/- sign

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

    newTCoords->InsertTCoord(ptId,tc);
    }

  this->Output->GetPointData()->CopyTCoordsOff();
  this->Output->GetPointData()->PassData(this->Input->GetPointData());

  this->Output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

}

void vtkTextureMapToCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Automatic Cylinder Generation: " << 
                  (this->AutomaticCylinderGeneration ? "On\n" : "Off\n");
  os << indent << "Prevent Seam: " << 
                  (this->PreventSeam ? "On\n" : "Off\n");
  os << indent << "Point1: (" << this->Point1[0] << ", "
                              << this->Point1[1] << ", "
                              << this->Point1[2] << ")\n";
  os << indent << "Point2: (" << this->Point2[0] << ", "
                              << this->Point2[1] << ", "
                              << this->Point2[2] << ")\n";
}

