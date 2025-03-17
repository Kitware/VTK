// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef AvmeshMetadata_h
#define AvmeshMetadata_h

#include <string>
#include <vector>

class vtkFieldData;

struct AvmeshPatch
{
  AvmeshPatch();

  void ToFieldData(vtkFieldData* fieldData) const;

  char Label[32]; // descriptive name, e.g. "wing", "inlet"
  char Type[16];  // BC, e.g. "noslipwall", "unspecified"
  int Pid;        // Patch ID, always negative
};

struct AvmeshMetadata
{
  AvmeshMetadata();

  void ToFieldData(vtkFieldData* fieldData) const;

  // File header
  char MagicString[6];
  int MagicNumber;
  int Version;
  int MeshCount;
  char ContactInfo[128];
  int Precision;
  int Dimensions;
  std::string Description;

  // Mesh header
  char MeshName[128];
  char MeshType[128];
  char MeshGenerator[128];
  char CoordinateSystem[128];
  double Scale;
  char Units[128];
  double RefLen[3]; // Scalar in rev1
  double RefArea;
  double RefPoint[3];
  char RefDescription[128];
  int Refined; // rev2 only
  char MeshDescription[128];

  // Unstruc header
  int NumNodes;
  int NumFaces;
  int NumCells;
  int MaxNodesPerFace;
  int MaxNodesPerCell;
  int MaxFacesPerCell;
  char ElementScheme[32]; // rev2 only
  int FacePolyOrder;      // rev2 only
  int CellPolyOrder;      // rev2 only
  int NumPatches;
  int NumHexCells;
  int NumTetCells;
  int NumPriCells;
  int NumPyrCells;
  int NumPolyCells; // rev1 only
  int NumBndTriFaces;
  int NumTriFaces;
  int NumBndQuadFaces;
  int NumQuadFaces;
  int NumBndPolyCells;  // rev1 only
  int NumPolyFaces;     // rev1 only
  int BndPolyFacesSize; // rev1 only
  int PolyFacesSize;    // rev1 only
  int NumEdges;
  int NumNodesOnGeometry;
  int NumEdgesOnGeometry;
  int NumFacesOnGeometry;
  int GeomRegionId;

  std::vector<AvmeshPatch> Patches;
};

#endif // AvmeshMetadata_h
