/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectors.h
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
// .NAME vtkVectors - represent and manipulate 3D vectors
// .SECTION Description
// vtkVectors represents 3D vectors. The data model for vtkVectors is an 
// array of vx-vy-vz triplets accessible by (point or cell) id.

#ifndef __vtkVectors_h
#define __vtkVectors_h

#include "vtkAttributeData.h"

class vtkIdList;
class vtkVectors;

class VTK_COMMON_EXPORT vtkVectors : public vtkAttributeData
{
public:
  static vtkVectors *New(int dataType);
  static vtkVectors *New();

  vtkTypeMacro(vtkVectors,vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the same type object as this (virtual constructor).
  vtkAttributeData *MakeObject()
    {return vtkVectors::New(this->GetDataType());};

  // Description:
  // Return number of vectors in array.
  vtkIdType GetNumberOfVectors() {return this->Data->GetNumberOfTuples();};

  // Description:
  // Return a pointer to a float vector v[3] for a specific id.
  float *GetVector(vtkIdType id) { return this->Data->GetTuple(id);};

  // Description:
  // Copy vector components into user provided array v[3] for specified
  // id.
  void GetVector(vtkIdType id, float v[3]) { this->Data->GetTuple(id,v);};
  void GetVector(vtkIdType id, double v[3]) { this->Data->GetTuple(id,v);};

  // Description:
  // Insert vector into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfVectors() to allocate memory prior
  // to using SetVector().
  void SetVector(vtkIdType id, const float v[3]) {this->Data->SetTuple(id,v);};
  void SetVector(vtkIdType id, const double v[3]) {this->Data->SetTuple(id,v);};
  void SetVector(vtkIdType id, double vx, double vy, double vz);

  // Description:
  // Insert vector into object. Range checking performed and memory
  // allocated as necessary.
  void InsertVector(vtkIdType id, const float v[3])
    {this->Data->InsertTuple(id,v);};
  void InsertVector(vtkIdType id, const double v[3]) 
    {this->Data->InsertTuple(id,v);};
  void InsertVector(vtkIdType id, double vx, double vy, double vz);

  // Description:
  // Insert vector into next available slot. Returns id of slot.
  vtkIdType InsertNextVector(const float v[3]) 
    {return this->Data->InsertNextTuple(v);};
  vtkIdType InsertNextVector(const double v[3]) 
    {return this->Data->InsertNextTuple(v);};
  vtkIdType InsertNextVector(double vx, double vy, double vz);

  // Description:
  // Specify the number of vectors for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetVector() method for fast insertion.
  void SetNumberOfVectors(vtkIdType number);

  // Description:
  // Compute the largest norm for these vectors.
  void ComputeMaxNorm();

  // Description:
  // Return the maximum norm for these vectors.
  double GetMaxNorm();

  // Description:
  // Given a list of pt ids, return an array of vectors.
  void GetVectors(vtkIdList *ptId, vtkVectors *fv);


protected:
  vtkVectors();
  ~vtkVectors() {};
  vtkVectors(const vtkVectors&);
  void operator=(const vtkVectors&);

  double MaxNorm;
  vtkTimeStamp ComputeTime; // Time at which MaxNorm computed

};


inline void vtkVectors::SetNumberOfVectors(vtkIdType number)
{
  this->Data->SetNumberOfComponents(3);
  this->Data->SetNumberOfTuples(number);
}

inline void vtkVectors::SetVector(vtkIdType id, double vx, double vy, double vz)
{
  double v[3];
  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  this->Data->SetTuple(id,v);
}

inline void vtkVectors::InsertVector(vtkIdType id, double vx, double vy,
                                     double vz)
{
  double v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  this->Data->InsertTuple(id,v);
}

inline vtkIdType vtkVectors::InsertNextVector(double vx, double vy, double vz)
{
  double v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  return this->Data->InsertNextTuple(v);
}


// These include files are placed here so that if Vectors.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

