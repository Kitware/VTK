/*=========================================================================

  Program:   Visualization Library
  Module:    ExtractG.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlExtractGeomtry - extract cells that lie either entirely in or outside of a specified implicit function
// .SECTION Description
// vlExtractGeometry extracts from its input dataset all cells that are either
// completely inside or outside of a specified implicit function. Any type of
// dataset can be input to this filter. On output the filter generates an
// unstructured grid.

#ifndef __vlExtractGeometry_h
#define __vlExtractGeometry_h

#include "DS2UGrid.hh"
#include "ImpFunc.hh"

class vlExtractGeometry : public vlDataSetToUnstructuredGridFilter
{
public:
  vlExtractGeometry(vlImplicitFunction *f=NULL);
  ~vlExtractGeometry() {};
  char *GetClassName() {return "vlExtractGeometry";};
  void PrintSelf(ostream& os, vlIndent indent);

  // take into account changes to the implicit function
  unsigned long int GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks.
  vlSetObjectMacro(ImplicitFunction,vlImplicitFunction);
  vlGetObjectMacro(ImplicitFunction,vlImplicitFunction);

  // Description:
  // Boolean controls whether to extract cells that are inside of implicit 
  // function (ExtractInside == 1) or outside of implicit function 
  // (ExtractInside == 0).
  vlSetMacro(ExtractInside,int);
  vlGetMacro(ExtractInside,int);
  vlBooleanMacro(ExtractInside,int);

protected:
  // Usual data generation method
  void Execute();

  vlImplicitFunction *ImplicitFunction;
  int ExtractInside;
};

#endif


