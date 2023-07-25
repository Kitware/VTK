// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLMoleculeMapper
 * @brief   An accelerated class for rendering molecules
 *
 * A vtkMoleculeMapper that uses imposters to do the rendering. It uses
 * vtkOpenGLSphereMapper and vtkOpenGLStickMapper to do the rendering.
 */

#ifndef vtkOpenGLMoleculeMapper_h
#define vtkOpenGLMoleculeMapper_h

#include "vtkDomainsChemistryOpenGL2Module.h" // For export macro
#include "vtkMoleculeMapper.h"
#include "vtkNew.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLSphereMapper;
class vtkOpenGLStickMapper;

class VTKDOMAINSCHEMISTRYOPENGL2_EXPORT vtkOpenGLMoleculeMapper : public vtkMoleculeMapper
{
public:
  static vtkOpenGLMoleculeMapper* New();
  vtkTypeMacro(vtkOpenGLMoleculeMapper, vtkMoleculeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Reimplemented from base class
   */
  void Render(vtkRenderer*, vtkActor*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  ///@}

  /**
   * provide access to the underlying mappers
   */
  vtkOpenGLSphereMapper* GetFastAtomMapper() { return this->FastAtomMapper; }
  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

  /**
   * Helper method to set ScalarMode on both FastAtomMapper and FastBondMapper.
   * true means VTK_COLOR_MODE_MAP_SCALARS, false VTK_COLOR_MODE_DIRECT_SCALARS.
   */
  void SetMapScalars(bool map) override;

protected:
  vtkOpenGLMoleculeMapper();
  ~vtkOpenGLMoleculeMapper() override;

  void UpdateAtomGlyphPolyData() override;
  void UpdateBondGlyphPolyData() override;

  ///@{
  /**
   * Internal mappers
   */
  vtkNew<vtkOpenGLSphereMapper> FastAtomMapper;
  vtkNew<vtkOpenGLStickMapper> FastBondMapper;
  ///@}

private:
  vtkOpenGLMoleculeMapper(const vtkOpenGLMoleculeMapper&) = delete;
  void operator=(const vtkOpenGLMoleculeMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
