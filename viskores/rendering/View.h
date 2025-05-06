//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_View_h
#define viskores_rendering_View_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/TextAnnotation.h>

#include <functional>
#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief The abstract class representing the view of a rendering scene.
class VISKORES_RENDERING_EXPORT View
{
  struct InternalData;

public:
  View(const viskores::rendering::Scene& scene,
       const viskores::rendering::Mapper& mapper,
       const viskores::rendering::Canvas& canvas,
       const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
       const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  View(const viskores::rendering::Scene& scene,
       const viskores::rendering::Mapper& mapper,
       const viskores::rendering::Canvas& canvas,
       const viskores::rendering::Camera& camera,
       const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
       const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  virtual ~View();

  /// @brief Specify the scene object holding the objects to render.
  VISKORES_CONT
  const viskores::rendering::Scene& GetScene() const;
  /// @copydoc GetScene
  VISKORES_CONT
  viskores::rendering::Scene& GetScene();
  /// @copydoc GetScene
  VISKORES_CONT
  void SetScene(const viskores::rendering::Scene& scene);

  /// @brief Specify the mapper object determining how objects are rendered.
  VISKORES_CONT
  const viskores::rendering::Mapper& GetMapper() const;
  /// @copydoc GetMapper
  VISKORES_CONT
  viskores::rendering::Mapper& GetMapper();

  /// @brief Specify the canvas object that holds the buffer to render into.
  VISKORES_CONT
  const viskores::rendering::Canvas& GetCanvas() const;
  /// @copydoc GetCanvas
  VISKORES_CONT
  viskores::rendering::Canvas& GetCanvas();

  VISKORES_CONT
  const viskores::rendering::WorldAnnotator& GetWorldAnnotator() const;

  /// @brief Specify the perspective from which to render a scene.
  VISKORES_CONT
  const viskores::rendering::Camera& GetCamera() const;
  /// @copydoc GetCamera
  VISKORES_CONT
  viskores::rendering::Camera& GetCamera();
  /// @copydoc GetCamera
  VISKORES_CONT
  void SetCamera(const viskores::rendering::Camera& camera);

  /// @brief Specify the color used where nothing is rendered.
  VISKORES_CONT
  const viskores::rendering::Color& GetBackgroundColor() const;
  /// @copydoc GetBackgroundColor
  VISKORES_CONT
  void SetBackgroundColor(const viskores::rendering::Color& color);

  /// @brief Specify the color of foreground elements.
  ///
  /// The foreground is typically used for annotation elements.
  /// The foreground should contrast well with the background.
  VISKORES_CONT
  void SetForegroundColor(const viskores::rendering::Color& color);

  VISKORES_CONT
  bool GetWorldAnnotationsEnabled() const { return this->WorldAnnotationsEnabled; }

  VISKORES_CONT
  void SetWorldAnnotationsEnabled(bool val) { this->WorldAnnotationsEnabled = val; }

  VISKORES_CONT void SetRenderAnnotationsEnabled(bool val) { this->RenderAnnotationsEnabled = val; }
  VISKORES_CONT bool GetRenderAnnotationsEnabled() const { return this->RenderAnnotationsEnabled; }

  /// @brief Render a scene and store the result in the canvas' buffers.
  virtual void Paint() = 0;
  virtual void RenderScreenAnnotations() = 0;
  virtual void RenderWorldAnnotations() = 0;

  void RenderAnnotations();

  /// @copydoc viskores::rendering::Canvas::SaveAs
  void SaveAs(const std::string& fileName) const;

  VISKORES_CONT
  void SetAxisColor(viskores::rendering::Color c);

  VISKORES_CONT
  void ClearTextAnnotations();

  VISKORES_CONT
  void AddTextAnnotation(std::unique_ptr<viskores::rendering::TextAnnotation> ann);

  VISKORES_CONT
  void ClearAdditionalAnnotations();

  VISKORES_CONT
  void AddAdditionalAnnotation(std::function<void(void)> ann);

protected:
  void SetupForWorldSpace(bool viewportClip = true);

  void SetupForScreenSpace(bool viewportClip = false);


  viskores::rendering::Color AxisColor = viskores::rendering::Color::white;
  bool WorldAnnotationsEnabled = true;
  bool RenderAnnotationsEnabled = true;

private:
  std::unique_ptr<InternalData> Internal;
};

} // namespace viskores::rendering
} // namespace viskores

#endif //viskores_rendering_View_h
