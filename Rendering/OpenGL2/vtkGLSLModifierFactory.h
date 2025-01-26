// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModifierFactory
 * @brief   This class empowers developers to write and use GLSL mods easily in VTK.
 *
 * For example, the mod classes could be registered via plugins.
 */

#ifndef vtkGLSLModifierFactory_h
#define vtkGLSLModifierFactory_h

#include "vtkRenderingOpenGL2Module.h"

#include <functional>
#include <map>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkGLSLModifierBase;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModifierFactory
{
public:
  using NewModFunction = std::function<vtkGLSLModifierBase*(void* userData)>;

  /// Used by VTK to create a mod for rendering.
  static vtkGLSLModifierBase* CreateAMod(const std::string& modName);

  /// Developers should register runtime GLSL mods by invoking this function.
  /// \param modName name of your mod's C++ class.
  /// \param createFunction a function that VTK will call to create your mod.
  /// \param userData pass any application specific data that you feel is necessary while
  /// initializing your mod.
  static void RegisterAMod(
    const std::string& modName, NewModFunction createFunction, void* userData = nullptr);

protected:
  vtkGLSLModifierFactory() = default;
  ~vtkGLSLModifierFactory() = default;

private:
  vtkGLSLModifierFactory(const vtkGLSLModifierFactory&) = delete;
  vtkGLSLModifierFactory& operator=(const vtkGLSLModifierFactory&) = delete;

  static vtkGLSLModifierFactory& GetInstance();

  struct ModCreator
  {
    NewModFunction F;
    void* UserData;
  };
  std::map<std::string, ModCreator> ModTable;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkGLSLModifierFactory.h
