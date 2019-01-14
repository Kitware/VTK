/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

#ifndef Q_VTK_INTERACTOR_INTERNAL_H
#define Q_VTK_INTERACTOR_INTERNAL_H

#include <QtCore/QObject>

#include <map>
class QVTKInteractor;
class QSignalMapper;
class QTimer;

// internal class, do not use
class QVTKInteractorInternal : public QObject
{
  Q_OBJECT
public:
  QVTKInteractorInternal(QVTKInteractor* p);
  ~QVTKInteractorInternal() override;
public Q_SLOTS:
  void TimerEvent(int id);
public:
  QSignalMapper* SignalMapper;
  typedef std::map<int, QTimer*> TimerMap;
  TimerMap Timers;
  QVTKInteractor* Parent;
};


#endif
