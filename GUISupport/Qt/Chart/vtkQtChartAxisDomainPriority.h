/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisDomainPriority.h

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

/// \file vtkQtChartAxisDomainPriority.h
/// \date February 14, 2008

#ifndef _vtkQtChartAxisDomainPriority_h
#define _vtkQtChartAxisDomainPriority_h

#include "vtkQtChartExport.h"
#include <QList> // Needed for parameter and return type.


/// \class vtkQtChartAxisDomainPriority
/// \brief
///   The vtkQtChartAxisDomainPriority class stores the domain
///   priority order.
class VTKQTCHART_EXPORT vtkQtChartAxisDomainPriority
{
public:
  enum DomainType
    {
    Number = 0, ///< Domain for int and double.
    Date,       ///< Domain for QDate and QDateTime.
    Time,       ///< Domain for QTime.
    String      ///< Domain for QString.
    };

public:
  vtkQtChartAxisDomainPriority();
  vtkQtChartAxisDomainPriority(const vtkQtChartAxisDomainPriority &other);
  ~vtkQtChartAxisDomainPriority() {}

  /// \brief
  ///   Gets the default domain priority order.
  /// \return
  ///   The default domain priority order.
  QList<int> getDefaultOrder() const;

  /// \brief
  ///   Gets the current domain priority order.
  /// \return
  ///   A reference to the domain priority order.
  const QList<int> &getOrder() const {return this->Order;}

  /// \brief
  ///   Sets the domain priority order.
  /// \param order The new domain priority order.
  void setOrder(const QList<int> &order);

  vtkQtChartAxisDomainPriority &operator=(
      const vtkQtChartAxisDomainPriority &other);
  bool operator==(const vtkQtChartAxisDomainPriority &other) const;
  bool operator!=(const vtkQtChartAxisDomainPriority &other) const;

private:
  QList<int> Order; ///< Stores the domain priority order.
};

#endif
