/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBTree.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkOBBTree - generate oriented bounding box (OBB) tree
// .SECTION Description
// vtkOBBTree is an object to generate oriented bounding box (OBB) trees.
// An oriented bounding box is a bounding box that does not necessarily line 
// up along coordinate axes. The OBB tree is a hierarchical tree structure 
// of such boxes, where deeper levels of OBB confine smaller regions of space.
//
// To build the OBB, a recursive, top-down process is used. First, the root OBB
// is constructed by finding the mean and covariance matrix of the cells (and
// their points) that define the dataset. The eigenvectors of the covariance
// matrix are extracted, giving a set of three orthogonal vectors that define 
// the tightest-fitting OBB. To create the two children OBB's, a split plane 
// is found that (approximately) divides the number cells in half. These are 
// then assigned to the children OBB's. This process then continues until
// the MaxLevel ivar limits the recursion, or no split plane can be found.
//
// A good reference for OBB-trees is Gottschalk & Manocha in Proceedings of 
// Siggraph `96.

// .SECTION Caveats
// Since this algorithms works from a list of cells, the OBB tree will only 
// bound the "geometry" attached to the cells if the convex hull of the 
// cells bounds the geometry.
//
// Long, skinny cells (i.e., cells with poor aspect ratio) may cause 
// unsatisfactory results. This is due to the fact that this is a top-down
// implementation of the OBB tree, requiring that one or more complete cells
// are contained in each OBB. This requirement makes it hard to find good 
// split planes during the recurion process. A bottom-up implementation would
// go a long way to correcting this problem.

// .SECTION See Also
// vtkLocator vtkCellLocator vtkLocatorFilter

#ifndef __vtkOBBTree_h
#define __vtkOBBTree_h

#include "vtkCellLocator.h"

//
// Special class defines node for the OBB tree
//
//BTX - begin tcl exclude
//
class vtkOBBNode { //;prevent man page generation
public:
  vtkOBBNode();
  ~vtkOBBNode();

  float Corner[3]; //center point of this node
  float Axes[3][3]; //the axes defining the OBB - ordered from long->short
  vtkOBBNode *Parent; //parent node; NULL if root
  vtkOBBNode **Kids; //two children of this node; NULL if leaf
  vtkIdList *Cells; //list of cells in node
};
//ETX - end tcl exclude
//

class VTK_EXPORT vtkOBBTree : public vtkCellLocator
{
public:
  vtkOBBTree();
  char *GetClassName() {return "vtkOBBTree";};

  void ComputeOBB(vtkFloatPoints *pts, float corner[3], float max[3], 
                  float mid[3], float min[3], float size[3]);

  int IntersectWithLine(float a0[3], float a1[3], float& t, 
                        float x[3], float pcoords[3],int &subId);

  // satisfy locator'a abstract interface
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);

protected:
  vtkOBBNode *Tree;
  void BuildTree(vtkIdList *cells, vtkOBBNode *parent, int level);
  vtkFloatPoints *PointsList;
  int *InsertedPoints;
  int OBBCount;
  int DeepestLevel;

  void DeleteTree(vtkOBBNode *OBBptr);
  void GeneratePolygons(vtkOBBNode *OBBptr, int level, int repLevel, 
                        vtkFloatPoints* pts, vtkCellArray *polys);

};

#endif


