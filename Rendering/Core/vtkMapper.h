/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMapper
 * @brief   abstract class specifies interface to map data to graphics primitives
 *
 * vtkMapper is an abstract class to specify interface between data and
 * graphics primitives. Subclasses of vtkMapper map data through a
 * lookuptable and control the creation of rendering primitives that
 * interface to the graphics library. The mapping can be controlled by
 * supplying a lookup table and specifying a scalar range to map data
 * through.
 *
 * There are several important control mechanisms affecting the behavior of
 * this object. The ScalarVisibility flag controls whether scalar data (if
 * any) controls the color of the associated actor(s) that refer to the
 * mapper. The ScalarMode ivar is used to determine whether scalar point data
 * or cell data is used to color the object. By default, point data scalars
 * are used unless there are none, in which cell scalars are used. Or you can
 * explicitly control whether to use point or cell scalar data. Finally, the
 * mapping of scalars through the lookup table varies depending on the
 * setting of the ColorMode flag. See the documentation for the appropriate
 * methods for an explanation.
 *
 * Another important feature of the mapper is the ability to shift the
 * z-buffer to resolve coincident topology. For example, if you'd like to
 * draw a mesh with some edges a different color, and the edges lie on the
 * mesh, this feature can be useful to get nice looking lines. (See the
 * ResolveCoincidentTopology-related methods.)
 *
 * @sa
 * vtkDataSetMapper vtkPolyDataMapper
*/

#ifndef vtkMapper_h
#define vtkMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractMapper3D.h"
#include "vtkSystemIncludes.h" // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include <vector> // for method args

#define VTK_RESOLVE_OFF 0
#define VTK_RESOLVE_POLYGON_OFFSET 1
#define VTK_RESOLVE_SHIFT_ZBUFFER 2

#define VTK_GET_ARRAY_BY_ID 0
#define VTK_GET_ARRAY_BY_NAME 1

#define VTK_MATERIALMODE_DEFAULT  0
#define VTK_MATERIALMODE_AMBIENT  1
#define VTK_MATERIALMODE_DIFFUSE  2
#define VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE  3

