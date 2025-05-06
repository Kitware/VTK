// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "AvmeshInternals.h"
#include "AvmeshMetadata.h"
#include "BinaryFile.h"

#include <vtkCellType.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkUnstructuredGrid.h>

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

AvmeshError::AvmeshError(std::string msg)
  : std::runtime_error(msg)
{
}

namespace
{
typedef std::array<int, 5> Bface; // enough to hold a quad plus a patch ID
typedef std::vector<Bface> BfaceList;

AvmeshMetadata ReadMetadata(BinaryFile& fin)
{
  AvmeshMetadata meta;

  // File header
  // Make sure the magic string is in place
  fin.ReadCString(meta.MagicString, 6);
  if (strncmp(meta.MagicString, "AVMESH", 6) != 0)
  {
    throw AvmeshError("Not a AVMESH file");
  }

  // Use the magic number to determine if byte-swapping is needed.  NOTE: while
  // the AVMESH standard theoretically allows for big-endian files, practically
  // speaking, they're always little-endian.
  meta.MagicNumber = fin.ReadInt();
  if (meta.MagicNumber != 1)
  {
    if (BinaryFile::SwapInt(meta.MagicNumber) == 1)
    {
      fin.SetSwap(true);
    }
    else
    {
      throw AvmeshError("Could not establish endianness");
    }
  }

  meta.Version = fin.ReadInt(); // must be 1 or 2, but we'll verify this later
  meta.MeshCount = fin.ReadInt();
  fin.ReadCString(meta.ContactInfo);
  meta.Precision = fin.ReadInt();
  meta.Dimensions = fin.ReadInt();
  meta.Description = fin.ReadStdString();

  // Mesh header
  fin.ReadCString(meta.MeshName);
  fin.ReadCString(meta.MeshType); // must be "unstruc", but we'll check later
  fin.ReadCString(meta.MeshGenerator);
  fin.ReadCString(meta.CoordinateSystem);
  meta.Scale = fin.ReadDouble();
  fin.ReadCString(meta.Units);

  if (meta.Version == 1)
  {
    meta.RefLen[0] = fin.ReadDouble();
    meta.RefLen[1] = meta.RefLen[0];
    meta.RefLen[2] = meta.RefLen[0];
  }
  else
  {
    fin.ReadArray(meta.RefLen, 3);
  }

  meta.RefArea = fin.ReadDouble();
  fin.ReadArray(meta.RefPoint, 3);
  fin.ReadCString(meta.RefDescription);

  meta.Refined = (meta.Version == 2) ? fin.ReadInt() : 0;

  fin.ReadCString(meta.MeshDescription);

  // Unstruc header
  meta.NumNodes = fin.ReadInt();
  meta.NumFaces = fin.ReadInt();
  meta.NumCells = fin.ReadInt();
  meta.MaxNodesPerFace = fin.ReadInt();
  meta.MaxNodesPerCell = fin.ReadInt();
  meta.MaxFacesPerCell = fin.ReadInt();

  if (meta.Version == 2)
  {
    fin.ReadCString(meta.ElementScheme, sizeof(meta.ElementScheme));
    meta.FacePolyOrder = fin.ReadInt();
    meta.CellPolyOrder = fin.ReadInt();
  }
  else
  {
    strncpy(meta.ElementScheme, "uniform", sizeof(meta.ElementScheme));
    meta.FacePolyOrder = 1;
    meta.CellPolyOrder = 1;
  }

  meta.NumPatches = fin.ReadInt();
  meta.NumHexCells = fin.ReadInt();
  meta.NumTetCells = fin.ReadInt();
  meta.NumPriCells = fin.ReadInt();
  meta.NumPyrCells = fin.ReadInt();

  meta.NumPolyCells = (meta.Version == 1) ? fin.ReadInt() : 0;

  meta.NumBndTriFaces = fin.ReadInt();
  meta.NumTriFaces = fin.ReadInt();
  meta.NumBndQuadFaces = fin.ReadInt();
  meta.NumQuadFaces = fin.ReadInt();

  if (meta.Version == 1)
  {
    meta.NumBndPolyCells = fin.ReadInt();
    meta.NumPolyFaces = fin.ReadInt();
    meta.BndPolyFacesSize = fin.ReadInt();
    meta.PolyFacesSize = fin.ReadInt();
  }
  else
  {
    meta.NumBndPolyCells = 0;
    meta.NumPolyFaces = 0;
    meta.BndPolyFacesSize = 0;
    meta.PolyFacesSize = 0;
  }

  meta.NumEdges = fin.ReadInt();
  meta.NumNodesOnGeometry = fin.ReadInt();
  meta.NumEdgesOnGeometry = fin.ReadInt();
  meta.NumFacesOnGeometry = fin.ReadInt();
  meta.GeomRegionId = fin.ReadInt();

  // Patch info
  meta.Patches.resize(meta.NumPatches);
  for (auto& patch : meta.Patches)
  {
    fin.ReadCString(patch.Label, sizeof(patch.Label));
    fin.ReadCString(patch.Type, sizeof(patch.Type));
    patch.Pid = fin.ReadInt();
  }

  return meta;
}

void CheckAssumptions(AvmeshMetadata const& meta)
{
  bool readable = true;
  std::string failMsg;

  // rev0 is a weird face-based format that nobody uses anymore,
  // and rev3 doesn't exist yet
  if (meta.Version < 1 || meta.Version > 2)
  {
    failMsg += "Only AVMESH rev1 and rev2 allowed\n";
    readable = false;
  }

  if (meta.Dimensions < 2 || meta.Dimensions > 3)
  {
    failMsg += "Dimensions must be 2 or 3\n";
    readable = false;
  }

  // Never seen a single precision one in the wild
  if (meta.Precision != 2)
  {
    failMsg += "Only double precision supported\n";
    readable = false;
  }

  if (meta.MeshCount < 1)
  {
    failMsg += "No meshes in file\n";
    readable = false;
  }

  // Never seen a multi-mesh AVMESH file in the wild
  if (meta.MeshCount > 1)
  {
    failMsg += "Multi-mesh AVMESH file detected.  Only the first mesh will be read.\n";
  }

  if (strncmp(meta.MeshType, "unstruc", sizeof(meta.MeshType)) != 0)
  {
    failMsg += "Only unstruc files allowed\n";
    readable = false;
  }

  // Higher order AVMESH grids do exist in practice for use with COFFE,
  // but we're not going to support that here.
  if (meta.FacePolyOrder != 1 || meta.CellPolyOrder != 1)
  {
    failMsg += "Only linear (P1) meshes allowed\n";
    readable = false;
  }

  // Arbitrary poly AVMESH files don't exist in practice since neither Kestrel
  // nor Helios support it.
  if (meta.NumPolyCells != 0 || meta.NumBndPolyCells != 0 || meta.NumPolyFaces != 0 ||
    meta.BndPolyFacesSize != 0 || meta.PolyFacesSize != 0)
  {
    failMsg += "Arbitrary polyhedral grids not allowed\n";
    readable = false;
  }

  if (!readable)
  {
    throw AvmeshError(failMsg);
  }
}

vtkSmartPointer<vtkPoints> ReadVolumeVerts(BinaryFile& fin, int nNodes)
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nNodes);
  double* buff = (double*)(points->GetVoidPointer(0));
  size_t nitems = (size_t)nNodes * 3;
  fin.ReadArray(buff, nitems);
  return points;
}

