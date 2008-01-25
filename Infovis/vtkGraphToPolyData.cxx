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

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkGraphToPolyData, "1.8");
vtkStandardNewMacro(vtkGraphToPolyData);

vtkGraphToPolyData::vtkGraphToPolyData()
{
  this->EdgeGlyphOutput = false;
  this->EdgeGlyphPosition = 1.0;
  this->SetNumberOfOutputPorts(2);
}

int vtkGraphToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
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
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *arrowOutput = vtkPolyData::SafeDownCast(
    arrowInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* edgeGhostLevels = vtkDataArray::SafeDownCast(
    input->GetEdgeData()->GetAbstractArray("vtkGhostLevels"));

  if (edgeGhostLevels == NULL)
    {
    vtkSmartPointer<vtkIdTypeArray> cells = 
      vtkSmartPointer<vtkIdTypeArray>::New();
    cells->SetNumberOfTuples(3*input->GetNumberOfEdges());
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      cells->SetValue(3*e.Id + 0, 2);
      cells->SetValue(3*e.Id + 1, e.Source);
      cells->SetValue(3*e.Id + 2, e.Target);
      }
    vtkSmartPointer<vtkCellArray> newLines = 
      vtkSmartPointer<vtkCellArray>::New();
    newLines->SetCells(input->GetNumberOfEdges(), cells);

    // Send the data to output.
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);

    // Points correspond to vertices, so pass the data along.
    output->GetPointData()->PassData(input->GetVertexData());

    // Cells correspond to edges, so pass the cell data along.
    output->GetCellData()->PassData(input->GetEdgeData());
    }
  else
    {
    vtkIdType numEdges = input->GetNumberOfEdges();
    vtkDataSetAttributes *inputCellData = input->GetEdgeData();
    vtkCellData *outputCellData = output->GetCellData();
    outputCellData->CopyAllocate(inputCellData);
    vtkSmartPointer<vtkCellArray> newLines = 
      vtkSmartPointer<vtkCellArray>::New();
    newLines->Allocate(newLines->EstimateSize(numEdges, 2));
    vtkIdType points[2];

    // Only create lines for non-ghost edges
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      if (edgeGhostLevels->GetComponent(e.Id, 0) == 0) 
        {
        points[0] = e.Source;
        points[1] = e.Target;
        vtkIdType ind = newLines->InsertNextCell(2, points);
        outputCellData->CopyData(inputCellData, e.Id, ind);
        }
      } 

    // Send data to output
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);
    output->GetPointData()->PassData(input->GetVertexData());

    // Clean up
    output->Squeeze();
    }

  if (this->EdgeGlyphOutput)
    {
    vtkDataSetAttributes *inputCellData = input->GetEdgeData();
    
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
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      if (!edgeGhostLevels || edgeGhostLevels->GetComponent(e.Id, 0) == 0) 
        {
        vtkIdType source = e.Source;
        vtkIdType target = e.Target;
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
          arrowPointData->CopyData(inputCellData, e.Id, ind);
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
