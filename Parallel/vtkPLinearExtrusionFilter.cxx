/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLinearExtrusionFilter.cxx
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
#include "vtkPLinearExtrusionFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPLinearExtrusionFilter, "1.3");
vtkStandardNewMacro(vtkPLinearExtrusionFilter);

//----------------------------------------------------------------------------
vtkPLinearExtrusionFilter::vtkPLinearExtrusionFilter()
{
  // Since I do not thing the visual impact of invariance is significant, 
  // we will default to not spend the extra effort to get the 
  // extra layer of ghost cells.
  this->PieceInvariant = 0;
}

//----------------------------------------------------------------------------
void vtkPLinearExtrusionFilter::Execute()
{
  vtkPolyData *output = this->GetOutput();

  this->vtkLinearExtrusionFilter::Execute();

  if (this->PieceInvariant)
    {
    output->RemoveGhostCells(output->GetUpdateGhostLevel() + 1);
    }
}


//--------------------------------------------------------------------------
void 
vtkPLinearExtrusionFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkPolyData *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }
  if (this->PieceInvariant)
    {
    input->SetUpdatePiece(piece);
    input->SetUpdateNumberOfPieces(numPieces);
    input->SetUpdateGhostLevel(ghostLevel + 1);
    }
  else
    {
    input->SetUpdatePiece(piece);
    input->SetUpdateNumberOfPieces(numPieces);
    input->SetUpdateGhostLevel(ghostLevel);
    }
}


//----------------------------------------------------------------------------
void vtkPLinearExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PieceInvariant: "
     << this->PieceInvariant << "\n";
}

