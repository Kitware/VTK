/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cutter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCutter - Cut vtkDataSets with user-specified implicit function
// .SECTION Description
// vtkCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = 0.

#ifndef __vtkCutter_h
#define __vtkCutter_h

#include "DS2PolyF.hh"
#include "ImpFunc.hh"

class vtkCutter : public vtkDataSetToPolyFilter
{
public:
  vtkCutter(vtkImplicitFunction *cf=NULL);
  ~vtkCutter();
  char *GetClassName() {return "vtkCutter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  vtkSetObjectMacro(CutFunction,vtkImplicitFunction);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

protected:
  void Execute();
  vtkImplicitFunction *CutFunction;
  
};

#endif


