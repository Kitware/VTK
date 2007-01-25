/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIdTypeArray.h"
#include "vtkAbstractGraph.h"

vtkCxxRevisionMacro(vtkGraphToPolyData, "1.4");
vtkStandardNewMacro(vtkGraphToPolyData);

vtkGraphToPolyData::vtkGraphToPolyData()
{
}

void vtkGraphToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkGraphToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAbstractGraph");
  return 1;
}

int vtkGraphToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkAbstractGraph *input = vtkAbstractGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* arcGhostLevels = vtkDataArray::SafeDownCast(
    input->GetCellData()->GetAbstractArray("vtkGhostLevels"));

  if (arcGhostLevels == NULL)
    {
    vtkCellArray* newLines = vtkCellArray::New();
    vtkIdType points[2];
    for (vtkIdType i = 0; i < input->GetNumberOfArcs(); i++)
      {
      points[0] = input->GetSourceNode(i);
      points[1] = input->GetTargetNode(i);
      newLines->InsertNextCell(2, points);
      }

    // Send the data to output.
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);

    // Points correspond to nodes, so pass the data along.
    output->GetPointData()->PassData(input->GetPointData());

    // Cells correspond to arcs, so pass the cell data along.
    output->GetCellData()->PassData(input->GetCellData());

    // Clean up.
    newLines->Delete();
    }
  else
    {
    vtkIdType numArcs = input->GetNumberOfArcs();
    vtkCellData* inputCellData = input->GetCellData();
    vtkCellData* outputCellData = output->GetCellData();
    outputCellData->CopyAllocate(inputCellData);
    vtkCellArray* newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numArcs, 2));
    vtkIdType points[2];

    // Only create lines for non-ghost arcs
    for (vtkIdType i = 0; i < numArcs; i++)
      {
      if (arcGhostLevels->GetComponent(i, 0) == 0) 
        {
        points[0] = input->GetSourceNode(i);
        points[1] = input->GetTargetNode(i);
        vtkIdType ind = newLines->InsertNextCell(2, points);
        outputCellData->CopyData(inputCellData, i, ind);
        }
      } 

    // Send data to output
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);
    output->GetPointData()->PassData(input->GetPointData());

    // Clean up
    newLines->Delete();
    output->Squeeze();
    }

  return 1;
}
