/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpScalar.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkWarpScalar, "1.41");
vtkStandardNewMacro(vtkWarpScalar);

vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;

  this->InputScalarsSelection = NULL;
  // Accept the memory leak for now.
}

vtkWarpScalar::~vtkWarpScalar()
{
  this->SetInputScalarsSelection(NULL);
}

float *vtkWarpScalar::DataNormal(vtkIdType id, vtkDataArray *normals)
{
  return normals->GetTuple(id);
}

float *vtkWarpScalar::InstanceNormal(vtkIdType vtkNotUsed(id), 
                                     vtkDataArray *vtkNotUsed(normals))
{
  return this->Normal;
}

float *vtkWarpScalar::ZNormal(vtkIdType vtkNotUsed(id), 
                              vtkDataArray *vtkNotUsed(normals))
{
  static float zNormal[3]={0.0,0.0,1.0};
  return zNormal;
}

void vtkWarpScalar::Execute()
{
  vtkPoints *inPts;
  vtkDataArray *inNormals;
  vtkDataArray *inScalars;
  vtkPoints *newPts;
  vtkPointData *pd;
  int i;
  vtkIdType ptId, numPts;
  float *x, *n, s, newX[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  
  vtkDebugMacro(<<"Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inNormals = pd->GetNormals();

  inScalars = pd->GetScalars(this->InputScalarsSelection);
  if ( !inPts || !inScalars )
    {
    vtkErrorMacro(<<"No data to warp");
    return;
    }

  numPts = inPts->GetNumberOfPoints();

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

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 10000) ) 
      {
      this->UpdateProgress ((float)ptId/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    x = inPts->GetPoint(ptId);
    n = (this->*(this->PointNormal))(ptId,inNormals);
    if ( this->XYPlane )
      {
      s = x[2];
      }
    else
      {
      s = inScalars->GetComponent(ptId,0);
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    newPts->SetPoint(ptId, newX);
    }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry 
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry 
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->InputScalarsSelection)
    {
    os << indent << "InputScalarsSelection: " << this->InputScalarsSelection;
    } 
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " 
     << this->Normal[1] << ", " << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
}