vtkSmartPointer<vtkUnstructuredGrid> AddPartitionedDataSet(
  vtkPartitionedDataSetCollection* output, const char* name)
{
  auto ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  auto partitionedDataSet = vtkSmartPointer<vtkPartitionedDataSet>::New();
  partitionedDataSet->SetNumberOfPartitions(1);
  partitionedDataSet->SetPartition(0, ugrid);
  unsigned int num = output->GetNumberOfPartitionedDataSets();
  output->SetPartitionedDataSet(num, partitionedDataSet);
  output->GetMetaData(num)->Set(vtkCompositeDataSet::NAME(), name);
  return ugrid;
}

void Read2DSurfaceConn(BinaryFile& fin, int nbnd, BfaceList& bfaces, bool fileHasNeighbors)
{
  // For 2D grids, we read the boundary edges as if they are triangles,
  // but only the first two nodes are significant.
  for (int i = 0; i < nbnd; ++i)
  {
    fin.ReadArray(bfaces[i].data(), 3);
    bfaces[i][3] = bfaces[i][2] = bfaces[i][1]; // duplicate nodes to mark it as a line
    if (fileHasNeighbors)
    {
      (void)fin.ReadInt();
    }
    bfaces[i][4] = fin.ReadInt();
  }

  // Convert from 1-based to 0-based connectivity
  for (auto& face : bfaces)
  {
    for (int i = 0; i < 4; ++i)
    {
      face[i]--;
    }
  }
}

