// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// AvmeshMetadata stores all the the metadata included in the header of an
// AVMESH file (usually about 1500 bytes).  These are all defined in the AVMESH
// standard available at https://github.com/DOD-HPCMP-CREATE/avmeshlib.  This is
// a rigid standard.  All of these variables must be present and in the order in
// which they are declared below.  The lengths of string variables are all
// rigidly defined, usually at 128 characters.  If a user tries to set strings
// longer than these bounds, the tools in the CREATE-AV ecosystem that
// manipulate AVMESH files will truncate them.
//
// Exceptions to the rules:
// - the Description field in the main file header
// - fields marked rev1 or rev2 only

#ifndef AvmeshMetadata_h
#define AvmeshMetadata_h

#include <string>
#include <vector>
#include <vtkABINamespace.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkFieldData;

struct AvmeshPatch
{
  AvmeshPatch();

  void ToFieldData(vtkFieldData* fieldData) const;

  // These strings have maximum fixed lengths of 32 and 16 as specified in the AVMESH file format
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
  std::string Description; // written as an int length, then a C string

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

VTK_ABI_NAMESPACE_END
#endif // AvmeshMetadata_h
