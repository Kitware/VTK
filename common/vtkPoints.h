/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.h
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
// .NAME vtkPoints - represent and manipulate 3D points
// .SECTION Description
// vtkPoints represents 3D points. The data model for vtkPoints is an 
// array of vx-vy-vz triplets accessible by (point or cell) id.

#ifndef __vtkPoints_h
#define __vtkPoints_h

#include "vtkAttributeData.h"

class vtkIdList;
class vtkPoints;

class VTK_EXPORT vtkPoints : public vtkAttributeData
{
public:
  vtkPoints(int dataType=VTK_FLOAT);
  static vtkPoints *New() {return new vtkPoints;};
  const char *GetClassName() {return "vtkPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload vtkAttributeData API
  vtkAttributeData *MakeObject();

  // generic access to point data
  int GetNumberOfPoints();
  float *GetPoint(int id);
  void GetPoint(int id, float x[3]);
  void SetNumberOfPoints(int number);
  void SetPoint(int id, float x[3]);
  void InsertPoint(int id, float x[3]);
  void InsertPoint(int id, float x, float y, float z);
  int InsertNextPoint(float x[3]);
  int InsertNextPoint(float x, float y, float z);


// Description:
// Given a list of pt ids, return an array of points.
  void GetPoints(vtkIdList& ptId, vtkPoints& fp);


  // Compute point attributes

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
  float Bounds[6];
  vtkTimeStamp ComputeTime; // Time at which bounds computed

};

// Description:
// Create a coy of this object.
inline vtkAttributeData *vtkPoints::MakeObject()
{
  return new vtkPoints(this->GetDataType());
}

// Description:
// Return number of points in array.
inline int vtkPoints::GetNumberOfPoints()
{
  return this->Data->GetNumberOfTuples();
}

// Description:
// Return a pointer to a float point x[3] for a specific id.
inline float *vtkPoints::GetPoint(int id)
{
  return this->Data->GetTuple(id);
}

// Description:
// Coy point components into user provided array v[3] for specified
// id.
inline void vtkPoints::GetPoint(int id, float x[3])
{
  this->Data->GetTuple(id,x);
}

// Description:
// Specify the number of points for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetPoint() method for fast insertion.
inline void vtkPoints::SetNumberOfPoints(int number)
{
  this->Data->SetNumberOfComponents(3);
  this->Data->SetNumberOfTuples(number);
}

// Description:
// Insert point into object. No range checking performed (fast!).
// Make sure you use SetNumberOfPoints() to allocate memory prior
// to using SetPoint().
inline void vtkPoints::SetPoint(int id, float x[3])
{
  this->Data->SetTuple(id,x);
}

// Description:
// Insert point into object. Range checking performed and memory
// allocated as necessary.
inline void vtkPoints::InsertPoint(int id, float x[3])
{
  this->Data->InsertTuple(id,x);
}

// Description:
// Insert point into position indicated.
inline void vtkPoints::InsertPoint(int id, float x, float y, float z)
{
  float p[3];

  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->Data->InsertTuple(id,p);
}

// Description:
// Insert point at end of array and return its location (id) in the array.
inline int vtkPoints::InsertNextPoint(float x, float y, float z)
{
  float p[3];

  p[0] = x;
  p[1] = y;
  p[2] = z;
  return this->Data->InsertNextTuple(p);
}

// Description:
// Insert point into next available slot. Returns id of slot.
inline int vtkPoints::InsertNextPoint(float x[3])
{
  return this->Data->InsertNextTuple(x);
}

// These include files are placed here so that if Points.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

