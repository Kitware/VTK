/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTCoords.h
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

class VTK_COMMON_EXPORT vtkTCoords : public vtkAttributeData
{
public:
  static vtkTCoords *New(int dataType, int dim=2);
  static vtkTCoords *New();


  vtkTypeMacro(vtkTCoords,vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the data for this object. The tuple dimension must be consistent with
  // the object.
  void SetData(vtkDataArray *);

  // Description:
  // Create a copy of this object.
  vtkAttributeData *MakeObject();
 
  // Description:
  // Return number of texture coordinates in array.
  vtkIdType GetNumberOfTCoords() {return this->Data->GetNumberOfTuples();};

  // Description:
  // Return a pointer to float texture coordinates tc[3] for a specific id.
  float *GetTCoord(vtkIdType id){return this->Data->GetTuple(id);};

  // Description: 
  // Copy texture coordinate components into user provided array tc[3] for
  // specified id.
  void GetTCoord(vtkIdType id, float tc[3]) { this->Data->GetTuple(id,tc);};

  // Description:
  // Specify the number of texture coordinates for this object to hold. Make
  // sure that you set the number of components in texture first.
  void SetNumberOfTCoords(vtkIdType number)
    {this->Data->SetNumberOfTuples(number);};

  // Description:
  // Insert TCoord into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfTCoords() to allocate memory prior
  // to using SetTCoord().
  void SetTCoord(vtkIdType id, const float tc[3])
    {this->Data->SetTuple(id,tc);};
  void SetTCoord(vtkIdType id, float r, float s, float t);

  // Description:
  // Insert TCoord into object. Range checking performed and memory
  // allocated as necessary.
  void InsertTCoord(vtkIdType id, const float tc[3]) 
    {this->Data->InsertTuple(id,tc);};

  // Description:
  // Insert TCoord into position indicated.
  void InsertTCoord(vtkIdType id, float tx, float ty, float tz);

  // Description:
  // Insert TCoord at end of array and return its location (id) in the array.
  vtkIdType InsertNextTCoord(const float tc[3]) 
    {return this->Data->InsertNextTuple(tc);}
  vtkIdType InsertNextTCoord(float tx, float ty, float tz);

  // Description:
  // Set/Get the number of components in texture. Should be 1<=n<=3.
  void SetNumberOfComponents(int num);
  int GetNumberOfComponents() 
    {return this->Data->GetNumberOfComponents();}

  // Description:
  // Get a list of texture coordinates
  void GetTCoords(vtkIdList *ptId, vtkTCoords *fv);

protected:
  vtkTCoords();
  ~vtkTCoords() {};
  
private:
  vtkTCoords(const vtkTCoords&);  // Not implemented.
  void operator=(const vtkTCoords&);  // Not implemented.
};

inline vtkAttributeData *vtkTCoords::MakeObject()
{
  return vtkTCoords::New(this->GetDataType(), this->GetNumberOfComponents());
}

inline void vtkTCoords::SetNumberOfComponents(int num)
{
  num = (num < 1 ? 1 : (num > 3 ? 3 : num));
  this->Data->SetNumberOfComponents(num);
}

inline void vtkTCoords::SetTCoord(vtkIdType id, float tx, float ty, float tz)
{
  float tc[3];
  tc[0] = tx;
  tc[1] = ty;
  tc[2] = tz;
  this->Data->SetTuple(id,tc);
}

inline void vtkTCoords::InsertTCoord(vtkIdType id, float tx, float ty, float tz)
{
  float tc[3];

  tc[0] = tx;
  tc[1] = ty;
  tc[2] = tz;
  this->Data->InsertTuple(id,tc);
}

inline vtkIdType vtkTCoords::InsertNextTCoord(float tx, float ty, float tz)
{
  float tc[3];

  tc[0] = tx;
  tc[1] = ty;
  tc[2] = tz;
  return this->Data->InsertNextTuple(tc);
}


// These include files are placed here so that if TCoords.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif
