// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariVolumeNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariVolumeNode);

//----------------------------------------------------------------------------
void vtkAnariVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkAnariVolumeNode::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  vtkVolume* vol = (vtkVolume*)this->GetRenderable();
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
