/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeSurfaceLICMapperDelegator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeSurfaceLICMapperDelegator
 * @brief   Delegates rendering to vtkBatchedSurfaceLICMapper.
 *
 * @sa vtkBatchedSurfaceLICMapper
 */

#ifndef vtkCompositeSurfaceLICMapperDelegator_h
#define vtkCompositeSurfaceLICMapperDelegator_h

#include "vtkOpenGLCompositePolyDataMapperDelegator.h"

#include "vtkRenderingLICOpenGL2Module.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGLICOPENGL2_EXPORT vtkCompositeSurfaceLICMapperDelegator
  : public vtkOpenGLCompositePolyDataMapperDelegator
{
public:
  static vtkCompositeSurfaceLICMapperDelegator* New();
  vtkTypeMacro(vtkCompositeSurfaceLICMapperDelegator, vtkOpenGLCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy over the reference to the input array of vectors
   * that gets used by BatchedSurfaceLICMapper
   */
  void ShallowCopy(vtkCompositePolyDataMapper* mapper) override;

protected:
  vtkCompositeSurfaceLICMapperDelegator();
  ~vtkCompositeSurfaceLICMapperDelegator() override;

private:
  vtkCompositeSurfaceLICMapperDelegator(const vtkCompositeSurfaceLICMapperDelegator&) = delete;
  void operator=(const vtkCompositeSurfaceLICMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
