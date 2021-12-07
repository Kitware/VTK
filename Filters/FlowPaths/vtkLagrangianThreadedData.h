/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#endif // vtkLagrangianThreadedData_h
// VTK-HeaderTest-Exclude: vtkLagrangianThreadedData.h
