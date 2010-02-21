/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractPolyhedralMesh.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkExtractPolyhedralMesh, "$Revision$");
vtkStandardNewMacro(vtkExtractPolyhedralMesh);

//----------------------------------------------------------------------------
// Construct object with ExtractInside turned on.
vtkExtractPolyhedralMesh::vtkExtractPolyhedralMesh()
{
  this->ExtractNon3DCells = 1;
}

//----------------------------------------------------------------------------
vtkExtractPolyhedralMesh::~vtkExtractPolyhedralMesh()
{
}

//----------------------------------------------------------------------------
int vtkExtractPolyhedralMesh::RequestData(
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
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Loop over all cells, checking topological dimension. If a 3D cell, then
  // grap its faces and construct a polyhedron cell. If topological dimension
  // two or less, pass to output if requested.

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractPolyhedralMesh::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractPolyhedralMesh::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extract Non-3D Cells: " 
     << (this->ExtractNon3DCells ? "On\n" : "Off\n");
}
