/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiPartExtentTranslator.h
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
// .NAME vtkMultiPartExtentTranslator - Returns the whole extent for any piece.
// .SECTION Description
// This is used when a parallel data set is composed of multple structured
// pieces.

#ifndef __vtkMultiPartExtentTranslator_h
#define __vtkMultiPartExtentTranslator_h

#include "vtkExtentTranslator.h"


class VTK_COMMON_EXPORT vtkMultiPartExtentTranslator : 
  public vtkExtentTranslator
{
public:
  static vtkMultiPartExtentTranslator *New();

  vtkTypeRevisionMacro(vtkMultiPartExtentTranslator,vtkExtentTranslator);

  // Description:
  // These are the main methods that should be called. These methods 
  // are responsible for converting a piece to an extent. The signatures
  // without arguments are only thread safe when each thread accesses a
  // different instance. The signatures with arguements are fully thread
  // safe. 
  virtual int PieceToExtentThreadSafe(int piece, int numPieces, 
                                      int ghostLevel, int *wholeExtent, 
                                      int *resultExtent, int splitMode, 
                                      int byPoints);
  
protected:
  vtkMultiPartExtentTranslator();
  ~vtkMultiPartExtentTranslator();

private:
  vtkMultiPartExtentTranslator(const vtkMultiPartExtentTranslator&);  // Not implemented.
  void operator=(const vtkMultiPartExtentTranslator&);  // Not implemented.
};

#endif