void Read3DSurfaceConn(
  BinaryFile& fin, int ntri, int nquad, BfaceList& bfaces, bool fileHasNeighbors)
{
  // tris
  for (int i = 0; i < ntri; ++i)
  {
    fin.ReadArray(bfaces[i].data(), 3);
    bfaces[i][3] = bfaces[i][2]; // duplicate nodes to mark it as a triangle
    if (fileHasNeighbors)
    {
      (void)fin.ReadInt();
    }
    bfaces[i][4] = fin.ReadInt();
  }

  // quads
  for (int i = ntri; i < ntri + nquad; ++i)
  {
    fin.ReadArray(bfaces[i].data(), 4);
    if (fileHasNeighbors)
    {
      (void)fin.ReadInt();
    }
    bfaces[i][4] = fin.ReadInt();
  }

  // Convert from 1-based to 0-based connectivity
  for (auto& face : bfaces)
  {
    for (int i = 0; i < 4; ++i)
    {
      face[i]--;
    }
  }
}

void Read2DVolumeConn(BinaryFile& fin, int nquad, int ntri, vtkUnstructuredGrid* ugrid)
{
  size_t ncell = (size_t)nquad + (size_t)ntri;
  size_t connSize = 4 * (size_t)nquad + 3 * (size_t)ntri;

  ugrid->GetCells()->Use32BitStorage();
  ugrid->AllocateExact(ncell, connSize);

  // Connectivity in the file is 32-bit ints, but InsertNextCell requires vtkIdType.
  int cell[8];
  vtkIdType nodeids[8];

  // Read quads first.  Note that quads are stored as hexs, but only the first
  // 4 nodes are significant.
  for (int i = 0; i < nquad; ++i)
  {
    fin.ReadArray(cell, 8);
    for (int j = 0; j < 4; ++j)
    {
      nodeids[j] = cell[j] - 1; // convert to 0-based connectivity
    }
    ugrid->InsertNextCell(VTK_QUAD, 4, nodeids);
  }

  // Then read tris.  Note that tris are stored as tets, but only the first
  // 3 nodes are significant.
  for (int i = 0; i < ntri; ++i)
  {
    fin.ReadArray(cell, 4);
    for (int j = 0; j < 3; ++j)
    {
      nodeids[j] = cell[j] - 1; // convert to 0-based connectivity
    }
    ugrid->InsertNextCell(VTK_TRIANGLE, 3, nodeids);
  }
}

int NodesPerCell(int etype)
{
  switch (etype)
  {
    case VTK_LINE:
      return 2;
    case VTK_TRIANGLE:
      return 3;
    case VTK_QUAD:
    case VTK_TETRA:
      return 4;
    case VTK_PYRAMID:
      return 5;
    case VTK_WEDGE:
      return 6;
    case VTK_HEXAHEDRON:
      return 8;
    default:
      return 0;
  }
}

// Populate the connectivity of a 3D volume mesh by directly accessing the raw
// offset and connectivity arrays.  For large meshes (say, 100 million cells),
// this can be 4 to 5 times faster than iteratively calling InsertNextCell.
void Read3DVolumeConnFast(
  BinaryFile& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid)
{
  size_t ncell = (size_t)nhex + (size_t)ntet + (size_t)npri + (size_t)npyr;
  size_t connSize = 8 * (size_t)nhex + 4 * (size_t)ntet + 6 * (size_t)npri + 5 * (size_t)npyr;

  vtkNew<vtkCellArray> cells;
  cells->Use32BitStorage(); // AVMESH files always use 32-bit signed ints
  cells->AllocateExact(ncell, connSize);

  // Get pointer to the cell offsets
  auto offsetsArr = cells->GetOffsetsArray32();
  offsetsArr->SetNumberOfTuples(ncell + 1);
  int* offsets = offsetsArr->GetPointer(0);

  // Get pointer to the cell types
  vtkNew<vtkUnsignedCharArray> cellTypesArr;
  cellTypesArr->SetNumberOfTuples(ncell);
  uint8_t* cellTypes = cellTypesArr->GetPointer(0);

  // Loop over cells to set types and offsets
  int n = 0;
  offsets[0] = 0;
  for (int i = 0; i < nhex; ++i, ++n)
  {
    cellTypes[n] = VTK_HEXAHEDRON;
    offsets[n + 1] = offsets[n] + 8;
  }
  for (int i = 0; i < ntet; ++i, ++n)
  {
    cellTypes[n] = VTK_TETRA;
    offsets[n + 1] = offsets[n] + 4;
  }
  for (int i = 0; i < npri; ++i, ++n)
  {
    cellTypes[n] = VTK_WEDGE;
    offsets[n + 1] = offsets[n] + 6;
  }
  for (int i = 0; i < npyr; ++i, ++n)
  {
    cellTypes[n] = VTK_PYRAMID;
    offsets[n + 1] = offsets[n] + 5;
  }

  // Now read the heavy connectivity data in one big chunk
  auto connArr = cells->GetConnectivityArray32();
  connArr->SetNumberOfTuples(connSize);
  int* conn = connArr->GetPointer(0);
  fin.ReadArray(conn, connSize);

  // Make connectivity 0-based (AVMESH is always 1-based)
  for (size_t i = 0; i < connSize; ++i)
  {
    conn[i]--;
  }

  // Fix the node order of prisms (wedges), which are the only cell type for
  // which AVMESH and VTK have different conventions.
  n = nhex + ntet;
  for (int i = 0; i < npri; ++i, ++n)
  {
    int* pri = conn + offsets[n];
    std::swap(pri[1], pri[2]);
    std::swap(pri[4], pri[5]);
  }

  ugrid->SetCells(cellTypesArr, cells);
}

