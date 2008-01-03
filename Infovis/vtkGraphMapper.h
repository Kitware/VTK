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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraphMapper - map vtkAbstractGraph and derived 
// classes to graphics primitives

// .SECTION Description
// vtkGraphMapper is a mapper to map vtkAbstractGraph 
// (and all derived classes) to graphics primitives. 

#ifndef __vtkGraphMapper_h
#define __vtkGraphMapper_h

#include "vtkMapper.h"

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

class vtkAbstractGraph;
class vtkGlyph2D;
class vtkGraphToPolyData;
class vtkCellCenters;
class vtkPolyDataMapper;
class vtkLookupTable;
class vtkTexture;
class vtkVertexGlyphFilter;
class vtkViewTheme;


class VTK_RENDERING_EXPORT vtkGraphMapper : public vtkMapper 
{
public:
  static vtkGraphMapper *New();
  vtkTypeRevisionMacro(vtkGraphMapper,vtkMapper);
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
  // Get/Set the vertex point size
  vtkGetMacro(VertexPointSize,int);
  void SetVertexPointSize(int size);
  
  // Description:
  // Get/Set the edge line width
  vtkGetMacro(EdgeLineWidth,int);
  void SetEdgeLineWidth(int width);
  
  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

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
  void SetInput(vtkAbstractGraph *input);
  vtkAbstractGraph *GetInput();
  
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

  
  vtkSmartPointer<vtkGraphToPolyData>   GraphToPoly;
  vtkSmartPointer<vtkVertexGlyphFilter> VertexGlyph;
  vtkSmartPointer<vtkGlyph2D>           IconGlyph;
  
  vtkSmartPointer<vtkPolyDataMapper>    EdgeMapper;
  vtkSmartPointer<vtkPolyDataMapper>    VertexMapper;
  vtkSmartPointer<vtkPolyDataMapper>    OutlineMapper;
  
  vtkSmartPointer<vtkActor>             EdgeActor;
  vtkSmartPointer<vtkActor>             VertexActor;
  vtkSmartPointer<vtkActor>             OutlineActor;
  
  vtkSmartPointer<vtkTexture>           IconTexture;
  
  // Color maps
  vtkSmartPointer<vtkLookupTable>       EdgeLookupTable;
  vtkSmartPointer<vtkLookupTable>       VertexLookupTable;

  //virtual void ReportReferences(vtkGarbageCollector*);

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkGraphMapper(const vtkGraphMapper&);  // Not implemented.
  void operator=(const vtkGraphMapper&);  // Not implemented.
  
  int VertexPointSize;
  int EdgeLineWidth;
};

#endif


