/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariProfiling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnariProfiling.h"

#ifdef USE_NVTX
#include <nvtx3/nvToolsExt.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkAnariProfiling::vtkAnariProfiling()
{
  this->StartProfiling("", vtkAnariProfiling::BROWN);
}

//----------------------------------------------------------------------------
vtkAnariProfiling::vtkAnariProfiling(const char* label, const uint32_t color)
{
  this->StartProfiling(label, color);
}

//----------------------------------------------------------------------------
vtkAnariProfiling::~vtkAnariProfiling()
{
  this->StopProfiling();
}

//----------------------------------------------------------------------------
void vtkAnariProfiling::StartProfiling(const char* label, const uint32_t color)
{
#ifdef USE_NVTX
  // Initialize
  nvtxEventAttributes_t eventAttrib = { 0 };
  eventAttrib.version = NVTX_VERSION;
  eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;

  // Configure attributes
  eventAttrib.colorType = NVTX_COLOR_ARGB;
  eventAttrib.color = color;
  eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
  eventAttrib.message.ascii = (label) ? label : "unknown";
  nvtxRangePushEx(&eventAttrib);
#endif
}

//----------------------------------------------------------------------------
void vtkAnariProfiling::StopProfiling()
{
#ifdef USE_NVTX
  nvtxRangePop();
#endif
}

VTK_ABI_NAMESPACE_END
