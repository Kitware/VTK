/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBasicStyleManager.h

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

/// \file vtkQtChartBasicStyleManager.h
/// \date March 13, 2009

#ifndef _vtkQtChartBasicStyleManager_h
#define _vtkQtChartBasicStyleManager_h


#include "vtkQtChartExport.h"
#include "vtkQtChartStyleManager.h"

class vtkQtChartBasicStyleManagerInternal;
class vtkQtChartColors;
class vtkQtChartStyleRegistry;


/// \class vtkQtChartBasicStyleManager
/// \brief
///   The vtkQtChartBasicStyleManager class manages chart options
///   using a vtkQtChartStyleRegistry.
///
/// The style registry keeps track of previously used style indexes
/// and gives them to new options objects. This reduces the number of
/// reserved indexes, but may cause a re-added options object to get
/// new properties.
class VTKQTCHART_EXPORT vtkQtChartBasicStyleManager :
    public vtkQtChartStyleManager
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a basic chart style manager.
  /// \param parent The parent object.
  vtkQtChartBasicStyleManager(QObject *parent=0);
  virtual ~vtkQtChartBasicStyleManager();

  /// \name vtkQtChartStyleManager Methods
  //@{
  virtual int getStyleIndex(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options) const;

  virtual int insertStyle(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options);

  virtual void removeStyle(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options);
  //@}

  /// \name Color Methods
  //@{
  /// \brief
  ///   Gets the list of colors.
  /// \return
  ///   A pointer to the list of colors.
  vtkQtChartColors *getColors() {return this->Colors;}

  /// \brief
  ///   Gets the list of colors.
  /// \return
  ///   A pointer to the list of colors.
  const vtkQtChartColors *getColors() const {return this->Colors;}
  //@}

private:
  /// Stores the object/style map.
  vtkQtChartBasicStyleManagerInternal *Internal;
  vtkQtChartStyleRegistry *Styles; ///< Stores the style registry.
  vtkQtChartColors *Colors;        ///< Stores the color list.

private:
  vtkQtChartBasicStyleManager(const vtkQtChartBasicStyleManager &);
  vtkQtChartBasicStyleManager &operator=(const vtkQtChartBasicStyleManager &);
};

#endif
