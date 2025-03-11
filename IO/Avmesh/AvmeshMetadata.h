#ifndef AvmeshMetadata_h
#define AvmeshMetadata_h

#include <string>
#include <vector>

class vtkFieldData;

struct AvmeshPatch
{
  AvmeshPatch();

  void ToFieldData(vtkFieldData* fieldData) const;

  char label[32];
  char type[16];
  int pid;
};

struct AvmeshMetadata
{
  AvmeshMetadata();

  void ToFieldData(vtkFieldData* fieldData) const;

  // File header
  char magicString[6];
  int magicNumber;
  int version;
  int meshCount;
  char contactInfo[128];
  int precision;
  int dimensions;
  std::string description;

  // Mesh header
  char meshName[128];
  char meshType[128];
  char meshGenerator[128];
  char coordinateSystem[128];
  double scale;
  char units[128];
  double refLen[3]; // Scalar in rev1
  double refArea;
  double refPoint[3];
  char refDescription[128];
  int refined; // rev2 only
  char meshDescription[128];

  // Unstruc header
  int nNodes;
  int nFaces;
  int nCells;
  int nMaxNodesPerFace;
  int nMaxNodesPerCell;
  int nMaxFacesPerCell;
  char elementScheme[32]; // rev2 only
  int facePolyOrder;      // rev2 only
  int cellPolyOrder;      // rev2 only
  int nPatches;
  int nHexCells;
  int nTetCells;
  int nPriCells;
  int nPyrCells;
  int nPolyCells; // rev1 only
  int nBndTriFaces;
  int nTriFaces;
  int nBndQuadFaces;
  int nQuadFaces;
  int nBndPolyCells;    // rev1 only
  int nPolyFaces;       // rev1 only
  int bndPolyFacesSize; // rev1 only
  int polyFacesSize;    // rev1 only
  int nEdges;
  int nNodesOnGeometry;
  int nEdgesOnGeometry;
  int nFacesOnGeometry;
  int geomRegionId;

  std::vector<AvmeshPatch> patches;
};

#endif // AvmeshMetadata_h
