/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOSPRayVolumeNode - links vtkVolume and vtkMapper to OSPRay
// .SECTION Description
// Translates vtkVolume/Mapper state into OSPRay rendering calls

#ifndef vtkOSPRayVolumeNode_h
#define vtkOSPRayVolumeNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkVolumeNode.h"

class vtkVolume;
class vtkCompositeDataDisplayAttributes;
class vtkDataArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkPiecewiseFunction;
class vtkPolyData;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeNode :
  public vtkVolumeNode
{
public:
  static vtkOSPRayVolumeNode* New();
  vtkTypeMacro(vtkOSPRayVolumeNode, vtkVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to take into account my renderables time, including
  // mapper and data into mapper inclusive of composite input
  virtual unsigned long GetMTime();

protected:
  vtkOSPRayVolumeNode();
  ~vtkOSPRayVolumeNode();

private:
  vtkOSPRayVolumeNode(const vtkOSPRayVolumeNode&); // Not implemented.
  void operator=(const vtkOSPRayVolumeNode&); // Not implemented.
};
#endif
