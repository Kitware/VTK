/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesDomain.h

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

/// \file vtkQtChartSeriesDomain.h
/// \date March 3, 2008

#ifndef _vtkQtChartSeriesDomain_h
#define _vtkQtChartSeriesDomain_h

#include "vtkQtChartExport.h"

class vtkQtChartAxisDomain;
class vtkQtChartSeriesDomainInternal;


/// \class vtkQtChartSeriesDomain
/// \brief
///   The vtkQtChartSeriesDomain class is used to associate the two
///   domains for a series.
class VTKQTCHART_EXPORT vtkQtChartSeriesDomain
{
public:
  vtkQtChartSeriesDomain();
  vtkQtChartSeriesDomain(const vtkQtChartSeriesDomain &other);
  ~vtkQtChartSeriesDomain();

  /// \brief
  ///   Gets the x-axis domain for the series.
  /// \return
  ///   A reference to the x-axis domain.
  const vtkQtChartAxisDomain &getXDomain() const;

  /// \brief
  ///   Gets the x-axis domain for the series.
  /// \return
  ///   A reference to the x-axis domain.
  vtkQtChartAxisDomain &getXDomain();

  /// \brief
  ///   Gets the y-axis domain for the series.
  /// \return
  ///   A reference to the y-axis domain.
  const vtkQtChartAxisDomain &getYDomain() const;

  /// \brief
  ///   Gets the y-axis domain for the series.
  /// \return
  ///   A reference to the y-axis domain.
  vtkQtChartAxisDomain &getYDomain();

  vtkQtChartSeriesDomain &operator=(const vtkQtChartSeriesDomain &other);

private:
  vtkQtChartSeriesDomainInternal *Internal; ///< Stores the domains.
};

#endif
