/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataLIC2DExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataLIC2DExtentTranslator.h"

#include "vtkObjectFactory.h"
#include "vtkImageDataLIC2D.h"

vtkStandardNewMacro(vtkImageDataLIC2DExtentTranslator);
vtkCxxSetObjectMacro(vtkImageDataLIC2DExtentTranslator, InputExtentTranslator, vtkExtentTranslator);
//----------------------------------------------------------------------------
vtkImageDataLIC2DExtentTranslator::vtkImageDataLIC2DExtentTranslator()
{
  this->Algorithm = 0;
  this->InputExtentTranslator = 0;
  this->InputWholeExtent[0] =
    this->InputWholeExtent[1] =
    this->InputWholeExtent[2] =
    this->InputWholeExtent[3] =
    this->InputWholeExtent[4] =
    this->InputWholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
vtkImageDataLIC2DExtentTranslator::~vtkImageDataLIC2DExtentTranslator()
{
  this->SetInputExtentTranslator(0);
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2DExtentTranslator::SetAlgorithm(
  vtkImageDataLIC2D* alg)
{
  if (this->Algorithm.GetPointer() != alg)
    {
    this->Algorithm = alg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkImageDataLIC2D* vtkImageDataLIC2DExtentTranslator::GetAlgorithm()
{
  return this->Algorithm.GetPointer();
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2DExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces,
                                     int ghostLevel, int *wholeExtent,
                                     int *resultExtent, int splitMode,
                                     int byPoints)
{
  if (!this->Algorithm)
    {
    return this->Superclass::PieceToExtentThreadSafe(piece, numPieces, ghostLevel, wholeExtent,
      resultExtent, splitMode, byPoints);
    }

  // Let the input extent translator do the translation.
  int inExt[6];
  this->InputExtentTranslator->PieceToExtentThreadSafe(piece, numPieces,
    ghostLevel, this->InputWholeExtent, inExt, splitMode, byPoints);
  this->Algorithm->TranslateInputExtent(inExt, this->InputWholeExtent, resultExtent);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2DExtentTranslator::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Algorithm: "               << this->Algorithm << endl;
  os << indent << "InputWholeExtent: ("
               << this->InputWholeExtent[0]   << ", "
               << this->InputWholeExtent[1]   << ", "
               << this->InputWholeExtent[2]   << ", "
               << this->InputWholeExtent[3]   << ", "
               << this->InputWholeExtent[4]   << ", "
               << this->InputWholeExtent[5]   << ")" << endl;
  os << indent << "InputExtentTranslator: "
               << this->InputExtentTranslator << endl;
}
