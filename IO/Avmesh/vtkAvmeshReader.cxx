#include "vtkAvmeshReader.h"
#include "AvmeshMetadata.h"
#include "BinaryFile.h"

#include <vtkCellType.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkUnstructuredGrid.h>
#include <vtksys/SystemTools.hxx>

vtkAvmeshReader::vtkAvmeshReader()
  : FileName("")
  , SurfaceOnly(false)
{
}

vtkAvmeshReader::~vtkAvmeshReader() = default;

int vtkAvmeshReader::CanReadFile(VTK_FILEPATH const char* filename)
{
  return vtksys::SystemTools::TestFileAccess(filename, vtksys::TEST_FILE_READ);
}

int vtkAvmeshReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make surf the file is ready for reading
  BinaryFile fin(FileName.c_str());
  if (!fin.good())
  {
    vtkErrorMacro(<< "Could not open AVMESH file");
    return 0;
  }

  // Read all the metadata in one big chunk
  AvmeshMetadata meta;
  if (!ReadMetadata(fin, meta))
  {
    return 0;
  }

  // Make sure the metadata conforms to our assumptions
  if (!CheckAssumptions(meta))
  {
    return 0;
  }

  // Read all the points.  Need to read them all even if we're in surface-only
  // mode because there is no guarantee that the surface points will come first.
  auto points = ReadVolumeVerts(fin, meta.nNodes);

  // If we're reading the volume grid, construct the volume block and attach
  // the points to it.
  vtkSmartPointer<vtkUnstructuredGrid> volGrid = nullptr;
  if (!SurfaceOnly)
  {
    volGrid = AddBlock(output, "Flowfield");
    volGrid->SetPoints(points);
  }

  // Read the surface data as one big block.  We'll sort it into patches later.
  bool readNeighborData = (meta.version == 1);
  BfaceList bfaces(meta.nBndTriFaces + meta.nBndQuadFaces);
  if (meta.dimensions == 2)
  {
    Read2DSurfaceConn(fin, meta.nBndTriFaces, bfaces, readNeighborData);
  }
  else
  {
    Read3DSurfaceConn(fin, meta.nBndTriFaces, meta.nBndQuadFaces, bfaces, readNeighborData);
  }

  // If we're reading the volume grid, read the connectivity
  if (!SurfaceOnly)
  {
    if (meta.dimensions == 2)
    {
      Read2DVolumeConn(fin, meta.nHexCells, meta.nTetCells, volGrid);
    }
    else
    {
      Read3DVolumeConn(
        fin, meta.nHexCells, meta.nTetCells, meta.nPriCells, meta.nPyrCells, volGrid);
    }
  }

  // Now work with the surface data
  BuildBoundaryBlocks(output, points, meta.patches, bfaces);

  return 1;
}

