/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreGraphicsGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoreGraphicsGPUInfoList.h"

#include "vtkGPUInfoListArray.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <OpenGL/OpenGL.h>

vtkStandardNewMacro(vtkCoreGraphicsGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkCoreGraphicsGPUInfoList::Probe()
{
  if(!this->Probed)
  {
    this->Probed=true;
    this->Array=new vtkGPUInfoListArray;

    // Technique based on Apple QA1168
    // <https://developer.apple.com/library/mac/qa/qa1168/_index.html>

    // Get renderer info for all renderers that match the display mask.
    // Using a -1/0xFFFFFFFF display mask enables us to find all renderers,
    // including those GPUs that are not attached to monitors, aka. offline renderers.
    GLint count = 0;
    CGLRendererInfoObj infoObj = 0;
    CGLError error = CGLQueryRendererInfo(0xFFFFFFFF, &infoObj, &count);

    if ((error == kCGLNoError) && (count > 0))
    {
      for (GLint i = 0; i < count; i++)
      {
        GLint vramGL = 0;
        vtkTypeUInt64 vramVTK = 0;
        error = CGLDescribeRenderer(infoObj, i, kCGLRPVideoMemoryMegabytes, &vramGL);
        vramVTK = static_cast<vtkTypeUInt64>(vramGL) * 1024 * 1024;

        // The software renderer will return a video memory of 0, so ignore it.
        if ((error == kCGLNoError) && (vramVTK > 0))
        {
          vtkGPUInfo *info=vtkGPUInfo::New();
          info->SetDedicatedVideoMemory(vramVTK);

          this->Array->v.push_back(info);
        }
      }
    }
    else
    {
      this->Array->v.resize(0);
    }

    CGLDestroyRendererInfo(infoObj);
  }
  assert("post: probed" && this->IsProbed());
}

// ----------------------------------------------------------------------------
vtkCoreGraphicsGPUInfoList::vtkCoreGraphicsGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkCoreGraphicsGPUInfoList::~vtkCoreGraphicsGPUInfoList()
{
}

// ----------------------------------------------------------------------------
void vtkCoreGraphicsGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
