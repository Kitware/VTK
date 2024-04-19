// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDSPFilterDefinition
 * @brief   used by the Exodus readers
 *
 * vtkDSPFilterDefinition is used by vtkExodusReader, vtkExodusIIReader and
 * vtkPExodusReader to do temporal smoothing of data
 * @sa
 * vtkDSPFilterGroup vtkExodusReader vtkExodusIIReader vtkPExodusReader
 */

#ifndef vtkDSPFilterDefinition_h
#define vtkDSPFilterDefinition_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDSPFilterDefinitionVectorDoubleSTLCloak;
class vtkDSPFilterDefinitionStringSTLCloak;

class VTKFILTERSHYBRID_EXPORT vtkDSPFilterDefinition : public vtkObject
{
public:
  vtkTypeMacro(vtkDSPFilterDefinition, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDSPFilterDefinition* New();

protected:
  vtkDSPFilterDefinition();
  vtkDSPFilterDefinition(vtkDSPFilterDefinition* other);
  ~vtkDSPFilterDefinition() override;

public:
  void Copy(vtkDSPFilterDefinition* other);
  void Clear();
  bool IsThisInputVariableInstanceNeeded(int a_timestep, int a_outputTimestep);

  void PushBackNumeratorWeight(double a_value);
  void PushBackDenominatorWeight(double a_value);
  void PushBackForwardNumeratorWeight(double a_value);
  void SetInputVariableName(const char* a_value);
  void SetOutputVariableName(const char* a_value);
  const char* GetInputVariableName();
  const char* GetOutputVariableName();

  int GetNumNumeratorWeights();
  int GetNumDenominatorWeights();
  int GetNumForwardNumeratorWeights();

  double GetNumeratorWeight(int a_which);
  double GetDenominatorWeight(int a_which);
  double GetForwardNumeratorWeight(int a_which);

  vtkDSPFilterDefinitionVectorDoubleSTLCloak* NumeratorWeights;
  vtkDSPFilterDefinitionVectorDoubleSTLCloak* DenominatorWeights;
  vtkDSPFilterDefinitionVectorDoubleSTLCloak* ForwardNumeratorWeights;

  vtkDSPFilterDefinitionStringSTLCloak* InputVariableName;
  vtkDSPFilterDefinitionStringSTLCloak* OutputVariableName;

protected:
private:
  vtkDSPFilterDefinition(const vtkDSPFilterDefinition&) = delete;
  void operator=(const vtkDSPFilterDefinition&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
