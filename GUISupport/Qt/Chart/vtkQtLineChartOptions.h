/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartOptions.h

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

/// \file vtkQtLineChartOptions.h
/// \date June 6, 2008

#ifndef _vtkQtLineChartOptions_h
#define _vtkQtLineChartOptions_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartHelpFormatter;


/// \class vtkQtLineChartOptions
/// \brief
///   The vtkQtLineChartOptions class stores the line chart options.
class VTKQTCHART_EXPORT vtkQtLineChartOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart options instance.
  /// \param parent The parent object.
  vtkQtLineChartOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another line chart options instance.
  /// \param other The line chart options to copy.
  vtkQtLineChartOptions(const vtkQtLineChartOptions &other);
  virtual ~vtkQtLineChartOptions();

  /// \brief
  ///   Gets the chart help text formatter.
  ///
  /// The help text formatter stores the format string. It is also
  /// used to generate the help text.
  ///
  /// \return
  ///   A pointer to the chart help text formatter.
  vtkQtChartHelpFormatter *getHelpFormat() {return this->Help;}

  /// \brief
  ///   Gets the chart help text formatter.
  /// \return
  ///   A pointer to the chart help text formatter.
  const vtkQtChartHelpFormatter *getHelpFormat() const {return this->Help;}

  /// \brief
  ///   Makes a copy of another line chart options instance.
  /// \param other The line chart options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtLineChartOptions &operator=(const vtkQtLineChartOptions &other);

private:
  vtkQtChartHelpFormatter *Help; ///< Stores the help text format.
};

#endif
