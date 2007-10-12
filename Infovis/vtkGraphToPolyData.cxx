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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraphToPolyData.h"

#include "vtkAbstractGraph.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkGraphToPolyData, "1.7");
vtkStandardNewMacro(vtkGraphToPolyData);

vtkGraphToPolyData::vtkGraphToPolyData()
{
  this->EdgeGlyphOutput = false;
  this->EdgeGlyphPosition = 1.0;
  this->SetNumberOfOutputPorts(2);
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
  vtkInformation *arrowInfo = outputVector->GetInformationObject(1);

  // get the input and ouptut
  vtkAbstractGraph *input = vtkAbstractGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *arrowOutput = vtkPolyData::SafeDownCast(
    arrowInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* edgeGhostLevels = vtkDataArray::SafeDownCast(
    input->GetCellData()->GetAbstractArray("vtkGhostLevels"));

  if (edgeGhostLevels == NULL)
    {
    vtkCellArray* newLines = vtkCellArray::New();
    vtkIdType points[2];
    for (vtkIdType i = 0; i < input->GetNumberOfEdges(); i++)
      {
      points[0] = input->GetSourceVertex(i);
      points[1] = input->GetTargetVertex(i);
      newLines->InsertNextCell(2, points);
      }

    // Send the data to output.
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);

    // Points correspond to vertices, so pass the data along.
    output->GetPointData()->PassData(input->GetPointData());

    // Cells correspond to edges, so pass the cell data along.
    output->GetCellData()->PassData(input->GetCellData());

    // Clean up.
    newLines->Delete();
    }
  else
    {
    vtkIdType numEdges = input->GetNumberOfEdges();
    vtkCellData* inputCellData = input->GetCellData();
    vtkCellData* outputCellData = output->GetCellData();
    outputCellData->CopyAllocate(inputCellData);
    vtkCellArray* newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numEdges, 2));
    vtkIdType points[2];

    // Only create lines for non-ghost edges
    for (vtkIdType i = 0; i < numEdges; i++)
      {
      if (edgeGhostLevels->GetComponent(i, 0) == 0) 
        {
        points[0] = input->GetSourceVertex(i);
        points[1] = input->GetTargetVertex(i);
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

  if (this->EdgeGlyphOutput)
    {
    vtkIdType numEdges = input->GetNumberOfEdges();
    vtkCellData* inputCellData = input->GetCellData();
    
    vtkPointData* arrowPointData = arrowOutput->GetPointData();
    arrowPointData->CopyAllocate(inputCellData);
    vtkPoints* newPoints = vtkPoints::New();
    arrowOutput->SetPoints(newPoints);
    newPoints->Delete();
    vtkDoubleArray* orientArr = vtkDoubleArray::New();
    orientArr->SetNumberOfComponents(3);
    orientArr->SetName("orientation");
    arrowPointData->AddArray(orientArr);
    arrowPointData->SetVectors(orientArr);
    orientArr->Delete();
    double sourcePt[3] = {0, 0, 0};
    double targetPt[3] = {0, 0, 0};
    double pt[3] = {0, 0, 0};
    double orient[3] = {0, 0, 0};
    for (vtkIdType i = 0; i < numEdges; i++)
      {
      if (!edgeGhostLevels || edgeGhostLevels->GetComponent(i, 0) == 0) 
        {
        vtkIdType source = input->GetSourceVertex(i);
        vtkIdType target = input->GetTargetVertex(i);
        // Do not render arrows for self loops.
        if (source != target)
          {
          input->GetPoint(source, sourcePt);
          input->GetPoint(target, targetPt);
          for (int j = 0; j < 3; j++)
            {
            pt[j] = (1 - this->EdgeGlyphPosition)*sourcePt[j] + this->EdgeGlyphPosition*targetPt[j];
            orient[j] = targetPt[j] - sourcePt[j];
            }
          vtkIdType ind = newPoints->InsertNextPoint(pt);
          orientArr->InsertNextTuple(orient);
          arrowPointData->CopyData(inputCellData, i, ind);
          }
        }
      }    
    }

  return 1;
}

void vtkGraphToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EdgeGlyphOutput: " << (this->EdgeGlyphOutput ? "on" : "off") << endl;
  os << indent << "EdgeGlyphPosition: " << this->EdgeGlyphPosition << endl;
}
