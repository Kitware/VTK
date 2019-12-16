/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearCellExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLinearCellExtrusionFilter.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPolygon.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <vector>

vtkStandardNewMacro(vtkLinearCellExtrusionFilter);

//----------------------------------------------------------------------------
vtkLinearCellExtrusionFilter::vtkLinearCellExtrusionFilter()
{
  // set default array
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int vtkLinearCellExtrusionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);
  vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);

  vtkCellArray* polys = input->GetPolys();
  vtkNew<vtkPoints> outputPoints;
  outputPoints->DeepCopy(input->GetPoints());

  output->SetPoints(outputPoints);
  output->GetCellData()->ShallowCopy(input->GetCellData());

  if (this->MergeDuplicatePoints)
  {
    this->CreateDefaultLocator();
    this->Locator->SetDataSet(output);
    this->Locator->InitPointInsertion(outputPoints, outputPoints->GetBounds());

    for (vtkIdType i = 0; i < outputPoints->GetNumberOfPoints(); i++)
    {
      vtkIdType dummy;
      this->Locator->InsertUniquePoint(outputPoints->GetPoint(i), dummy);
    }
  }

  vtkDataArray* inputNormals = input->GetCellData()->GetNormals();

  output->Allocate(polys->GetSize() * 2); // estimation

  std::vector<std::array<double, 3> > topPoints;
  std::vector<vtkIdType> polyhedronIds; // used for polyhedrons

  vtkIdType cellId = 0;
  auto iter = vtk::TakeSmartPointer(polys->NewIterator());
  for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), cellId++)
  {
    vtkIdType cellSize;
    const vtkIdType* cellPoints;
    iter->GetCurrentCell(cellSize, cellPoints);

    topPoints.resize(cellSize);
    for (vtkIdType i = 0; i < cellSize; i++)
    {
      outputPoints->GetPoint(cellPoints[i], topPoints[i].data());
    }

    double normal[3];
    if (this->UseUserVector)
    {
      normal[0] = this->UserVector[0];
      normal[1] = this->UserVector[1];
      normal[2] = this->UserVector[2];
    }
    else
    {
      if (inputNormals)
      {
        inputNormals->GetTuple(cellId, normal);
      }
      else
      {
        vtkPolygon::ComputeNormal(cellSize, topPoints[0].data(), normal);
      }
    }

    // extrude
    double currentValue = (array ? array->GetComponent(cellId, 0) : 1.0);
    double scale = currentValue * this->ScaleFactor;
    for (vtkIdType i = 0; i < cellSize; i++)
    {
      auto& p = topPoints[i];
      p[0] += scale * normal[0];
      p[1] += scale * normal[1];
      p[2] += scale * normal[2];
    }

    if (cellSize == 3) // triangle => wedge
    {
      vtkIdType newPts[3];

      if (this->MergeDuplicatePoints)
      {
        this->Locator->InsertUniquePoint(topPoints[0].data(), newPts[0]);
        this->Locator->InsertUniquePoint(topPoints[1].data(), newPts[1]);
        this->Locator->InsertUniquePoint(topPoints[2].data(), newPts[2]);
      }
      else
      {
        newPts[0] = outputPoints->InsertNextPoint(topPoints[0].data());
        newPts[1] = outputPoints->InsertNextPoint(topPoints[1].data());
        newPts[2] = outputPoints->InsertNextPoint(topPoints[2].data());
      }

      vtkIdType ptsId[6] = { cellPoints[2], cellPoints[1], cellPoints[0], newPts[2], newPts[1],
        newPts[0] };

      output->InsertNextCell(VTK_WEDGE, 6, ptsId);
    }
    else if (cellSize == 4) // quad => hexahedron
    {
      vtkIdType newPts[4];

      if (this->MergeDuplicatePoints)
      {
        this->Locator->InsertUniquePoint(topPoints[0].data(), newPts[0]);
        this->Locator->InsertUniquePoint(topPoints[1].data(), newPts[1]);
        this->Locator->InsertUniquePoint(topPoints[2].data(), newPts[2]);
        this->Locator->InsertUniquePoint(topPoints[3].data(), newPts[3]);
      }
      else
      {
        newPts[0] = outputPoints->InsertNextPoint(topPoints[0].data());
        newPts[1] = outputPoints->InsertNextPoint(topPoints[1].data());
        newPts[2] = outputPoints->InsertNextPoint(topPoints[2].data());
        newPts[3] = outputPoints->InsertNextPoint(topPoints[3].data());
      }

      vtkIdType ptsId[8] = { cellPoints[3], cellPoints[2], cellPoints[1], cellPoints[0], newPts[3],
        newPts[2], newPts[1], newPts[0] };

      output->InsertNextCell(VTK_HEXAHEDRON, 8, ptsId);
    }
    else // generic case => polyhedron
    {
      polyhedronIds.resize(2 * (cellSize + 1) + cellSize * 5);

      vtkIdType* topFace = polyhedronIds.data();
      vtkIdType* baseFace = topFace + cellSize + 1;

      topFace[0] = cellSize;
      baseFace[0] = cellSize;
      for (vtkIdType i = 0; i < cellSize; i++)
      {
        if (this->MergeDuplicatePoints)
        {
          this->Locator->InsertUniquePoint(topPoints[i].data(), topFace[i + 1]);
        }
        else
        {
          topFace[i + 1] = outputPoints->InsertNextPoint(topPoints[i].data());
        }
        baseFace[i + 1] = cellPoints[cellSize - i - 1];
      }

      for (vtkIdType i = 0; i < cellSize; i++)
      {
        vtkIdType* currentSide = polyhedronIds.data() + 2 * (cellSize + 1) + 5 * i;
        currentSide[0] = 4;
        currentSide[1] = topFace[1 + (i + 1) % cellSize];
        currentSide[2] = topFace[1 + i];
        currentSide[3] = cellPoints[i];
        currentSide[4] = cellPoints[(i + 1) % cellSize];
      }

      output->InsertNextCell(VTK_POLYHEDRON, cellSize + 2, polyhedronIds.data());

      if (cellId % 1000 == 0)
      {
        this->UpdateProgress(cellId / static_cast<double>(polys->GetNumberOfCells()));
      }
    }
  }

  output->Squeeze();

  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkLinearCellExtrusionFilter::CreateDefaultLocator()
{
  if (!this->Locator)
  {
    this->Locator = vtkSmartPointer<vtkMergePoints>::New();
  }
}

//----------------------------------------------------------------------------
void vtkLinearCellExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleFactor: " << this->ScaleFactor << "\n"
     << indent << "UserVector: " << this->UserVector[0] << " " << this->UserVector[1] << " "
     << this->UserVector[2] << "\n"
     << indent << "UseUserVector: " << (this->UseUserVector ? "ON" : "OFF") << "\n"
     << indent << "MergeDuplicatePoints: " << (this->MergeDuplicatePoints ? "ON" : "OFF") << endl;
}

//----------------------------------------------------------------------------
int vtkLinearCellExtrusionFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}
