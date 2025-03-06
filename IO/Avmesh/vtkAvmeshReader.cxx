#include "vtkAvmeshReader.h"
#include "AvmeshMetadata.h"
#include "vtkCellType.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGrid.h"

vtkAvmeshReader::vtkAvmeshReader()
  : FileName("")
  , SurfaceOnly(false)
{
}

vtkAvmeshReader::~vtkAvmeshReader() = default;

int vtkAvmeshReader::CanReadFile(VTK_FILEPATH const char* filename)
{
  std::ifstream fin(filename, std::ios::binary);

  if (fin.good())
  {
    char buff[6];
    fin.read(buff, sizeof(buff));
    if (strncmp(buff, "AVMESH", 6) == 0)
    {
      return 1;
    }
  }

  return 0;
}

int vtkAvmeshReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make surf the file is ready for reading
  std::ifstream fin(FileName.c_str(), std::ios::binary);
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

bool vtkAvmeshReader::ReadMetadata(std::ifstream& fin, AvmeshMetadata& meta) const
{
  // File header
  ReadString(fin, meta.magicString, 6);
  if (strncmp(meta.magicString, "AVMESH", 6) != 0)
  {
    return false;
  }

  meta.magicNumber = ReadInt(fin); // FIXME need to allow for byteswapping
  if (meta.magicNumber != 1)
  {
    return false;
  }

  meta.version = ReadInt(fin);
  meta.meshCount = ReadInt(fin);
  ReadString(fin, meta.contactInfo);
  meta.precision = ReadInt(fin);
  meta.dimensions = ReadInt(fin);
  ReadVarString(fin, meta.description);

  // Mesh header
  ReadString(fin, meta.meshName);
  ReadString(fin, meta.meshType);
  ReadString(fin, meta.meshGenerator);
  ReadString(fin, meta.coordinateSystem);
  meta.scale = ReadDouble(fin);
  ReadString(fin, meta.units);

  if (meta.version == 1)
  {
    meta.refLen[0] = ReadDouble(fin);
    meta.refLen[1] = meta.refLen[0];
    meta.refLen[2] = meta.refLen[0];
  }
  else
  {
    ReadArray(fin, meta.refLen, 3);
  }

  meta.refArea = ReadDouble(fin);
  ReadArray(fin, meta.refPoint, 3);
  ReadString(fin, meta.refDescription);

  meta.refined = (meta.version == 2) ? ReadInt(fin) : 0;

  ReadString(fin, meta.meshDescription);

  // Unstruc header
  meta.nNodes = ReadInt(fin);
  meta.nFaces = ReadInt(fin);
  meta.nCells = ReadInt(fin);
  meta.nMaxNodesPerFace = ReadInt(fin);
  meta.nMaxNodesPerCell = ReadInt(fin);
  meta.nMaxFacesPerCell = ReadInt(fin);

  if (meta.version == 2)
  {
    ReadString(fin, meta.elementScheme, sizeof(meta.elementScheme));
    meta.facePolyOrder = ReadInt(fin);
    meta.cellPolyOrder = ReadInt(fin);
  }

  meta.nPatches = ReadInt(fin);
  meta.nHexCells = ReadInt(fin);
  meta.nTetCells = ReadInt(fin);
  meta.nPriCells = ReadInt(fin);
  meta.nPyrCells = ReadInt(fin);

  meta.nPolyCells = (meta.version == 1) ? ReadInt(fin) : 0;

  meta.nBndTriFaces = ReadInt(fin);
  meta.nTriFaces = ReadInt(fin);
  meta.nBndQuadFaces = ReadInt(fin);
  meta.nQuadFaces = ReadInt(fin);

  if (meta.version == 1)
  {
    meta.nBndPolyCells = ReadInt(fin);
    meta.nPolyFaces = ReadInt(fin);
    meta.bndPolyFacesSize = ReadInt(fin);
    meta.polyFacesSize = ReadInt(fin);
  }
  else
  {
    meta.nBndPolyCells = 0;
    meta.nPolyFaces = 0;
    meta.bndPolyFacesSize = 0;
    meta.polyFacesSize = 0;
  }

  meta.nEdges = ReadInt(fin);
  meta.nNodesOnGeometry = ReadInt(fin);
  meta.nEdgesOnGeometry = ReadInt(fin);
  meta.nFacesOnGeometry = ReadInt(fin);
  meta.geomRegionId = ReadInt(fin);

  // Patch info
  meta.patches.resize(meta.nPatches);
  for (auto& patch : meta.patches)
  {
    patch.pid = ReadInt(fin);
    ReadString(fin, patch.label, sizeof(patch.label));
    ReadString(fin, patch.type, sizeof(patch.type));
  }

  return true;
}

bool vtkAvmeshReader::CheckAssumptions(AvmeshMetadata const& meta) const
{
  bool readable = true;

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

  if (meta.meshCount > 1)
  {
    vtkWarningMacro("Multi-mesh AVMESH file detected.  Only the first mesh will be read.");
  }

  if (strncmp(meta.meshType, "unstruc", sizeof(meta.meshType)) != 0)
  {
    vtkErrorMacro("Only unstruc files allowed");
    readable = false;
  }

  if (meta.facePolyOrder != 1 || meta.cellPolyOrder != 1)
  {
    vtkErrorMacro("Only linear (P1) meshes allowed");
    readable = false;
  }

  if (meta.nPolyCells != 0 || meta.nBndPolyCells != 0 || meta.nPolyFaces != 0 ||
    meta.bndPolyFacesSize != 0 || meta.polyFacesSize != 0)
  {
    vtkErrorMacro("Arbitrary polyhedral grids not allowed");
    readable = false;
  }

  return readable;
}

