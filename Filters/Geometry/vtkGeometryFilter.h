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
 * In some cases (especially for large unstructured grids) the
 * vtkGeometryFilter can be slow. Consequently the filter has an optional
 * "fast mode" that may execute significantly faster (>4-5x) than normal
 * execution. The fast mode visits a subset of cells that may be on the
 * boundary of the dataset (and skips interior cells which contribute nothing
 * to the output). The set of subsetted cells is determined by inspecting
 * the topological connectivity degree of each point (i.e., the number of
 * unique cells using a particular point is that point's degree). With fast
 * mode enabled, those cells connected to a point with degree <= Degree
 * are visited. Note that this approach may miss some cells which contribute
 * boundary faces--thus the output is an approximation to the normal
 * execution of vtkGeometryFilter.
 *
 * Finally, this filter takes an optional second, vtkPolyData input. This
 * input represents a list of faces that are to be excluded from the output
 * of vtkGeometryFilter.
 *
 * @warning
 * While vtkGeometryFilter and vtkDataSetSurfaceFilter perform similar operations,
 * there are important differences as follows:
 * 1. vtkGeometryFilter preserves topological connectivity. vtkDataSetSurfaceFilter
 *    produces output primitives which may be disconnected from one another.
 * 2. vtkGeometryFilter can generate output based on cell ids, point ids, and/or
 *    extent (bounding box) clipping. vtkDataSetSurfaceFilter strictly extracts
 *    the boundary surface of a dataset.
 * 3. vtkGeometryFilter is much faster than vtkDataSetSurfaceFilter, especially
 *    for vtkUnstructuredGrids. As a result, vtkDataSetSurfaceFilter will
 *    delegate the processing of linear unstructured grids to vtkGeometryFilter.
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

class vtkIncrementalPointLocator;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkGeometryFilter;
class vtkDataSetSurfaceFilter;
struct vtkGeometryFilterHelper;
struct vtkExcludedFaces;

// Used to coordinate delegation to vtkDataSetSurfaceFilter
struct VTKFILTERSGEOMETRY_EXPORT vtkGeometryFilterHelper
{
  unsigned char IsLinear;
  static vtkGeometryFilterHelper* CharacterizeUnstructuredGrid(vtkUnstructuredGrid*);
  static void CopyFilterParams(vtkGeometryFilter* gf, vtkDataSetSurfaceFilter* dssf);
  static void CopyFilterParams(vtkDataSetSurfaceFilter* dssf, vtkGeometryFilter* gf);
};

class VTKFILTERSGEOMETRY_EXPORT vtkGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkGeometryFilter* New();
  vtkTypeMacro(vtkGeometryFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping, bool);
  vtkGetMacro(PointClipping, bool);
  vtkBooleanMacro(PointClipping, bool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping, bool);
  vtkGetMacro(CellClipping, bool);
  vtkBooleanMacro(CellClipping, bool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping, bool);
  vtkGetMacro(ExtentClipping, bool);
  vtkBooleanMacro(ExtentClipping, bool);
  //@}

  //@{
  /**
   * Specify the minimum point id for point id selection.
   */
  vtkSetClampMacro(PointMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMinimum, vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum point id for point id selection.
   */
  vtkSetClampMacro(PointMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMaximum, vtkIdType);
  //@}

  //@{
  /**
   * Specify the minimum cell id for point id selection.
   */
  vtkSetClampMacro(CellMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMinimum, vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum cell id for point id selection.
   */
  vtkSetClampMacro(CellMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMaximum, vtkIdType);
  //@}

  /**
   * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);

  //@{
  /**
   * Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double extent[6]);
  double* GetExtent() VTK_SIZEHINT(6) { return this->Extent; }
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the
   * documentation for the vtkAlgorithm::DesiredOutputPrecision enum for an
   * explanation of the available precision settings. This only applies for
   * data types where we create points (merging) as opposed to passing them
   * from input to output, such as unstructured grids.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

  //@{
  /**
   * Turn on/off fast mode execution. If enabled, fast mode typically runs
   * much faster (2-3x) than the standard algorithm, however the output is an
   * approximation to the correct result. Also, note that the FastMode
   * depends on the data member Degree for its execution.
   */
  vtkSetMacro(FastMode, bool);
  vtkGetMacro(FastMode, bool);
  vtkBooleanMacro(FastMode, bool);
  //@}

  //@{
  /**
   * If fast mode is enabled, then Degree controls which cells are
   * visited. Basically, any cell connected to a point with connectivity
   * degree <= is visited and processed. Low degree points tend to be
   * located on the boundary of datasets - thus attached cells frequently
   * produce output boundary fragments.
   */
  vtkSetClampMacro(Degree, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(Degree, unsigned int);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default an instance
   * of vtkMergePoints is used. (This method is now deprecated and has no
   * effect.)
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   * This method is now deprecated.
   */
  void CreateDefaultLocator();

  // The following are methods compatible with vtkDataSetSurfaceFilter.

  //@{
  /**
   * If PieceInvariant is true, vtkDataSetSurfaceFilter requests
   * 1 ghost level from input in order to remove internal surface
   * that are between processes. False by default.
   */
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
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
  vtkSetMacro(PassThroughCellIds, vtkTypeBool);
  vtkGetMacro(PassThroughCellIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughCellIds, vtkTypeBool);
  vtkSetMacro(PassThroughPointIds, vtkTypeBool);
  vtkGetMacro(PassThroughPointIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughPointIds, vtkTypeBool);
  //@}

  //@{
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
  //@}

  //@{
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
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Disable delegation to an internal vtkDataSetSurfaceFilter.
   */
  vtkSetMacro(Delegation, vtkTypeBool);
  vtkGetMacro(Delegation, vtkTypeBool);
  vtkBooleanMacro(Delegation, vtkTypeBool);
  //@}

  //@{
  /**
   * Direct access methods so that this class can be used as an
   * algorithm without using it as a filter (i.e., no pipeline updates).
   * Also some internal methods with additional options.
   */
  int PolyDataExecute(vtkDataSet* input, vtkPolyData* output, vtkExcludedFaces* exc);
  virtual int PolyDataExecute(vtkDataSet*, vtkPolyData*);

  int UnstructuredGridExecute(
    vtkDataSet* input, vtkPolyData* output, vtkGeometryFilterHelper* info, vtkExcludedFaces* exc);
  virtual int UnstructuredGridExecute(vtkDataSet* input, vtkPolyData* output);

  int StructuredExecute(
    vtkDataSet* input, vtkPolyData* output, vtkInformation* inInfo, vtkExcludedFaces* exc);
  virtual int StructuredExecute(vtkDataSet* input, vtkPolyData* output, vtkInformation* inInfo);

  int DataSetExecute(vtkDataSet* input, vtkPolyData* output, vtkExcludedFaces* exc);
  virtual int DataSetExecute(vtkDataSet* input, vtkPolyData* output);
  //@}

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

  bool Merging;
  vtkIncrementalPointLocator* Locator;

  bool FastMode;
  unsigned int Degree;

  // This methods support compatability with vtkDataSetSurfaceFilter
  int PieceInvariant;
  vtkTypeBool PassThroughCellIds;
  char* OriginalCellIdsName;

  vtkTypeBool PassThroughPointIds;
  char* OriginalPointIdsName;

  int NonlinearSubdivisionLevel;

  vtkTypeBool Delegation;

private:
  vtkGeometryFilter(const vtkGeometryFilter&) = delete;
  void operator=(const vtkGeometryFilter&) = delete;
};

#endif
