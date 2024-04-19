// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef UNSTRUCTUREDGHOSTZONESCOMMON_H_
#define UNSTRUCTUREDGHOSTZONESCOMMON_H_

// VTK includes
#include "vtkMPIUtilities.h"
#include "vtkMathUtilities.h"

// C/C++ includes
#include <cmath>
#include <iomanip>
#include <sstream>

//------------------------------------------------------------------------------
//    G L O B A L   D A T A
//------------------------------------------------------------------------------
struct global
{
  static double Origin[3];
  static double Spacing[3];
  static int Dims[3];

  static int Rank;
  static int NRanks;

  static vtkUnstructuredGrid* Grid;
};

int CheckGrid(vtkUnstructuredGrid* ghostGrid, int iteration);

//------------------------------------------------------------------------------
void UpdateGrid(int iteration);

//------------------------------------------------------------------------------
void SetXYZCellField();

//------------------------------------------------------------------------------
void SetXYZNodeField();

//------------------------------------------------------------------------------
void WriteDataSet(vtkUnstructuredGrid* grid, const std::string& file);

//------------------------------------------------------------------------------
void GetPoint(int i, int j, int k, double pnt[3]);

void GenerateDataSet();

#endif /* UNSTRUCTUREDGHOSTZONESCOMMON_H_ */
