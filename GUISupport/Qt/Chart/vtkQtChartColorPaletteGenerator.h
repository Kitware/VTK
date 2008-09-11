/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColorPaletteGenerator.h

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

// .NAME vtkQtChartColorPaletteGenerator - A few simple color schemes for charts
//
// .SECTION Description
//
// While vtkQtChartStyleGenerator lets you specify all the details
// you want by passing in brushes and pens, that's often an annoying
// amount of work.  This class provides several simple color schemes to
// make life easier.

#ifndef __vtkQtChartColorPaletteGenerator_h
#define __vtkQtChartColorPaletteGenerator_h


#include "vtkQtChartExport.h"
#include "vtkQtChartStyleGenerator.h"

class VTKQTCHART_EXPORT vtkQtChartColorPaletteGenerator : public vtkQtChartStyleGenerator
{
  Q_OBJECT

public:
  enum ColorScheme
    {
    Spectrum = 0, //< 7 different hues.
    Warm,         //< 6 warm colors (red to yellow).
    Cool,         //< 7 cool colors (green to purple).
    Blues,        //< 7 different blues.
    WildFlower,   //< 7 colors from blue to magenta.
    Citrus,       //< 6 colors from green to orange.
    Custom        //< User specified color scheme.
    };
  
  // Description:
  //   Creates an options generator instance.
  // \param scheme The initial color scheme.
  // \param parent The parent object.
  vtkQtChartColorPaletteGenerator(ColorScheme scheme=Spectrum, QObject *parent=0);
  virtual ~vtkQtChartColorPaletteGenerator();

  // Description:
  //   Gets the current color scheme.
  // \return
  //   The current color scheme.
  ColorScheme getColorScheme() const {return this->Scheme;}

  // Description:
  //   Sets the color scheme.
  //
  // The color scheme will automatically be changed to \c Custom if
  // the color or style lists are modified.
  //
  // \param scheme The new color scheme.
  void setColorScheme(ColorScheme scheme);
  
private:
  ColorScheme Scheme; //< Stores the current color scheme.
};

#endif

