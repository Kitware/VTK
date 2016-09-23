/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayActorNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayActorNode.h"

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

#include "ospray/ospray.h"

vtkInformationKeyMacro(vtkOSPRayActorNode, ENABLE_SCALING, Integer);
vtkInformationKeyMacro(vtkOSPRayActorNode, SCALE_ARRAY_NAME, String);
vtkInformationKeyMacro(vtkOSPRayActorNode, SCALE_FUNCTION, ObjectBase);

//============================================================================
vtkStandardNewMacro(vtkOSPRayActorNode);

//----------------------------------------------------------------------------
vtkOSPRayActorNode::vtkOSPRayActorNode()
{
}

//----------------------------------------------------------------------------
vtkOSPRayActorNode::~vtkOSPRayActorNode()
{
}

//----------------------------------------------------------------------------
void vtkOSPRayActorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayActorNode::SetEnableScaling(int value, vtkActor *actor)
{
  if (!actor)
  {
    return;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *info = mapper->GetInformation();
    info->Set(vtkOSPRayActorNode::ENABLE_SCALING(), value);
  }
}

//----------------------------------------------------------------------------
int vtkOSPRayActorNode::GetEnableScaling(vtkActor *actor)
{
  if (!actor)
  {
    return 0;
  }
  vtkMapper *mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation *info = mapper->GetInformation();
    if (info && info->Has(vtkOSPRayActorNode::ENABLE_SCALING()))
    {
      return (info->Get(vtkOSPRayActorNode::ENABLE_SCALING()));
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayActorNode::SetScaleArrayName
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
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(), arrayName);
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayActorNode::SetScaleFunction(vtkPiecewiseFunction *scaleFunction,
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
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_FUNCTION(), scaleFunction);
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkOSPRayActorNode::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  vtkActor *act = (vtkActor*)this->GetRenderable();
  if (act->GetMTime() > mtime)
  {
    mtime = act->GetMTime();
  }
  vtkDataObject * dobj = NULL;
  vtkPolyData *poly = NULL;
  vtkMapper *mapper = act->GetMapper();
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
      (mapper->GetInformation()->Get(vtkOSPRayActorNode::SCALE_FUNCTION()));
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
