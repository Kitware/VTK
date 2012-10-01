/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGraphMapper - map vtkGraph and derived
// classes to graphics primitives

// .SECTION Description
// vtkGraphMapper is a mapper to map vtkGraph
// (and all derived classes) to graphics primitives.

#ifndef __vtkGraphMapper_h
#define __vtkGraphMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper.h"

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

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
  static vtkGraphMapper *New();
  vtkTypeMacro(vtkGraphMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);
  void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // The array to use for coloring vertices.  Default is "color".
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();

  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  void ColorVerticesOn();
  void ColorVerticesOff();

  // Description:
  // Whether scaled glyphs are on or not.  Default is off.
  // By default this mapper uses vertex glyphs that do not
  // scale. If you turn this option on you will get circles
  // at each vertex and they will scale as you zoom in/out.
  void SetScaledGlyphs(bool arg);
  vtkGetMacro(ScaledGlyphs,bool);
  vtkBooleanMacro(ScaledGlyphs, bool);

  // Description:
  // Glyph scaling array name. Default is "scale"
  vtkSetStringMacro(ScalingArrayName);
  vtkGetStringMacro(ScalingArrayName);

  // Description:
  // Whether to show edges or not.  Default is on.
  void SetEdgeVisibility(bool vis);
  bool GetEdgeVisibility();
  vtkBooleanMacro(EdgeVisibility, bool);

  // Description:
  // The array to use for coloring edges.  Default is "color".
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();

  // Description:
  // Whether to color edges.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();

  // Description:
  // The array to use for coloring edges.  Default is "color".
  vtkSetStringMacro(EnabledEdgesArrayName);
  vtkGetStringMacro(EnabledEdgesArrayName);

  // Description:
  // Whether to enable/disable edges using array values.  Default is off.
  vtkSetMacro(EnableEdgesByArray,int);
  vtkGetMacro(EnableEdgesByArray,int);
  vtkBooleanMacro(EnableEdgesByArray,int);

  // Description:
  // The array to use for coloring edges.  Default is "color".
  vtkSetStringMacro(EnabledVerticesArrayName);
  vtkGetStringMacro(EnabledVerticesArrayName);

  // Description:
  // Whether to enable/disable vertices using array values.  Default is off.
  vtkSetMacro(EnableVerticesByArray,int);
  vtkGetMacro(EnableVerticesByArray,int);
  vtkBooleanMacro(EnableVerticesByArray,int);

  // Description:
  // The array to use for assigning icons.
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();

  // Description:
  // Associate the icon at index "index" in the vtkTexture to all vertices
  // containing "type" as a value in the vertex attribute array specified by
  // IconArrayName.
  void AddIconType(char *type, int index);

  // Description:
  // Clear all icon mappings.
  void ClearIconTypes();

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet.
  void SetIconSize(int *size);
  int *GetIconSize();

  // Description:
  // Specify where the icons should be placed in relation to the vertex.
  // See vtkIconGlyphFilter.h for possible values.
  void SetIconAlignment(int alignment);

  // Description:
  // The texture containing the icon sheet.
  vtkTexture *GetIconTexture();
  void SetIconTexture(vtkTexture *texture);

  // Description:
  // Whether to show icons.  Default is off.
  void SetIconVisibility(bool vis);
  bool GetIconVisibility();
  vtkBooleanMacro(IconVisibility, bool);

  // Description:
  // Get/Set the vertex point size
  vtkGetMacro(VertexPointSize,float);
  void SetVertexPointSize(float size);

  // Description:
  // Get/Set the edge line width
  vtkGetMacro(EdgeLineWidth,float);
  void SetEdgeLineWidth(float width);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the mtime also considering the lookup table.
  unsigned long GetMTime();

  // Description:
  // Set the Input of this mapper.
  void SetInputData(vtkGraph *input);
  vtkGraph *GetInput();

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double* bounds)
    { Superclass::GetBounds(bounds); }

  // Description:
  // Access to the lookup tables used by the vertex and edge mappers.
  vtkGetObjectMacro(EdgeLookupTable, vtkLookupTable);
  vtkGetObjectMacro(VertexLookupTable, vtkLookupTable);

protected:
  vtkGraphMapper();
  ~vtkGraphMapper();

  // Description:
  // Used to store the vertex and edge color array names
  vtkGetStringMacro(VertexColorArrayNameInternal);
  vtkSetStringMacro(VertexColorArrayNameInternal);
  vtkGetStringMacro(EdgeColorArrayNameInternal);
  vtkSetStringMacro(EdgeColorArrayNameInternal);
  char* VertexColorArrayNameInternal;
  char* EdgeColorArrayNameInternal;

  char* EnabledEdgesArrayName;
  char* EnabledVerticesArrayName;
  int EnableEdgesByArray;
  int EnableVerticesByArray;

  vtkGetStringMacro(IconArrayNameInternal);
  vtkSetStringMacro(IconArrayNameInternal);
  char* IconArrayNameInternal;

  //BTX
  vtkSmartPointer<vtkGlyph3D>                    CircleGlyph;
  vtkSmartPointer<vtkGlyph3D>                    CircleOutlineGlyph;

  vtkSmartPointer<vtkGraphToPolyData>            GraphToPoly;
  vtkSmartPointer<vtkVertexGlyphFilter>          VertexGlyph;
  vtkSmartPointer<vtkIconGlyphFilter>            IconGlyph;
  vtkSmartPointer<vtkMapArrayValues>                   IconTypeToIndex;
  vtkSmartPointer<vtkTransformCoordinateSystems> IconTransform;

  vtkSmartPointer<vtkPolyDataMapper>    EdgeMapper;
  vtkSmartPointer<vtkPolyDataMapper>    VertexMapper;
  vtkSmartPointer<vtkPolyDataMapper>    OutlineMapper;
  vtkSmartPointer<vtkPolyDataMapper2D>  IconMapper;

  vtkSmartPointer<vtkActor>             EdgeActor;
  vtkSmartPointer<vtkActor>             VertexActor;
  vtkSmartPointer<vtkActor>             OutlineActor;
  vtkSmartPointer<vtkTexturedActor2D>   IconActor;
  //ETX

  // Color maps
  vtkLookupTable* EdgeLookupTable;
  vtkLookupTable* VertexLookupTable;

  virtual void ReportReferences(vtkGarbageCollector*);

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkGraphMapper(const vtkGraphMapper&);  // Not implemented.
  void operator=(const vtkGraphMapper&);  // Not implemented.

  // Helper function
  vtkPolyData* CreateCircle(bool filled);

  float VertexPointSize;
  float EdgeLineWidth;
  bool ScaledGlyphs;
  char* ScalingArrayName;
};

#endif


