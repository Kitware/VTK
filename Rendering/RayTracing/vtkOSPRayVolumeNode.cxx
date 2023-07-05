// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayVolumeNode.h"

#include "vtkAbstractVolumeMapper.h"
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
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "RTWrapper/RTWrapper.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOSPRayVolumeNode);

//------------------------------------------------------------------------------
vtkOSPRayVolumeNode::vtkOSPRayVolumeNode() = default;

//------------------------------------------------------------------------------
vtkOSPRayVolumeNode::~vtkOSPRayVolumeNode() = default;

//------------------------------------------------------------------------------
void vtkOSPRayVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkOSPRayVolumeNode::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  vtkVolume* vol = (vtkVolume*)this->GetRenderable();
  if (!vol)
  {
    return mtime;
  }

  if (vol->GetMTime() > mtime)
  {
    mtime = vol->GetMTime();
  }
  if (vol->GetProperty())
  {
    mtime = std::max(mtime, vol->GetProperty()->GetMTime());
  }
  vtkAbstractVolumeMapper* mapper = vol->GetMapper();

  if (mapper)
  {
    vtkDataObject* dobj = mapper->GetDataSetInput();
    if (dobj)
    {
      mtime = std::max(mtime, dobj->GetMTime());
    }
    if (mapper->GetMTime() > mtime)
    {
      mtime = mapper->GetMTime();
    }
    if (mapper->GetInformation()->GetMTime() > mtime)
    {
      mtime = mapper->GetInformation()->GetMTime();
    }
  }
  return mtime;
}
VTK_ABI_NAMESPACE_END
