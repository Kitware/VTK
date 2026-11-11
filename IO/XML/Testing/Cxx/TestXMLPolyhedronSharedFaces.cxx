// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Polyhedra whose shared faces are stored once, a cell listing face n as -(n+1) to traverse it in
// reverse, round trip through the XML format with their faces and signed ids, in every data mode
// and in a file of two pieces.

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using Lists = std::vector<std::vector<vtkIdType>>;

int Failures = 0;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    std::cerr << message << "\n";
    ++Failures;
  }
}

vtkSmartPointer<vtkCellArray> NewCellArray(const Lists& lists)
{
  auto array = vtkSmartPointer<vtkCellArray>::New();
  for (const auto& list : lists)
  {
    array->InsertNextCell(static_cast<vtkIdType>(list.size()), list.data());
  }
  return array;
}

vtkSmartPointer<vtkUnstructuredGrid> MakeGrid(
  const Lists& faces, const Lists& locations, const Lists& cellPoints)
{
  vtkNew<vtkPoints> points;
  for (const auto& p :
    { std::vector<double>{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 1, 1 } })
  {
    points->InsertNextPoint(p.data());
  }
  vtkNew<vtkPolyhedronCellArray> polyhedra;
  polyhedra->SetData(NewCellArray(faces), NewCellArray(locations));
  vtkNew<vtkUnsignedCharArray> types;
  for (std::size_t i = 0; i < cellPoints.size(); ++i)
  {
    types->InsertNextValue(VTK_POLYHEDRON);
  }
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  grid->SetPolyhedralCells(types, NewCellArray(cellPoints), polyhedra);
  return grid;
}

Lists FaceStreams(vtkUnstructuredGrid* grid)
{
  vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray();
  Lists streams;
  vtkNew<vtkIdList> stream;
  for (vtkIdType cellId = 0; cellId < polyhedra->GetNumberOfCells(); ++cellId)
  {
    polyhedra->GetFaceStream(cellId, stream);
    streams.emplace_back(stream->begin(), stream->end());
  }
  return streams;
}

std::vector<vtkIdType> SignedFaceIds(vtkUnstructuredGrid* grid)
{
  vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray();
  std::vector<vtkIdType> ids;
  for (vtkIdType cellId = 0; cellId < polyhedra->GetNumberOfCells(); ++cellId)
  {
    for (int face = 0; face < polyhedra->GetCellNumberOfFaces(cellId); ++face)
    {
      ids.push_back(polyhedra->GetSignedFaceId(cellId, face));
    }
  }
  return ids;
}

std::string Write(vtkUnstructuredGrid* grid, int dataMode)
{
  vtkNew<vtkXMLUnstructuredGridWriter> writer;
  writer->SetInputData(grid);
  writer->SetDataMode(dataMode);
  writer->WriteToOutputStringOn();
  writer->Write();
  return writer->GetOutputString();
}

vtkSmartPointer<vtkUnstructuredGrid> Read(const std::string& text)
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(text);
  reader->Update();
  return reader->GetOutput();
}

void CheckRoundTrips(const std::string& name, vtkUnstructuredGrid* grid)
{
  Check(grid->GetPolyhedronCellArray()->Validate(), name + ": invalid test grid");
  for (const int dataMode : { vtkXMLWriter::Ascii, vtkXMLWriter::Binary, vtkXMLWriter::Appended })
  {
    const std::string where = name + ", data mode " + vtk::to_string(dataMode);
    const std::string text = Write(grid, dataMode);
    Check(text.find("version=\"2.5\"") != std::string::npos, where + ": the version should be 2.5");
    auto read = Read(text);
    Check(read->GetNumberOfCells() == grid->GetNumberOfCells(), where + ": wrong number of cells");
    Check(read->GetPolyhedronCellArray() &&
        read->GetPolyhedronCellArray()->GetNumberOfFaces() ==
          grid->GetPolyhedronCellArray()->GetNumberOfFaces(),
      where + ": the shared faces should be read once");
    if (Failures == 0)
    {
      Check(SignedFaceIds(read) == SignedFaceIds(grid), where + ": wrong signed face ids");
      Check(FaceStreams(read) == FaceStreams(grid), where + ": wrong faces");
    }
  }

  // Two pieces in one file: the faces of the second are appended after those of the first, the
  // reversed ones moving away from zero.
  std::string text = Write(grid, vtkXMLWriter::Ascii);
  const std::size_t begin = text.find("<Piece");
  const std::size_t end = text.find("</Piece>") + std::string("</Piece>").size();
  text.insert(end, "\n" + text.substr(begin, end - begin));
  auto read = Read(text);
  const vtkIdType numFaces = grid->GetPolyhedronCellArray()->GetNumberOfFaces();
  Check(read->GetNumberOfCells() == 2 * grid->GetNumberOfCells() &&
      read->GetPolyhedronCellArray() &&
      read->GetPolyhedronCellArray()->GetNumberOfFaces() == 2 * numFaces,
    name + ", two pieces: wrong number of cells or faces");
  if (Failures == 0)
  {
    std::vector<vtkIdType> expectedIds = SignedFaceIds(grid);
    for (const vtkIdType id : SignedFaceIds(grid))
    {
      expectedIds.push_back(id < 0 ? id - numFaces : id + numFaces);
    }
    Check(SignedFaceIds(read) == expectedIds, name + ", two pieces: wrong signed face ids");
    Lists expectedStreams = FaceStreams(grid);
    for (const auto& stream : FaceStreams(grid))
    {
      expectedStreams.push_back(stream);
    }
    Check(FaceStreams(read) == expectedStreams, name + ", two pieces: wrong faces");
  }
}
} // anonymous namespace

int TestXMLPolyhedronSharedFaces(int, char*[])
{
  // Two tetrahedra sharing the face {1, 2, 3}, stored once for the first; the second lists it as
  // -(3+1).
  CheckRoundTrips("shared face",
    MakeGrid(
      { { 0, 2, 1 }, { 0, 1, 3 }, { 0, 3, 2 }, { 1, 2, 3 }, { 1, 2, 4 }, { 2, 3, 4 }, { 3, 1, 4 } },
      { { 0, 1, 2, 3 }, { -4, 4, 5, 6 } }, { { 0, 1, 2, 3 }, { 1, 2, 3, 4 } }));

  // A tetrahedron whose faces are all stored the other way around, so that every id is negative.
  CheckRoundTrips("reversed faces",
    MakeGrid({ { 0, 1, 2 }, { 0, 3, 1 }, { 0, 2, 3 }, { 1, 3, 2 } }, { { -1, -2, -3, -4 } },
      { { 0, 1, 2, 3 } }));

  return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
