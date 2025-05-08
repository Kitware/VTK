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
#ifndef viskores_rendering_ScalarRenderer_h
#define viskores_rendering_ScalarRenderer_h

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/Camera.h>

#include <memory>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT ScalarRenderer
{
public:
  ScalarRenderer();

  // Disable copying due to unique_ptr;
  ScalarRenderer(const ScalarRenderer&) = delete;
  ScalarRenderer& operator=(const ScalarRenderer&) = delete;

  ScalarRenderer(ScalarRenderer&&) noexcept;
  ScalarRenderer& operator=(ScalarRenderer&&) noexcept;
  // Default destructor implemented in source file to support PIMPL idiom.
  ~ScalarRenderer();

  void SetInput(viskores::cont::DataSet& dataSet);

  void SetWidth(viskores::Int32 width);
  void SetHeight(viskores::Int32 height);
  void SetDefaultValue(viskores::Float32 value);

  struct VISKORES_RENDERING_EXPORT Result
  {
    viskores::Int32 Width;
    viskores::Int32 Height;
    viskores::cont::ArrayHandle<viskores::Float32> Depths;
    std::vector<viskores::cont::ArrayHandle<viskores::Float32>> Scalars;
    std::vector<std::string> ScalarNames;
    std::map<std::string, viskores::Range> Ranges;

    viskores::cont::DataSet ToDataSet();
  };

  ScalarRenderer::Result Render(const viskores::rendering::Camera& camera);

private:
  struct InternalsType;
  std::unique_ptr<InternalsType> Internals;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_ScalarRenderer_h
