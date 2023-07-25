// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2015 Greg Fiumara
// SPDX-License-Identifier: BSD-3-Clause AND MIT
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

#include "vtkABINamespace.h"

#include <vector> // needed for std::vector

namespace CIEDE2000
{
VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkCIEDE2000.h
