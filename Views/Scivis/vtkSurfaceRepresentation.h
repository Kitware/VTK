// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSurfaceRepresentation
 * @brief   Renders any dataset as a surface with a flat property API.
 *
 * vtkSurfaceRepresentation is a data representation that renders any
 * dataset as a surface.  Non-polygonal inputs are automatically
 * converted via vtkGeometryFilterDispatcher, which handles a wide range
 * of data types including vtkDataSet, vtkHyperTreeGrid, vtkCellGrid,
 * vtkGenericDataSet, and composite datasets.  All common display
 * properties (color, opacity, representation mode, scalar coloring,
 * scalar bar) are exposed through a flat API so that callers never need
 * to reach into mapper, actor, or property objects directly.
 *
 * @par Internal pipeline:
 * @verbatim
 * Input (any dataset)
 *   -> vtkGeometryFilterDispatcher (extract outer surface)
 *     -> vtkCompositePolyDataMapper
 *       -> vtkActor
 * @endverbatim
 *
 * @sa vtkDataRepresentation vtkStandardRenderView vtkVolumeRepresentation
 * vtkGeometryFilterDispatcher
 */

#ifndef vtkSurfaceRepresentation_h
#define vtkSurfaceRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkNew.h"               // For ivars
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCompositePolyDataMapper;
class vtkExtractSelection;
class vtkGeometryFilterDispatcher;
class vtkProperty;
class vtkScalarBarActor;
class vtkScalarsToColors;
class vtkSelection;