void Read3DVolumeConnOfType(BinaryFile& fin, int etype, int ncell, vtkUnstructuredGrid* ugrid)
{
  // Connectivity in the file is 32-bit ints, but InsertNextCell requires vtkIdType.
  int cell[8];
  vtkIdType nodeids[8];
  int nNodesPerCell = NodesPerCell(etype);
  for (int i = 0; i < ncell; ++i)
  {
    fin.ReadArray(cell, nNodesPerCell);
    for (int j = 0; j < nNodesPerCell; ++j)
    {
      nodeids[j] = cell[j] - 1; // convert to 0-based connectivity
    }
    if (etype == VTK_WEDGE) // wedges are the only cell type with a winding
    {                       // order that doesn't match VTK's
      std::swap(nodeids[1], nodeids[2]);
      std::swap(nodeids[4], nodeids[5]);
    }
    ugrid->InsertNextCell(etype, nNodesPerCell, nodeids);
  }
}

// Construct connectivity of a 3D volume mesh by iteratively calling InsertNextCell.
void Read3DVolumeConnIterative(
  BinaryFile& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid)
{
  size_t ncell = (size_t)nhex + (size_t)ntet + (size_t)npri + (size_t)npyr;
  size_t connSize = 8 * (size_t)nhex + 4 * (size_t)ntet + 6 * (size_t)npri + 5 * (size_t)npyr;

  ugrid->GetCells()->Use32BitStorage();
  ugrid->AllocateExact(ncell, connSize);

  Read3DVolumeConnOfType(fin, VTK_HEXAHEDRON, nhex, ugrid);
  Read3DVolumeConnOfType(fin, VTK_TETRA, ntet, ugrid);
  Read3DVolumeConnOfType(fin, VTK_WEDGE, npri, ugrid);
  Read3DVolumeConnOfType(fin, VTK_PYRAMID, npyr, ugrid);
}

void Read3DVolumeConn(BinaryFile& fin, int nhex, int ntet, int npri, int npyr,
  bool BuildConnectivityIteratively, vtkUnstructuredGrid* ugrid)
{
  if (BuildConnectivityIteratively)
  {
    Read3DVolumeConnIterative(fin, nhex, ntet, npri, npyr, ugrid);
  }
  else
  {
    Read3DVolumeConnFast(fin, nhex, ntet, npri, npyr, ugrid);
  }
}

