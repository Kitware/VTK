/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellDataToPointData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCellDataToPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPCellDataToPointData, "1.1");
vtkStandardNewMacro(vtkPCellDataToPointData);

//----------------------------------------------------------------------------
vtkPCellDataToPointData::vtkPCellDataToPointData()
{
  this->PieceInvariant = 1;
}

//----------------------------------------------------------------------------
void vtkPCellDataToPointData::Execute()
{
  vtkDataSet *output = this->GetOutput();

  this->vtkCellDataToPointData::Execute();

  // Remove the extra (now ivalid) ghost cells.
  // This is only necessary fro unstructured data.  
  if (this->PieceInvariant)
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(output);
    vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(output);
    if (pd)
      {
      pd->RemoveGhostCells(pd->GetUpdateGhostLevel()+1);
      }
    if (ug)
      {
      ug->RemoveGhostCells(ug->GetUpdateGhostLevel()+1);
      }
    }                                       
}

//--------------------------------------------------------------------------
void vtkPCellDataToPointData::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();
  int i;

  if (input == NULL)
    {
    return;
    }
  if (this->PieceInvariant == 0)
    { 
    // I believe the default input update extent 
    // is set to the input update extent.
    return;
    }

  switch (input->GetDataObjectType())
    {
    case VTK_POLY_DATA:
    case VTK_UNSTRUCTURED_GRID:
      input->SetUpdatePiece(piece);
      input->SetUpdateNumberOfPieces(numPieces);
      input->SetUpdateGhostLevel(ghostLevel);
      break;
    case VTK_IMAGE_DATA:
    case VTK_STRUCTURED_POINTS:
    case VTK_STRUCTURED_GRID:
    case VTK_RECTILINEAR_GRID:
      {
      int *wholeExt;
      int ext[6];
      wholeExt = input->GetWholeExtent();
      output->GetUpdateExtent(ext);
      for (i = 0; i < 3; ++i)
        {
        --ext[i*2];
        if (ext[i*2] < wholeExt[i*2])
          {
          ext[i*2] = wholeExt[i*2];
          }
        ++ext[i*2+1];
        if (ext[i*2+1] > wholeExt[i*2+1])
          {
          ext[i*2+1] = wholeExt[i*2+1];
          }
        }
      input->SetUpdateExtent(ext);
      }
    }  
}

//----------------------------------------------------------------------------
void vtkPCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PieceInvariant: "
     << this->PieceInvariant << "\n";
}
