// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkNew.h"                    // For vtkNew
#include "vtkSystemIncludes.h"         // For PrintSelf signature and vtkType

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractCellLocator;
class vtkBilinearQuadIntersection;
class vtkDataSet;
class vtkGenericCell;
class vtkIdList;
class vtkPointData;
struct vtkLagrangianThreadedData;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianParticle
{
public:
  /**
   * An enum to inform about a reason for termination
   * PARTICLE_TERMINATION_NOT_TERMINATED = 0, means the particle have not yet been terminated
   * PARTICLE_TERMINATION_SURF_TERMINATED = 1, means the particle have been terminated during
   *   a surface interaction
   * PARTICLE_TERMINATION_FLIGHT_TERMINATED = 2, means the particle have been terminated by
   *   the model during a FreeFlightCheck() call
   * PARTICLE_TERMINATION_SURF_BREAK = 3, means the particle have been terminated during a
   *   surface interaction by a break, meaning new particles have been created from it.
   * PARTICLE_TERMINATION_OUT_OF_DOMAIN = 4, means the particle was terminated when going
   *   out of domain, if the surface is watertight this should not happen.
   * PARTICLE_TERMINATION_OUT_OF_STEPS = 5, means the particle was terminated because
   *   maximum number of steps was reached
   * PARTICLE_TERMINATION_OUT_OF_TIME = 6, means the particle was terminated because
   *   maximum integration time was reached
   * PARTICLE_TERMINATION_TRANSFERRED = 7, means the particle was terminated because
   *   it was transferred to another process to continue the integration
   */
  typedef enum ParticleTermination
  {
    PARTICLE_TERMINATION_NOT_TERMINATED = 0,
    PARTICLE_TERMINATION_SURF_TERMINATED,
    PARTICLE_TERMINATION_FLIGHT_TERMINATED,
    PARTICLE_TERMINATION_SURF_BREAK,
    PARTICLE_TERMINATION_OUT_OF_DOMAIN,
    PARTICLE_TERMINATION_OUT_OF_STEPS,
    PARTICLE_TERMINATION_OUT_OF_TIME,
    PARTICLE_TERMINATION_TRANSFERRED,
    PARTICLE_TERMINATION_ABORTED
  } ParticleTermination;

