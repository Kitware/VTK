/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSPFilterDefinition.cxx

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

#include "vtkDSPFilterDefinition.h"
#include "vtkObjectFactory.h"

#include <stdlib.h>
#include <map>
#include <algorithm>
#include <vector>
#include <string>

vtkStandardNewMacro(vtkDSPFilterDefinition);

class vtkDSPFilterDefinitionVectorDoubleSTLCloak
{
public:
  std::vector<double> m_vector;
};
class vtkDSPFilterDefinitionStringSTLCloak
{
public:
  std::string m_string;
};


//----------------------------------------------------------------------------
vtkDSPFilterDefinition::vtkDSPFilterDefinition()
{
  //printf("    in vtkDSPFilterDefinition::vtkDSPFilterDefinition()\n");
  this->NumeratorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->ForwardNumeratorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->DenominatorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->InputVariableName = new vtkDSPFilterDefinitionStringSTLCloak;
  this->OutputVariableName = new vtkDSPFilterDefinitionStringSTLCloak;

  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string="";
  this->OutputVariableName->m_string="";
}
//----------------------------------------------------------------------------
vtkDSPFilterDefinition::vtkDSPFilterDefinition(vtkDSPFilterDefinition *other)
{
  //printf("    in vtkDSPFilterDefinition::vtkDSPFilterDefinition(vtkDSPFilterDefinition *other)\n");
  this->NumeratorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->ForwardNumeratorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->DenominatorWeights = new vtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->InputVariableName = new vtkDSPFilterDefinitionStringSTLCloak;
  this->OutputVariableName = new vtkDSPFilterDefinitionStringSTLCloak;

  this->NumeratorWeights->m_vector = other->NumeratorWeights->m_vector;
  this->ForwardNumeratorWeights->m_vector = other->ForwardNumeratorWeights->m_vector;
  this->DenominatorWeights->m_vector = other->DenominatorWeights->m_vector;
  this->InputVariableName->m_string = other->InputVariableName->m_string;
  this->OutputVariableName->m_string = other->OutputVariableName->m_string;
}
//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::Copy(vtkDSPFilterDefinition *other)
{
  //printf("    in vtkDSPFilterDefinition::Copy(vtkDSPFilterDefinition *other)\n");
  this->NumeratorWeights->m_vector = other->NumeratorWeights->m_vector;
  this->ForwardNumeratorWeights->m_vector = other->ForwardNumeratorWeights->m_vector;
  this->DenominatorWeights->m_vector = other->DenominatorWeights->m_vector;
  this->InputVariableName->m_string = other->InputVariableName->m_string;
  this->OutputVariableName->m_string = other->OutputVariableName->m_string;
}
//----------------------------------------------------------------------------
vtkDSPFilterDefinition::~vtkDSPFilterDefinition()
{
  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string="";
  this->OutputVariableName->m_string="";


  delete this->NumeratorWeights;
  delete this->ForwardNumeratorWeights;
  delete this->DenominatorWeights;
  delete this->InputVariableName;
  delete this->OutputVariableName;
}
//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::Clear()
{
  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string="";
  this->OutputVariableName->m_string="";
}
//----------------------------------------------------------------------------
bool vtkDSPFilterDefinition::IsThisInputVariableInstanceNeeded( int a_timestep, int a_outputTimestep )
{
  if(a_outputTimestep<a_timestep)
    {
    int l_index = a_timestep-a_outputTimestep;
    if( (int)(this->ForwardNumeratorWeights->m_vector.size())>=l_index )
      {
      //the filter does use this future input
      //printf("FILTER USES FUTURE INPUT %d for output %d\n",a_timestep,a_outputTimestep);
      return(true);
      }
    else
      {
      //future inputs not used for 1d filter
      //printf("FILTER doesn't use FUTURE INPUT %d for output %d\n",a_timestep,a_outputTimestep);
      return(false);
      }
    }
  if( this->DenominatorWeights->m_vector.size() > 1 )
    {
    //with an iir filter, all prev outputs since the beginning of time are used,
    //therefore all prev inputs are used as well
    return(true);
    }

  //For an fir filter, only a certain number of past inputs are needed
  if( a_timestep < a_outputTimestep-((int)(this->NumeratorWeights->m_vector.size())-1) )
    {
    //this input is too far in the past
    return(false);
    }
  return(true);
}


//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::PushBackNumeratorWeight(double a_value)
{
  this->NumeratorWeights->m_vector.push_back(a_value);
}
//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::PushBackDenominatorWeight(double a_value)
{
  this->DenominatorWeights->m_vector.push_back(a_value);
}
//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::PushBackForwardNumeratorWeight(double a_value)
{
  this->ForwardNumeratorWeights->m_vector.push_back(a_value);
}

//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::SetInputVariableName(char *a_value)
{
  this->InputVariableName->m_string = a_value;
}
//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::SetOutputVariableName(char *a_value)
{
  this->OutputVariableName->m_string = a_value;
}

//----------------------------------------------------------------------------
const char *vtkDSPFilterDefinition::GetInputVariableName()
{
  return this->InputVariableName->m_string.c_str();
}
//----------------------------------------------------------------------------
const char *vtkDSPFilterDefinition::GetOutputVariableName()
{
  return this->OutputVariableName->m_string.c_str();
}


//----------------------------------------------------------------------------
int vtkDSPFilterDefinition::GetNumNumeratorWeights()
{
  return static_cast<int>(this->NumeratorWeights->m_vector.size());
}
//----------------------------------------------------------------------------
int vtkDSPFilterDefinition::GetNumDenominatorWeights()
{
  return static_cast<int>(this->DenominatorWeights->m_vector.size());
}
//----------------------------------------------------------------------------
int vtkDSPFilterDefinition::GetNumForwardNumeratorWeights()
{
  return static_cast<int>(this->ForwardNumeratorWeights->m_vector.size());
}



//----------------------------------------------------------------------------
double vtkDSPFilterDefinition::GetNumeratorWeight(int a_which)
{
  return this->NumeratorWeights->m_vector[a_which];
}
//----------------------------------------------------------------------------
double vtkDSPFilterDefinition::GetDenominatorWeight(int a_which)
{
  return this->DenominatorWeights->m_vector[a_which];
}
//----------------------------------------------------------------------------
double vtkDSPFilterDefinition::GetForwardNumeratorWeight(int a_which)
{
  return this->ForwardNumeratorWeights->m_vector[a_which];
}


//----------------------------------------------------------------------------
void vtkDSPFilterDefinition::PrintSelf(ostream &os, vtkIndent indent)
{

  this->Superclass::PrintSelf( os, indent );
}
