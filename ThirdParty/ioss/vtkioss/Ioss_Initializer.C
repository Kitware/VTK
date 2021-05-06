// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h>
#include <Ioss_StandardElementTypes.h>
#if defined IOSS_THREADSAFE
#include <mutex>
#endif

Ioss::Initializer::Initializer()
{
  // List all storage types here with a call to their factory method.
  // This is Used to get the linker to pull in all needed libraries.
  Ioss::Sphere::factory();

  Ioss::Edge2::factory();
  Ioss::Edge3::factory();
  Ioss::Edge4::factory();

  Ioss::Spring2::factory();
  Ioss::Spring3::factory();
  Ioss::Beam2::factory();
  Ioss::Beam3::factory();
  Ioss::Beam4::factory();
  Ioss::ShellLine2D2::factory();
  Ioss::ShellLine2D3::factory();

  Ioss::Hex8::factory();
  Ioss::Hex16::factory();
  Ioss::Hex20::factory();
  Ioss::Hex27::factory();
  Ioss::Hex32::factory();
  Ioss::Hex64::factory();

  Ioss::Node::factory();

  Ioss::Pyramid5::factory();
  Ioss::Pyramid13::factory();
  Ioss::Pyramid14::factory();
  Ioss::Pyramid18::factory();
  Ioss::Pyramid19::factory();

  Ioss::Quad4::factory();
  Ioss::Quad6::factory();
  Ioss::Quad8::factory();
  Ioss::Quad9::factory();
  Ioss::Quad12::factory();
  Ioss::Quad16::factory();

  Ioss::Shell4::factory();
  Ioss::Shell8::factory();
  Ioss::Shell9::factory();

  Ioss::Tet4::factory();
  Ioss::Tet8::factory();
  Ioss::Tet10::factory();
  Ioss::Tet11::factory();
  Ioss::Tet14::factory();
  Ioss::Tet15::factory();
  Ioss::Tet16::factory();
  Ioss::Tet40::factory();

  Ioss::Tri3::factory();
  Ioss::Tri4::factory();
  Ioss::Tri6::factory();
  Ioss::Tri7::factory();
  Ioss::Tri9::factory();
  Ioss::Tri13::factory();

  Ioss::TriShell3::factory();
  Ioss::TriShell4::factory();
  Ioss::TriShell6::factory();
  Ioss::TriShell7::factory();

  Ioss::Unknown::factory();

  Ioss::Wedge6::factory();
  Ioss::Wedge12::factory();
  Ioss::Wedge15::factory();
  Ioss::Wedge16::factory();
  Ioss::Wedge18::factory();
  Ioss::Wedge20::factory();
  Ioss::Wedge21::factory();
  Ioss::Wedge24::factory();
  Ioss::Wedge52::factory();

  Ioss::Super::factory();
}
