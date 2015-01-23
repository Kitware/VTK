/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleTracerBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME vtkParticleTracerBase - A parallel particle tracer for vector fields
// .SECTION Description
// vtkPParticleTracerBase is the base class for parallel filters that advect particles
// in a vector field. Note that the input vtkPointData structure must
// be identical on all datasets.
// .SECTION See Also
// vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
// vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkStreamTracer

#ifndef vtkPParticleTracerBase_h
#define vtkPParticleTracerBase_h

#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkParticleTracerBase.h"

//BTX
#include <vector> // STL Header
//ETX

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticleTracerBase : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkPParticleTracerBase,vtkParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the controller used when sending particles between processes
  // The controller must be an instance of vtkMPIController.
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  struct  RemoteParticleInfo
  {
    vtkParticleTracerBaseNamespace::ParticleInformation Current;
    vtkParticleTracerBaseNamespace::ParticleInformation Previous;
    vtkSmartPointer<vtkPointData> PreviousPD;
  };

  typedef std::vector<RemoteParticleInfo>  RemoteParticleVector;

  vtkPParticleTracerBase();
  ~vtkPParticleTracerBase();

  virtual int RequestUpdateExtent(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);

  //
  // Generate output
  //
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

//
//BTX

  virtual vtkPolyData* Execute(vtkInformationVector** inputVector);
  virtual bool SendParticleToAnotherProcess(vtkParticleTracerBaseNamespace::ParticleInformation & info,
                                            vtkParticleTracerBaseNamespace::ParticleInformation & previous,
                                            vtkPointData*);

  // Description : Before starting the particle trace, classify
  // all the injection/seed points according to which processor
  // they belong to. This saves us retesting at every injection time
  // providing 1) The volumes are static, 2) the seed points are static
  // If either are non static, then this step is skipped.
  virtual void AssignSeedsToProcessors(double time,
                                       vtkDataSet *source, int sourceID, int ptId,
                                       vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints,
                                       int &localAssignedCount);

  // Description : once seeds have been assigned to a process, we
  // give each one a uniqu ID. We need to use MPI to find out
  // who is using which numbers.
  virtual void AssignUniqueIds(
    vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints);

  // Description : Perform a GatherV operation on a vector of particles
  // this is used during classification of seed points and also between iterations
  // of the main loop as particles leave each processor domain
  virtual void SendReceiveParticles(RemoteParticleVector &outofdomain, RemoteParticleVector &received);

  void UpdateParticleListFromOtherProcesses();

  // Description:
  // Method that checks that the input arrays are ordered the
  // same on all data sets. This needs to be true for all
  // blocks in a composite data set as well as across all processes.
  virtual bool IsPointDataValid(vtkDataObject* input);


//
//ETX
//

  // MPI controller needed when running in parallel
  vtkMultiProcessController* Controller;

  // List used for transmitting between processors during parallel operation
  RemoteParticleVector MPISendList;

  RemoteParticleVector Tail; //this is to receive the "tails" of traces from other processes
 private:
  vtkPParticleTracerBase(const vtkPParticleTracerBase&);  // Not implemented.
  void operator=(const vtkPParticleTracerBase&);  // Not implemented.

};

#endif
