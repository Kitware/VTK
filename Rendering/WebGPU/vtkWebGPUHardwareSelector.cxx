// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
void vtkWebGPUHardwareSelector::PreCapturePass(int) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PostCapturePass(int) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginSelection() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndSelection() {}

//------------------------------------------------------------------------------
// just add debug output if compiled with vtkWebGPUHardwareSelectorDEBUG
void vtkWebGPUHardwareSelector::SavePixelBuffer(int) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginRenderProp(vtkRenderWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginRenderProp() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndRenderProp(vtkRenderWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndRenderProp() {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::RenderCompositeIndex(unsigned int) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::RenderProcessId(unsigned int) {}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PrintSelf(ostream&, vtkIndent) {}
VTK_ABI_NAMESPACE_END
