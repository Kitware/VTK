/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianParticle
 * @brief   Basis class for Lagrangian particles.
 *
 *
 * Particle to inject and integrate in the vtkLagrangianParticleTracker.
 * This class does NOT inherit from vtkObject in order to increase performance
 * and reduce memory usage.
 *
 * @sa
 * vtkLagrangianParticleTracker vtkLagrangianBasicIntegrationModel
 * vtkLagrangianMatidaIntegrationModel
*/

#ifndef vtkLagrangianParticle_h
#define vtkLagrangianParticle_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSystemIncludes.h" // For PrintSelf signature and vtkType

class vtkDataSet;
class vtkPointData;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianParticle
{
public:

  typedef enum ParticleTermination
  {
    PARTICLE_TERMINATION_NOT_TERMINATED = 0,
    PARTICLE_TERMINATION_SURF_TERMINATED,
    PARTICLE_TERMINATION_FLIGHT_TERMINATED,
    PARTICLE_TERMINATION_SURF_BREAK,
    PARTICLE_TERMINATION_OUT_OF_DOMAIN,
    PARTICLE_TERMINATION_OUT_OF_STEPS
  } ParticleTermination;

  typedef enum SurfaceInteraction
  {
    SURFACE_INTERACTION_NO_INTERACTION = 0,
    SURFACE_INTERACTION_TERMINATED,
    SURFACE_INTERACTION_BREAK,
    SURFACE_INTERACTION_BOUNCE,
    SURFACE_INTERACTION_PASS,
    SURFACE_INTERACTION_OTHER
  } SurfaceInteraction;

  /**
   * Constructor to create a particle from a seed.
   * numberOfVariable correspond to the result
   * of vtkLagrangianBasicIntegrationModel::GetNumberOfIndependantVariable()
   * and defines the size of the allocated memory for equation variables.
   * seedId is the index of the seed used to generate the particle
   * seedArrayTupleIndex is the index of the tuple to use to recover associated seed data
   * particle data is a pointer to the pointData associated to all particles.
   */
  vtkLagrangianParticle(int numberOfVariables, vtkIdType seedId, vtkIdType particleId,
    vtkIdType seedArrayTupleIndex, double integrationTime, vtkPointData* seedData);

  /**
   * Constructor wrapper to create a partially integrated particle in the domain.
   * It uses the constructor while setting NumberOfSteps and PreviousIntegrationTime
   */
  static vtkLagrangianParticle* NewInstance(int numberOfVariables, vtkIdType seedId,
    vtkIdType particleId, vtkIdType seedArrayTupleIndex, double integrationTime,
    vtkPointData* seedData, vtkIdType numberOfSteps, double previousIntegrationTime);

  /**
   * method to create a particle from a parent particle.
   * This method should not be used until all particles from seeds have been created.
   * Copy all data from the parentParticle into the particle
   * but take a step to move the particle one step further than the parent
   */
  vtkLagrangianParticle* NewParticle(vtkIdType particleId);

  /**
   * method to create an exact clone of a particle.
   */
  vtkLagrangianParticle* CloneParticle();

  /**
   * Destructor.
   */
  virtual ~vtkLagrangianParticle();

