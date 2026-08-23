// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSurfaceRepresentation
 * @brief   Renders any dataset as a surface.
 *
 * vtkSurfaceRepresentation is a data representation that renders any dataset as
 * a surface.  Non-polygonal inputs are converted by vtkGeometryFilterDispatcher,
 * which handles vtkDataSet, vtkHyperTreeGrid, vtkCellGrid, vtkGenericDataSet and
 * composite datasets, and selections made in a view are extracted and drawn
 * highlighted over the surface.
 *
 * The API here covers what it takes to make data visible -- whether it is drawn,
 * how, and how opaque -- and legible -- what color, and colored by which array.
 * Everything else is a property of the objects underneath, which are reachable
 * through GetProperty(), GetMapper(), GetGeometryFilter() and GetActor(): line
 * width, point size, lighting and the physically based properties on the
 * property; normal generation, feature angle, triangulation, subdivision and the
 * AMR and composite options on the geometry filter.
 *
 * @par Internal pipeline:
 * @verbatim
 * Input (any dataset)
 *   -> vtkGeometryFilterDispatcher (extract outer surface)
 *     -> vtkCompositePolyDataMapper
 *       -> vtkActor
 * @endverbatim
 *
 * @sa vtkScivisDataRepresentation vtkScivisView vtkVolumeRepresentation
 * vtkGeometryFilterDispatcher
 */

#ifndef vtkSurfaceRepresentation_h
#define vtkSurfaceRepresentation_h

#include "vtkNew.h" // For ivars
#include "vtkScivisDataRepresentation.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBlockProperties;
class vtkCompositePolyDataMapper;
class vtkDataArray;
class vtkExtractSelection;
class vtkGeometryFilterDispatcher;
class vtkProperty;
class vtkScalarsToColors;
class vtkScivisView;
class vtkSelection;

