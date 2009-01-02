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


/// \class vtkQtChartAxisDomain
/// \brief
///   The vtkQtChartAxisDomain class is used to merge similar domains
///   for an axis.
class VTKQTCHART_EXPORT vtkQtChartAxisDomain
{
public:
  vtkQtChartAxisDomain();
  vtkQtChartAxisDomain(const vtkQtChartAxisDomain &other);
  ~vtkQtChartAxisDomain() {}

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets whether or not the domain is empty.
  ///
  /// The domain is empty if both the range and list are empty.
  ///
  /// \return
  ///   True if the domain is empty.
  bool isEmpty() const;

  /// \brief
  ///   Gets whether or not the range is inside the list.
  /// \return
  ///   True if the range is inside the list.
  bool isRangeInList() const;

  /// \brief
  ///   Gets the axis domain type.
  /// \return
  ///   The axis domain type.
  vtkQtChartAxis::AxisDomain getDomainType() const;

  /// \brief
  ///   Gets the QVariant domain type.
  /// \return
  ///   The QVariant domain type.
  QVariant::Type getVariantType() const;

  /// \brief
  ///   Gets whether or not the given type is compatible with the
  ///   current domain type.
  /// \param domain The QVariant domain type.
  /// \return
  ///   True if the given type is compatible with the current domain.
  bool isTypeCompatible(QVariant::Type domain) const;

  /// \brief
  ///   Gets the current domain.
  /// \param isRange Used to return whether or not the domain is a range.
  /// \return
  ///   A reference to the current domain values.
  const QList<QVariant> &getDomain(bool &isRange) const;

  /// \brief
  ///   Sets the domain to the given range.
  /// \param range A list of two values.
  void setRange(const QList<QVariant> &range);

  /// \brief
  ///   Sets the domain to the given list of values.
  /// \note
  ///   The list should be sorted before calling this method.
  /// \param domain The list of domain values.
  void setDomain(const QList<QVariant> &domain);

  /// \brief
  ///   Merges the given range with the current domain.
  /// \param range A list of two values.
  /// \return
  ///   True if the merge was successful.
  bool mergeRange(const QList<QVariant> &range);

  /// \brief
  ///   Merges the given list with the current domain.
  /// \note
  ///   The list should be sorted before calling this method.
  /// \param domain The list of domain values.
  /// \return
  ///   True if the merge was successful.
  bool mergeDomain(const QList<QVariant> &domain);

  /// \brief
  ///   Merges the given domain with the current domain.
  ///
  /// The axis domain preferences are merged as well as the domain
  /// values.
  ///
  /// \param other The domain to merge.
  /// \return
  ///   True if the merge was successful.
  bool mergeDomain(const vtkQtChartAxisDomain &other);

  /// Clears the domain contents.
  void clear();
  //@}

  /// \name Preference Methods
  //@{
  /// \brief
  ///   Gets whether or not the range should be padded.
  /// \return
  ///   True if the range should be padded.
  bool isRangePaddingUsed() const {return this->PadRange;}

  /// \brief
  ///   Sets whether or not the range should be padded.
  /// \param padRange True if the range should be padded.
  void setRangePaddingUsed(bool padRange) {this->PadRange = padRange;}

  /// \brief
  ///   Gets whether or not the range should be expanded to zero.
  /// \return
  ///   True if the range should be expanded to zero.
  bool isExpansionToZeroUsed() const {return this->ExpandToZero;}

  /// \brief
  ///   Sets whether or not the range should be expanded to zero.
  /// \param expand True if the range should be expanded to zero.
  void setExpansionToZeroUsed(bool expand) {this->ExpandToZero = expand;}

  /// \brief
  ///   Gets whether or not space should be added to the end labels.
  /// \return
  ///   True if space should be added to the end labels.
  bool isExtraSpaceUsed() const {return this->AddSpace;}

  /// \brief
  ///   Sets whether or not space should be added to the end labels.
  /// \param addSpace True if space should be added to the end labels.
  void setExtraSpaceUsed(bool addSpace) {this->AddSpace = addSpace;}

  /// \brief
  ///   Sets the axis preferences.
  /// \param padRange True if the range should be padded.
  /// \param expandToZero True if the range should be expanded to zero.
  /// \param addSpace True if space should be added to the end labels.
  void setPreferences(bool padRange, bool expandToZero, bool addSpace);
  //@}

  vtkQtChartAxisDomain &operator=(const vtkQtChartAxisDomain &other);

public:
  /// \brief
  ///   Translates the QVariant type to axis domain type.
  /// \param domain The QVariant domain type.
  /// \return
  ///   The axis domain type.
  static vtkQtChartAxis::AxisDomain getAxisDomain(QVariant::Type domain);

  /// \brief
  ///   Sorts the list of variants according to value.
  ///
  /// The list of variants is sorted according to value using a quick
  /// sort algorithm. The list is sorted in place and non-recursively.
  ///
  /// \param list The list of shapes to be sorted.
  static void sort(QList<QVariant> &list);

private:
  /// \brief
  ///   Merges the given numeric range with the current domain.
  ///
  /// The numeric values are promoted to doubles if there is a mix of
  /// int and double.
  ///
  /// \param range A list of two numbers.
  /// \return
  ///   True if the merge was successful.
  bool mergeNumberRange(const QList<QVariant> &range);

  /// \brief
  ///   Merges the given numeric list with the current domain.
  /// \param domain The list of domain numbers.
  /// \return
  ///   True if the merge was successful.
  bool mergeNumberDomain(const QList<QVariant> &domain);

  /// \brief
  ///   Merges the given string list with the current domain.
  ///
  /// New strings are appended to the list. Duplicate strings are not
  /// added.
  ///
  /// \param domain The list of domain strings.
  /// \return
  ///   True if the merge was successful.
  bool mergeStringDomain(const QList<QVariant> &domain);

  /// \brief
  ///   Merges the given date range with the current domain.
  ///
  /// The date values are promoted to date-time if there is a mix of
  /// date and date-time.
  ///
  /// \param range A list of two dates.
  /// \return
  ///   True if the merge was successful.
  bool mergeDateRange(const QList<QVariant> &range);

  /// \brief
  ///   Merges the given date list with the current domain.
  /// \param domain The list of domain dates.
  /// \return
  ///   True if the merge was successful.
  bool mergeDateDomain(const QList<QVariant> &domain);

  /// \brief
  ///   Merges the given time range with the current domain.
  /// \param range A list of two times.
  /// \return
  ///   True if the merge was successful.
  bool mergeTimeRange(const QList<QVariant> &range);

  /// \brief
  ///   Merges the given time list with the current domain.
  /// \param domain The list of domain times.
  /// \return
  ///   True if the merge was successful.
  bool mergeTimeDomain(const QList<QVariant> &domain);

private:
  QList<QVariant> List;  ///< Stores the domain list.
  QList<QVariant> Range; ///< Stores the domain range.
  bool PadRange;         ///< True if the range should be padded.
  bool ExpandToZero;     ///< True if the range should be expanded to zero.
  bool AddSpace;         ///< True if space should be added to the end labels.
};

#endif
