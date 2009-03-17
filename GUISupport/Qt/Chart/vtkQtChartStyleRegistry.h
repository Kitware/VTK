/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleRegistry.h

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

/// \file vtkQtChartStyleRegistry.h
/// \date March 13, 2009

#ifndef _vtkQtChartStyleRegistry_h
#define _vtkQtChartStyleRegistry_h


#include "vtkQtChartExport.h"

class vtkQtChartStyleRegistryInternal;


/// \class vtkQtChartStyleRegistry
/// \brief
///   The vtkQtChartStyleRegistry class keeps track of reserved style
///   indexes.
class VTKQTCHART_EXPORT vtkQtChartStyleRegistry
{
public:
  vtkQtChartStyleRegistry();
  ~vtkQtChartStyleRegistry();

  /// \brief
  ///   Reserves a style index for the style generator.
  ///
  /// The index returned is the lowest index available. If there are
  /// empty spots from removals, the index will come from the first
  /// empty spot.
  ///
  /// \return
  ///   The reserved style index for the style generator.
  int reserveStyle();

  /// \brief
  ///   Releases a series style index.
  ///
  /// When an index is released, the empty spot is saved so it can be
  /// used for the next reservation.
  ///
  /// \param index The style index to release.
  void releaseStyle(int index);

private:
  vtkQtChartStyleRegistryInternal *Internal; ///< Stores the style indexes.

private:
  vtkQtChartStyleRegistry(const vtkQtChartStyleRegistry &);
  vtkQtChartStyleRegistry &operator=(const vtkQtChartStyleRegistry &);
};

#endif
