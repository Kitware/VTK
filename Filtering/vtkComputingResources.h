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

#ifndef __vtkComputingResources_h
#define __vtkComputingResources_h

#include "vtkObject.h"
#include "vtkThreadedStreamingTypes.i"

class vtkInformation;
class vtkProcessingUnitResource;
class vtkThreadedStreamingPipeline;

class vtkComputingResources : public vtkObject
{
public:
  static vtkComputingResources* New();
  vtkTypeRevisionMacro(vtkComputingResources,vtkObject);
  
  void Clear();
  void ObtainMinimumResources();
  void ObtainMaximumResources();
  vtkProcessingUnitResource *GetResourceFor(int processingUnit);
  void Deploy(vtkThreadedStreamingPipeline *exec, vtkInformation *info);
  

  bool Reserve(vtkComputingResources *res);
  void Collect(vtkComputingResources *res);

protected:
  vtkComputingResources();
  ~vtkComputingResources();

//BTX
  typedef vtksys::hash_map<int, vtkProcessingUnitResource*> ProcessingUnitToResourceHashMap;
  ProcessingUnitToResourceHashMap ResourceMap;
//ETX
private:
  vtkComputingResources(const vtkComputingResources&);  // Not implemented.
  void operator=(const vtkComputingResources&);  // Not implemented.
};

//BTX
class vtkProcessingUnitResource {
public:
  virtual  int ProcessingUnit() = 0;
  virtual bool HasResource() = 0;
  virtual void Clear() = 0;
  virtual void ObtainMinimum() = 0;
  virtual void ObtainMaximum() = 0;;
  virtual void IncreaseByRatio(float ratio, vtkProcessingUnitResource *refResource) = 0;
  virtual void AllocateFor(vtkThreadedStreamingPipeline *exec) = 0;
  virtual bool CanAccommodate(vtkProcessingUnitResource *refResource) = 0;
  virtual void Reserve(vtkProcessingUnitResource *refResource) = 0;
  virtual void Collect(vtkProcessingUnitResource *refResource) = 0;
};
//ETX

inline vtkProcessingUnitResource *vtkComputingResources::
GetResourceFor(int processingUnit)
{
  ProcessingUnitToResourceHashMap::iterator i = this->ResourceMap.find(processingUnit);
  if (i!=this->ResourceMap.end())
    return (*i).second;
  return NULL;
}

#endif
