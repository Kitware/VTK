/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
// subtracts the 2nd through last implicit functions from the first.


#ifndef __vtkImplicitBoolean_h
#define __vtkImplicitBoolean_h

#include "vtkImplicitFunction.h"
#include "vtkImplicitFunctionCollection.h"

#define VTK_UNION 0
#define VTK_INTERSECTION 1
#define VTK_DIFFERENCE 2
#define VTK_UNION_OF_MAGNITUDES 3

class VTK_EXPORT vtkImplicitBoolean : public vtkImplicitFunction
{
public:
  vtkImplicitBoolean();
  ~vtkImplicitBoolean();
  static vtkImplicitBoolean *New() {return new vtkImplicitBoolean;};
  const char *GetClassName() {return "vtkImplicitBoolean";};
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
  char *GetOperationTypeAsString();

protected:
  vtkImplicitFunctionCollection FunctionList;

  int OperationType;

};

// Description:
// Return the boolean operation type as a descriptive character string.
inline char *vtkImplicitBoolean::GetOperationTypeAsString(void)
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


