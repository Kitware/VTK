/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBranchExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBranchExtentTranslator.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkBranchExtentTranslator);

vtkCxxSetObjectMacro(vtkBranchExtentTranslator,OriginalSource,vtkImageData);

//----------------------------------------------------------------------------
vtkBranchExtentTranslator::vtkBranchExtentTranslator()
{
  this->OriginalSource = NULL;
  this->AssignedPiece = 0;
  this->AssignedNumberOfPieces = 1;
}

//----------------------------------------------------------------------------
vtkBranchExtentTranslator::~vtkBranchExtentTranslator()
{
  this->SetOriginalSource(NULL);
}

//----------------------------------------------------------------------------
int vtkBranchExtentTranslator::PieceToExtent()
{
  //cerr << this << " PieceToExtent: " << this->Piece << " of " << this->NumberOfPieces << endl;
  //cerr << "OriginalData: " << this->OriginalSource << endl;
  //cerr << "OriginalData is of type " << this->OriginalSource->GetClassName() << endl;
  //cerr << "OriginalSource: " << this->OriginalSource->GetSource() << endl;
  //cerr << "OriginalSource is of type: " << this->OriginalSource->GetSource()->GetClassName() << endl;
  
  
  if (this->OriginalSource == NULL)
    { // If the user has not set the original source, then just default
    // to the method in the superclass.
    return this->vtkExtentTranslator::PieceToExtent();
    }

  this->OriginalSource->UpdateInformation();
  this->OriginalSource->GetWholeExtent(this->Extent);

  //cerr << this << "WholeExtent: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
  //     << this->Extent[2] << ", " << this->Extent[3] << ", " 
  //     << this->Extent[4] << ", " << this->Extent[5] << endl;
  
  if (this->SplitExtent(this->Piece, this->NumberOfPieces, this->Extent, 3) == 0)
    {
    //cerr << "Split thinks nothing is in the piece" << endl;
    //cerr << this << " Split: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
    //   << this->Extent[2] << ", " << this->Extent[3] << ", "
    //   << this->Extent[4] << ", " << this->Extent[5] << endl;    
    // Nothing in this piece.
    this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
    this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
    //cerr << this << " Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
    //   << this->Extent[2] << ", " << this->Extent[3] << ", " 
    //   << this->Extent[4] << ", " << this->Extent[5] << endl;    
    return 0;
    }

  //cerr << this << " Split: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
  //     << this->Extent[2] << ", " << this->Extent[3] << ", " 
  //     << this->Extent[4] << ", " << this->Extent[5] << endl;    
  
  // Clip with the whole extent passed in.
  if (this->Extent[0] < this->WholeExtent[0])
    {
    this->Extent[0] = this->WholeExtent[0];
    }  
  if (this->Extent[1] > this->WholeExtent[1])
    {
    this->Extent[1] = this->WholeExtent[1];
    }
  if (this->Extent[2] < this->WholeExtent[2])
    {
    this->Extent[2] = this->WholeExtent[2];
    }  
  if (this->Extent[3] > this->WholeExtent[3])
    {
    this->Extent[3] = this->WholeExtent[3];
    }
  if (this->Extent[4] < this->WholeExtent[4])
    {
    this->Extent[4] = this->WholeExtent[4];
    }  
  if (this->Extent[5] > this->WholeExtent[5])
    {
    this->Extent[5] = this->WholeExtent[5];
    }
  
  
  if (this->Extent[0] > this->Extent[1] ||
      this->Extent[2] > this->Extent[3] ||
      this->Extent[4] > this->Extent[5])
    {
    this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
    this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
    //cerr << this << " Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
    //   << this->Extent[2] << ", " << this->Extent[3] << ", " 
    //   << this->Extent[4] << ", " << this->Extent[5] << endl;
    return 0;
    }  
  
  //cerr << this << " Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", " 
  //     << this->Extent[2] << ", " << this->Extent[3] << ", " 
  //     << this->Extent[4] << ", " << this->Extent[5] << endl;
  
  return 1;
}



//----------------------------------------------------------------------------
void vtkBranchExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Original Source: (" << this->OriginalSource << ")\n";

  os << indent << "AssignedPiece: " << this->AssignedPiece << endl;
  os << indent << "AssignedNumberOfPieces: " << this->AssignedNumberOfPieces << endl;
}







