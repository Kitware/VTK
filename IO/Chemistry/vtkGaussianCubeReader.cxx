// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGaussianCubeReader.h"

#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include <vtksys/SystemTools.hxx>

#include <cctype>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGaussianCubeReader);

//------------------------------------------------------------------------------
// Construct object with merging set to true.
vtkGaussianCubeReader::vtkGaussianCubeReader()
{
  this->Transform = vtkTransform::New();
  // Add the second output for the grid data

  this->SetNumberOfOutputPorts(2);
  vtkImageData* grid;
  grid = vtkImageData::New();
  grid->ReleaseData();
  this->GetExecutive()->SetOutputData(1, grid);
  grid->Delete();
}

//------------------------------------------------------------------------------
vtkGaussianCubeReader::~vtkGaussianCubeReader()
{

  this->Transform->Delete();
  // must delete the second output added
}

//------------------------------------------------------------------------------
int vtkGaussianCubeReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE* fp;
  char title[256];
  char data_name[256];
  double elements[16];
  int JN1, N1N2, n1, n2, n3, i, j, k;
  bool orbitalCubeFile = false;
  int numberOfOrbitals;

  // Output 0 (the default is the polydata)
  // Output 1 will be the gridded Image data

  vtkImageData* grid = this->GetGridOutput();

  if (!this->FileName)
  {
    return 0;
  }

  if ((fp = vtksys::SystemTools::Fopen(this->FileName, "r")) == nullptr)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  if (!fgets(title, 256, fp))
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // TODO: SystemTools::Split should be replaced by a SystemTools::SplitN call
  // which only splits up to N times as soon as it exists
  std::vector<std::string> tokens;
  vtksys::SystemTools::Split(title, tokens, ':');
  if (tokens.size() > 2)
  {
    for (std::size_t token = 3; token < tokens.size(); ++token)
    {
      tokens[2] += ":" + tokens[token];
    }
    strcpy(data_name, tokens[2].c_str());
    vtk::print(stderr, "label = {:s}\n", data_name);
  }

  if (!fgets(title, 256, fp))
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // Read in number of atoms, x-origin, y-origin z-origin

  // Need to read number of atoms into a temp variable to avoid issues with the variable
  // size of vtkIdType
  long long numberOfAtoms;
  auto resultAtoms = vtk::scan<long long, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultAtoms)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading atoms, x-origin y-origin z-origin.");
    fclose(fp);
    return 0;
  }
  std::tie(numberOfAtoms, elements[3], elements[7], elements[11]) = resultAtoms->values();

  this->NumberOfAtoms = numberOfAtoms;

  if (this->NumberOfAtoms < 0)
  {
    this->NumberOfAtoms = -this->NumberOfAtoms;
    orbitalCubeFile = true;
  }

  auto resultN1 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN1)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  std::tie(n1, elements[0], elements[4], elements[8]) = resultN1->values();
  auto resultN2 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN2)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  std::tie(n2, elements[1], elements[5], elements[9]) = resultN2->values();
  auto resultN3 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN3)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  std::tie(n3, elements[2], elements[6], elements[10]) = resultN3->values();
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  Transform->SetMatrix(elements);
  Transform->Inverse();

  this->ReadMolecule(fp, output);

  if (orbitalCubeFile)
  {
    auto resultOrbital = vtk::scan_value<int>(fp);
    if (!resultOrbital)
    {
      vtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading number of orbitals.");
      fclose(fp);
      return 0;
    }
    numberOfOrbitals = resultOrbital->value();
    for (k = 0; k < numberOfOrbitals; k++)
    {
      if (!vtk::scan_value<float>(fp))
      {
        vtkErrorMacro("GaussianCubeReader error reading file: "
          << this->FileName << " Premature EOF while reading orbitals.");
        fclose(fp);
        return 0;
      }
    }
  }

  vtkInformation* gridInfo = this->GetExecutive()->GetOutputInformation(1);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, n1 - 1, 0, n2 - 1, 0, n3 - 1);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  grid->SetExtent(gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  grid->SetOrigin(0, 0, 0);
  grid->SetSpacing(1, 1, 1);
  grid->AllocateScalars(VTK_FLOAT, 1);

  grid->GetPointData()->GetScalars()->SetName(title);

  auto cubedata = vtkFloatArray::SafeDownCast(grid->GetPointData()->GetScalars());
  N1N2 = n1 * n2;

  for (i = 0; i < n1; i++)
  {
    JN1 = 0;
    for (j = 0; j < n2; j++)
    {
      for (k = 0; k < n3; k++)
      {
        auto resultCubeData = vtk::scan_value<float>(fp);
        if (!resultCubeData)
        {
          vtkErrorMacro("GaussianCubeReader error reading file: "
            << this->FileName << " Premature EOF while reading scalars.");
          fclose(fp);
          return 0;
        }
        cubedata->SetValue(k * N1N2 + JN1 + i, resultCubeData->value());
      }
      JN1 += n1;
    }
  }
  fclose(fp);

  return 1;
}

