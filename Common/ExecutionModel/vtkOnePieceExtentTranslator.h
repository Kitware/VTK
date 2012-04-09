/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOnePieceExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOnePieceExtentTranslator - Returns the whole extent for any piece..
// vtkOnePieceExtentTranslator returns the whole extent for any piece.

#ifndef __vtkOnePieceExtentTranslator_h
#define __vtkOnePieceExtentTranslator_h

#include "vtkExtentTranslator.h"

class VTK_COMMON_EXPORT vtkOnePieceExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkOnePieceExtentTranslator* New();
  vtkTypeMacro(vtkOnePieceExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOnePieceExtentTranslator();
  ~vtkOnePieceExtentTranslator();

  virtual int PieceToExtentThreadSafe(int vtkNotUsed(piece), 
                                      int vtkNotUsed(numPieces), 
                                      int vtkNotUsed(ghostLevel), 
                                      int *wholeExtent, int *resultExtent, 
                                      int vtkNotUsed(splitMode), 
                                      int vtkNotUsed(byPoints));
private:
  vtkOnePieceExtentTranslator(const vtkOnePieceExtentTranslator&); // Not implemented.
  void operator=(const vtkOnePieceExtentTranslator&); // Not implemented.
  
};

#endif

