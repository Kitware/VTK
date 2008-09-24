/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartPenBrushGenerator.h

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

/// \file vtkQtChartPenBrushGenerator.h
/// \date September 22, 2008

#ifndef _vtkQtChartPenBrushGenerator_h
#define _vtkQtChartPenBrushGenerator_h

#include "vtkQtChartExport.h"
#include "vtkQtChartStyleGenerator.h"

class vtkQtChartColors;
class vtkQtChartPenBrushGeneratorInternal;


/// \class vtkQtChartPenBrushGenerator
/// \brief
///   The vtkQtChartPenBrushGenerator class generates series drawing
///   options using pen and brush lists.
class VTKQTCHART_EXPORT vtkQtChartPenBrushGenerator :
  public vtkQtChartStyleGenerator
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a pen/brush generator.
  /// \param parent The parent object.
  vtkQtChartPenBrushGenerator(QObject *parent=0);
  virtual ~vtkQtChartPenBrushGenerator();

  /// \name vtkQtChartStyleGenerator Methods
  //@{
  /// \brief
  ///   Gets the brush for the specified series index.
  ///
  /// If the index is greater than the internal brush list, the index
  /// will be wrapped to repeat the brushes.
  ///
  /// \param index The series index.
  /// \return
  ///   The brush for the specified series index.
  virtual QBrush getSeriesBrush(int index) const;

  /// \brief
  ///   Gets the pen for the specified series index.
  ///
  /// If the index is greater than the internal pen list, the index
  /// will be wrapped to repeat the pens.
  ///
  /// \param index The series index.
  /// \return
  ///   The pen for the specified series index.
  virtual QPen getSeriesPen(int index) const;
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

  /// \name Pen Methods
  //@{
  /// \brief
  ///   Gets the number of pens (stroke styles) in the list.
  /// \return
  ///   The number of pens (stroke styles) in the list.
  int getNumberOfPens() const;

  /// \brief
  ///   Gets a stroke style from the list.
  ///
  /// The index must be in the range [0, getNumberOfPens()-1]. If it
  /// is not, an error message will be printed and an empty QPen will
  /// be returned.
  ///
  /// \param index The index of the pen.
  /// \return
  ///   The stroke style for the given index.
  QPen getPen(int index) const;

  /// \brief
  ///   Sets the stroke style for the given index.
  ///
  /// This method will do nothing if the index is out of range.
  ///
  /// \param index The pen list index.
  /// \param pen The new stroke style.
  void setPen(int index, const QPen &pen);

  /// Clears the list of pens (stroke styles).
  void clearPens();

  /// \brief
  ///   Adds the color list to the pen list.
  /// \param colors The list of colors to add.
  void addPens(const vtkQtChartColors &colors);

  /// \brief
  ///   Adds a pen to the list of stroke styles.
  /// \param pen The stroke style to add.
  void addPen(const QPen &pen);

  /// \brief
  ///   Inserts a pen into the list of stroke styles.
  /// \param index Where to insert the pen.
  /// \param pen The stroke style to insert.
  void insertPen(int index, const QPen &pen);

  /// \brief
  ///   Removes the pen at the given index.
  /// \param index The index of the pen to remove.
  void removePen(int index);
  //@}

private:
  /// Stores the pen and brush lists.
  vtkQtChartPenBrushGeneratorInternal *Internal;
};

#endif
