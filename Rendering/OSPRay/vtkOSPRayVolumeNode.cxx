/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayVolumeNode.h"

#include "vtkActor.h"
#include "vtkVolume.h"
#include "vtkAbstractVolumeMapper.h"
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

//============================================================================
vtkStandardNewMacro(vtkOSPRayVolumeNode);

//----------------------------------------------------------------------------
vtkOSPRayVolumeNode::vtkOSPRayVolumeNode()
{
}

//----------------------------------------------------------------------------
vtkOSPRayVolumeNode::~vtkOSPRayVolumeNode()
{
}

//----------------------------------------------------------------------------
void vtkOSPRayVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
unsigned long vtkOSPRayVolumeNode::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  vtkVolume *vol = (vtkVolume*)this->GetRenderable();
  if (vol->GetMTime() > mtime)
    {
    mtime = vol->GetMTime();
    }
  // vtkDataObject * dobj = NULL;
  // vtkPolyData *poly = NULL;
  vtkAbstractVolumeMapper *mapper = vol->GetMapper();
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
    // dobj = mapper->GetInputDataObject(0, 0);
    // poly = vtkPolyData::SafeDownCast(dobj);
    }

    //Carson: TODO: check datasets time

  // if (poly)
  //   {
  //   if (poly->GetMTime() > mtime)
  //     {
  //     mtime = poly->GetMTime();
  //     }
  //   }
  // else if (dobj)
  //   {
  //   vtkCompositeDataSet *comp = vtkCompositeDataSet::SafeDownCast
  //     (dobj);
  //   if (comp)
  //     {
  //     vtkCompositeDataIterator*dit = comp->NewIterator();
  //     dit->SkipEmptyNodesOn();
  //     while(!dit->IsDoneWithTraversal())
  //       {
  //       poly = vtkPolyData::SafeDownCast(comp->GetDataSet(dit));
  //       if (poly)
  //         {
  //         if (poly->GetMTime() > mtime)
  //           {
  //           mtime = poly->GetMTime();
  //           }
  //         }
  //       dit->GoToNextItem();
  //       }
  //     dit->Delete();
  //     }
  //   }
  return mtime;
}
