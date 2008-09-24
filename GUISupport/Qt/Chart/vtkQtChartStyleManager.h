/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleManager.h

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

/// \file vtkQtChartStyleManager.h
/// \date February 15, 2008

#ifndef _vtkQtChartStyleManager_h
#define _vtkQtChartStyleManager_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartStyleGenerator;


/// \class vtkQtChartStyleManager
/// \brief
///   The vtkQtChartStyleManager class allows several chart layers
///   to share the same style generator.
///
/// Sharing a style generator keeps the style from repeating. This is
/// useful when several chart layers are displayed in the same chart.
/// For example, a line chart and a bar chart can share a style
/// generator to make sure that none of the series are the same color.
class VTKQTCHART_EXPORT vtkQtChartStyleManager : public QObject
{
public:
  /// \brief
  ///   Creates a chart style manager.
  /// \param parent The parent object.
  vtkQtChartStyleManager(QObject *parent=0);
  virtual ~vtkQtChartStyleManager() {}

  /// \brief
  ///   Gets the options generator.
  /// \return
  ///   A pointer to the options generator.
  vtkQtChartStyleGenerator *getGenerator();

  /// \brief
  ///   Sets the options generator.
  /// \param generator The new options generator.
  void setGenerator(vtkQtChartStyleGenerator *generator);

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
  /// Stores the default style generator.
  vtkQtChartStyleGenerator *DefaultGenerator;

  /// Stores the current style generator.
  vtkQtChartStyleGenerator *Generator;
  QList<int> Ids; ///< Stores the list of available indexes.
};

#endif
