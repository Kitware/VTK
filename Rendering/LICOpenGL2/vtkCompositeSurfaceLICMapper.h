/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeSurfaceLICMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeSurfaceLICMapper
 * @brief   mapper for composite dataset
 *
 * vtkCompositeSurfaceLICMapper is similar to
 * vtkGenericCompositeSurfaceLICMapper but requires that its inputs all have the
 * same properties (normals, tcoord, scalars, etc) It will only draw
 * polys and it does not support edge flags. The advantage to using
 * this class is that it generally should be faster
 */

#ifndef vtkCompositeSurfaceLICMapper_h
#define vtkCompositeSurfaceLICMapper_h

#include "vtkCompositePolyDataMapper2.h"
#include "vtkNew.h"                       // for ivars
#include "vtkRenderingLICOpenGL2Module.h" // For export macro

class vtkSurfaceLICInterface;

class VTKRENDERINGLICOPENGL2_EXPORT vtkCompositeSurfaceLICMapper
  : public vtkCompositePolyDataMapper2
{
public:
  static vtkCompositeSurfaceLICMapper* New();
  vtkTypeMacro(vtkCompositeSurfaceLICMapper, vtkCompositePolyDataMapper2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the vtkSurfaceLICInterface used by this mapper
   */
  vtkSurfaceLICInterface* GetLICInterface() { return this->LICInterface.Get(); }
  //@}

  /**
   * Lots of LIC setup code
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

protected:
  vtkCompositeSurfaceLICMapper();
  ~vtkCompositeSurfaceLICMapper() override;

  vtkNew<vtkSurfaceLICInterface> LICInterface;

  vtkCompositeMapperHelper2* CreateHelper() override;

  // copy values to the helpers
  void CopyMapperValuesToHelper(vtkCompositeMapperHelper2* helper) override;

private:
  vtkCompositeSurfaceLICMapper(const vtkCompositeSurfaceLICMapper&) = delete;
  void operator=(const vtkCompositeSurfaceLICMapper&) = delete;
};

#endif
