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
/**
 * @class   vtkUniformGrid
 * @brief   image data with blanking
 *
 * vtkUniformGrid is a subclass of vtkImageData. In addition to all
 * the image data functionality, it supports blanking.
*/

#ifndef vtkUniformGrid_h
#define vtkUniformGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImageData.h"

class vtkEmptyCell;
class vtkStructuredVisibilityConstraint;
class vtkUnsignedCharArray;
class vtkAMRBox;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGrid : public vtkImageData
{
public:
  //@{
  /**
   * Construct an empty uniform grid.
   */
  static vtkUniformGrid *New();
  vtkTypeMacro(vtkUniformGrid,vtkImageData);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Copy the geometric and topological structure of an input image data
   * object.
   */
  void CopyStructure(vtkDataSet *ds) VTK_OVERRIDE;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_UNIFORM_GRID;};

  //@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  vtkCell *GetCell(int i, int j, int k) VTK_OVERRIDE;
  vtkCell *GetCell(vtkIdType cellId) VTK_OVERRIDE;
  void GetCell(vtkIdType cellId, vtkGenericCell *cell) VTK_OVERRIDE;
  vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkIdType cellId, double tol2,
    int& subId, double pcoords[3], double *weights) VTK_OVERRIDE;
  vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkGenericCell *gencell,
    vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double *weights) VTK_OVERRIDE;
  vtkCell *FindAndGetCell(
    double x[3], vtkCell *cell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3],
    double *weights) VTK_OVERRIDE;
  int GetCellType(vtkIdType cellId) VTK_OVERRIDE;
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds) VTK_OVERRIDE
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->GetDataDescription(),
                                      this->GetDimensions());}
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds) VTK_OVERRIDE
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  void Initialize() VTK_OVERRIDE;
  int GetMaxCellSize() VTK_OVERRIDE {return 8;}; //voxel is the largest
  //@}

  /**
   * Returns the data description of this uniform grid instance.
   */
  int GetGridDescription();

  /**
   * Initialize with no ghost cell arrays, from the definition in
   * the given box. The box is expetced to be 3D, if you have 2D
   * data the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox *def, double* origin, double* spacing);
  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays nGhosts cells thick in all directions. The box is expetced
   * to be 3D, if you have 2D data the set the third dimensions 0.
   * eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox *def, double* origin, double* spacing, int nGhosts);

  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays of the thickness given in each direction by "nGhosts" array.
   * The box and ghost array are expected to be 3D, if you have 2D data
   * the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox *def, double* origin, double* spacing, const int nGhosts[3]);
  /**
   * Construct a uniform grid, from the definition in the given box
   * "def", with ghost cell arrays of the thickness given in each
   * direction by "nGhosts*". The box and ghost array are expected
   * to be 3D, if you have 2D data the set the third dimensions 0. eg.
   * (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox *def, double* origin, double* spacing, int nGhostsI,int nGhostsJ,int nGhostsK);

  //@{
  /**
   * Methods for supporting blanking of cells. Blanking turns on or off
   * points in the structured grid, and hence the cells connected to them.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual void BlankPoint(vtkIdType ptId);
  virtual void UnBlankPoint(vtkIdType ptId);
  virtual void BlankPoint( const int i, const int j, const int k );
  virtual void UnBlankPoint( const int i, const int j, const int k );
  //@}

  //@{
  /**
   * Methods for supporting blanking of cells. Blanking turns on or off
   * cells in the structured grid.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual void BlankCell(vtkIdType ptId);
  virtual void UnBlankCell(vtkIdType ptId);
  virtual void BlankCell( const int i, const int j, const int k );
  virtual void UnBlankCell( const int i, const int j, const int k );
  //@}

  /**
   * Returns 1 if there is any visibility constraint on the cells,
   * 0 otherwise.
   */
  bool HasAnyBlankCells() VTK_OVERRIDE;
  /**
   * Returns 1 if there is any visibility constraint on the points,
   * 0 otherwise.
   */
  bool HasAnyBlankPoints() VTK_OVERRIDE;

  /**
   * Return non-zero value if specified point is visible.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual unsigned char IsPointVisible(vtkIdType ptId);

  /**
   * Return non-zero value if specified cell is visible.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual unsigned char IsCellVisible(vtkIdType cellId);

  virtual vtkImageData* NewImageDataCopy();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkUniformGrid* GetData(vtkInformation* info);
  static vtkUniformGrid* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkUniformGrid();
  ~vtkUniformGrid() VTK_OVERRIDE;

  /**
   * Returns the cell dimensions for this vtkUniformGrid instance.
   */
  void GetCellDims( int cellDims[3] );

  /**
   * Override this method because of blanking.
   */
  void ComputeScalarRange() VTK_OVERRIDE;

  vtkEmptyCell* GetEmptyCell();

private:
  vtkUniformGrid(const vtkUniformGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUniformGrid&) VTK_DELETE_FUNCTION;

  vtkEmptyCell *EmptyCell;

  static unsigned char MASKED_CELL_VALUE;
};


#endif



