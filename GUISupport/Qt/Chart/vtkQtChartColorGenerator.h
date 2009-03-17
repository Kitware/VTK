/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColorGenerator.h

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

/// \file vtkQtChartColorGenerator.h
/// \date March 16, 2009

#ifndef _vtkQtChartColorGenerator_h
#define _vtkQtChartColorGenerator_h


#include "vtkQtChartExport.h"
#include "vtkQtChartStyleBrush.h"

class vtkQtChartColors;


/// \class vtkQtChartColorGenerator
/// \brief
///   The vtkQtChartColorGenerator class generates series brushs
///   using a color list.
class VTKQTCHART_EXPORT vtkQtChartColorGenerator : public vtkQtChartStyleBrush
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a color generator.
  /// \param parent The parent object.
  vtkQtChartColorGenerator(QObject *parent=0);
  virtual ~vtkQtChartColorGenerator() {}

  /// \name vtkQtChartStyleBrush Methods
  //@{
  /// \brief
  ///   Gets the brush for the specified series style index.
  ///
  /// If the index is greater than the internal color list, the index
  /// will be wrapped around repeating the colors.
  ///
  /// \param index The series style index.
  /// \return
  ///   The brush for the specified series style index.
  virtual QBrush getStyleBrush(int index) const;
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

  /// \brief
  ///   Sets the list of colors.
  /// \param colors The new list of colors.
  void setColors(vtkQtChartColors *colors) {this->Colors = colors;}
  //@}

private:
  vtkQtChartColors *Colors; ///< Stores the color list.

private:
  vtkQtChartColorGenerator(const vtkQtChartColorGenerator &);
  vtkQtChartColorGenerator &operator=(const vtkQtChartColorGenerator &);
};

#endif
