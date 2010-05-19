/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImplicitBoolean - implicit function consisting of boolean combinations of implicit functions
// .SECTION Description
// vtkImplicitBoolean is an implicit function consisting of boolean
// combinations of implicit functions. The class has a list of functions
// (FunctionList) that are combined according to a specified operator
// (VTK_UNION or VTK_INTERSECTION or VTK_DIFFERENCE). You can use nested
// combinations of vtkImplicitFunction's (and/or vtkImplicitBoolean) to create
// elaborate implicit functions.  vtkImplicitBoolean is a concrete
// implementation of vtkImplicitFunction.
//
// The operators work as follows. The VTK_UNION operator takes the minimum
// value of all implicit functions. The VTK_INTERSECTION operator takes the
// maximum value of all implicit functions. The VTK_DIFFERENCE operator
// subtracts the 2nd through last implicit functions from the first. The
// VTK_UNION_OF_MAGNITUDES takes the minimum absolute value of the 
// implicit functions.

#ifndef __vtkImplicitBoolean_h
#define __vtkImplicitBoolean_h

#include "vtkImplicitFunction.h"
 
class vtkImplicitFunctionCollection;

#define VTK_UNION 0
#define VTK_INTERSECTION 1
#define VTK_DIFFERENCE 2
#define VTK_UNION_OF_MAGNITUDES 3

class VTK_FILTERING_EXPORT vtkImplicitBoolean : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitBoolean,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Default boolean method is union.
  static vtkImplicitBoolean *New();

  // Description:
  // Evaluate boolean combinations of implicit function using current operator.
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate gradient of boolean combination.
  void EvaluateGradient(double x[3], double g[3]);

  // Description:
  // Override modified time retrieval because of object dependencies.
  unsigned long GetMTime();

  // Description:
  // Add another implicit function to the list of functions.
  void AddFunction(vtkImplicitFunction *in);

  // Description:
  // Remove a function from the list of implicit functions to boolean.
  void RemoveFunction(vtkImplicitFunction *in);

  // Description:
  // Return the collection of implicit functions.
  vtkImplicitFunctionCollection *GetFunction() {return this->FunctionList;};

  // Description:
  // Specify the type of boolean operation.
  vtkSetClampMacro(OperationType,int,VTK_UNION,VTK_UNION_OF_MAGNITUDES);
  vtkGetMacro(OperationType,int);
  void SetOperationTypeToUnion() 
    {this->SetOperationType(VTK_UNION);};
  void SetOperationTypeToIntersection() 
    {this->SetOperationType(VTK_INTERSECTION);};
  void SetOperationTypeToDifference() 
    {this->SetOperationType(VTK_DIFFERENCE);};
  void SetOperationTypeToUnionOfMagnitudes() 
    {this->SetOperationType(VTK_UNION_OF_MAGNITUDES);};
  const char *GetOperationTypeAsString();
  
protected:
  vtkImplicitBoolean();
  ~vtkImplicitBoolean();

  vtkImplicitFunctionCollection *FunctionList;

  int OperationType;

private:
  vtkImplicitBoolean(const vtkImplicitBoolean&);  // Not implemented.
  void operator=(const vtkImplicitBoolean&);  // Not implemented.
};

// Description:
// Return the boolean operation type as a descriptive character string.
inline const char *vtkImplicitBoolean::GetOperationTypeAsString(void)
{
  if ( this->OperationType == VTK_UNION )
    {
    return "Union";
    }
  else if ( this->OperationType == VTK_INTERSECTION ) 
    {
    return "Intersection";
    }
  else if ( this->OperationType == VTK_DIFFERENCE ) 
    {
    return "Difference";
    }
  else 
    {
    return "UnionOfMagnitudes";
    }
}

#endif


