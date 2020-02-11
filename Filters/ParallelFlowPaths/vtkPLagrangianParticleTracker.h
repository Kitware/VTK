/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPLagrangianParticleTracker
 * @brief    parallel Lagrangian particle tracker
 *
 * This class implements parallel Lagrangian particle tracker.
 * The implementation is as follows:
 * First seeds input is parsed to create particle in each rank
 * Particles which are not contained by the flow in a rank are sent to other ranks
 * which can potentially contain it and will grab only if they actually contain it
 * Then each rank begin integrating.
 * When a particle goes out of domain, the particle will be sent to other ranks
 * the same way.
 * When a rank runs out of particle, it waits for other potential particles
 * from other ranks.
 * When all ranks run out of particles, integration is over.
 * The master rank takes care of communications between rank regarding integration termination
 * particles are directly streamed rank to rank, without going through the master
 *
 * @sa
 * vtkStreamTracer
 */

#ifndef vtkPLagrangianParticleTracker_h
#define vtkPLagrangianParticleTracker_h

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
#include "vtkLagrangianParticleTracker.h"
#include "vtkNew.h" // for ivars

class MasterFlagManager;
class ParticleStreamManager;
class RankFlagManager;
class vtkMPIController;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPLagrangianParticleTracker
  : public vtkLagrangianParticleTracker
{
public:
  vtkTypeMacro(vtkPLagrangianParticleTracker, vtkLagrangianParticleTracker);
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPLagrangianParticleTracker* New();

protected:
  vtkPLagrangianParticleTracker();
  ~vtkPLagrangianParticleTracker() override;

  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void GenerateParticles(const vtkBoundingBox* bounds, vtkDataSet* seeds,
    vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes, vtkPointData* seedData,
    int nVar, std::queue<vtkLagrangianParticle*>& particles) override;

  /**
   * Flags description :
   * Worker flag working : the worker has at least one particle in it's queue and
      is currently integrating it.
   * Worker flag empty : the worker has no more particle in it's queue and is
       activelly waiting for more particle to integrate from other ranks.
   * Worker flag finished : the worker has received a master empty flag and after
       checking one last time, still doesn't have any particle to integrate. It is
       now just waiting for master to send the master finished flag.
   * Master flag working : there is at least one worker or the master that have one
       or more particle to integrate.
   * Master flag empty : all ranks, including master, have no more particles to integrate
   * Master flag finished : all workers ranks have sent the worker flag finished
   */
  virtual void GetParticleFeed(std::queue<vtkLagrangianParticle*>& particleQueue) override;
  virtual int Integrate(vtkInitialValueProblemSolver* integrator, vtkLagrangianParticle*,
    std::queue<vtkLagrangianParticle*>& particleQueue, vtkPolyData* particlePathsOutput,
    vtkPolyLine* particlePath, vtkDataObject* interactionOutput) override;

  //@{
  /**
   * Non threadsafe methods to send and receive particles
   */
  void SendParticle(vtkLagrangianParticle* particle);
  void ReceiveParticles(std::queue<vtkLagrangianParticle*>& particleQueue);
  //@}

  bool FinalizeOutputs(vtkPolyData* particlePathsOutput, vtkDataObject* interactionOutput) override;

  bool UpdateSurfaceCacheIfNeeded(vtkDataObject*& surfaces) override;

  /**
   * Get an unique id for a particle
   * This method is thread safe
   */
  virtual vtkIdType GetNewParticleId() override;

  /**
   * Get the complete number of created particles
   */
  vtkGetMacro(ParticleCounter, vtkIdType);

  vtkNew<vtkUnstructuredGrid> TmpSurfaceInput;
  vtkNew<vtkMultiBlockDataSet> TmpSurfaceInputMB;
  vtkMPIController* Controller;
  ParticleStreamManager* StreamManager;
  MasterFlagManager* MFlagManager;
  RankFlagManager* RFlagManager;

  std::mutex StreamManagerMutex;

private:
  vtkPLagrangianParticleTracker(const vtkPLagrangianParticleTracker&) = delete;
  void operator=(const vtkPLagrangianParticleTracker&) = delete;
};
#endif
