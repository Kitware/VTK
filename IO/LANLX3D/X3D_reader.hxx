// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright (c) 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
   \file X3D_reader.hxx

   Read X3D file by block and return data in standard library types.

   An X3D file consists of a fixed sequence of blocks.  An X3D block
   consists of a sequence of fixed, parameterized lines, or a sequence
   of blocks which consist of a fixed sequence of lines.  An X3D line
   consists of a fixed sequence of tokens.  An X3D token is specified
   by a FORTRAN data or control descriptor.

   See: G. A. Hansen, "Summary of the FLAG X3D Format", V 1.0,
        LA-UR-04-9033, 2005-1-14
        Brian Jean, "Summary of the FLAG X3D Format", V 1.3,
        2008-2-11

   \author Mark G. Gray <gray@lanl.gov>
*/

#ifndef X3D_READER_HXX
#define X3D_READER_HXX

#include "X3D.hxx"
#include "vtkABINamespace.h"

#include <fstream>
#include <stdexcept>

namespace X3D
{
VTK_ABI_NAMESPACE_BEGIN

/**
   \class ReadError

   Exception thrown by Reader.

   When Reader encounters an inconsistency in its input stream,
   it throws this exception with a message containing what it
   expected, what it found, and where (file or block name: file byte
   offset) it found the discrepancy.
*/
class ReadError : public std::runtime_error
{
public:
  explicit ReadError(const std::string& m)
    : std::runtime_error(m.c_str())
  {
  }
  explicit ReadError(const std::string& expect, const std::string& found, const std::string& where)
    : std::runtime_error(error_message(expect, found, where).c_str())
  {
  }
  explicit ReadError(int expect, int found, const std::string& where)
    : std::runtime_error(error_message(expect, found, where).c_str())
  {
  }
};

/**
   \class Reader

   Provide C++ STL representation of X3D file.

   Member functions named after X3D top level blocks seek that block
   in file, read its contents, and return STL based container with
   the block's data.  Block data may be accessed in any order.
*/
class Reader
{
public:
  /**
      Initialize Reader from named file.

      Open the named X3D file, index the location of its top level
      blocks, and read and store its header block.

      Supports both version 1.0 X3D files with "All the columns for
      face data must appear on a single line.", and version 1.3 X3D
      files with "...the maximum number of columns per physical line
      is 13."

      \param filename name of X3D file
      \param version number
  */
  explicit Reader(const std::string& filename, Version version = Version::v1_3);

  // Header Data Block
  Header header() const { return size; }

  // Material Data Blocks
  Materials matnames() { return materials("matnames"); }
  Materials mateos() { return materials("mateos"); }
  Materials matopc() { return materials("matopc"); }

  // Coordinate Data Block
  Nodes nodes();

  // Faces Data Block
  Faces faces();

  // Cell Block
  Cells cells();
  int number_of_cells() const;

  // Slaved Node Block
  ConstrainedNodes constrained_nodes();

  // Shared Nodes Block
  SharedNodes shared_nodes();

  // Cell Data Block
  CellData cell_data();

  // Point-centered Physical Data Block
  NodeData node_data();

  static const char* const pythonName;

private:
  std::string expect_starts_with(const std::string& s);
  std::streampos offset_of(const std::string& block);
  Materials materials(const std::string& s);
  typedef std::map<std::string, std::streampos> Offset;

  std::string filename; // name of X3D file to read
  Version version;      // X3D format version to process
  std::ifstream file;   // stream to read from
  Offset offset;        // file offsets to top blocks
  Header size;          // Header Block sizes
  Faces all_faces;
  bool faces_read;
};
VTK_ABI_NAMESPACE_END
}
#endif
