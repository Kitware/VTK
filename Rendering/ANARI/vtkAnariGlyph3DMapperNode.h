// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariGlyph3DMapperNode
 * @brief   A Glyph mapper node for ANARI (ANAlytic Rendering Interface)
 *          instead of OpenGL.
 *
 *
 * ANARI provides cross-vendor portability to diverse rendering engines,
 * including those using state-of-the-art ray tracing. This is a render
 * pass that can be put into a vtkRenderWindow which makes it use the
 * back-end loaded with ANARI instead of OpenGL to render. Adding or
 * removing the pass will swap back and forth between the two.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 *
 */

#ifndef vtkAnariGlyph3DMapperNode_h
#define vtkAnariGlyph3DMapperNode_h

#include "vtkAnariCompositePolyDataMapperNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

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

#endif
