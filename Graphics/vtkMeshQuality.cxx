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

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTetra.h"

vtkCxxRevisionMacro(vtkMeshQuality, "1.6");
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
  int j;
  vtkDataSet *input = this->GetInput();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkIdList *id = vtkIdList::New();
  vtkCellData *celld = vtkCellData::New();
  vtkFloatArray *scalars = vtkFloatArray::New();
  if (this->Volume && this->Ratio)
    {
    scalars->SetNumberOfComponents(2);
    }
  scalars->SetNumberOfTuples(numCells);
  
  double p1[3],p2[3],p3[3],p4[3]; 
  double dp1[3],dp2[3],dp3[3],dp4[3];
  double volume, ratio;
  double incenter[3], circenter[3];
  
  for (j=0; j<numCells; j++)
    {
    input->GetCellPoints(j,id);
    input->GetPoint(id->GetId(0),p1);
    dp1[0]=p1[0]; dp1[1]=p1[1]; dp1[2]=p1[2];
    input->GetPoint(id->GetId(1),p2);
    dp2[0]=p2[0]; dp2[1]=p2[1]; dp2[2]=p2[2];
    input->GetPoint(id->GetId(2),p3);
    dp3[0]=p3[0]; dp3[1]=p3[1]; dp3[2]=p3[2];
    input->GetPoint(id->GetId(3),p4);
    dp4[0]=p4[0]; dp4[1]=p4[1]; dp4[2]=p4[2];
    
    if (this->Volume && this->Ratio)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkTetra::Insphere(dp1,dp2,dp3,dp4,incenter);
      
      ratio = ratio/3;
      
      scalars->SetTuple2(j,volume,ratio);
      }
    else if (this->Ratio)
      {
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkTetra::Insphere(dp1,dp2,dp3,dp4,incenter);
      ratio = ratio/3;
      scalars->SetTuple1(j,ratio);
      }
    else if (this->Volume)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      scalars->SetTuple1(j,volume);
      }
    else
      {
      vtkErrorMacro(<<"Nothing to be calculated!!!!");
      }
    }
  
  int idx = celld->AddArray(scalars);
  celld->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  this->GetOutput()->SetFieldData(celld);
  celld->Delete();
  id->Delete();
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
