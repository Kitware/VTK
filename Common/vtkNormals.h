/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormals.h
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
// .NAME vtkNormals - represent and manipulate 3D normals
// .SECTION Description
// vtkNormals represents 3D normals. The data model for vtkNormals is an 
// array of nx-ny-nz triplets accessible by (point or cell) id. Each normal
// is assumed to have magnitude |n| = 1.

#ifndef __vtkNormals_h
#define __vtkNormals_h

#include "vtkAttributeData.h"

class vtkIdList;
class vtkNormals;

class VTK_COMMON_EXPORT vtkNormals : public vtkAttributeData
{
public:
  static vtkNormals *New(int dataType);
  static vtkNormals *New();

  vtkTypeMacro(vtkNormals,vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  vtkAttributeData *MakeObject()
    {return vtkNormals::New(this->GetDataType());}
  
  // Description:
  // Return number of normals in array.
  vtkIdType GetNumberOfNormals() 
    {return this->Data->GetNumberOfTuples();}

  // Description:
  // Return a pointer to a float normal n[3] for a specific id.
  float *GetNormal(vtkIdType id) 
    {return this->Data->GetTuple(id);}
  
  // Description:
  // Copy normal components into user provided array n[3] for specified
  // id.
  void GetNormal(vtkIdType id, float n[3]) 
    {this->Data->GetTuple(id,n);}
  void GetNormal(vtkIdType id, double n[3]) 
    {this->Data->GetTuple(id,n);}
  
  // Description:
  // Insert normal into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfNormals() to allocate memory prior
  // to using SetNormal().
  void SetNormal(vtkIdType id, const float n[3]) 
    {this->Data->SetTuple(id,n);}
  void SetNormal(vtkIdType id, const double n[3]) 
    {this->Data->SetTuple(id,n);}
  void SetNormal(vtkIdType id, double nx, double ny, double nz);

  // Description:
  // Insert normal into object. Range checking performed and memory
  // allocated as necessary.
  void InsertNormal(vtkIdType id, const double n[3]) 
    {this->Data->InsertTuple(id,n);}
  void InsertNormal(vtkIdType id, float n[3]) 
    {this->Data->InsertTuple(id,n);}
  void InsertNormal(vtkIdType id, double nx, double ny, double nz);

  // Description:
  // Insert normal into next available slot. Returns id of slot.
  vtkIdType InsertNextNormal(const float n[3]) 
    {return this->Data->InsertNextTuple(n);}
  vtkIdType InsertNextNormal(const double n[3]) 
    {return this->Data->InsertNextTuple(n);}
  vtkIdType InsertNextNormal(double nx, double ny, double nz);

  // Description:
  // Specify the number of normals for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetNormal() method for fast insertion.
  void SetNumberOfNormals(vtkIdType number);

  // Description:
  // Given a list of pt ids, return an array of normals.
  void GetNormals(vtkIdList *ptId, vtkNormals *fn);

protected:
  vtkNormals();
  ~vtkNormals() {};
  
private:
  vtkNormals(const vtkNormals&);  // Not implemented.
  void operator=(const vtkNormals&);  // Not implemented.
};


inline void vtkNormals::SetNumberOfNormals(vtkIdType number)
{
  this->Data->SetNumberOfComponents(3);
  this->Data->SetNumberOfTuples(number);
}

inline void vtkNormals::SetNormal(vtkIdType id, double nx, double ny, double nz)
{
  double n[3];
  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  this->Data->SetTuple(id,n);
}

inline void vtkNormals::InsertNormal(vtkIdType id, double nx, double ny,
                                     double nz)
{
  double n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  this->Data->InsertTuple(id,n);
}

inline vtkIdType vtkNormals::InsertNextNormal(double nx, double ny, double nz)
{
  double n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  return this->Data->InsertNextTuple(n);
}


// These include files are placed here so that if Normals.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif
