/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocator.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCellLocator - octree-based spatial search object to quickly locate cells
// .SECTION Description
// vtkCellLocator is a spatial search object to quickly locate cells in 3-D.
// vtkCellLocator uses a uniform-level octree subdivision, where each octant
// carries an indication of whether it is empty or not, and each leaf octant 
// carries a list of the cells inside of it. (An octant is not empty if it 
// has one or more cells inside of it). Typical operation are intersection 
// with a line to return candidate cells, or intersection with another 
// vtkCellLocator to return candidate cells.
// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// variable depth octrees and k-d trees. These are often more efficient 
// for the operations described here.

#ifndef __vtkCellLocator_h
#define __vtkCellLocator_h

#include "vtkObject.hh"
#include "vtkPoints.hh"
#include "vtkIdList.hh"
#include "vtkDataSet.hh"


class vtkCellLocator : public vtkObject
{
public:
  vtkCellLocator();
  virtual ~vtkCellLocator();
  char *GetClassName() {return "vtkCellLocator";};
  void Initialize();
  virtual void FreeSearchStructure();

  // Description:
  // Set list of cells to insert into locator.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set the level of the octree (set automatically if Automatic is true).
  vtkSetClampMacro(Level,int,1,this->MaxLevel);
  vtkGetMacro(Level,int);

  // Description:
  // Set the maximum allowable level for the octree.
  vtkSetClampMacro(MaxLevel,int,1,5);
  vtkGetMacro(MaxLevel,int);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of cells in octant.
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of cells in each octant.
  vtkSetClampMacro(NumberOfCellsInOctant,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfCellsInOctant,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // intersection computations.
  vtkSetClampMacro(Tolerance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Tolerance,float);

  virtual int FindClosestCell(float x[3], float dist2, int& subId, float pcoords[3]);
  virtual int IntersectWithLine(float a0[3], float a1[3], vtkIdList& cells);
  vtkIdList *GetOctantCells(int octantId);
  virtual int IntersectWithCellLocator(vtkCellLocator& locator, vtkIdList cells);

protected:
  // place points in appropriate cells
  void SubDivide();

  vtkDataSet *DataSet; // Dataset of cells to insert
  int MaxLevel; // Maximum tree level
  int Level; // Tree level
  int NumberOfOctants; // number of octants in tree
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfCellsInOctant; // Used with previous boolean to control subdivide
  float Tolerance; // for performing intersection
  float Bounds[6]; // bounding box root octant
  int NumberOfParents; // number of parent octants
  float H[3]; // width of root octant in x-y-z directions
  int NumberOfDivisions; // number of "leaf" octant sub-divisions
  vtkIdList **Tree; // octree
  vtkTimeStamp SubDivideTime;  

  void MarkParents(void*, int i, int j, int k);
  void GetChildren(int idx, int level, int children[8]);
};

#endif


