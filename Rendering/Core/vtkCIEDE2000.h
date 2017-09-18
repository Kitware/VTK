/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCIEDE2000.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Private header used by vtkColorTransferFunction to support
// LAB/CIEDE2000 interpolation.
#ifndef vtkCIEDE2000_h
#define vtkCIEDE2000_h

#include <vector> // needed for std::vector

namespace CIEDE2000
{
/**
 * Node of the color path
 */
struct Node
{
  double rgb[3];   // RGB color
  double distance; // Distance from the start
};

/**
 * Returns the distance between two colors as given by the
 * CIE Delta E 2000 (CIEDE2000) color distance measure.
 */
double GetCIEDeltaE2000(const double _lab1[3], const double _lab2[3]);

/**
 * Calculates the shortest color path between two colors with respect
 * to the CIEDE2000 measure and returns its overall length.
 */
double GetColorPath(const double _rgb1[3], const double _rgb2[3], std::vector<Node>& _path);
}

#endif
// VTK-HeaderTest-Exclude: vtkCIEDE2000.h
