// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBYUReader.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStringScanner.h"

#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBYUReader);

vtkBYUReader::vtkBYUReader()
{
  this->GeometryFileName = nullptr;
  this->DisplacementFileName = nullptr;
  this->ScalarFileName = nullptr;
  this->TextureFileName = nullptr;

  this->ReadDisplacement = 1;
  this->ReadScalar = 1;
  this->ReadTexture = 1;

  this->PartNumber = 0;

  this->SetNumberOfInputPorts(0);
}

vtkBYUReader::~vtkBYUReader()
{
  delete[] this->GeometryFileName;
  delete[] this->DisplacementFileName;
  delete[] this->ScalarFileName;
  delete[] this->TextureFileName;
}

int vtkBYUReader::CanReadFile(const char* filename)
{
  FILE* fp = vtksys::SystemTools::Fopen(filename, "r");
  if (fp == nullptr)
    return 0;

  auto resultNum = vtk::scan<int, int, int, int>(fp, "{:d} {:d} {:d} {:d}");
  if (!resultNum)
  {
    fclose(fp);
    return 0;
  }
  auto& [numParts, numPts, numPolys, numEdges] = resultNum->values();
  if ((numParts < 1) || (numPts < 1) || (numPolys < 1))
  {
    fclose(fp);
    return 0;
  }

  for (int part = 0; part < numParts; part++)
  {
    auto resultPart = vtk::scan<int, int>(fp, "{:d} {:d}");
    if (!resultPart)
    {
      fclose(fp);
      return 0;
    }
    auto& [partStart, partEnd] = resultPart->values();
    if ((partStart < 1) || (partStart > numPolys) || (partEnd < 1) || (partEnd > numPolys) ||
      (partStart >= partEnd))
    {
      fclose(fp);
      return 0;
    }
  }

  fclose(fp);
  return 1;
}

int vtkBYUReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  FILE* geomFp;
  int numPts;

  if (this->GeometryFileName == nullptr || this->GeometryFileName[0] == '\0')
  {
    vtkErrorMacro(<< "No GeometryFileName specified!");
    return 0;
  }
  if ((geomFp = vtksys::SystemTools::Fopen(this->GeometryFileName, "r")) == nullptr)
  {
    vtkErrorMacro(<< "Geometry file: " << this->GeometryFileName << " not found");
    return 0;
  }
  else
  {
    this->ReadGeometryFile(geomFp, numPts, outInfo);
    fclose(geomFp);
  }

  this->ReadDisplacementFile(numPts, outInfo);
  this->ReadScalarFile(numPts, outInfo);
  this->ReadTextureFile(numPts, outInfo);
  this->UpdateProgress(1.0);

  return 1;
}

void vtkBYUReader::ReadGeometryFile(FILE* geomFile, int& numPts, vtkInformation* outInfo)
{
  int numParts, numPolys, numEdges;
  int partStart, partEnd;
  int i;
  vtkPoints* newPts;
  vtkCellArray* newPolys;
  float x[3];
  vtkIdList* pts;
  int polyId, pt = 0;
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  //
  // Read header (not using fixed format! - potential problem in some files.)
  //
  auto resultNum = vtk::scan<int, int, int, int>(geomFile, "{:d} {:d} {:d} {:d}");
  if (!resultNum)
  {
    vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                  << "Message: " << resultNum.error().msg());
    return;
  }
  std::tie(numParts, numPts, numPolys, numEdges) = resultNum->values();

  if (this->PartNumber > numParts)
  {
    vtkWarningMacro(<< "Specified part number > number of parts");
    this->PartNumber = 0;
  }

  if (this->PartNumber > 0) // read just part specified
  {
    vtkDebugMacro(<< "Reading part number: " << this->PartNumber);
    for (i = 0; i < (this->PartNumber - 1); i++)
    {
      auto resultSkip = vtk::scan<int, int>(geomFile, "{:d} {:d}");
      if (!resultSkip)
      {
        vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                      << "Message: " << resultSkip.error().msg());
        return;
      }
    }
    auto resultPart = vtk::scan<int, int>(geomFile, "{:d} {:d}");
    if (!resultPart)
    {
      vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                    << "Message: " << resultPart.error().msg());
      return;
    }
    std::tie(partStart, partEnd) = resultPart->values();
    for (i = this->PartNumber; i < numParts; i++)
    {
      auto resultSkip = vtk::scan<int, int>(geomFile, "{:d} {:d}");
      if (!resultSkip)
      {
        vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                      << "Message: " << resultSkip.error().msg());
        return;
      }
    }
  }
  else // read all parts
  {
    vtkDebugMacro(<< "Reading all parts.");
    for (i = 0; i < numParts; i++)
    {
      auto resultSkip = vtk::scan<int, int>(geomFile, "{:d} {:d}");
      if (!resultSkip)
      {
        vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                      << "Message: " << resultSkip.error().msg());
        return;
      }
    }
    partStart = 1;
    partEnd = VTK_INT_MAX;
  }

  if (numParts < 1 || numPts < 1 || numPolys < 1)
  {
    vtkErrorMacro(<< "Bad MOVIE.BYU file");
    pts->Delete();
    return;
  }
  //
  // Allocate data objects
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->AllocateEstimate(numPolys + numEdges, 1);
  //
  // Read data
  //
  // read point coordinates
  for (i = 0; i < numPts; i++)
  {
    auto resultPoint = vtk::scan<float, float, float>(geomFile, "{} {} {}");
    if (!resultPoint)
    {
      vtkErrorMacro(<< "Error reading geometry file: " << this->GeometryFileName
                    << "Message: " << resultPoint.error().msg());
      return;
    }
    std::tie(x[0], x[1], x[2]) = resultPoint->values();
    newPts->InsertPoint(i, x);
  }
  this->UpdateProgress(0.333);

  // read poly data. Have to fix 1-offset. Only reading part number specified.
  for (polyId = 1; polyId <= numPolys; polyId++)
  {
    // read this polygon
    vtk::scan_result_type<std::FILE*&, int> resultPolyId;
    for (pts->Reset();
         ((resultPolyId = vtk::scan_value<int>(geomFile))) && (pt = resultPolyId->value()) > 0;)
    {
      pts->InsertNextId(pt - 1); // convert to vtk 0-offset
    }
    pts->InsertNextId(-(pt + 1));

    // Insert polygon (if in selected part)
    if (partStart <= polyId && polyId <= partEnd)
    {
      newPolys->InsertNextCell(pts);
    }
  }
  this->UpdateProgress(0.6667);

  vtkDebugMacro(<< "Reading:" << numPts << " points, " << numPolys << " polygons.");

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
  pts->Delete();
}

