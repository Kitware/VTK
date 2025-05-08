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

#include <viskores/rendering/Scene.h>

#include <vector>

namespace viskores
{
namespace rendering
{

struct Scene::InternalsType
{
  std::vector<viskores::rendering::Actor> Actors;
};

Scene::Scene()
  : Internals(new InternalsType)
{
}

void Scene::AddActor(viskores::rendering::Actor actor)
{
  this->Internals->Actors.push_back(std::move(actor));
}

const viskores::rendering::Actor& Scene::GetActor(viskores::IdComponent index) const
{
  return this->Internals->Actors[static_cast<std::size_t>(index)];
}

viskores::IdComponent Scene::GetNumberOfActors() const
{
  return static_cast<viskores::IdComponent>(this->Internals->Actors.size());
}

void Scene::Render(viskores::rendering::Mapper& mapper,
                   viskores::rendering::Canvas& canvas,
                   const viskores::rendering::Camera& camera) const
{
  for (const auto& actor : this->Internals->Actors)
  {
    actor.Render(mapper, canvas, camera);
  }
}

viskores::Bounds Scene::GetSpatialBounds() const
{
  viskores::Bounds bounds;
  for (const auto& actor : this->Internals->Actors)
  {
    bounds.Include(actor.GetSpatialBounds());
  }

  return bounds;
}
}
} // namespace viskores::rendering
