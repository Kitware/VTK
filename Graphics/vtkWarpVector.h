/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.h
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
// .NAME vtkWarpVector - deform geometry with vector data
// .SECTION Description
// vtkWarpVector is a filter that modifies point coordinates by moving
// points along vector times the scale factor. Useful for showing flow
// profiles or mechanical deformation.
//
// The filter passes both its point data and cell data to its output.

#ifndef __vtkWarpVector_h
#define __vtkWarpVector_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkWarpVector : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpVector *New();
  vtkTypeRevisionMacro(vtkWarpVector,vtkPointSetToPointSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // If you want to warp by an arbitrary vector array, then set its name here.
  // By default this in NULL and the filter will use the active vector array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}
  
protected:
  vtkWarpVector();
  ~vtkWarpVector();

  void Execute();
  float ScaleFactor;

  char *InputVectorsSelection;
  vtkSetStringMacro(InputVectorsSelection);

private:
  vtkWarpVector(const vtkWarpVector&);  // Not implemented.
  void operator=(const vtkWarpVector&);  // Not implemented.
};

#endif
