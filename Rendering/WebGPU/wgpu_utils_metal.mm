/*=========================================================================

  Program:   Visualization Toolkit
  Module:    wgpu_utils_metal.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if !defined(DAWN_ENABLE_BACKEND_METAL)
#error "utils_metal.mm requires the Metal backend to be enabled."
#endif // !defined(DAWN_ENABLE_BACKEND_METAL)
#import <QuartzCore/CAMetalLayer.h>

//------------------------------------------------------------------------------
static std::unique_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptorCocoa(void* window)
{
  NSWindow* nsWindow = reinterpret_cast<NSWindow*>(window) NSView* view = [nsWindow contentView];
  // Create a CAMetalLayer that covers the whole window that will be passed to
  // CreateSurface.
  [view setWantsLayer:YES];
  [view setLayer:[CAMetalLayer layer]];
  // Use retina if the window was created with retina support.
  [[view layer] setContentsScale:[nsWindow backingScaleFactor]];
  auto desc = std::unique_ptr<wgpu::SurfaceDescriptorFromMetalLayer>(
    new wgpu::SurfaceDescriptorFromMetalLayer());
  desc->layer = [view layer];
  return std::move(desc);
}
