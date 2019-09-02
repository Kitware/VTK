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

/*=========================================================================
The MIT License (MIT)

Copyright (c) 2015 Greg Fiumara

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
=========================================================================*/

/**
 * Private header used by vtkColorTransferFunction to support
 * LAB/CIEDE2000 interpolation.
 *
 * Reference:
 * "Color Interpolation for Non-Euclidean Color Spaces",
 * Zeyen, M., Post, T., Hagen, H., Ahrens, J., Rogers, D. and Bujack, R.,
 * SciVis ShortPapers IEEE VIS 2018.
 * (https://datascience.dsscale.org/wp-content/uploads/sites/3/2019/01/ColorInterpolationforNon-EuclideanColorSpaces.pdf)
 *
 * The implementation is a modified version based on the following:
 * https://github.com/gfiumara/CIEDE2000
 *
 */
#ifndef vtkCIEDE2000_h
#define vtkCIEDE2000_h
#ifndef __VTK_WRAP__

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
 * Map a RGB color to its corresponding color in the sampled RGB space.
 */
void MapColor(double rgb[3]);

/**
 * Returns the distance between two colors as given by the
 * CIE Delta E 2000 (CIEDE2000) color distance measure.
 */
double GetCIEDeltaE2000(const double lab1[3], const double lab2[3]);

/**
 * Calculates the shortest color path between two colors with respect
 * to the CIEDE2000 measure and returns its overall length.
 */
double GetColorPath(const double rgb1[3], const double rgb2[3], std::vector<Node>& path,
  bool forceExactSupportColors);
}

#endif
#endif
// VTK-HeaderTest-Exclude: vtkCIEDE2000.h
