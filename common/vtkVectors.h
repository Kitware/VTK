/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectors.h
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
// .NAME vtkVectors - represent and manipulate 3D vectors
// .SECTION Description
// vtkVectors represents 3D vectors. The data model for vtkVectors is an 
// array of vx-vy-vz triplets accessible by (point or cell) id.

#ifndef __vtkVectors_h
#define __vtkVectors_h

#include "vtkAttributeData.h"

class vtkIdList;
class vtkVectors;

class VTK_EXPORT vtkVectors : public vtkAttributeData
{
public:
  vtkVectors(int dataType=VTK_FLOAT);
  static vtkVectors *New(int dataType=VTK_FLOAT) {return new vtkVectors(dataType);};
  const char *GetClassName() {return "vtkVectors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload vtkAttributeData API
  vtkAttributeData *MakeObject();

  // generic access to vector data
  int GetNumberOfVectors();
  float *GetVector(int id);
  void GetVector(int id, float v[3]);
  void SetNumberOfVectors(int number);
  void SetVector(int id, float v[3]);
  void InsertVector(int id, float v[3]);
  void InsertVector(int id, float vx, float vy, float vz);
  int InsertNextVector(float v[3]);
  int InsertNextVector(float vx, float vy, float vz);

  // Compute vector attributes

// Description:
// Compute the largest norm for these vectors.
  void ComputeMaxNorm();


// Description:
// Return the maximum norm for these vectors.
  float GetMaxNorm();



// Description:
// Given a list of pt ids, return an array of vectors.
  void GetVectors(vtkIdList& ptId, vtkVectors& fv);


protected:
  float MaxNorm;
  vtkTimeStamp ComputeTime; // Time at which MaxNorm computed
};

// Description:
// Create a copy of this object.
inline vtkAttributeData *vtkVectors::MakeObject()
{
  return new vtkVectors(this->GetDataType());
}

// Description:
// Return number of vectors in array.
inline int vtkVectors::GetNumberOfVectors()
{
  return this->Data->GetNumberOfTuples();
}

// Description:
// Return a pointer to a float vector v[3] for a specific id.
inline float *vtkVectors::GetVector(int id)
{
  return this->Data->GetTuple(id);
}

// Description:
// Copy vector components into user provided array v[3] for specified
// id.
inline void vtkVectors::GetVector(int id, float v[3])
{
  this->Data->GetTuple(id,v);
}

// Description:
// Specify the number of vectors for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetVector() method for fast insertion.
inline void vtkVectors::SetNumberOfVectors(int number)
{
  this->Data->SetNumberOfComponents(3);
  this->Data->SetNumberOfTuples(number);
}

// Description:
// Insert vector into object. No range checking performed (fast!).
// Make sure you use SetNumberOfVectors() to allocate memory prior
// to using SetVector().
inline void vtkVectors::SetVector(int id, float v[3])
{
  this->Data->SetTuple(id,v);
}

// Description:
// Insert vector into object. Range checking performed and memory
// allocated as necessary.
inline void vtkVectors::InsertVector(int id, float v[3])
{
  this->Data->InsertTuple(id,v);
}

// Description:
// Insert vector into position indicated.
inline void vtkVectors::InsertVector(int id, float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  this->Data->InsertTuple(id,v);
}

// Description:
// Insert vector at end of array and return its location (id) in the array.
inline int vtkVectors::InsertNextVector(float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  return this->Data->InsertNextTuple(v);
}

// Description:
// Insert vector into next available slot. Returns id of slot.
inline int vtkVectors::InsertNextVector(float v[3])
{
  return this->Data->InsertNextTuple(v);
}

// These include files are placed here so that if Vectors.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

