// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "AvmeshMetadata.h"
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
void AddFieldInt(vtkFieldData* fieldData, const char* name, int val)
{
  vtkNew<vtkIntArray> arr;
  arr->SetName(name);
  arr->InsertNextValue(val);
  fieldData->AddArray(arr);
}

void AddFieldDouble(vtkFieldData* fieldData, const char* name, double val)
{
  vtkNew<vtkDoubleArray> arr;
  arr->SetName(name);
  arr->InsertNextValue(val);
  fieldData->AddArray(arr);
}

void AddFieldDoubleTuple(vtkFieldData* fieldData, const char* name, const double vals[3])
{
  vtkNew<vtkDoubleArray> arr;
  arr->SetName(name);
  arr->SetNumberOfComponents(3);
  arr->InsertNextTuple3(vals[0], vals[1], vals[2]);
  fieldData->AddArray(arr);
}

void AddFieldString(vtkFieldData* fieldData, const char* name, std::string str)
{
  vtkNew<vtkStringArray> arr;
  arr->SetName(name);
  arr->InsertNextValue(str);
  fieldData->AddArray(arr);
}
} // namespace

AvmeshPatch::AvmeshPatch()
  : Label("")
  , Type("")
  , Pid(0)
{
}

void AvmeshPatch::ToFieldData(vtkFieldData* fieldData) const
{
  AddFieldString(fieldData, "Label", Label);
  AddFieldString(fieldData, "Type", Type);
  AddFieldInt(fieldData, "Pid", Pid);
}

AvmeshMetadata::AvmeshMetadata()
  : MagicString("")
{
}

void AvmeshMetadata::ToFieldData(vtkFieldData* fieldData) const
{
  // skip magicString and magicNumber since they're always the same
  AddFieldInt(fieldData, "Version", Version);
  AddFieldInt(fieldData, "MeshCount", MeshCount);
  AddFieldString(fieldData, "ContactInfo", ContactInfo);
  AddFieldInt(fieldData, "Precision", Precision);
  AddFieldInt(fieldData, "Dimensions", Dimensions);
  AddFieldString(fieldData, "Description", Description);

  AddFieldString(fieldData, "MeshName", MeshName);
  AddFieldString(fieldData, "MeshType", MeshType);
  AddFieldString(fieldData, "MeshGenerator", MeshGenerator);
  AddFieldString(fieldData, "CoordinateSystem", CoordinateSystem);
  AddFieldDouble(fieldData, "Scale", Scale);
  AddFieldString(fieldData, "Units", Units);
  AddFieldDoubleTuple(fieldData, "ReferenceLength", RefLen);
  AddFieldDouble(fieldData, "ReferenceArea", RefArea);
  AddFieldDoubleTuple(fieldData, "ReferencePoint", RefPoint);
  AddFieldString(fieldData, "ReferenceDescription", RefDescription);
  AddFieldInt(fieldData, "Refined", Refined);
  AddFieldString(fieldData, "MeshDescription", MeshDescription);

  AddFieldInt(fieldData, "NumNodes", NumNodes);
  AddFieldInt(fieldData, "NumFaces", NumFaces);
  AddFieldInt(fieldData, "NumCells", NumCells);
  AddFieldInt(fieldData, "MaxNodesPerFace", MaxNodesPerFace);
  AddFieldInt(fieldData, "MaxNodesPerCell", MaxNodesPerCell);
  AddFieldInt(fieldData, "MaxFacesPerCell", MaxFacesPerCell);
  AddFieldString(fieldData, "ElementScheme", ElementScheme);
  AddFieldInt(fieldData, "FacePolyOrder", FacePolyOrder);
  AddFieldInt(fieldData, "CellPolyOrder", CellPolyOrder);
  AddFieldInt(fieldData, "NumPatches", NumPatches);
  AddFieldInt(fieldData, "NumHexCells", NumHexCells);
  AddFieldInt(fieldData, "NumTetCells", NumTetCells);
  AddFieldInt(fieldData, "NumPriCells", NumPriCells);
  AddFieldInt(fieldData, "NumPyrCells", NumPyrCells);
  AddFieldInt(fieldData, "NumPolyCells", NumPolyCells);
  AddFieldInt(fieldData, "NumBndTriFaces", NumBndTriFaces);
  AddFieldInt(fieldData, "NumTriFaces", NumTriFaces);
  AddFieldInt(fieldData, "NumBndQuadFaces", NumBndQuadFaces);
  AddFieldInt(fieldData, "NumQuadFaces", NumQuadFaces);
  AddFieldInt(fieldData, "NumBndPolyCells", NumBndPolyCells);
  AddFieldInt(fieldData, "NumPolyFaces", NumPolyFaces);
  AddFieldInt(fieldData, "BndPolyFacesSize", BndPolyFacesSize);
  AddFieldInt(fieldData, "PolyFacesSize", PolyFacesSize);
  AddFieldInt(fieldData, "NumEdges", NumEdges);
  AddFieldInt(fieldData, "NumNodesOnGeometry", NumNodesOnGeometry);
  AddFieldInt(fieldData, "NumEdgesOnGeometry", NumEdgesOnGeometry);
  AddFieldInt(fieldData, "NumFacesOnGeometry", NumFacesOnGeometry);
  AddFieldInt(fieldData, "GeomRegionId", GeomRegionId);
}

VTK_ABI_NAMESPACE_END
