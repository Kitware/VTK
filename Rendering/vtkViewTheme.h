/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewTheme.h

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
// .NAME vtkViewTheme - Sets theme colors for a graphical view.
//
// .SECTION Description
// This may be set on any subclass of vtkView.  The view class will attempt
// to use the values set in the theme to customize the view.  Views will not
// generally use every aspect of the theme.
// NOTICE: This class will be deprecated in favor of a more robust
// solution based on style sheets.  Do not become overly-dependent on the
// functionality of themes.

#ifndef __vtkViewTheme_h
#define __vtkViewTheme_h

#include "vtkObject.h"

class vtkScalarsToColors;

class VTK_RENDERING_EXPORT vtkViewTheme : public vtkObject
{
public:
  static vtkViewTheme* New();
  vtkTypeRevisionMacro(vtkViewTheme, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The size of points or vertices
  vtkSetMacro(PointSize, double);
  vtkGetMacro(PointSize, double);

  // Description:
  // The width of lines or edges
  vtkSetMacro(LineWidth, double);
  vtkGetMacro(LineWidth, double);

  // Description:
  // The color and opacity of points or vertices when not mapped through
  // a lookup table.
  vtkSetVector3Macro(PointColor, double);
  vtkGetVector3Macro(PointColor, double);
  vtkSetMacro(PointOpacity, double);
  vtkGetMacro(PointOpacity, double);
  
  // Description:
  // The ranges to use in a lookup table.
  vtkSetVector2Macro(PointHueRange, double);
  vtkGetVector2Macro(PointHueRange, double);
  vtkSetVector2Macro(PointSaturationRange, double);
  vtkGetVector2Macro(PointSaturationRange, double);
  vtkSetVector2Macro(PointValueRange, double);
  vtkGetVector2Macro(PointValueRange, double);
  vtkSetVector2Macro(PointAlphaRange, double);
  vtkGetVector2Macro(PointAlphaRange, double);

  // Description:
  // The color and opacity of cells or edges when not mapped through
  // a lookup table.
  vtkSetVector3Macro(CellColor, double);
  vtkGetVector3Macro(CellColor, double);
  vtkSetMacro(CellOpacity, double);
  vtkGetMacro(CellOpacity, double);
  
  // Description:
  // The ranges to use in a lookup table.
  vtkSetVector2Macro(CellHueRange, double);
  vtkGetVector2Macro(CellHueRange, double);
  vtkSetVector2Macro(CellSaturationRange, double);
  vtkGetVector2Macro(CellSaturationRange, double);
  vtkSetVector2Macro(CellValueRange, double);
  vtkGetVector2Macro(CellValueRange, double);
  vtkSetVector2Macro(CellAlphaRange, double);
  vtkGetVector2Macro(CellAlphaRange, double);

  // Description:
  // The color of any outlines in the view.
  vtkSetVector3Macro(OutlineColor, double);
  vtkGetVector3Macro(OutlineColor, double);
  
  // Description:
  // The color of selected points or vertices.
  vtkSetVector3Macro(SelectedPointColor, double);
  vtkGetVector3Macro(SelectedPointColor, double);
  vtkSetMacro(SelectedPointOpacity, double);
  vtkGetMacro(SelectedPointOpacity, double);
  
  // Description:
  // The color of selected cells or edges.
  vtkSetVector3Macro(SelectedCellColor, double);
  vtkGetVector3Macro(SelectedCellColor, double);
  vtkSetMacro(SelectedCellOpacity, double);
  vtkGetMacro(SelectedCellOpacity, double);

  // Description:
  // The view background color.
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);
  
  // Description:
  // The second background color (for gradients).
  vtkSetVector3Macro(BackgroundColor2, double);
  vtkGetVector3Macro(BackgroundColor2, double);
  
  // Description:
  // The color to use for labelling graph vertices.
  vtkSetVector3Macro(VertexLabelColor, double);
  vtkGetVector3Macro(VertexLabelColor, double);
  
  // Description:
  // The color to use for labelling graph edges.
  vtkSetVector3Macro(EdgeLabelColor, double);
  vtkGetVector3Macro(EdgeLabelColor, double);

  // Description:
  // Convenience methods for creating some default view themes.
  // The return reference is reference-counted, so you will have to call
  // Delete() on the reference when you are finished with it.
  static vtkViewTheme* CreateOceanTheme();
  static vtkViewTheme* CreateMellowTheme();
  static vtkViewTheme* CreateNeonTheme();

  // Description:
  // Whether a given lookup table matches the point or cell theme of this
  // theme.
  bool LookupMatchesPointTheme(vtkScalarsToColors* s2c);
  bool LookupMatchesCellTheme(vtkScalarsToColors* s2c);

protected:
  vtkViewTheme();
  ~vtkViewTheme();

  double PointSize;
  double LineWidth;

  double PointColor[3];
  double PointOpacity;
  double PointHueRange[2];
  double PointSaturationRange[2];
  double PointValueRange[2];
  double PointAlphaRange[2];

  double CellColor[3];
  double CellOpacity;
  double CellHueRange[2];
  double CellSaturationRange[2];
  double CellValueRange[2];
  double CellAlphaRange[2];
  
  double OutlineColor[3];

  double SelectedPointColor[3];
  double SelectedPointOpacity;
  double SelectedCellColor[3];
  double SelectedCellOpacity;

  double BackgroundColor[3];
  double BackgroundColor2[3];
  double VertexLabelColor[3];
  double EdgeLabelColor[3];

private:
  vtkViewTheme(const vtkViewTheme&);  // Not implemented.
  void operator=(const vtkViewTheme&);  // Not implemented.
};

#endif

