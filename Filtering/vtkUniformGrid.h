/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUniformGrid - image data with blanking
// .SECTION Description
// vtkUniformGrid is a subclass of vtkImageData. In addition to all
// the image data functionality, it supports blanking.

#ifndef __vtkUniformGrid_h
#define __vtkUniformGrid_h

#include "vtkImageData.h"

class vtkEmptyCell;
class vtkStructuredVisibilityConstraint;
class vtkUnsignedCharArray;
class vtkAMRBox;

class VTK_FILTERING_EXPORT vtkUniformGrid : public vtkImageData
{
public:
  // Description:
  // Construct an empty uniform grid.
  static vtkUniformGrid *New();
  vtkTypeMacro(vtkUniformGrid,vtkImageData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the geometric and topological structure of an input image data
  // object.
  virtual void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  virtual int GetDataObjectType() {return VTK_UNIFORM_GRID;};

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  virtual vtkCell *GetCell(vtkIdType cellId);
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  virtual vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkIdType cellId, double tol2,
    int& subId, double pcoords[3], double *weights);
  virtual vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkGenericCell *gencell,
    vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double *weights);
  virtual vtkCell *FindAndGetCell(
    double x[3], vtkCell *cell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3],
    double *weights);
  virtual int GetCellType(vtkIdType cellId);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
                                      this->GetDimensions());}
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  virtual void Initialize();
  virtual int GetMaxCellSize() {return 8;}; //voxel is the largest

  //BTX
  // Description:
  // Initialize with no ghost cell arrays, from the definition in
  // the given box. The box is expetced to be 3D, if you have 2D 
  // data the set the third dimensions 0. eg. (X,X,0)(X,X,0)
  // Returns 0 if the initialization failed.
  int Initialize(const vtkAMRBox *def);
  // Description:
  // Initialize from the definition in the given box, with ghost cell
  // arrays nGhosts cells thick in all directions. The box is expetced
  // to be 3D, if you have 2D data the set the third dimensions 0.
  // eg. (X,X,0)(X,X,0)
  // Returns 0 if the initialization failed.
  int Initialize(const vtkAMRBox *def, int nGhosts);
  // Description:
  // Initialize from the definition in the given box, with ghost cell 
  // arrays of the thickness given in each direction by "nGhosts" array.
  // The box and ghost array are expected to be 3D, if you have 2D data 
  // the set the third dimensions 0. eg. (X,X,0)(X,X,0)
  // Returns 0 if the initialization failed.
  int Initialize(const vtkAMRBox *def, const int nGhosts[3]);
  // Description:
  // Construct a uniform grid, from the definition in the given box
  // "def", with ghost cell arrays of the thickness given in each 
  // direction by "nGhosts*". The box and ghost array are expected
  // to be 3D, if you have 2D data the set the third dimensions 0. eg.
  // (X,X,0)(X,X,0)
  // Returns 0 if the initialization failed.
  int Initialize(const vtkAMRBox *def,int nGhostsI,int nGhostsJ,int nGhostsK);
  //ETX

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Methods for supporting blanking of cells. Blanking turns on or off
  // points in the structured grid, and hence the cells connected to them.
  // These methods should be called only after the dimensions of the
  // grid are set.
  virtual void BlankPoint(vtkIdType ptId);
  virtual void UnBlankPoint(vtkIdType ptId);

  // Description:
  // Methods for supporting blanking of cells. Blanking turns on or off
  // cells in the structured grid.
  // These methods should be called only after the dimensions of the
  // grid are set.
  virtual void BlankCell(vtkIdType ptId);
  virtual void UnBlankCell(vtkIdType ptId);

  // Description:
  // Get the array that defines the blanking (visibility) of each point.
  virtual vtkUnsignedCharArray *GetPointVisibilityArray();

  // Description:
  // Set an array that defines the (blanking) visibility of the points
  // in the grid. Make sure that length of the visibility array matches
  // the number of points in the grid.
  virtual void SetPointVisibilityArray(vtkUnsignedCharArray *pointVisibility);

  // Description:
  // Get the array that defines the blanking (visibility) of each cell.
  virtual vtkUnsignedCharArray *GetCellVisibilityArray();

  // Description:
  // Set an array that defines the (blanking) visibility of the cells
  // in the grid. Make sure that length of the visibility array matches
  // the number of points in the grid.
  virtual void SetCellVisibilityArray(vtkUnsignedCharArray *pointVisibility);

  // Description:
  // Return non-zero value if specified point is visible.
  // These methods should be called only after the dimensions of the
  // grid are set.
  virtual unsigned char IsPointVisible(vtkIdType ptId);

  // Description:
  // Return non-zero value if specified cell is visible.
  // These methods should be called only after the dimensions of the
  // grid are set.
  virtual unsigned char IsCellVisible(vtkIdType cellId);

  // Description:
  // Returns 1 if there is any visibility constraint on the points,
  // 0 otherwise.
  virtual unsigned char GetPointBlanking();

  // Description:
  // Returns 1 if there is any visibility constraint on the cells,
  // 0 otherwise.
  virtual unsigned char GetCellBlanking();

  virtual vtkImageData* NewImageDataCopy();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkUniformGrid* GetData(vtkInformation* info);
  static vtkUniformGrid* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkUniformGrid();
  ~vtkUniformGrid();
  
  // Description:
  // Override this method because of blanking.
  virtual void ComputeScalarRange();
  
  vtkStructuredVisibilityConstraint* PointVisibility;

  void SetPointVisibility(vtkStructuredVisibilityConstraint *pointVisibility);
  vtkGetObjectMacro(PointVisibility, vtkStructuredVisibilityConstraint);

  vtkStructuredVisibilityConstraint* CellVisibility;

  void SetCellVisibility(vtkStructuredVisibilityConstraint *cellVisibility);
  vtkGetObjectMacro(CellVisibility, vtkStructuredVisibilityConstraint);

  vtkEmptyCell *EmptyCell;

private:
  vtkUniformGrid(const vtkUniformGrid&);  // Not implemented.
  void operator=(const vtkUniformGrid&);  // Not implemented.
};


#endif



