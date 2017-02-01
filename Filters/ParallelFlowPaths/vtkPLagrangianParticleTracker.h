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
 * When a rank runs out of particle, it waits for other potentiel particles
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
#include "vtkSmartPointer.h" // for ivars

class MasterFlagManager;
class ParticleStreamManager;
class RankFlagManager;
class vtkMPIController;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;

class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPLagrangianParticleTracker :
public vtkLagrangianParticleTracker
{
public:
  vtkTypeMacro(vtkPLagrangianParticleTracker, vtkLagrangianParticleTracker);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkPLagrangianParticleTracker* New();

protected:
  vtkPLagrangianParticleTracker();
  ~vtkPLagrangianParticleTracker();

  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) VTK_OVERRIDE;

  void GenerateParticles(const vtkBoundingBox* bounds, vtkDataSet* seeds,
    vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes,
    vtkPointData* seedData, int nVar, std::queue<vtkLagrangianParticle*>& particles) VTK_OVERRIDE;

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
  virtual void GetParticleFeed(std::queue<vtkLagrangianParticle*>& particleQueue) VTK_OVERRIDE;
  virtual int Integrate(vtkLagrangianParticle*, std::queue<vtkLagrangianParticle*>& particleQueue,
    vtkPolyData* particlePathsOutput, vtkIdList* particlePathPointId,
    vtkDataObject* interactionOutput) VTK_OVERRIDE;

  void SendParticle(vtkLagrangianParticle* particle);
  void ReceiveParticles(std::queue<vtkLagrangianParticle*>& particleQueue);

  bool FinalizeOutputs(vtkPolyData* particlePathsOutput,
    vtkDataObject* interractionOutput) VTK_OVERRIDE;

  bool CheckParticlePathsRenderingThreshold(vtkPolyData* particlePathsOutput) VTK_OVERRIDE;

  void InitializeSurface(vtkDataObject*& surfaces) VTK_OVERRIDE;

  /**
   * Get an unique id for a particle
   */
  virtual vtkIdType GetNewParticleId() VTK_OVERRIDE;

  //@{
  /**
   * Get the complete number of created particles
   */
  vtkGetMacro(ParticleCounter, vtkIdType);
  //@}

  vtkSmartPointer<vtkUnstructuredGrid> TmpSurfaceInput;
  vtkSmartPointer<vtkMultiBlockDataSet> TmpSurfaceInputMB;
  vtkMPIController* Controller;
  ParticleStreamManager* StreamManager;
  MasterFlagManager* MFlagManager;
  RankFlagManager* RFlagManager;

private:
  vtkPLagrangianParticleTracker(const vtkPLagrangianParticleTracker&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkPLagrangianParticleTracker&) VTK_DELETE_FUNCTION;  // Not implemented.
};
#endif
