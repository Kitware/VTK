/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkImplicitFunctionCollection.h"

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
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate gradient of boolean combination.
  void EvaluateGradient(float x[3], float g[3]);

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


