// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @struct   vtkLagrangianThreadedData
 * @brief   struct to hold a user data
 *
 * Struct to hold threaded data used by the Lagrangian Particle Tracker.
 * Can be inherited and initialized in custom models.
 */

#ifndef vtkLagrangianThreadedData_h
#define vtkLagrangianThreadedData_h

#include "vtkBilinearQuadIntersection.h"
#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkInitialValueProblemSolver;

struct VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianThreadedData
{
  vtkNew<vtkGenericCell> GenericCell;
  vtkNew<vtkIdList> IdList;
  vtkNew<vtkPolyData> ParticlePathsOutput;

  // FindInLocators cache data
  int LastDataSetIndex = -1;
  vtkIdType LastCellId = -1;
  double LastCellPosition[3];
  std::vector<double> LastWeights;

  vtkBilinearQuadIntersection* BilinearQuadIntersection;
  vtkDataObject* InteractionOutput;
  vtkInitialValueProblemSolver* Integrator;

  vtkLagrangianThreadedData()
  {
    this->BilinearQuadIntersection = new vtkBilinearQuadIntersection;
    this->IdList->Allocate(10);
  }

  ~vtkLagrangianThreadedData() { delete this->BilinearQuadIntersection; }
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangianThreadedData_h
// VTK-HeaderTest-Exclude: vtkLagrangianThreadedData.h
