/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOSPRayVolumeMapper - Standalone OSPRayVolumeMapper.
// .SECTION Description
// This is a standalone interface for ospray volume rendering to be used
// within otherwise OpenGL rendering contexts such as within the
// SmartVolumeMapper.

#ifndef vtkOSPRayVolumeMapper_h
#define vtkOSPRayVolumeMapper_h


#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkOSPRayVolumeInterface.h"
#include "vtkNew.h" //to ease garbage collection
#include "vtkGarbageCollector.h" //ditto

#include <vector>

class vtkGarbageCollector;
class vtkOSPRayPass;
class vtkRenderer;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeMapper
  : public vtkOSPRayVolumeInterface
{
public:
  static vtkOSPRayVolumeMapper *New();
  vtkTypeMacro(vtkOSPRayVolumeMapper,vtkOSPRayVolumeInterface);
  void PrintSelf( ostream& os, vtkIndent indent );

  //Description:
  //Render the volume onto the screen.
  //Overridden to use OSPRay to do the work.
  virtual void Render(vtkRenderer *, vtkVolume *);

 protected:
  vtkOSPRayVolumeMapper();
  ~vtkOSPRayVolumeMapper();

  vtkOSPRayPass *OSPRayPass;
  vtkRenderer *InternalRenderer;

  std::vector<float> ZBuffer;
private:
  vtkOSPRayVolumeMapper(const vtkOSPRayVolumeMapper&);  // Not implemented.
  void operator=(const vtkOSPRayVolumeMapper&);  // Not implemented.
};

#endif
