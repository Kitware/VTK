/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeMapper.h
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
// .NAME vtkUnstructuredGridVolumeMapper - Abstract class for a unstructured grid volume mapper

// .SECTION Description
// vtkUnstructuredGridVolumeMapper is the abstract definition of a volume mapper for 
// unstructured data (vtkUnstructuredGrid).  Several  basic types of volume mappers 
// are supported as subclasses

// .SECTION see also
// vtkUnstructuredGridVolumeRayCastMapper

#ifndef __vtkUnstructuredGridVolumeMapper_h
#define __vtkUnstructuredGridVolumeMapper_h

#include "vtkAbstractVolumeMapper.h"

class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGrid;
class vtkWindow;


class VTK_RENDERING_EXPORT vtkUnstructuredGridVolumeMapper : public vtkAbstractVolumeMapper
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridVolumeMapper,vtkAbstractVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  
private:
  vtkUnstructuredGridVolumeMapper(const vtkUnstructuredGridVolumeMapper&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeMapper&);  // Not implemented.
};


#endif