bool vtkAvmeshReader::ReadMetadata(BinaryFile& fin, AvmeshMetadata& meta) const
{
  // File header
  // Make sure the magic string is in place
  fin.ReadCString(meta.magicString, 6);
  if (strncmp(meta.magicString, "AVMESH", 6) != 0)
  {
    return false;
  }

  // Use the magic number to determine if byte-swapping is needed.  NOTE: while
  // the AVMESH standard theoretically allows for big-endian files, practically
  // speaking, they're always little-endian.
  meta.magicNumber = fin.ReadInt();
  if (meta.magicNumber != 1)
  {
    if (BinaryFile::SwapInt(meta.magicNumber) == 1)
    {
      fin.SetSwap(true);
    }
    else
    {
      return false;
    }
  }

  meta.version = fin.ReadInt(); // must be 1 or 2, but we'll verify this later
  meta.meshCount = fin.ReadInt();
  fin.ReadCString(meta.contactInfo);
  meta.precision = fin.ReadInt();
  meta.dimensions = fin.ReadInt();
  meta.description = fin.ReadStdString();

  // Mesh header
  fin.ReadCString(meta.meshName);
  fin.ReadCString(meta.meshType); // must be "unstruc", but we'll check later
  fin.ReadCString(meta.meshGenerator);
  fin.ReadCString(meta.coordinateSystem);
  meta.scale = fin.ReadDouble();
  fin.ReadCString(meta.units);

  if (meta.version == 1)
  {
    meta.refLen[0] = fin.ReadDouble();
    meta.refLen[1] = meta.refLen[0];
    meta.refLen[2] = meta.refLen[0];
  }
  else
  {
    fin.ReadArray(meta.refLen, 3);
  }

  meta.refArea = fin.ReadDouble();
  fin.ReadArray(meta.refPoint, 3);
  fin.ReadCString(meta.refDescription);

  meta.refined = (meta.version == 2) ? fin.ReadInt() : 0;

  fin.ReadCString(meta.meshDescription);

  // Unstruc header
  meta.nNodes = fin.ReadInt();
  meta.nFaces = fin.ReadInt();
  meta.nCells = fin.ReadInt();
  meta.nMaxNodesPerFace = fin.ReadInt();
  meta.nMaxNodesPerCell = fin.ReadInt();
  meta.nMaxFacesPerCell = fin.ReadInt();

  if (meta.version == 2)
  {
    fin.ReadCString(meta.elementScheme, sizeof(meta.elementScheme));
    meta.facePolyOrder = fin.ReadInt();
    meta.cellPolyOrder = fin.ReadInt();
  }

  meta.nPatches = fin.ReadInt();
  meta.nHexCells = fin.ReadInt();
  meta.nTetCells = fin.ReadInt();
  meta.nPriCells = fin.ReadInt();
  meta.nPyrCells = fin.ReadInt();

  meta.nPolyCells = (meta.version == 1) ? fin.ReadInt() : 0;

  meta.nBndTriFaces = fin.ReadInt();
  meta.nTriFaces = fin.ReadInt();
  meta.nBndQuadFaces = fin.ReadInt();
  meta.nQuadFaces = fin.ReadInt();

  if (meta.version == 1)
  {
    meta.nBndPolyCells = fin.ReadInt();
    meta.nPolyFaces = fin.ReadInt();
    meta.bndPolyFacesSize = fin.ReadInt();
    meta.polyFacesSize = fin.ReadInt();
  }
  else
  {
    meta.nBndPolyCells = 0;
    meta.nPolyFaces = 0;
    meta.bndPolyFacesSize = 0;
    meta.polyFacesSize = 0;
  }

  meta.nEdges = fin.ReadInt();
  meta.nNodesOnGeometry = fin.ReadInt();
  meta.nEdgesOnGeometry = fin.ReadInt();
  meta.nFacesOnGeometry = fin.ReadInt();
  meta.geomRegionId = fin.ReadInt();

  // Patch info
  meta.patches.resize(meta.nPatches);
  for (auto& patch : meta.patches)
  {
    patch.pid = fin.ReadInt();
    fin.ReadCString(patch.label, sizeof(patch.label));
    fin.ReadCString(patch.type, sizeof(patch.type));
  }

  return true;
}

bool vtkAvmeshReader::CheckAssumptions(AvmeshMetadata const& meta) const
{
  bool readable = true;

  // rev0 is a weird face-based format that nobody uses anymore,
  // and rev3 doesn't exist yet
  if (meta.version < 1 || meta.version > 2)
  {
    vtkErrorMacro("Only AVMESH rev1 and rev2 allowed");
    readable = false;
  }

  if (meta.dimensions < 2 || meta.dimensions > 3)
  {
    vtkErrorMacro("Dimensions must be 2 or 3");
    readable = false;
  }

  // Never seen a single precision one in the wild
  if (meta.precision != 2)
  {
    vtkErrorMacro("Only double precision supported");
    readable = false;
  }

  if (meta.meshCount < 1)
  {
    vtkErrorMacro("No meshes in file");
    readable = false;
  }

  // Never seen a multi-mesh AVMESH file in the wild
  if (meta.meshCount > 1)
  {
    vtkWarningMacro("Multi-mesh AVMESH file detected.  Only the first mesh will be read.");
  }

  if (strncmp(meta.meshType, "unstruc", sizeof(meta.meshType)) != 0)
  {
    vtkErrorMacro("Only unstruc files allowed");
    readable = false;
  }

  // Higher order AVMESH grids do exist in practice for use with COFFE,
  // but we're not going to support that here.
  if (meta.facePolyOrder != 1 || meta.cellPolyOrder != 1)
  {
    vtkErrorMacro("Only linear (P1) meshes allowed");
    readable = false;
  }

  // Arbitrary poly AVMESH files don't exist in practice since neither Kestrel
  // nor Helios support it.
  if (meta.nPolyCells != 0 || meta.nBndPolyCells != 0 || meta.nPolyFaces != 0 ||
    meta.bndPolyFacesSize != 0 || meta.polyFacesSize != 0)
  {
    vtkErrorMacro("Arbitrary polyhedral grids not allowed");
    readable = false;
  }

  return readable;
}

