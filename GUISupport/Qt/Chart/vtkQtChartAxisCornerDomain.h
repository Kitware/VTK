/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisCornerDomain.h

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

/// \file vtkQtChartAxisCornerDomain.h
/// \date March 3, 2008

#ifndef _vtkQtChartAxisCornerDomain_h
#define _vtkQtChartAxisCornerDomain_h

#include "vtkQtChartExport.h"
#include "vtkQtChartAxis.h" // needed for enum

class vtkQtChartAxisCornerDomainInternal;
class vtkQtChartAxisDomainPriority;
class vtkQtChartSeriesDomain;


/// \class vtkQtChartAxisCornerDomain
/// \brief
///   The vtkQtChartAxisCornerDomain class is used to merge similar
///   domains for a pair of axes.
class VTKQTCHART_EXPORT vtkQtChartAxisCornerDomain
{
public:
  vtkQtChartAxisCornerDomain();
  vtkQtChartAxisCornerDomain(const vtkQtChartAxisCornerDomain &other);
  ~vtkQtChartAxisCornerDomain();

  /// \brief
  ///   Gets the number of domains.
  ///
  /// Compatible domains are merged together. The number of domains is
  /// the number of different types of domains added to the corner.
  ///
  /// \return
  ///   The number of domains.
  int getNumberOfDomains() const;

  /// \brief
  ///   Gets the domain for the given index.
  /// \param index The domain index.
  /// \return
  ///   A pointer to the domain at the given index.
  const vtkQtChartSeriesDomain *getDomain(int index) const;

  /// \brief
  ///   Gets the domain for the given index.
  /// \param index The domain index.
  /// \return
  ///   A pointer to the domain at the given index.
  vtkQtChartSeriesDomain *getDomain(int index);

  /// \brief
  ///   Gets the best domain match for the given priorities.
  /// \param xPriority The x-axis priority.
  /// \param yPriority The y-axis priority.
  /// \return
  ///   A pointer to the domain for the given priorities.
  const vtkQtChartSeriesDomain *getDomain(
      const vtkQtChartAxisDomainPriority &xPriority,
      const vtkQtChartAxisDomainPriority &yPriority) const;

  /// \brief
  ///   Gets the best domain match for the given type and priority.
  /// \param xDomain The domain type for the x-axis.
  /// \param yPriority The y-axis priority.
  /// \return
  ///   A pointer to the domain for the given type and priority.
  const vtkQtChartSeriesDomain *getDomain(
      vtkQtChartAxis::AxisDomain xDomain,
      const vtkQtChartAxisDomainPriority &yPriority) const;

  /// \brief
  ///   Gets the best domain match for the given type and priority.
  /// \param xPriority The x-axis priority.
  /// \param yDomain The domain type for the y-axis.
  /// \return
  ///   A pointer to the domain for the given type and priority.
  const vtkQtChartSeriesDomain *getDomain(
      const vtkQtChartAxisDomainPriority &xPriority,
      vtkQtChartAxis::AxisDomain yDomain) const;

  /// \brief
  ///   Gets the best domain match for the given types.
  /// \param xDomain The domain type for the x-axis.
  /// \param yDomain The domain type for the y-axis.
  /// \param index Used to return the index of the returned domain.
  /// \return
  ///   A pointer to the domain for the given types.
  const vtkQtChartSeriesDomain *getDomain(
      vtkQtChartAxis::AxisDomain xDomain,
      vtkQtChartAxis::AxisDomain yDomain, int *index=0) const;

  /// \brief
  ///   Merges the given domain with the current domains.
  ///
  /// If the domain is compatible with one of the current domains, it
  /// is merged. Otherwise, the domain is added to the list. The index
  /// can be used to get a pointer to the combined or newly created
  /// domain.
  ///
  /// \param domain The domain to add.
  /// \param index Used to return the index where the domain was added.
  /// \return
  ///   True if the new domain changed the current domains.
  bool mergeDomain(const vtkQtChartSeriesDomain &domain, int *index=0);

  /// \brief
  ///   Removes the domain at the specified index.
  /// \param index The index to remove.
  void removeDomain(int index);

  /// Removes all the domains.
  void clear();

  /// \brief
  ///   Sets the preferences for the horizontal axis.
  /// \param padRange True if the range should be padded.
  /// \param expandToZero True if the range should be expanded to zero.
  /// \param addSpace True if space should be added to the end labels.
  void setHorizontalPreferences(bool padRange, bool expandToZero,
      bool addSpace);

  /// \brief
  ///   Sets the preferences for the vertical axis.
  /// \param padRange True if the range should be padded.
  /// \param expandToZero True if the range should be expanded to zero.
  /// \param addSpace True if space should be added to the end labels.
  void setVerticalPreferences(bool padRange, bool expandToZero,
      bool addSpace);

  vtkQtChartAxisCornerDomain &operator=(
      const vtkQtChartAxisCornerDomain &other);

private:
  vtkQtChartAxisCornerDomainInternal *Internal; ///< Stores the domains.
};

#endif
