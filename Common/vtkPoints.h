/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.h
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
// .NAME vtkPoints - represent and manipulate 3D points
// .SECTION Description
// vtkPoints represents 3D points. The data model for vtkPoints is an 
// array of vx-vy-vz triplets accessible by (point or cell) id.

#ifndef __vtkPoints_h
#define __vtkPoints_h

#include "vtkObject.h"
#include "vtkDataArray.h"

class vtkIdList;
class vtkPoints;

class VTK_COMMON_EXPORT vtkPoints : public vtkObject
{
public:
//BTX
  static vtkPoints *New(int dataType);
//ETX
  static vtkPoints *New();

  vtkTypeMacro(vtkPoints,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate initial memory size.
  virtual int Allocate(const vtkIdType sz, const vtkIdType ext=1000);
  
  // Description:
  // Return object to instantiated state.
  virtual void Initialize();

  // Description:
  // Creates object of same type as this object.
  vtkPoints *MakeObject();

  // Description:
  // Set/Get the underlying data array. This function must be implemented
  // in a concrete subclass to check for consistency. (The tuple size must
  // match the type of data. For example, 3-tuple data array can be assigned to
  // a vector, normal, or points object, but not a tensor object, which has a 
  // tuple dimension of 9. Scalars, on the other hand, can have tuple dimension
  //  from 1-4, depending on the type of scalar.)
  virtual void SetData(vtkDataArray *);
  vtkDataArray *GetData() {return this->Data;};

  // Description:
  // Return the underlying data type. An integer indicating data type is 
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType();

  // Description:
  // Specify the underlying data type of the object.
  virtual void SetDataType(int dataType);
  void SetDataTypeToBit() {this->SetDataType(VTK_BIT);};
  void SetDataTypeToChar() {this->SetDataType(VTK_CHAR);};
  void SetDataTypeToUnsignedChar() {this->SetDataType(VTK_UNSIGNED_CHAR);};
  void SetDataTypeToShort() {this->SetDataType(VTK_SHORT);};
  void SetDataTypeToUnsignedShort() {this->SetDataType(VTK_UNSIGNED_SHORT);};
  void SetDataTypeToInt() {this->SetDataType(VTK_INT);};
  void SetDataTypeToUnsignedInt() {this->SetDataType(VTK_UNSIGNED_INT);};
  void SetDataTypeToLong() {this->SetDataType(VTK_LONG);};
  void SetDataTypeToUnsignedLong() {this->SetDataType(VTK_UNSIGNED_LONG);};
  void SetDataTypeToFloat() {this->SetDataType(VTK_FLOAT);};
  void SetDataTypeToDouble() {this->SetDataType(VTK_DOUBLE);};

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  void *GetVoidPointer(const int id) {return this->Data->GetVoidPointer(id);};

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() {this->Data->Squeeze();};

  // Description:
  // Make object look empty but do not delete memory.  
  virtual void Reset() {this->Data->Reset();};

  // Description:
  // Different ways to copy data. Shallow copy does reference count (i.e.,
  // assigns pointers and updates reference count); deep copy runs through
  // entire data array assigning values.
  virtual void DeepCopy(vtkPoints *ad);
  virtual void ShallowCopy(vtkPoints *ad);

  // Description:
  // Return the memory in kilobytes consumed by this attribute data. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the 
  // memory required to actually represent the data represented 
  // by this object. The information returned is valid only after
  // the pipeline has been updated.
  unsigned long GetActualMemorySize();

  // Description:
  // Return number of points in array.
  vtkIdType GetNumberOfPoints() { return this->Data->GetNumberOfTuples();};

  // Description:
  // Return a pointer to a float point x[3] for a specific id.
  float *GetPoint(vtkIdType id) { return this->Data->GetTuple(id);};

  // Description:
  // Copy point components into user provided array v[3] for specified
  // id.
  void GetPoint(vtkIdType id, float x[3]) { this->Data->GetTuple(id,x);};
  void GetPoint(vtkIdType id, double x[3]) { this->Data->GetTuple(id,x);};

  // Description:
  // Insert point into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfPoints() to allocate memory prior
  // to using SetPoint().
  void SetPoint(vtkIdType id, const float x[3]) { this->Data->SetTuple(id,x);};
  void SetPoint(vtkIdType id, const double x[3]) { this->Data->SetTuple(id,x);};
  void SetPoint(vtkIdType id, double x, double y, double z);

  // Description:
  // Insert point into object. Range checking performed and memory
  // allocated as necessary.
  void InsertPoint(vtkIdType id, const float x[3])
    { this->Data->InsertTuple(id,x);};
  void InsertPoint(vtkIdType id, const double x[3])
    {this->Data->InsertTuple(id,x);};
  void InsertPoint(vtkIdType id, double x, double y, double z);
  
  // Description:
  // Insert point into next available slot. Returns id of slot.
  vtkIdType InsertNextPoint(const float x[3]) { 
    return this->Data->InsertNextTuple(x);};
  vtkIdType InsertNextPoint(const double x[3]) { 
    return this->Data->InsertNextTuple(x);};
  vtkIdType InsertNextPoint(double x, double y, double z);

  // Description:
  // Specify the number of points for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetPoint() method for fast insertion.
  void SetNumberOfPoints(vtkIdType number);

  // Description:
  // Given a list of pt ids, return an array of points.
  void GetPoints(vtkIdList *ptId, vtkPoints *fp);

  // Description:
  // Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
  virtual void ComputeBounds();

  // Description:
  // Return the bounds of the points.
  float *GetBounds();

  // Description:
  // Return the bounds of the points.
  void GetBounds(float bounds[6]);

protected:
  vtkPoints(int dataType=VTK_FLOAT);
  ~vtkPoints();

  float Bounds[6];
  vtkTimeStamp ComputeTime; // Time at which bounds computed
  vtkDataArray *Data;  // Array which represents data

private:
  vtkPoints(const vtkPoints&);  // Not implemented.
  void operator=(const vtkPoints&);  // Not implemented.
};

inline void vtkPoints::SetNumberOfPoints(vtkIdType number)
{
  this->Data->SetNumberOfComponents(3);
  this->Data->SetNumberOfTuples(number);
}

inline void vtkPoints::SetPoint(vtkIdType id, double x, double y, double z)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->Data->SetTuple(id,p);
}

inline void vtkPoints::InsertPoint(vtkIdType id, double x, double y, double z)
{
  double p[3];

  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->Data->InsertTuple(id,p);
}

inline vtkIdType vtkPoints::InsertNextPoint(double x, double y, double z)
{
  double p[3];

  p[0] = x;
  p[1] = y;
  p[2] = z;
  return this->Data->InsertNextTuple(p);
}


// These include files are placed here so that if Points.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

