// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeometryFilter
 * @brief   extract boundary geometry from dataset (or convert data to polygonal type)
 *
 * vtkGeometryFilter is a general-purpose filter to extract dataset boundary
 * geometry, topology, and associated attribute data from any type of
 * dataset. Geometry is obtained as follows: all 0D, 1D, and 2D cells are
 * extracted. All 2D faces that are used by only one 3D cell (i.e., boundary
 * faces) are extracted. It also is possible to specify conditions on point
 * ids, cell ids, and on a bounding box (referred to as "Extent") to control
 * the extraction process.  This point and cell id- and extent-based clipping
 * is a powerful way to "see inside" datasets; however it may impact
 * performance significantly.
 *
 * This filter may also be used to convert any type of data to polygonal
 * type. This is particularly useful for surface rendering. The conversion
 * process may be less than satisfactory for some 3D datasets. For example,
 * this filter will extract the outer surface of a volume or structured grid
 * dataset (if point, cell, and extent clipping is disabled). (For structured
 * data you may want to use vtkImageDataGeometryFilter,
 * vtkStructuredGridGeometryFilter, vtkExtractUnstructuredGrid,
 * vtkRectilinearGridGeometryFilter, or vtkExtractVOI.)
 *
 * Another important feature of vtkGeometryFilter is that it preserves
 * topological connectivity. This enables filters that depend on correct
 * connectivity (e.g., vtkQuadricDecimation, vtkFeatureEdges, etc.) to
 * operate properly . It is possible to label the output polydata with an
 * originating cell (PassThroughCellIds) or point id (PassThroughPointIds).
 * The output precision of created points (if they need to be created) can
 * also be specified.
 *
 * Finally, this filter takes an optional second, vtkPolyData input. This
 * input represents a list of faces that are to be excluded from the output
 * of vtkGeometryFilter.
 *
 * @warning
 * While vtkGeometryFilter and vtkDataSetSurfaceFilter perform similar operations,
 * there are important differences as follows:
 * 1. vtkGeometryFilter can preserve (using RemoveGhostInterfaces) topological connectivity.
 *    vtkDataSetSurfaceFilter produces output primitives which may be disconnected from one another.
 * 2. vtkGeometryFilter can generate output based on cell ids, point ids, and/or
 *    extent (bounding box) clipping. vtkDataSetSurfaceFilter strictly extracts
 *    the boundary surface of a dataset.
 * 3. vtkGeometryFilter is much faster than vtkDataSetSurfaceFilter, because it's
 *    multi-threaded. As a result, vtkDataSetSurfaceFilter will delegate the processing
 *    of linear unstructured grids to vtkGeometryFilter.
 * 4. vtkGeometryFilter can (currently) only handle linear cells. The filter
 *    will delegate to vtkDataSetSurfaceFilter for higher-order cells. (This
 *    is a historical artifact and may be rectified in the future.)
 *
 * @warning
 * If point merging (MergingOff) is disabled, the filter will (if possible)
 * use the input points and point attributes.  This can result in a lot of
 * unused points in the output, at some gain in filter performance.  If
 * enabled, point merging will generate only new points that are used by the
 * output polydata cells.
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkDataSetSurfaceFilter vtkImageDataGeometryFilter
 * vtkStructuredGridGeometryFilter vtkExtractGeometry vtkExtractVOI
 * vtkMarkBoundaryFilter vtkRemovePolyData
 */

#ifndef vtkGeometryFilter_h
#define vtkGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include <array> // For std::array

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;
class vtkStructuredGrid;
class vtkUnstructuredGridBase;
class vtkGeometryFilter;
class vtkDataSetSurfaceFilter;
struct vtkGeometryFilterHelper;

// Used to coordinate delegation to vtkDataSetSurfaceFilter
struct VTKFILTERSGEOMETRY_EXPORT vtkGeometryFilterHelper
{
  enum CellType
  {
    VERTS = 0,
    LINES = 1,
    POLYS = 2,
    STRIPS = 3,
    OTHER_LINEAR_CELLS = 4,
    NON_LINEAR_CELLS = 5,
    NUM_CELL_TYPES
  };
  using CellTypesInformation = std::array<bool, NUM_CELL_TYPES>;
  CellTypesInformation CellTypesInfo;
  unsigned char IsLinear;
  static vtkGeometryFilterHelper* CharacterizeUnstructuredGrid(vtkUnstructuredGridBase*);
  static void CopyFilterParams(vtkGeometryFilter* gf, vtkDataSetSurfaceFilter* dssf);
  static void CopyFilterParams(vtkDataSetSurfaceFilter* dssf, vtkGeometryFilter* gf);
  bool HasOnlyVerts()
  {
    return this->CellTypesInfo[VERTS] && !this->CellTypesInfo[LINES] &&
      !this->CellTypesInfo[POLYS] && !this->CellTypesInfo[STRIPS] &&
      !this->CellTypesInfo[OTHER_LINEAR_CELLS] && !this->CellTypesInfo[NON_LINEAR_CELLS];
  }
  bool HasOnlyLines()
  {
    return !this->CellTypesInfo[VERTS] && this->CellTypesInfo[LINES] &&
      !this->CellTypesInfo[POLYS] && !this->CellTypesInfo[STRIPS] &&
      !this->CellTypesInfo[OTHER_LINEAR_CELLS] && !this->CellTypesInfo[NON_LINEAR_CELLS];
  }
  bool HasOnlyPolys()
  {
    return !this->CellTypesInfo[VERTS] && !this->CellTypesInfo[LINES] &&
      this->CellTypesInfo[POLYS] && !this->CellTypesInfo[STRIPS] &&
      !this->CellTypesInfo[OTHER_LINEAR_CELLS] && !this->CellTypesInfo[NON_LINEAR_CELLS];
  }
  bool HasOnlyStrips()
  {
    return !this->CellTypesInfo[VERTS] && !this->CellTypesInfo[LINES] &&
      !this->CellTypesInfo[POLYS] && this->CellTypesInfo[STRIPS] &&
      !this->CellTypesInfo[OTHER_LINEAR_CELLS] && !this->CellTypesInfo[NON_LINEAR_CELLS];
  }
};

