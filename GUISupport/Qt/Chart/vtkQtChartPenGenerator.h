/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartPenGenerator.h

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

/// \file vtkQtChartPenGenerator.h
/// \date March 17, 2009

#ifndef _vtkQtChartPenGenerator_h
#define _vtkQtChartPenGenerator_h


#include "vtkQtChartExport.h"
#include "vtkQtChartStylePen.h"

class vtkQtChartColors;
class vtkQtChartPenGeneratorInternal;


/// \class vtkQtChartPenGenerator
/// \brief
///   The vtkQtChartPenGenerator class generates series pen options
///   using a list of pens.
class VTKQTCHART_EXPORT vtkQtChartPenGenerator : public vtkQtChartStylePen
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a pen generator.
  /// \param parent The parent object.
  vtkQtChartPenGenerator(QObject *parent=0);
  virtual ~vtkQtChartPenGenerator();

  /// \name vtkQtChartStylePen Methods
  //@{
  /// \brief
  ///   Gets the pen for the specified series style index.
  ///
  /// If the index is greater than the internal pen list, the index
  /// will be wrapped to repeat the pens.
  ///
  /// \param index The series style index.
  /// \return
  ///   The pen for the specified series style index.
  virtual QPen getStylePen(int index) const;
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
  vtkQtChartPenGeneratorInternal *Internal; ///< Stores the pen list.

private:
  vtkQtChartPenGenerator(const vtkQtChartPenGenerator &);
  vtkQtChartPenGenerator &operator=(const vtkQtChartPenGenerator &);
};

#endif
