/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBrushGenerator.h

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

/// \file vtkQtChartBrushGenerator.h
/// \date March 17, 2009

#ifndef _vtkQtChartBrushGenerator_h
#define _vtkQtChartBrushGenerator_h


#include "vtkQtChartExport.h"
#include "vtkQtChartStyleBrush.h"

class vtkQtChartBrushGeneratorInternal;
class vtkQtChartColors;


/// \class vtkQtChartBrushGenerator
/// \brief
///   The vtkQtChartBrushGenerator class generates series brush
///   options using a list of brushes.
class VTKQTCHART_EXPORT vtkQtChartBrushGenerator : public vtkQtChartStyleBrush
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a brush generator.
  /// \param parent The parent object.
  vtkQtChartBrushGenerator(QObject *parent=0);
  virtual ~vtkQtChartBrushGenerator();

  /// \name vtkQtChartStyleBrush Methods
  //@{
  /// \brief
  ///   Gets the brush for the specified series style index.
  ///
  /// If the index is greater than the internal brush list, the index
  /// will be wrapped to repeat the brushes.
  ///
  /// \param index The series style index.
  /// \return
  ///   The brush for the specified series style index.
  virtual QBrush getStyleBrush(int index) const;
  //@}

  /// \name Brush Methods
  //@{
  /// \brief
  ///   Gets the number of brushes (fill styles) in the list.
  /// \return
  ///   The number of brushes (fill styles) in the list.
  int getNumberOfBrushes() const;

  /// \brief
  ///   Gets a fill style from the list.
  ///
  /// The index must be in the range [0, getNumberOfBrushes()-1]. If
  /// it is not, an error message will be printed and an empty QBrush
  /// will be returned.
  ///
  /// \param index The index of the brush.
  /// \return
  ///   The fill style for the given index.
  QBrush getBrush(int index) const;

  /// \brief
  ///   Sets the fill style for the given index.
  ///
  /// This method will do nothing if the index is out of range.
  ///
  /// \param index The brush list index.
  /// \param brush The new fill style.
  void setBrush(int index, const QBrush &brush);

  /// Clears the list of brushes (fill styles).
  void clearBrushes();

  /// \brief
  ///   Adds the color list to the brush list.
  /// \param colors The list of colors to add.
  void addBrushes(const vtkQtChartColors &colors);

  /// \brief
  ///   Adds a new brush to the list of fill styles.
  /// \param brush The fill style to add.
  void addBrush(const QBrush &brush);

  /// \brief
  ///   Inserts a new brush into the list of fill styles.
  /// \param index Where to insert the brush.
  /// \param brush The fill style to insert.
  void insertBrush(int index, const QBrush &brush);

  /// \brief
  ///   Removes the brush at the given index.
  /// \param index The index of the brush to remove.
  void removeBrush(int index);
  //@}

private:
  vtkQtChartBrushGeneratorInternal *Internal; ///< Stores the brush list.

private:
  vtkQtChartBrushGenerator(const vtkQtChartBrushGenerator &);
  vtkQtChartBrushGenerator &operator=(const vtkQtChartBrushGenerator &);
};

#endif
