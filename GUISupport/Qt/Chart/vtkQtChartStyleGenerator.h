/* -*- Mode: C++; -*- */

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

// .NAME vtkQtChartStyleGenerator - Chart style generator that takes QBrush and QPen arguments for full generality
//
// .SECTION Description
//
// Charts are painted using QPens for the lines (and area borders) and
// QBrushes for filled areas.  This class holds a single chart style,
// i.e. a list of pens and a list of brushes.
//
// A chart uses the style at render time by asking for the pen and
// brush for series N.  The style returns the Nth entry in the pen and
// brush lists.  If N is larger than the list or less than 0 an error
// will be printed and an empty QPen or QBrush will be returned.
//
// .SECTION See Also
// vtkQtChartStyleGenerator
// 

#ifndef __vtkQtChartStyleGenerator_h
#define __vtkQtChartStyleGenerator_h

#include "vtkQtChartExport.h"
#include <QObject>
#include <QBrush>
#include <QPen>

class vtkQtChartStyleGeneratorInternal;

class VTKQTCHART_EXPORT vtkQtChartStyleGenerator : public QObject
{
  Q_OBJECT
public:

  vtkQtChartStyleGenerator(QObject *parent=0);
  ~vtkQtChartStyleGenerator();

  // Description:
  // Return the number of brushes (fill styles) in the color list.
  int getNumberOfBrushes() const;

  // Description:
  // Return the number of pens (stroke styles) in the color list.
  int getNumberOfPens() const;

  // Description:
  // Get a fill style from the list.  The index must be in the range
  // [0, getNumberOfBrushes()-1].  If it is not, an error message will
  // be printed and an empty QBrush will be returned.
  QBrush getBrush(int index) const; 

  // Description:
  // Get a stroke style from the list.  The index must be in the range
  // [0, getNumberOfPens()-1].  If it is not, an error message will
  // be printed and an empty QPen will be returned.
  QPen getPen(int index) const; 

  // Description: 
  // Get a brush/pen for the specified series.  If the index is out of
  // range it will wrap around to the beginning instead of triggering
  // an error.
  QBrush getSeriesBrush(int index) const;
  QPen getSeriesPen(int index) const;

  // Description:
  // Clear out the list of brushes.
  void clearBrushes();

  // Description:
  // Add a new brush to the list of fill styles.
  void addBrush(const QBrush &brush);

  // Description:
  // Insert a new brush into the list of fill styles at the specified
  // index.  
  void insertBrush(int index, const QBrush &brush);

  // Description:
  // Set the brush at the given index.  This method will do nothing
  // (except print an error message) if the index is out of range.
  void setBrush(int index, const QBrush &brush);

  // Description:
  // Remove the brush at the given index.  This method will do nothing
  // (except print an error message) if the index is out of range.
  void removeBrush(int index);

  // Description:
  // Clear the list of pens (stroke styles).
  void clearPens();

  // Description:
  // Add a pen to the list of stroke styles.
  void addPen(const QPen &pen);

  // Description:
  // Insert a new pen into the list of stroke styles at the specified
  // index.
  void insertPen(int index, const QPen &pen);

  // Description:
  // Sets the pen (stroke style) for the given index.  This method
  // does nothing except print a warning if the index is out of range.
  void setPen(int index, const QPen &pen);

  // Description:
  // Remove the pen / stroke style for the given index.
  void removePen(int index);

private:
  /// Stores the list of colors and pen styles.
  vtkQtChartStyleGeneratorInternal *Internal;
};

#endif

