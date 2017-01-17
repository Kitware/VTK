/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkForceTime_h
#define vtkForceTime_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSHYBRID_EXPORT vtkForceTime : public vtkPassInputTypeAlgorithm
{
public :
  static vtkForceTime* New();
  vtkTypeMacro(vtkForceTime, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Replace the pipeline time by this one.
  vtkSetMacro(ForcedTime, double);
  vtkGetMacro(ForcedTime, double);

  // Description:
  // Use the ForcedTime. If disabled, use usual pipeline time.
  vtkSetMacro(IgnorePipelineTime, bool);
  vtkGetMacro(IgnorePipelineTime, bool);
  vtkBooleanMacro(IgnorePipelineTime, bool);

protected:
  vtkForceTime();
  ~vtkForceTime() VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) VTK_OVERRIDE;

  int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkForceTime(const vtkForceTime&) VTK_DELETE_FUNCTION;
  void operator=(const vtkForceTime&) VTK_DELETE_FUNCTION;

  double ForcedTime;
  bool IgnorePipelineTime;
  double PipelineTime;
  bool PipelineTimeFlag;
  vtkDataObject* Cache;
};

#endif //vtkForceTime_h
