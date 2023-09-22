// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLagrangianMatidaIntegrationModel
 * vtkLagrangianBasicIntegrationModel implementation
 *
 *
 * vtkLagrangianBasicIntegrationModel implementation using
 * article :
 * "Matida, E. A., et al. "Improved numerical simulation of aerosol deposition in
 * an idealized mouth-throat." Journal of Aerosol Science 35.1 (2004): 1-19."
 * Input Array to process are expected as follow :
 * Index 1 is the "FlowVelocity" from flow input in the tracker
 * Index 2 is the "FlowDensity" from flow input in the tracker
 * Index 3 is the "FlowDynamicViscosity" from flow input in the tracker
 * Index 4 is the "ParticleDiameter" from seed (source) input in the tracker
 * Index 5 is the "ParticleDensity" from seed (source) input in the tracker
 *
 * @sa
 * vtkLagrangianParticleTracker vtkLagrangianParticle
 * vtkLagrangianBasicIntegrationModel
 */

#ifndef vtkLagrangianMatidaIntegrationModel_h
#define vtkLagrangianMatidaIntegrationModel_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkLagrangianBasicIntegrationModel.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianMatidaIntegrationModel
  : public vtkLagrangianBasicIntegrationModel
{
public:
  vtkTypeMacro(vtkLagrangianMatidaIntegrationModel, vtkLagrangianBasicIntegrationModel);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLagrangianMatidaIntegrationModel* New();

  // Needed for multiple signatures polymorphism
  using Superclass::FunctionValues;

  /**
   * Evaluate the integration model velocity field
   * f at position x, using data from cell in dataSet with index cellId
   */
  int FunctionValues(vtkLagrangianParticle* particle, vtkDataSet* dataSet, vtkIdType cellId,
    double* weights, double* x, double* f) override;

  ///@{
  /**
   * Specify the acceleration of gravity.
   * Default value is (0, 0, -9.8)
   */
  vtkSetVector3Macro(Gravity, double);
  vtkGetVector3Macro(Gravity, double);

protected:
  vtkLagrangianMatidaIntegrationModel();
  ~vtkLagrangianMatidaIntegrationModel() override;

  static double GetRelaxationTime(double dynVisc, double diameter, double density);

  static double GetDragCoefficient(const double* flowVelocity, const double* particleVelocity,
    double dynVisc, double particleDiameter, double flowDensity);

  double Gravity[3] = { 0, 0, -9.8 };

private:
  vtkLagrangianMatidaIntegrationModel(const vtkLagrangianMatidaIntegrationModel&) = delete;
  void operator=(const vtkLagrangianMatidaIntegrationModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
