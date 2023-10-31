// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariActorNode.h"
#include "vtkAnariProfiling.h"

#include "vtkActor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkTimeStamp.h"

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariActorNode);

//============================================================================
vtkInformationKeyMacro(vtkAnariActorNode, LUMINOSITY, Double);
vtkInformationKeyMacro(vtkAnariActorNode, ENABLE_SCALING, Integer);
vtkInformationKeyMacro(vtkAnariActorNode, SCALE_ARRAY_NAME, String);
vtkInformationKeyMacro(vtkAnariActorNode, SCALE_FUNCTION, ObjectBase);

//----------------------------------------------------------------------------
vtkAnariActorNode::vtkAnariActorNode()
  : LastMapper(nullptr)
  , MapperChangedTime()
{
}

//----------------------------------------------------------------------------
void vtkAnariActorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariActorNode::SetEnableScaling(int value, vtkActor* actor)
{
  if (!actor)
  {
    return;
  }

  vtkMapper* mapper = actor->GetMapper();

  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();
    info->Set(vtkAnariActorNode::ENABLE_SCALING(), value);
  }
}

//----------------------------------------------------------------------------
int vtkAnariActorNode::GetEnableScaling(vtkActor* actor)
{
  if (!actor)
  {
    return 0;
  }

  vtkMapper* mapper = actor->GetMapper();

  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();

    if (info && info->Has(vtkAnariActorNode::ENABLE_SCALING()))
    {
      return (info->Get(vtkAnariActorNode::ENABLE_SCALING()));
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariActorNode::SetScaleArrayName(const char* arrayName, vtkActor* actor)
{
  if (!actor)
  {
    return;
  }

  vtkMapper* mapper = actor->GetMapper();

  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();
    info->Set(vtkAnariActorNode::SCALE_ARRAY_NAME(), arrayName);
  }
}

//----------------------------------------------------------------------------
const char* vtkAnariActorNode::GetScaleArrayName(vtkActor* actor)
{
  if (!actor)
  {
    return nullptr;
  }

  vtkMapper* mapper = actor->GetMapper();

  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();

    if (info && info->Has(vtkAnariActorNode::SCALE_ARRAY_NAME()))
    {
      return (info->Get(vtkAnariActorNode::SCALE_ARRAY_NAME()));
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariActorNode::SetScaleFunction(vtkPiecewiseFunction* scaleFunction, vtkActor* actor)
{
  if (!actor)
  {
    return;
  }
  vtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();
    info->Set(vtkAnariActorNode::SCALE_FUNCTION(), scaleFunction);
  }
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkAnariActorNode::GetScaleFunction(vtkActor* actor)
{
  if (!actor)
  {
    return nullptr;
  }

  vtkMapper* mapper = actor->GetMapper();

  if (mapper)
  {
    vtkInformation* info = mapper->GetInformation();

    if (info && info->Has(vtkAnariActorNode::SCALE_FUNCTION()))
    {
      vtkPiecewiseFunction* pwf =
        vtkPiecewiseFunction::SafeDownCast(info->Get(vtkAnariActorNode::SCALE_FUNCTION()));
      return pwf;
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariActorNode::SetLuminosity(double value, vtkProperty* property)
{
  if (!property)
  {
    return;
  }
  vtkInformation* info = property->GetInformation();
  info->Set(vtkAnariActorNode::LUMINOSITY(), value);
}

//----------------------------------------------------------------------------
double vtkAnariActorNode::GetLuminosity(vtkProperty* property)
{
  if (!property)
  {
    return 0.0;
  }
  vtkInformation* info = property->GetInformation();
  if (info && info->Has(vtkAnariActorNode::LUMINOSITY()))
  {
    double retval = info->Get(vtkAnariActorNode::LUMINOSITY());
    return retval;
  }
  return 0.0;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkAnariActorNode::GetMTime()
{
  vtkAnariProfiling startProfiling("vtkAnariActorNode::GetMTime", vtkAnariProfiling::BROWN);

  vtkMTimeType mtime = this->Superclass::GetMTime();
  vtkActor* act = (vtkActor*)this->GetRenderable();

  if (!act)
  {
    return mtime;
  }

  if (act->GetMTime() > mtime)
  {
    mtime = act->GetMTime();
  }

  if (vtkProperty* prop = act->GetProperty())
  {
    if (prop->GetMTime() > mtime)
    {
      mtime = prop->GetMTime();
    }

    if (prop->GetInformation()->GetMTime() > mtime)
    {
      mtime = prop->GetInformation()->GetMTime();
    }
  }

  vtkDataObject* dobj = nullptr;
  vtkPolyData* poly = nullptr;
  vtkMapper* mapper = act->GetMapper();

  if (mapper)
  {
    if (mapper->GetMTime() > mtime)
    {
      mtime = mapper->GetMTime();
    }
    if (mapper->GetInformation()->GetMTime() > mtime)
    {
      mtime = mapper->GetInformation()->GetMTime();
    }
    if (mapper != this->LastMapper)
    {
      this->MapperChangedTime.Modified();
      mtime = this->MapperChangedTime;
      this->LastMapper = mapper;
    }
    vtkPiecewiseFunction* pwf = vtkPiecewiseFunction::SafeDownCast(
      mapper->GetInformation()->Get(vtkAnariActorNode::SCALE_FUNCTION()));
    if (pwf)
    {
      if (pwf->GetMTime() > mtime)
      {
        mtime = pwf->GetMTime();
      }
    }

    if (mapper->GetNumberOfInputPorts() > 0)
    {
      dobj = mapper->GetInputDataObject(0, 0);
      poly = vtkPolyData::SafeDownCast(dobj);
    }
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
    vtkCompositeDataSet* comp = vtkCompositeDataSet::SafeDownCast(dobj);
    if (comp)
    {
      vtkCompositeDataIterator* dit = comp->NewIterator();
      dit->SkipEmptyNodesOn();
      while (!dit->IsDoneWithTraversal())
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

  vtkTexture* texture = act->GetTexture();

  if (texture)
  {
    if (texture->GetMTime() > mtime)
    {
      mtime = texture->GetMTime();
    }

    if (texture->GetInput() && texture->GetInput()->GetMTime() > mtime)
    {
      mtime = texture->GetInput()->GetMTime();
    }
  }

  return mtime;
}

VTK_ABI_NAMESPACE_END
