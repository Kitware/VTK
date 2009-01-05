/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartHelpFormatter.cxx

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

/// \file vtkQtChartHelpFormatter.cxx
/// \date June 5, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartHelpFormatter.h"

#include <QStringList>


vtkQtChartHelpFormatter::vtkQtChartHelpFormatter()
  : Format()
{
}

vtkQtChartHelpFormatter::vtkQtChartHelpFormatter(const QString &format)
  : Format(format)
{
}

QString vtkQtChartHelpFormatter::getHelpText(const QString &series,
    const QStringList &data) const
{
  // First, add the series name into the text.
  QString result = this->Format;
  QStringList tokens = result.split("%s", QString::KeepEmptyParts,
      Qt::CaseInsensitive);
  if(tokens.size() > 1)
    {
    result = tokens.join(series);
    }

  // Next, add each of the components into the string.
  QStringList::ConstIterator iter = data.begin();
  for(int i = 1; iter != data.end(); ++iter, ++i)
    {
    QString comp = "%" + QString::number(i);
    tokens = result.split(comp, QString::KeepEmptyParts, Qt::CaseInsensitive);
    if(tokens.size() > 1)
      {
      result = tokens.join(*iter);
      }
    }

  return result;
}


