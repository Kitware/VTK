/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeshQuality.h"

#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkTetra.h"
#include "vtkCellArray.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkMeshQuality, "1.8");
vtkStandardNewMacro(vtkMeshQuality);

//----------------------------------------------------------------------------
// Constructor
vtkMeshQuality::vtkMeshQuality() 
{
 this->GeometryOff();
 this->TopologyOff();
 this->FieldDataOff();
 this->PointDataOff();
 this->CellDataOn();
 
 this->Volume = 1;
 this->Ratio = 1;
}

//----------------------------------------------------------------------------
//destructor
vtkMeshQuality::~vtkMeshQuality() 
{ 
}

//----------------------------------------------------------------------------
void vtkMeshQuality::Execute()
{
  vtkDataSet* ds = this->GetInput();
  if( !ds )
    {
    vtkErrorMacro("Input not set");
    return;
    }

  if ( ds->GetDataObjectType() != VTK_UNSTRUCTURED_GRID )
    {
    vtkErrorMacro("Wrong input type, should be vtkUnstructuredGrid");
    return;
    }

  if( !this->Volume && !this->Ratio )
    {
    vtkErrorMacro(<<"Nothing to be calculated");
    }

  int j;
  vtkUnstructuredGrid *input = (vtkUnstructuredGrid*)ds;
  vtkIdType numCells = input->GetNumberOfCells();
  vtkCellData *celld = vtkCellData::New();
  vtkFloatArray *scalars = vtkFloatArray::New();

  if (this->Volume && this->Ratio)
    {
    scalars->SetNumberOfComponents(2);
    }
  scalars->SetNumberOfTuples(numCells);
  
  double dp1[3],dp2[3],dp3[3],dp4[3];
  double volume, ratio;
  double incenter[3], circenter[3];
  
  vtkCellArray *cellArray = input->GetCells();
  vtkPoints *inPts = input->GetPoints();

  vtkIdType npts, *pts = 0;
  for(j=0, cellArray->InitTraversal(); cellArray->GetNextCell(npts, pts); j++)
    {
    inPts->GetPoint(pts[0],dp1);
    inPts->GetPoint(pts[1],dp2);
    inPts->GetPoint(pts[2],dp3);
    inPts->GetPoint(pts[3],dp4);
    
    if (this->Volume && this->Ratio)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkTetra::Insphere(dp1,dp2,dp3,dp4,incenter);
      
      ratio /= 3;
      scalars->SetTuple2(j,volume,ratio);
      }
    else if (this->Ratio)
      {
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkTetra::Insphere(dp1,dp2,dp3,dp4,incenter);
      ratio /= 3;
      scalars->SetTuple1(j,ratio);
      }
    else if (this->Volume)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      scalars->SetTuple1(j,volume);
      }
    }
  
  int idx = celld->AddArray(scalars);
  celld->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  this->GetOutput()->SetFieldData(celld);
  celld->Delete();
  scalars->Delete();
}

//----------------------------------------------------------------------------
void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataObjectFilter::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Volume: " << (this->Volume ? "On\n" : "Off\n");
  os << indent << "Ratio: " << (this->Ratio ? "On\n" : "Off\n");
}
