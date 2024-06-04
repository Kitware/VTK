// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCompositePolyDataMapper.h"

#include "vtkNew.h"                       // for ivars
#include "vtkRenderingLICOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkSurfaceLICInterface;
class vtkCompositePolyDataMapperDelegator;

class VTKRENDERINGLICOPENGL2_EXPORT VTK_MARSHALAUTO vtkCompositeSurfaceLICMapper
  : public vtkCompositePolyDataMapper
{
public:
  static vtkCompositeSurfaceLICMapper* New();
  vtkTypeMacro(vtkCompositeSurfaceLICMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the vtkSurfaceLICInterface used by this mapper
   */
  vtkSurfaceLICInterface* GetLICInterface() { return this->LICInterface.Get(); }
  ///@}

  /**
   * Lots of LIC setup code
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

protected:
  vtkCompositeSurfaceLICMapper();
  ~vtkCompositeSurfaceLICMapper() override;

  vtkNew<vtkSurfaceLICInterface> LICInterface;

  vtkCompositePolyDataMapperDelegator* CreateADelegator() override;

private:
  vtkCompositeSurfaceLICMapper(const vtkCompositeSurfaceLICMapper&) = delete;
  void operator=(const vtkCompositeSurfaceLICMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