//------------------------------------------------------------------------------
void vtkGaussianCubeReader::ReadSpecificMolecule(FILE* fp)
{
  int j;
  float x[3];

  for (int i = 0; i < this->NumberOfAtoms; i++)
  {
    auto resultAtom = vtk::scan<int, float, float, float, float>(fp, "{:d} {:f} {:f} {:f} {:f}");
    if (!resultAtom)
    {
      vtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading molecule.");
      fclose(fp);
      return;
    }
    std::tie(j, std::ignore, x[0], x[1], x[2]) = resultAtom->values();
    this->Transform->TransformPoint(x, x);
    this->Points->InsertNextPoint(x);
    this->AtomType->InsertNextValue(j - 1);
    this->AtomTypeStrings->InsertNextValue("Xx");
    this->Residue->InsertNextValue(-1);
    this->Chain->InsertNextValue(0);
    this->SecondaryStructures->InsertNextValue(0);
    this->SecondaryStructuresBegin->InsertNextValue(0);
    this->SecondaryStructuresEnd->InsertNextValue(0);
    this->IsHetatm->InsertNextValue(0);
  }

  // We only have one submodel
  this->Model->SetNumberOfValues(this->NumberOfAtoms);
  for (int i = 0; i < this->NumberOfAtoms; ++i)
  {
    this->Model->SetValue(i, 1);
  }
}

//------------------------------------------------------------------------------
vtkImageData* vtkGaussianCubeReader::GetGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
void vtkGaussianCubeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Filename: " << (this->FileName ? this->FileName : "(none)") << "\n";

  os << "Transform: ";
  if (this->Transform)
  {
    os << endl;
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}

//------------------------------------------------------------------------------
// Default implementation - copy information from first input to all outputs
int vtkGaussianCubeReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  // the set the information for the imagedat output
  vtkInformation* gridInfo = this->GetExecutive()->GetOutputInformation(1);

  FILE* fp;
  char title[256];

  if (!this->FileName)
  {
    return 0;
  }

  if ((fp = vtksys::SystemTools::Fopen(this->FileName, "r")) == nullptr)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  if (!fgets(title, 256, fp))
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }
  if (!fgets(title, 256, fp))
  {
    vtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // Read in number of atoms, x-origin, y-origin z-origin
  auto resultN1 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN1)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }

  resultN1 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN1)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }
  int n1 = std::get<0>(resultN1->values());
  auto resultN2 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN2)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }
  int n2 = std::get<0>(resultN2->values());
  auto resultN3 = vtk::scan<int, double, double, double>(fp, "{:d} {:f} {:f} {:f}");
  if (!resultN3)
  {
    vtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }
  int n3 = std::get<0>(resultN3->values());

  vtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, n1 - 1, 0, n2 - 1, 0, n3 - 1);
  gridInfo->Set(vtkDataObject::ORIGIN(), 0, 0, 0);
  gridInfo->Set(vtkDataObject::SPACING(), 1, 1, 1);

  fclose(fp);

  vtkDataObject::SetPointDataActiveScalarInfo(gridInfo, VTK_FLOAT, -1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkGaussianCubeReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
VTK_ABI_NAMESPACE_END
