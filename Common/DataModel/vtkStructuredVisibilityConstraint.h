/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredVisibilityConstraint.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredVisibilityConstraint - helper object to manage the
// visibility of points and cells
// .SECTION Description
// vtkStructuredVisibilityConstraint is a general class to manage
// a list of points/cell marked as invalid or invisible. Currently,
// it does this by maintaining an unsigned char array associated
// with points/cells. To conserve memory, this array is allocated
// only when it is needed (when Blank() is called the first time).
// Make sure to call Initialize() with the right dimensions before
// calling any methods that set/get visibility.

#ifndef __vtkStructuredVisibilityConstraint_h
#define __vtkStructuredVisibilityConstraint_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkUnsignedCharArray.h" // Needed for inline methods.

class VTKCOMMONDATAMODEL_EXPORT vtkStructuredVisibilityConstraint : public vtkObject
{
public:
  static vtkStructuredVisibilityConstraint *New();

  vtkTypeMacro(vtkStructuredVisibilityConstraint,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns 1 is the point/cell is visible, 0 otherwise.
  unsigned char IsVisible(vtkIdType id);

  // Description:
  // Sets the visibility flag of the given point/cell off.
  // The first time blank is called, a new visibility array
  // is created if it doesn't exist.
  void Blank(vtkIdType id);

  // Description:
  // Sets the visibility flag of the given point/cell on.
  void UnBlank(vtkIdType id);

  // Description:
  // Get the dimensions used to initialize the object.
  vtkGetVectorMacro(Dimensions,int,3);

  // Description:
  // Set the dimensions and set the Initialized flag to 1. Once
  // an object is initialized, it's dimensions can not be
  // changed anymore.
  void Initialize(int dims[3]);

  // Description:
  // Allocates the internal visibility data-structure iff the object has
  // been initialized.
  void Allocate();

  // Description:
  // Set/Get the array used to store the visibility flags.
  void SetVisibilityById(vtkUnsignedCharArray* vis);
  vtkGetObjectMacro(VisibilityById, vtkUnsignedCharArray);

  // Description:
  // Copies the dimensions, the visibility array pointer
  // and the initialized flag.
  void ShallowCopy(vtkStructuredVisibilityConstraint* src);

  // Description:
  // Copies the dimensions, the visibility array
  // and the initialized flag.
  void DeepCopy(vtkStructuredVisibilityConstraint* src);

  // Description:
  // Returns 0 if there is no visibility array (all cells/points
  // are visible), 0 otherwise.
  unsigned char IsConstrained()
    {
      return this->VisibilityById ? 1 : 0;
    }

protected:
  vtkStructuredVisibilityConstraint();
  ~vtkStructuredVisibilityConstraint();

  vtkUnsignedCharArray* VisibilityById;
  int Dimensions[3];
  vtkIdType NumberOfIds;
  unsigned char Initialized;

private:
  vtkStructuredVisibilityConstraint(const vtkStructuredVisibilityConstraint&);  // Not implemented.
  void operator=(const vtkStructuredVisibilityConstraint&);  // Not implemented.
};

//----------------------------------------------------------------------------
// These methods are inline for efficiency.

//----------------------------------------------------------------------------
inline void vtkStructuredVisibilityConstraint::Allocate()
{
  if( !this->VisibilityById )
    {
    this->VisibilityById = vtkUnsignedCharArray::New();
    this->VisibilityById->SetNumberOfTuples( this->NumberOfIds );
    for( int i=0; i < this->NumberOfIds; ++i )
      {
      this->VisibilityById->SetValue( i, 1 );
      }
    }
}
//----------------------------------------------------------------------------
inline unsigned char vtkStructuredVisibilityConstraint::IsVisible(
  vtkIdType id)
{
  vtkUnsignedCharArray* vis = this->VisibilityById;
  return vis ? vis->GetValue(id) : 1;
}

//----------------------------------------------------------------------------
inline void vtkStructuredVisibilityConstraint::Blank(vtkIdType id)
{
  vtkUnsignedCharArray* vis = this->VisibilityById;
  if (!vis)
    {
    this->VisibilityById = vtkUnsignedCharArray::New();
    vis = this->VisibilityById;
    this->VisibilityById->SetNumberOfTuples(this->NumberOfIds);
    for (int i=0; i<this->NumberOfIds; ++i)
      {
      this->VisibilityById->SetValue(i, 1);
      }
    }
  vis->SetValue(id, 0);
}

//----------------------------------------------------------------------------
inline void vtkStructuredVisibilityConstraint::UnBlank(vtkIdType id)
{
  vtkUnsignedCharArray* vis = this->VisibilityById;
  if (!vis)
    {
    return;
    }
  vis->SetValue(id, 1);
}

//----------------------------------------------------------------------------
inline void vtkStructuredVisibilityConstraint::Initialize(int dims[3])
{
  if (this->Initialized)
    {
    return;
    }
  for (int i=0; i<3; i++)
    {
    this->Dimensions[i] = dims[i];
    }
  this->NumberOfIds = static_cast<vtkIdType>(dims[0])*
                      static_cast<vtkIdType>(dims[1])*
                      static_cast<vtkIdType>(dims[2]);
  this->Initialized = 1;
}

#endif

