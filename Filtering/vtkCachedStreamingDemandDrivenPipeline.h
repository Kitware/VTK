/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCachedStreamingDemandDrivenPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCachedStreamingDemandDrivenPipeline -
// .SECTION Description
// vtkCachedStreamingDemandDrivenPipeline

#ifndef __vtkCachedStreamingDemandDrivenPipeline_h
#define __vtkCachedStreamingDemandDrivenPipeline_h

#include "vtkStreamingDemandDrivenPipeline.h"

class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkCachedStreamingDemandDrivenPipelineInternals;

class VTK_FILTERING_EXPORT vtkCachedStreamingDemandDrivenPipeline : 
  public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCachedStreamingDemandDrivenPipeline* New();
  vtkTypeRevisionMacro(vtkCachedStreamingDemandDrivenPipeline,
                       vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring the given algorithm's outputs up-to-date.  The algorithm
  // must already be managed by this executive.
  virtual int Update();
  virtual int Update(int port);
  virtual int Update(vtkAlgorithm* algorithm);
  virtual int Update(vtkAlgorithm* algorithm, int port);

  // Description:
  // This is the maximum number of images that can be retained in memory.
  // it defaults to 10.
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize, int);

protected:
  vtkCachedStreamingDemandDrivenPipeline();
  ~vtkCachedStreamingDemandDrivenPipeline();

  virtual int NeedToExecuteData(int outputPort);
  virtual int ExecuteData(int outputPort);
  
  int CacheSize;
  
  vtkDataObject **Data;
  unsigned long *Times;

private:
  vtkCachedStreamingDemandDrivenPipelineInternals* CachedStreamingDemandDrivenInternal;
private:
  vtkCachedStreamingDemandDrivenPipeline(const vtkCachedStreamingDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkCachedStreamingDemandDrivenPipeline&);  // Not implemented.
};

#endif
