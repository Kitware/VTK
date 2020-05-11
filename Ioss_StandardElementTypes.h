/*
 * Copyright(C) 1999-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef Ioss_STANDARD_ELEMENT_TYPES_H
#define Ioss_STANDARD_ELEMENT_TYPES_H

#include "vtk_ioss_mangle.h"

#include <Ioss_Beam2.h>
#include <Ioss_Beam3.h>
#include <Ioss_Beam4.h>
#include <Ioss_Edge2.h>
#include <Ioss_Edge3.h>
#include <Ioss_Edge4.h>
#include <Ioss_Hex16.h>
#include <Ioss_Hex20.h>
#include <Ioss_Hex27.h>
#include <Ioss_Hex32.h>
#include <Ioss_Hex64.h>
#include <Ioss_Hex8.h>
#include <Ioss_Initializer.h>
#include <Ioss_Node.h>
#include <Ioss_Pyramid13.h>
#include <Ioss_Pyramid14.h>
#include <Ioss_Pyramid18.h>
#include <Ioss_Pyramid19.h>
#include <Ioss_Pyramid5.h>
#include <Ioss_Quad12.h>
#include <Ioss_Quad16.h>
#include <Ioss_Quad4.h>
#include <Ioss_Quad6.h>
#include <Ioss_Quad8.h>
#include <Ioss_Quad9.h>
#include <Ioss_Shell4.h>
#include <Ioss_Shell8.h>
#include <Ioss_Shell9.h>
#include <Ioss_ShellLine2D2.h>
#include <Ioss_ShellLine2D3.h>
#include <Ioss_Sphere.h>
#include <Ioss_Spring2.h>
#include <Ioss_Spring3.h>
#include <Ioss_Super.h>
#include <Ioss_Tet10.h>
#include <Ioss_Tet11.h>
#include <Ioss_Tet14.h>
#include <Ioss_Tet15.h>
#include <Ioss_Tet16.h>
#include <Ioss_Tet4.h>
#include <Ioss_Tet40.h>
#include <Ioss_Tet8.h>
#include <Ioss_Tri13.h>
#include <Ioss_Tri3.h>
#include <Ioss_Tri4.h>
#include <Ioss_Tri6.h>
#include <Ioss_Tri7.h>
#include <Ioss_Tri9.h>
#include <Ioss_TriShell3.h>
#include <Ioss_TriShell4.h>
#include <Ioss_TriShell6.h>
#include <Ioss_TriShell7.h>
#include <Ioss_Unknown.h>
#include <Ioss_Wedge12.h>
#include <Ioss_Wedge15.h>
#include <Ioss_Wedge16.h>
#include <Ioss_Wedge18.h>
#include <Ioss_Wedge20.h>
#include <Ioss_Wedge21.h>
#include <Ioss_Wedge24.h>
#include <Ioss_Wedge52.h>
#include <Ioss_Wedge6.h>
#endif
