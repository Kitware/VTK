// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
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

#include <generated/Iogn_GeneratedMesh.h>

#include <Ioss_Hex8.h>
#include <Ioss_Shell4.h>
#include <Ioss_Tet4.h>
#include <Ioss_TriShell3.h>
#include <Ioss_Utils.h>

#include <algorithm>
#include <cassert> // for assert
#include <cmath>   // for atan2, cos, sin
#include <cstdlib> // for nullptr, exit, etc
#include <fmt/ostream.h>
#include <iostream>
#include <numeric>
#include <string>
#include <sys/types.h> // for ssize_t
#include <tokenize.h>  // for tokenize
#include <vector>      // for vector

namespace Iogn {
  GeneratedMesh::GeneratedMesh(int64_t num_x, int64_t num_y, int64_t num_z, int proc_count,
                               int my_proc)
      : numX(num_x), numY(num_y), numZ(num_z), myNumZ(num_z), myStartZ(0),
        processorCount(proc_count), myProcessor(my_proc), timestepCount(0), offX(0), offY(0),
        offZ(0), sclX(1), sclY(1), sclZ(1), doRotation(false), createTets(false)
  {
    initialize();
  }

  GeneratedMesh::GeneratedMesh(const std::string &parameters, int proc_count, int my_proc)
      : numX(0), numY(0), numZ(0), myNumZ(0), myStartZ(0), processorCount(proc_count),
        myProcessor(my_proc), timestepCount(0), offX(0), offY(0), offZ(0), sclX(1), sclY(1),
        sclZ(1), doRotation(false), createTets(false)
  {
    // Possible that the 'parameters' has the working directory path
    // prepended to the parameter list.  Strip off everything in front
    // of the last '/' (if any)...
    auto params = Ioss::tokenize(parameters, "/");

    auto groups = Ioss::tokenize(params.back(), "|+");

    // First 'group' is the interval specification -- IxJxK
    auto tokens = Ioss::tokenize(groups[0], "x");
    assert(tokens.size() == 3);
    numX = std::stoull(tokens[0]);
    numY = std::stoull(tokens[1]);
    numZ = std::stoull(tokens[2]);

    if (numX <= 0 || numY <= 0 || numZ <= 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iogn::GeneratedMesh::GeneratedMesh)\n"
                 "       All interval counts must be greater than 0.\n"
                 "       numX = {}, numY = {}, numZ = {}\n",
                 numX, numY, numZ);
      IOSS_ERROR(errmsg);
    }
    initialize();
    parse_options(groups);
  }

  GeneratedMesh::GeneratedMesh()
      : numX(0), numY(0), numZ(0), myNumZ(0), myStartZ(0), processorCount(0), myProcessor(0),
        timestepCount(0), offX(0), offY(0), offZ(0), sclX(1), sclY(1), sclZ(1), doRotation(false),
        createTets(false)
  {
    initialize();
  }

  GeneratedMesh::~GeneratedMesh() = default;

  void GeneratedMesh::initialize()
  {
    if (processorCount > numZ) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iogn::GeneratedMesh::initialize)\n"
                 "       The number of mesh intervals in the Z direction ({})\n"
                 "       must be at least as large as the number of processors ({}).\n"
                 "       The current parameters do not meet that requirement. Execution will "
                 "terminate.\n",
                 numZ, processorCount);
      IOSS_ERROR(errmsg);
    }

    if (processorCount > 1) {
      myNumZ = numZ / processorCount;
      if (myProcessor < (numZ % processorCount)) {
        myNumZ++;
      }

      // Determine myStartZ for this processor...
      size_t extra = numZ % processorCount;
      if (extra > myProcessor) {
        extra = myProcessor;
      }
      size_t per_proc = numZ / processorCount;
      myStartZ        = myProcessor * per_proc + extra;
    }
    else {
      myNumZ = numZ;
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        rotmat[i][j] = 0.0;
      }
      rotmat[i][i] = 1.0;
    }

    variableCount[Ioss::COMMSET]      = 0;
    variableCount[Ioss::EDGEBLOCK]    = 0;
    variableCount[Ioss::EDGESET]      = 0;
    variableCount[Ioss::ELEMENTBLOCK] = 0;
    variableCount[Ioss::ELEMENTSET]   = 0;
    variableCount[Ioss::FACEBLOCK]    = 0;
    variableCount[Ioss::FACESET]      = 0;
    variableCount[Ioss::INVALID_TYPE] = 0;
    variableCount[Ioss::NODEBLOCK]    = 0;
    variableCount[Ioss::NODESET]      = 0;
    variableCount[Ioss::REGION]       = 0;
    variableCount[Ioss::SIDEBLOCK]    = 0;
    variableCount[Ioss::SIDESET]      = 0;
    variableCount[Ioss::SUPERELEMENT] = 0;
  }

  void GeneratedMesh::create_tets(bool yesno) { createTets = yesno; }

  int64_t GeneratedMesh::add_shell_block(ShellLocation loc)
  {
    shellBlocks.push_back(loc);
    return shellBlocks.size();
  }

  int64_t GeneratedMesh::add_nodeset(ShellLocation loc)
  {
    nodesets.push_back(loc);
    return nodesets.size();
  }

  int64_t GeneratedMesh::add_sideset(ShellLocation loc)
  {
    sidesets.push_back(loc);
    return sidesets.size();
  }

  void GeneratedMesh::set_bbox(double xmin, double ymin, double zmin, double xmax, double ymax,
                               double zmax)
  {
    // NOTE: All calculations are based on the currently
    // active interval settings. If scale or offset or zdecomp
    // specified later in the option list, you may not get the
    // desired bounding box.
    if (numX == 0 || numY == 0 || numZ == 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iogn::GeneratedMesh::set_bbox)\n"
                 "       All interval counts must be greater than 0.\n"
                 "       numX = {}, numY = {}, numZ = {}\n",
                 numX, numY, numZ);
      IOSS_ERROR(errmsg);
    }

    double x_range = xmax - xmin;
    double y_range = ymax - ymin;
    double z_range = zmax - zmin;

    sclX = x_range / static_cast<double>(numX);
    sclY = y_range / static_cast<double>(numY);
    sclZ = z_range / static_cast<double>(numZ);

    offX = xmin;
    offY = ymin;
    offZ = zmin;
  }

  void GeneratedMesh::set_scale(double scl_x, double scl_y, double scl_z)
  {
    sclX = scl_x;
    sclY = scl_y;
    sclZ = scl_z;
  }

  void GeneratedMesh::set_offset(double off_x, double off_y, double off_z)
  {
    offX = off_x;
    offY = off_y;
    offZ = off_z;
  }

  void GeneratedMesh::parse_options(const std::vector<std::string> &groups)
  {
    for (size_t i = 1; i < groups.size(); i++) {
      auto option = Ioss::tokenize(groups[i], ":");
      // option[0] is the type of the option and option[1] is the argument to the option.

      if (option[0] == "shell") {
        // Option of the form  "shell:xXyYzZ"
        // The argument specifies whether there is a shell block
        // at the location. 'x' is minX, 'X' is maxX, etc.
        for (auto opt : option[1]) {
          switch (opt) {
          case 'x': add_shell_block(MX); break;
          case 'X': add_shell_block(PX); break;
          case 'y': add_shell_block(MY); break;
          case 'Y': add_shell_block(PY); break;
          case 'z': add_shell_block(MZ); break;
          case 'Z': add_shell_block(PZ); break;
          default:
            std::ostringstream errmsg;
            fmt::print(errmsg, "ERROR: Unrecognized shell location option '{}'.", opt);
            IOSS_ERROR(errmsg);
          }
        }
      }
      else if (option[0] == "nodeset") {
        // Option of the form  "nodeset:xXyYzZ"
        // The argument specifies whether there is a nodeset
        // at the location. 'x' is minX, 'X' is maxX, etc.
        for (auto opt : option[1]) {
          switch (opt) {
          case 'x': add_nodeset(MX); break;
          case 'X': add_nodeset(PX); break;
          case 'y': add_nodeset(MY); break;
          case 'Y': add_nodeset(PY); break;
          case 'z': add_nodeset(MZ); break;
          case 'Z': add_nodeset(PZ); break;
          default:
            std::ostringstream errmsg;
            fmt::print(errmsg, "ERROR: Unrecognized nodeset location option '{}'.", opt);
            IOSS_ERROR(errmsg);
          }
        }
      }
      else if (option[0] == "sideset") {
        // Option of the form  "sideset:xXyYzZ"
        // The argument specifies whether there is a sideset
        // at the location. 'x' is minX, 'X' is maxX, etc.
        for (auto opt : option[1]) {
          switch (opt) {
          case 'x': add_sideset(MX); break;
          case 'X': add_sideset(PX); break;
          case 'y': add_sideset(MY); break;
          case 'Y': add_sideset(PY); break;
          case 'z': add_sideset(MZ); break;
          case 'Z': add_sideset(PZ); break;
          default:
            std::ostringstream errmsg;
            fmt::print(errmsg, "ERROR: Unrecognized sideset location option '{}'.", opt);
            IOSS_ERROR(errmsg);
          }
        }
      }
      else if (option[0] == "scale") {
        // Option of the form  "scale:xs,ys,zs
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() == 3);
        sclX = std::stod(tokens[0]);
        sclY = std::stod(tokens[1]);
        sclZ = std::stod(tokens[2]);
      }

      else if (option[0] == "offset") {
        // Option of the form  "offset:xo,yo,zo
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() == 3);
        offX = std::stod(tokens[0]);
        offY = std::stod(tokens[1]);
        offZ = std::stod(tokens[2]);
      }

      else if (option[0] == "zdecomp") {
        // Option of the form  "zdecomp:1,1,2,2,1,2,...
        // Specifies the number of intervals in the z direction
        // for each processor.  The number of tokens must match
        // the number of processors.  Note that the new numZ will
        // be the sum of the intervals specified in this command.
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() == processorCount);
        Ioss::Int64Vector Zs;
        numZ = 0;
        for (size_t j = 0; j < processorCount; j++) {
          Zs.push_back(std::stoull(tokens[j]));
          numZ += Zs[j];
        }
        myNumZ   = Zs[myProcessor];
        myStartZ = 0;
        for (size_t j = 0; j < myProcessor; j++) {
          myStartZ += Zs[j];
        }
      }

      else if (option[0] == "bbox") {
        // Bounding-Box Option of the form  "bbox:xmin,ymin,zmin,xmax,ymax,zmaxo
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() == 6);
        double xmin = std::stod(tokens[0]);
        double ymin = std::stod(tokens[1]);
        double zmin = std::stod(tokens[2]);
        double xmax = std::stod(tokens[3]);
        double ymax = std::stod(tokens[4]);
        double zmax = std::stod(tokens[5]);

        set_bbox(xmin, ymin, zmin, xmax, ymax, zmax);
      }

      else if (option[0] == "rotate") {
        // Rotate Option of the form  "rotate:axis,angle,axis,angle,...
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() % 2 == 0);
        for (size_t ir = 0; ir < tokens.size();) {
          std::string axis         = tokens[ir++];
          double      angle_degree = std::stod(tokens[ir++]);
          set_rotation(axis, angle_degree);
        }
      }

      else if (option[0] == "times") {
        timestepCount = std::stoull(option[1]);
      }

      else if (option[0] == "tets") {
        createTets = true;
      }

      else if (option[0] == "variables") {
        // Variables Option of the form  "variables:global,10,element,100,..."
        auto tokens = Ioss::tokenize(option[1], ",");
        assert(tokens.size() % 2 == 0);
        for (size_t ir = 0; ir < tokens.size();) {
          std::string type  = tokens[ir++];
          int         count = std::stoull(tokens[ir++]);
          set_variable_count(type, count);
        }
        if (timestepCount == 0) {
          timestepCount = 1;
        }
      }

      else if (option[0] == "help") {
        fmt::print(Ioss::OUTPUT(),
                   "\nValid Options for GeneratedMesh parameter string:\n"
                   "\tIxJxK -- specifies intervals; must be first option. Ex: 4x10x12\n"
                   "\toffset:xoff, yoff, zoff\n"
                   "\tscale: xscl, yscl, zscl\n"
                   "\tzdecomp:n1,n2,n3,...,n#proc\n"
                   "\tbbox: xmin, ymin, zmin, xmax, ymax, zmax\n"
                   "\trotate: axis,angle,axis,angle,...\n"
                   "\tshell:xXyYzZ (specifies which plane to apply shell)\n"
                   "\tnodeset:xXyYzZ (specifies which plane to apply nodeset)\n"
                   "\tsideset:xXyYzZ (specifies which plane to apply sideset)\n"
                   "\ttets (split each hex into 6 tets)\n"
                   "\tvariables:type,count,...  "
                   "type=global|element|node|nodal|nodeset|sideset|surface\n"
                   "\ttimes:count (number of timesteps to generate)\n"
                   "\tshow -- show mesh parameters\n"
                   "\thelp -- show this list\n\n");
      }

      else if (option[0] == "show") {
        show_parameters();
      }

      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Unrecognized option '{}'.  It will be ignored.\n", option[0]);
        IOSS_ERROR(errmsg);
      }
    }
  }

  void GeneratedMesh::show_parameters() const
  {
    if (myProcessor == 0) {
      fmt::print(Ioss::OUTPUT(),
                 "\nMesh Parameters:\n"
                 "\tIntervals: {} by {} by {}\n"
                 "\tX = {} * (0..{}) + {}\tRange: {} <= X <= {}\n"
                 "\tY = {} * (0..{}) + {}\tRange: {} <= Y <= {}\n"
                 "\tZ = {} * (0..{}) + {}\tRange: {} <= Z <= {}\n\n"
                 "\tNode Count (total)    = {:12n}\n"
                 "\tElement Count (total) = {:12n}\n"
                 "\tBlock Count           = {:12n}\n"
                 "\tNodeSet Count         = {:12n}\n"
                 "\tSideSet Count         = {:12n}\n"
                 "\tTimestep Count        = {:12n}\n\n",
                 numX, numY, numZ, sclX, numX, offX, offX, offX + numX * sclX, sclY, numY, offY,
                 offY, offY + numY * sclY, sclZ, numZ, offZ, offZ, offZ + numZ * sclZ, node_count(),
                 element_count(), block_count(), nodeset_count(), sideset_count(),
                 timestep_count());

      if (doRotation) {
        fmt::print(Ioss::OUTPUT(), "\tRotation Matrix: \n\t");
        for (auto &elem : rotmat) {
          for (double jj : elem) {
            fmt::print(Ioss::OUTPUT(), "{:14.e}\t", jj);
          }
          fmt::print(Ioss::OUTPUT(), "\n\t");
        }
        fmt::print(Ioss::OUTPUT(), "\n");
      }
    }
  }

  int64_t GeneratedMesh::node_count() const { return (numX + 1) * (numY + 1) * (numZ + 1); }

  int64_t GeneratedMesh::node_count_proc() const { return (numX + 1) * (numY + 1) * (myNumZ + 1); }

  int64_t GeneratedMesh::block_count() const { return shellBlocks.size() + 1; }

  int64_t GeneratedMesh::nodeset_count() const { return nodesets.size(); }

  int64_t GeneratedMesh::sideset_count() const { return sidesets.size(); }

  int64_t GeneratedMesh::element_count() const
  {
    int64_t count = element_count(1);
    for (size_t i = 0; i < shellBlocks.size(); i++) {
      count += element_count(i + 2);
    }
    return count;
  }

  int64_t GeneratedMesh::element_count_proc() const
  {
    int64_t count = 0;
    for (int64_t i = 0; i < block_count(); i++) {
      count += element_count_proc(i + 1);
    }
    return count;
  }

  int64_t GeneratedMesh::element_count(int64_t block_number) const
  {
    assert(block_number <= block_count());

    if (block_number == 1) {
      int64_t mult = createTets ? 6 : 1;
      return mult * numX * numY * numZ;
    }
    ShellLocation loc = shellBlocks[block_number - 2];
    return shell_element_count(loc);
  }

  int64_t GeneratedMesh::shell_element_count(ShellLocation loc) const
  {
    int64_t mult = createTets ? 2 : 1;
    switch (loc) {
    case MX:
    case PX: return mult * numY * numZ;
    case MY:
    case PY: return mult * numX * numZ;
    case MZ:
    case PZ: return mult * numX * numY;
    }
    return 0;
  }

  int64_t GeneratedMesh::element_count_proc(int64_t block_number) const
  {
    assert(block_number <= block_count());

    if (block_number == 1) {
      int64_t mult = createTets ? 6 : 1;
      return mult * numX * numY * myNumZ;
    }
    ShellLocation loc = shellBlocks[block_number - 2];
    return shell_element_count_proc(loc);
  }

  int64_t GeneratedMesh::shell_element_count_proc(ShellLocation loc) const
  {
    int64_t mult = createTets ? 2 : 1;
    switch (loc) {
    case MX:
    case PX: return mult * numY * myNumZ;
    case MY:
    case PY: return mult * numX * myNumZ;
    case MZ:
      if (myProcessor == 0) {
        return mult * numX * numY;
      }
      else {
        return 0;
      }
    case PZ:
      if (myProcessor == processorCount - 1) {
        return mult * numX * numY;
      }
      else {
        return 0;
      }
    }
    return 0;
  }

  int64_t GeneratedMesh::nodeset_node_count(int64_t id) const
  {
    // id is position in nodeset list + 1
    assert(id > 0 && (size_t)id <= nodesets.size());
    ShellLocation loc = nodesets[id - 1];
    switch (loc) {
    case MX:
    case PX: return (numY + 1) * (numZ + 1);
    case MY:
    case PY: return (numX + 1) * (numZ + 1);
    case MZ:
    case PZ: return (numX + 1) * (numY + 1);
    }
    return 0;
  }

  int64_t GeneratedMesh::nodeset_node_count_proc(int64_t id) const
  {
    // id is position in nodeset list + 1
    assert(id > 0 && (size_t)id <= nodesets.size());
    ShellLocation loc = nodesets[id - 1];
    switch (loc) {
    case MX:
    case PX: return (numY + 1) * (myNumZ + 1);
    case MY:
    case PY: return (numX + 1) * (myNumZ + 1);
    case MZ:
      if (myProcessor == 0) {
        return (numX + 1) * (numY + 1);
      }
      else {
        return 0;
      }
    case PZ:
      if (myProcessor == processorCount - 1) {
        return (numX + 1) * (numY + 1);
      }
      else {
        return 0;
      }
    }
    return 0;
  }

  int64_t GeneratedMesh::sideset_side_count(int64_t id) const
  {
    // id is position in sideset list + 1
    int64_t mult = createTets ? 2 : 1;
    assert(id > 0 && (size_t)id <= sidesets.size());
    ShellLocation loc = sidesets[id - 1];
    switch (loc) {
    case MX:
    case PX: return mult * numY * numZ;
    case MY:
    case PY: return mult * numX * numZ;
    case MZ:
    case PZ: return mult * numX * numY;
    }
    return 0;
  }

  int64_t GeneratedMesh::sideset_side_count_proc(int64_t id) const
  {
    // id is position in sideset list + 1
    assert(id > 0 && (size_t)id <= sidesets.size());
    int64_t       mult = createTets ? 2 : 1;
    ShellLocation loc  = sidesets[id - 1];
    switch (loc) {
    case MX:
    case PX: return mult * numY * myNumZ;
    case MY:
    case PY: return mult * numX * myNumZ;
    case MZ:
      if (myProcessor == 0) {
        return mult * numX * numY;
      }
      else {
        return 0;
      }
    case PZ:
      if (myProcessor == processorCount - 1) {
        return mult * numX * numY;
      }
      else {
        return 0;
      }
    }
    return 0;
  }

  std::pair<std::string, int> GeneratedMesh::topology_type(int64_t block_number) const
  {
    assert(block_number <= block_count() && block_number > 0);

    if (createTets) {
      if (block_number == 1) {
        return std::make_pair(std::string(Ioss::Tet4::name), 4);
      }
      return std::make_pair(std::string(Ioss::TriShell3::name), 3);
    }

    if (block_number == 1) {
      return std::make_pair(std::string(Ioss::Hex8::name), 8);
    }
    return std::make_pair(std::string(Ioss::Shell4::name), 4);
  }

  void GeneratedMesh::node_map(Ioss::Int64Vector &map) const
  {
    map.resize(node_count_proc());
    int64_t offset = myStartZ * (numX + 1) * (numY + 1);
    std::iota(map.begin(), map.end(), offset + 1);
  }

  void GeneratedMesh::node_map(Ioss::IntVector &map) const
  {
    map.resize(node_count_proc());
    int offset = myStartZ * (numX + 1) * (numY + 1);
    std::iota(map.begin(), map.end(), offset + 1);
  }

  int64_t GeneratedMesh::communication_node_count_proc() const
  {
    int64_t count = (numX + 1) * (numY + 1);
    if (myProcessor != 0 && myProcessor != processorCount - 1) {
      count *= 2;
    }

    return count;
  }

  void GeneratedMesh::owning_processor(int *owner, int64_t num_node)
  {
    for (int64_t i = 0; i < num_node; i++) {
      owner[i] = myProcessor;
    }

    if (myProcessor != 0) {
      int64_t count = (numX + 1) * (numY + 1);
      for (int64_t i = 0; i < count; i++) {
        owner[i] = myProcessor - 1;
      }
    }
  }

  void GeneratedMesh::build_node_map(Ioss::Int64Vector &map, std::vector<int> &proc, int64_t slab,
                                     size_t slabOffset, size_t adjacentProc, size_t index)
  {
    int64_t offset = (myStartZ + slabOffset) * (numX + 1) * (numY + 1);
    for (int64_t i = 0; i < slab; i++) {
      map[index]    = offset + i + 1;
      proc[index++] = static_cast<int>(adjacentProc);
    }
  }

  void GeneratedMesh::node_communication_map(Ioss::Int64Vector &map, std::vector<int> &proc)
  {
    bool isFirstProc = myProcessor == 0;
    bool isLastProc  = myProcessor == processorCount - 1;

    int64_t count = (numX + 1) * (numY + 1);
    int64_t slab  = count;
    if (!isFirstProc && !isLastProc) {
      count *= 2;
    }
    map.resize(count);
    proc.resize(count);

    size_t offset = 0;
    if (!isFirstProc) {
      build_node_map(map, proc, slab, 0, myProcessor - 1, offset);
      offset += slab;
    }
    if (!isLastProc) {
      build_node_map(map, proc, slab, myNumZ, myProcessor + 1, offset);
    }
  }

  void GeneratedMesh::element_map(int64_t block_number, Ioss::Int64Vector &map) const
  {
    raw_element_map(block_number, map);
  }

  void GeneratedMesh::element_map(int64_t block_number, Ioss::IntVector &map) const
  {
    raw_element_map(block_number, map);
  }

  template <typename INT>
  void GeneratedMesh::raw_element_map(int64_t block_number, std::vector<INT> &map) const
  {
    assert(block_number <= block_count() && block_number > 0);

    INT count = element_count_proc(block_number);
    map.reserve(count);

    if (block_number == 1) {
      // Hex/Tet block...
      INT mult   = createTets ? 6 : 1;
      count      = element_count_proc(1);
      INT offset = mult * myStartZ * numX * numY;
      for (INT i = 0; i < count; i++) {
        map.push_back(offset + i + 1);
      }
    }
    else {
      INT start = element_count(1);

      // Shell blocks...
      for (INT ib = 0; (size_t)ib < shellBlocks.size(); ib++) {
        INT mult = createTets ? 2 : 1;
        count    = element_count_proc(ib + 2);
        if (block_number == ib + 2) {
          INT           offset = 0;
          ShellLocation loc    = shellBlocks[ib];

          switch (loc) {
          case MX:
          case PX: offset = mult * myStartZ * numY; break;

          case MY:
          case PY: offset = mult * myStartZ * numX; break;

          case MZ:
          case PZ: offset = 0; break;
          }
          for (INT i = 0; i < count; i++) {
            map.push_back(start + offset + i + 1);
          }
        }
        else {
          start += element_count(ib + 2);
        }
      }
    }
  }

  void GeneratedMesh::element_map(Ioss::Int64Vector &map) const { raw_element_map(map); }

  void GeneratedMesh::element_map(Ioss::IntVector &map) const { raw_element_map(map); }

  template <typename INT> void GeneratedMesh::raw_element_map(std::vector<INT> &map) const
  {
    INT mult  = createTets ? 6 : 1;
    INT count = element_count_proc();
    map.reserve(count);

    // Hex block...
    count      = element_count_proc(1);
    INT offset = mult * myStartZ * numX * numY;
    for (INT i = 0; i < count; i++) {
      map.push_back(offset + i + 1);
    }

    INT start = element_count(1);

    // Shell blocks...
    INT smult = createTets ? 2 : 1;
    for (INT ib = 0; (size_t)ib < shellBlocks.size(); ib++) {
      count             = element_count_proc(ib + 2);
      offset            = 0;
      ShellLocation loc = shellBlocks[ib];

      switch (loc) {
      case MX:
      case PX: offset = smult * myStartZ * numY; break;

      case MY:
      case PY: offset = smult * myStartZ * numX; break;

      case MZ:
      case PZ: offset = 0; break;
      }
      for (INT i = 0; i < count; i++) {
        map.push_back(start + offset + i + 1);
      }
      start += element_count(ib + 2);
    }
  }

  void GeneratedMesh::element_surface_map(ShellLocation loc, Ioss::Int64Vector &map) const
  {
    int64_t count = shell_element_count_proc(loc);
    map.resize(2 * count);
    int64_t index  = 0;
    int64_t offset = 0;

    if (createTets) {
      // For tet elements
      switch (loc) {
      case MX:
        offset = myStartZ * numX * numY + 1;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t j = 0; j < numY; ++j) {
            map[index++] = 6 * offset - 4; // 1-based elem id
            map[index++] = 3;              // 0-based local face id
            map[index++] = 6 * offset - 3;
            map[index++] = 3; // 0-based local face id
            offset += numX;
          }
        }
        break;

      case PX:
        offset = myStartZ * numX * numY + numX;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t j = 0; j < numY; ++j) {
            map[index++] = 6 * offset - 1; // 1-based elem id
            map[index++] = 3;              // 0-based local face id
            map[index++] = 6 * offset;     // 1-based elem id
            map[index++] = 3;              // 0-based local face id
            offset += numX;
          }
        }
        break;

      case MY:
        offset = myStartZ * numX * numY + 1;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t i = 0; i < numX; ++i) {
            map[index++] = 6 * offset - 2;   // 1-based elem id
            map[index++] = 0;                // 0-based local face id
            map[index++] = 6 * offset++ - 1; // 1-based elem id
            map[index++] = 0;                // 0-based local face id
          }
          offset += numX * (numY - 1);
        }
        break;

      case PY:
        offset = myStartZ * numX * numY + numX * (numY - 1) + 1;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t i = 0; i < numX; ++i) {
            map[index++] = 6 * offset - 5;
            map[index++] = 1; // 0-based local face id
            map[index++] = 6 * offset++ - 4;
            map[index++] = 1; // 0-based local face id
          }
          offset += numX * (numY - 1);
        }
        break;

      case MZ:
        if (myProcessor == 0) {
          offset = 1;
          for (size_t i = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++) {
              map[index++] = 6 * offset - 5;
              map[index++] = 3;
              map[index++] = 6 * offset++;
              map[index++] = 2;
            }
          }
        }
        break;

      case PZ:
        if (myProcessor == processorCount - 1) {
          offset = (numZ - 1) * numX * numY + 1;
          for (size_t i = 0, k = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++, k++) {
              map[index++] = 6 * offset - 3;
              map[index++] = 1;
              map[index++] = 6 * offset++ - 2;
              map[index++] = 1;
            }
          }
        }
        break;
      }
    }
    else {
      // For hex elements
      switch (loc) {
      case MX:
        offset = myStartZ * numX * numY + 1; // 1-based elem id
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t j = 0; j < numY; ++j) {
            map[index++] = offset;
            map[index++] = 3; // 0-based local face id
            offset += numX;
          }
        }
        break;

      case PX:
        offset = myStartZ * numX * numY + numX;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t j = 0; j < numY; ++j) {
            map[index++] = offset; // 1-based elem id
            map[index++] = 1;      // 0-based local face id
            offset += numX;
          }
        }
        break;

      case MY:
        offset = myStartZ * numX * numY + 1;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t i = 0; i < numX; ++i) {
            map[index++] = offset++;
            map[index++] = 0; // 0-based local face id
          }
          offset += numX * (numY - 1);
        }
        break;

      case PY:
        offset = myStartZ * numX * numY + numX * (numY - 1) + 1;
        for (size_t k = 0; k < myNumZ; ++k) {
          for (size_t i = 0; i < numX; ++i) {
            map[index++] = offset++;
            map[index++] = 2; // 0-based local face id
          }
          offset += numX * (numY - 1);
        }
        break;

      case MZ:
        if (myProcessor == 0) {
          offset = 1;
          for (size_t i = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++) {
              map[index++] = offset++;
              map[index++] = 4;
            }
          }
        }
        break;

      case PZ:
        if (myProcessor == processorCount - 1) {
          offset = (numZ - 1) * numX * numY + 1;
          for (size_t i = 0, k = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++, k++) {
              map[index++] = offset++;
              map[index++] = 5;
            }
          }
        }
        break;
      }
    }
  }

  void GeneratedMesh::coordinates(std::vector<double> &coord) const
  {
    /* create global coordinates */
    int64_t count = node_count_proc();
    coord.resize(count * 3);
    coordinates(&coord[0]);
  }

  void GeneratedMesh::coordinates(double *coord) const
  {
    /* create global coordinates */
    int64_t count = node_count_proc();

    int64_t k = 0;
    for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
      for (size_t i = 0; i < numY + 1; i++) {
        for (size_t j = 0; j < numX + 1; j++) {
          coord[k++] = sclX * static_cast<double>(j) + offX;
          coord[k++] = sclY * static_cast<double>(i) + offY;
          coord[k++] = sclZ * static_cast<double>(m) + offZ;
        }
      }
    }

    if (doRotation) {
      for (int64_t i = 0; i < count * 3; i += 3) {
        double xn    = coord[i + 0];
        double yn    = coord[i + 1];
        double zn    = coord[i + 2];
        coord[i + 0] = xn * rotmat[0][0] + yn * rotmat[1][0] + zn * rotmat[2][0];
        coord[i + 1] = xn * rotmat[0][1] + yn * rotmat[1][1] + zn * rotmat[2][1];
        coord[i + 2] = xn * rotmat[0][2] + yn * rotmat[1][2] + zn * rotmat[2][2];
      }
    }
  }

  void GeneratedMesh::coordinates(std::vector<double> &x, std::vector<double> &y,
                                  std::vector<double> &z) const
  {
    /* create global coordinates */
    int64_t count = node_count_proc();
    x.reserve(count);
    y.reserve(count);
    z.reserve(count);

    for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
      for (size_t i = 0; i < numY + 1; i++) {
        for (size_t j = 0; j < numX + 1; j++) {
          x.push_back(sclX * static_cast<double>(j) + offX);
          y.push_back(sclY * static_cast<double>(i) + offY);
          z.push_back(sclZ * static_cast<double>(m) + offZ);
        }
      }
    }
    if (doRotation) {
      for (int64_t i = 0; i < count; i++) {
        double xn = x[i];
        double yn = y[i];
        double zn = z[i];
        x.push_back(xn * rotmat[0][0] + yn * rotmat[1][0] + zn * rotmat[2][0]);
        y.push_back(xn * rotmat[0][1] + yn * rotmat[1][1] + zn * rotmat[2][1]);
        z.push_back(xn * rotmat[0][2] + yn * rotmat[1][2] + zn * rotmat[2][2]);
      }
    }
  }

  void GeneratedMesh::coordinates(int component, std::vector<double> &xyz) const
  {
    assert(!doRotation);
    /* create global coordinates */
    size_t count = node_count_proc();
    xyz.reserve(count);

    if (component == 1) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz.push_back(sclX * static_cast<double>(j) + offX);
          }
        }
      }
    }
    else if (component == 2) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz.push_back(sclY * static_cast<double>(i) + offY);
          }
        }
      }
    }
    else if (component == 3) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz.push_back(sclZ * static_cast<double>(m) + offZ);
          }
        }
      }
    }
  }

  void GeneratedMesh::coordinates(int component, double *xyz) const
  {
    assert(!doRotation);
    /* create global coordinates */
    size_t idx = 0;
    if (component == 1) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz[idx++] = sclX * static_cast<double>(j) + offX;
          }
        }
      }
    }
    else if (component == 2) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz[idx++] = sclY * static_cast<double>(i) + offY;
          }
        }
      }
    }
    else if (component == 3) {
      for (size_t m = myStartZ; m < myStartZ + myNumZ + 1; m++) {
        for (size_t i = 0; i < numY + 1; i++) {
          for (size_t j = 0; j < numX + 1; j++) {
            xyz[idx++] = sclZ * static_cast<double>(m) + offZ;
          }
        }
      }
    }
  }

  void GeneratedMesh::connectivity(int64_t block_number, Ioss::Int64Vector &connect) const
  {
    if (block_number == 1) { // HEX Element Block
      int64_t npe = createTets ? 4 : 8;
      connect.resize(element_count_proc(block_number) * npe);
    }
    else {
      int64_t npe = createTets ? 3 : 4;
      connect.resize(element_count_proc(block_number) * npe);
    }
    raw_connectivity(block_number, &connect[0]);
  }

  void GeneratedMesh::connectivity(int64_t block_number, Ioss::IntVector &connect) const
  {
    if (block_number == 1) { // HEX Element Block
      int64_t npe = createTets ? 4 : 8;
      connect.resize(element_count_proc(block_number) * npe);
    }
    else {
      int64_t npe = createTets ? 3 : 4;
      connect.resize(element_count_proc(block_number) * npe);
    }
    raw_connectivity(block_number, &connect[0]);
  }

  void GeneratedMesh::connectivity(int64_t block_number, int64_t *connect) const
  {
    raw_connectivity(block_number, connect);
  }

  void GeneratedMesh::connectivity(int64_t block_number, int *connect) const
  {
    raw_connectivity(block_number, connect);
  }

  template <typename INT>
  void GeneratedMesh::raw_connectivity(int64_t block_number, INT *connect) const
  {
    assert(block_number <= block_count());

    INT xp1yp1 = (numX + 1) * (numY + 1);

    /* build connectivity array (node list) for mesh */
    if (block_number == 1) { // main block elements

      if (createTets) {
        // Tet elements
        INT tet_vert[][4] = {{0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6},
                             {0, 5, 6, 4}, {1, 5, 6, 0}, {1, 6, 2, 0}};

        INT    hex_vert[8];
        size_t cnt = 0;
        for (size_t m = myStartZ; m < myNumZ + myStartZ; m++) {
          for (size_t i = 0, k = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++, k++) {
              size_t base = (m * xp1yp1) + k + i + 1;

              hex_vert[0] = base;
              hex_vert[1] = base + 1;
              hex_vert[2] = base + numX + 2;
              hex_vert[3] = base + numX + 1;

              hex_vert[4] = xp1yp1 + base;
              hex_vert[5] = xp1yp1 + base + 1;
              hex_vert[6] = xp1yp1 + base + numX + 2;
              hex_vert[7] = xp1yp1 + base + numX + 1;

              for (auto &elem : tet_vert) {
                connect[cnt++] = hex_vert[elem[0]];
                connect[cnt++] = hex_vert[elem[1]];
                connect[cnt++] = hex_vert[elem[2]];
                connect[cnt++] = hex_vert[elem[3]];
              }
            }
          }
        }
      }
      else {
        // Hex elements
        size_t cnt = 0;
        for (size_t m = myStartZ; m < myNumZ + myStartZ; m++) {
          for (size_t i = 0, k = 0; i < numY; i++) {
            for (size_t j = 0; j < numX; j++, k++) {
              size_t base = (m * xp1yp1) + k + i + 1;

              connect[cnt++] = base;
              connect[cnt++] = base + 1;
              connect[cnt++] = base + numX + 2;
              connect[cnt++] = base + numX + 1;

              connect[cnt++] = xp1yp1 + base;
              connect[cnt++] = xp1yp1 + base + 1;
              connect[cnt++] = xp1yp1 + base + numX + 2;
              connect[cnt++] = xp1yp1 + base + numX + 1;
            }
          }
        }
      }
    }
    else { // Shell blocks....
      ShellLocation loc = shellBlocks[block_number - 2];

      if (createTets) {
        // Tet shells
        size_t cnt           = 0;
        INT    tet_vert[][3] = {{0, 3, 2}, {0, 2, 1}};
        INT    hex_vert[4];
        switch (loc) {
        case MX: // Minimum X Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numY; j++) {
              size_t base = layer_off + j * (numX + 1) + 1 + myStartZ * xp1yp1;
              hex_vert[0] = base;
              hex_vert[1] = base + xp1yp1;
              hex_vert[2] = base + xp1yp1 + (numX + 1);
              hex_vert[3] = base + (numX + 1);

              for (auto &elem : tet_vert) {
                connect[cnt++] = hex_vert[elem[0]];
                connect[cnt++] = hex_vert[elem[1]];
                connect[cnt++] = hex_vert[elem[2]];
              }
            }
          }
          break;
        case PX: // Maximum X Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numY; j++) {
              size_t base = layer_off + j * (numX + 1) + numX + 1 + myStartZ * xp1yp1;
              hex_vert[0] = base;
              hex_vert[1] = base + (numX + 1);
              hex_vert[2] = base + xp1yp1 + (numX + 1);
              hex_vert[3] = base + xp1yp1;

              for (auto &elem : tet_vert) {
                connect[cnt++] = hex_vert[elem[0]];
                connect[cnt++] = hex_vert[elem[1]];
                connect[cnt++] = hex_vert[elem[2]];
              }
            }
          }
          break;
        case MY: // Minimum Y Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numX; j++) {
              size_t base = layer_off + j + 1 + myStartZ * xp1yp1;
              hex_vert[0] = base;
              hex_vert[1] = base + 1;
              hex_vert[2] = base + xp1yp1 + 1;
              hex_vert[3] = base + xp1yp1;

              for (auto &elem : tet_vert) {
                connect[cnt++] = hex_vert[elem[0]];
                connect[cnt++] = hex_vert[elem[1]];
                connect[cnt++] = hex_vert[elem[2]];
              }
            }
          }
          break;
        case PY: // Maximum Y Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numX; j++) {
              size_t base = layer_off + (numX + 1) * (numY) + j + 1 + myStartZ * xp1yp1;
              hex_vert[0] = base;
              hex_vert[1] = base + xp1yp1;
              hex_vert[2] = base + xp1yp1 + 1;
              hex_vert[3] = base + 1;

              for (auto &elem : tet_vert) {
                connect[cnt++] = hex_vert[elem[0]];
                connect[cnt++] = hex_vert[elem[1]];
                connect[cnt++] = hex_vert[elem[2]];
              }
            }
          }
          break;
        case MZ: // Minimum Z Face
          if (myProcessor == 0) {
            for (size_t i = 0, k = 0; i < numY; i++) {
              for (size_t j = 0; j < numX; j++, k++) {
                size_t base = i + k + 1 + myStartZ * xp1yp1;
                hex_vert[0] = base;
                hex_vert[1] = base + numX + 1;
                hex_vert[2] = base + numX + 2;
                hex_vert[3] = base + 1;

                for (auto &elem : tet_vert) {
                  connect[cnt++] = hex_vert[elem[0]];
                  connect[cnt++] = hex_vert[elem[1]];
                  connect[cnt++] = hex_vert[elem[2]];
                }
              }
            }
          }
          break;
        case PZ: // Maximum Z Face
          if (myProcessor == processorCount - 1) {
            for (size_t i = 0, k = 0; i < numY; i++) {
              for (size_t j = 0; j < numX; j++, k++) {
                size_t base = xp1yp1 * (numZ - myStartZ) + k + i + 1 + myStartZ * xp1yp1;
                hex_vert[0] = base;
                hex_vert[1] = base + 1;
                hex_vert[2] = base + numX + 2;
                hex_vert[3] = base + numX + 1;

                for (auto &elem : tet_vert) {
                  connect[cnt++] = hex_vert[elem[0]];
                  connect[cnt++] = hex_vert[elem[1]];
                  connect[cnt++] = hex_vert[elem[2]];
                }
              }
            }
          }
          break;
        }
      }
      else {
        // Hex shells
        size_t cnt = 0;
        switch (loc) {
        case MX: // Minimum X Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numY; j++) {
              size_t base    = layer_off + j * (numX + 1) + 1 + myStartZ * xp1yp1;
              connect[cnt++] = base;
              connect[cnt++] = base + xp1yp1;
              connect[cnt++] = base + xp1yp1 + (numX + 1);
              connect[cnt++] = base + (numX + 1);
            }
          }
          break;
        case PX: // Maximum X Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numY; j++) {
              size_t base    = layer_off + j * (numX + 1) + numX + 1 + myStartZ * xp1yp1;
              connect[cnt++] = base;
              connect[cnt++] = base + (numX + 1);
              connect[cnt++] = base + xp1yp1 + (numX + 1);
              connect[cnt++] = base + xp1yp1;
            }
          }
          break;
        case MY: // Minimum Y Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numX; j++) {
              size_t base    = layer_off + j + 1 + myStartZ * xp1yp1;
              connect[cnt++] = base;
              connect[cnt++] = base + 1;
              connect[cnt++] = base + xp1yp1 + 1;
              connect[cnt++] = base + xp1yp1;
            }
          }
          break;
        case PY: // Maximum Y Face
          for (size_t i = 0; i < myNumZ; i++) {
            size_t layer_off = i * xp1yp1;
            for (size_t j = 0; j < numX; j++) {
              size_t base    = layer_off + (numX + 1) * (numY) + j + 1 + myStartZ * xp1yp1;
              connect[cnt++] = base;
              connect[cnt++] = base + xp1yp1;
              connect[cnt++] = base + xp1yp1 + 1;
              connect[cnt++] = base + 1;
            }
          }
          break;
        case MZ: // Minimum Z Face
          if (myProcessor == 0) {
            for (size_t i = 0, k = 0; i < numY; i++) {
              for (size_t j = 0; j < numX; j++, k++) {
                size_t base    = i + k + 1 + myStartZ * xp1yp1;
                connect[cnt++] = base;
                connect[cnt++] = base + numX + 1;
                connect[cnt++] = base + numX + 2;
                connect[cnt++] = base + 1;
              }
            }
          }
          break;
        case PZ: // Maximum Z Face
          if (myProcessor == processorCount - 1) {
            for (size_t i = 0, k = 0; i < numY; i++) {
              for (size_t j = 0; j < numX; j++, k++) {
                size_t base    = xp1yp1 * (numZ - myStartZ) + k + i + 1 + myStartZ * xp1yp1;
                connect[cnt++] = base;
                connect[cnt++] = base + 1;
                connect[cnt++] = base + numX + 2;
                connect[cnt++] = base + numX + 1;
              }
            }
          }
          break;
        }
        assert((cnt == size_t(4 * element_count_proc(block_number)) && !createTets) ||
               (cnt == size_t(3 * element_count_proc(block_number)) && createTets));
      }
    }
    return;
  }

  void GeneratedMesh::nodeset_nodes(int64_t id, Ioss::Int64Vector &nodes) const
  {
    // id is position in nodeset list + 1
    assert(id > 0 && (size_t)id <= nodesets.size());
    ShellLocation loc = nodesets[id - 1];
    nodes.resize(nodeset_node_count_proc(id));

    size_t xp1yp1 = (numX + 1) * (numY + 1);
    size_t k      = 0;

    switch (loc) {
    case MX: // Minimum X Face
      for (size_t i = 0; i < myNumZ + 1; i++) {
        size_t layer_off = myStartZ * xp1yp1 + i * xp1yp1;
        for (size_t j = 0; j < numY + 1; j++) {
          nodes[k++] = layer_off + j * (numX + 1) + 1;
        }
      }
      break;
    case PX: // Maximum X Face
      for (size_t i = 0; i < myNumZ + 1; i++) {
        size_t layer_off = myStartZ * xp1yp1 + i * xp1yp1;
        for (size_t j = 0; j < numY + 1; j++) {
          nodes[k++] = layer_off + j * (numX + 1) + numX + 1;
        }
      }
      break;
    case MY: // Minimum Y Face
      for (size_t i = 0; i < myNumZ + 1; i++) {
        size_t layer_off = myStartZ * xp1yp1 + i * xp1yp1;
        for (size_t j = 0; j < numX + 1; j++) {
          nodes[k++] = layer_off + j + 1;
        }
      }
      break;
    case PY: // Maximum Y Face
      for (size_t i = 0; i < myNumZ + 1; i++) {
        size_t layer_off = myStartZ * xp1yp1 + i * xp1yp1;
        for (size_t j = 0; j < numX + 1; j++) {
          nodes[k++] = layer_off + (numX + 1) * (numY) + j + 1;
        }
      }
      break;
    case MZ: // Minimum Z Face
      if (myProcessor == 0) {
        for (size_t i = 0; i < (numY + 1) * (numX + 1); i++) {
          nodes[i] = i + 1;
        }
      }
      break;
    case PZ: // Maximum Z Face
      if (myProcessor == processorCount - 1) {
        size_t offset = (numY + 1) * (numX + 1) * numZ;
        for (size_t i = 0; i < (numY + 1) * (numX + 1); i++) {
          nodes[i] = offset + i + 1;
        }
      }
      break;
    }
  }

  std::string GeneratedMesh::get_sideset_topology() const { return createTets ? "tri3" : "quad4"; }

  void GeneratedMesh::sideset_elem_sides(int64_t id, Ioss::Int64Vector &elem_sides) const
  {
    // id is position in sideset list + 1
    assert(id > 0 && (size_t)id <= sidesets.size());
    ShellLocation loc = sidesets[id - 1];

    // If there is a shell block on this face, then the sideset is
    // applied to the shell block; if not, it is applied to the
    // underlying hex elements.
    bool    underlying_shell = false;
    int64_t shell_block      = 0;
    for (size_t i = 0; i < shellBlocks.size(); i++) {
      if (shellBlocks[i] == loc) {
        underlying_shell = true;
        shell_block      = i + 2;
        break;
      }
    }

    if (underlying_shell) {
      // Get ids of shell elements at this location...
      element_map(shell_block, elem_sides);

      // Insert face_ordinal in between each entry in elem_sides...
      // Face will be 0 for all shells...
      elem_sides.resize(2 * sideset_side_count_proc(id));
      ssize_t face_ordinal = 0;
      ssize_t i            = 2 * sideset_side_count_proc(id) - 1;
      ssize_t j            = sideset_side_count_proc(id) - 1;
      while (i >= 0) {
        elem_sides[i--] = face_ordinal;
        elem_sides[i--] = elem_sides[j--];
      }
    }
    else {
      element_surface_map(loc, elem_sides);
    }
  }

  std::vector<std::string> GeneratedMesh::sideset_touching_blocks(int64_t /*set_id*/) const
  {
    std::vector<std::string> result(1, "block_1");
    return result;
  }

  void GeneratedMesh::set_variable_count(const std::string &type, size_t count)
  {
    if (type == "global") {
      variableCount[Ioss::REGION] = count;
    }
    else if (type == "element") {
      variableCount[Ioss::ELEMENTBLOCK] = count;
    }
    else if (type == "nodal" || type == "node") {
      variableCount[Ioss::NODEBLOCK] = count;
    }
    else if (type == "nodeset") {
      variableCount[Ioss::NODESET] = count;
    }
    else if (type == "surface" || type == "sideset") {
      variableCount[Ioss::SIDEBLOCK] = count;
    }
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iogn::GeneratedMesh::set_variable_count)\n"
                 "       Unrecognized variable type '{}'. Valid types are:\n"
                 "       global, element, node, nodal, nodeset, surface, sideset.\n",
                 type);
      IOSS_ERROR(errmsg);
    }
  }

  void GeneratedMesh::set_rotation(const std::string &axis, double angle_degrees)
  {
    // PI / 180. Used in converting angle in degrees to radians
    static double degang = std::atan2(0.0, -1.0) / 180.0;

    doRotation = true;

    int n1 = -1;
    int n2 = -1;
    int n3 = -1;

    if (axis == "x" || axis == "X") {
      n1 = 1;
      n2 = 2;
      n3 = 0;
    }
    else if (axis == "y" || axis == "Y") {
      n1 = 2;
      n2 = 0;
      n3 = 1;
    }
    else if (axis == "z" || axis == "Z") {
      n1 = 0;
      n2 = 1;
      n3 = 2;
    }
    else {
      fmt::print(Ioss::WARNING(),
                 "\nInvalid axis specification '{}'. Valid options are 'x', 'y', or 'z'\n", axis);
      return;
    }

    double ang    = angle_degrees * degang; // Convert angle in degrees to radians
    double cosang = std::cos(ang);
    double sinang = std::sin(ang);

    assert(n1 >= 0 && n2 >= 0 && n3 >= 0);
    std::array<std::array<double, 3>, 3> by;
    by[n1][n1] = cosang;
    by[n2][n1] = -sinang;
    by[n1][n3] = 0.0;
    by[n1][n2] = sinang;
    by[n2][n2] = cosang;
    by[n2][n3] = 0.0;
    by[n3][n1] = 0.0;
    by[n3][n2] = 0.0;
    by[n3][n3] = 1.0;

    std::array<std::array<double, 3>, 3> res;
    for (int i = 0; i < 3; i++) {
      res[i][0] = rotmat[i][0] * by[0][0] + rotmat[i][1] * by[1][0] + rotmat[i][2] * by[2][0];
      res[i][1] = rotmat[i][0] * by[0][1] + rotmat[i][1] * by[1][1] + rotmat[i][2] * by[2][1];
      res[i][2] = rotmat[i][0] * by[0][2] + rotmat[i][1] * by[1][2] + rotmat[i][2] * by[2][2];
    }
    rotmat = res;
  }
} // namespace Iogn
