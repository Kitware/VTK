/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridGeometryFilter
 * @brief   extract geometry from an unstructured grid
 *
 * vtkUnstructuredGridGeometryFilter is a filter that extracts
 * geometry (and associated data) from an unstructured grid. It differs from
 * vtkGeometryFilter by not tessellating higher order faces: 2D faces of
 * quadratic 3D cells will be quadratic. A quadratic edge is extracted as a
 * quadratic edge. For that purpose, the output of this filter is an
 * unstructured grid, not a polydata.
 * Also, the face of a voxel is a pixel, not a quad.
 * Geometry is obtained as follows: all 0D, 1D, and 2D cells are extracted.
 * All 2D faces that are used by only one 3D cell (i.e., boundary faces) are
 * extracted. It also is possible to specify conditions on point ids, cell ids,
 * and on bounding box (referred to as "Extent") to control the extraction
 * process.
 *
 * @warning
 * When vtkUnstructuredGridGeometryFilter extracts cells (or boundaries of
 * cells) it will (by default) merge duplicate vertices. This may cause
 * problems in some cases. Turn merging off to prevent this from occurring.
 *
 * @sa
 * vtkGeometryFilter
*/

#ifndef vtkUnstructuredGridGeometryFilter_h
#define vtkUnstructuredGridGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkUnstructuredGridBaseAlgorithm.h"

class vtkIncrementalPointLocator;
class vtkHashTableOfSurfels; // internal class

class VTKFILTERSGEOMETRY_EXPORT vtkUnstructuredGridGeometryFilter
    : public vtkUnstructuredGridBaseAlgorithm
{
public:
  static vtkUnstructuredGridGeometryFilter *New();
  vtkTypeMacro(vtkUnstructuredGridGeometryFilter,
               vtkUnstructuredGridBaseAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping,int);
  vtkGetMacro(PointClipping,int);
  vtkBooleanMacro(PointClipping,int);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping,int);
  vtkGetMacro(CellClipping,int);
  vtkBooleanMacro(CellClipping,int);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping,int);
  vtkGetMacro(ExtentClipping,int);
  vtkBooleanMacro(ExtentClipping,int);
  //@}

  //@{
  /**
   * Specify the minimum point id for point id selection.
   */
  vtkSetClampMacro(PointMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMinimum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum point id for point id selection.
   */
  vtkSetClampMacro(PointMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMaximum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the minimum cell id for point id selection.
   */
  vtkSetClampMacro(CellMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMinimum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum cell id for point id selection.
   */
  vtkSetClampMacro(CellMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMaximum,vtkIdType);
  //@}

  /**
   * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double xMin, double xMax, double yMin, double yMax,
                 double zMin, double zMax);

  //@{
  /**
   * Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double extent[6]);
  double *GetExtent() { return this->Extent;};
  //@}

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);
  //@}

  //@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
   * since in that case each tringle strip can represent more than on of the
   * input cells.
   */
  vtkSetMacro(PassThroughCellIds,int);
  vtkGetMacro(PassThroughCellIds,int);
  vtkBooleanMacro(PassThroughCellIds,int);
  vtkSetMacro(PassThroughPointIds,int);
  vtkGetMacro(PassThroughPointIds,int);
  vtkBooleanMacro(PassThroughPointIds,int);
  //@}

  //@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to NULL, then vtkOriginalCellIds or vtkOriginalPointIds (the default)
   * is used, respectively.
   */
  vtkSetStringMacro(OriginalCellIdsName);
  virtual const char *GetOriginalCellIdsName() {
    return (  this->OriginalCellIdsName
            ? this->OriginalCellIdsName : "vtkOriginalCellIds");
  }
  vtkSetStringMacro(OriginalPointIdsName);
  virtual const char *GetOriginalPointIdsName() {
    return (  this->OriginalPointIdsName
            ? this->OriginalPointIdsName : "vtkOriginalPointIds");
  }
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the MTime also considering the locator.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkUnstructuredGridGeometryFilter();
  ~vtkUnstructuredGridGeometryFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  vtkIdType PointMaximum;
  vtkIdType PointMinimum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

  int PassThroughCellIds;
  int PassThroughPointIds;
  char *OriginalCellIdsName;
  char *OriginalPointIdsName;

  int Merging;
  vtkIncrementalPointLocator *Locator;

  vtkHashTableOfSurfels *HashTable;

private:
  vtkUnstructuredGridGeometryFilter(const vtkUnstructuredGridGeometryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridGeometryFilter&) VTK_DELETE_FUNCTION;
};

#endif
