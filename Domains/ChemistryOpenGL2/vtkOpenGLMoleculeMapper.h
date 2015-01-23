/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLMoleculeMapper - An accelerated class for rendering molecules
// .SECTION Description
// A vtkMoleculeMapper that uses imposters to do the rendering. It uses
// vtkOpenGLSphereMapper and vtkOpenGLStickMapper to do the rendering.

#ifndef vtkOpenGLMoleculeMapper_h
#define vtkOpenGLMoleculeMapper_h

#include "vtkDomainsChemistryOpenGL2Module.h" // For export macro
#include "vtkMoleculeMapper.h"
#include "vtkNew.h" // For vtkNew

class vtkOpenGLSphereMapper;
class vtkOpenGLStickMapper;

class VTKDOMAINSCHEMISTRYOPENGL2_EXPORT vtkOpenGLMoleculeMapper : public vtkMoleculeMapper
{
public:
  static vtkOpenGLMoleculeMapper* New();
  vtkTypeMacro(vtkOpenGLMoleculeMapper, vtkMoleculeMapper)

  // Description:
  // Reimplemented from base class
  virtual void Render(vtkRenderer *, vtkActor *);
  virtual void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkOpenGLMoleculeMapper();
  ~vtkOpenGLMoleculeMapper();

  virtual void UpdateAtomGlyphPolyData();
  virtual void UpdateBondGlyphPolyData();

  // Description:
  // Internal mappers
  vtkNew<vtkOpenGLSphereMapper> FastAtomMapper;
  vtkNew<vtkOpenGLStickMapper> FastBondMapper;

private:
  vtkOpenGLMoleculeMapper(const vtkOpenGLMoleculeMapper&); // Not implemented.
  void operator=(const vtkOpenGLMoleculeMapper&); // Not implemented.
};

#endif