  //@{
  /**
   * Get a pointer to Particle variables at its previous position
   * See GetEquationVariables for content description
   */
  inline double* GetPrevEquationVariables()
  {
    return this->PrevEquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the particle variables array.
   * To be used with vtkInitialValueProblemSolver::ComputeNextStep.
   * returned pointer contains the following:
   * x y z u v w k0 .. kn t
   * x y z is the position of the particle
   * u v w is the velocity of the particle
   * k0 .. kn are user variables
   * t is the time, always the last variables.
   * the number of user variables can be recovered by GetNumberOfUserVariables,
   * but it is always NumberOfVariables - 7.
   */
  inline double* GetEquationVariables()
  {
    return this->EquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the particle variables array at its next position.
   * To be used with vtkInitialValueProblemSolver::ComputeNextStep.
   * See GetEquationVariables for content description
   */
  inline double* GetNextEquationVariables()
  {
    return this->NextEquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the previous particle position.
   * Convenience method, giving the same
   * results as GetPrevEquationVariables().
   */
  inline double* GetPrevPosition()
  {
    return this->PrevEquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the particle position.
   * Convenience method, giving the same
   * results as GetEquationVariables().
   */
  inline double* GetPosition()
  {
    return this->EquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the next particle position.
   * Convenience method, giving the same
   * results as GetNextEquationVariables();
   */
  inline double* GetNextPosition()
  {
    return this->NextEquationVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the previous particle velocity.
   * Convenience method, giving the result:
   * GetPrevEquationVariables() + 3;
   */
  inline double* GetPrevVelocity()
  {
    return this->PrevVelocity;
  }
  //@}

  //@{
  /**
   * Get a pointer to the particle velocity.
   * Convenience method, giving the result:
   * GetEquationVariables() + 3;
   */
  inline double* GetVelocity()
  {
    return this->Velocity;
  }
  //@}

  //@{
  /**
   * Get a pointer to the next particle velocity.
   * Convenience method, giving the result:
   * GetNextEquationVariables() + 3;
   */
  inline double* GetNextVelocity()
  {
    return this->NextVelocity;
  }
  //@}

  //@{
  /**
   * Get a pointer to the previous user variables.
   * Convenience method, giving the result:
   * GetPrevEquationVariables() + 6;
   */
  inline double* GetPrevUserVariables()
  {
    return this->PrevUserVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the user variables.
   * Convenience method, giving the result:
   * GetEquationVariables() + 6;
   */
  inline double* GetUserVariables()
  {
    return this->UserVariables;
  }
  //@}

  //@{
  /**
   * Get a pointer to the next user variables.
   * Convenience method, giving the result:
   * GetNextEquationVariables() + 6;
   */
  inline double* GetNextUserVariables()
  {
    return this->NextUserVariables;
  }
  //@}

  /**
   * Move the particle to its next position by putting next equation
   * variable to equation variable and clearing next equation variable.
   * Be sure to have set the StepTime first for accurate IntegrationTime
   * computation
   */
  virtual void MoveToNextPosition();

  /**
   * Get particle id.
   */
  virtual vtkIdType GetId();

  //@{
  /**
   * Set/Get parent particle id.
   * Allow to find the seed particle of any particle.
   */
  virtual void SetParentId(vtkIdType parentId);
  virtual vtkIdType GetParentId();
  //@}

  /**
   * Get the particle original seed index in the seed dataset.
   * Allows to track a specific seed along the tracks.
   */
  virtual vtkIdType GetSeedId();

  /**
   * Get the particle data tuple in a seed array.
   * To be used on the output of
   * vtkLagrangianBasicIntegrationModel::GetSeedArray
   */
  virtual vtkIdType GetSeedArrayTupleIndex();

  /**
   * Get the number of variables used to initialize EquationVariables.
   */
  virtual int GetNumberOfVariables();

  /**
   * Get the number of variables specific to the user.
   */
  virtual int GetNumberOfUserVariables();

  /**
   * Get the particle data.
   */
  virtual vtkPointData* GetSeedData();

  /**
   * Get the last traversed cell id
   */
  vtkIdType GetLastCellId();

  /**
   * Get the dataset containing the last traversed cell
   */
  vtkDataSet* GetLastDataSet();

  /**
   * Get the last intersected surface cell id.
   */
  vtkIdType GetLastSurfaceCellId();

  /**
   * Get the dataset containing the last intersected surface cell
   */
  vtkDataSet* GetLastSurfaceDataSet();

  /**
   * Set the last dataset and last cell id
   */
  void SetLastCell(vtkDataSet* dataset, vtkIdType cellId);

  /**
   * Set the last surface dataset and last surface cell id
   */
  void SetLastSurfaceCell(vtkDataSet* dataset, vtkIdType cellId);

  /**
   * Get particle current number of steps.
   */
  virtual vtkIdType GetNumberOfSteps();

  //@{
  /**
   * Set/Get particle termination.
   * Values out of enum range are accepted
   * Values < 100 are system reserved and should not be used
   */
  virtual void SetTermination(int termination);
  virtual int GetTermination();
  //@}

  //@{
  /**
   * Set/Get particle interaction.
   * Values out of enum range are accepted
   * Values < 100 are system reserved and should not be used
   */
  virtual void SetInteraction(int interaction);
  virtual int GetInteraction();
  //@}

  //@{
  /**
   * Set/Get user flag.
   */
  virtual void SetUserFlag(int flag);
  virtual int GetUserFlag();
  //@}

  //@{
  /**
   * Set/Get parallel specific flag, indication to insert or not
   * the previous position after streaming.
   * No effect in serial.
   */
  virtual void SetPInsertPreviousPosition(bool val);
  virtual bool GetPInsertPreviousPosition();
  //@}

  //@{
  /**
   * Set/Get parallel specific flag, indication that the particle
   * may be manually shifted after streaming.
   * No effect in serial.
   */
  virtual void SetPManualShift(bool val);
  virtual bool GetPManualShift();
  //@}

  /**
   * Get reference to step time of this particle
   */
  virtual double& GetStepTimeRef();

  /**
   * Get the integration time
   */
  virtual double GetIntegrationTime();

  /**
   * Get the integration time at previous position
   */
  virtual double GetPrevIntegrationTime();

  /**
   * Convienience setter for integration time,
   * do not use unless manual particle shifting
   * One using this method may want ot consider
   * modifing EquationVariable[numVals] which
   * contain integrationTime as well,
   * if it matters in their model.
   */
  virtual void SetIntegrationTime(double time);

  /**
   * Compute and return the position vector magnitude
   */
  double GetPositionVectorMagnitude();

  /**
   * Print information about the particle.
   */
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:

  /**
   * Constructor wrapper for internal convenience
   */
  vtkLagrangianParticle* NewInstance(int numberOfVariables,
    vtkIdType seedId, vtkIdType particleId, vtkIdType seedArrayTupleIndex,
    double integrationTime, vtkPointData* seedData);

  vtkLagrangianParticle(const vtkLagrangianParticle&); // Not implemented
  vtkLagrangianParticle(); // Not implemented
  void operator=(const vtkLagrangianParticle&); // Not implemented

  double* PrevEquationVariables;
  double* PrevVelocity;
  double* PrevUserVariables;

  double* EquationVariables;
  double* Velocity;
  double* UserVariables;

  double* NextEquationVariables;
  double* NextVelocity;
  double* NextUserVariables;

  vtkIdType Id;
  vtkIdType ParentId;
  vtkIdType SeedId;
  vtkIdType SeedArrayTupleIndex;
  vtkIdType NumberOfSteps;
  vtkPointData* SeedData;
  vtkDataSet* LastDataSet;
  vtkIdType LastCellId;
  double StepTime;
  double IntegrationTime;
  double PrevIntegrationTime;
  int Termination;
  int Interaction;
  int UserFlag;
  vtkDataSet* LastSurfaceDataSet;
  vtkIdType LastSurfaceCellId;
  int NumberOfVariables;

  // Parallel related flags
  bool PInsertPreviousPosition;
  bool PManualShift;
};

#endif
// VTK-HeaderTest-Exclude: vtkLagrangianParticle.h
