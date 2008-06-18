/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleGenerator.h

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

/// \file vtkQtChartStyleGenerator.h
/// \date February 15, 2008

#ifndef _pqChartSeriesOptionsGenerator_h
#define _pqChartSeriesOptionsGenerator_h


#include "vtkQtChartExport.h"
#include <QObject>
#include <QColor> // Needed for return type.
#include <QPen>   // Needed for return type.

class vtkQtChartStyleGeneratorInternal;


/// \class vtkQtChartStyleGenerator
/// \brief
///   The vtkQtChartStyleGenerator class is used to generate drawing
///   options for a chart.
class VTKQTCHART_EXPORT vtkQtChartStyleGenerator : public QObject
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
  ///   Creates an options generator instance.
  /// \param scheme The initial color scheme.
  /// \param parent The parent object.
  vtkQtChartStyleGenerator(ColorScheme scheme=Spectrum, QObject *parent=0);
  virtual ~vtkQtChartStyleGenerator();

  /// \brief
  ///   Gets the number of colors in the color list.
  /// \return
  ///   The number of colors in the color list.
  int getNumberOfColors() const;

  /// \brief
  ///   Gets the number of pen styles in the style list.
  /// \return
  ///   The number of pen styles in the style list.
  int getNumberOfStyles() const;

  /// \brief
  ///   Gets a color from the color list.
  ///
  /// This method provides access to the list of colors. The index
  /// must be in the color list range in order to return a valid
  /// color.
  ///
  /// \param index The list index for the color.
  /// \return
  ///   A color from the color list.
  /// \sa vtkQtChartStyleGenerator::getSeriesColor(int, QColor &)
  QColor getColor(int index) const;

  /// \brief
  ///   Gets a pen style from the pen styles list.
  ///
  /// This method provides access to the list of styles. If the index
  /// is out of range, a default style will be returned.
  ///
  /// \param index The list index for the style.
  /// \return
  ///   The pen style for the given index.
  /// \sa vtkQtChartStyleGenerator::getSeriesPen(int, QPen &)
  Qt::PenStyle getPenStyle(int index) const;

  virtual QColor getSeriesColor(int index) const;
  virtual QPen getSeriesPen(int indexpen) const;

  /// \brief
  ///   Gets the current color scheme.
  /// \return
  ///   The current color scheme.
  ColorScheme getColorScheme() const {return this->Scheme;}

  /// \brief
  ///   Sets the color scheme.
  ///
  /// The color scheme will automatically be changed to \c Custom if
  /// the color or style lists are modified.
  ///
  /// \param scheme The new color scheme.
  void setColorScheme(ColorScheme scheme);

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
  ///   Sets the color for the given index.
  ///
  /// This method does nothing if the index is out of range.
  ///
  /// \param index Which color to modify.
  /// \param color The new color.
  void setColor(int index, const QColor &color);

  /// \brief
  ///   Removes the color for the given index.
  /// \param index Which color to remove from the list.
  void removeColor(int index);

  /// Clears the list of pen styles.
  void clearPenStyles();

  /// \brief
  ///   Adds a pen style to the list of pen style.
  /// \param style The new pen style to add.
  void addPenStyle(Qt::PenStyle style);

  /// \brief
  ///   Inserts a new pen style into the list of pen style.
  /// \param index Where to insert the new pen style.
  /// \param style The new pen style to insert.
  void insertPenStyle(int index, Qt::PenStyle style);

  /// \brief
  ///   Sets the pen style for the given index.
  ///
  /// This method does nothing if the index is out of range.
  ///
  /// \param index Which pen style to modify.
  /// \param style The new pen style.
  void setPenStyle(int index, Qt::PenStyle style);

  /// \brief
  ///   Removes the pen style for the given index.
  /// \param index Which pen style to remove from the list.
  void removePenStyle(int index);

private:
  /// Stores the list of colors and pen styles.
  vtkQtChartStyleGeneratorInternal *Internal;
  ColorScheme Scheme; ///< Stores the current color scheme.
};

#endif

