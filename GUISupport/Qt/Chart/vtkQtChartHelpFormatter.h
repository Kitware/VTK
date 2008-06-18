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


class VTKQTCHART_EXPORT vtkQtChartHelpFormatter
{
public:
  vtkQtChartHelpFormatter();
  vtkQtChartHelpFormatter(const QString &format);
  ~vtkQtChartHelpFormatter() {}

  const QString &getFormat() const {return this->Format;}
  void setFormat(const QString &format) {this->Format = format;}

  QString getHelpText(const QString &series, const QStringList &data) const;

private:
  QString Format;
};

#endif