class VTKVIEWSSCIVIS_EXPORT vtkSurfaceRepresentation : public vtkDataRepresentation
{
public:
  static vtkSurfaceRepresentation* New();
  vtkTypeMacro(vtkSurfaceRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The representation stores its properties on the geometry filter, mapper,
   * actor and scalar bar it owns rather than in its own ivars.  Those objects
   * are also reachable through GetActor(), GetScalarBarActor() and friends, so
   * the modified time reported here is the latest of this object's own and
   * theirs.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Representation type constants.
   *
   * POINTS, WIREFRAME, SURFACE and SURFACE_WITH_EDGES draw the extracted
   * surface and differ only in how the actor's property renders it.  OUTLINE
   * and FEATURE_EDGES change what geometry is extracted in the first place:
   * OUTLINE produces the bounding box of the input and FEATURE_EDGES produces
   * the edges whose adjacent faces meet at more than the feature angle.
   */
  enum RepresentationType
  {
    POINTS = 0,
    WIREFRAME = 1,
    SURFACE = 2,
    SURFACE_WITH_EDGES = 3,
    OUTLINE = 4,
    FEATURE_EDGES = 5
  };

  ///@{
  /**
   * Set/get the representation mode.  Use the RepresentationType enum or the
   * SetRepresentationTo* convenience methods.  The Python wrapping also accepts
   * the matching string names (case-insensitive), e.g. "Wireframe" or
   * "SurfaceWithEdges".
   *
   * The modes are mutually exclusive: there is no way to ask for an outline and
   * feature edges at the same time.  The default is SURFACE.
   */
  void SetRepresentation(int type);
  int GetRepresentation();
  void SetRepresentationToPoints() { this->SetRepresentation(POINTS); }
  void SetRepresentationToWireframe() { this->SetRepresentation(WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(SURFACE); }
  void SetRepresentationToSurfaceWithEdges() { this->SetRepresentation(SURFACE_WITH_EDGES); }
  void SetRepresentationToOutline() { this->SetRepresentation(OUTLINE); }
  void SetRepresentationToFeatureEdges() { this->SetRepresentation(FEATURE_EDGES); }
  const char* GetRepresentationAsString();
  ///@}

  ///@{
  /**
   * Color and material properties (forwarded to vtkProperty).
   */
  void SetColor(double r, double g, double b);
  double* GetColor() VTK_SIZEHINT(3);
  void SetOpacity(double val);
  double GetOpacity();
  void SetEdgeColor(double r, double g, double b);
  double* GetEdgeColor() VTK_SIZEHINT(3);
  void SetEdgeOpacity(double val);
  double GetEdgeOpacity();
  void SetAmbient(double val);
  double GetAmbient();
  void SetDiffuse(double val);
  double GetDiffuse();
  void SetSpecular(double val);
  double GetSpecular();
  void SetSpecularPower(double val);
  double GetSpecularPower();
  void SetLineWidth(double val);
  double GetLineWidth();
  void SetPointSize(double val);
  double GetPointSize();
  void SetLighting(bool val);
  bool GetLighting();
  void SetInterpolation(int val);
  int GetInterpolation();
  void SetRenderPointsAsSpheres(bool val);
  bool GetRenderPointsAsSpheres();
  void SetRenderLinesAsTubes(bool val);
  bool GetRenderLinesAsTubes();
  void SetRoughness(double val);
  double GetRoughness();
  void SetMetallic(double val);
  double GetMetallic();
  ///@}

  ///@{
  /**
   * Scalar coloring (forwarded to mapper).
   *
   * ColorByPointArray and ColorByCellArray color by an array attached to the
   * points or the cells.  ColorByFieldArray colors by an array in the field
   * data, which carries no per-element association of its own: by default its
   * tuples are consumed one per cell, and SetFieldDataTupleId() switches to
   * coloring the whole surface with a single tuple.
   *
   * The overloads taking a component select which component of a
   * multi-component array to map; the others map the array as the lookup table
   * sees fit, which for vectors is the magnitude.
   *
   * The range that scalars are mapped through belongs to the lookup table, not
   * to the representation: set it with GetLookupTable()->SetRange(), or on a
   * table of your own before handing it over.  A table shared between
   * representations therefore keeps one range, and the representation never
   * writes over the range you gave it.
   */
  void SetScalarVisibility(bool val);
  bool GetScalarVisibility();
  void ColorByPointArray(const char* arrayName);
  void ColorByPointArray(const char* arrayName, int component);
  void ColorByCellArray(const char* arrayName);
  void ColorByCellArray(const char* arrayName, int component);
  void ColorByFieldArray(const char* arrayName);
  void ColorByFieldArray(const char* arrayName, int component);
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkScalarsToColors* GetLookupTable();
  void SetInterpolateScalarsBeforeMapping(bool val);
  bool GetInterpolateScalarsBeforeMapping();
  ///@}

  ///@{
  /**
   * Which tuple of the array selected by ColorByFieldArray() to color with.
   * The default, -1, consumes the array one tuple per cell.  An index of 0 or
   * more colors the entire surface with the tuple at that index, which is how
   * a per-block or per-dataset value is drawn.  Has no effect when coloring by
   * a point or cell array.
   */
  void SetFieldDataTupleId(vtkIdType id);
  vtkIdType GetFieldDataTupleId();
  ///@}

  ///@{
  /**
   * Normal generation (forwarded to vtkGeometryFilterDispatcher).
   * GeneratePointNormals enables smooth-shading normals (default off).
   * GenerateCellNormals enables flat-shading normals (default off).
   * FeatureAngle is the angle (degrees) that defines a sharp edge for
   * normal splitting, and the angle used to select the edges extracted by
   * the FEATURE_EDGES representation (default 30).
   * Splitting controls whether sharp edges cause point duplication so
   * that normals are discontinuous across them (default on).
   */
  void SetGeneratePointNormals(bool val);
  bool GetGeneratePointNormals();
  void SetGenerateCellNormals(bool val);
  bool GetGenerateCellNormals();
  void SetFeatureAngle(double val);
  double GetFeatureAngle();
  void SetSplitting(bool val);
  bool GetSplitting();
  ///@}

  ///@{
  /**
   * Mesh processing options (forwarded to vtkGeometryFilterDispatcher).
   * Triangulate forces triangulation of the output (default off).
   * NonlinearSubdivisionLevel controls subdivision of nonlinear cells
   * for better approximation (default 1).
   * MatchBoundariesIgnoringCellOrder removes internal faces between
   * volumetric cells of different order (default off).
   */
  void SetTriangulate(bool val);
  bool GetTriangulate();
  void SetNonlinearSubdivisionLevel(int val);
  int GetNonlinearSubdivisionLevel();
  void SetMatchBoundariesIgnoringCellOrder(bool val);
  bool GetMatchBoundariesIgnoringCellOrder();
  ///@}

  ///@{
  /**
   * Picking support (forwarded to vtkGeometryFilterDispatcher).
   * When on, the output contains arrays mapping surface cells/points
   * back to original input cell/point ids (default on).
   */
  void SetPassThroughCellIds(bool val);
  bool GetPassThroughCellIds();
  void SetPassThroughPointIds(bool val);
  bool GetPassThroughPointIds();
  ///@}

  ///@{
  /**
   * Composite and AMR options (forwarded to vtkGeometryFilterDispatcher).
   * BlockColorsDistinctValues sets the number of distinct values used
   * in the vtkBlockColors array for composite datasets (default 7).
   * HideInternalAMRFaces hides internal faces within AMR grids (default on).
   * UseNonOverlappingAMRMetaDataForOutlines uses AMR metadata to generate
   * outlines even for blocks without heavy data (default on).
   */
  void SetBlockColorsDistinctValues(int val);
  int GetBlockColorsDistinctValues();
  void SetHideInternalAMRFaces(bool val);
  bool GetHideInternalAMRFaces();
  void SetUseNonOverlappingAMRMetaDataForOutlines(bool val);
  bool GetUseNonOverlappingAMRMetaDataForOutlines();
  ///@}

  ///@{
  /**
   * Parallel option (forwarded to vtkGeometryFilterDispatcher).
   * When on, point and cell arrays named vtkProcessId are added to
   * identify which MPI rank produced each element (default auto).
   */
  void SetGenerateProcessIds(bool val);
  bool GetGenerateProcessIds();
  ///@}

  ///@{
  /**
   * Visibility, pickability, and actor transforms.
   */
  void SetVisibility(bool val);
  bool GetVisibility();
  void SetPickable(bool val);
  bool GetPickable();
  void SetPosition(double x, double y, double z);
  double* GetPosition() VTK_SIZEHINT(3);
  void SetOrientation(double x, double y, double z);
  double* GetOrientation() VTK_SIZEHINT(3);
  void SetScale(double x, double y, double z);
  double* GetScale() VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * Scalar bar control.
   */
  void SetScalarBarVisibility(bool val);
  bool GetScalarBarVisibility();
  vtkScalarBarActor* GetScalarBarActor();
  ///@}

  ///@{
  /**
   * Selection display properties.
   * These control the appearance of highlighted cells or points
   * when a selection is active.  Defaults: red color, wireframe,
   * line width 2, opacity 1.
   */
  void SetSelectionColor(double r, double g, double b);
  double* GetSelectionColor() VTK_SIZEHINT(3);
  void SetSelectionOpacity(double val);
  double GetSelectionOpacity();
  void SetSelectionLineWidth(double val);
  double GetSelectionLineWidth();
  void SetSelectionPointSize(double val);
  double GetSelectionPointSize();
  /**
   * The selection is drawn from the same geometry as the representation
   * itself, so only the property-level RepresentationType values (POINTS,
   * WIREFRAME, SURFACE, SURFACE_WITH_EDGES) apply here.  OUTLINE and
   * FEATURE_EDGES are rejected with a warning.
   *
   * Until this is set, the selection style follows the selection itself:
   * point selections are drawn as points and cell selections as wireframe.
   * Calling this setter pins the style to the requested value for every
   * subsequent selection.  The getter always reports the style in effect.
   */
  void SetSelectionRepresentation(int type);
  int GetSelectionRepresentation();
  ///@}

  /**
   * Provide access to the internal selection actor for advanced usage.
   */
  vtkActor* GetSelectionActor();

  /**
   * Provide access to the internal actor for advanced usage.
   */
  vtkActor* GetActor();

protected:
  vtkSurfaceRepresentation();
  ~vtkSurfaceRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

  vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection) override;

private:
  vtkSurfaceRepresentation(const vtkSurfaceRepresentation&) = delete;
  void operator=(const vtkSurfaceRepresentation&) = delete;

  /**
   * Return true when the mapper already colors by `arrayName` in `scalarMode`,
   * so the ColorBy*Array() methods can skip a redundant modification.
   */
  bool IsColoringBy(const char* arrayName, int scalarMode);

  /**
   * Map the given component of the active color array, used by the
   * ColorBy*Array() overloads that take a component.
   */
  void ColorByComponent(int component);

  vtkNew<vtkGeometryFilterDispatcher> GeometryFilter;
  vtkNew<vtkCompositePolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;
  vtkNew<vtkScalarBarActor> ScalarBar;
  bool ScalarBarVisible;
  int RepresentationValue;

  vtkNew<vtkExtractSelection> SelectionExtractor;
  vtkNew<vtkGeometryFilterDispatcher> SelectionGeometryFilter;
  vtkNew<vtkCompositePolyDataMapper> SelectionMapper;
  vtkNew<vtkActor> SelectionActor;
  int SelectionRepresentationValue;
  bool UserSetSelectionRepresentation;
};

VTK_ABI_NAMESPACE_END
#endif
