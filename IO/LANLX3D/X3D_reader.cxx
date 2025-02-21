// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright (c) 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
#include "X3D_reader.hxx"
#include "X3D_tokens.hxx"

#include <iostream>

using namespace std;

namespace X3D
{
VTK_ABI_NAMESPACE_BEGIN
// Does the start of s1 match s2?  Coming in C++20!
inline bool starts_with(const string& s1, const string& s2)
{
  return 0 == s1.compare(0, s2.size(), s2);
}

// Get next line from file and match against s or throw ReadError.
string Reader::expect_starts_with(const string& s)
{
  string line;

  getline(file, line);
  if (starts_with(line, s))
  {
    return line;
  }
  else // unexpected block begin/end
  {
    throw ReadError(s, line, filename + ": " + to_string(file.tellg()));
  }
}

// Return offset of block header in file using just-in-time search.
// A valid block header in valid X3D file must have been either
// previously cached, or in remainder of file not yet searched
// for headers.  In the latter case cache more headers until it is
// found.
streampos Reader::offset_of(const string& block)
{
  if (offset.find(block) == offset.end())
  { // block not cached
    size_t num_cached_blocks = offset.size();
    if (num_cached_blocks)
    { // some blocks cached; start after them...
      string const& last_block = TOP_BLOCK[num_cached_blocks - 1];
      if (file.tellg() < offset.at(last_block)) // ...unless beyond them
        file.seekg(offset.at(last_block));      // move to last cached block
    }
    for (size_t i = num_cached_blocks; i < TOP_BLOCK.size(); i++)
    {
      string const& next_block = TOP_BLOCK[i]; // look for next uncached block
      streampos position;
      while ((position = file.tellg()) > 0)
      {
        string line;
        getline(file, line);
        if (starts_with(line, next_block))
        {                                // found next block header
          offset[next_block] = position; // add to cache
          if (next_block == block)
          {
            return position; // found what we're looking for
          }
          else
          {
            break; // look for next block
          }
        }
      }
    }

    // read all headers or EOF w/o finding block
    throw ReadError(block, "EOF", filename);
  }
  return offset.at(block); // block cached
}

// Construct X3D Reader from data on filename
Reader::Reader(const string& filename_, const Version version_)
  : filename(filename_)
  , version(version_)
  , faces_read(false)
{
  // Open file and read magic string
  file.exceptions(ifstream::badbit); // Exception on filesystem errors
  file.open(filename);
  if (!file.is_open())
    throw ReadError("Error opening file: " + filename);

  expect_starts_with(MAGIC_STRING); // match X3D header line

  // Read Header Block
  string const& block = TOP_BLOCK[0];
  file.seekg(offset_of(block));
  expect_starts_with(block);

  Xformat x3(3);
  Aformat a23(23);
  Iformat i10(10);
  for (auto const& key : HEADER_KEYS)
  {
    file >> x3 >> a23 >> i10 >> eat_endl; // (3X, A23, I10)
    if (key != a23())                     // unexpected key
      throw ReadError(key, a23(), block + ": " + to_string(file.tellg()));
    size[key] = i10();
  }
  expect_starts_with("end_" + block);
}

// Read Materials Data Blocks: "matnames", "mateos", or "matopc"
Materials Reader::materials(const string& block)
{
  Materials m;
  Xformat x3(3);
  Iformat i10(10);
  Aformat a;

  file.seekg(offset_of(block));
  expect_starts_with(block);
  int num_materials = size.at("materials");
  for (int i = 0; i < num_materials; i++)
  {
    file >> x3 >> i10 >> x3 >> a >> eat_endl; // (3X, I10, 3X, A)
    if (i + 1 != i10())                       // unexpected material id
      throw ReadError(i + 1, i10(), block + ": " + to_string(file.tellg()));
    m.push_back(a());
  }
  expect_starts_with("end_" + block);
  return m;
}

// Read Nodes Block, a.k.a. coordinate data
Nodes Reader::nodes()
{
  Nodes n;
  string block("nodes");
  Iformat i10(10);
  Xformat x1(1);
  PEformat pe22_14(22, 14);

  file.seekg(offset_of(block));
  expect_starts_with(block);
  int num_nodes = size.at(block);
  for (int i = 0; i < num_nodes; i++)
  {                     // (i10, 3(1PE22.14))
    file >> i10;        // node id
    if (i + 1 != i10()) // unexpected node id
      throw ReadError(i + 1, i10(), block + ": " + to_string(file.tellg()));
    Node vec;
    for (unsigned int j = 0; j < vec.size(); j++)
    { // node coordinates
      file >> x1 >> pe22_14;
      vec[j] = pe22_14();
    }
    file >> eat_endl;
    n.push_back(vec);
  }
  expect_starts_with("end_" + block);
  return n;
}

// Read Faces Data Block
Faces Reader::faces()
{
  if (faces_read)
    return all_faces;

  faces_read = true;
  string block("faces");
  Iformat i10(10);
  Rformat rn(version == Version::v1_3 ? 13 : 0);

  file.seekg(offset_of(block));
  expect_starts_with(block);
  int num_faces = size.at(block);
  int this_process_id = size.at("process");
  for (int i = 0; i < num_faces; i++)
  { // ((2+num_nodes)I10)
    rn.reset();
    Face fl;
    file >> i10 >> rn;  // face id
    if (i + 1 != i10()) // unexpected face id
      throw ReadError(i + 1, i10(), block + ": " + to_string(file.tellg()));
    fl.face_id = i + 1;
    file >> i10 >> rn; // number of face nodes
    int num_nodes = i10();
    for (int j = 0; j < num_nodes; j++)
    { // node ids
      file >> i10 >> rn;
      fl.node_id.push_back(i10());
    }
    file >> i10 >> rn;
    if (this_process_id != i10()) // unexpected process id
      throw ReadError(this_process_id, i10(), block + ": " + to_string(file.tellg()));
    file >> i10 >> rn;
    fl.neighbor_process_id = i10();
    file >> i10 >> rn;
    fl.neighbor_face_id = i10();
    for (int j = 0; j < 5; j++) // discard five ones of "no significance"
      file >> i10 >> rn;
    if (rn())
      file >> eat_endl; // eat newline
    all_faces.push_back(fl);
  }
  expect_starts_with("end_" + block);
  return all_faces;
}

int Reader::number_of_cells() const
{
  return size.at("elements");
}

// Read Cells Block
Cells Reader::cells()
{
  Cells c;
  string block("cells");
  Iformat i10(10);

  file.seekg(offset_of(block));
  expect_starts_with(block);
  // N.B. X3D inconsistency: block="cells", num_cells = size["elements"]
  int num_cells = size.at("elements");
  for (int i = 0; i < num_cells; i++)
  { // ((2+num_faces)(I10))
    file >> i10;
    if (i + 1 != i10()) // unexpected element id
      throw ReadError(i + 1, i10(), block + ": " + to_string(file.tellg()));
    file >> i10;
    int num_faces = i10();
    vector<int> cl;
    for (int j = 0; j < num_faces; j++)
    {
      file >> i10;
      cl.push_back(i10());
    }
    file >> eat_endl;
    c.push_back(cl);
  }
  expect_starts_with("end_" + block);
  return c;
}

// Read Constrained Nodes Block
ConstrainedNodes Reader::constrained_nodes()
{
  ConstrainedNodes s;
  string block("slaved_nodes");
  Aformat a12(12);
  Iformat i10(10);

  file.seekg(offset_of(block));
  file >> a12 >> i10 >> eat_endl;
  if (a12() != block)
    throw ReadError(block, a12(), filename + ": " + to_string(file.tellg()));
  int num_lines = size.at(block);
  if (i10() != num_lines)
    throw ReadError(num_lines, i10(), block);
  for (int i = 0; i < num_lines; i++)
  { // ((3+num_masters)I10)
    ConstrainedNode sl;
    file >> i10;
    if (i + 1 != i10()) // unexpected constrained node id
      throw ReadError(i + 1, i10(), block + ": " + to_string(file.tellg()));
    file >> i10;
    sl.vertex_id = i10();
    file >> i10;
    int num_masters = i10();
    for (int j = 0; j < num_masters; j++)
    {
      file >> i10;
      sl.master.push_back(i10());
    }
    file >> eat_endl;
    s.push_back(sl);
  }
  expect_starts_with("end_" + block);
  return s;
}

// Read Parallel Shared Nodes Block
SharedNodes Reader::shared_nodes()
{
  SharedNodes s;
  string block("ghost_nodes");
  Aformat a12(12);
  Iformat i10(10);

  auto offs = offset_of(block);
  file.seekg(offs);
  file >> a12 >> i10 >> eat_endl;
  if (a12() != block)
    throw ReadError(block, a12(), filename + ": " + to_string(file.tellg()));
  int num_lines = size.at(block);
  if (i10() != num_lines)
    throw ReadError(num_lines, i10(), block);
  for (int i = 0; i < num_lines; i++)
  { // (4I10)
    SharedNode sl;
    for (unsigned int j = 0; j < sl.size(); j++)
    {
      file >> i10;
      sl[j] = i10();
    }
    file >> eat_endl;
    s.push_back(sl);
  }
  expect_starts_with("end_" + block);
  return s;
}

// Read Cell Data Block
CellData Reader::cell_data()
{
  CellData cd;
  string block("cell_data");
  Aformat a;
  Iformat i10(10); // I10
  Rformat r10(10); // 10 per line
  PEformat pe20_12(20, 12);
  int num_fields = size.at(block + "_fields");
  int num_elements = size.at("elements");

  file.seekg(offset_of(block));
  expect_starts_with(block);
  for (int i = 0; i < num_fields; i++)
  {
    file >> a >> eat_endl; // get field name
    string field_name(a());
    if (field_name == "matid" || field_name == "partelm")
    { // mandatory field
      r10.reset();
      vector<int> f;
      for (int j = 0; j < num_elements; j++)
      {                     // (10I10)
        file >> i10 >> r10; // read integer from line
        f.push_back(i10()); // and add to field
      }
      if (r10())
        file >> eat_endl; // eat EOL from partial last line
      if (field_name == "matid")
        cd.matid = f; // add matid to struct
      else
        cd.partelm = f; // add partelm to struct
    }
    else
    { // get optional zone-centered field
      vector<double> f;
      for (int j = 0; j < num_elements; j++)
      {                              //  (1PE20.12)
        file >> pe20_12 >> eat_endl; // read double from line
        f.push_back(pe20_12());      // and add to field
      }
      cd.fields[field_name] = f; // add field to map
    }
    expect_starts_with("end_" + field_name);
    cd.names.push_back(field_name); // add to names vector
  }
  expect_starts_with("end_" + block);
  return cd;
}

// Read Node Data Block, a.k.a. Point-centered Physical Data Block
NodeData Reader::node_data()
{
  NodeData nd;
  string block("node_data");
  Aformat a;
  PEformat pe20_12(20, 12); // 1PE20.12
  int num_fields = size.at(block + "_fields");
  int num_nodes = size.at("nodes");

  file.seekg(offset_of(block));
  expect_starts_with(block);

  for (int i = 0; i < num_fields; i++)
  {
    // get field block
    file >> a >> eat_endl;
    string field_block(a());
    nd.names.push_back(field_block);
    // read vector field in f
    vector<Node> field;
    for (int j = 0; j < num_nodes; j++)
    {
      Node v;
      // read vector from line
      for (unsigned int k = 0; k < v.size(); k++)
      { // (3(1PE20.12))
        file >> pe20_12;
        v[k] = pe20_12();
      }
      file >> eat_endl;
      field.push_back(v); // add vector to field
    }
    expect_starts_with("end_" + field_block);
    nd.fields[field_block] = field; // add field to map
  }
  expect_starts_with("end_" + block);
  return nd;
}
VTK_ABI_NAMESPACE_END
}