void vtkBYUReader::ReadDisplacementFile(int numPts, vtkInformation* outInfo)
{
  FILE* dispFp;
  int i;
  float v[3];
  vtkFloatArray* newVectors;
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->ReadDisplacement && this->DisplacementFileName)
  {
    if (!(dispFp = vtksys::SystemTools::Fopen(this->DisplacementFileName, "r")))
    {
      vtkErrorMacro(<< "Couldn't open displacement file");
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Allocate and read data
  //
  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->SetNumberOfTuples(numPts);

  for (i = 0; i < numPts; i++)
  {
    auto resultDisp = vtk::scan<float, float, float>(dispFp, "{} {} {}");
    if (!resultDisp)
    {
      vtkErrorMacro(<< "Error reading displacement file: " << this->DisplacementFileName
                    << "Message: " << resultDisp.error().msg());
      fclose(dispFp);
      return;
    }
    std::tie(v[0], v[1], v[2]) = resultDisp->values();
    newVectors->SetTypedTuple(i, v);
  }

  fclose(dispFp);
  vtkDebugMacro(<< "Read " << numPts << " displacements");

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBYUReader::ReadScalarFile(int numPts, vtkInformation* outInfo)
{
  FILE* scalarFp;
  int i;
  float s;
  vtkFloatArray* newScalars;
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->ReadScalar && this->ScalarFileName)
  {
    if (!(scalarFp = vtksys::SystemTools::Fopen(this->ScalarFileName, "r")))
    {
      vtkErrorMacro(<< "Couldn't open scalar file");
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Allocate and read data
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  for (i = 0; i < numPts; i++)
  {
    auto resultScalar = vtk::scan_value<float>(scalarFp);
    if (!resultScalar)
    {
      vtkErrorMacro(<< "Error reading scalar file: " << this->ScalarFileName
                    << "Message: " << resultScalar.error().msg());
      fclose(scalarFp);
      return;
    }
    s = resultScalar->value();
    newScalars->SetValue(i, s);
  }

  fclose(scalarFp);
  vtkDebugMacro(<< "Read " << numPts << " scalars");

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBYUReader::ReadTextureFile(int numPts, vtkInformation* outInfo)
{
  FILE* textureFp;
  int i;
  float t[2];
  vtkFloatArray* newTCoords;
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->ReadTexture && this->TextureFileName)
  {
    if (!(textureFp = vtksys::SystemTools::Fopen(this->TextureFileName, "r")))
    {
      vtkErrorMacro(<< "Couldn't open texture file");
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Allocate and read data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(3);
  newTCoords->SetNumberOfTuples(numPts);

  for (i = 0; i < numPts; i++)
  {
    auto resultTCoord = vtk::scan<float, float>(textureFp, "{:e} {:e}");
    if (!resultTCoord)
    {
      vtkErrorMacro(<< "Error reading texture file: " << this->TextureFileName
                    << "Message: " << resultTCoord.error().msg());
      fclose(textureFp);
      return;
    }
    std::tie(t[0], t[1]) = resultTCoord->values();
    newTCoords->SetTypedTuple(i, t);
  }

  fclose(textureFp);
  vtkDebugMacro(<< "Read " << numPts << " texture coordinates");

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

//------------------------------------------------------------------------------
// This source does not know how to generate pieces yet.
int vtkBYUReader::ComputeDivisionExtents(
  vtkDataObject* vtkNotUsed(output), int idx, int numDivisions)
{
  if (idx == 0 && numDivisions == 1)
  {
    // I will give you the whole thing
    return 1;
  }
  else
  {
    // I have nothing to give you for this piece.
    return 0;
  }
}

void vtkBYUReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Geometry File Name: " << (this->GeometryFileName ? this->GeometryFileName : "(none)")
     << "\n";
  os << indent << "Read Displacement: " << (this->ReadDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement File Name: "
     << (this->DisplacementFileName ? this->DisplacementFileName : "(none)") << "\n";
  os << indent << "Part Number: " << this->PartNumber << "\n";
  os << indent << "Read Scalar: " << (this->ReadScalar ? "On\n" : "Off\n");
  os << indent << "Scalar File Name: " << (this->ScalarFileName ? this->ScalarFileName : "(none)")
     << "\n";
  os << indent << "Read Texture: " << (this->ReadTexture ? "On\n" : "Off\n");
  os << indent
     << "Texture File Name: " << (this->TextureFileName ? this->TextureFileName : "(none)") << "\n";
}
VTK_ABI_NAMESPACE_END
