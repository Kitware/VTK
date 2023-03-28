/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUHardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPUHardwareSelector.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

//#define vtkWebGPUHardwareSelectorDEBUG
#ifdef vtkWebGPUHardwareSelectorDEBUG
#include "vtkImageImport.h"
#include "vtkNew.h"
#include "vtkPNMWriter.h"
#include "vtkWindows.h" // OK on UNix etc
#include <sstream>
#endif

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUHardwareSelector);

vtkWebGPUHardwareSelector::vtkWebGPUHardwareSelector() = default;
vtkWebGPUHardwareSelector::~vtkWebGPUHardwareSelector() = default;

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PreCapturePass(int pass) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PostCapturePass(int pass) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginSelection() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndSelection() {}

//------------------------------------------------------------------------------
// just add debug output if compiled with vtkWebGPUHardwareSelectorDEBUG
void vtkWebGPUHardwareSelector::SavePixelBuffer(int passNo) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginRenderProp(vtkRenderWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginRenderProp() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndRenderProp(vtkRenderWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndRenderProp() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::RenderCompositeIndex(unsigned int index) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::RenderProcessId(unsigned int processid) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PrintSelf(ostream& os, vtkIndent indent) {}
VTK_ABI_NAMESPACE_END
