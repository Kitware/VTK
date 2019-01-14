/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXActorNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOptiXActorNode.h"

#include "vtkActor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyData.h"
#include "vtkViewNodeCollection.h"

vtkInformationKeyMacro(vtkOptiXActorNode, ENABLE_SCALING, Integer);
vtkInformationKeyMacro(vtkOptiXActorNode, SCALE_ARRAY_NAME, String);
vtkInformationKeyMacro(vtkOptiXActorNode, SCALE_FUNCTION, ObjectBase);

//============================================================================
vtkStandardNewMacro(vtkOptiXActorNode);

//------------------------------------------------------------------------------
vtkOptiXActorNode::vtkOptiXActorNode()
  : LastUsedMapper(nullptr)
{
}

//------------------------------------------------------------------------------
vtkOptiXActorNode::~vtkOptiXActorNode()
{
}

//------------------------------------------------------------------------------
void vtkOptiXActorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOptiXActorNode::SetEnableScaling(int value, vtkActor *actor)
{
  if (!actor)
  {
    return;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *info = mapper->GetInformation();
    info->Set(vtkOptiXActorNode::ENABLE_SCALING(), value);
  }
}

//------------------------------------------------------------------------------
int vtkOptiXActorNode::GetEnableScaling(vtkActor *actor)
{
  if (!actor)
  {
    return 0;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *info = mapper->GetInformation();
    if (info && info->Has(vtkOptiXActorNode::ENABLE_SCALING()))
    {
      return (info->Get(vtkOptiXActorNode::ENABLE_SCALING()));
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkOptiXActorNode::SetScaleArrayName
  (const char *arrayName, vtkActor *actor)
{
  if (!actor)
  {
    return;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *mapperInfo = mapper->GetInformation();
    mapperInfo->Set(vtkOptiXActorNode::SCALE_ARRAY_NAME(), arrayName);
  }
}

//------------------------------------------------------------------------------
void vtkOptiXActorNode::SetScaleFunction(vtkPiecewiseFunction *scaleFunction,
                                          vtkActor *actor)
{
  if (!actor)
  {
    return;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *mapperInfo = mapper->GetInformation();
    mapperInfo->Set(vtkOptiXActorNode::SCALE_FUNCTION(), scaleFunction);
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkOptiXActorNode::GetMTime()
{
  vtkActor *act = (vtkActor*)this->GetRenderable();
  vtkMapper *mapper = act->GetMapper();
  if (LastUsedMapper != mapper)
  {
    LastUsedMapper = mapper;
    this->Modified();
  }

  vtkMTimeType mtime = this->Superclass::GetMTime();
  if (act->GetMTime() > mtime)
  {
    mtime = act->GetMTime();
  }
  vtkDataObject * dobj = nullptr;
  vtkPolyData *poly = nullptr;
  if (mapper)
  {
    //if (act->GetRedrawMTime() > mtime)
    //  {
    //  mtime = act->GetRedrawMTime();
    // }
    if (mapper->GetMTime() > mtime)
    {
      mtime = mapper->GetMTime();
    }
    if (mapper->GetInformation()->GetMTime() > mtime)
    {
      mtime = mapper->GetInformation()->GetMTime();
    }
    vtkPiecewiseFunction *pwf = vtkPiecewiseFunction::SafeDownCast
      (mapper->GetInformation()->Get(vtkOptiXActorNode::SCALE_FUNCTION()));
    if (pwf)
    {
      if (pwf->GetMTime() > mtime)
      {
        mtime = pwf->GetMTime();
      }
    }
    dobj = mapper->GetInputDataObject(0, 0);
    poly = vtkPolyData::SafeDownCast(dobj);
  }
  if (poly)
  {
    if (poly->GetMTime() > mtime)
    {
      mtime = poly->GetMTime();
    }
  }
  else if (dobj)
  {
    vtkCompositeDataSet *comp = vtkCompositeDataSet::SafeDownCast
      (dobj);
    if (comp)
    {
      vtkCompositeDataIterator*dit = comp->NewIterator();
      dit->SkipEmptyNodesOn();
      while(!dit->IsDoneWithTraversal())
      {
        poly = vtkPolyData::SafeDownCast(comp->GetDataSet(dit));
        if (poly)
        {
          if (poly->GetMTime() > mtime)
          {
            mtime = poly->GetMTime();
          }
        }
        dit->GoToNextItem();
      }
      dit->Delete();
    }
  }
  return mtime;
}
