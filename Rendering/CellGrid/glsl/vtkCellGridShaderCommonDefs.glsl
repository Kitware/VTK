// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// NB: NumIntPt does *NOT* have to be equivalent to NumPtsPerCell.
//     They are different when the "shape" attribute has a different
//     interpolation order than the "color" attribute.

// Number Of corner vertices per (triangular) side
#define NumPtsPerSide {NumPtsPerSide}
// Number of corner vertices per (tetrahedral) cell
#define NumPtsPerCell {NumPtsPerCell}
// The starting ID of all sides of this type (or 0 when DrawingCellsNotSides is true).
#define SideOffset {SideOffset}
// Non-zero (true) when coloring by a scalar field (i.e., color_vals is provided)
#define HaveColors {HaveColors}
// Non-zero (true) when drawing cells rather than sides of cells
#define DrawingCellsNotSides {DrawingCellsNotSides}

// Defines SHAPE_hexahedron, SHAPE_tetrahedron, etc. based on the shape of cell being processed.
// This is used to determine "normal" direction vectors from the shape gradient.
#define SHAPE_{ShapeName} {ShapeName}

// Ensure only the basis functions required are processed by the GLSL compiler.
#define BASIS_{ShapeBasisName}
#ifndef BASIS_{ColorBasisName}
   // Now, the color basis and shape basis might be the same.
   // Prevent a warning; do not define the same BASIS_xxx macro twice.
#  define BASIS_{ColorBasisName}
#endif

// Define which interpolation functions to call for the cell shape and color.
#define axisPermutationForSide {ShapeName}_axisPermutationForSide
#define normalToSideAt {ShapeName}_normalToSideAt

// Define different kinds of color override types
#define ScalarVisualizationOverride_NONE {ScalarVisualizationOverride_NONE}
#define ScalarVisualizationOverride_R {ScalarVisualizationOverride_R}
#define ScalarVisualizationOverride_S {ScalarVisualizationOverride_S}
#define ScalarVisualizationOverride_T {ScalarVisualizationOverride_T}
#define ScalarVisualizationOverride_L2_NORM_R_S {ScalarVisualizationOverride_L2_NORM_R_S}
#define ScalarVisualizationOverride_L2_NORM_S_T {ScalarVisualizationOverride_L2_NORM_S_T}
#define ScalarVisualizationOverride_L2_NORM_T_R {ScalarVisualizationOverride_L2_NORM_T_R}
