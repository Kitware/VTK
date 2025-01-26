// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDSPFilterGroup.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDSPFilterDefinition.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringFormatter.h"
#include "vtkStructuredGrid.h"

#include <cctype>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDSPFilterGroup);

class vtkDSPFilterGroupVectorIntSTLCloak
{
public:
  std::vector<int> m_vector;
};
class vtkDSPFilterGroupVectorVectorIntSTLCloak
{
public:
  std::vector<std::vector<int>> m_vector;
};

class vtkDSPFilterGroupVectorArraySTLCloak
{
public:
  std::vector<vtkFloatArray*> m_vector;
};
class vtkDSPFilterGroupVectorVectorArraySTLCloak
{
public:
  std::vector<std::vector<vtkFloatArray*>> m_vector;
};
class vtkDSPFilterGroupVectorStringSTLCloak
{
public:
  std::vector<std::string> m_vector;
};

class vtkDSPFilterGroupVectorDefinitionSTLCloak
{
public:
  std::vector<vtkDSPFilterDefinition*> m_vector;
};

//------------------------------------------------------------------------------
vtkDSPFilterGroup::vtkDSPFilterGroup()
{
  this->FilterDefinitions = new vtkDSPFilterGroupVectorDefinitionSTLCloak;
  this->CachedInputs = new vtkDSPFilterGroupVectorArraySTLCloak;
  this->CachedInputNames = new vtkDSPFilterGroupVectorStringSTLCloak;
  this->CachedInputTimesteps = new vtkDSPFilterGroupVectorIntSTLCloak;
  this->CachedOutputs = new vtkDSPFilterGroupVectorVectorArraySTLCloak;
  this->CachedOutputTimesteps = new vtkDSPFilterGroupVectorVectorIntSTLCloak;

  this->FilterDefinitions->m_vector.resize(0);
  this->CachedInputs->m_vector.resize(0);
  this->CachedInputNames->m_vector.resize(0);
  this->CachedInputTimesteps->m_vector.resize(0);
  this->CachedOutputs->m_vector.resize(0);
  this->CachedOutputTimesteps->m_vector.resize(0);
}

//------------------------------------------------------------------------------
vtkDSPFilterGroup::~vtkDSPFilterGroup()
{
  this->FilterDefinitions->m_vector.resize(0);
  this->CachedInputs->m_vector.resize(0);
  this->CachedInputNames->m_vector.resize(0);
  this->CachedInputTimesteps->m_vector.resize(0);
  this->CachedOutputs->m_vector.resize(0);
  this->CachedOutputTimesteps->m_vector.resize(0);

  delete this->FilterDefinitions;
  delete this->CachedInputs;
  delete this->CachedInputNames;
  delete this->CachedInputTimesteps;
  delete this->CachedOutputs;
  delete this->CachedOutputTimesteps;
}

//------------------------------------------------------------------------------
void vtkDSPFilterGroup::AddFilter(vtkDSPFilterDefinition* filter)
{
  // XXX can't just add this filter, need to check for duplicates and removals?

  vtkDSPFilterDefinition* thefilter = vtkDSPFilterDefinition::New();
  thefilter->Copy(filter);

  this->FilterDefinitions->m_vector.push_back(thefilter);

  std::vector<vtkFloatArray*> l_cachedOutsForThisFilter;
  l_cachedOutsForThisFilter.resize(0);
  this->CachedOutputs->m_vector.push_back(l_cachedOutsForThisFilter);

  std::vector<int> l_cachedOutTimesForThisFilter;
  l_cachedOutTimesForThisFilter.resize(0);
  this->CachedOutputTimesteps->m_vector.push_back(l_cachedOutTimesForThisFilter);
}

//------------------------------------------------------------------------------
void vtkDSPFilterGroup::RemoveFilter(const char* a_outputVariableName)
{
  std::vector<vtkDSPFilterDefinition*>::iterator l_iter;
  std::vector<std::vector<vtkFloatArray*>>::iterator l_cachedOutputsIter =
    this->CachedOutputs->m_vector.begin();
  std::vector<std::vector<int>>::iterator l_cachedOutputTimesIter =
    this->CachedOutputTimesteps->m_vector.begin();

  for (l_iter = this->FilterDefinitions->m_vector.begin();
       l_iter != this->FilterDefinitions->m_vector.end(); ++l_iter)
  {
    if (!strcmp(a_outputVariableName, (*l_iter)->GetOutputVariableName()))
    {
      // this is the filter to delete
      this->FilterDefinitions->m_vector.erase(l_iter);
      if (l_cachedOutputsIter != this->CachedOutputs->m_vector.end())
        this->CachedOutputs->m_vector.erase(l_cachedOutputsIter);
      if (l_cachedOutputTimesIter != this->CachedOutputTimesteps->m_vector.end())
        this->CachedOutputTimesteps->m_vector.erase(l_cachedOutputTimesIter);
      break;
    }
    ++l_cachedOutputsIter;
    ++l_cachedOutputTimesIter;
  }
}

