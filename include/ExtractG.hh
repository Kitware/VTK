/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExtractG.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkExtractGeomtry - extract cells that lie either entirely in or outside of a specified implicit function
// .SECTION Description
// vtkExtractGeometry extracts from its input dataset all cells that are either
// completely inside or outside of a specified implicit function. Any type of
// dataset can be input to this filter. On output the filter generates an
// unstructured grid.

#ifndef __vtkExtractGeometry_h
#define __vtkExtractGeometry_h

#include "DS2UGrid.hh"
#include "ImpFunc.hh"

class vtkExtractGeometry : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkExtractGeometry(vtkImplicitFunction *f=NULL);
  ~vtkExtractGeometry() {};
  char *GetClassName() {return "vtkExtractGeometry";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // take into account changes to the implicit function
  unsigned long int GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Boolean controls whether to extract cells that are inside of implicit 
  // function (ExtractInside == 1) or outside of implicit function 
  // (ExtractInside == 0).
  vtkSetMacro(ExtractInside,int);
  vtkGetMacro(ExtractInside,int);
  vtkBooleanMacro(ExtractInside,int);

protected:
  // Usual data generation method
  void Execute();

  vtkImplicitFunction *ImplicitFunction;
  int ExtractInside;
};

#endif


