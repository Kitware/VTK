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

/// \file vtkQtChartStyleGenerator.h
/// \date February 15, 2008

#ifndef __vtkQtChartStyleGenerator_h
#define __vtkQtChartStyleGenerator_h

#include "vtkQtChartExport.h"
#include <QObject>
#include <QBrush> // Needed for return value.
#include <QPen> // Needed for return value.


/// \class vtkQtChartStyleGenerator
/// \brief
///   The vtkQtChartStyleGenerator class is the base class for all
///   series options generators.
class VTKQTCHART_EXPORT vtkQtChartStyleGenerator : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style generator.
  /// \param parent The parent object.
  vtkQtChartStyleGenerator(QObject *parent=0);
  virtual ~vtkQtChartStyleGenerator() {}

  /// \brief
  ///   Gets the brush for the specified series index.
  /// \param index The series index.
  /// \return
  ///   The brush for the specified series index.
  virtual QBrush getSeriesBrush(int index) const = 0;

  /// \brief
  ///   Gets the pen for the specified series index.
  /// \param index The series index.
  /// \return
  ///   The pen for the specified series index.
  virtual QPen getSeriesPen(int index) const = 0;
};

#endif