  /**
   * An enum to inform about a surface interaction
   * SURFACE_INTERACTION_NO_INTERACTION = 0, no interaction have taken place
   * SURFACE_INTERACTION_TERMINATED = 1, a particle was terminated on interaction
   * SURFACE_INTERACTION_BREAK = 2, a particle broke on interaction, hence terminating it and
   * creating new particles from it
   * SURFACE_INTERACTION_BOUNCE = 3, a particle bounced on interaction
   * SURFACE_INTERACTION_PASS = 4, a particle passed through the surface, hence having
   * no effect on the particle but actually recording it going through
   * SURFACE_INTERACTION_OTHER = 5, another type of undefined interaction happened.
   */
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
    vtkIdType seedArrayTupleIndex, double integrationTime, vtkPointData* seedData,
    int numberOfTrackedUserData);

  /**
   * Constructor wrapper to create a partially integrated particle in the domain.
   * It uses the constructor while setting NumberOfSteps and PreviousIntegrationTime
   */
  static vtkLagrangianParticle* NewInstance(int numberOfVariables, vtkIdType seedId,
    vtkIdType particleId, vtkIdType seedArrayTupleIndex, double integrationTime,
    vtkPointData* seedData, int numberOfTrackedUserData, vtkIdType numberOfSteps = 0,
    double previousIntegrationTime = 0);

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

  ///@{
  /**
   * Get a pointer to Particle variables at its previous position
   * See GetEquationVariables for content description
   */
  inline double* GetPrevEquationVariables() { return this->PrevEquationVariables.data(); }
  ///@}

  ///@{
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
  inline double* GetEquationVariables() { return this->EquationVariables.data(); }
  ///@}

  ///@{
  /**
   * Get a pointer to the particle variables array at its next position.
   * To be used with vtkInitialValueProblemSolver::ComputeNextStep.
   * See GetEquationVariables for content description
   */
  inline double* GetNextEquationVariables() { return this->NextEquationVariables.data(); }
  ///@}

  ///@{
  /**
   * Get a pointer to the previous particle position.
   * Convenience method, giving the same
   * results as GetPrevEquationVariables().
   */
  inline double* GetPrevPosition() { return this->PrevEquationVariables.data(); }
  ///@}

  ///@{
  /**
   * Get a pointer to the particle position.
   * Convenience method, giving the same
   * results as GetEquationVariables().
   */
  inline double* GetPosition() { return this->EquationVariables.data(); }
  ///@}

  ///@{
  /**
   * Get a pointer to the next particle position.
   * Convenience method, giving the same
   * results as GetNextEquationVariables();
   */
  inline double* GetNextPosition() { return this->NextEquationVariables.data(); }
  ///@}

  ///@{
  /**
   * Get a pointer to the previous particle velocity.
   * Convenience method, giving the result:
   * GetPrevEquationVariables() + 3;
   */
  inline double* GetPrevVelocity() { return this->PrevVelocity; }
  ///@}

  ///@{
  /**
   * Get a pointer to the particle velocity.
   * Convenience method, giving the result:
   * GetEquationVariables() + 3;
   */
  inline double* GetVelocity() { return this->Velocity; }
  ///@}

  ///@{
  /**
   * Get a pointer to the next particle velocity.
   * Convenience method, giving the result:
   * GetNextEquationVariables() + 3;
   */
  inline double* GetNextVelocity() { return this->NextVelocity; }
  ///@}

  ///@{
  /**
   * Get a pointer to the previous user variables.
   * Convenience method, giving the result:
   * GetPrevEquationVariables() + 6;
   */
  inline double* GetPrevUserVariables() { return this->PrevUserVariables; }
  ///@}

  ///@{
  /**
   * Get a pointer to the user variables.
   * Convenience method, giving the result:
   * GetEquationVariables() + 6;
   */
  inline double* GetUserVariables() { return this->UserVariables; }
  ///@}

  ///@{
  /**
   * Get a pointer to the next user variables.
   * Convenience method, giving the result:
   * GetNextEquationVariables() + 6;
   */
  inline double* GetNextUserVariables() { return this->NextUserVariables; }
  ///@}

  ///@{
  /**
   * Get a reference to PrevTrackedUserData
   * See GetTrackedUserData for an explanation on how to use it.
   */
  inline std::vector<double>& GetPrevTrackedUserData() { return this->PrevTrackedUserData; }
  ///@}

  ///@{
  /**
   * Get a reference to TrackedUserData.
   * The tracked user data is a vector of double associated with each position of the particle,
   * but it is not integrated contrary to the UserVariables and EquationVariables.
   * It is, however, automatically tracked from one position to the next, copied when creating
   * new particles with NewInstance and CloneParticle and transferred from one node to the next
   * when particles move from one domain to the another in parallel.
   * If you are using these, you are supposed to compute and set the next tracked user data
   * your implementation of FunctionValues in your model.
   */
  inline std::vector<double>& GetTrackedUserData() { return this->TrackedUserData; }
  ///@}

  ///@{
  /**
   * Get a reference to NextTrackedUserData
   * See GetTrackedUserData for an explanation on how to use it.
   */
  inline std::vector<double>& GetNextTrackedUserData() { return this->NextTrackedUserData; }
  ///@}

  ///@{
  /**
   * Get/Set a pointer to a vtkLagrangianThreadedData that is considered to be local to the thread.
   * This structure contains multiple objects to be used by the tracker and the model, it also
   * contains a user data that can be used to store any kind of data, structure, class instance
   * that you may need. This is set by the vtkLagrangianParticleTracker and can be
   * initialized/finalized in the model
   */
  inline vtkLagrangianThreadedData* GetThreadedData() { return this->ThreadedData; }
  inline void SetThreadedData(vtkLagrangianThreadedData* threadedData)
  {
    this->ThreadedData = threadedData;
  }
  ///@}

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

  ///@{
  /**
   * Set/Get parent particle id.
   * Allow to find the seed particle of any particle.
   */
  virtual void SetParentId(vtkIdType parentId);
  virtual vtkIdType GetParentId();
  ///@}

  /**
   * Get the particle original seed index in the seed dataset.
   * Allows to track a specific seed along the tracks.
   */
  virtual vtkIdType GetSeedId();

  /**
   * Get the number of variables used to initialize EquationVariables.
   */
  virtual int GetNumberOfVariables();

  /**
   * Get the number of variables specific to the user.
   */
  virtual int GetNumberOfUserVariables();

  /**
   * Get the particle seed data, for reading only.
   */
  virtual vtkPointData* GetSeedData();

  /**
   * Get the index of the tuple for this particle in the point data
   * returned by GetSeedData method
   */
  virtual vtkIdType GetSeedArrayTupleIndex() const;

  /**
   * Get the last intersected surface cell id.
   */
  vtkIdType GetLastSurfaceCellId();

  /**
   * Get the dataset containing the last intersected surface cell
   */
  vtkDataSet* GetLastSurfaceDataSet();

  /**
   * Set the last surface dataset and last surface cell id
   */
  void SetLastSurfaceCell(vtkDataSet* dataset, vtkIdType cellId);

  /**
   * Get particle current number of steps.
   */
  virtual vtkIdType GetNumberOfSteps();

  ///@{
  /**
   * Set/Get particle termination.
   * Values out of enum range are accepted
   * Values < 100 are system reserved and should not be used
   */
  virtual void SetTermination(int termination);
  virtual int GetTermination();
  ///@}

  ///@{
  /**
   * Set/Get particle interaction.
   * Values out of enum range are accepted
   * Values < 100 are system reserved and should not be used
   */
  virtual void SetInteraction(int interaction);
  virtual int GetInteraction();
  ///@}

  ///@{
  /**
   * Set/Get user flag.
   */
  virtual void SetUserFlag(int flag);
  virtual int GetUserFlag();
  ///@}

  ///@{
  /**
   * Set/Get parallel specific flag, indication to insert or not
   * the previous position after streaming.
   * No effect in serial.
   */
  virtual void SetPInsertPreviousPosition(bool val);
  virtual bool GetPInsertPreviousPosition();
  ///@}

  ///@{
  /**
   * Set/Get parallel specific flag, indication that the particle
   * may be manually shifted after streaming.
   * No effect in serial.
   */
  virtual void SetPManualShift(bool val);
  virtual bool GetPManualShift();
  ///@}

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
   * Convenience setter for integration time,
   * do not use unless manual particle shifting
   * One using this method may want to consider
   * modifying EquationVariable[numVals] which
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
  vtkLagrangianParticle(const vtkLagrangianParticle&) = delete;
  vtkLagrangianParticle() = delete;
  void operator=(const vtkLagrangianParticle&) = delete;

  std::vector<double> PrevEquationVariables;
  double* PrevVelocity;
  double* PrevUserVariables;

  std::vector<double> EquationVariables;
  double* Velocity;
  double* UserVariables;

  std::vector<double> NextEquationVariables;
  double* NextVelocity;
  double* NextUserVariables;

  std::vector<double> PrevTrackedUserData;
  std::vector<double> TrackedUserData;
  std::vector<double> NextTrackedUserData;

  vtkLagrangianThreadedData* ThreadedData = nullptr;

  vtkIdType Id;
  vtkIdType ParentId;
  vtkIdType SeedId;
  vtkIdType NumberOfSteps;
  vtkIdType SeedArrayTupleIndex;
  vtkPointData* SeedData;

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

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkLagrangianParticle.h