class VTKFILTERSGEOMETRY_EXPORT vtkGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkGeometryFilter* New();
  vtkTypeMacro(vtkGeometryFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping, bool);
  vtkGetMacro(PointClipping, bool);
  vtkBooleanMacro(PointClipping, bool);
  ///@}

  ///@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping, bool);
  vtkGetMacro(CellClipping, bool);
  vtkBooleanMacro(CellClipping, bool);
  ///@}

  ///@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping, bool);
  vtkGetMacro(ExtentClipping, bool);
  vtkBooleanMacro(ExtentClipping, bool);
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
  double* GetExtent() VTK_SIZEHINT(6) { return this->Extent; }
  ///@}

  ///@{
  /**
   * Turn on/off merging of points. This will reduce the number of output
   * points, at some cost to performance. If Merging is off, then if possible
   * (i.e., if the point representation is explicit), the filter will reuse
   * the input points to create the output polydata. Certain input dataset
   * types (with implicit point representations) will always create new
   * points (effectively performing a merge operation).
   */
  vtkSetMacro(Merging, bool);
  vtkGetMacro(Merging, bool);
  vtkBooleanMacro(Merging, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the
   * documentation for the vtkAlgorithm::DesiredOutputPrecision enum for an
   * explanation of the available precision settings. This only applies for
   * data types where we create points (merging) as opposed to passing them
   * from input to output, such as unstructured grids.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  ///@}

  ///@{
  /**
   * Turn on/off fast mode execution. If enabled, fast mode typically runs
   * much faster (2-3x) than the standard algorithm, however the output is an
   * approximation to the correct result. FastMode is only meaningful when
   * the input is vtkImageData/vtkRectilinearGrid/vtkStructuredGrid and there
   * are blank cells.
   */
  vtkSetMacro(FastMode, bool);
  vtkGetMacro(FastMode, bool);
  vtkBooleanMacro(FastMode, bool);
  ///@}

  // The following are methods compatible with vtkDataSetSurfaceFilter.

  ///@{
  /**
   * If PieceInvariant is true, vtkGeometryFilter requests
   * 1 ghost level from input in order to remove internal surface
   * that are between processes. False by default.
   */
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
  ///@}

  ///@{
  /**
   * This parameter drives the generation or not of a CellData array for the output
   * polygonal dataset that holds the cell index of the original 3D cell that produced
   * each output cell. This is useful for cell picking. The default is off to conserve memory.
   *
   * Note: Use SetOriginalCellIdsName() to set the name of the CellData array.
   */
  vtkSetMacro(PassThroughCellIds, vtkTypeBool);
  vtkGetMacro(PassThroughCellIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughCellIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * This parameter drives the generation or not of a PointData array for the output
   * polygonal dataset that holds the cell/point index of the original point that produced
   * each output point. This is useful for point picking. The default is off to conserve memory.
   *
   * Note: Use SetOriginalPointIdsName() to set the name of the PointData array.
   */
  vtkSetMacro(PassThroughPointIds, vtkTypeBool);
  vtkGetMacro(PassThroughPointIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughPointIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to nullptr, then vtkOriginalCellIds or vtkOriginalPointIds (the default)
   * is used, respectively.
   */
  vtkSetStringMacro(OriginalCellIdsName);
  virtual const char* GetOriginalCellIdsName()
  {
    return (this->OriginalCellIdsName ? this->OriginalCellIdsName : "vtkOriginalCellIds");
  }
  vtkSetStringMacro(OriginalPointIdsName);
  virtual const char* GetOriginalPointIdsName()
  {
    return (this->OriginalPointIdsName ? this->OriginalPointIdsName : "vtkOriginalPointIds");
  }
  ///@}

  ///@{
  /**
   * If a second, vtkPolyData input is provided, this second input specifies
   * a list of faces to be excluded from the output (in the
   * vtkPolyData::Polys attribute). This is useful to prevent the same face
   * to be output multiple times in complex pipelines. (A candidate output
   * boundary face is the same as a face in the excluded face list if it uses
   * the same point ids as one of the polygons defined in the second input.) For
   * example, a face may be extracted separately via a threshold filter; thus
   * this face should not be also extracted via the vtkGeometryFilter. (This
   * functionality is related to vtkRemovePolyData.)
   */
  void SetExcludedFacesData(vtkPolyData*);
  void SetExcludedFacesConnection(vtkAlgorithmOutput* algOutput);
  vtkPolyData* GetExcludedFaces();
  ///@}

  ///@{
  /**
   * If the input is an unstructured grid with nonlinear faces, this parameter
   * determines how many times the face is subdivided into linear faces.  If 0,
   * the output is the equivalent of its linear counterpart (and the midpoints
   * determining the nonlinear interpolation are discarded).  If 1 (the
   * default), the nonlinear face is triangulated based on the midpoints.  If
   * greater than 1, the triangulated pieces are recursively subdivided to reach
   * the desired subdivision.  Setting the value to greater than 1 may cause
   * some point data to not be passed even if no nonlinear faces exist.  This
   * option has no effect if the input is not an unstructured grid.
   */
  vtkSetMacro(NonlinearSubdivisionLevel, int);
  vtkGetMacro(NonlinearSubdivisionLevel, int);
  ///@}

  ///@{
  /**
   * When two volumetric cells of different order are connected by their corners (for instance, a
   * quadratic hexahedron next to a linear hexahedron ), the internal face is rendered and is not
   * considered as a ghost cell. To remove these faces, switch MatchBoundariesIgnoringCellOrder to 1
   * (default is 0).
   */
  vtkSetMacro(MatchBoundariesIgnoringCellOrder, int);
  vtkGetMacro(MatchBoundariesIgnoringCellOrder, int);
  ///@}

  ///@{
  /**
   * Disable delegation to an internal vtkDataSetSurfaceFilter.
   */
  vtkSetMacro(Delegation, vtkTypeBool);
  vtkGetMacro(Delegation, vtkTypeBool);
  vtkBooleanMacro(Delegation, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get if Ghost interfaces will be removed.
   * When you are rendering you want to remove ghost interfaces that originate from duplicate cells.
   *
   * There are certain algorithms though that need the ghost interfaces, such as GhostCellGenerator
   * and FeatureEdges.
   *
   * Since Rendering is the most common case, the Default is on.
   *
   * Note: DON'T change it if there are no ghost cells.
   */
  vtkSetMacro(RemoveGhostInterfaces, bool);
  vtkBooleanMacro(RemoveGhostInterfaces, bool);
  vtkGetMacro(RemoveGhostInterfaces, bool);
  ///@}

  ///@{
  /**
   * Direct access methods so that this class can be used as an
   * algorithm without using it as a filter (i.e., no pipeline updates).
   * Also some internal methods with additional options.
   */
  int PolyDataExecute(vtkDataSet* input, vtkPolyData* output, vtkPolyData* exc);
  virtual int PolyDataExecute(vtkDataSet*, vtkPolyData*);

  int UnstructuredGridExecute(
    vtkDataSet* input, vtkPolyData* output, vtkGeometryFilterHelper* info, vtkPolyData* exc);
  virtual int UnstructuredGridExecute(vtkDataSet* input, vtkPolyData* output);

  int StructuredExecute(vtkDataSet* input, vtkPolyData* output, int* wholeExtent, vtkPolyData* exc,
    bool* extractFace = nullptr);
  virtual int StructuredExecute(
    vtkDataSet* input, vtkPolyData* output, int* wholeExt, bool* extractFace = nullptr);

  int DataSetExecute(vtkDataSet* input, vtkPolyData* output, vtkPolyData* exc);
  virtual int DataSetExecute(vtkDataSet* input, vtkPolyData* output);
  ///@}

protected:
  vtkGeometryFilter();
  ~vtkGeometryFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // special cases for performance
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkIdType PointMaximum;
  vtkIdType PointMinimum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  bool PointClipping;
  bool CellClipping;
  bool ExtentClipping;
  int OutputPointsPrecision;
  bool RemoveGhostInterfaces;

  bool Merging;
  vtkIncrementalPointLocator* Locator;

  bool FastMode;

  // These methods support compatibility with vtkDataSetSurfaceFilter
  int PieceInvariant;
  vtkTypeBool PassThroughCellIds;
  char* OriginalCellIdsName;

  vtkTypeBool PassThroughPointIds;
  char* OriginalPointIdsName;

  int NonlinearSubdivisionLevel;
  int MatchBoundariesIgnoringCellOrder;

  vtkTypeBool Delegation;

private:
  vtkGeometryFilter(const vtkGeometryFilter&) = delete;
  void operator=(const vtkGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
