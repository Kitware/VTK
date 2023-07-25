// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDSPFilterGroup
 * @brief   used by the Exodus readers
 *
 * vtkDSPFilterGroup is used by vtkExodusReader, vtkExodusIIReader and
 * vtkPExodusReader to do temporal smoothing of data
 * @sa
 * vtkDSPFilterDefinition vtkExodusReader vtkExodusIIReader vtkPExodusReader
 */

#ifndef vtkDSPFilterGroup_h
#define vtkDSPFilterGroup_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDSPFilterGroupVectorIntSTLCloak;
class vtkDSPFilterGroupVectorVectorIntSTLCloak;
class vtkDSPFilterGroupVectorArraySTLCloak;
class vtkDSPFilterGroupVectorVectorArraySTLCloak;
class vtkDSPFilterGroupVectorStringSTLCloak;
class vtkDSPFilterGroupVectorDefinitionSTLCloak;
class vtkFloatArray;
class vtkDSPFilterDefinition;

class VTKFILTERSHYBRID_EXPORT vtkDSPFilterGroup : public vtkObject
{
public:
  static vtkDSPFilterGroup* New();
  vtkTypeMacro(vtkDSPFilterGroup, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void AddFilter(vtkDSPFilterDefinition* filter);
  void RemoveFilter(const char* a_outputVariableName);

  bool IsThisInputVariableInstanceNeeded(const char* a_name, int a_timestep, int a_outputTimestep);
  bool IsThisInputVariableInstanceCached(const char* a_name, int a_timestep);
  void AddInputVariableInstance(const char* a_name, int a_timestep, vtkFloatArray* a_data);

  vtkFloatArray* GetOutput(int a_whichFilter, int a_whichTimestep, int& a_instancesCalculated);

  vtkFloatArray* GetCachedInput(int a_whichFilter, int a_whichTimestep);
  vtkFloatArray* GetCachedOutput(int a_whichFilter, int a_whichTimestep);

  const char* GetInputVariableName(int a_whichFilter);

  int GetNumFilters();

  void Copy(vtkDSPFilterGroup* other);

  vtkDSPFilterDefinition* GetFilter(int a_whichFilter);

  vtkDSPFilterGroupVectorDefinitionSTLCloak* /*std::vector<vtkDSPFilterDefinition *>*/
    FilterDefinitions;

protected:
  vtkDSPFilterGroup();
  ~vtkDSPFilterGroup() override;

  vtkDSPFilterGroupVectorArraySTLCloak* /*std::vector<vtkFloatArray *>*/ CachedInputs;
  vtkDSPFilterGroupVectorStringSTLCloak* /*std::vector<std::string>*/ CachedInputNames;
  vtkDSPFilterGroupVectorIntSTLCloak* /*std::vector<int>*/ CachedInputTimesteps;

  vtkDSPFilterGroupVectorVectorArraySTLCloak* /*std::vector< std::vector<vtkFloatArray *> >*/
    CachedOutputs;
  vtkDSPFilterGroupVectorVectorIntSTLCloak* /*std::vector< std::vector<int> >*/
    CachedOutputTimesteps;

private:
  vtkDSPFilterGroup(const vtkDSPFilterGroup&) = delete;
  void operator=(const vtkDSPFilterGroup&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
