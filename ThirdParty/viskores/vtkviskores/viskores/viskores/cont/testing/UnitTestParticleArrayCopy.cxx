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

#include <random>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/cont/testing/Testing.h>

void TestParticleArrayCopy()
{
  std::random_device device;
  std::default_random_engine generator(static_cast<viskores::UInt32>(277));
  viskores::FloatDefault x0(-1), x1(1);
  std::uniform_real_distribution<viskores::FloatDefault> dist(x0, x1);

  std::vector<viskores::Particle> particles;
  viskores::Id N = 17;
  for (viskores::Id i = 0; i < N; i++)
  {
    auto x = dist(generator);
    auto y = dist(generator);
    auto z = dist(generator);
    particles.push_back(viskores::Particle(viskores::Vec3f(x, y, z), i));
  }

  for (viskores::Id i = 0; i < 2; i++)
  {
    auto particleAH = viskores::cont::make_ArrayHandle(particles, viskores::CopyFlag::Off);

    //Test copy position only
    if (i == 0)
    {
      viskores::cont::ArrayHandle<viskores::Vec3f> pos;
      viskores::cont::ParticleArrayCopy<viskores::Particle>(particleAH, pos);

      auto pPortal = particleAH.ReadPortal();
      for (viskores::Id j = 0; j < N; j++)
      {
        auto p = pPortal.Get(j);
        auto pt = pos.ReadPortal().Get(j);
        VISKORES_TEST_ASSERT(p.GetPosition() == pt, "Positions do not match");
      }
    }
    else //Test copy everything
    {
      viskores::cont::ArrayHandle<viskores::Vec3f> pos;
      viskores::cont::ArrayHandle<viskores::Id> ids, steps;
      viskores::cont::ArrayHandle<viskores::ParticleStatus> status;
      viskores::cont::ArrayHandle<viskores::FloatDefault> ptime;

      viskores::cont::ParticleArrayCopy<viskores::Particle>(
        particleAH, pos, ids, steps, status, ptime);

      auto pPortal = particleAH.ReadPortal();
      for (viskores::Id j = 0; j < N; j++)
      {
        auto p = pPortal.Get(j);
        auto pt = pos.ReadPortal().Get(j);
        VISKORES_TEST_ASSERT(p.GetPosition() == pt, "Positions do not match");
        VISKORES_TEST_ASSERT(p.GetID() == ids.ReadPortal().Get(j), "IDs do not match");
        VISKORES_TEST_ASSERT(p.GetNumberOfSteps() == steps.ReadPortal().Get(j),
                             "Steps do not match");
        VISKORES_TEST_ASSERT(p.GetStatus() == status.ReadPortal().Get(j), "Status do not match");
        VISKORES_TEST_ASSERT(p.GetTime() == ptime.ReadPortal().Get(j), "Times do not match");
      }
    }
  }

  //Test copying a vector of ArrayHandles.
  std::vector<viskores::cont::ArrayHandle<viskores::Particle>> particleVec;
  viskores::Id totalNumParticles = 0;
  viskores::Id pid = 0;
  for (viskores::Id i = 0; i < 4; i++)
  {
    viskores::Id n = 5 + i;
    std::vector<viskores::Particle> vec;
    for (viskores::Id j = 0; j < n; j++)
    {
      auto x = dist(generator);
      auto y = dist(generator);
      auto z = dist(generator);
      vec.push_back(viskores::Particle(viskores::Vec3f(x, y, z), pid));
      pid++;
    }
    auto ah = viskores::cont::make_ArrayHandle(vec, viskores::CopyFlag::On);
    particleVec.push_back(ah);
    totalNumParticles += ah.GetNumberOfValues();
  }

  viskores::cont::ArrayHandle<viskores::Vec3f> res;
  viskores::cont::ParticleArrayCopy<viskores::Particle>(particleVec, res);
  VISKORES_TEST_ASSERT(res.GetNumberOfValues() == totalNumParticles, "Wrong number of particles");

  viskores::Id resIdx = 0;
  auto resPortal = res.ReadPortal();
  for (const auto& v : particleVec)
  {
    viskores::Id n = v.GetNumberOfValues();
    auto portal = v.ReadPortal();
    for (viskores::Id i = 0; i < n; i++)
    {
      auto p = portal.Get(i);
      auto pRes = resPortal.Get(resIdx);
      VISKORES_TEST_ASSERT(p.GetPosition() == pRes, "Positions do not match");
      resIdx++;
    }
  }
}

int UnitTestParticleArrayCopy(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestParticleArrayCopy, argc, argv);
}
