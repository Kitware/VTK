/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTCoords.h
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
// .NAME vtkTCoords - represent and manipulate texture coordinates
// .SECTION Description
// vtkTCoords represents and manipulates 1D, 2D, or 3D texture coordinates.
// Texture coordinates are 1D (s), 2D (s,t), or 3D (r,s,t) parametric values
// that map geometry into regular 1D, 2D, or 3D arrays of color and/or
// transparency values. During rendering the array are mapped onto the
// geometry for fast image detailing. 

#ifndef __vtkTCoords_h
#define __vtkTCoords_h

#include "vtkAttributeData.h"

class vtkIdList;
class vtkTCoords;

class VTK_EXPORT vtkTCoords : public vtkAttributeData
{
public:
  vtkTCoords(int dataType=VTK_FLOAT, int dim=2);
  static vtkTCoords *New(int dataType=VTK_FLOAT, int dim=2) 
    {return new vtkTCoords(dataType,dim);};
  const char *GetClassName() {return "vtkTCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload vtkAttributeData API
  void SetData(vtkDataArray *);
  vtkAttributeData *MakeObject();

  // generic access to TCoord data
  int GetNumberOfTCoords();
  float *GetTCoord(int id);
  void GetTCoord(int id, float tc[3]);
  void SetNumberOfTCoords(int number);
  void SetTCoord(int id, float tc[3]);
  void InsertTCoord(int id, float tc[3]);
  void InsertTCoord(int id, float tx, float ty, float tz);
  int InsertNextTCoord(float tc[3]);
  int InsertNextTCoord(float tx, float ty, float tz);

  // Description:
  // Specify/Get the dimension of the texture coordinates.
  void SetNumberOfComponents(int num);
  int GetNumberOfComponents() {return this->Data->GetNumberOfComponents();}

  // Get a list of texture coordinates
  void GetTCoords(vtkIdList& ptId, vtkTCoords& fv);

};

// Description:
// Create a copy of this object.
inline vtkAttributeData *vtkTCoords::MakeObject()
{
  return new vtkTCoords(this->GetDataType(), this->GetNumberOfComponents());
}

// Description:
// Specify the number of components in texture. Should be 1<=n<=3.
inline void vtkTCoords::SetNumberOfComponents(int num)
{
  num = (num < 1 ? 1 : (num > 3 ? 3 : num));
  this->Data->SetNumberOfComponents(num);
}

// Description:
// Return number of texture coordinates in array.
inline int vtkTCoords::GetNumberOfTCoords()
{
  return this->Data->GetNumberOfTuples();
}

// Description:
// Return a pointer to float texture coordinates tc[3] for a specific id.
inline float *vtkTCoords::GetTCoord(int id)
{
  return this->Data->GetTuple(id);
}

// Description:
// Copy texture coordinate components into user provided array tc[3] for specified
// id.
inline void vtkTCoords::GetTCoord(int id, float tc[3])
{
  this->Data->GetTuple(id,tc);
}

// Description:
// Specify the number of texture coordinates for this object to hold. Make sure
// that you set the number of components in texture first.
inline void vtkTCoords::SetNumberOfTCoords(int number)
{
  this->Data->SetNumberOfTuples(number);
}

// Description:
// Insert TCoord into object. No range checking performed (fast!).
// Make sure you use SetNumberOfTCoords() to allocate memory prior
// to using SetTCoord().
inline void vtkTCoords::SetTCoord(int id, float tc[3])
{
  this->Data->SetTuple(id,tc);
}

// Description:
// Insert TCoord into object. Range checking performed and memory
// allocated as necessary.
inline void vtkTCoords::InsertTCoord(int id, float tc[3])
{
  this->Data->InsertTuple(id,tc);
}

// Description:
// Insert TCoord into position indicated.
inline void vtkTCoords::InsertTCoord(int id, float tx, float ty, float tz)
{
  float tc[3];

  tc[0] = tx;
  tc[1] = ty;
  tc[2] = tz;
  this->Data->InsertTuple(id,tc);
}

// Description:
// Insert TCoord at end of array and return its location (id) in the array.
inline int vtkTCoords::InsertNextTCoord(float tx, float ty, float tz)
{
  float tc[3];

  tc[0] = tx;
  tc[1] = ty;
  tc[2] = tz;
  return this->Data->InsertNextTuple(tc);
}

// Description:
// Insert TCoord into next available slot. Returns id of slot.
inline int vtkTCoords::InsertNextTCoord(float tc[3])
{
  return this->Data->InsertNextTuple(tc);
}

// These include files are placed here so that if TCoords.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif
