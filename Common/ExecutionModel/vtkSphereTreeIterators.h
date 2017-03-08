/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTreeIterators.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSphereTreeIterators
 * @brief   a variety of high-performance iterators for sphere trees
 *
 * This header file is provided to achieve inline, optimized performance
 * for iteration over sphere trees.
 */

#ifndef vtkSphereTreeIterators_h
#define vtkSphereTreeIterators_h

#include "vtkSphereTree.h"
#include "vtkMath.h"
#include "vtkDataSet.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

// This is the base class for a variety of iterators.
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSphereTreeIterator : public vtkObjectBase
{
public:
  virtual vtkIdType Begin() = 0;
  virtual vtkIdType Next() = 0;
  inline vtkIdType End() {return -1;}

  //@{
  /**
   * Decrease the reference count (release by another object). This has
   * the same effect as invoking Delete() (i.e., it reduces the reference
   * count by 1).
   */
  //@}
  void UnRegister();
  virtual void UnRegister(vtkObjectBase *)
    { this->UnRegister(); }

protected:
  friend class vtkSphereTree;

  vtkSphereTreeIterator();
  void Initialize(vtkIdType taskNum);

  vtkSphereTree *Tree;
  vtkIdType      TaskNumber;
  vtkIdType      NumberOfTasks;
};

//============================================================================
//===================Iterator definitions=====================================
vtkSphereTreeIterator::vtkSphereTreeIterator()
  : Tree(NULL), TaskNumber(0), NumberOfTasks(0)
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkSphereTreeIterator or subclass");
#endif
}

//----------------------------------------------------------------------------
void vtkSphereTreeIterator::UnRegister()
{
  int refcount = this->GetReferenceCount()-1;
  this->SetReferenceCount(refcount);
  if (refcount <= 0)
  {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkSphereTreeIterator or subclass");
#endif
    delete this;
  }
}

//----------------------------------------------------------------------------
void vtkSphereTreeIterator::Initialize(vtkIdType taskNum)
{
  if ( taskNum < 0 )
  {
    this->TaskNumber = 0;
    this->NumberOfTasks = 1;
  }
  else
  {
    this->TaskNumber = taskNum;
    this->NumberOfTasks = this->Tree->NumberOfTasks;
  }
}

//----------------------------------------------------------------------------
// Common superclass for plane iterators
class vtkSphereTreePlaneIterator : public vtkSphereTreeIterator
{
public:
  vtkSphereTreePlaneIterator();
  virtual void Initialize(vtkIdType taskNum,
                          double normal[3], double origin[3], double value);

  double      Normal[3];
  double      Origin[3];
  double      Value;
};
//----------------------------------------------------------------------------
vtkSphereTreePlaneIterator::vtkSphereTreePlaneIterator()
{
  this->Normal[0] = this->Normal[1] = 0.0; this->Normal[2] = 1.0;
  this->Origin[0] = this->Origin[0] = this->Origin[0] = 0.0;
  this->Value = 0.0;
}
//----------------------------------------------------------------------------
// Generally called by subclass
void vtkSphereTreePlaneIterator::
Initialize(vtkIdType taskNum, double normal[3], double origin[3], double value)
{
  this->Normal[0] = normal[0];
  this->Normal[1] = normal[1];
  this->Normal[2] = normal[2];
  vtkMath::Normalize(this->Normal); //just to be sure

  this->Origin[0] = origin[0];
  this->Origin[1] = origin[1];
  this->Origin[2] = origin[2];

  this->Value = value;

  this->vtkSphereTreeIterator::Initialize(taskNum);
}

//----------------------------------------------------------------------------
// This class is used when there is no sphere tree hierarchy. Hence it just
// processes the spheres directly.
class vtkSphereTreeDefaultPlaneIterator : public vtkSphereTreePlaneIterator
{
public:
  static vtkSphereTreeDefaultPlaneIterator *New()
    {return new vtkSphereTreeDefaultPlaneIterator;}

  vtkSphereTreeDefaultPlaneIterator() :
    Input(NULL), CellId(0), StartCellId(0), EndCellId(0) {}

  virtual void Initialize(vtkIdType taskNum,
                          double normal[3], double origin[3], double value);

  virtual vtkIdType Begin();
  virtual vtkIdType Next();

