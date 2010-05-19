/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaskPoints.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkMaskPoints);

//----------------------------------------------------------------------------
vtkMaskPoints::vtkMaskPoints()
{
  this->OnRatio = 2;
  this->Offset = 0;
  this->RandomMode = 0;
  this->MaximumNumberOfPoints = VTK_LARGE_ID;
  this->GenerateVertices = 0;
  this->SingleVertexPerCell = 0;
}

//----------------------------------------------------------------------------
int vtkMaskPoints::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *newPts;
  vtkPointData *pd;
  vtkIdType numNewPts;
  double x[3];
  vtkIdType ptId, id;
  vtkPointData *outputPD = output->GetPointData();
  vtkIdType numPts=input->GetNumberOfPoints();
  
  // Check input
  //
  vtkDebugMacro(<<"Masking points");

  if ( numPts < 1 )
    {
    return 1;
    }

  pd = input->GetPointData();
  id = 0;
  
  // Allocate space
  //
  numNewPts = numPts / this->OnRatio;
  if (numNewPts > this->MaximumNumberOfPoints)
    {
    numNewPts = this->MaximumNumberOfPoints;
    }
  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  outputPD->CopyAllocate(pd);

  // Traverse points and copy
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 +1;
  if ( this->RandomMode ) // retro mode
    {
    double cap;
    
    if ((static_cast<double>(numPts)/this->OnRatio) > this->MaximumNumberOfPoints)
      {
      cap = 2.0*numPts/this->MaximumNumberOfPoints - 1;
      }
    else 
      {
      cap = 2.0*this->OnRatio - 1;
      }

    for (ptId = this->Offset; 
    (ptId < numPts) && (id < this->MaximumNumberOfPoints) && !abort;  
         ptId += (1 + static_cast<int>(static_cast<double>(vtkMath::Random())*cap)) )
      {
      input->GetPoint(ptId, x);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      if ( ! (id % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5*id/numPts);
        abort = this->GetAbortExecute();
        }
      }
    }
  else // a.r. mode
    {
    for ( ptId = this->Offset; 
    (ptId < numPts) && (id < (this->MaximumNumberOfPoints-1)) && !abort;
    ptId += this->OnRatio )
      {
      input->GetPoint(ptId, x);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      if ( ! (id % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5*id/numPts);
        abort = this->GetAbortExecute();
        }
      }
    }

  // Generate vertices if requested
  //
  if ( this->GenerateVertices )
    {
    vtkCellArray *verts = vtkCellArray::New();
    if (this->SingleVertexPerCell)
      {
      verts->Allocate(id*2);
      }
    else 
      {
      verts->Allocate(verts->EstimateSize(1,id+1));
      verts->InsertNextCell(id+1);
      }
    for ( ptId=0; ptId<(id+1) && !abort; ptId++)
      {
      if ( ! (ptId % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5+0.5*ptId/(id+1));
        abort = this->GetAbortExecute();
        }
      if (this->SingleVertexPerCell)
        {
          verts->InsertNextCell(1,&ptId);
        }
      else 
        {
        verts->InsertCellPoint(ptId);
        }

      }
    output->SetVerts(verts);
    verts->Delete();
    }

  // Update ourselves
  //
  output->SetPoints(newPts);
  newPts->Delete();
  
  output->Squeeze();

  vtkDebugMacro(<<"Masked " << numPts << " original points to " 
                << id+1 << " points");

  return 1;
}

//----------------------------------------------------------------------------
int vtkMaskPoints::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkMaskPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Vertices: " 
     << (this->GenerateVertices ? "On\n" : "Off\n");
  os << indent << "SingleVertexPerCell: " 
     << (this->SingleVertexPerCell ? "On\n" : "Off\n");
  os << indent << "MaximumNumberOfPoints: " 
     << this->MaximumNumberOfPoints << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Random Mode: " << (this->RandomMode ? "On\n" : "Off\n");
}
