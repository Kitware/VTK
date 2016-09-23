/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapToPolyData.cxx

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
#include "vtkTreeMapToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkTreeMapToPolyData);

vtkTreeMapToPolyData::vtkTreeMapToPolyData()
{
  this->SetRectanglesArrayName("area");
  this->SetLevelArrayName("level");
  this->LevelDeltaZ = 0.001;
  this->AddNormals = true;
}

vtkTreeMapToPolyData::~vtkTreeMapToPolyData()
{
}

int vtkTreeMapToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

int vtkTreeMapToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkTree *inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *outputPoly = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // For each input vertex create 4 points and 1 cell (quad)
  vtkPoints* outputPoints = vtkPoints::New();
  outputPoints->SetNumberOfPoints(inputTree->GetNumberOfVertices()*4);
  vtkCellArray* outputCells = vtkCellArray::New();

  // Create an array for the point normals
  vtkFloatArray* normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(inputTree->GetNumberOfVertices()*4);
  normals->SetName("normals");

  vtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
  {
    vtkErrorMacro("Area array not found.");
    return 0;
  }
  vtkDataArray* levelArray = this->GetInputArrayToProcess(1, inputTree);

  // Now set the point coordinates, normals, and insert the cell
  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    // Grab coords from the input
    double coords[4];
    coordArray->GetTuple(i,coords);

    double z = 0;
    if (levelArray)
    {
      z = this->LevelDeltaZ * levelArray->GetTuple1(i);
    }
    else
    {
      z = this->LevelDeltaZ * inputTree->GetLevel(i);
    }

    int index = i*4;
    outputPoints->SetPoint(index,   coords[0], coords[2], z);
    outputPoints->SetPoint(index+1, coords[1], coords[2], z);
    outputPoints->SetPoint(index+2, coords[1], coords[3], z);
    outputPoints->SetPoint(index+3, coords[0], coords[3], z);

    // Create an asymetric gradient on the cells
    // this gradient helps differentiate same colored
    // cells from their neighbors. The asymetric
    // nature of the gradient is required.
    normals->SetComponent(index,   0, 0);
    normals->SetComponent(index,   1, .707);
    normals->SetComponent(index,   2, .707);

    normals->SetComponent(index+1, 0, 0);
    normals->SetComponent(index+1, 1, .866);
    normals->SetComponent(index+1, 2, .5);

    normals->SetComponent(index+2, 0, 0);
    normals->SetComponent(index+2, 1, .707);
    normals->SetComponent(index+2, 2, .707);

    normals->SetComponent(index+3, 0, 0);
    normals->SetComponent(index+3, 1, 0);
    normals->SetComponent(index+3, 2, 1);


    // Create the cell that uses these points
    vtkIdType cellConn[] = {index, index+1, index+2, index+3};
    outputCells->InsertNextCell(4, cellConn);
  }

  // Pass the input point data to the output cell data :)
  outputPoly->GetCellData()->PassData(inputTree->GetVertexData());

  // Set the output points and cells
  outputPoly->SetPoints(outputPoints);
  outputPoly->SetPolys(outputCells);

  if( this->AddNormals )
  {
      // Set the point normals
    outputPoly->GetPointData()->AddArray(normals);
    outputPoly->GetPointData()->SetActiveNormals("normals");
  }

  // Clean up.
  normals->Delete();
  outputPoints->Delete();
  outputCells->Delete();

  return 1;
}

void vtkTreeMapToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LevelDeltaZ: " << this->LevelDeltaZ << endl;
  os << indent << "AddNormals: " << this->AddNormals << endl;
}
