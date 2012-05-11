/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPoints - represent and manipulate 3D points
// .SECTION Description
// vtkPoints represents 3D points. The data model for vtkPoints is an
// array of vx-vy-vz triplets accessible by (point or cell) id.

#ifndef __vtkPoints_h
#define __vtkPoints_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkDataArray.h" // Needed for inline methods

class vtkIdList;
class vtkPoints;

class VTKCOMMONCORE_EXPORT vtkPoints : public vtkObject
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
  // Return a pointer to a double point x[3] for a specific id.
  // WARNING: Just don't use this error-prone method, the returned pointer
  // and its values are only valid as long as another method invocation is not
  // performed. Prefer GetPoint() with the return value in argument.
  double *GetPoint(vtkIdType id) { return this->Data->GetTuple(id);};

  // Description:
  // Copy point components into user provided array v[3] for specified
  // id.
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
  double *GetBounds();

  // Description:
  // Return the bounds of the points.
  void GetBounds(double bounds[6]);

protected:
  vtkPoints(int dataType=VTK_FLOAT);
  ~vtkPoints();

  double Bounds[6];
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

#endif

