/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocator.h
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
// .NAME vtkLocator - abstract base class for objects that accelerate spatial searches
// .SECTION Description
// vtkLocator is an abstract base class for spatial search objects, or 
// locators. The principle behind locators is that they divide 3-space into 
// small pieces (or "buckets") that can be quickly found in response to 
// queries like point location, line intersection, or object-object 
// intersection.
//
// The purpose of this base class is to provide ivars and methods shared by 
// all locators. The GenerateRepresentation() is one such interesting method.
// This method works in conjunction with vtkLocatorFilter to create polygonal
// representations for the locator. For example, if the locator is an OBB tree
// (i.e., vtkOBBTree.h), then the representation is a set of one or more 
// oriented bounding boxes, depending upon the specified level.
// 
// Locators typically work as follows. One or more "entities", such as 
// points or cells, are inserted into the tree. These entities are associated
// with one or more buckets. Then, when performing geometric operations, the
// operations are performed first on the buckets, and then if the operation
// tests positive, then on the entities in the bucket. For example, during
// collision tests, the locators are collided first to identify intersecting
// buckets. If an intersection is found, more expensive operations are then
// carried out on the entities in the bucket.
// 
// To obtain good performance, locators are often organized in a tree
// structure.  In such a structure, there are frequently multiple "levels"
// corresponding to different nodes in the tree. So the word level (in the
// context of the locator) can be used to specify a particular representation
// in the tree.  For example, in an octree (which is a tree with 8 children),
// level 0 is the bounding box, or root octant, and level 1 consists of its
// eight children.

// .SECTION See Also
// vtkPointLocator vtkCellLocator vtkOBBTree vtkLocatorFilter

#ifndef __vtkLocator_h
#define __vtkLocator_h

#include "vtkObject.h"
#include "vtkDataSet.h"
class vtkPolyData;

class VTK_COMMON_EXPORT vtkLocator : public vtkObject
{
public:
  vtkTypeMacro(vtkLocator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the locator from the points/cells defining this dataset.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set the maximum allowable level for the tree. If the Automatic ivar is 
  // off, this will be the target depth of the locator.
  vtkSetClampMacro(MaxLevel,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(MaxLevel,int);

  // Description:
  // Get the level of the locator (determined automatically if Automatic is 
  // true). The value of this ivar may change each time the locator is built.
  vtkGetMacro(Level,int);

  // Description:
  // Boolean controls whether locator depth/resolution of locator is computed
  // automatically from average number of entities in bucket. If not set,
  // there will be an explicit method to control the construction of the
  // locator (found in the subclass).
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // geometric operations.
  vtkSetClampMacro(Tolerance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Boolean controls whether to maintain list of entities in each bucket.
  // Normally the lists are maintained, but if the locator is being used
  // as a geometry simplification technique, there is no need to keep them.
  vtkSetMacro(RetainCellLists,int);
  vtkGetMacro(RetainCellLists,int);
  vtkBooleanMacro(RetainCellLists,int);

  // Description:
  // Cause the locator to rebuild itself if it or its input dataset has 
  // changed.
  virtual void Update();

  // Description:
  // Initialize locator. Frees memory and resets object as appropriate.
  virtual void Initialize();

  // Description:
  // Build the locator from the input dataset.
  virtual void BuildLocator() = 0;

  // Description:
  // Free the memory required for the spatial data structure.
  virtual void FreeSearchStructure() = 0;

  // Description:
  // Method to build a representation at a particular level. Note that the 
  // method GetLevel() returns the maximum number of levels available for
  // the tree. You must provide a vtkPolyData object into which to place the 
  // data.
  virtual void GenerateRepresentation(int level, vtkPolyData *pd) = 0;

  // Description:
  // Return the time of the last data structure build.
  vtkGetMacro(BuildTime, unsigned long);

protected:
  vtkLocator();
  ~vtkLocator();
  vtkLocator(const vtkLocator&);
  void operator=(const vtkLocator&);

  vtkDataSet *DataSet;
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  float Tolerance; // for performing merging
  int MaxLevel;
  int Level;
  int RetainCellLists;

  vtkTimeStamp BuildTime;  // time at which locator was built

};

#endif