  vtkDataSet *Input;
  vtkIdType CellId;
  vtkIdType StartCellId;
  vtkIdType EndCellId;
};
//----------------------------------------------------------------------------
void vtkSphereTreeDefaultPlaneIterator::
Initialize(vtkIdType taskNum, double normal[3], double origin[3], double value)
{
  this->vtkSphereTreePlaneIterator::
    Initialize(taskNum,normal,origin,value);

  vtkIdType numCells = this->Input->GetNumberOfCells();
  if ( this->NumberOfTasks == 1 )
  {//process entire dataset (serial taversal)
    this->CellId = this->StartCellId = 0;
    this->EndCellId = numCells;
  }
  else
  {//process a task of data (parallel traversal)
    this->StartCellId = this->CellId = taskNum * this->Tree->TaskSize;
    if ( taskNum >= (this->NumberOfTasks-1) )
    {
      this->EndCellId = numCells;
    }
    else
    {
      this->EndCellId = this->StartCellId + this->Tree->TaskSize;
      this->EndCellId = (this->EndCellId > numCells ? numCells : this->EndCellId );
    }
  }

}
//----------------------------------------------------------------------------
vtkIdType vtkSphereTreeDefaultPlaneIterator::Begin()
{
  this->CellId = this->StartCellId;
  return Next();
}
//----------------------------------------------------------------------------
vtkIdType vtkSphereTreeDefaultPlaneIterator::Next()
{
  vtkIdType cellId;
  double *sphere, dist;

  while ( this->CellId < this->EndCellId )
  {
    cellId = this->CellId++;
    sphere = this->Tree->TreePtr + 4*cellId;
    dist = vtkPlane::DistanceToPlane(sphere,this->Normal,this->Origin);
    if ( (dist+this->Value) <= sphere[3] )
    {
      return cellId;
    }
  }

  return this->End(); //traversal ends
}

//----------------------------------------------------------------------------
class vtkSphereTreeStructuredPlaneIterator : public vtkSphereTreePlaneIterator
{
public:
  static vtkSphereTreeStructuredPlaneIterator *New()
    {return new vtkSphereTreeStructuredPlaneIterator;}
  vtkSphereTreeStructuredPlaneIterator() :
    MaxLevel(0),CurrentLevel(0) {}

  virtual void Initialize(vtkIdType taskNum,
                          double normal[3], double origin[3], double value);
  virtual vtkIdType Begin();
  virtual vtkIdType Next();

  // Customized for structured grid
  vtkStructuredGrid *Input;

  // control iteration
  vtkIdType CellId;
  int       MaxLevel;
  int       CurrentLevel; //current level in the tree during iteration
  int       Dimensions[3];

  // track information for each level
  int        LDims[VTK_MAX_SPHERE_TREE_LEVELS][3];
  int        IJKStart[VTK_MAX_SPHERE_TREE_LEVELS][3];
  int        IJKEnd[VTK_MAX_SPHERE_TREE_LEVELS][3];
  int        IJK[VTK_MAX_SPHERE_TREE_LEVELS][3];
  int        SliceOffset[VTK_MAX_SPHERE_TREE_LEVELS];
  double    *Spheres[VTK_MAX_SPHERE_TREE_LEVELS];
};
//----------------------------------------------------------------------------
void vtkSphereTreeStructuredPlaneIterator::
Initialize(vtkIdType taskNum,
           double normal[3], double origin[3], double value)
{
  // Update grid dimensions
  this->Input->GetDimensions(this->Dimensions);

  // Determine traversal range (task)
  this->vtkSphereTreePlaneIterator::
    Initialize(taskNum,normal,origin,value);
}
//----------------------------------------------------------------------------
vtkIdType vtkSphereTreeStructuredPlaneIterator::Begin()
{
  // Initialize data structures
  this->Spheres[0] = static_cast<vtkStructuredHierarchy*>
    (this->Tree->Hierarchy)->H->GetPointer(0);
  this->MaxLevel = static_cast<int>(this->Spheres[0][0]);
  vtkIdType level, leafLevel = this->MaxLevel - 1;
  int resolution = this->Tree->Resolution;
  this->Spheres[0] += 2; //offset due to tree header information
  this->Input->GetDimensions(this->LDims[leafLevel]);

  vtkIdType size[VTK_MAX_SPHERE_TREE_LEVELS];
  this->LDims[leafLevel][0] -= 1;
  this->LDims[leafLevel][1] -= 1;
  this->LDims[leafLevel][2] -= 1;
  this->SliceOffset[leafLevel] = this->LDims[leafLevel][0]*this->LDims[leafLevel][1];
  size[leafLevel] = this->LDims[leafLevel][0]*this->LDims[leafLevel][1]*this->LDims[leafLevel][2];

  for (level=this->MaxLevel-2; level >= 0; --level)
  {
    this->LDims[level][0] = (this->LDims[level+1][0]-1)/resolution + 1;
    this->LDims[level][1] = (this->LDims[level+1][1]-1)/resolution + 1;
    this->LDims[level][2] = (this->LDims[level+1][2]-1)/resolution + 1;
    this->SliceOffset[level] = this->LDims[level][0]*this->LDims[level][1];
    size[level] = this->LDims[level][0]*this->LDims[level][1]*this->LDims[level][2];
  }

  for (level=1; level < this->MaxLevel-1; ++level)
  {
    this->Spheres[level] = this->Spheres[level-1] + 4*size[level-1];
  }
  this->Spheres[leafLevel] = this->Tree->TreePtr;

  // Initial starting traversal position at all levels of the tree.
  for (level=0; level < this->MaxLevel; ++level)
  {
    this->IJK[level][0] = 0;
    this->IJK[level][1] = 0;
    this->IJK[level][2] = 0;
    this->IJKStart[level][0] = 0;
    this->IJKStart[level][1] = 0;
    this->IJKStart[level][2] = 0;
    this->IJKEnd[level][0] = resolution;
    this->IJKEnd[level][1] = resolution;
    this->IJKEnd[level][2] = resolution;
  }

  // This sets up the initial traversal to start from the top level of the tree
  this->IJK[0][0] = -1;
  this->IJKEnd[0][0] = this->LDims[0][0];
  this->IJKEnd[0][1] = this->LDims[0][1];
  this->IJKEnd[0][2] = this->LDims[0][2];

  this->CurrentLevel = 0;
  this->CellId = 0;
  return this->Next();
}