void BuildSurface(vtkUnstructuredGrid* surfGrid, vtkPoints* volPoints,
  BfaceList::const_iterator firstFace, BfaceList::const_iterator lastFace)
{
  // Start by finding the set of unique volume node IDs that belong to this patch
  int nface = lastFace - firstFace;
  std::unordered_set<int> s2v(nface / 2); // a surface mesh has about half as many nodes as faces
  for (auto face = firstFace; face != lastFace; ++face)
  {
    s2v.insert(face->begin(), face->begin() + 4); // insert nodes only, not patch ID at end
  }

  // Number of unique nodes on this patch
  size_t pnnode = s2v.size();

  // Now construct the volume-to-surface mapping, which maps a volume node ID
  // from the whole grid to a surface node ID on this patch.
  std::unordered_map<int, int> v2s(pnnode);
  int s = 0;
  for (auto v : s2v)
  {
    v2s[v] = s++;
  }

  // Now use the surface-to-volume mapping to extract the points needed for
  // this patch.
  vtkNew<vtkPoints> surfPoints;
  surfPoints->SetDataTypeToDouble();
  surfPoints->Allocate(pnnode);
  for (auto v : s2v)
  {
    auto pt = volPoints->GetPoint(v);
    surfPoints->InsertNextPoint(pt);
  }
  surfGrid->SetPoints(surfPoints);

  // Use the volume-to-surface mapping to construct this patch's connectivty
  // based on this patch's node IDs.
  surfGrid->Allocate(nface);
  for (BfaceList::const_iterator face = firstFace; face != lastFace; ++face)
  {
    int etype;
    if ((*face)[1] == (*face)[2])
    {
      etype = VTK_LINE;
    }
    else if ((*face)[2] == (*face)[3])
    {
      etype = VTK_TRIANGLE;
    }
    else
    {
      etype = VTK_QUAD;
    }

    int nodesPer = NodesPerCell(etype);
    vtkIdType nodeids[4];
    for (int i = 0; i < nodesPer; ++i)
    {
      nodeids[i] = v2s.at((*face)[i]);
    }
    surfGrid->InsertNextCell(etype, nodesPer, nodeids);
  }
}

void BuildBoundaryPartitionedDataSets(vtkPartitionedDataSetCollection* output, vtkPoints* volPoints,
  std::vector<AvmeshPatch> const& patches, BfaceList& bfaces)
{
  // There is no guarantee that the boundary connectivity and patch IDs will be
  // in any particular order.  So we need to group them together by patch ID.
  // Once we have all the faces that belong to a patch, we can construct a collection
  // for that patch.
  auto firstFace = bfaces.begin();
  for (auto const& patch : patches)
  {
    auto lastFace = std::stable_partition(
      firstFace, bfaces.end(), [&patch](Bface const& face) { return face[4] == patch.Pid; });
    auto surfGrid = AddPartitionedDataSet(output, patch.Label);
    BuildSurface(surfGrid, volPoints, firstFace, lastFace);
    patch.ToFieldData(surfGrid->GetFieldData());
    firstFace = lastFace;
  }
}
} // namespace

void ReadAvmesh(vtkPartitionedDataSetCollection* output, std::string fname, bool SurfaceOnly,
  bool BuildConnectivityIteratively)
{
  // Make surf the file is ready for reading
  BinaryFile fin(fname.c_str());
  if (!fin.good())
  {
    throw AvmeshError("Could not open AVMESH file");
  }

  // Read all the metadata in one big chunk (could throw exception)
  auto meta = ReadMetadata(fin);

  // Make sure the metadata conforms to our assumptions (could throw exception)
  CheckAssumptions(meta);

  // Add metadata to output as field data
  meta.ToFieldData(output->GetFieldData());

  // Read all the points.  Need to read them all even if we're in surface-only
  // mode because there is no guarantee that the surface points will come first.
  auto points = ReadVolumeVerts(fin, meta.NumNodes);

  // If we're reading the volume grid, construct the volume partitioned dataset and attach
  // the points to it.
  vtkSmartPointer<vtkUnstructuredGrid> volGrid = nullptr;
  if (!SurfaceOnly)
  {
    volGrid = AddPartitionedDataSet(output, "Flowfield");
    volGrid->SetPoints(points);
  }

  // Read the surface data as one big partitioned dataset.  We'll sort it into patches later.
  bool readNeighborData = (meta.Version == 1);
  BfaceList bfaces(meta.NumBndTriFaces + meta.NumBndQuadFaces);
  if (meta.Dimensions == 2)
  {
    Read2DSurfaceConn(fin, meta.NumBndTriFaces, bfaces, readNeighborData);
  }
  else
  {
    Read3DSurfaceConn(fin, meta.NumBndTriFaces, meta.NumBndQuadFaces, bfaces, readNeighborData);
  }

  // If we're reading the volume grid, read the connectivity
  if (!SurfaceOnly)
  {
    if (meta.Dimensions == 2)
    {
      Read2DVolumeConn(fin, meta.NumHexCells, meta.NumTetCells, volGrid);
    }
    else
    {
      Read3DVolumeConn(fin, meta.NumHexCells, meta.NumTetCells, meta.NumPriCells, meta.NumPyrCells,
        BuildConnectivityIteratively, volGrid);
    }
  }

  // Now work with the surface data
  BuildBoundaryPartitionedDataSets(output, points, meta.Patches, bfaces);
}

VTK_ABI_NAMESPACE_END
