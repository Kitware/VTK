// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderMaterialLibrary
 * @brief   a renderer-agnostic collection of materials for VTK apps to draw from
 */

#ifndef vtkRenderMaterialLibrary_h
#define vtkRenderMaterialLibrary_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderMaterialLibraryInternals;
class vtkTexture;
struct TextureInfo;

class VTKRENDERINGCORE_EXPORT vtkRenderMaterialLibrary : public vtkObject
{
public:
  static vtkRenderMaterialLibrary* New();
  vtkTypeMacro(vtkRenderMaterialLibrary, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool ReadFile(const char* FileName);
  bool ReadBuffer(const char* Buffer);

  std::set<std::string> GetMaterialNames();
  std::string LookupImplName(const std::string& nickname);

protected:
  vtkRenderMaterialLibrary();
  ~vtkRenderMaterialLibrary() override;

private:
  vtkRenderMaterialLibrary(const vtkRenderMaterialLibrary&) = delete;
  void operator=(const vtkRenderMaterialLibrary&) = delete;

  vtkRenderMaterialLibraryInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
