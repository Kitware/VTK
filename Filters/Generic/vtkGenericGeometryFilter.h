// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericGeometryFilter
 * @brief   extract geometry from data (or convert data to polygonal type)
 *
 * vtkGenericGeometryFilter is a general-purpose filter to extract geometry (and
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
 * When vtkGenericGeometryFilter extracts cells (or boundaries of cells) it
 * will (by default) merge duplicate vertices. This may cause problems
 * in some cases. For example, if you've run vtkPolyDataNormals to
 * generate normals, which may split meshes and create duplicate
 * vertices, vtkGenericGeometryFilter will merge these points back
 * together. Turn merging off to prevent this from occurring.
 *
 * @sa
 * vtkImageDataGeometryFilter vtkStructuredGridGeometryFilter
 * vtkExtractGeometry vtkExtractVOI
 */

#ifndef vtkGenericGeometryFilter_h
#define vtkGenericGeometryFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;
class vtkPointData;

class VTKFILTERSGENERIC_EXPORT vtkGenericGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGenericGeometryFilter* New();
  vtkTypeMacro(vtkGenericGeometryFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping, vtkTypeBool);
  vtkGetMacro(PointClipping, vtkTypeBool);
  vtkBooleanMacro(PointClipping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping, vtkTypeBool);
  vtkGetMacro(CellClipping, vtkTypeBool);
  vtkBooleanMacro(CellClipping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping, vtkTypeBool);
  vtkGetMacro(ExtentClipping, vtkTypeBool);
  vtkBooleanMacro(ExtentClipping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the minimum point id for point id selection.
   */
  vtkSetClampMacro(PointMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMinimum, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify the maximum point id for point id selection.
   */
  vtkSetClampMacro(PointMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMaximum, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify the minimum cell id for point id selection.
   */
  vtkSetClampMacro(CellMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMinimum, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify the maximum cell id for point id selection.
   */
  vtkSetClampMacro(CellMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMaximum, vtkIdType);
  ///@}

  /**
   * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);

  ///@{
  /**
   * Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double extent[6]);
  double* GetExtent() { return this->Extent; }
  ///@}

  ///@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging, vtkTypeBool);
  vtkGetMacro(Merging, vtkTypeBool);
  vtkBooleanMacro(Merging, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the MTime also considering the locator.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory.
   */
  vtkSetMacro(PassThroughCellIds, vtkTypeBool);
  vtkGetMacro(PassThroughCellIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughCellIds, vtkTypeBool);
  ///@}

protected:
  vtkGenericGeometryFilter();
  ~vtkGenericGeometryFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void PolyDataExecute(); // special cases for performance
  void UnstructuredGridExecute();
  void StructuredGridExecute();
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  vtkIdType PointMaximum;
  vtkIdType PointMinimum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  vtkTypeBool PointClipping;
  vtkTypeBool CellClipping;
  vtkTypeBool ExtentClipping;

  vtkTypeBool Merging;
  vtkIncrementalPointLocator* Locator;

  // Used internal by vtkGenericAdaptorCell::Tessellate()
  vtkPointData* InternalPD;

  vtkTypeBool PassThroughCellIds;

private:
  vtkGenericGeometryFilter(const vtkGenericGeometryFilter&) = delete;
  void operator=(const vtkGenericGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