vtkSmartPointer<vtkPoints> vtkAvmeshReader::ReadVolumeVerts(BinaryFile& fin, int nNodes) const
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nNodes);
  double* buff = (double*)(points->GetVoidPointer(0));
  size_t nitems = (size_t)nNodes * 3;
  fin.ReadArray(buff, nitems);
  return points;
}

vtkSmartPointer<vtkUnstructuredGrid> vtkAvmeshReader::AddBlock(
  vtkMultiBlockDataSet* output, const char* blockName) const
{
  auto ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  int blockNum = output->GetNumberOfBlocks();
  output->SetBlock(blockNum, ugrid);
  output->GetMetaData(blockNum)->Set(vtkCompositeDataSet::NAME(), blockName);
  return ugrid;
}

void vtkAvmeshReader::Read2DSurfaceConn(
  BinaryFile& fin, int nbnd, BfaceList& bfaces, bool fileHasNeighbors) const
{
  // For 2D grids, we read the boundary edges as if they are triangles,
  // but only the first two nodes are significant.
  for (int i = 0; i < nbnd; ++i)
  {
    fin.ReadArray(&bfaces[i][0], 3);
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

void vtkAvmeshReader::Read3DSurfaceConn(
  BinaryFile& fin, int ntri, int nquad, BfaceList& bfaces, bool fileHasNeighbors) const
{
  // tris
  for (int i = 0; i < ntri; ++i)
  {
    fin.ReadArray(&bfaces[i][0], 3);
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
    fin.ReadArray(&bfaces[i][0], 4);
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

void vtkAvmeshReader::Read2DVolumeConn(
  BinaryFile& fin, int nquad, int ntri, vtkUnstructuredGrid* ugrid) const
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

void vtkAvmeshReader::Read3DVolumeConn(
  BinaryFile& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid) const
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

void vtkAvmeshReader::Read3DVolumeConnOfType(
  BinaryFile& fin, int etype, int ncell, vtkUnstructuredGrid* ugrid) const
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

int vtkAvmeshReader::NodesPerCell(int etype) const
{
  switch (etype)
  {
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

void vtkAvmeshReader::BuildBoundaryBlocks(vtkMultiBlockDataSet* output, vtkPoints* volPoints,
  std::vector<AvmeshPatch> const& patches, BfaceList& bfaces) const
{
  // There is no guarantee that the boundary connectivity and patch IDs will be
  // in any particular order.  So we need to group them together by patch ID.
  // Once we have all the faces that belong to a patch, we can construct a block
  // for that patch.
  auto firstFace = bfaces.begin();
  for (auto const& patch : patches)
  {
    auto lastFace = std::stable_partition(
      firstFace, bfaces.end(), [&patch](Bface const& face) { return face[4] == patch.pid; });
    auto surfGrid = AddBlock(output, patch.label);
    BuildSurfaceBlock(patch.pid, surfGrid, volPoints, firstFace, lastFace);
    firstFace = lastFace;
  }
}

static void makeUnique(std::vector<int>& vec)
{
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

void vtkAvmeshReader::BuildSurfaceBlock(int pid, vtkUnstructuredGrid* surfGrid,
  vtkPoints* volPoints, BfaceList::const_iterator firstFace,
  BfaceList::const_iterator lastFace) const
{
  // Start by constructing a surface-to-volume mapping.  This maps a surface
  // node ID for this patch to a volume node ID for the whole grid.
  int nface = lastFace - firstFace;
  std::vector<int> s2v;
  s2v.reserve(4 * nface);
  for (auto face = firstFace; face != lastFace; ++face)
  {
    std::copy_n(face->begin(), 4, std::back_inserter(s2v));
  }
  makeUnique(s2v);

  // Number of unique nodes on this patch
  int pnnode = s2v.size();

  // Now construct the volume-to-surface mapping, which maps a volume node ID
  // from the whole grid to a surface node ID on this patch.
  int nmax = *std::max_element(s2v.begin(), s2v.end());
  std::vector<int> v2s(nmax + 1);
  for (int i = 0; i < pnnode; ++i)
  {
    v2s[s2v[i]] = i;
  }

  // Now use the surface-to-volume mapping to extract the points needed for
  // this patch.
  auto surfPoints = vtkSmartPointer<vtkPoints>::New();
  surfPoints->SetDataTypeToDouble();
  surfPoints->Allocate(pnnode);
  for (int i = 0; i < pnnode; ++i)
  {
    auto pt = volPoints->GetPoint(s2v[i]);
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
      nodeids[i] = v2s[(*face)[i]];
    }
    surfGrid->InsertNextCell(etype, nodesPer, nodeids);
  }
}
