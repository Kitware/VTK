// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkGraphMapper
 * @brief   map vtkGraph and derived
 * classes to graphics primitives
 *
 *
 * vtkGraphMapper is a mapper to map vtkGraph
 * (and all derived classes) to graphics primitives.
 */

#ifndef vtkGraphMapper_h
#define vtkGraphMapper_h

#include "vtkMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include "vtkSmartPointer.h" // Required for smart pointer internal ivars.

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkMapArrayValues;
class vtkCamera;
class vtkFollower;
class vtkGraph;
class vtkGlyph3D;
class vtkGraphToPolyData;
class vtkIconGlyphFilter;
class vtkCellCenters;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;
class vtkLookupTable;
class vtkTransformCoordinateSystems;
class vtkTexture;
class vtkTexturedActor2D;
class vtkVertexGlyphFilter;

class VTKRENDERINGCORE_EXPORT vtkGraphMapper : public vtkMapper
{
public:
  static vtkGraphMapper* New();
  vtkTypeMacro(vtkGraphMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void Render(vtkRenderer* ren, vtkActor* act) override;

  ///@{
  /**
   * The array to use for coloring vertices.  Default is "color".
   */
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();
  ///@}

  ///@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  void ColorVerticesOn();
  void ColorVerticesOff();
  ///@}

  ///@{
  /**
   * Whether scaled glyphs are on or not.  Default is off.
   * By default this mapper uses vertex glyphs that do not
   * scale. If you turn this option on you will get circles
   * at each vertex and they will scale as you zoom in/out.
   */
  void SetScaledGlyphs(bool arg);
  vtkGetMacro(ScaledGlyphs, bool);
  vtkBooleanMacro(ScaledGlyphs, bool);
  ///@}

  ///@{
  /**
   * Glyph scaling array name. Default is "scale"
   */
  vtkSetStringMacro(ScalingArrayName);
  vtkGetStringMacro(ScalingArrayName);
  ///@}

  ///@{
  /**
   * Whether to show edges or not.  Default is on.
   */
  void SetEdgeVisibility(bool vis);
  bool GetEdgeVisibility();
  vtkBooleanMacro(EdgeVisibility, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  ///@}

  ///@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  vtkSetStringMacro(EnabledEdgesArrayName);
  vtkGetStringMacro(EnabledEdgesArrayName);
  ///@}

  ///@{
  /**
   * Whether to enable/disable edges using array values.  Default is off.
   */
  vtkSetMacro(EnableEdgesByArray, vtkTypeBool);
  vtkGetMacro(EnableEdgesByArray, vtkTypeBool);
  vtkBooleanMacro(EnableEdgesByArray, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  vtkSetStringMacro(EnabledVerticesArrayName);
  vtkGetStringMacro(EnabledVerticesArrayName);
  ///@}

  ///@{
  /**
   * Whether to enable/disable vertices using array values.  Default is off.
   */
  vtkSetMacro(EnableVerticesByArray, vtkTypeBool);
  vtkGetMacro(EnableVerticesByArray, vtkTypeBool);
  vtkBooleanMacro(EnableVerticesByArray, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The array to use for assigning icons.
   */
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();
  ///@}

  /**
   * Associate the icon at index "index" in the vtkTexture to all vertices
   * containing "type" as a value in the vertex attribute array specified by
   * IconArrayName.
   */
  void AddIconType(const char* type, int index);

  /**
   * Clear all icon mappings.
   */
  void ClearIconTypes();

  ///@{
  /**
   * Specify the Width and Height, in pixels, of an icon in the icon sheet.
   */
  void SetIconSize(int* size);
  int* GetIconSize();
  ///@}

  /**
   * Specify where the icons should be placed in relation to the vertex.
   * See vtkIconGlyphFilter.h for possible values.
   */
  void SetIconAlignment(int alignment);

  ///@{
  /**
   * The texture containing the icon sheet.
   */
  vtkTexture* GetIconTexture();
  void SetIconTexture(vtkTexture* texture);
  ///@}

  ///@{
  /**
   * Whether to show icons.  Default is off.
   */
  void SetIconVisibility(bool vis);
  bool GetIconVisibility();
  vtkBooleanMacro(IconVisibility, bool);
  ///@}

  ///@{
  /**
   * Get/Set the vertex point size
   */
  vtkGetMacro(VertexPointSize, float);
  void SetVertexPointSize(float size);
  ///@}

  ///@{
  /**
   * Get/Set the edge line width
   */
  vtkGetMacro(EdgeLineWidth, float);
  void SetEdgeLineWidth(float width);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Get the mtime also considering the lookup table.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set the Input of this mapper.
   */
  void SetInputData(vtkGraph* input);
  vtkGraph* GetInput();
  ///@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double* bounds) override { Superclass::GetBounds(bounds); }

  ///@{
  /**
   * Access to the lookup tables used by the vertex and edge mappers.
   */
  vtkGetObjectMacro(EdgeLookupTable, vtkLookupTable);
  vtkGetObjectMacro(VertexLookupTable, vtkLookupTable);
  ///@}

protected:
  vtkGraphMapper();
  ~vtkGraphMapper() override;

  ///@{
  /**
   * Used to store the vertex and edge color array names
   */
  vtkGetStringMacro(VertexColorArrayNameInternal);
  vtkSetStringMacro(VertexColorArrayNameInternal);
  vtkGetStringMacro(EdgeColorArrayNameInternal);
  vtkSetStringMacro(EdgeColorArrayNameInternal);
  char* VertexColorArrayNameInternal;
  char* EdgeColorArrayNameInternal;
  ///@}

  char* EnabledEdgesArrayName;
  char* EnabledVerticesArrayName;
  vtkTypeBool EnableEdgesByArray;
  vtkTypeBool EnableVerticesByArray;

  vtkGetStringMacro(IconArrayNameInternal);
  vtkSetStringMacro(IconArrayNameInternal);
  char* IconArrayNameInternal;

  vtkSmartPointer<vtkGlyph3D> CircleGlyph;
  vtkSmartPointer<vtkGlyph3D> CircleOutlineGlyph;

  vtkSmartPointer<vtkGraphToPolyData> GraphToPoly;
  vtkSmartPointer<vtkVertexGlyphFilter> VertexGlyph;
  vtkSmartPointer<vtkIconGlyphFilter> IconGlyph;
  vtkSmartPointer<vtkMapArrayValues> IconTypeToIndex;
  vtkSmartPointer<vtkTransformCoordinateSystems> IconTransform;

  vtkSmartPointer<vtkPolyDataMapper> EdgeMapper;
  vtkSmartPointer<vtkPolyDataMapper> VertexMapper;
  vtkSmartPointer<vtkPolyDataMapper> OutlineMapper;
  vtkSmartPointer<vtkPolyDataMapper2D> IconMapper;

  vtkSmartPointer<vtkActor> EdgeActor;
  vtkSmartPointer<vtkActor> VertexActor;
  vtkSmartPointer<vtkActor> OutlineActor;
  vtkSmartPointer<vtkTexturedActor2D> IconActor;

  // Color maps
  vtkLookupTable* EdgeLookupTable;
  vtkLookupTable* VertexLookupTable;

  void ReportReferences(vtkGarbageCollector*) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGraphMapper(const vtkGraphMapper&) = delete;
  void operator=(const vtkGraphMapper&) = delete;

  // Helper function
  vtkPolyData* CreateCircle(bool filled);

  float VertexPointSize;
  float EdgeLineWidth;
  bool ScaledGlyphs;
  char* ScalingArrayName;
};

VTK_ABI_NAMESPACE_END
#endif
