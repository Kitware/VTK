/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartHelpFormatter.h

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

/// \file vtkQtChartHelpFormatter.h
/// \date June 5, 2008

#ifndef _vtkQtChartHelpFormatter_h
#define _vtkQtChartHelpFormatter_h

#include "vtkQtChartExport.h"
#include <QString> // needed for return type

class QStringList;


/// \class vtkQtChartHelpFormatter
/// \brief
///   The vtkQtChartHelpFormatter class is used to generate help text
///   from a format string.
class VTKQTCHART_EXPORT vtkQtChartHelpFormatter
{
public:
  vtkQtChartHelpFormatter();

  /// \brief
  ///   Creates a help formatter instance.
  /// \param format The help string format.
  vtkQtChartHelpFormatter(const QString &format);
  ~vtkQtChartHelpFormatter() {}

  /// \brief
  ///   Gets the help string format.
  /// \return
  ///   A reference to the help string format.
  const QString &getFormat() const {return this->Format;}

  /// \brief
  ///   Sets the help string format.
  /// \param format The help string format.
  void setFormat(const QString &format) {this->Format = format;}

  /// \brief
  ///   Creates a help string for the given parameters.
  ///
  /// The series name replaces all instances of %s in the format
  /// string. The data list replaces %1, %2, ... %n in the format
  /// string. the list will only replace numbers up to its length.
  ///
  /// \param series The series name.
  /// \param data The list of data arguments.
  /// \return
  ///   The formatted help string.
  QString getHelpText(const QString &series, const QStringList &data) const;

private:
  QString Format; ///< Stores the help format.
};

#endif
