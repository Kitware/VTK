/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.h
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
// .NAME vtkPointSet - abstract class for specifying dataset behavior
// .SECTION Description
// vtkPointSet is an abstract class that specifies the interface for 
// datasets that explicitly use "point" arrays to represent geometry.
// For example, vtkPolyData and vtkUnstructuredGrid require point arrays
// to specify point position, while vtkStructuredPoints generates point
// positions implicitly.

// .SECTION See Also
// vtkPolyData vtkStructuredGrid vtkUnstructuredGrid

#ifndef __vtkPointSet_h
#define __vtkPointSet_h

#include "vtkDataSet.h"
#include "vtkPointLocator.h"

class VTK_EXPORT vtkPointSet : public vtkDataSet
{
public:
  vtkTypeMacro(vtkPointSet,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Reset to an empty state and free any memory.
  void Initialize();

  // Description:
  // Copy the geometric structure of an input point set object.
  void CopyStructure(vtkDataSet *pd);

  // Description:
  // See vtkDataSet for additional information.
  vtkIdType GetNumberOfPoints();
  float *GetPoint(vtkIdType ptId) {return this->Points->GetPoint(ptId);};
  void GetPoint(vtkIdType ptId, float x[3]) {this->Points->GetPoint(ptId,x);};
  vtkIdType FindPoint(float x[3]);
  vtkIdType FindPoint(float x, float y, float z) { return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkIdType cellId, float tol2,
               int& subId, float pcoords[3], float *weights);
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkGenericCell *gencell,
	       vtkIdType cellId, float tol2, int& subId, 
               float pcoords[3], float *weights);

  // Description:
  // Get MTime which also considers its vtkPoints MTime.
  unsigned long GetMTime();

  // Description:
  // Compute the (X, Y, Z)  bounds of the data.
  void ComputeBounds();
  
  // Description:
  // Reclaim any unused memory.
  void Squeeze();

  // Description:
  // Specify point array to define point coordinates.
  vtkSetObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);

  // Description:
  // Detect reference loop PointSet <-> locator.
  void UnRegister(vtkObject *o);
  
  // Description:
  // Get the net reference count. That is the count minus
  // any self created loops. This is used in the Source/Data
  // registration to properly free the objects.
  virtual int GetNetReferenceCount();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);

protected:
  vtkPointSet();
  ~vtkPointSet();
  vtkPointSet(const vtkPointSet&) {};
  void operator=(const vtkPointSet&) {};

  vtkPoints *Points;
  vtkPointLocator *Locator;

};

inline vtkIdType vtkPointSet::GetNumberOfPoints()
{
  if (this->Points)
    {
    return this->Points->GetNumberOfPoints();
    }
  else
    {
    return 0;
    }
}


#endif


