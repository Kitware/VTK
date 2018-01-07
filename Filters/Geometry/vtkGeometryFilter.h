/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGeometryFilter
 * @brief   extract geometry from data (or convert data to polygonal type)
 *
 * vtkGeometryFilter is a general-purpose filter to extract geometry (and
 * associated data) from any type of dataset. Geometry is obtained as
 * follows: all 0D, 1D, and 2D cells are extracted. All 2D faces that are
 * used by only one 3D cell (i.e., boundary faces) are extracted. It also is
 * possible to specify conditions on point ids, cell ids, and on
 * bounding box (referred to as "Extent") to control the extraction process.
 *
 * This filter also may be used to convert any type of data to polygonal
 * type. The conversion process may be less than satisfactory for some 3D
 * datasets. For example, this filter will extract the outer surface of a
 * volume or structured grid dataset. (For structured data you may want to
 * use vtkImageDataGeometryFilter, vtkStructuredGridGeometryFilter,
 * vtkExtractUnstructuredGrid, vtkRectilinearGridGeometryFilter, or
 * vtkExtractVOI.)
 *
 * @warning
 * When vtkGeometryFilter extracts cells (or boundaries of cells) it
 * will (by default) merge duplicate vertices. This may cause problems
 * in some cases. For example, if you've run vtkPolyDataNormals to
 * generate normals, which may split meshes and create duplicate
 * vertices, vtkGeometryFilter will merge these points back
 * together. Turn merging off to prevent this from occurring.
 *
 * @warning
 * This filter assumes that the input dataset is composed of either:
 * 0D cells OR 1D cells OR 2D and/or 3D cells. In other words,
 * the input dataset cannot be a combination of different dimensional cells
 * with the exception of 2D and 3D cells.
 *
 * @sa
 * vtkImageDataGeometryFilter vtkStructuredGridGeometryFilter
 * vtkExtractGeometry vtkExtractVOI
*/

#ifndef vtkGeometryFilter_h
#define vtkGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSGEOMETRY_EXPORT vtkGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGeometryFilter *New();
  vtkTypeMacro(vtkGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping,vtkTypeBool);
  vtkGetMacro(PointClipping,vtkTypeBool);
  vtkBooleanMacro(PointClipping,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping,vtkTypeBool);
  vtkGetMacro(CellClipping,vtkTypeBool);
  vtkBooleanMacro(CellClipping,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping,vtkTypeBool);
  vtkGetMacro(ExtentClipping,vtkTypeBool);
  vtkBooleanMacro(ExtentClipping,vtkTypeBool);
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
  double *GetExtent() VTK_SIZEHINT(6) { return this->Extent;};
  //@}

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging,vtkTypeBool);
  vtkGetMacro(Merging,vtkTypeBool);
  vtkBooleanMacro(Merging,vtkTypeBool);
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
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. This only applies for data types where
   * we create points as opposed to pass them, such as rectilinear grid.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

protected:
  vtkGeometryFilter();
  ~vtkGeometryFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  //special cases for performance
  void PolyDataExecute(vtkDataSet *, vtkPolyData *);
  void UnstructuredGridExecute(vtkDataSet *, vtkPolyData *);
  void StructuredGridExecute(vtkDataSet *, vtkPolyData *, vtkInformation *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  vtkIdType PointMaximum;
  vtkIdType PointMinimum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  vtkTypeBool PointClipping;
  vtkTypeBool CellClipping;
  vtkTypeBool ExtentClipping;
  int OutputPointsPrecision;

  vtkTypeBool Merging;
  vtkIncrementalPointLocator *Locator;
private:
  vtkGeometryFilter(const vtkGeometryFilter&) = delete;
  void operator=(const vtkGeometryFilter&) = delete;
};

#endif


