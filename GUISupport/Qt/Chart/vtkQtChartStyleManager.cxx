/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleManager.cxx

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

/// \file vtkQtChartStyleManager.cxx
/// \date February 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartStyleManager.h"

#include <QList>
#include <QMap>


class vtkQtChartStyleManagerInternal
{
public:
  vtkQtChartStyleManagerInternal();
  ~vtkQtChartStyleManagerInternal() {}

  QMap<QString, QObject *> Generators;
};


//----------------------------------------------------------------------------
vtkQtChartStyleManagerInternal::vtkQtChartStyleManagerInternal()
  : Generators()
{
}


//----------------------------------------------------------------------------
vtkQtChartStyleManager::vtkQtChartStyleManager(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartStyleManagerInternal();
}

vtkQtChartStyleManager::~vtkQtChartStyleManager()
{
  delete this->Internal;
}

QObject *vtkQtChartStyleManager::getGenerator(const QString &name) const
{
  QMap<QString, QObject *>::Iterator iter =
      this->Internal->Generators.find(name);
  if(iter != this->Internal->Generators.end())
    {
    return *iter;
    }

  return 0;
}

void vtkQtChartStyleManager::setGenerator(const QString &name,
    QObject *generator)
{
  if(generator && !name.isEmpty())
    {
    this->Internal->Generators.insert(name, generator);
    }
}

void vtkQtChartStyleManager::removeGenerator(const QString &name)
{
  QMap<QString, QObject *>::Iterator iter =
      this->Internal->Generators.find(name);
  if(iter != this->Internal->Generators.end())
    {
    this->Internal->Generators.erase(iter);
    }
}

void vtkQtChartStyleManager::removeGenerator(QObject *generator)
{
  if(generator)
    {
    QMap<QString, QObject *>::Iterator iter =
        this->Internal->Generators.begin();
    while(iter != this->Internal->Generators.end())
      {
      if(*iter == generator)
        {
        iter = this->Internal->Generators.erase(iter);
        }
      else
        {
        ++iter;
        }
      }
    }
}