class vtkActor;
class vtkDataSet;
class vtkDataObject;
class vtkFloatArray;
class vtkHardwareSelector;
class vtkImageData;
class vtkProp;
class vtkRenderer;
class vtkScalarsToColors;
class vtkUnsignedCharArray;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT vtkMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkMapper, vtkAbstractMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper *m) override;

  /**
   * Overload standard modified time function. If lookup table is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Method initiates the mapping process. Generally sent by the actor
   * as each frame is rendered.
   */
  virtual void Render(vtkRenderer *ren, vtkActor *a) = 0;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override {}

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();
  //@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility, vtkTypeBool);
  vtkGetMacro(ScalarVisibility, vtkTypeBool);
  vtkBooleanMacro(ScalarVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off flag to control whether the mapper's data is static. Static data
   * means that the mapper does not propagate updates down the pipeline, greatly
   * decreasing the time it takes to update many mappers. This should only be
   * used if the data never changes.
   */
  vtkSetMacro(Static, vtkTypeBool);
  vtkGetMacro(Static, vtkTypeBool);
  vtkBooleanMacro(Static, vtkTypeBool);
  //@}

  //@{
  /**
   * default (ColorModeToDefault), unsigned char scalars are treated
   * as colors, and NOT mapped through the lookup table, while
   * everything else is.  ColorModeToDirectScalar extends
   * ColorModeToDefault such that all integer types are treated as
   * colors with values in the range 0-255 and floating types are
   * treated as colors with values in the range 0.0-1.0.  Setting
   * ColorModeToMapScalars means that all scalar data will be mapped
   * through the lookup table.  (Note that for multi-component
   * scalars, the particular component to use for mapping can be
   * specified using the SelectColorArray() method.)
   */
  vtkSetMacro(ColorMode, int);
  vtkGetMacro(ColorMode, int);
  void SetColorModeToDefault()
    { this->SetColorMode(VTK_COLOR_MODE_DEFAULT); }
  void SetColorModeToMapScalars()
    { this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS); }
  void SetColorModeToDirectScalars()
  { this->SetColorMode(VTK_COLOR_MODE_DIRECT_SCALARS); }
  //@}

  /**
   * Return the method of coloring scalar data.
   */
  const char *GetColorModeAsString();

  //@{
  /**
   * By default, vertex color is used to map colors to a surface.
   * Colors are interpolated after being mapped.
   * This option avoids color interpolation by using a one dimensional
   * texture map for the colors.
   */
  vtkSetMacro(InterpolateScalarsBeforeMapping, vtkTypeBool);
  vtkGetMacro(InterpolateScalarsBeforeMapping, vtkTypeBool);
  vtkBooleanMacro(InterpolateScalarsBeforeMapping, vtkTypeBool);
  //@}

  //@{
  /**
   * Control whether the mapper sets the lookuptable range based on its
   * own ScalarRange, or whether it will use the LookupTable ScalarRange
   * regardless of it's own setting. By default the Mapper is allowed to set
   * the LookupTable range, but users who are sharing LookupTables between
   * mappers/actors will probably wish to force the mapper to use the
   * LookupTable unchanged.
   */
  vtkSetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkGetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkBooleanMacro(UseLookupTableScalarRange, vtkTypeBool);
  //@}

  //@{
  /**
   * Specify range in terms of scalar minimum and maximum (smin,smax). These
   * values are used to map scalars into lookup table. Has no effect when
   * UseLookupTableScalarRange is true.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVectorMacro(ScalarRange, double, 2);
  //@}

  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (ScalarModeToDefault), the filter will use point data,
   * and if no point data is available, then cell data is used. Alternatively
   * you can explicitly set the filter to use point data
   * (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
   * You can also choose to get the scalars from an array in point field
   * data (ScalarModeToUsePointFieldData) or cell field data
   * (ScalarModeToUseCellFieldData).  If scalars are coming from a field
   * data array, you must call SelectColorArray before you call
   * GetColors.
   */

  // When ScalarMode is set to use Field Data (ScalarModeToFieldData),
  // you must call SelectColorArray to choose the field data array to
  // be used to color cells. In this mode, the default behavior is to
  // treat the field data tuples as being associated with cells. If
  // the poly data contains triangle strips, the array is expected to
  // contain the cell data for each mini-cell formed by any triangle
  // strips in the poly data as opposed to treating them as a single
  // tuple that applies to the entire strip.  This mode can also be
  // used to color the entire poly data by a single color obtained by
  // mapping the tuple at a given index in the field data array
  // through the color map. Use SetFieldDataTupleId() to specify
  // the tuple index.
  vtkSetMacro(ScalarMode, int);
  vtkGetMacro(ScalarMode, int);
  void SetScalarModeToDefault()
    { this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT); }
  void SetScalarModeToUsePointData()
    { this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA); }
  void SetScalarModeToUseCellData()
    { this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA); }
  void SetScalarModeToUsePointFieldData()
    { this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); }
  void SetScalarModeToUseCellFieldData()
    { this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA); }
  void SetScalarModeToUseFieldData()
    { this->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA); }

  //@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which array to use for coloring using these methods.
   * The lookup table will decide how to convert vectors to colors.
   */
  void SelectColorArray(int arrayNum);
  void SelectColorArray(const char* arrayName);
  //@}


  // When ScalarMode is set to UseFieldData, set the index of the
  // tuple by which to color the entire data set. By default, the
  // index is -1, which means to treat the field data array selected
  // with SelectColorArray as having a scalar value for each cell.
  // Indices of 0 or higher mean to use the tuple at the given index
  // for coloring the entire data set.
  vtkSetMacro(FieldDataTupleId, vtkIdType);
  vtkGetMacro(FieldDataTupleId, vtkIdType);

  //@{
  /**
   * Legacy:
   * These methods used to be used to specify the array component.
   * It is better to do this in the lookup table.
   */
  void ColorByArrayComponent(int arrayNum, int component);
  void ColorByArrayComponent(const char* arrayName, int component);
  //@}

  /**
   * Set/Get the array name or number and component to color by.
   */
  vtkGetStringMacro(ArrayName);
  vtkSetStringMacro(ArrayName);
  vtkGetMacro(ArrayId, int);
  vtkSetMacro(ArrayId, int);
  vtkGetMacro(ArrayAccessMode, int);
  vtkSetMacro(ArrayAccessMode, int);
  vtkGetMacro(ArrayComponent, int);
  vtkSetMacro(ArrayComponent, int);

  /**
   * Return the method for obtaining scalar data.
   */
  const char *GetScalarModeAsString();

  //@{
  /**
   * Set/Get a global flag that controls whether coincident topology (e.g., a
   * line on top of a polygon) is shifted to avoid z-buffer resolution (and
   * hence rendering problems). If not off, there are two methods to choose
   * from. PolygonOffset uses graphics systems calls to shift polygons, lines
   * and points from each other. ShiftZBuffer is a legacy method that used to
   * remap the z-buffer to distinguish vertices, lines, and polygons, but
   * does not always produce acceptable results. You should only use the
   * PolygonOffset method (or none) at this point.
   */
  static void SetResolveCoincidentTopology(int val);
  static int  GetResolveCoincidentTopology();
  static void SetResolveCoincidentTopologyToDefault();
  static void SetResolveCoincidentTopologyToOff()
    { SetResolveCoincidentTopology(VTK_RESOLVE_OFF) ;}
  static void SetResolveCoincidentTopologyToPolygonOffset()
    { SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET); }
  static void SetResolveCoincidentTopologyToShiftZBuffer()
    { SetResolveCoincidentTopology(VTK_RESOLVE_SHIFT_ZBUFFER); }
  //@}

  //@{
  /**
   * Used to set the polygon offset scale factor and units.
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   * These are global variables.
   */
  static void SetResolveCoincidentTopologyPolygonOffsetParameters(
    double factor, double units);
  static void GetResolveCoincidentTopologyPolygonOffsetParameters(
    double& factor, double& units);
  //@}

  //@{
  /**
   * Used to set the polygon offset values relative to the global
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   */
  void SetRelativeCoincidentTopologyPolygonOffsetParameters(
    double factor, double units);
  void GetRelativeCoincidentTopologyPolygonOffsetParameters(
    double& factor, double& units);
  //@}

  //@{
  /**
   * Used to set the line offset scale factor and units.
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   * These are global variables.
   */
  static void SetResolveCoincidentTopologyLineOffsetParameters(
    double factor, double units);
  static void GetResolveCoincidentTopologyLineOffsetParameters(
    double& factor, double& units);
  //@}

  //@{
  /**
   * Used to set the line offset values relative to the global
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   */
  void SetRelativeCoincidentTopologyLineOffsetParameters(
    double factor, double units);
  void GetRelativeCoincidentTopologyLineOffsetParameters(
    double& factor, double& units);
  //@}

  //@{
  /**
   * Used to set the point offset value
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   * These are global variables.
   */
  static void SetResolveCoincidentTopologyPointOffsetParameter(
    double units);
  static void GetResolveCoincidentTopologyPointOffsetParameter(
    double& units);
  //@}

  //@{
  /**
   * Used to set the point offset value relative to the global
   * Used when ResolveCoincidentTopology is set to PolygonOffset.
   */
  void SetRelativeCoincidentTopologyPointOffsetParameter(double units);
  void GetRelativeCoincidentTopologyPointOffsetParameter(double& units);
  //@}

  //@{
  /**
   * Get the net parameters for handling coincident topology
   * obtained by summing the global values with the relative values.
   */
  void GetCoincidentTopologyPolygonOffsetParameters(
    double& factor, double& units);
  void GetCoincidentTopologyLineOffsetParameters(
    double& factor, double& units);
  void GetCoincidentTopologyPointOffsetParameter(double& units);
  //@}

  //@{
  /**
   * Used when ResolveCoincidentTopology is set to PolygonOffset. The polygon
   * offset can be applied either to the solid polygonal faces or the
   * lines/vertices. When set (default), the offset is applied to the faces
   * otherwise it is applied to lines and vertices.
   * This is a global variable.
   */
  static void SetResolveCoincidentTopologyPolygonOffsetFaces(int faces);
  static int GetResolveCoincidentTopologyPolygonOffsetFaces();
  //@}

  //@{
  /**
   * Used to set the z-shift if ResolveCoincidentTopology is set to
   * ShiftZBuffer. This is a global variable.
   */
  static void SetResolveCoincidentTopologyZShift(double val);
  static double GetResolveCoincidentTopologyZShift();
  //@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override
    { this->vtkAbstractMapper3D::GetBounds(bounds); }

  /**
   * This instance variable is used by vtkLODActor to determine which
   * mapper to use.  It is an estimate of the time necessary to render.
   * Setting the render time does not modify the mapper.
   */
  void SetRenderTime(double time) {this->RenderTime = time;}
  vtkGetMacro(RenderTime, double);

  /**
   * Get the input as a vtkDataSet.  This method is overridden in
   * the specialized mapper classes to return more specific data types.
   */
  vtkDataSet *GetInput();

  /**
   * Get the input to this mapper as a vtkDataSet, instead of as a
   * more specialized data type that the subclass may return from
   * GetInput().  This method is provided for use in the wrapper languages,
   * C++ programmers should use GetInput() instead.
   */
  vtkDataSet *GetInputAsDataSet()
    { return this->GetInput(); }

  //@{
  /**
   * Map the scalars (if there are any scalars and ScalarVisibility is on)
   * through the lookup table, returning an unsigned char RGBA array. This is
   * typically done as part of the rendering process. The alpha parameter
   * allows the blending of the scalars with an additional alpha (typically
   * which comes from a vtkActor, etc.)
   */
  virtual vtkUnsignedCharArray *MapScalars(double alpha);
  virtual vtkUnsignedCharArray *MapScalars(double alpha,
                                           int &cellFlag);
  virtual vtkUnsignedCharArray *MapScalars(vtkDataSet *input,
                                           double alpha);
  virtual vtkUnsignedCharArray *MapScalars(vtkDataSet *input,
                                           double alpha,
                                           int &cellFlag);
  //@}

  /**
   * Returns if the mapper does not expect to have translucent geometry. This
   * may happen when using ColorMode is set to not map scalars i.e. render the
   * scalar array directly as colors and the scalar array has opacity i.e. alpha
   * component.  Default implementation simply returns true. Note that even if
   * this method returns true, an actor may treat the geometry as translucent
   * since a constant translucency is set on the property, for example.
   */
  virtual bool GetIsOpaque();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  virtual bool GetSupportsSelection()
    { return false; }

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  virtual void ProcessSelectorPixelBuffers(vtkHardwareSelector * /* sel */,
    std::vector<unsigned int> & /* pixeloffsets */,
    vtkProp * /* prop */) { };

  /**
   * Returns if we can use texture maps for scalar coloring. Note this doesn't
   * say we "will" use scalar coloring. It says, if we do use scalar coloring,
   * we will use a texture.
   * When rendering multiblock datasets, if any 2 blocks provide different
   * lookup tables for the scalars, then also we cannot use textures. This case
   * can be handled if required.
   */
  virtual int CanUseTextureMapForColoring(vtkDataObject* input);

  /**
   * Call to force a rebuild of color result arrays on next MapScalars.
   * Necessary when using arrays in the case of multiblock data.
   */
  void ClearColorArrays();

  /**
   * Provide read access to the color array
   */
  vtkUnsignedCharArray *GetColorMapColors();

  /**
   * Provide read access to the color texture coordinate array
   */
  vtkFloatArray *GetColorCoordinates();

  /**
   * Provide read access to the color texture array
   */
  vtkImageData* GetColorTextureMap();

