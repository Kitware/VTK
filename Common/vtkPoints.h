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

#include "vtkAttributeData.h"

class vtkIdList;
class vtkPoints;

class VTK_EXPORT vtkPoints : public vtkAttributeData
{
public:
//BTX
  static vtkPoints *New(int dataType);
//ETX
  static vtkPoints *New();

  vtkTypeMacro(vtkPoints,vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  vtkAttributeData *MakeObject();

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
  ~vtkPoints() {};
  vtkPoints(const vtkPoints&) {};
  void operator=(const vtkPoints&) {};

  float Bounds[6];
  vtkTimeStamp ComputeTime; // Time at which bounds computed

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

