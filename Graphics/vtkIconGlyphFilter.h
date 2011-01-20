/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIconGlyphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIconGlyphFilter - Filter that generates a polydata consisting of
// quads with texture coordinates referring to a set of icons within a sheet
// of icons.
// .SECTION Description
// vtkIconGlyphFilter takes in a vtkPointSet where each point corresponds to
// the center of an icon. Scalar integer data must also be set to give each
// point an icon index. This index is a zero based row major index into an
// image that contains a grid of icons (each icon is the same size). You must
// also specify 1) the size of the icon in the icon sheet (in pixels), 2) the
// size of the icon sheet (in pixels), and 3) the display size of each icon
// (again in display coordinates, or pixels).
//
// Various other parameters are used to control how this data is combined. If
// UseIconSize is true then the DisplaySize is ignored. If PassScalars is true,
// then the scalar index information is passed to the output. Also, there is an
// optional IconScale array which, if UseIconScaling is on, will scale each icon
// independently.

// .SECTION See Also
// vtkPolyDataAlgorithm vtkGlyph3D vtkGlyph2D

#ifndef __vtkIconGlyphFilter_h
#define __vtkIconGlyphFilter_h

#include "vtkPolyDataAlgorithm.h"

#define VTK_ICON_GRAVITY_TOP_RIGHT     1
#define VTK_ICON_GRAVITY_TOP_CENTER    2
#define VTK_ICON_GRAVITY_TOP_LEFT      3
#define VTK_ICON_GRAVITY_CENTER_RIGHT  4
#define VTK_ICON_GRAVITY_CENTER_CENTER 5
#define VTK_ICON_GRAVITY_CENTER_LEFT   6
#define VTK_ICON_GRAVITY_BOTTOM_RIGHT  7
#define VTK_ICON_GRAVITY_BOTTOM_CENTER 8
#define VTK_ICON_GRAVITY_BOTTOM_LEFT   9

#define VTK_ICON_SCALING_OFF 0
#define VTK_ICON_SCALING_USE_SCALING_ARRAY 1


class VTK_GRAPHICS_EXPORT vtkIconGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  // Description
  // Standard VTK methods.
  static vtkIconGlyphFilter *New();
  vtkTypeMacro(vtkIconGlyphFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet.
  vtkSetVector2Macro(IconSize,int);
  vtkGetVectorMacro(IconSize,int,2);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet.
  vtkSetVector2Macro(IconSheetSize,int);
  vtkGetVectorMacro(IconSheetSize,int,2);

  // Description:
  // Specify the Width and Height, in pixels, of the size of the icon when it
  // is rendered. By default, the IconSize is used to set the display size
  // (i.e., UseIconSize is true by default). Note that assumes that
  // IconScaling is disabled, or if enabled, the scale of a particular icon
  // is 1.
  vtkSetVector2Macro(DisplaySize,int);
  vtkGetVectorMacro(DisplaySize,int,2);

  // Description:
  // Specify whether the Quad generated to place the icon on will be either
  // the dimensions specified by IconSize or the DisplaySize.
  vtkSetMacro(UseIconSize,bool);
  vtkGetMacro(UseIconSize,bool);
  vtkBooleanMacro(UseIconSize, bool);

  // Description:
  // Specify how to specify individual icons. By default, icon scaling
  // is off, but if it is on, then the filter looks for an array named
  // "IconScale" to control individual icon size.
  vtkSetMacro(IconScaling,int);
  vtkGetMacro(IconScaling,int);
  void SetIconScalingToScalingOff() {this->SetIconScaling(VTK_ICON_SCALING_OFF);}
  void SetIconScalingToScalingArray()
    {this->SetIconScaling(VTK_ICON_SCALING_USE_SCALING_ARRAY);}

  // Description:
  // Specify whether to pass the scalar icon index to the output. By
  // default this is not passed since it can affect color during the
  // rendering process. Note that all other point data is passed to
  // the output regardless of the value of this flag.
  vtkSetMacro(PassScalars,bool);
  vtkGetMacro(PassScalars,bool);
  vtkBooleanMacro(PassScalars,bool);

  // Description:
  // Specify if the input points define the center of the icon quad or one of
  // top right corner, top center, top left corner, center right, center, center
  // center left, bottom right corner, bottom center or bottom left corner.
  vtkSetMacro(Gravity, int);
  vtkGetMacro(Gravity, int);
  void SetGravityToTopRight() {this->SetGravity(VTK_ICON_GRAVITY_TOP_RIGHT);};
  void SetGravityToTopCenter() {this->SetGravity(VTK_ICON_GRAVITY_TOP_CENTER);};
  void SetGravityToTopLeft() {this->SetGravity(VTK_ICON_GRAVITY_TOP_LEFT);};
  void SetGravityToCenterRight() {this->SetGravity(VTK_ICON_GRAVITY_CENTER_RIGHT);};
  void SetGravityToCenterCenter() {this->SetGravity(VTK_ICON_GRAVITY_CENTER_CENTER);};
  void SetGravityToCenterLeft() {this->SetGravity(VTK_ICON_GRAVITY_CENTER_LEFT);};
  void SetGravityToBottomRight() {this->SetGravity(VTK_ICON_GRAVITY_BOTTOM_RIGHT);};
  void SetGravityToBottomCenter() {this->SetGravity(VTK_ICON_GRAVITY_BOTTOM_CENTER);};
  void SetGravityToBottomLeft() {this->SetGravity(VTK_ICON_GRAVITY_BOTTOM_LEFT);};

  // Description:
  // Specify an offset (in pixels or display coordinates) that offsets the icons
  // from their generating points.
  vtkSetVector2Macro(Offset,int);
  vtkGetVectorMacro(Offset,int,2);

protected:
  vtkIconGlyphFilter();
  ~vtkIconGlyphFilter();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  int IconSize[2]; // Size in pixels of an icon in an icon sheet
  int IconSheetSize[2]; // Size in pixels of the icon sheet
  int DisplaySize[2]; // Size in pixels of the icon when displayed

  int  Gravity;
  bool UseIconSize;
  int  IconScaling;
  bool PassScalars;
  int Offset[2];

private:
  vtkIconGlyphFilter(const vtkIconGlyphFilter&);  // Not implemented.
  void operator=(const vtkIconGlyphFilter&);  // Not implemented.

  void IconConvertIndex(int id, int & j, int & k);
};

inline void vtkIconGlyphFilter::IconConvertIndex(int id, int & j, int & k)
{
  int dimX = this->IconSheetSize[0]/this->IconSize[0];
  int dimY = this->IconSheetSize[1]/this->IconSize[1];

  j = id - dimX * static_cast<int>(id/dimX);
  k = dimY - static_cast<int>(id/dimX) - 1;
}

#endif
