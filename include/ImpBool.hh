/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpBool.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkImplicitBoolean - implicit function consisting of boolean combinations of implicit functions
// .SECTION Description
// vtkImplicitBoolean is an implicit function consisting of boolean combinations
// of implicit functions. The class has a list of functions (FunctionList) that
// are combined according to a specified operator (UNION or INTERSECTION or 
// DIFFERENCE). You can use nested combinations of vtkImplicitFunctions 
// (and/or vtkImplicitBoolean) to create elaborate implicit functions. 
// vtkImplicitBoolean is a concrete implementation of vtkImplicitFunction.
//    The operators work as follows. The UNION operator takes the minimum value
// of all implicit functions. The INTERSECTION operator takes the maximum value
// of all implicit functions. The DIFFERENCE operator substracts the 2cnd 
// through last implicit functions from the first.


#ifndef __vtkImplicitBoolean_h
#define __vtkImplicitBoolean_h

#include "ImpFunc.hh"
#include "ImpFuncC.hh"

#define UNION 0
#define INTERSECTION 1
#define DIFFERENCE 2

class vtkImplicitBoolean : public vtkImplicitFunction
{
public:
  vtkImplicitBoolean();
  ~vtkImplicitBoolean();
  char *GetClassName() {return "vtkImplicitBoolean";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Override modified time retrieval because of object dependencies.
  unsigned long int GetMTime();

  void AddFunction(vtkImplicitFunction *in);
  void AddFunction(vtkImplicitFunction &in) {this->AddFunction(&in);};
  void RemoveFunction(vtkImplicitFunction *in);
  void RemoveFunction(vtkImplicitFunction &in) {this->RemoveFunction(&in);};
  vtkImplicitFunctionCollection *GetFunction() {return &(this->FunctionList);};

  // Description:
  // Specify the type of boolean operation.
  vtkSetClampMacro(OperationType,int,UNION,DIFFERENCE);
  vtkGetMacro(OperationType,int);

protected:
  vtkImplicitFunctionCollection FunctionList;

  int OperationType;

};

#endif


