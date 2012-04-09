/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputingResources.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2008, 2009 by SCI Institute, University of Utah.
  
  This is part of the Parallel Dataflow System originally developed by
  Huy T. Vo and Claudio T. Silva. For more information, see:

  "Parallel Dataflow Scheme for Streaming (Un)Structured Data" by Huy
  T. Vo, Daniel K. Osmari, Brian Summa, Joao L.D. Comba, Valerio
  Pascucci and Claudio T. Silva, SCI Institute, University of Utah,
  Technical Report #UUSCI-2009-004, 2009.

  "Multi-Threaded Streaming Pipeline For VTK" by Huy T. Vo and Claudio
  T. Silva, SCI Institute, University of Utah, Technical Report
  #UUSCI-2009-005, 2009.
  -------------------------------------------------------------------------*/
// .NAME vtkComputingResources - Definition of computing resource
// (threads/kernels)
// .SECTION Description
// This is a class for distribute the number of threads to a network of modules

// .SECTION See Also
// vtkExecutionScheduler

#ifndef __vtkComputingResources_h
#define __vtkComputingResources_h

#include "vtkObject.h"

class vtkInformation;
class vtkProcessingUnitResource;
class vtkThreadedStreamingPipeline;

class VTK_FILTERING_EXPORT vtkComputingResources : public vtkObject 
{
public:
  static vtkComputingResources* New();
  vtkTypeMacro(vtkComputingResources,vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set resources to an empty states
  void Clear();

  // Description:
  // Assign a minimum amount of usable resources to this object,
  // e.g. 1 thread
  void ObtainMinimumResources();

  // Description:
  // Assign a maximum amount of usable resources to this object
  void ObtainMaximumResources();

  //BTX
  // Description:
  // Return the resources of a specific type of processing unit that
  // is hold in this object
  vtkProcessingUnitResource *GetResourceFor(int processingUnit);
  //ETX

  // Description:
  // Assign the resources and information of this object to an
  // executive, i.e., set the number of threads of the algorithm the
  // executive is pointing to
  void Deploy(vtkThreadedStreamingPipeline *exec, vtkInformation *info);
  
  // Description:
  // Take an amount of computing resources out of this object. Return
  // true if it is successful.
  bool Reserve(vtkComputingResources *res);

  // Description:
  // Add an amount of computing resources to this object
  void Collect(vtkComputingResources *res);

protected:
  vtkComputingResources();
  ~vtkComputingResources();

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
private:
  vtkComputingResources(const vtkComputingResources&);  // Not implemented.
  void operator=(const vtkComputingResources&);  // Not implemented.
};

//BTX
//----------------------------------------------------------------------------
// A basic resource class. It is put here for later inheritance for
// any type of computing, e.g. CPU/GPU.
//----------------------------------------------------------------------------
class vtkProcessingUnitResource {
public:
  virtual ~vtkProcessingUnitResource() {}

  // Description:
  // Return the type of unit this computing resource is holding
  virtual int ProcessingUnit() = 0;

  // Description:
  // Return true if this resource is not empty
  virtual bool HasResource() = 0;

  // Description:
  // Make this resource empty
  virtual void Clear() = 0;

  // Description:
  // Give this object a minimum amount of resource it can allocate
  virtual void ObtainMinimum() = 0;

  // Description:
  // Give this object a maximum amount of resource it can allocate
  virtual void ObtainMaximum() = 0;

  // Description:
  // Given a ratio and a resource, increase this resource by a ratio
  // of the reference resource. This is the basic function for
  // resource distributing
  virtual void IncreaseByRatio(float ratio, vtkProcessingUnitResource *refResource) = 0;

  // Description:
  // This actually set the amount of resource on the algorithm holding
  // by the input executive
  virtual void AllocateFor(vtkThreadedStreamingPipeline *exec) = 0;

  // Description:
  // Return true if this object can allocate at least refResource
  virtual bool CanAccommodate(vtkProcessingUnitResource *refResource) = 0;

  // Description:
  // Reserve an amount of resource given by refResource from this object
  virtual void Reserve(vtkProcessingUnitResource *refResource) = 0;

  // Description:
  // Add an amount of resource given by refResource to this object
  virtual void Collect(vtkProcessingUnitResource *refResource) = 0;
};
//ETX

#endif
