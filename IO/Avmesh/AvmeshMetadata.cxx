// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "AvmeshMetadata.h"
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

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
  : label("")
  , type("")
  , pid(0)
{
}

void AvmeshPatch::ToFieldData(vtkFieldData* fieldData) const
{
  AddFieldString(fieldData, "label", label);
  AddFieldString(fieldData, "type", type);
  AddFieldInt(fieldData, "pid", pid);
}

AvmeshMetadata::AvmeshMetadata()
  : magicString("")
{
}

void AvmeshMetadata::ToFieldData(vtkFieldData* fieldData) const
{
  // skip magicString and magicNumber since they're always the same
  AddFieldInt(fieldData, "version", version);
  AddFieldInt(fieldData, "meshCount", meshCount);
  AddFieldString(fieldData, "contactInfo", contactInfo);
  AddFieldInt(fieldData, "precision", precision);
  AddFieldInt(fieldData, "dimensions", dimensions);
  AddFieldString(fieldData, "description", description);

  AddFieldString(fieldData, "meshName", meshName);
  AddFieldString(fieldData, "meshType", meshType);
  AddFieldString(fieldData, "meshGenerator", meshGenerator);
  AddFieldString(fieldData, "coordinateSystem", coordinateSystem);
  AddFieldDouble(fieldData, "scale", scale);
  AddFieldString(fieldData, "units", units);
  AddFieldDoubleTuple(fieldData, "referenceLength", refLen);
  AddFieldDouble(fieldData, "referenceArea", refArea);
  AddFieldDoubleTuple(fieldData, "referencePoint", refPoint);
  AddFieldString(fieldData, "referenceDescription", refDescription);
  AddFieldInt(fieldData, "refined", refined);
  AddFieldString(fieldData, "meshDescription", meshDescription);

  AddFieldInt(fieldData, "nNodes", nNodes);
  AddFieldInt(fieldData, "nFaces", nFaces);
  AddFieldInt(fieldData, "nCells", nCells);
  AddFieldInt(fieldData, "nMaxNodesPerFace", nMaxNodesPerFace);
  AddFieldInt(fieldData, "nMaxNodesPerCell", nMaxNodesPerCell);
  AddFieldInt(fieldData, "nMaxFacesPerCell", nMaxFacesPerCell);
  AddFieldString(fieldData, "elementScheme", elementScheme);
  AddFieldInt(fieldData, "facePolyOrder", facePolyOrder);
  AddFieldInt(fieldData, "cellPolyOrder", cellPolyOrder);
  AddFieldInt(fieldData, "nPatches", nPatches);
  AddFieldInt(fieldData, "nHexCells", nHexCells);
  AddFieldInt(fieldData, "nTetCells", nTetCells);
  AddFieldInt(fieldData, "nPriCells", nPriCells);
  AddFieldInt(fieldData, "nPyrCells", nPyrCells);
  AddFieldInt(fieldData, "nPolyCells", nPolyCells);
  AddFieldInt(fieldData, "nBndTriFaces", nBndTriFaces);
  AddFieldInt(fieldData, "nTriFaces", nTriFaces);
  AddFieldInt(fieldData, "nBndQuadFaces", nBndQuadFaces);
  AddFieldInt(fieldData, "nQuadFaces", nQuadFaces);
  AddFieldInt(fieldData, "nBndPolyCells", nBndPolyCells);
  AddFieldInt(fieldData, "nPolyFaces", nPolyFaces);
  AddFieldInt(fieldData, "bndPolyFacesSize", bndPolyFacesSize);
  AddFieldInt(fieldData, "polyFacesSize", polyFacesSize);
  AddFieldInt(fieldData, "nEdges", nEdges);
  AddFieldInt(fieldData, "nNodesOnGeometry", nNodesOnGeometry);
  AddFieldInt(fieldData, "nEdgesOnGeometry", nEdgesOnGeometry);
  AddFieldInt(fieldData, "nFacesOnGeometry", nFacesOnGeometry);
  AddFieldInt(fieldData, "geomRegionId", geomRegionId);
}