//----------------------------------------------------------------------------
vtkIdType vtkSphereTreeStructuredPlaneIterator::Next()
{
  // Advance to the next sphere block, or cell's sphere. This may cause a reset
  // of all indices throughout the tree.
  int upLevel, level, resolution=this->Tree->Resolution;
  double dist, *sphere;
  vtkIdType iStart, jStart, kStart;

  while ( 1 )
  {
    upLevel = 0;
    level = this->CurrentLevel;
    if ( ++this->IJK[level][0] >= this->IJKEnd[level][0] )
    {
      this->IJK[level][0] = this->IJKStart[level][0];
      if ( ++this->IJK[level][1] >= this->IJKEnd[level][1] )
      {
        this->IJK[level][1] = this->IJKStart[level][1];
        if ( ++this->IJK[level][2] >= this->IJKEnd[level][2] )
        {
          upLevel = 1;
        } //advance k
      } //advance j
    } //advance i

    // Let's see if we've popped up from a lower level. If not, evaluate
    // distance to plane and see if we should proceed.
    if ( ! upLevel )
    {
      vtkIdType cellId =
        this->IJK[level][0] + this->IJK[level][1]*this->LDims[level][0] +
        this->IJK[level][2]*this->SliceOffset[level];
      sphere = this->Spheres[level] + 4*cellId;
      dist = vtkPlane::DistanceToPlane(sphere,this->Normal,this->Origin);
      if ( (dist+this->Value) < sphere[3] )
      {
        if ( level == this->MaxLevel-1) //if deepest level
        {
          return cellId;
        }
        else //we have to descend into deeper block
        {
          this->IJKStart[level+1][0] = this->IJK[level+1][0] = iStart =
            this->IJK[level][0] * resolution;
          this->IJK[level+1][0] -= 1; //prepare for iteration
          this->IJKStart[level+1][1] = this->IJK[level+1][1] = jStart =
            this->IJK[level][1] * resolution;
          this->IJKStart[level+1][2] = this->IJK[level+1][2] = kStart =
            this->IJK[level][2] * resolution;
          this->IJKEnd[level+1][0] =
            ( iStart+resolution < this->LDims[level+1][0] ?
              iStart+resolution : this->LDims[level+1][0]);
          this->IJKEnd[level+1][1] =
            ( jStart+resolution < this->LDims[level+1][1] ?
              jStart+resolution : this->LDims[level+1][1]);
          this->IJKEnd[level+1][2] =
            ( kStart+resolution < this->LDims[level+1][2] ?
              kStart+resolution : this->LDims[level+1][2]);

          this->CurrentLevel++;
        }
      }
    }

    else //we are at a new, higher level and need to advance at that level
    {
      if ( --this->CurrentLevel < 0 )
      {
        return this->End(); //we are done traversing!
      }
    }
  } //while
}

//----------------------------------------------------------------------------
class vtkSphereTreeUnstructuredPlaneIterator :
  public vtkSphereTreePlaneIterator
{
public:
  static vtkSphereTreeUnstructuredPlaneIterator *New()
    {return new vtkSphereTreeUnstructuredPlaneIterator;}
  vtkSphereTreeUnstructuredPlaneIterator() {}

  virtual void Initialize(vtkIdType taskNum,
                          double normal[3], double origin[3], double value);
  virtual vtkIdType Begin();
  virtual vtkIdType Next();

  // Customized for unstructured grid
  vtkUnstructuredGrid *Input;

};
//----------------------------------------------------------------------------
void vtkSphereTreeUnstructuredPlaneIterator::
Initialize(vtkIdType taskNum,
           double normal[3], double origin[3], double value)
{
  // Determine traversal range (task)
  this->vtkSphereTreePlaneIterator::
    Initialize(taskNum,normal,origin,value);
}
//----------------------------------------------------------------------------
// Find the first cell to process; set the stage for continued processing.
vtkIdType vtkSphereTreeUnstructuredPlaneIterator::Begin()
{
  return this->End();
}
//----------------------------------------------------------------------------
vtkIdType vtkSphereTreeUnstructuredPlaneIterator::Next()
{
  return this->End();
}


#endif
