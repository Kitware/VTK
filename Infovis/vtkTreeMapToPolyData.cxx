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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkTreeMapToPolyData.h"

#include "vtkAbstractGraph.h"
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

vtkCxxRevisionMacro(vtkTreeMapToPolyData, "1.5");
vtkStandardNewMacro(vtkTreeMapToPolyData);

vtkTreeMapToPolyData::vtkTreeMapToPolyData()
{
  this->RectanglesFieldName = 0;
  this->SetRectanglesFieldName("rectangles");
  this->LevelsFieldName = 0;
  this->LevelDeltaZ = 0.001;
}

vtkTreeMapToPolyData::~vtkTreeMapToPolyData()
{
  this->SetRectanglesFieldName(0);
  this->SetLevelsFieldName(0);
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

  vtkDataArray* levelArray = NULL;
  if (this->LevelsFieldName)
    {
    levelArray = inputTree->GetPointData()->GetArray(this->LevelsFieldName);
    }
  
  // Now set the point coordinates, normals, and insert the cell
  vtkDataArray *coordArray = inputTree->GetPointData()->GetArray(this->RectanglesFieldName);
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
  outputPoly->GetCellData()->PassData(inputTree->GetPointData());
  
  // Set the output points and cells
  outputPoly->SetPoints(outputPoints);
  outputPoly->SetPolys(outputCells);

  // Set the point normals
  outputPoly->GetPointData()->AddArray(normals);
  outputPoly->GetPointData()->SetActiveNormals("normals");

  // Clean up.
  normals->Delete();
  outputPoints->Delete();
  outputCells->Delete();
  
  return 1;
}

void vtkTreeMapToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LevelsFieldName: " << (this->LevelsFieldName ? this->LevelsFieldName : "(none)") << endl;
  os << indent << "RectanglesFieldName: " << (this->RectanglesFieldName ? this->RectanglesFieldName : "(none)") << endl;
  os << indent << "LevelDeltaZ: " << this->LevelDeltaZ << endl;
}
