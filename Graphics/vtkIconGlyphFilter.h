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
// .NAME vtkIconGlyphFilter - Filter that generates a polydata with texture
// coordinates corresponding to icons within a sheet of icons.
// .SECTION Description
// vtkIconGlyphFilter takes in a vtkPointSet where each point corresponds to
// the center of an icon. Scalar integer data must also be set to give each
// point an icon index. This index is a zero based row major index into an
// image that contains a grid of icons. You must also set pixel Size of the 
// icon image and the size of a particular icon.

// .SECTION See Also
// vtkPolyDataAlgorithm

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


class VTK_GRAPHICS_EXPORT vtkIconGlyphFilter : public vtkPolyDataAlgorithm
{
public:

  // Description
  static vtkIconGlyphFilter *New();
  vtkTypeMacro(vtkIconGlyphFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet
  vtkSetVector2Macro(IconSize,int);
  vtkGetVectorMacro(IconSize,int,2);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet
  vtkSetVector2Macro(IconSheetSize,int);
  vtkGetVectorMacro(IconSheetSize,int,2);


  // Description:
  // Specify whether the Quad generated to place the icon on will be either
  // 1 x 1 or the dimensions specified by IconSize.
  void SetUseIconSize(bool b);
  bool GetUseIconSize();
  vtkBooleanMacro(UseIconSize, bool);

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

protected:
  vtkIconGlyphFilter();
  ~vtkIconGlyphFilter();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  int IconSize[2]; // Size in pixels of an icon in an icon sheet
  int IconSheetSize[2]; // Size in pixels of the icon sheet

  int Gravity;
  bool UseIconSize;

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
