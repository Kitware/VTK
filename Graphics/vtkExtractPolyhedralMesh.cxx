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
#include "vtkGenericCell.h"
#include "vtkCellArray.h"

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

  // Initialize algorithms
  vtkDebugMacro(<<"Executing polyhedral extraction filter");

  // Note that point and cell data are passed through. Points are passed
  // through if the input is a vtkPointSet. Otherwise points have to be
  // extracted and generated.
  vtkPoints *newPts=NULL;
  int pointSetInput;
  vtkIdType numCells = input->GetNumberOfCells();
  output->Allocate(numCells);
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  outputPD->PassData(pd);
  outputCD->PassData(cd);

  // If the input is a point set, then the input points can be passed to
  // the output.
  if ( vtkPointSet::IsTypeOf("vtkPointSet") )
    {
    pointSetInput = 1;
    vtkPointSet *ptSet = static_cast<vtkPointSet*>(input);
    output->SetPoints(ptSet->GetPoints());
    }
  else
    {
    pointSetInput = 0;
    newPts = vtkPoints::New();
    newPts->SetNumberOfPoints(input->GetNumberOfPoints());
    }

  // Loop over all cells, checking topological dimension. If a 3D cell, then
  // grap its faces and construct a polyhedron cell. If topological dimension
  // two or less, pass to output if requested.
  int npts;
  double x[3];
  vtkCell *face;
  vtkCellArray *cellArray = vtkCellArray::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  int abort=0, progressInterval = numCells/20 + 1;
  vtkIdType ptId, cellId;
  int i, j;
  for(cellId=0; cellId < numCells && !abort; cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
      }

    input->GetCell(cellId,cell);
    if (cell->GetCellType() != VTK_EMPTY_CELL)
      {
      switch (cell->GetCellDimension())
        {
        // Pass thru lover topological cells
        case 0: case 1: case 2:
          if ( this->ExtractNon3DCells )
            {
            output->InsertNextCell(cell->GetCellType(),cell->PointIds);
            }
          break;

        // 3D cells process properly
        case 3:
          cellArray->Reset();
          int numFaces = cell->GetNumberOfFaces();
          for (j=0; j < numFaces; j++)
            {
            face = cell->GetFace(j);
            npts = face->GetNumberOfPoints();
            cellArray->InsertNextCell(face->PointIds);
            for ( i=0; i < npts; i++)
              {
              ptId = face->GetPointId(i);
              input->GetPoint(ptId, x);
              if ( !pointSetInput )
                {
                newPts->SetPoint(ptId,x);
                }
              }//for all points of the faces
            }//for all faces of the cell
          output->InsertNextCell(VTK_POLYHEDRON, cell->PointIds->GetNumberOfIds(), 
            cell->PointIds->GetPointer(0), cellArray->GetNumberOfCells(), 
            cellArray->GetPointer());
          /*
          std::cout << cell->PointIds->GetNumberOfIds() << " " << cellArray->GetNumberOfCells() << std::endl;
          
          for (int i = 0; i < 8; i++)
            {
            std::cout << cell->PointIds->GetPointer(0)[i] << " ";
            }
          std::cout << std::endl;
          for (int i = 0; i < 30; i++)
            {
            std::cout << cellArray->GetPointer()[i] << " ";
            }
          std::cout << std::endl;
          */

          break;
        } //switch
      } //if non-empty cells
    } //for all cells
  
  vtkDebugMacro(<<"Extracted " << input->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Clean up
  cellArray->Delete();
  cell->Delete();
  if ( ! pointSetInput )
    {
    newPts->Delete();
    }

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
