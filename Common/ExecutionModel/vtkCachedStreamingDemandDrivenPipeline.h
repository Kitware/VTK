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
/**
 * @class   vtkCachedStreamingDemandDrivenPipeline
 *
 * vtkCachedStreamingDemandDrivenPipeline
*/

#ifndef vtkCachedStreamingDemandDrivenPipeline_h
#define vtkCachedStreamingDemandDrivenPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkStreamingDemandDrivenPipeline.h"

class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkCachedStreamingDemandDrivenPipeline :
  public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCachedStreamingDemandDrivenPipeline* New();
  vtkTypeMacro(vtkCachedStreamingDemandDrivenPipeline,
                       vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * This is the maximum number of images that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize, int);
  //@}

protected:
  vtkCachedStreamingDemandDrivenPipeline();
  ~vtkCachedStreamingDemandDrivenPipeline() override;

  int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec) override;
  int ExecuteData(vtkInformation* request,
                          vtkInformationVector** inInfoVec,
                          vtkInformationVector* outInfoVec) override;

  int CacheSize;

  vtkDataObject **Data;
  vtkMTimeType *Times;

private:
  vtkCachedStreamingDemandDrivenPipeline(const vtkCachedStreamingDemandDrivenPipeline&) = delete;
  void operator=(const vtkCachedStreamingDemandDrivenPipeline&) = delete;
};

#endif
