/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColors.h

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

/// \file vtkQtChartColors.h
/// \date September 22, 2008

#ifndef _vtkQtChartColors_h
#define _vtkQtChartColors_h

#include "vtkQtChartExport.h"
#include <QColor> // Needed for return type.

class vtkQtChartColorsInternal;


/// \class vtkQtChartColors
/// \brief
///   The vtkQtChartColors class stores a list of colors.
class VTKQTCHART_EXPORT vtkQtChartColors
{
public:
  enum ColorScheme
    {
    Spectrum = 0, ///< 7 different hues.
    Warm,         ///< 6 warm colors (red to yellow).
    Cool,         ///< 7 cool colors (green to purple).
    Blues,        ///< 7 different blues.
    WildFlower,   ///< 7 colors from blue to magenta.
    Citrus,       ///< 6 colors from green to orange.
    Custom        ///< User specified color scheme.
    };
  
public:
  /// \brief
  ///   Creates a chart colors instance.
  /// \param scheme The initial color scheme.
  vtkQtChartColors(ColorScheme scheme=Spectrum);

  /// \brief
  ///   Makes a copy of another chart colors instance.
  /// \param other The chart colors to copy.
  vtkQtChartColors(const vtkQtChartColors &other);
  ~vtkQtChartColors();

  /// \brief
  ///   Makes a copy of another chart colors instance.
  /// \param other The chart colors to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtChartColors &operator=(const vtkQtChartColors &other);

  /// \brief
  ///   Gets the current color scheme.
  /// \return
  ///   The current color scheme.
  ColorScheme getColorScheme() const {return this->Scheme;}

  /// \brief
  ///   Sets the color scheme.
  ///
  /// The color scheme will automatically be changed to \c Custom if
  /// the color list is modified.
  ///
  /// \param scheme The new color scheme.
  void setColorScheme(ColorScheme scheme);

  /// \brief
  ///   Gets the number of colors in the color list.
  /// \return
  ///   The number of colors in the color list.
  int getNumberOfColors() const;

  /// \brief
  ///   Gets the color for the given index.
  /// \return
  ///   The color for the given index.
  QColor getColor(int index) const;

  /// \brief
  ///   Sets the color for the given index.
  ///
  /// This method does nothing if the index is out of range.
  ///
  /// \param index Which color to modify.
  /// \param color The new color.
  void setColor(int index, const QColor &color);

  /// Clears the list of colors.
  void clearColors();

  /// \brief
  ///   Adds a color to the list of colors.
  /// \param color The new color to add.
  void addColor(const QColor &color);

  /// \brief
  ///   Inserts a new color into the list of colors.
  /// \param index Where to insert the new color.
  /// \param color The new color to insert.
  void insertColor(int index, const QColor &color);

  /// \brief
  ///   Removes the color for the given index.
  /// \param index Which color to remove from the list.
  void removeColor(int index);

public:
  /// \brief
  ///   Creates a lighter color from the given color.
  ///
  /// The \c QColor::light method does not work for black. This
  /// function uses a 3D equation in rgb space to compute the
  /// lighter color, which works for all colors including black.
  /// the factor determines how light the new color will be. The
  /// factor is used to find the point between the current color
  /// and white.
  ///
  /// \param color The starting color.
  /// \param factor A percentage (0.0 to 1.0) of the distance from
  ///   the given color to white.
  /// \return
  ///   The new lighter color.
  static QColor lighter(const QColor &color, float factor=0.7);

  /// \brief
  ///   Gets a new color between the given colors in hsv space.
  ///
  /// The interpolation does not wrap around in the hue component.
  ///
  /// \param color1 The first color.
  /// \param color2 The last color.
  /// \param fraction The fraction (0.0 to 1.0) of the distance from
  ///   \c color1 to \c color2.
  /// \return
  ///   The interpolated color.
  static QColor interpolateHsv(const QColor &color1, const QColor &color2,
      float fraction);

  /// \brief
  ///   Gets a new color between the given colors in rgb space.
  /// \param color1 The first color.
  /// \param color2 The last color.
  /// \param fraction The fraction (0.0 to 1.0) of the distance from
  ///   \c color1 to \c color2.
  /// \return
  ///   The interpolated color.
  static QColor interpolateRgb(const QColor &color1, const QColor &color2,
      float fraction);

private:
  /// \brief
  ///   Computes the distance between the two points.
  /// \param x1 The x-coordinate for the first point.
  /// \param y1 The y-coordinate for the first point.
  /// \param z1 The z-coordinate for the first point.
  /// \param x2 The x-coordinate for the second point.
  /// \param y2 The y-coordinate for the second point.
  /// \param z2 The z-coordinate for the second point.
  static float getDistance(float x1, float y1, float z1, float x2, float y2,
      float z2);

  /// \brief
  ///   Gets the component for interpolation.
  ///
  /// The new component value will be distance \c f from the first point
  /// \c x1 and distance \c s from the second point \c x2.
  ///
  /// \param x1 The first end point component.
  /// \param x2 The second end point component.
  /// \param f The distance from the first end point.
  /// \param s The distance from the second end point.
  static float getComponent(float x1, float x2, float f, float s);

private:
  vtkQtChartColorsInternal *Internal; ///< Stores the color list.
  ColorScheme Scheme;                 ///< Stores the color scheme.
};

#endif
