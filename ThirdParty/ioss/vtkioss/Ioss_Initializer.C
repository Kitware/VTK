// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
