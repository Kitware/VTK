/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeTable.h
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
// .NAME vtkEdgeTable - keep track of edges (edge is pair of integer id's)
// .SECTION Description
// vtkEdgeTable is a general object for keeping track of lists of edges. An
// edge is defined by the pair of point id's (p1,p2). Methods are available
// to insert edges, check if edges exist, and traverse the list of edges.
// Also, it's possible to associate attribute information with each edge.

#ifndef __vtkEdgeTable_h
#define __vtkEdgeTable_h

#include "vtkObject.h"

class vtkIdList;
class vtkPoints;

class VTK_EXPORT vtkEdgeTable : public vtkObject
{
public:
  // Description:
  // Instantiate object assuming that 1000 edges are to be inserted.
  static vtkEdgeTable *New();

  vtkTypeMacro(vtkEdgeTable,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Free memory and return to instantiated state.
  void Initialize();

  // Description:
  // Initialize the edge insertion process. Provide an estimate of the
  // number of points in a dataset (the maximum range value of p1 or
  // p2).  The storeAttributes variable controls whether attributes
  // are to be stored with the edge. If on, additional memory will be
  // required by the data structure to store an integer id per each
  // edge.  This method is used in conjunction with one of the two
  // InsertEdge() methods described below (don't mix the InsertEdge()
  // methods). 
  int InitEdgeInsertion(vtkIdType numPoints, int storeAttributes=0);

  // Description:
  // Insert the edge (p1,p2) into the table. It is the user's
  // responsibility to check if the edge has already been inserted
  // (use IsEdge()). If the storeAttributes flag in InitEdgeInsertion()
  // has been set, then the method returns a unique integer id (i.e.,
  // the edge id) that can be used to set and get edge
  // attributes. Otherwise, the method will return 1. Do not mix this
  // method with the InsertEdge() method that follows.
  vtkIdType InsertEdge(vtkIdType p1, vtkIdType p2);

  // Description:
  // Insert the edge (p1,p2) into the table with the attribute id
  // specified (make sure the attributeId >= 0). Note that the
  // attributeId is ignored if the storeAttributes variable was set to
  // 0 in the InitEdgeInsertion() method. It is the user's
  // responsibility to check if the edge has already been inserted
  // (use IsEdge()). Do not mix this method with the previous
  // InsertEdge() method.
  void InsertEdge(vtkIdType p1, vtkIdType p2, int attributeId);

  // Description: 
  // Return an integer id for the edge, or an attributeId of the edge
  // (p1,p2) if the edge has been previously defined (it depends upon
  // which version of InsertEdge() is being used); otherwise -1. The
  // unique integer id can be used to set and retrieve attributes to
  // the edge.
  int IsEdge(vtkIdType p1, vtkIdType p2);

  // Description:
  // Initialize the point insertion process. The newPts is an object
  // representing point coordinates into which incremental insertion methods
  // place their data. The points are associated with the edge.
  int InitPointInsertion(vtkPoints *newPts, vtkIdType estSize);

  // Description:
  // Insert a unique point on the specified edge. Invoke this method only
  // after InitPointInsertion() has been called. Return 0 if point was 
  // already in the list, otherwise return 1.
  int InsertUniquePoint(vtkIdType p1, vtkIdType p2, float x[3],
                        vtkIdType &ptId);

  // Description:
  // Return the number of edges that have been inserted thus far.
  vtkGetMacro(NumberOfEdges, vtkIdType);

  // Description:
  // Intialize traversal of edges in table.
  void InitTraversal();

  // Description:
  // Traverse list of edges in table. Return the edge as (p1,p2), where p1
  // and p2 are point id's. Method return value is zero if list is exhausted;
  // non-zero otherwise. The value of p1 is guaranteed to be <= p2.
  int GetNextEdge(vtkIdType &p1, vtkIdType &p2);

  // Description:
  // Reset the object and prepare for reinsertion of edges. Does not delete
  // memory like the Initialize() method.
  void Reset();

protected:
  vtkEdgeTable();
  ~vtkEdgeTable();
  vtkEdgeTable(const vtkEdgeTable&);
  void operator=(const vtkEdgeTable&);

  vtkIdList **Table;
  vtkIdList **Attributes;
  int StoreAttributes;
  vtkIdType TableMaxId; //maximum point id inserted
  vtkIdType TableSize;  //allocated size of table
  int Position[2];
  int Extend;
  vtkIdType NumberOfEdges;
  vtkPoints *Points; //support point insertion

  vtkIdList **Resize(vtkIdType size);
};

#endif

