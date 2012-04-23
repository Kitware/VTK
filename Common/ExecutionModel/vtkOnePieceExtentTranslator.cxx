/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOnePieceExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOnePieceExtentTranslator.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOnePieceExtentTranslator);
//-----------------------------------------------------------------------------
vtkOnePieceExtentTranslator::vtkOnePieceExtentTranslator()
{
}

//-----------------------------------------------------------------------------
vtkOnePieceExtentTranslator::~vtkOnePieceExtentTranslator()
{
}

//-----------------------------------------------------------------------------
int vtkOnePieceExtentTranslator::PieceToExtentThreadSafe(
  int vtkNotUsed(piece),
  int vtkNotUsed(numPieces),
  int vtkNotUsed(ghostLevel),
  int *wholeExtent, int *resultExtent,
  int vtkNotUsed(splitMode),
  int vtkNotUsed(byPoints))
{
  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkOnePieceExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
