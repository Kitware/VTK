/*=========================================================================

  Program:   Visualization Library
  Module:    ImpBool.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlImplicitBoolean - implicit function consisting of boolean combinations of implicit functions
// .SECTION Description
// vlImplicitBoolean is an implicit function consisting of boolean combinations
// of implicit functions. The class has a list of functions (FunctionList) that
// are combined according to a specified operator (UNION or INTERSECTION or 
// DIFFERENCE). You can use nested combinations of vlImplicitFunctions 
// (and/or vlImplicitBoolean) to create elaborate implicit functions. 
// vlImplicitBoolean is a concrete implementation of vlImplicitFunction.
//    The operators work as follows. The UNION operator takes the minimum value
// of all implicit functions. The INTERSECTION operator takes the maximum value
// of all implicit functions. The DIFFERENCE operator substracts the 2cnd 
// through last implicit functions from the first.


#ifndef __vlImplicitBoolean_h
#define __vlImplicitBoolean_h

#include "ImpFunc.hh"
#include "ImpFuncC.hh"

#define UNION 0
#define INTERSECTION 1
#define DIFFERENCE 2

class vlImplicitBoolean : public vlImplicitFunction
{
public:
  vlImplicitBoolean();
  ~vlImplicitBoolean();
  char *GetClassName() {return "vlImplicitBoolean";};
  void PrintSelf(ostream& os, vlIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Override modified time retrieval because of object dependencies.
  unsigned long int GetMTime();

  void AddFunction(vlImplicitFunction *in);
  void AddFunction(vlImplicitFunction &in) {this->AddFunction(&in);};
  void RemoveFunction(vlImplicitFunction *in);
  void RemoveFunction(vlImplicitFunction &in) {this->RemoveFunction(&in);};
  vlImplicitFunctionCollection *GetFunction() {return &(this->FunctionList);};

  // Description:
  // Specify the type of boolean operation.
  vlSetClampMacro(OperationType,int,UNION,DIFFERENCE);
  vlGetMacro(OperationType,int);

protected:
  vlImplicitFunctionCollection FunctionList;

  int OperationType;

};

#endif