class VTKVIEWSSCIVIS_EXPORT vtkSurfaceRepresentation : public vtkScivisDataRepresentation
{
public:
  static vtkSurfaceRepresentation* New();
  vtkTypeMacro(vtkSurfaceRepresentation, vtkScivisDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The representation stores its properties on the geometry filter, mapper and
   * actor it owns rather than in its own ivars.  Those objects are also
   * reachable through GetActor() and friends, so the modified time reported here
   * is the latest of this object's own and theirs.
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
   * The color the surface is drawn in when it is not colored by an array, and
   * how opaque it is.  EdgeColor is the color of the edges drawn by the
   * SURFACE_WITH_EDGES mode.
   *
   * For the rest of the appearance -- line width, point size, lighting,
   * ambient, diffuse, specular, roughness, metallic -- use GetProperty().
   */
  void SetColor(double r, double g, double b);
  double* GetColor() VTK_SIZEHINT(3);
  void SetOpacity(double val);
  double GetOpacity();
  void SetEdgeColor(double r, double g, double b);
  double* GetEdgeColor() VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * Color by an array rather than by a single color.
   *
   * ColorByPointArray and ColorByCellArray color by an array attached to the
   * points or the cells.  ColorByFieldArray colors by an array in the field
   * data, which carries no per-element association of its own: its tuples are
   * consumed one per cell.  ResetColorArray goes back to whatever the data's
   * active scalars are.
   *
   * The overloads taking a component select which component of a
   * multi-component array to map; the others map the array as the color map
   * sees fit, which for vectors is the magnitude.
   *
   * The range that scalars are mapped through belongs to the color map, not to
   * the representation: set it with GetColorMap()->SetRange(), or on a map of
   * your own before handing it over.  A map shared between representations
   * therefore keeps one range, and the representation never writes over the
   * range you gave it.
   */
  void SetScalarVisibility(bool val);
  bool GetScalarVisibility();
  void ColorByPointArray(const char* arrayName);
  void ColorByPointArray(const char* arrayName, int component);
  void ColorByCellArray(const char* arrayName);
  void ColorByCellArray(const char* arrayName, int component);
  void ColorByFieldArray(const char* arrayName);
  void ColorByFieldArray(const char* arrayName, int component);
  void ResetColorArray();
  ///@}

  ///@{
  /**
   * How the values of the color array become colors.
   *
   * The default maps them through the color map.  DIRECT_SCALARS instead takes
   * an unsigned char array of three or four components as the colors
   * themselves, which is how data that already carries its own colors is drawn.
   */
  enum ColorModeType
  {
    MAP_SCALARS = 0,
    DIRECT_SCALARS = 1
  };
  void SetColorMode(int mode);
  int GetColorMode();
  void SetColorModeToMapScalars() { this->SetColorMode(MAP_SCALARS); }
  void SetColorModeToDirectScalars() { this->SetColorMode(DIRECT_SCALARS); }
  ///@}

  ///@{
  /**
   * Which tuple of the array selected by ColorByFieldArray() to color with.
   * The default, -1, consumes the array one tuple per cell.  An index of 0 or
   * more colors the entire surface with the tuple at that index, which is how a
   * per-block or per-dataset value is drawn.  Has no effect while coloring by a
   * point or cell array.
   */
  void SetFieldDataTupleId(vtkIdType id);
  vtkIdType GetFieldDataTupleId();
  ///@}

  ///@{
  /**
   * Whether scalars are interpolated across a cell before being turned into
   * colors rather than after.  Interpolating first is more faithful on a coarse
   * mesh, at the cost of a texture lookup per fragment.  Default is off.
   */
  void SetInterpolateScalarsBeforeMapping(bool val);
  bool GetInterpolateScalarsBeforeMapping();
  ///@}

  /**
   * Per-block appearance for a composite input: whether a block is drawn, and
   * in what color and opacity, by flat block index.
   *
   * Those live in an object of their own because addressing a block by index
   * means walking the data to find it, which every one of them would otherwise
   * have to remember to do:
   *
   * @code
   * rep->GetBlocks()->SetVisibility(2, false);
   * @endcode
   */
  vtkBlockProperties* GetBlocks();

  ///@{
  /**
   * The vtkScivisDataRepresentation contract.  The rendered array is resolved from
   * the data, so it reports the active scalars when no array has been selected,
   * and answers for the first block of a composite input that has anything to
   * say.  The range is reported over the input rather than over the extracted
   * surface, so that it agrees with what a volume of the same data would say.
   */
  void SetVisibility(bool val) override;
  bool GetVisibility() override;
  bool GetBounds(double bounds[6]) override;
  const char* GetRenderedArrayName() override;
  int GetRenderedFieldAssociation() override;
  bool GetDataRange(
    const char* arrayName, int fieldAssoc, double range[2], int component = -1) override;
  void SetColorMap(vtkScalarsToColors* map) override;
  vtkScalarsToColors* GetColorMap() override;
  ///@}

  ///@{
  /**
   * How a selection made in a view is drawn over the surface.  The selection is
   * drawn from the same geometry as the representation itself, so only the
   * property-level values (POINTS, WIREFRAME, SURFACE, SURFACE_WITH_EDGES)
   * apply here; OUTLINE and FEATURE_EDGES are rejected with a warning.
   *
   * Until this is set, the style follows the selection itself: point selections
   * are drawn as points and cell selections as wireframe.  Calling this setter
   * pins the style to the requested value for every subsequent selection.  The
   * getter always reports the style in effect.
   *
   * For the color, opacity, line width and point size of the highlight, use
   * GetSelectionActor()->GetProperty().
   */
  void SetSelectionRepresentation(int type);
  int GetSelectionRepresentation();
  ///@}

  ///@{
  /**
   * The objects this representation is built from, for everything the API above
   * does not cover.
   *
   * Note that the representation holds the geometry filter to the pass-through
   * of original point and cell ids, which is what lets a selection made in a
   * view be mapped back to the input.  Turning that off breaks selection.
   */
  vtkActor* GetActor();
  vtkProperty* GetProperty();
  vtkGeometryFilterDispatcher* GetGeometryFilter();
  vtkActor* GetSelectionActor();
  ///@}

protected:
  vtkSurfaceRepresentation();
  ~vtkSurfaceRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AddToView(vtkScivisView* view) override;
  bool RemoveFromView(vtkScivisView* view) override;

  vtkSelection* ConvertSelection(vtkScivisView* view, vtkSelection* selection) override;

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

  /**
   * The array being mapped to colors, and the attributes it comes from, as the
   * mapper resolves them from the data.
   */
  vtkDataArray* GetRenderedScalars(int& fieldAssoc);

  vtkNew<vtkGeometryFilterDispatcher> GeometryFilter;
  vtkNew<vtkCompositePolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;
  vtkNew<vtkBlockProperties> Blocks;
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
