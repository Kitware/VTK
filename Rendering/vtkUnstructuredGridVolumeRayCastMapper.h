/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkUnstructuredGridVolumeRayCastMapper - A software mapper for unstructured volumes
// .SECTION Description
// This is a software ray caster for rendering volumes in vtkUnstructuredGrid. 

// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkUnstructuredGridVolumeRayCastMapper_h
#define __vtkUnstructuredGridVolumeRayCastMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkMultiThreader;
class vtkRenderer;
class vtkTimerLog;
class vtkVolume;
class vtkUnstructuredGridBunykRayCastFunction;
class vtkRayCastImageDisplayHelper;

class VTK_RENDERING_EXPORT vtkUnstructuredGridVolumeRayCastMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  static vtkUnstructuredGridVolumeRayCastMapper *New();
  vtkTypeRevisionMacro(vtkUnstructuredGridVolumeRayCastMapper,vtkUnstructuredGridVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );
  
private:
  vtkUnstructuredGridVolumeRayCastMapper(const vtkUnstructuredGridVolumeRayCastMapper&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeRayCastMapper&);  // Not implemented.
};

#endif

