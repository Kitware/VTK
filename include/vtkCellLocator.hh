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
// vtkCellLocator is a spatial search object to quickly locate cells in 3D.
// vtkCellLocator uses a uniform-level octree subdivision, where each octant
// (an octant is also referred to as a bucket) carries an indication of whether 
// it is empty or not, and each leaf octant carries a list of the cells inside 
// of it. (An octant is not empty if it has one or more cells inside of it.) 
// Typical operations are intersection with a line to return candidate cells, 
// or intersection with another vtkCellLocator to return candidate cells.

// .SECTION Caveats
// Many other types of spatial locators have been developed, such as 
// variable depth octrees and kd-trees. These are often more efficient 
// for the operations described here. vtkCellLocator has been designed
// for subclassing; so these locators can be derived if necessary.

// .SECTION See Also
// vtkLocator vtkPointLocator vtkOBBTree

#ifndef __vtkCellLocator_h
#define __vtkCellLocator_h

#include "vtkLocator.hh"

class vtkCellLocator : public vtkLocator
{
public:
  vtkCellLocator();
  char *GetClassName() {return "vtkCellLocator";};
  virtual void FreeSearchStructure();

  // Description:
  // Specify the average number of cells in each octant.
  vtkSetClampMacro(NumberOfCellsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfCellsPerBucket,int);

  // methods that all cell locators must provide
  virtual int IntersectWithLine(float a0[3], float a1[3], float tol,
				float& t, float x[3], float pcoords[3],
				int &subId);
  virtual vtkIdList *GetCells(int bucket);
  virtual void InitializeIntersection(vtkCellLocator& locator);
  virtual int GetNextIntersection(int& bucket1, int& bucket2);

  // satisfy vtkLocator abstract interface
  void GenerateRepresentation(int level, vtkPolyData *pd);
  
protected:
  // place points in appropriate cells
  void BuildLocator();

  int NumberOfCellsPerBucket; // cells per octant
  int NumberOfOctants; // number of octants in tree
  float Bounds[6]; // bounding box root octant
  int NumberOfParents; // number of parent octants
  float H[3]; // width of root octant in x-y-z directions
  int NumberOfDivisions; // number of "leaf" octant sub-divisions
  vtkIdList **Tree; // octree
  vtkTimeStamp BuildTime;  

  void MarkParents(void*, int, int, int, int, int);
  void GetChildren(int idx, int level, int children[8]);
  int GenerateIndex(int offset, int numDivs, int i, int j, int k, int &idx);
  void GenerateFace(int face, int numDivs, int i, int j, int k,
                    vtkFloatPoints *pts, vtkCellArray *polys);
};

#endif


