/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedAAHexahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProjectedAAHexahedraMapper - volume mapper for axis-aligned hexahedra
// .SECTION Description
// High quality volume renderer for axis-aligned hexahedra

// .SECTION Implementation
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This mapper implements the paper
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data",
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.

#ifndef __vtkProjectedAAHexahedraMapper_h
#define __vtkProjectedAAHexahedraMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkFloatArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkVisibilitySort;
class vtkVolumeProperty;
class vtkRenderWindow;

class VTK_VOLUMERENDERING_EXPORT vtkProjectedAAHexahedraMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeMacro(vtkProjectedAAHexahedraMapper,
               vtkUnstructuredGridVolumeMapper);
  static vtkProjectedAAHexahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Algorithm used to sort the cells according to viewpoint of the camera.
  // Initial value is a vtkCellCenterDepthSort object.
  virtual void SetVisibilitySort(vtkVisibilitySort *sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  // Description:
  // Check if the required OpenGL extensions are supported by the OpenGL
  // context attached to the render window `w'.
  virtual bool IsRenderSupported(vtkRenderWindow *w)=0;

protected:
  vtkProjectedAAHexahedraMapper();
  ~vtkProjectedAAHexahedraMapper();

  // Description:
  // The visibility sort will probably make a reference loop by holding a
  // reference to the input.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  vtkVisibilitySort *VisibilitySort;

private:
  vtkProjectedAAHexahedraMapper(const vtkProjectedAAHexahedraMapper &);  // Not Implemented.
  void operator=(const vtkProjectedAAHexahedraMapper &);  // Not Implemented.
};

#endif
