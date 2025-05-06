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

#ifndef viskores_interop_anari_ANARIScene_h
#define viskores_interop_anari_ANARIScene_h

#include <viskores/interop/anari/ANARIMapper.h>
// std
#include <string>
#include <type_traits>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief Object which manages a collection of mappers representing a single scene.
///
/// This object is a container of named mappers which will automatically keep
/// an `ANARIWorld` up to date which contains any `ANARISurface` or
/// `ANARIVolume` objects coming from the contained mappers. While applications
/// are free to do this work themselves, it is very convenient and useful to
/// let `ANARIScene` do the work of keeping an `ANARIWorld` up to date for you.
///
/// Mappers in `ANARIScene` can also be selectively hidden for quick scene
/// updates. A hidden mapper's geometry/volume are just skipped when updating
/// the list of object handles in the world.
///
/// NOTE: This object will not create any lights in the scene, so the
/// `ANARIWorld` used by the application is expected to have application-managed
/// `ANARILight` objects added to it when desired.
///
/// NOTE: Unlike `ANARIMapper` and `ANARIActor`, `ANARIScene` is not C++
/// copyable or movable.
struct VISKORES_ANARI_EXPORT ANARIScene
{
  /// @brief Constructor
  ///
  ANARIScene(anari_cpp::Device device);

  /// @brief Destructor
  ///
  ~ANARIScene();

  ANARIScene(const ANARIScene&) = delete;
  ANARIScene(ANARIScene&&) = delete;
  ANARIScene& operator=(const ANARIScene&) = delete;
  ANARIScene& operator=(ANARIScene&&) = delete;

  /// @brief Add a mapper to the scene.
  /// @tparam ANARIMapperType Any subclass of `ANARIMapper`.
  ///
  /// NOTE: This will replace any mapper that has the same name.
  template <typename ANARIMapperType>
  ANARIMapperType& AddMapper(const ANARIMapperType& mapper, bool visible = true);

  /// @brief Add a mapper to the scene.
  /// @tparam ANARIMapperType Any subclass of `ANARIMapper`.
  ///
  /// NOTE: Replace the i'th mapper with a new instance.
  /// NOTE: It is undefined behavior to use this to put 2 or more mappers in the
  /// scene with the same name.
  template <typename ANARIMapperType>
  void ReplaceMapper(const ANARIMapperType& newMapper, viskores::IdComponent id, bool visible);

  /// @brief Get number of mappers in this scene.
  ///
  viskores::IdComponent GetNumberOfMappers() const;

  /// @brief Ask whether a mapper has the passed in name or not.
  ///
  bool HasMapperWithName(const char* name) const;

  /// @brief Get the index to the mapper with the given name.
  ///
  viskores::IdComponent GetMapperIndexByName(const char* name);

  /// @brief Get the associated mapper by index.
  ///
  ANARIMapper& GetMapper(viskores::IdComponent id);

  /// @brief Get the associated mapper by name.
  ///
  ANARIMapper& GetMapper(const char* name);

  /// @brief Get the associated mapper by name.
  ///
  bool GetMapperVisible(viskores::IdComponent id) const;
  void SetMapperVisible(viskores::IdComponent id, bool shown);

  /// @brief Remove mapper by index.
  ///
  void RemoveMapper(viskores::IdComponent id);

  /// @brief Remove mapper by name.
  ///
  void RemoveMapper(const char* name);

  /// @brief  Clear out this scene of all mappers.
  void RemoveAllMappers();

  /// @brief Get the `ANARIDevice` handle this scene is talking to.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  anari_cpp::Device GetDevice() const;

  /// @brief Get the `ANARIWorld` handle this scene is working on.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  anari_cpp::World GetANARIWorld();

private:
  void UpdateWorld();

  anari_cpp::Device Device{ nullptr };
  anari_cpp::World World{ nullptr };

  struct SceneMapper
  {
    std::unique_ptr<ANARIMapper> Mapper;
    bool Show{ true };
  };

  std::vector<SceneMapper> Mappers;
};

// Inlined definitions ////////////////////////////////////////////////////////

template <typename ANARIMapperType>
inline ANARIMapperType& ANARIScene::AddMapper(const ANARIMapperType& mapper, bool visible)
{
  static_assert(std::is_base_of<ANARIMapper, ANARIMapperType>::value,
                "Only ANARIMapper types can be added to ANARIScene");

  auto* name = mapper.GetName();
  if (HasMapperWithName(name))
  {
    auto idx = GetMapperIndexByName(name);
    ReplaceMapper(mapper, idx, visible);
    return (ANARIMapperType&)GetMapper(idx);
  }
  else
  {
    this->Mappers.push_back({ std::make_unique<ANARIMapperType>(mapper), visible });
    UpdateWorld();
    return (ANARIMapperType&)GetMapper(GetNumberOfMappers() - 1);
  }
}

template <typename ANARIMapperType>
inline void ANARIScene::ReplaceMapper(const ANARIMapperType& newMapper,
                                      viskores::IdComponent id,
                                      bool visible)
{
  static_assert(std::is_base_of<ANARIMapper, ANARIMapperType>::value,
                "Only ANARIMapper types can be added to ANARIScene");
  const bool wasVisible = GetMapperVisible(id);
  Mappers[id] = { std::make_unique<ANARIMapperType>(newMapper), visible };
  if (wasVisible || visible)
    UpdateWorld();
}

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
