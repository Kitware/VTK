/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBTree.hh
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
// .NAME vtkOBBTree - generate oriented bounding box (OBB) tree
// .SECTION Description
// vtkOBBTree is an object to generate oriented bounding box (OBB) trees.
// .SECTION Caveats
// Since this algorithms works of a list of points, the OBB tree will only 
// bound the "geometry" attached to the points if the convex hull of the 
// points bounds the geometry.
// .SECTION See Also
// vtkOBBSimplification vtkLocator vtkCellLocator 

#ifndef __vtkOBBTree_h
#define __vtkOBBTree_h

#include "vtkObject.hh"
#include "vtkPolyData.hh"
#include "vtkIdList.hh"

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

class vtkOBBTree : public vtkObject
{
public:
  vtkOBBTree();
  virtual ~vtkOBBTree();
  char *GetClassName() {return "vtkOBBTree";};
  void Initialize();
  void FreeSearchStructure();

  // Description:
  // Build the OBB from the points/cells defining this dataset.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of points in leaf bounding box.
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of points in each leaf bounding box.
  // This is used to automatically generate the OBB.
  vtkSetClampMacro(NumberOfCellsPerOBB,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfCellsPerOBB,int);

  // Description:
  // Set the level of the OBB tree (set automatically if Automatic is true).
  vtkSetClampMacro(Level,int,1,this->MaxLevel);
  vtkGetMacro(Level,int);

  // Description:
  // Set the maximum allowable level for the OBB tree.
  vtkSetClampMacro(MaxLevel,int,1,24);
  vtkGetMacro(MaxLevel,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // intersection computations.
  vtkSetClampMacro(Tolerance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Boolean controls whether to maintain list of cells in each leaf OBB 
  // node. Normally this is the case, but if the OBB tree is being used
  // as a geometry simplification technique, there is no need to keep them.
  vtkSetMacro(RetainCellLists,int);
  vtkGetMacro(RetainCellLists,int);
  vtkBooleanMacro(RetainCellLists,int);

  void ComputeOBB(vtkFloatPoints *pts, float corner[3], float max[3], 
                  float mid[3], float min[3], float size[3]);

  int IntersectWithLine(float a0[3], float a1[3], float& t, 
                        float x[3], float pcoords[3],int &subId);

  void InitializeTreeIntersection(vtkOBBNode& tree);
  int GetNextTreeIntersection(vtkOBBNode& n1, vtkOBBNode& n2);

  void GenerateRepresentation(int level, float ar, float d, vtkPolyData *pd);

  void Update();

protected:
  void SubDivide();

  vtkDataSet *DataSet;
  int Level;
  int MaxLevel;
  float Tolerance;
  vtkOBBNode *Tree;
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfCellsPerOBB; //Used with previous boolean to control subdivide
  int RetainCellLists;
  vtkTimeStamp SubDivideTime;  

  // methods and variables for building OBB tree
  void BuildTree(vtkIdList *cells, vtkOBBNode *parent, int level);
  vtkFloatPoints *PointsList;
  int *InsertedPoints;
  int OBBCount;
  int DeepestLevel;

  void GeneratePolygons(vtkOBBNode *OBBptr, int level, int repLevel, float ar,
                        float d, vtkFloatPoints* pts, vtkCellArray *polys);

};

#endif