//------------------------------------------------------------------------------
void vtkDSPFilterGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
const char* vtkDSPFilterGroup::GetInputVariableName(int a_whichFilter)
{
  return this->FilterDefinitions->m_vector[a_whichFilter]->GetInputVariableName();
}
//------------------------------------------------------------------------------
bool vtkDSPFilterGroup::IsThisInputVariableInstanceNeeded(
  const char* a_name, int a_timestep, int a_outputTimestep)
{
  for (int i = 0; i < this->GetNumFilters(); i++)
  {
    if (!strcmp(this->FilterDefinitions->m_vector[i]->GetInputVariableName(), a_name))
    {
      if (this->FilterDefinitions->m_vector[i]->IsThisInputVariableInstanceNeeded(
            a_timestep, a_outputTimestep))
      {
        return (true);
      }
    }
  }
  return (false);
}
//------------------------------------------------------------------------------
bool vtkDSPFilterGroup::IsThisInputVariableInstanceCached(const char* a_name, int a_timestep)
{
  for (int i = 0; i < (int)this->CachedInputTimesteps->m_vector.size(); i++)
  {
    if (this->CachedInputTimesteps->m_vector[i] == a_timestep)
    {
      if (this->CachedInputNames->m_vector[i] == a_name)
      {
        return (true);
      }
    }
  }
  return (false);
}
//------------------------------------------------------------------------------
void vtkDSPFilterGroup::AddInputVariableInstance(
  const char* a_name, int a_timestep, vtkFloatArray* a_data)
{
  // This assumes that the instance is not already cached! perhaps should check anyway?

  this->CachedInputTimesteps->m_vector.push_back(a_timestep);
  this->CachedInputNames->m_vector.emplace_back(a_name);

  vtkFloatArray* l_array = vtkFloatArray::New();
  l_array->DeepCopy(a_data);
  this->CachedInputs->m_vector.push_back(l_array);
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkDSPFilterGroup::GetCachedInput(int a_whichFilter, int a_whichTimestep)
{
  std::string l_inputName =
    this->FilterDefinitions->m_vector[a_whichFilter]->GetInputVariableName();
  for (int i = 0; i < (int)this->CachedInputTimesteps->m_vector.size(); i++)
  {
    if (this->CachedInputTimesteps->m_vector[i] == a_whichTimestep)
    {
      if (this->CachedInputNames->m_vector[i] == l_inputName)
      {
        return (this->CachedInputs->m_vector[i]);
      }
    }
  }
  return (nullptr);
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkDSPFilterGroup::GetCachedOutput(int a_whichFilter, int a_whichTimestep)
{
  for (int i = 0; i < (int)this->CachedOutputs->m_vector[a_whichFilter].size(); i++)
  {
    if (a_whichTimestep == this->CachedOutputTimesteps->m_vector[a_whichFilter][i])
    {
      vtkFloatArray* l_tmp = (this->CachedOutputs->m_vector[a_whichFilter])[i];
      if (!strcmp(l_tmp->GetName(),
            this->FilterDefinitions->m_vector[a_whichFilter]->GetOutputVariableName()))
      {
        return (l_tmp);
      }
    }
  }

  return (nullptr);
}

//------------------------------------------------------------------------------
void vtkDSPFilterGroup::Copy(vtkDSPFilterGroup* other)
{
  this->FilterDefinitions->m_vector = other->FilterDefinitions->m_vector;
}

//------------------------------------------------------------------------------
int vtkDSPFilterGroup::GetNumFilters()
{
  return static_cast<int>(this->FilterDefinitions->m_vector.size());
}

//------------------------------------------------------------------------------
vtkDSPFilterDefinition* vtkDSPFilterGroup::GetFilter(int a_whichFilter)
{
  return this->FilterDefinitions->m_vector[a_whichFilter];
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkDSPFilterGroup::GetOutput(
  int a_whichFilter, int a_whichTimestep, int& a_instancesCalculated)
{
  int i, j, k;
  int l_numFilters = this->GetNumFilters();

  if ((int)this->CachedOutputs->m_vector.size() < l_numFilters)
  {
    // this shouldn't happen with saf. Should happen 1 time with exodus.

    int l_numNow = (int)this->CachedOutputs->m_vector.size();
    for (i = l_numNow; i < l_numFilters; i++)
    {
      std::vector<vtkFloatArray*> l_cachedOutsForThisFilter;
      l_cachedOutsForThisFilter.resize(0);
      this->CachedOutputs->m_vector.push_back(l_cachedOutsForThisFilter);

      std::vector<int> l_cachedOutTimesForThisFilter;
      l_cachedOutTimesForThisFilter.resize(0);
      this->CachedOutputTimesteps->m_vector.push_back(l_cachedOutTimesForThisFilter);
    }
  }

  // is this output array already cached?
  vtkFloatArray* l_tmp = this->GetCachedOutput(a_whichFilter, a_whichTimestep);
  if (l_tmp)
  {
    return (l_tmp);
  }

  vtkFloatArray* l_output = vtkFloatArray::New();
  l_output->SetName(FilterDefinitions->m_vector[a_whichFilter]->GetOutputVariableName());

  int l_numNumerators = FilterDefinitions->m_vector[a_whichFilter]->GetNumNumeratorWeights();
  int l_numForwardNumerators =
    FilterDefinitions->m_vector[a_whichFilter]->GetNumForwardNumeratorWeights();
  if (!l_numNumerators && !l_numForwardNumerators)
  {
    vtk::print("vtkDSPFilterGroup::GetOutput there are no numerator filter weights?\n");
    return (nullptr);
  }
  int l_numDenominators = FilterDefinitions->m_vector[a_whichFilter]->GetNumDenominatorWeights();

  double l_a1 = 1.0;
  if (l_numDenominators)
  {
    l_a1 = FilterDefinitions->m_vector[a_whichFilter]->GetDenominatorWeight(0);
  }

  // There should always be a valid input at the same time as an output
  vtkFloatArray* l_firstInput = this->GetCachedInput(a_whichFilter, a_whichTimestep);

  if (!l_firstInput)
  {
    vtk::print(
      "\n  vtkDSPFilterGroup::GetOutput error time {:d} has no input\n\n", a_whichTimestep);
    return (nullptr);
  }

  const int l_numEntries = l_firstInput->GetNumberOfTuples();
  const int l_numComponents = l_firstInput->GetNumberOfComponents();

  if (!l_numEntries || !l_numComponents)
  {
    vtk::print("\n  vtkDSPFilterGroup::GetOutput error time {:d}, l_numEntries={:d}, "
               "l_numComponents={:d}\n\n",
      a_whichTimestep, l_numEntries, l_numComponents);
    return (nullptr);
  }

  l_output->SetNumberOfComponents(l_numComponents);
  l_output->SetNumberOfTuples(l_numEntries);

  for (i = 0; i < l_numNumerators; i++)
  {
    int l_useThisTimestep = a_whichTimestep - i;
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetNumeratorWeight(i) / l_a1;

    // pre-time is considered infinite procession of input value at time 0
    l_useThisTimestep = std::max(l_useThisTimestep, 0);

    vtkFloatArray* l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);
    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    if (!i)
    {
      for (j = 0; j < l_numEntries * l_numComponents; j++)
        l_outPtr[i] = 0;
    }

    if (l_input)
    {
      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] += l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
    else
    {
      vtk::print("error vtkDSPFilterGroup::GetOutput can't get input {:d}\n", l_useThisTimestep);
    }
  }

  for (i = 1; i < l_numDenominators; i++)
  {
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetDenominatorWeight(i) / l_a1;

    if (a_whichTimestep - i < 0)
      break; // pre-time outputs are considered to be zero

    vtkFloatArray* l_input =
      this->GetOutput(a_whichFilter, a_whichTimestep - i, a_instancesCalculated);

    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    if (l_input)
    {
      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] -= l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
  }

  // Handle forward inputs
  for (i = 0; i < l_numForwardNumerators; i++)
  {
    int l_useThisTimestep = a_whichTimestep + i + 1;
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetForwardNumeratorWeight(i) / l_a1;

    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    vtkFloatArray* l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);

    while (!l_input && l_useThisTimestep >= 0)
    {
      // Try the timestep before: all post-time inputs are considered to be the same as the last
      // input
      l_useThisTimestep--;
      l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);
    }

    if (l_input)
    {
      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] += l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
    else
    {
      vtk::print(
        "\nerror vtkDSPFilterGroup::GetOutput can't get forward input {:d}\n\n", l_useThisTimestep);
    }
  }

  a_instancesCalculated++;

  this->CachedOutputs->m_vector[a_whichFilter].push_back(l_output);
  this->CachedOutputTimesteps->m_vector[a_whichFilter].push_back(a_whichTimestep);

  return (l_output);
}
VTK_ABI_NAMESPACE_END
