/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGrid - topologically regular array of data
// .SECTION Description
// vtkStructuredGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredGrid represents a geometric structure that is a
// topologically regular array of points. The topology is that of a cube that
// has been subdivided into a regular array of smaller cubes. Each point/cell
// can be addressed with i-j-k indices. Examples include finite difference 
// grids.
//
// A unusual feature of vtkStructuredGrid is the ability to blank, 
// or "turn-off" points and cells in the dataset. This is controlled by 
// defining a "blanking array" whose values (0,1) specify whether
// a point should be blanked or not.

#ifndef __vtkStructuredGrid_h
#define __vtkStructuredGrid_h

#include "vtkPointSet.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"

class vtkVertex;
class vtkLine;
class vtkQuad;
class vtkHexahedron;
class vtkEmptyCell;

class VTK_COMMON_EXPORT vtkStructuredGrid : public vtkPointSet 
{
public:
  static vtkStructuredGrid *New();

  vtkTypeRevisionMacro(vtkStructuredGrid,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_STRUCTURED_GRID;}

  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return vtkStructuredGrid::New();}

  // Description:
  // Copy the geometric and topological structure of an input poly data object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  vtkIdType GetNumberOfPoints() {return vtkPointSet::GetNumberOfPoints();}
  float *GetPoint(vtkIdType ptId) {return this->vtkPointSet::GetPoint(ptId);}
  void GetPoint(vtkIdType ptId, float p[3])
    {this->vtkPointSet::GetPoint(ptId,p);}
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  void GetCellBounds(vtkIdType cellId, float bounds[6]);
  int GetCellType(vtkIdType cellId);
  vtkIdType GetNumberOfCells();
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {
      vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());
    }
  void Initialize();
  int GetMaxCellSize() {return 8;}; //hexahedron is the largest
  void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                        vtkIdList *cellIds);
  virtual void GetScalarRange(float range[2]);
  float *GetScalarRange() {return this->vtkPointSet::GetScalarRange();}

  // Description:
  // following methods are specific to structured grid
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  virtual int *GetDimensions ();
  virtual void GetDimensions (int dim[3]);

  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Methods for supporting blanking of cells. Blanking turns on or off
  // points in the structured grid, and hence the cells connected to them.
  void SetBlanking(int blanking);
  int GetBlanking() {return this->Blanking;}
  void BlankingOn();
  void BlankingOff();
  void BlankPoint(vtkIdType ptId);
  void UnBlankPoint(vtkIdType ptId);
  
  // Description:
  // Get the array that defines the blanking (visibility) of each point.
  vtkUnsignedCharArray *GetPointVisibility() 
    {return this->PointVisibility;}

  // Description:
  // Set an array that defines the (blanking) visibility of the points 
  // in the grid. Make sure that length of the visibility array matches 
  // the number of points in the grid.
  void SetPointVisibility(vtkUnsignedCharArray *pointVisibility);

  // Description:
  // Return non-zero value if specified point is visible. Use this method 
  // only if blanking has been enabled (with BlankingOn()).
  unsigned char IsPointVisible(vtkIdType ptId)
    {return (this->Blanking ? this->PointVisibility->GetValue(ptId) : 1);}
  
  // Description:
  // Return non-zero value if specified point is visible. Use this method 
  // only if blanking has been enabled (with BlankingOn()).
  unsigned char IsCellVisible(vtkIdType cellId);
  
  // Description:
  // Required for the lowest common denominator for setting the UpdateExtent
  // (i.e. vtkDataSetToStructuredPointsFilter).  This assumes that WholeExtent
  // is valid (UpdateInformation has been called).
  void SetUpdateExtent(int piece, int numPieces, int ghostLevel);
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}

  // Description:
  // Call superclass method to avoid hiding
  void SetUpdateExtent( int x1, int x2, int y1, int y2, int z1, int z2 )
    { this->vtkPointSet::SetUpdateExtent( x1, x2, y1, y2, z1, z2 ); };
  void SetUpdateExtent( int ext[6] )
    { this->vtkPointSet::SetUpdateExtent( ext ); };

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent,int);

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
  vtkStructuredGrid();
  ~vtkStructuredGrid();

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkQuad *Quad;  
  vtkHexahedron *Hexahedron;
  vtkEmptyCell *EmptyCell;

  // The extent type is a 3D extent
  int GetExtentType() { return VTK_3D_EXTENT; }
  
  // Description:
  // Reallocates and copies to set the Extent to the UpdateExtent.
  // This is used internally when the exact extent is requested, 
  // and the source generated more than the update extent. 
  virtual void Crop();

  int Dimensions[3];
  int DataDescription;
  int Blanking;
  vtkUnsignedCharArray *PointVisibility;
  void AllocatePointVisibility();

private:
  // Description:
  // For legacy compatibility. Do not use.
  void GetCellNeighbors(vtkIdType cellId, vtkIdList& ptIds, vtkIdList& cellIds)
    {this->GetCellNeighbors(cellId, &ptIds, &cellIds);}

  // Internal method used by DeepCopy and ShallowCopy.
  void InternalStructuredGridCopy(vtkStructuredGrid *src);

private:
  vtkStructuredGrid(const vtkStructuredGrid&);  // Not implemented.
  void operator=(const vtkStructuredGrid&);  // Not implemented.
};


inline vtkIdType vtkStructuredGrid::GetNumberOfCells() 
{
  int nCells=1;
  int dims[3];
  int i;

  this->GetDimensions(dims);
  for (i=0; i<3; i++)
    {
    if (dims[i] <= 0)
      {
      return 0;
      }
    if (dims[i] > 1)
      {
      nCells *= (dims[i]-1);
      }
    }

  return nCells;
}

inline int vtkStructuredGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif






