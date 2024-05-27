// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOBJReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFileResourceStream.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkResourceParser.h"
#include "vtkStringArray.h"

#include <cctype>
#include <sstream>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOBJReader);

//------------------------------------------------------------------------------
vtkOBJReader::vtkOBJReader()
{
  this->Comment = nullptr;
}

//------------------------------------------------------------------------------
vtkOBJReader::~vtkOBJReader()
{
  this->SetComment(nullptr);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkOBJReader::Open()
{
  if (this->Stream)
  {
    if (this->Stream->SupportSeek())
    {
      this->Stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
    }

    return this->Stream;
  }

  auto fileStream = vtkSmartPointer<vtkFileResourceStream>::New();
  if (!this->FileName || !fileStream->Open(this->FileName))
  {
    vtkErrorMacro(<< "Failed to open file: "
                  << (this->FileName ? this->FileName : "No file name set"));
    return nullptr;
  }

  return fileStream;
}

/*---------------------------------------------------------------------------*\

This is only partial support for the OBJ format, which is quite complicated.
To find a full specification, search the net for "OBJ format", eg.:

    https://en.wikipedia.org/wiki/Wavefront_.obj_file
    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm
    http://paulbourke.net/dataformats/obj/

We support the following types:

g <groupName>  [... <groupNameN]

    group name, primarily for faces

v <x> <y> <z>

    vertex

vn <x> <y> <z>

    vertex normal

vt <x> <y>

    texture coordinate
    note: vt are globally indexed, see "Referencing vertex data" section
    of Paul Bourke format description.

f <v_a> <v_b> <v_c> ...

    polygonal face linking vertices v_a, v_b, v_c, etc. which
    are 1-based indices into the vertex list

f <v_a>/<t_a> <v_b>/<t_b> ...

    polygonal face as above, but with texture coordinates for
    each vertex. t_a etc. are 1-based indices into the texture
    coordinates list (from the vt lines)

f <v_a>/<t_a>/<n_a> <v_b>/<t_b>/<n_b> ...

    polygonal face as above, with a normal at each vertex, as a
    1-based index into the normals list (from the vn lines)

f <v_a>//<n_a> <v_b>//<n_b> ...

    polygonal face as above but without texture coordinates.

    Per-face tcoords and normals are supported by duplicating
    the vertices on each face as necessary.

l <v_a> <v_b> ...

    lines linking vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

p <v_a> <v_b> ...

    points located at the vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

\*---------------------------------------------------------------------------*/

int vtkOBJReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Failed to open stream");
    return 0;
  }

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);
  parser->StopOnNewLineOn();

  const std::string noMaterialName = "NO_MATERIAL";

  // Vertices ("v")
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  // Vertex tcoords ("vt") use vtkSmartPointer because it may be replaced later
  auto tcoords = vtkSmartPointer<vtkFloatArray>::New();
  tcoords->SetNumberOfComponents(2);
  // Vertex normals ("vt") use vtkSmartPointer because it may be replaced later
  auto normals = vtkSmartPointer<vtkFloatArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetName("Normals");

  // Cells (faces="f")
  // OBJ format enables indexing points, normals and tcoords independently from each other
  // while VTK cells index both the points, normals and tcoords with the same indices.
  // We may need to duplicate data to ensure that the output polydata is complete and valid.
  // To do this we store each index independently and check them later.
  auto vertexPolys = vtkSmartPointer<vtkCellArray>::New();
  vtkNew<vtkCellArray> tcoordPolys;
  bool tcoordsMatchVertices = true;
  vtkNew<vtkCellArray> normalPolys;
  bool normalsMatchVertices = true;
  // Points ("p")
  vtkNew<vtkCellArray> pointElems;
  // Lines ("l")
  vtkNew<vtkCellArray> lineElems;

  // Cell group ID
  vtkNew<vtkFloatArray> faceScalars;
  faceScalars->SetNumberOfComponents(1);
  faceScalars->SetName("GroupIds");
  // Cell material ID
  vtkNew<vtkIntArray> materialIds;
  materialIds->SetNumberOfComponents(1);
  materialIds->SetName("MaterialIds");
  // Field material name
  vtkNew<vtkStringArray> materialNames;
  materialNames->SetName("MaterialNames");
  materialNames->SetNumberOfComponents(1);
  // Field material library (mtl) name
  vtkNew<vtkStringArray> libNames;
  libNames->SetName("MaterialLibraries");
  libNames->SetNumberOfComponents(1);

  // Map between materialIds and materialNames
  std::unordered_map<std::string, int> materialNameToId;
  // Map between cells id to material name
  std::unordered_map<vtkIdType, std::string> startCellToMaterialName;
  // For each material, store in a dynamic bitset used tcoords indices.
  // Bitsets are used because each material uses range of tcoords,
  // but this range is not always contiguous.
  // Real tcoords arrays are generated at the end by combining `tcoordsMap` and `tcoords`.
  std::unordered_map<std::string, std::vector<bool>> tcoordsMap;

  // Handling of "g" grouping
  int groupId = -1;
  int materialCount = 0;
  bool cellWithNotTextureFound = false;

  // work through the file line by line, assigning into the above structures as appropriate
  std::string command;      // the command, may be a comment
  std::string firstComment; // the first comment is stored
  int firstCommentLineCount = 0;

  std::string tcoordsName; // name of active tcoords
  int lineNumber = 0;      // current line number

  const auto flushLine = [this, &parser, &lineNumber]() {
    std::string remaining;

    auto result = parser->Parse(remaining);
    if (result != vtkParseResult::EndOfLine)
    {
      vtkWarningMacro(<< "unexpected data at end of line in OBJ file L." << lineNumber);
      result = parser->DiscardLine();
    }

    return result;
  };

  vtkParseResult result = vtkParseResult::Ok;
  while (result == vtkParseResult::Ok || result == vtkParseResult::EndOfLine)
  {
    ++lineNumber;

    result = parser->Parse(command);
    if (result != vtkParseResult::Ok)
    {
      continue; // let loop check
    }

    if (command[0] == '#') // comment
    {
      ++firstCommentLineCount;
      if (firstCommentLineCount == lineNumber) // store comment on first lines
      {
        if (command != "#") // first word is right next to #
        {
          firstComment +=
            command.substr(1); // drop # but keep potential first word e.g. #comment like this
        }
        else
        {
          // Otherwise remove leading blankspaces
          result = parser->DiscardUntil(
            [](char c) { return !std::isblank(static_cast<unsigned char>(c)); });
          if (result != vtkParseResult::Ok)
          {
            continue;
          }
        }

        std::string line;
        result = parser->ReadLine(line);
        if (result != vtkParseResult::EndOfLine)
        {
          continue;
        }

        firstComment += line; // read all first comments
        firstComment += '\n'; // resource parser consumed the newline marker
      }
      else
      {
        result = parser->DiscardLine();
      }
    }
    else if (command == "g")
    {
      // group definition, expect 0 or more words separated by whitespace.
      // But here we simply note its existence, without a name
      ++groupId;
      result = parser->DiscardLine(); // ignore group name
    }
    else if (command == "usemtl")
    {
      // material name (for texture coordinates), expect one string
      result = parser->Parse(tcoordsName);
      if (result != vtkParseResult::Ok)
      {
        vtkErrorMacro(<< "Failed to parse material name at L." << lineNumber);
        return 0;
      }

      if (materialNameToId.find(tcoordsName) == materialNameToId.end())
      {
        // haven't seen this material yet, keep a record of it
        materialNameToId.emplace(tcoordsName, materialCount);
        materialNames->InsertNextValue(tcoordsName);
        materialCount++;
      }

      if (tcoordsMap.find(tcoordsName) == tcoordsMap.end())
      {
        tcoordsMap.emplace(tcoordsName, std::vector<bool>{});
      }

      // remember that starting with current cell, we should draw with it
      startCellToMaterialName[vertexPolys->GetNumberOfCells()] = tcoordsName;

      result = flushLine();
    }
    else if (command == "mtllib")
    {
      std::string name;
      result = parser->Parse(name);
      if (result != vtkParseResult::Ok)
      {
        vtkErrorMacro(<< "Failed to parse material lib name at L." << lineNumber);
        return 0;
      }

      libNames->InsertNextValue(name);

      result = flushLine();
    }
    else if (command == "v") // vertex/point
    {
      std::array<double, 3> point;

      for (std::size_t i = 0; i < 3; ++i)
      {
        result = parser->Parse(point[i]);
        if (result != vtkParseResult::Ok)
        {
          vtkErrorMacro(<< "Failed to parse " << i << "th vertex value at L." << lineNumber);
          return 0;
        }
      }

      // Check last value (which is optional)
      double w{};
      result = parser->Parse(w);
      if (result == vtkParseResult::Error)
      {
        vtkErrorMacro(<< "Unexpected token at L." << lineNumber);
        return 0;
      }

      points->InsertNextPoint(point.data());

      // skip flushLine if we consumed end of line or whole stream
      if (result == vtkParseResult::EndOfLine || result == vtkParseResult::EndOfStream)
      {
        continue;
      }

      result = flushLine();
    }
    else if (command == "vt") // tcoord
    {
      std::array<double, 2> tcoord;

      for (std::size_t i = 0; i < 2; ++i)
      {
        result = parser->Parse(tcoord[i]);
        if (result != vtkParseResult::Ok)
        {
          vtkErrorMacro(<< "Failed to parse " << i << "th tcoord value at L." << lineNumber);
          return 0;
        }
      }

      // Check last value (which is optional)
      double z{};
      result = parser->Parse(z);
      if (result == vtkParseResult::Error)
      {
        vtkErrorMacro(<< "Unexpected token at L." << lineNumber);
        return 0;
      }

      tcoords->InsertNextTuple(tcoord.data());

      // skip flushLine if we consumed end of line or whole stream
      if (result == vtkParseResult::EndOfLine || result == vtkParseResult::EndOfStream)
      {
        continue;
      }

      result = flushLine();
    }
    else if (command == "vn") // normals
    {
      std::array<double, 3> normal;

      for (std::size_t i = 0; i < 3; ++i)
      {
        result = parser->Parse(normal[i]);
        if (result != vtkParseResult::Ok)
        {
          vtkErrorMacro(<< "Failed to parse " << i << "th normal value at L." << lineNumber);
          return 0;
        }
      }

      normals->InsertNextTuple(normal.data());

      result = flushLine();
    }
    else if (command == "p")
    {
      const auto pointCount = points->GetNumberOfPoints();

      pointElems->InsertNextCell(0); // we don't yet know how many points are to come
      int vertCount = 0;             // keep a count of how many there are

      while (result == vtkParseResult::Ok)
      {
        int vert = 0;
        result = parser->Parse(vert);
        if (result == vtkParseResult::Ok)
        {
          if (vert < 0)
          {
            vert = pointCount + vert + 1;
          }

          if (vert <= 0)
          {
            vtkErrorMacro(<< "Unexpected point index value: " << vert);
            return 0;
          }
          pointElems->InsertCellPoint(vert - 1);
          ++vertCount;
        }
        else if (result == vtkParseResult::Error)
        {
          char c = 0;
          result = parser->Parse(c);
          // checking result here is unnecessary

          if (c == '\\')
          {
            result = flushLine();
            // transform end of line in OK here to discriminate the real end of the command
            if (result == vtkParseResult::EndOfLine)
            {
              result = vtkParseResult::Ok;
            }
          }
          else
          {
            vtkErrorMacro(<< "Unexpected token in OBJ file at L." << lineNumber);
            return 0;
          }
        }
      }

      if (vertCount < 1)
      {
        vtkErrorMacro(<< "Error: empty `p` command in OBJ file at L." << lineNumber);
        return 0;
      }

      // now we know how many points there were in this cell
      pointElems->UpdateCellCount(vertCount);
    }
    else if (command == "l")
    {
      const auto pointCount = points->GetNumberOfPoints();

      lineElems->InsertNextCell(0); // we don't yet know how many points are to come
      int vertCount = 0;            // keep a count of how many there are

      while (result == vtkParseResult::Ok)
      {
        int vert = 0;
        result = parser->Parse(vert);
        if (result == vtkParseResult::Ok)
        {
          if (vert < 0)
          {
            vert = pointCount + vert + 1;
          }

          if (vert <= 0)
          {
            vtkErrorMacro(<< "Unexpected point index value: " << vert);
            return 0;
          }
          lineElems->InsertCellPoint(vert - 1);
          ++vertCount;

          char c = 0;
          result = parser->Parse(c, vtkResourceParser::DiscardNone);
          // checking result here is unnecessary

          if (c == '/')
          {
            result = parser->Parse(vert, vtkResourceParser::DiscardNone);
            if (result != vtkParseResult::Ok)
            {
              vtkErrorMacro(<< "Unexpected token in OBJ file at L." << lineNumber);
              return 0;
            }

            // this value is parsed but unused
          }
        }
        else if (result == vtkParseResult::Error)
        {
          char c = 0;
          result = parser->Parse(c);
          // checking result here is unnecessary

          if (c == '\\')
          {
            result = flushLine();
            // transform end of line in OK here to discriminate the real end of the command
            if (result == vtkParseResult::EndOfLine)
            {
              result = vtkParseResult::Ok;
            }
          }
          else
          {
            vtkErrorMacro(<< "Unexpected token in OBJ file at L." << lineNumber);
            return 0;
          }
        }
      }

      if (vertCount < 2)
      {
        vtkErrorMacro(<< "Empty `l` command in OBJ file at L." << lineNumber);
        return 0;
      }

      // now we know how many points there were in this cell
      lineElems->UpdateCellCount(vertCount);
    }
    else if (command == "f") // face
    {
      const auto globalVertexCount = points->GetNumberOfPoints();
      const auto globalTcoordCount = tcoords->GetNumberOfTuples();
      const auto globalNormalCount = normals->GetNumberOfTuples();

      // We don't yet know how many points are to come
      vertexPolys->InsertNextCell(0);
      tcoordPolys->InsertNextCell(0);
      normalPolys->InsertNextCell(0);

      // Keep a count of how many of each there are, they must match in a single "f" command
      int vertexCount = 0;
      int tcoordCount = 0;
      int normalCount = 0;

      // parse `v` or `v/vt` or `v//vn` or `v/vt/vn`
      while (result == vtkParseResult::Ok)
      {
        int vertex = 0;
        result = parser->Parse(vertex);
        if (result == vtkParseResult::Ok)
        {
          ++vertexCount;

          int vertexAbs = 0;
          if (vertex < 0)
          {
            vertexAbs = globalVertexCount + vertex;
          }
          else
          {
            vertexAbs = vertex - 1;
          }

          if (vertexAbs < 0)
          {
            vtkErrorMacro(<< "Unexpected point index value: " << vertexAbs);
            return 0;
          }
          vertexPolys->InsertCellPoint(vertexAbs);

          if (!cellWithNotTextureFound)
          {
            cellWithNotTextureFound = true;

            if (materialNameToId.find(noMaterialName) == materialNameToId.end())
            {
              // haven't seen this material yet, keep a record of it
              materialNameToId.emplace(noMaterialName, materialCount);
              materialNames->InsertNextValue(noMaterialName);
              materialCount++;
            }

            // remember that starting with current cell, we should draw with it
            startCellToMaterialName[vertexPolys->GetNumberOfCells() - 1] = noMaterialName;
          }

          // determine if we have tcoord or normal
          char c = 0;
          result = parser->Parse(c, vtkResourceParser::DiscardNone);
          // checking result here is unnecessary

          if (c == '/') // check tcoords
          {
            int tcoord = 0;
            result = parser->Parse(tcoord, vtkResourceParser::DiscardNone);
            if (result == vtkParseResult::Ok)
            {
              int tcoordAbs = 0;
              if (tcoord < 0)
              {
                tcoordAbs = globalTcoordCount + tcoord;
              }
              else
              {
                tcoordAbs = tcoord - 1;
              }

              tcoordCount++;

              if (tcoordAbs < 0)
              {
                vtkErrorMacro(<< "Unexpected point index value: " << tcoordAbs);
                return 0;
              }
              tcoordPolys->InsertCellPoint(tcoordAbs);

              if (tcoordsMap.empty()) // no active tcoords, create the default one
              {
                tcoordsName = "TCoords";
                tcoordsMap.emplace(tcoordsName, std::vector<bool>{});
              }

              // Set the current texture array with the value corresponding to the read tcoords
              auto iter = tcoordsMap.find(tcoordsName);
              assert(iter != tcoordsMap.end() && "Corrupted tcoordsName name");
              auto& tcoordArray = iter->second;
              if (static_cast<std::size_t>(tcoordAbs) >= tcoordArray.size())
              {
                tcoordArray.resize(tcoordAbs + 1);
              }

              tcoordArray[tcoordAbs] = true;

              if (tcoordAbs != vertexAbs)
              {
                tcoordsMatchVertices = false;
              }
            }
            else if (result != vtkParseResult::Error) // error may indicate a double slash
            {
              vtkErrorMacro(<< "Invalid token after / in OBJ file at L." << lineNumber);
              return 0;
            }

            c = 0;
            result = parser->Parse(c, vtkResourceParser::DiscardNone);
            if (c == '/')
            {
              int normal = 0;
              result = parser->Parse(normal, vtkResourceParser::DiscardNone);
              if (result != vtkParseResult::Ok)
              {
                vtkErrorMacro(<< "Invalid token after // in OBJ file at L." << lineNumber);
                return 0;
              }

              normalCount++;

              int normalAbs = 0;
              if (normal < 0)
              {
                normalAbs = globalNormalCount + normal;
              }
              else
              {
                normalAbs = normal - 1;
              }

              if (normalAbs < 0)
              {
                vtkErrorMacro(<< "Unexpected point index value: " << normalAbs);
                return 0;
              }
              normalPolys->InsertCellPoint(normalAbs);

              if (normalAbs != vertexAbs)
              {
                normalsMatchVertices = false;
              }
            }
          }
        }
        else if (result == vtkParseResult::Error)
        {
          char c = 0;
          result = parser->Parse(c);
          // checking result here is unnecessary

          if (c == '\\')
          {
            result = flushLine();
            // transform end of line in OK here to discriminate the real end of the command
            if (result == vtkParseResult::EndOfLine)
            {
              result = vtkParseResult::Ok;
            }
          }
          else
          {
            vtkErrorMacro(<< "Unexpected token in OBJ file at L." << lineNumber);
            return 0;
          }
        }
      }

      if (vertexCount < 3)
      {
        vtkErrorMacro(<< "Definition of a face needs at least 3 vertices." << lineNumber);
        return 0;
      }

      // count of tcoords and normals must be equal to number of vertices or zero
      if ((tcoordCount > 0 && tcoordCount != vertexCount) ||
        (normalCount > 0 && normalCount != vertexCount))
      {
        vtkErrorMacro(<< "Definition of a face must match for all points L." << lineNumber);
        return 0;
      }

      // now we know how many points there were in this cell
      vertexPolys->UpdateCellCount(vertexCount);
      tcoordPolys->UpdateCellCount(tcoordCount);
      normalPolys->UpdateCellCount(normalCount);

      if (faceScalars && vertexCount != 0)
      {
        if (groupId < 0)
        {
          groupId = 0;
        }

        faceScalars->InsertNextValue(groupId);
      }
    }
    else // ignore unknown commands
    {
      result = parser->DiscardLine();
    }
  }

  // the last result that ended the loop
  if (result != vtkParseResult::EndOfStream)
  {
    vtkErrorMacro(<< "Error during parsing of OBJ file L." << lineNumber);
    return 0;
  }

  if (!firstComment.empty())
  {
    this->SetComment(firstComment.c_str());
  }

  std::vector<vtkSmartPointer<vtkFloatArray>> newTcoordsVec;

  const bool hasMaterial =
    materialCount > 1 || (materialCount == 1 && materialNames->GetValue(0) != noMaterialName);

  // Fixing the OBJ is done because OBJ files can index normals, vertices and tcoords independently
  // but VTK cannot.
  const bool needFix = !normalsMatchVertices || !tcoordsMatchVertices;

  if (needFix)
  {
    vtkDebugMacro(<< "Duplicating vertices so that tcoords and normals are correct");

    const bool hasNormals = normals->GetNumberOfTuples() > 0;
    const bool hasTcoords = !tcoordsMap.empty();

    auto newPoints = vtkSmartPointer<vtkPoints>::New();
    newPoints->SetDataTypeToDouble();
    newPoints->SetNumberOfPoints(vertexPolys->GetNumberOfConnectivityIds());

    auto newNormals = vtkSmartPointer<vtkFloatArray>::New();

    if (hasNormals)
    {
      newNormals->SetName("Normals");
      newNormals->SetNumberOfComponents(3);
      newNormals->SetNumberOfTuples(vertexPolys->GetNumberOfConnectivityIds());
    }

    if (hasTcoords)
    {
      for (const auto& iter : tcoordsMap)
      {
        auto newTcoords = vtkSmartPointer<vtkFloatArray>::New();
        newTcoords->SetName(iter.first.c_str());
        newTcoords->SetNumberOfComponents(2);
        newTcoords->SetNumberOfTuples(vertexPolys->GetNumberOfConnectivityIds());
        newTcoords->FillValue(-1.0f);

        newTcoordsVec.emplace_back(newTcoords);
      }
    }

    // for each poly, copy its vertices into new_points (and point at them)
    // also copy its tcoords into new_tcoords
    // also copy its normals into new_normals
    auto newPolys = vtkSmartPointer<vtkCellArray>::New();

    vtkIdType nextVertex = 0;
    vtkNew<vtkIdList> vertexIds;
    vtkNew<vtkIdList> tcoordIds;
    vtkNew<vtkIdList> normalIds;
    vtkNew<vtkIdList> tmpCell;

    for (vtkIdType celli = 0; celli < vertexPolys->GetNumberOfCells(); ++celli)
    {
      vertexPolys->GetCellAtId(celli, vertexIds);

      if (hasNormals)
      {
        normalPolys->GetCellAtId(celli, normalIds);
      }

      if (hasTcoords)
      {
        tcoordPolys->GetCellAtId(celli, tcoordIds);
      }

      const auto vertexCount = vertexIds->GetNumberOfIds();
      const auto normalCount = normalIds->GetNumberOfIds();
      const auto tcoordCount = tcoordIds->GetNumberOfIds();

      int matId = 0;
      if (hasTcoords)
      {
        // keep a record of the material for each cell
        const auto citer = startCellToMaterialName.find(celli);
        if (citer != startCellToMaterialName.end())
        {
          const std::string& matname = citer->second;
          matId = materialNameToId.find(matname)->second;
        }
      }

      // If some vertices have tcoords and not others (likewise normals)
      // then we must do something else VTK will complain. (crash on render attempt)
      // Easiest solution is to delete polys that don't have complete tcoords (if there
      // are any tcoords in the dataset) or normals (if there are any normals in the dataset).
      // We allow cells with tcoords to mix with cells without tcoords
      if ((vertexCount != tcoordCount && tcoordCount > 0) ||
        (vertexCount != normalCount && normalCount > 0))
      {
        vtkWarningMacro(<< "Skipping poly " << celli + 1 << " (1-based index)");
      }
      else
      {
        tmpCell->SetNumberOfIds(vertexCount);

        // copy the corresponding points, tcoords and normals across
        for (vtkIdType vertexi = 0; vertexi < vertexCount; ++vertexi)
        {
          // copy the tcoord for this point across (if there is one)
          if (tcoordCount > 0)
          {
            std::size_t k = 0;
            for (const auto& iter : tcoordsMap)
            {
              auto& newTcoords = newTcoordsVec[k];

              std::array<float, 2> tcoordBuffer;
              const auto tcoordId = tcoordIds->GetId(vertexi);
              if (tcoordId < static_cast<vtkIdType>(iter.second.size()) && iter.second[tcoordId])
              {
                tcoords->GetTypedTuple(tcoordId, tcoordBuffer.data());
                newTcoords->SetTuple(nextVertex, tcoordBuffer.data());
              }

              ++k;
            }
          }

          // copy the normal for this point across (if there is one)
          if (normalCount > 0)
          {
            std::array<float, 3> normalBuffer;
            normals->GetTypedTuple(normalIds->GetId(vertexi), normalBuffer.data());
            newNormals->SetTuple(nextVertex, normalBuffer.data());
          }

          // copy the vertex into the new structure and update
          // the vertex index in the polys structure (pts is a pointer into it)
          newPoints->SetPoint(nextVertex, points->GetPoint(vertexIds->GetId(vertexi)));
          tmpCell->SetId(vertexi, nextVertex);
          nextVertex += 1;
        }

        newPolys->InsertNextCell(tmpCell);
        if (hasMaterial)
        {
          materialIds->InsertNextValue(matId);
        }
      }
    }

    points = newPoints;
    normals = newNormals;
    vertexPolys = newPolys;
  }
  else if (!tcoordsMap.empty())
  {
    // Generate tcoords arrays
    vtkNew<vtkIdList> pointIds;
    vtkNew<vtkIdList> tcoordIds;

    for (const auto& iter : tcoordsMap)
    {
      auto newTcoords = vtkSmartPointer<vtkFloatArray>::New();
      newTcoords->SetNumberOfComponents(2);
      newTcoords->SetName(iter.first.c_str());
      newTcoords->SetNumberOfTuples(points->GetNumberOfPoints());
      newTcoords->FillValue(-1.0f);

      const auto polyCount = vertexPolys->GetNumberOfCells();
      for (vtkIdType poly = 0; poly < polyCount; ++poly)
      {
        vertexPolys->GetCellAtId(poly, pointIds);
        tcoordPolys->GetCellAtId(poly, tcoordIds);

        if (tcoordIds->GetNumberOfIds() != 0)
        {
          for (vtkIdType point = 0; point < pointIds->GetNumberOfIds(); ++point)
          {
            std::array<float, 2> newTcoord;
            const auto tcoordId = tcoordIds->GetId(point);
            if (tcoordId < static_cast<vtkIdType>(iter.second.size()) && iter.second.at(tcoordId))
            {
              tcoords->GetTypedTuple(tcoordId, newTcoord.data());
              newTcoords->SetTuple(pointIds->GetId(point), newTcoord.data());
            }
          }
        }
      }

      newTcoordsVec.emplace_back(newTcoords);
    }

    if (hasMaterial)
    {
      // keep a record of the material for each cell
      for (vtkIdType celli = 0; celli < vertexPolys->GetNumberOfCells(); ++celli)
      {
        int matId = 0;
        const auto citer = startCellToMaterialName.find(celli);
        if (citer != startCellToMaterialName.end())
        {
          const auto& name = citer->second;
          matId = materialNameToId.find(name)->second;
        }

        materialIds->InsertNextValue(matId);
      }
    }
  }

  // Fill output
  output->SetPoints(points);

  // TODO: Support fixing for points
  if (pointElems->GetNumberOfCells() > 0 && !needFix)
  {
    output->SetVerts(pointElems);
  }

  // TODO: Support fixing for lines
  if (lineElems->GetNumberOfCells() > 0 && !needFix)
  {
    output->SetLines(lineElems);
  }

  if (vertexPolys->GetNumberOfCells() > 0)
  {
    output->SetPolys(vertexPolys);
  }

  if (normals->GetNumberOfTuples() > 0)
  {
    output->GetPointData()->SetNormals(normals);
  }

  if (groupId != -1 && faceScalars)
  {
    output->GetCellData()->AddArray(faceScalars);
  }

  for (const auto& newTcoords : newTcoordsVec)
  {
    output->GetPointData()->AddArray(newTcoords);
  }

  if (!newTcoordsVec.empty())
  {
    output->GetPointData()->SetActiveTCoords(newTcoordsVec[0]->GetName());
  }

  if (hasMaterial)
  {
    output->GetCellData()->AddArray(materialIds);
    output->GetFieldData()->AddArray(materialNames);

    if (libNames->GetNumberOfTuples() > 0)
    {
      output->GetFieldData()->AddArray(libNames);
    }
  }

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
void vtkOBJReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Comment: " << (this->Comment ? this->Comment : "(none)") << "\n";
}
VTK_ABI_NAMESPACE_END