vtkSmartPointer<vtkPoints> vtkAvmeshReader::ReadVolumeVerts(std::ifstream& fin, int nNodes) const
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nNodes);
  double* buff = (double*)(points->GetVoidPointer(0));
  size_t nitems = (size_t)nNodes * 3;
  ReadArray(fin, buff, nitems);
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
  std::ifstream& fin, int nbnd, BfaceList& bfaces, bool fileHasNeighbors) const
{
  // For 2D grids, we read the boundary edges as if they are triangles
  for (int i = 0; i < nbnd; ++i)
  {
    ReadArray(fin, &bfaces[i][0], 3);
    bfaces[i][3] = bfaces[i][2] = bfaces[i][1]; // duplicate nodes to mark it as a line
    if (fileHasNeighbors)
    {
      (void)ReadInt(fin);
    }
    bfaces[i][4] = ReadInt(fin);
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
  std::ifstream& fin, int ntri, int nquad, BfaceList& bfaces, bool fileHasNeighbors) const
{
  // tris
  for (int i = 0; i < ntri; ++i)
  {
    ReadArray(fin, &bfaces[i][0], 3);
    bfaces[i][3] = bfaces[i][2]; // duplicate nodes to mark it as a triangle
    if (fileHasNeighbors)
    {
      (void)ReadInt(fin);
    }
    bfaces[i][4] = ReadInt(fin);
  }

  // quads
  for (int i = ntri; i < ntri + nquad; ++i)
  {
    ReadArray(fin, &bfaces[i][0], 4);
    if (fileHasNeighbors)
    {
      (void)ReadInt(fin);
    }
    bfaces[i][4] = ReadInt(fin);
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
  std::ifstream& fin, int nquad, int ntri, vtkUnstructuredGrid* ugrid) const
{
  size_t ncell = (size_t)nquad + (size_t)ntri;
  size_t connSize = 4 * (size_t)nquad + 3 * (size_t)ntri;

  ugrid->GetCells()->Use32BitStorage();
  ugrid->AllocateExact(ncell, connSize);

  int cell[8];
  vtkIdType nodeids[8];

  // Read quads first.  Note that quads are stored as hexs, but only the first
  // 4 nodes are significant.
  for (int i = 0; i < nquad; ++i)
  {
    ReadArray(fin, cell, 8);
    for (int j = 0; j < 4; ++j)
    {
      nodeids[j] = cell[j] - 1;
    }
    ugrid->InsertNextCell(VTK_QUAD, 4, nodeids);
  }

  // Then read tris.  Note that tris are stored as tets, but only the first
  // 3 nodes are significant.
  for (int i = 0; i < ntri; ++i)
  {
    ReadArray(fin, cell, 4); // tris are stored as tets
    for (int j = 0; j < 3; ++j)
    {
      nodeids[j] = cell[j] - 1;
    }
    ugrid->InsertNextCell(VTK_TRIANGLE, 3, nodeids);
  }
}

void vtkAvmeshReader::Read3DVolumeConn(
  std::ifstream& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid) const
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
  std::ifstream& fin, int etype, int ncell, vtkUnstructuredGrid* ugrid) const
{
  int nNodesPerCell = NodesPerCell(etype);
  int cell[8];
  vtkIdType nodeids[8];
  for (int i = 0; i < ncell; ++i)
  {
    ReadArray(fin, cell, nNodesPerCell);
    for (int j = 0; j < nNodesPerCell; ++j)
    {
      nodeids[j] = cell[j] - 1;
    }
    if (etype == VTK_WEDGE)
    {
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
  int nface = lastFace - firstFace;
  std::vector<int> s2v;
  s2v.reserve(4 * nface);
  for (auto face = firstFace; face != lastFace; ++face)
  {
    std::copy_n(face->begin(), 4, std::back_inserter(s2v));
  }
  makeUnique(s2v);

  int pnnode = s2v.size();

  int nmax = *std::max_element(s2v.begin(), s2v.end());
  std::vector<int> v2s(nmax + 1);
  for (int i = 0; i < pnnode; ++i)
  {
    v2s[s2v[i]] = i;
  }

  auto surfPoints = vtkSmartPointer<vtkPoints>::New();
  surfPoints->SetDataTypeToDouble();
  surfPoints->Allocate(pnnode);
  for (int i = 0; i < pnnode; ++i)
  {
    auto pt = volPoints->GetPoint(s2v[i]);
    surfPoints->InsertNextPoint(pt);
  }
  surfGrid->SetPoints(surfPoints);

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

void vtkAvmeshReader::ReadString(std::ifstream& fin, char* str, int length) const
{
  fin.read(str, length);
}

void vtkAvmeshReader::ReadVarString(std::ifstream& fin, std::string& s) const
{
  int n = ReadInt(fin);
  s.resize(n);
  ReadString(fin, s.data(), n);
}

int vtkAvmeshReader::ReadInt(std::ifstream& fin) const
{
  int n;
  ReadArray(fin, &n, 1);
  return n;
}

double vtkAvmeshReader::ReadDouble(std::ifstream& fin) const
{
  double x;
  ReadArray(fin, &x, 1);
  return x;
}
