/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSPFilterDefinition.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkDSPFilterDefinition - used by the Exodus readers
// .SECTION Description
// vtkDSPFilterDefinition is used by vtkExodusReader, vtkExodusIIReader and
// vtkPExodusReader to do temporal smoothing of data
// .SECTION See Also
// vtkDSPFilterGroup vtkExodusReader vtkExodusIIReader vtkPExodusReader

#ifndef __vtkDSPFilterDefinition_h
#define __vtkDSPFilterDefinition_h



#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkObject.h"

class vtkDSPFilterDefinitionVectorDoubleSTLCloak;
class vtkDSPFilterDefinitionStringSTLCloak;

class VTKFILTERSHYBRID_EXPORT vtkDSPFilterDefinition : public vtkObject
{
 public:
  vtkTypeMacro(vtkDSPFilterDefinition, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);
  static vtkDSPFilterDefinition *New();

 protected:
  vtkDSPFilterDefinition();
  vtkDSPFilterDefinition(vtkDSPFilterDefinition *other);
  ~vtkDSPFilterDefinition();

 public:
  void Copy(vtkDSPFilterDefinition *other);
  void Clear();
  bool IsThisInputVariableInstanceNeeded( int a_timestep, int a_outputTimestep );

  void PushBackNumeratorWeight(double a_value);
  void PushBackDenominatorWeight(double a_value);
  void PushBackForwardNumeratorWeight(double a_value);
  void SetInputVariableName(char *a_value);
  void SetOutputVariableName(char *a_value);
  const char *GetInputVariableName();
  const char *GetOutputVariableName();

  int GetNumNumeratorWeights();
  int GetNumDenominatorWeights();
  int GetNumForwardNumeratorWeights();

  double GetNumeratorWeight(int a_which);
  double GetDenominatorWeight(int a_which);
  double GetForwardNumeratorWeight(int a_which);


  vtkDSPFilterDefinitionVectorDoubleSTLCloak *NumeratorWeights;
  vtkDSPFilterDefinitionVectorDoubleSTLCloak *DenominatorWeights;
  vtkDSPFilterDefinitionVectorDoubleSTLCloak *ForwardNumeratorWeights;

  vtkDSPFilterDefinitionStringSTLCloak *InputVariableName;
  vtkDSPFilterDefinitionStringSTLCloak *OutputVariableName;

protected:

private:
  vtkDSPFilterDefinition(const vtkDSPFilterDefinition&); // Not implemented
  void operator=(const vtkDSPFilterDefinition&); // Not implemented
};



#endif