protected:
  vtkMapper();
  ~vtkMapper() override;

  // color mapped colors
  vtkUnsignedCharArray *Colors;

  // Use texture coordinates for coloring.
  vtkTypeBool InterpolateScalarsBeforeMapping;
  // Coordinate for each point.
  vtkFloatArray *ColorCoordinates;
  // 1D ColorMap used for the texture image.
  vtkImageData* ColorTextureMap;
  void MapScalarsToTexture(vtkAbstractArray* scalars, double alpha);

  vtkScalarsToColors *LookupTable;
  vtkTypeBool ScalarVisibility;
  vtkTimeStamp BuildTime;
  double ScalarRange[2];
  vtkTypeBool UseLookupTableScalarRange;

  int ColorMode;
  int ScalarMode;

  double RenderTime;

  // for coloring by a component of a field data array
  int ArrayId;
  char* ArrayName;
  int ArrayComponent;
  int ArrayAccessMode;

  // If coloring by field data, which tuple to use to color the entire
  // data set. If -1, treat array values as cell data.
  vtkIdType FieldDataTupleId;

  vtkTypeBool Static;

  double CoincidentPolygonFactor;
  double CoincidentPolygonOffset;
  double CoincidentLineFactor;
  double CoincidentLineOffset;
  double CoincidentPointOffset;

private:
  vtkMapper(const vtkMapper&) = delete;
  void operator=(const vtkMapper&) = delete;
};

#endif
