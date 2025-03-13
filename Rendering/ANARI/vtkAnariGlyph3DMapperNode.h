// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariGlyph3DMapperNode
 * @brief   A Glyph mapper node for ANARI (ANAlytic Rendering Interface).
 *
 *
 * ANARI provides cross-vendor portability to diverse rendering engines,
 * including those using state-of-the-art ray tracing. This is the Glyph
 * Mapper node class, which is the ANARI equivalent of the vtkGlyph3DMapper
 * for glyphs. It is built on top of the vtkAnariCompositePolyDataMapperNode
 * to reuse existing composite structure traversal and point/mesh rendering
 * capabilities of ANARI.
 *
 * @par Thanks:
 * Kees van Kooten kvankooten@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 *
 */

#ifndef vtkAnariGlyph3DMapperNode_h
#define vtkAnariGlyph3DMapperNode_h

#include "vtkAnariCompositePolyDataMapperNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariGlyph3DMapperNodeInternals;
class vtkCompositeDataDisplayAttributes;
class vtkMapper;
class vtkProperty;
class vtkPolyData;

class VTKRENDERINGANARI_EXPORT vtkAnariGlyph3DMapperNode
  : public vtkAnariCompositePolyDataMapperNode
{
public:
  static vtkAnariGlyph3DMapperNode* New();
  vtkTypeMacro(vtkAnariGlyph3DMapperNode, vtkAnariCompositePolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sync VTK and ANARI objects.
   */
  virtual void Synchronize(bool prepass) override;

protected:
  vtkAnariGlyph3DMapperNode();
  ~vtkAnariGlyph3DMapperNode() override;

  vtkCompositeDataDisplayAttributes* GetCompositeDisplayAttributes() override;

  vtkAnariGlyph3DMapperNodeInternals* Internal = nullptr;

private:
  vtkAnariGlyph3DMapperNode(const vtkAnariGlyph3DMapperNode&) = delete;
  void operator=(const vtkAnariGlyph3DMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
