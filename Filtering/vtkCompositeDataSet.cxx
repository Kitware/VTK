/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkDataSet.h"

vtkCxxRevisionMacro(vtkCompositeDataSet, "1.1.2.1");

//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  this->SetUpdatePiece(piece);
  this->SetUpdateNumberOfPieces(numPieces);
  this->SetUpdateGhostLevel(ghostLevel);
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::GetUpdateExtent(int& piece, int& numPieces, int& ghostLevel)
{
  piece = this->GetUpdatePiece();
  numPieces = this->GetUpdateNumberOfPieces();
  ghostLevel = this->GetUpdateGhostLevel();
}

//----------------------------------------------------------------------------
int* vtkCompositeDataSet::GetUpdateExtent()
{
  return this->Superclass::GetUpdateExtent();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::GetUpdateExtent(int& x0, int& x1, int& y0, int& y1,
                                          int& z0, int& z1)
{
  this->Superclass::GetUpdateExtent(x0, x1, y0, y1, z0, z1);
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::GetUpdateExtent(int extent[6])
{
  this->Superclass::GetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  // this->UpdateExtent
  this->Superclass::PrintSelf(os,indent);
}

