/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarTree.h
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
// .NAME vtkScalarTree - organize data according to scalar values (used to accelerate contouring operations)

// .SECTION Description
// vtkScalarTree is an abstract class that defines the API to concrete
// scalar tree subclasses. A scalar tree is a data structure that organizes
// data according to its scalar value. This allows rapid access to data for
// those algorithms that access the data based on scalar value. For example,
// isocontouring operates on cells based on the scalar (isocontour) value.
//
// To use subclasses of this class, you must specify a dataset to operate on,
// and then specify a scalar value in the InitTraversal() method. Then
// calls to GetNextCell() return cells whose scalar data contains the
// scalar value specified.

// .SECTION See Also
// vtkSimpleScalarTree vtkBONOScalarTree

#ifndef __vtkScalarTree_h
#define __vtkScalarTree_h

#include "vtkObject.h"
#include "vtkDataSet.h"

class VTK_FILTERING_EXPORT vtkScalarTree : public vtkObject
{
public:
  vtkTypeMacro(vtkScalarTree,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the tree from the points/cells defining this dataset.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Construct the scalar tree from the dataset provided. Checks build times
  // and modified time from input and reconstructs the tree if necessary.
  virtual void BuildTree() = 0;
  
  // Description:
  // Initialize locator. Frees memory and resets object as appropriate.
  virtual void Initialize() = 0;

  // Description:
  // Begin to traverse the cells based on a scalar value. Returned cells
  // will have scalar values that span the scalar value specified.
  virtual void InitTraversal(float scalarValue) = 0;

  // Description:
  // Return the next cell that may contain scalar value specified to
  // initialize traversal. The value NULL is returned if the list is
  // exhausted. Make sure that InitTraversal() has been invoked first or
  // you'll get erratic behavior.
  virtual vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars) = 0;

protected:
  vtkScalarTree();
  ~vtkScalarTree();

  vtkDataSet   *DataSet;    //the dataset over which the scalar tree is built
  vtkDataArray *Scalars;    //the scalars of the DataSet

  vtkTimeStamp BuildTime; //time at which tree was built
  float        ScalarValue; //current scalar value for traversal

private:
  vtkScalarTree(const vtkScalarTree&);  // Not implemented.
  void operator=(const vtkScalarTree&);  // Not implemented.
};

#endif


