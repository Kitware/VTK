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
/**
 * @class   vtkImplicitBoolean
 * @brief   implicit function consisting of boolean combinations of implicit functions
 *
 * vtkImplicitBoolean is an implicit function consisting of boolean
 * combinations of implicit functions. The class has a list of functions
 * (FunctionList) that are combined according to a specified operator
 * (VTK_UNION or VTK_INTERSECTION or VTK_DIFFERENCE). You can use nested
 * combinations of vtkImplicitFunction's (and/or vtkImplicitBoolean) to create
 * elaborate implicit functions.  vtkImplicitBoolean is a concrete
 * implementation of vtkImplicitFunction.
 *
 * The operators work as follows. The VTK_UNION operator takes the minimum
 * value of all implicit functions. The VTK_INTERSECTION operator takes the
 * maximum value of all implicit functions. The VTK_DIFFERENCE operator
 * subtracts the 2nd through last implicit functions from the first. The
 * VTK_UNION_OF_MAGNITUDES takes the minimum absolute value of the
 * implicit functions.
*/

#ifndef vtkImplicitBoolean_h
#define vtkImplicitBoolean_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkImplicitFunctionCollection;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitBoolean : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitBoolean,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum OperationType
  {
    VTK_UNION=0,
    VTK_INTERSECTION,
    VTK_DIFFERENCE,
    VTK_UNION_OF_MAGNITUDES
  };

  /**
   * Default boolean method is union.
   */
  static vtkImplicitBoolean *New();

  //@{
  /**
   * Evaluate boolean combinations of implicit function using current operator.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  //@}

  /**
   * Evaluate gradient of boolean combination.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  /**
   * Override modified time retrieval because of object dependencies.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Add another implicit function to the list of functions.
   */
  void AddFunction(vtkImplicitFunction *in);

  /**
   * Remove a function from the list of implicit functions to boolean.
   */
  void RemoveFunction(vtkImplicitFunction *in);

  /**
   * Return the collection of implicit functions.
   */
  vtkImplicitFunctionCollection *GetFunction() {return this->FunctionList;};

  //@{
  /**
   * Specify the type of boolean operation.
   */
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
  //@}

protected:
  vtkImplicitBoolean();
  ~vtkImplicitBoolean() override;

  vtkImplicitFunctionCollection *FunctionList;

  int OperationType;

private:
  vtkImplicitBoolean(const vtkImplicitBoolean&) = delete;
  void operator=(const vtkImplicitBoolean&) = delete;
};

//@{
/**
 * Return the boolean operation type as a descriptive character string.
 */
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
//@}

#endif


