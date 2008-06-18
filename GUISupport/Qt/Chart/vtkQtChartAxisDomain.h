/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisDomain.h

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

/// \file vtkQtChartAxisDomain.h
/// \date February 14, 2008

#ifndef _vtkQtChartAxisDomain_h
#define _vtkQtChartAxisDomain_h

#include "vtkQtChartExport.h"
#include "vtkQtChartAxis.h" // needed for enum
#include <QList>            // needed for parameter
#include <QVariant>         // needed for parameter/enum


class VTKQTCHART_EXPORT vtkQtChartAxisDomain
{
public:
  vtkQtChartAxisDomain();
  vtkQtChartAxisDomain(const vtkQtChartAxisDomain &other);
  ~vtkQtChartAxisDomain() {}

  bool isEmpty() const;
  bool isRangeInList() const;
  vtkQtChartAxis::AxisDomain getDomainType() const;
  QVariant::Type getVariantType() const;
  bool isTypeCompatible(QVariant::Type domain) const;

  const QList<QVariant> &getDomain(bool &isRange) const;

  void setRange(const QList<QVariant> &range);
  void setDomain(const QList<QVariant> &domain);
  bool mergeRange(const QList<QVariant> &range);
  bool mergeDomain(const QList<QVariant> &domain);
  bool mergeDomain(const vtkQtChartAxisDomain &other);
  void clear();

  bool isRangePaddingUsed() const {return this->PadRange;}
  void setRangePaddingUsed(bool padRange) {this->PadRange = padRange;}

  bool isExpansionToZeroUsed() const {return this->ExpandToZero;}
  void setExpansionToZeroUsed(bool expand) {this->ExpandToZero = expand;}

  bool isExtraSpaceUsed() const {return this->AddSpace;}
  void setExtraSpaceUsed(bool addSpace) {this->AddSpace = addSpace;}

  void setPreferences(bool padRange, bool expandToZero, bool addSpace);

  vtkQtChartAxisDomain &operator=(const vtkQtChartAxisDomain &other);

public:
  static vtkQtChartAxis::AxisDomain getAxisDomain(QVariant::Type domain);

private:
  bool mergeNumberRange(const QList<QVariant> &range);
  bool mergeNumberDomain(const QList<QVariant> &range);
  bool mergeStringDomain(const QList<QVariant> &range);
  bool mergeDateRange(const QList<QVariant> &range);
  bool mergeDateDomain(const QList<QVariant> &range);
  bool mergeTimeRange(const QList<QVariant> &range);
  bool mergeTimeDomain(const QList<QVariant> &range);

private:
  QList<QVariant> List;
  QList<QVariant> Range;
  bool PadRange;
  bool ExpandToZero;
  bool AddSpace;
};

#endif
