/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.cxx
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
#include "vtkWarpScalar.h"

vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;
}

float *vtkWarpScalar::DataNormal(int id, vtkNormals *normals)
{
  return normals->GetNormal(id);
}

float *vtkWarpScalar::InstanceNormal(int vtkNotUsed(id), 
				     vtkNormals *vtkNotUsed(normals))
{
  return this->Normal;
}

float *vtkWarpScalar::ZNormal(int vtkNotUsed(id), 
			      vtkNormals *vtkNotUsed(normals))
{
  static float zNormal[3]={0.0,0.0,1.0};
  return zNormal;
}

void vtkWarpScalar::Execute()
{
  vtkPoints *inPts;
  vtkNormals *inNormals;
  vtkScalars *inScalars;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  int i, ptId, numPts;
  float *x, *n, s, newX[3];
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkPointSet *output=(vtkPointSet *)this->Output;
  
  vtkDebugMacro(<<"Warping data with scalars");

  inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();
  pd = input->GetPointData();
  inNormals = pd->GetNormals();
  inScalars = pd->GetScalars();

  if ( !inPts || !inScalars )
    {
    vtkErrorMacro(<<"No data to warp");
    return;
    }

  if ( inNormals && !this->UseNormal )
    {
    this->PointNormal = &vtkWarpScalar::DataNormal;
    vtkDebugMacro(<<"Using data normals");
    }
  else if ( this->XYPlane )
    {
    this->PointNormal = &vtkWarpScalar::ZNormal;
    vtkDebugMacro(<<"Using x-y plane normal");
    }
  else
    {
    this->PointNormal = &vtkWarpScalar::InstanceNormal;
    vtkDebugMacro(<<"Using Normal instance variable");
    }

  newPts = new vtkFloatPoints(numPts); 
  newPts->SetNumberOfPoints(numPts);
//
// Loop over all points, adjusting locations
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    n = (this->*(PointNormal))(ptId,inNormals);
    if ( this->XYPlane ) s = x[2];
    else s = inScalars->GetScalar(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    newPts->SetPoint(ptId, newX);
    }
//
// Update ourselves and release memory
//
  output->GetPointData()->CopyNormalsOff(); // distorted geometry 
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " 
     << this->Normal[1] << ", " << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
}
