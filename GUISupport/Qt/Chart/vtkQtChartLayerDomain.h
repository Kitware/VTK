/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLayerDomain.h

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

/// \file vtkQtChartLayerDomain.h
/// \date March 4, 2008

#ifndef _vtkQtChartLayerDomain_h
#define _vtkQtChartLayerDomain_h

#include "vtkQtChartExport.h"
#include "vtkQtChartLayer.h" // needed for enum

class vtkQtChartAxisCornerDomain;


/// \class vtkQtChartLayerDomain
/// \brief
///   The vtkQtChartLayerDomain class is used to merge chart layer
///   domains.
class VTKQTCHART_EXPORT vtkQtChartLayerDomain
{
public:
  vtkQtChartLayerDomain();
  ~vtkQtChartLayerDomain();

  /// \brief
  ///   Gets the domain for the specified corner.
  /// \param corner The chart corner.
  /// \return
  ///   A pointer to the merged domain for the specified corner.
  const vtkQtChartAxisCornerDomain *getDomain(
      vtkQtChartLayer::AxesCorner corner) const;

  /// \brief
  ///   Merges the domain for the given corner with the current
  ///   domains.
  /// \param domain The domain to add.
  /// \param corner The chart corner for the domain.
  void mergeDomain(const vtkQtChartAxisCornerDomain &domain,
      vtkQtChartLayer::AxesCorner corner);

  /// Removes all the domains.
  void clear();

private:
  vtkQtChartAxisCornerDomain *Domains[4]; ///< Stores the domains.
};

#endif
