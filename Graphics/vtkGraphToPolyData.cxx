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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGraphToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDirectedGraph.h"
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

#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

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

  // get the input and output
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
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    vtkSmartPointer<vtkPoints> newPoints =
      vtkSmartPointer<vtkPoints>::New();
    newPoints->DeepCopy(input->GetPoints());
    output->SetPoints(newPoints);
    vtkIdType numEdges = input->GetNumberOfEdges();
    bool noExtraPoints = true;
    for (vtkIdType e = 0; e < numEdges; ++e)
      {
      vtkIdType npts;
      double* pts;
      input->GetEdgePoints(e, npts, pts);
      vtkIdType source = input->GetSourceVertex(e);
      vtkIdType target = input->GetTargetVertex(e);
      if (npts == 0)
        {
        cells->InsertNextValue(2);
        cells->InsertNextValue(source);
        cells->InsertNextValue(target);
        }
      else
        {
        cells->InsertNextValue(2+npts);
        cells->InsertNextValue(source);
        for (vtkIdType i = 0; i < npts; ++i, pts += 3)
          {
          noExtraPoints = false;
          vtkIdType pt = output->GetPoints()->InsertNextPoint(pts);
          cells->InsertNextValue(pt);
          }
        cells->InsertNextValue(target);
        }
      }
    vtkSmartPointer<vtkCellArray> newLines = 
      vtkSmartPointer<vtkCellArray>::New();
    newLines->SetCells(numEdges, cells);

    // Send the data to output.
    output->SetLines(newLines);

    // Points only correspond to vertices if we didn't add extra points.
    if (noExtraPoints)
      {
      output->GetPointData()->PassData(input->GetVertexData());
      }

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
  os << indent << "EdgeGlyphOutput: "
    << (this->EdgeGlyphOutput ? "on" : "off") << endl;
  os << indent << "EdgeGlyphPosition: " << this->EdgeGlyphPosition << endl;
}
