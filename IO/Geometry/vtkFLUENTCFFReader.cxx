/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFLUENTCFFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This file reads the Fluent Common Fluid Format. It uses the HDF5 library
// Original author : Arthur Piquet
//
// This class is based on the vtkFLUENTReader class from Brian W. Dotson &
// Terry E. Jordan (Department of Energy, National Energy Technology
// Laboratory) & Douglas McCorkle (Iowa State University)
//
// This class could be improved for memory performance but the developper
// will need to rewrite entirely the structure of the class.

#include "vtkFLUENTCFFReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvexPointSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkEndian.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHexahedron.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWedge.h"
#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFLUENTCFFReader);

//------------------------------------------------------------------------------
vtkFLUENTCFFReader::vtkFLUENTCFFReader()
{
  this->status = H5open();
  if (this->status < 0)
  {
    vtkErrorMacro("HDF5 library initialisation error");
  }
  this->status = H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkFLUENTCFFReader::~vtkFLUENTCFFReader()
{
  status = H5close();
  if (status < 0)
  {
    vtkWarningMacro("HDF5 library closing error");
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (this->FluentCaseFile < 0)
  {
    vtkErrorMacro("HDF5 file not opened!");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

  // Read data (Fluent Format)
  this->ParseCaseFile();
  this->CleanCells();
  this->PopulateCellNodes();
  this->GetNumberOfCellZones();
  this->NumberOfScalars = 0;
  this->NumberOfVectors = 0;
  if (this->DataPass == 1)
  {
    this->GetData();
    this->PopulateCellTree();
  }
  // Convert Fluent format to VTK
  for (size_t i = 0; i < this->ScalarDataChunks.size(); i++)
  {
    this->CellDataArraySelection->AddArray(this->ScalarDataChunks[i].variableName.c_str());
  }
  for (size_t i = 0; i < this->VectorDataChunks.size(); i++)
  {
    this->CellDataArraySelection->AddArray(this->VectorDataChunks[i].variableName.c_str());
  }
  this->NumberOfCells = static_cast<vtkIdType>(this->Cells.size());

  output->SetNumberOfBlocks(static_cast<unsigned int>(this->CellZones.size()));

  std::vector<vtkUnstructuredGrid*> grid;
  grid.resize(this->CellZones.size());

  for (size_t test = 0; test < this->CellZones.size(); test++)
  {
    grid[test] = vtkUnstructuredGrid::New();
  }

  for (size_t i = 0; i < this->Cells.size(); i++)
  {
    size_t location =
      std::find(this->CellZones.begin(), this->CellZones.end(), this->Cells[i].zone) -
      this->CellZones.begin();

    if (this->Cells[i].type == 1)
    {
      for (int j = 0; j < 3; j++)
      {
        this->Triangle->GetPointIds()->SetId(j, this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Triangle->GetCellType(), this->Triangle->GetPointIds());
    }
    else if (this->Cells[i].type == 2)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Tetra->GetPointIds()->SetId(j, Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Tetra->GetCellType(), this->Tetra->GetPointIds());
    }
    else if (this->Cells[i].type == 3)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Quad->GetPointIds()->SetId(j, this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Quad->GetCellType(), this->Quad->GetPointIds());
    }
    else if (this->Cells[i].type == 4)
    {
      for (int j = 0; j < 8; j++)
      {
        this->Hexahedron->GetPointIds()->SetId(j, this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->Hexahedron->GetCellType(), this->Hexahedron->GetPointIds());
    }
    else if (this->Cells[i].type == 5)
    {
      for (int j = 0; j < 5; j++)
      {
        this->Pyramid->GetPointIds()->SetId(j, this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Pyramid->GetCellType(), this->Pyramid->GetPointIds());
    }
    else if (this->Cells[i].type == 6)
    {
      for (int j = 0; j < 6; j++)
      {
        this->Wedge->GetPointIds()->SetId(j, this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Wedge->GetCellType(), this->Wedge->GetPointIds());
    }
    else if (this->Cells[i].type == 7)
    {
      this->ConvexPointSet->GetPointIds()->SetNumberOfIds(
        static_cast<vtkIdType>(this->Cells[i].nodes.size()));
      for (size_t j = 0; j < this->Cells[i].nodes.size(); j++)
      {
        this->ConvexPointSet->GetPointIds()->SetId(
          static_cast<vtkIdType>(j), this->Cells[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->ConvexPointSet->GetCellType(), this->ConvexPointSet->GetPointIds());
    }
  }

  // Scalar Data
  for (size_t l = 0; l < this->ScalarDataChunks.size(); l++)
  {
    if (this->CellDataArraySelection->ArrayIsEnabled(
          this->ScalarDataChunks[l].variableName.c_str()))
    {
      for (size_t location = 0; location < this->CellZones.size(); location++)
      {
        vtkNew<vtkDoubleArray> v;
        unsigned int i = 0;
        for (size_t m = 0; m < this->ScalarDataChunks[l].scalarData.size(); m++)
        {
          if (this->Cells[m].zone == this->CellZones[location])
          {
            v->InsertValue(static_cast<vtkIdType>(i), this->ScalarDataChunks[l].scalarData[m]);
            i++;
          }
        }
        v->SetName(this->ScalarDataChunks[l].variableName.c_str());
        grid[location]->GetCellData()->AddArray(v);
      }
    }
  }
  this->ScalarDataChunks.clear();

  // Vector Data
  for (size_t l = 0; l < this->VectorDataChunks.size(); l++)
  {
    if (this->CellDataArraySelection->ArrayIsEnabled(
          this->VectorDataChunks[l].variableName.c_str()))
    {
      for (size_t location = 0; location < this->CellZones.size(); location++)
      {
        vtkNew<vtkDoubleArray> v;
        unsigned int i = 0;
        v->SetNumberOfComponents(3);
        for (size_t m = 0; m < this->VectorDataChunks[l].iComponentData.size(); m++)
        {
          if (this->Cells[m].zone == this->CellZones[location])
          {
            v->InsertComponent(
              static_cast<vtkIdType>(i), 0, this->VectorDataChunks[l].iComponentData[m]);
            v->InsertComponent(
              static_cast<vtkIdType>(i), 1, this->VectorDataChunks[l].jComponentData[m]);
            v->InsertComponent(
              static_cast<vtkIdType>(i), 2, this->VectorDataChunks[l].kComponentData[m]);
            i++;
          }
        }
        v->SetName(this->VectorDataChunks[l].variableName.c_str());
        grid[location]->GetCellData()->AddArray(v);
      }
    }
  }
  this->VectorDataChunks.clear();

  for (size_t addTo = 0; addTo < this->CellZones.size(); addTo++)
  {
    grid[addTo]->SetPoints(Points);
    output->SetBlock(static_cast<unsigned int>(addTo), grid[addTo]);
    grid[addTo]->Delete();
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << this->FileName << endl;
  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number Of Cell Zone: " << this->CellZones.size() << endl;
  if (this->DataPass == 1)
  {
    os << indent << "List Of Scalar Value : " << this->ScalarDataChunks.size() << endl;
    if (!this->ScalarDataChunks.empty())
    {
      os << indent;
      for (size_t i = 0; i < this->ScalarDataChunks.size(); i++)
      {
        os << this->ScalarDataChunks[i].variableName;
      }
      os << endl;
    }
    os << indent << "List Of Vector Value : " << this->VectorDataChunks.size() << endl;
    if (!this->VectorDataChunks.empty())
    {
      os << indent;
      for (size_t i = 0; i < this->VectorDataChunks.size(); i++)
      {
        os << this->VectorDataChunks[i].variableName;
      }
      os << endl;
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (!this->OpenCaseFile(this->FileName))
  {
    vtkErrorMacro("Unable to open cas file.");
    return 0;
  }

  this->DataPass = this->OpenDataFile(this->FileName);
  if (this->DataPass == 0)
  {
    vtkWarningMacro("Unable to open dat file.");
  }

  this->GridDimension = this->GetDimension();
  vtkDebugMacro(<< "\nDimension of file " << this->GridDimension);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkFLUENTCFFReader::OpenCaseFile(const std::string filename)
{
  // Check if the file is HDF5 or exist
  htri_t file_type = H5Fis_hdf5(filename.c_str());
  if (file_type != 1)
  {
    vtkErrorMacro("The file " << filename << " does not exist or is not a HDF5 file.");
    return false;
  }
  // Open file with default properties access
  this->FluentCaseFile = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  // Check if file is CFF Format like
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/meshes", false, nullptr);
  herr_t s2 = H5Gget_objinfo(this->FluentCaseFile, "/settings", false, nullptr);
  if (s1 == 0 && s2 == 0)
  {
    return true;
  }
  else
  {
    vtkErrorMacro("The file " << filename << " is not a CFF Fluent file.");
    return false;
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkFLUENTCFFReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::SetCellArrayStatus(const char* name, int stat)
{
  if (stat)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//------------------------------------------------------------------------------
bool vtkFLUENTCFFReader::OpenDataFile(const std::string filename)
{
  // dfilename represent the dat file name (extension .dat.h5)
  // when opening a .cas.h5, it will automatically open the associated .dat.h5 (if exist)
  // filename.cas.h5 -> filename.dat.h5
  std::string dfilename = filename;
  dfilename.erase(dfilename.length() - 6, 6);
  dfilename.append("dat.h5");

  // Check if the file is HDF5 or exist
  htri_t file_type = H5Fis_hdf5(dfilename.c_str());
  if (file_type != 1)
  {
    vtkWarningMacro("Could not open data file "
      << dfilename << "associated with cas file " << filename
      << ". Please verify the cas and dat files have the same base name.");
    return false;
  }
  // Open file with default properties access
  this->FluentDataFile = H5Fopen(dfilename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  return true;
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetNumberOfCellZones()
{
  for (size_t i = 0; i < this->Cells.size(); i++)
  {
    if (this->CellZones.empty())
    {
      this->CellZones.push_back(this->Cells[i].zone);
    }
    else
    {
      int match = 0;
      for (size_t j = 0; j < this->CellZones.size(); j++)
      {
        if (this->CellZones[j] == this->Cells[i].zone)
        {
          match = 1;
        }
      }
      if (match == 0)
      {
        this->CellZones.push_back(this->Cells[i].zone);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::ParseCaseFile()
{
  this->GetNodesGlobal();
  this->GetCellsGlobal();
  this->GetFacesGlobal();
  // .cas is always DP
  // .dat is DP or SP
  this->GetNodes();
  this->GetCells();
  this->GetFaces();

  this->GetCellTree();
  this->GetCellOverset();
  this->GetFaceTree();
  this->GetInterfaceFaceParents();
  this->GetNonconformalGridInterfaceFaceInformation();
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::GetDimension()
{
  hid_t group, attr;
  int32_t dimension;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1", H5P_DEFAULT);
  attr = H5Aopen(group, "dimension", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_INT32, &dimension);
  status = H5Aclose(attr);
  status = H5Gclose(group);
  return static_cast<int>(dimension);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetNodesGlobal()
{
  hid_t group, attr;
  uint64_t firstIndex, lastIndex;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1", H5P_DEFAULT);
  attr = H5Aopen(group, "nodeOffset", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &firstIndex);
  status = H5Aclose(attr);
  attr = H5Aopen(group, "nodeCount", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &lastIndex);
  status = H5Aclose(attr);
  status = H5Gclose(group);
  this->Points->Allocate(lastIndex);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetNodes()
{
  hid_t group, attr, dset;
  uint64_t nZones;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/nodes/zoneTopology", H5P_DEFAULT);
  attr = H5Aopen(group, "nZones", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nZones);
  status = H5Aclose(attr);

  std::vector<uint64_t> minId(nZones);
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> maxId(nZones);
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId.data());
  status = H5Dclose(dset);

  std::vector<int32_t> Id(nZones);
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> dimension(nZones);
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension.data());
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    uint64_t coords_minId, coords_maxId;
    hid_t group_coords, dset_coords;
    group_coords = H5Gopen(this->FluentCaseFile, "/meshes/1/nodes/coords", H5P_DEFAULT);
    dset_coords = H5Dopen(group_coords, std::to_string(iZone + 1).c_str(), H5P_DEFAULT);

    attr = H5Aopen(dset_coords, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &coords_minId);
    status = H5Aclose(attr);
    attr = H5Aopen(dset_coords, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &coords_maxId);
    status = H5Aclose(attr);

    unsigned int firstIndex = static_cast<unsigned int>(coords_minId);
    unsigned int lastIndex = static_cast<unsigned int>(coords_maxId);

    uint64_t size = lastIndex - firstIndex + 1;
    uint64_t gSize;
    if (this->GridDimension == 3)
    {
      gSize = size * 3;
    }
    else
    {
      gSize = size * 2;
    }

    std::vector<double> nodeData(gSize);
    status =
      H5Dread(dset_coords, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, nodeData.data());
    status = H5Dclose(dset_coords);
    status = H5Gclose(group_coords);

    if (this->GridDimension == 3)
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Points->InsertPoint(i - 1, nodeData[(i - firstIndex) * 3 + 0],
          nodeData[(i - firstIndex) * 3 + 1], nodeData[(i - firstIndex) * 3 + 2]);
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Points->InsertPoint(
          i - 1, nodeData[(i - firstIndex) * 2 + 0], nodeData[(i - firstIndex) * 2 + 1], 0.0);
      }
    }
  }

  status = H5Gclose(group);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetCellsGlobal()
{
  hid_t group, attr;
  uint64_t firstIndex, lastIndex;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1", H5P_DEFAULT);
  attr = H5Aopen(group, "cellOffset", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &firstIndex);
  status = H5Aclose(attr);
  attr = H5Aopen(group, "cellCount", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &lastIndex);
  status = H5Aclose(attr);
  status = H5Gclose(group);
  this->Cells.resize(lastIndex);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetCells()
{
  hid_t group, attr, dset;
  uint64_t nZones;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/cells/zoneTopology", H5P_DEFAULT);
  attr = H5Aopen(group, "nZones", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nZones);
  status = H5Aclose(attr);

  std::vector<uint64_t> minId(nZones);
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> maxId(nZones);
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId.data());
  status = H5Dclose(dset);

  std::vector<int32_t> Id(nZones);
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> dimension(nZones);
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension.data());
  status = H5Dclose(dset);

  std::vector<int32_t> cellType(nZones);
  dset = H5Dopen(group, "cellType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, cellType.data());
  status = H5Dclose(dset);

  std::vector<int32_t> childZoneId(nZones);
  dset = H5Dopen(group, "childZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, childZoneId.data());
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    unsigned int elementType = static_cast<unsigned int>(cellType[iZone]);
    unsigned int zoneId = static_cast<unsigned int>(Id[iZone]);
    unsigned int firstIndex = static_cast<unsigned int>(minId[iZone]);
    unsigned int lastIndex = static_cast<unsigned int>(maxId[iZone]);
    // This next line should be uncommented following test with Fluent file
    // containing tree format (AMR)
    //// unsigned int child = static_cast<unsigned int>(childZoneId[iZone]);
    // next child and parent variable should be initialized correctly

    if (elementType == 0)
    {
      std::vector<int16_t> cellTypeData;
      hid_t group_ctype;
      uint64_t nSections;
      group_ctype = H5Gopen(this->FluentCaseFile, "/meshes/1/cells/ctype", H5P_DEFAULT);
      attr = H5Aopen(group_ctype, "nSections", H5P_DEFAULT);
      status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
      status = H5Aclose(attr);
      status = H5Gclose(group_ctype);

      // Search for ctype section linked to the mixed zone
      uint64_t ctype_minId = 0, ctype_maxId = 0;
      for (uint64_t iSection = 0; iSection < nSections; iSection++)
      {
        int16_t ctype_elementType;
        std::string groupname =
          std::string("/meshes/1/cells/ctype/" + std::to_string(iSection + 1));
        group_ctype = H5Gopen(this->FluentCaseFile, groupname.c_str(), H5P_DEFAULT);

        attr = H5Aopen(group_ctype, "elementType", H5P_DEFAULT);
        status = H5Aread(attr, H5T_NATIVE_INT16, &ctype_elementType);
        status = H5Aclose(attr);
        attr = H5Aopen(group_ctype, "minId", H5P_DEFAULT);
        status = H5Aread(attr, H5T_NATIVE_UINT64, &ctype_minId);
        status = H5Aclose(attr);
        attr = H5Aopen(group_ctype, "maxId", H5P_DEFAULT);
        status = H5Aread(attr, H5T_NATIVE_UINT64, &ctype_maxId);
        status = H5Aclose(attr);

        if (static_cast<unsigned int>(ctype_elementType) == elementType &&
          static_cast<unsigned int>(ctype_minId) <= firstIndex &&
          static_cast<unsigned int>(ctype_maxId) >= lastIndex)
        {
          cellTypeData.resize(ctype_maxId - ctype_minId + 1);
          dset = H5Dopen(group_ctype, "cell-types", H5P_DEFAULT);
          status =
            H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, cellTypeData.data());
          status = H5Dclose(dset);
          status = H5Gclose(group_ctype);
          break;
        }
        status = H5Gclose(group_ctype);
      }

      if (!cellTypeData.empty())
      {
        for (unsigned int i = firstIndex; i <= lastIndex; i++)
        {
          this->Cells[i - 1].type = static_cast<unsigned int>(cellTypeData[i - ctype_minId]);
          this->Cells[i - 1].zone = zoneId;
          this->Cells[i - 1].parent = 0;
          this->Cells[i - 1].child = 0;
        }
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Cells[i - 1].type = elementType;
        this->Cells[i - 1].zone = zoneId;
        this->Cells[i - 1].parent = 0;
        this->Cells[i - 1].child = 0;
      }
    }
  }

  status = H5Gclose(group);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetFacesGlobal()
{
  hid_t group, attr;
  uint64_t firstIndex, lastIndex;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1", H5P_DEFAULT);
  attr = H5Aopen(group, "faceOffset", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &firstIndex);
  status = H5Aclose(attr);
  attr = H5Aopen(group, "faceCount", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &lastIndex);
  status = H5Aclose(attr);
  status = H5Gclose(group);
  this->Faces.resize(lastIndex);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetFaces()
{
  hid_t group, attr, dset;
  uint64_t nZones;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/zoneTopology", H5P_DEFAULT);
  attr = H5Aopen(group, "nZones", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nZones);
  status = H5Aclose(attr);

  std::vector<uint64_t> minId(nZones);
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> maxId(nZones);
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId.data());
  status = H5Dclose(dset);

  std::vector<int32_t> Id(nZones);
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id.data());
  status = H5Dclose(dset);

  std::vector<uint64_t> dimension(nZones);
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension.data());
  status = H5Dclose(dset);

  std::vector<int32_t> zoneT(nZones);
  dset = H5Dopen(group, "zoneType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, zoneT.data());
  status = H5Dclose(dset);

  std::vector<int32_t> faceT(nZones);
  dset = H5Dopen(group, "faceType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, faceT.data());
  status = H5Dclose(dset);

  std::vector<int32_t> childZoneId(nZones);
  dset = H5Dopen(group, "childZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, childZoneId.data());
  status = H5Dclose(dset);

  std::vector<int32_t> shadowZoneId(nZones);
  dset = H5Dopen(group, "shadowZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, shadowZoneId.data());
  status = H5Dclose(dset);

  std::vector<int32_t> flags(nZones);
  dset = H5Dopen(group, "flags", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, flags.data());
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    unsigned int zoneId = static_cast<unsigned int>(Id[iZone]);
    unsigned int firstIndex = static_cast<unsigned int>(minId[iZone]);
    unsigned int lastIndex = static_cast<unsigned int>(maxId[iZone]);
    // This next lines should be uncommented following test with Fluent file
    // containing tree format (AMR) and interface faces
    //// unsigned int child = static_cast<unsigned int>(childZoneId[iZone]);
    //// unsigned int shadow = static_cast<unsigned int>(shadowZoneId[iZone]);
    // next child, parent, periodicShadow variable should be initialized correctly

    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Faces[i - 1].zone = zoneId;
      this->Faces[i - 1].periodicShadow = 0;
      this->Faces[i - 1].parent = 0;
      this->Faces[i - 1].child = 0;
      this->Faces[i - 1].interfaceFaceParent = 0;
      this->Faces[i - 1].ncgParent = 0;
      this->Faces[i - 1].ncgChild = 0;
      this->Faces[i - 1].interfaceFaceChild = 0;
    }
  }

  status = H5Gclose(group);

  // FaceType
  uint64_t nSections;
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/nodes", H5P_DEFAULT);
  attr = H5Aopen(group, "nSections", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
  status = H5Aclose(attr);
  status = H5Gclose(group);

  for (uint64_t iSection = 0; iSection < nSections; iSection++)
  {
    uint64_t minId_fnodes, maxId_fnodes, nodes_size;
    std::string groupname = std::string("/meshes/1/faces/nodes/" + std::to_string(iSection + 1));
    group = H5Gopen(this->FluentCaseFile, groupname.c_str(), H5P_DEFAULT);

    attr = H5Aopen(group, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minId_fnodes);
    status = H5Aclose(attr);
    attr = H5Aopen(group, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId_fnodes);
    status = H5Aclose(attr);

    std::vector<int16_t> nnodes_fnodes(maxId_fnodes - minId_fnodes + 1);
    dset = H5Dopen(group, "nnodes", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nnodes_fnodes.data());
    status = H5Dclose(dset);

    dset = H5Dopen(group, "nodes", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &nodes_size);
    status = H5Aclose(attr);

    std::vector<uint32_t> nodes_fnodes(nodes_size);
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, nodes_fnodes.data());
    status = H5Dclose(dset);

    int numberOfNodesInFace = 0;
    uint64_t ptr = minId_fnodes;
    for (unsigned int i = static_cast<unsigned int>(minId_fnodes);
         i <= static_cast<unsigned int>(maxId_fnodes); i++)
    {
      numberOfNodesInFace = static_cast<int>(nnodes_fnodes[i - minId_fnodes]);

      this->Faces[i - 1].nodes.resize(numberOfNodesInFace);
      this->Faces[i - 1].type = numberOfNodesInFace;

      for (int k = 0; k < numberOfNodesInFace; k++)
      {
        this->Faces[i - 1].nodes[k] = static_cast<int>(nodes_fnodes[ptr - 1]) - 1;
        ptr++;
      }
    }
    status = H5Gclose(group);
  }

  // C0 C1
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/c0", H5P_DEFAULT);
  attr = H5Aopen(group, "nSections", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
  status = H5Aclose(attr);
  for (uint64_t iSection = 0; iSection < nSections; iSection++)
  {
    uint64_t minc0, maxc0;

    dset = H5Dopen(group, std::to_string(iSection + 1).c_str(), H5P_DEFAULT);

    attr = H5Aopen(dset, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minc0);
    status = H5Aclose(attr);
    attr = H5Aopen(dset, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxc0);
    status = H5Aclose(attr);

    std::vector<uint32_t> c0(maxc0 - minc0 + 1);
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, c0.data());
    status = H5Dclose(dset);

    for (unsigned int i = static_cast<unsigned int>(minc0); i <= static_cast<unsigned int>(maxc0);
         i++)
    {
      this->Faces[i - 1].c0 = static_cast<int>(c0[i - minc0]) - 1;
      if (this->Faces[i - 1].c0 >= 0)
      {
        this->Cells[this->Faces[i - 1].c0].faces.push_back(i - 1);
      }
    }
  }
  status = H5Gclose(group);

  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/c1", H5P_DEFAULT);
  attr = H5Aopen(group, "nSections", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
  status = H5Aclose(attr);
  for (size_t i = 0; i < this->Faces.size(); i++)
  {
    this->Faces[i].c1 = -1;
  }
  for (uint64_t iSection = 0; iSection < nSections; iSection++)
  {
    uint64_t minc1, maxc1;

    dset = H5Dopen(group, std::to_string(iSection + 1).c_str(), H5P_DEFAULT);

    attr = H5Aopen(dset, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minc1);
    status = H5Aclose(attr);
    attr = H5Aopen(dset, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxc1);
    status = H5Aclose(attr);

    std::vector<uint32_t> c1(maxc1 - minc1 + 1);
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, c1.data());
    status = H5Dclose(dset);

    for (unsigned int i = static_cast<unsigned int>(minc1); i <= static_cast<unsigned int>(maxc1);
         i++)
    {
      this->Faces[i - 1].c1 = static_cast<int>(c1[i - minc1]) - 1;
      if (this->Faces[i - 1].c1 >= 0)
      {
        this->Cells[this->Faces[i - 1].c1].faces.push_back(i - 1);
      }
    }
  }

  status = H5Gclose(group);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetPeriodicShadowFaces()
{
  // TODO: Periodic shadow faces read should be added following test with Fluent file
  // containing periodic faces
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetCellOverset()
{
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/special/Overset_DCI", false, nullptr);
  if (s1 == 0)
  {
    vtkWarningMacro("The overset layout of this CFF file cannot be displayed by this reader.");
    // TODO: Overset cells read should be added following test with Fluent file
    // containing overset cell zone
    // This function can read the overset structure but Ansys Fluent does not
    // give any explanation about the structure of the overset data.
    /*herr_t status;
    hid_t group, attr, dset;
    uint64_t nSections;
    group = H5Gopen(this->FluentCaseFile, "/special/Overset_DCI/cells", H5P_DEFAULT);

    dset = H5Dopen(group, "topology", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
    status = H5Aclose(attr);

    std::vector<int32_t> topology(nSections);
    status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, topology.data());
    status = H5Dclose(dset);

    for (int iSection = 0; iSection < nSections; iSection++)
    {
      hid_t groupTopo = H5Gopen(group, std::to_string(topology[iSection]).c_str(), H5P_DEFAULT);

      uint64_t minId, maxId;
      attr = H5Aopen(groupTopo, "minId", H5P_DEFAULT);
      status = H5Aread(attr, H5T_NATIVE_UINT64, &minId);
      status = H5Aclose(attr);
      attr = H5Aopen(groupTopo, "maxId", H5P_DEFAULT);
      status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId);
      status = H5Aclose(attr);

      std::vector<int32_t> ndata(maxId - minId + 1);
      dset = H5Dopen(groupTopo, "ndata", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, ndata.data());
      status = H5Dclose(dset);

      for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
i++)
      {
        if (ndata[i - minId] != 4)
          this->Cells[i - 1].overset = 1;
      }

      dset = H5Dopen(groupTopo, "data", H5P_DEFAULT);
      uint64_t size_data;
      attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
      status = H5Aread(attr, H5T_NATIVE_UINT64, &size_data);
      status = H5Aclose(attr);
      std::vector<int8_t> data(size_data);
      status = H5Dread(dset, H5T_NATIVE_INT8, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
      status = H5Dclose(dset);

      status = H5Dclose(dset);
      status = H5Gclose(groupTopo);
    }

    status = H5Gclose(group);*/
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetCellTree()
{
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/meshes/1/cells/tree", false, nullptr);
  if (s1 == 0)
  {
    hid_t group, attr, dset;
    uint64_t minId, maxId;
    group = H5Gopen(this->FluentCaseFile, "/meshes/1/cells/tree/1", H5P_DEFAULT);
    attr = H5Aopen(group, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minId);
    status = H5Aclose(attr);
    attr = H5Aopen(group, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId);
    status = H5Aclose(attr);

    std::vector<int16_t> nkids(maxId - minId + 1);
    dset = H5Dopen(group, "nkids", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nkids.data());
    status = H5Dclose(dset);

    uint64_t kids_size;
    dset = H5Dopen(group, "kids", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &kids_size);
    status = H5Aclose(attr);

    std::vector<uint32_t> kids(kids_size);
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, kids.data());
    status = H5Dclose(dset);

    uint64_t ptr = 0;
    for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
         i++)
    {
      this->Cells[i - 1].parent = 1;
      int numberOfKids = static_cast<int>(nkids[i - minId]);
      this->Cells[i - 1].childId.resize(numberOfKids);
      for (int j = 0; j < numberOfKids; j++)
      {
        this->Cells[kids[ptr] - 1].child = 1;
        this->Cells[i - 1].childId[j] = kids[ptr] - 1;
        ptr++;
      }
    }

    status = H5Gclose(group);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetFaceTree()
{
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/meshes/1/faces/tree", false, nullptr);
  if (s1 == 0)
  {
    hid_t group, attr, dset;
    uint64_t minId, maxId;
    group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/tree/1", H5P_DEFAULT);
    attr = H5Aopen(group, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minId);
    status = H5Aclose(attr);
    attr = H5Aopen(group, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId);
    status = H5Aclose(attr);

    std::vector<int16_t> nkids(maxId - minId + 1);
    dset = H5Dopen(group, "nkids", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nkids.data());
    status = H5Dclose(dset);

    uint64_t kids_size;
    dset = H5Dopen(group, "kids", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &kids_size);
    status = H5Aclose(attr);

    std::vector<uint32_t> kids(kids_size);
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, kids.data());
    status = H5Dclose(dset);

    uint64_t ptr = 0;
    for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
         i++)
    {
      this->Faces[i - 1].parent = 1;
      int numberOfKids = static_cast<int>(nkids[i - minId]);
      for (int j = 0; j < numberOfKids; j++)
      {
        this->Faces[kids[ptr] - 1].child = 1;
        ptr++;
      }
    }

    status = H5Gclose(group);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetInterfaceFaceParents()
{
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/meshes/1/faces/interface", false, nullptr);
  if (s1 == 0)
  {
    hid_t group, attr, dset;
    uint64_t nData, nZones;
    group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/interface", H5P_DEFAULT);
    attr = H5Aopen(group, "nData", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &nData);
    status = H5Aclose(attr);
    attr = H5Aopen(group, "nZones", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &nZones);
    status = H5Aclose(attr);

    std::vector<uint64_t> nciTopology(nData * nZones);
    dset = H5Dopen(group, "nciTopology", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, nciTopology.data());
    status = H5Dclose(dset);

    for (uint64_t iZone = 0; iZone < nZones; iZone++)
    {
      int zoneId = static_cast<int>(nciTopology[iZone * nData]);
      int minId = static_cast<int>(nciTopology[iZone * nData + 1]);
      int maxId = static_cast<int>(nciTopology[iZone * nData + 2]);

      hid_t group_int = H5Gopen(group, std::to_string(zoneId).c_str(), H5P_DEFAULT);

      std::vector<uint64_t> pf0(maxId - minId + 1);
      std::vector<uint64_t> pf1(maxId - minId + 1);
      dset = H5Dopen(group_int, "pf0", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, pf0.data());
      status = H5Dclose(dset);
      dset = H5Dopen(group_int, "pf1", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, pf1.data());
      status = H5Dclose(dset);

      for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
           i++)
      {
        unsigned int parentId0 = static_cast<unsigned int>(pf0[i - minId]);
        unsigned int parentId1 = static_cast<unsigned int>(pf1[i - minId]);

        this->Faces[parentId0 - 1].interfaceFaceParent = 1;
        this->Faces[parentId1 - 1].interfaceFaceParent = 1;
        this->Faces[i - 1].interfaceFaceChild = 1;
      }
      status = H5Gclose(group_int);
    }

    status = H5Gclose(group);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetNonconformalGridInterfaceFaceInformation()
{
  // TODO: Non conformal faces read should be added following test with Fluent file
  // containing interface faces
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::CleanCells()
{

  std::vector<int> t;
  for (size_t i = 0; i < Cells.size(); i++)
  {

    if (((this->Cells[i].type == 1) && (this->Cells[i].faces.size() != 3)) ||
      ((this->Cells[i].type == 2) && (this->Cells[i].faces.size() != 4)) ||
      ((this->Cells[i].type == 3) && (this->Cells[i].faces.size() != 4)) ||
      ((this->Cells[i].type == 4) && (this->Cells[i].faces.size() != 6)) ||
      ((this->Cells[i].type == 5) && (this->Cells[i].faces.size() != 5)) ||
      ((this->Cells[i].type == 6) && (this->Cells[i].faces.size() != 5)))
    {

      // Copy faces
      t.clear();
      for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
      {
        t.push_back(this->Cells[i].faces[j]);
      }

      // Clear Faces
      this->Cells[i].faces.clear();

      // Copy the faces that are not flagged back into the cell
      for (size_t j = 0; j < t.size(); j++)
      {
        if ((this->Faces[t[j]].child == 0) && (this->Faces[t[j]].ncgChild == 0) &&
          (this->Faces[t[j]].interfaceFaceChild == 0))
        {
          this->Cells[i].faces.push_back(t[j]);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateCellTree()
{
  for (size_t i = 0; i < this->Cells.size(); i++)
  {
    // If cell is parent cell -> interpolate data from children
    if (this->Cells[i].parent == 1)
    {
      for (size_t k = 0; k < this->ScalarDataChunks.size(); k++)
      {
        double data = 0.0;
        int ncell = 0;
        for (size_t j = 0; j < this->Cells[i].childId.size(); j++)
        {
          if (this->Cells[this->Cells[i].childId[j]].parent == 0)
          {
            data += this->ScalarDataChunks[k].scalarData[this->Cells[i].childId[j]];
            ncell++;
          }
        }
        if (ncell == 0)
          this->ScalarDataChunks[k].scalarData.push_back(0.0);
        else
          this->ScalarDataChunks[k].scalarData.push_back(data / (double)ncell);
      }
      for (size_t k = 0; k < this->VectorDataChunks.size(); k++)
      {
        double datax = 0.0;
        double datay = 0.0;
        double dataz = 0.0;
        int ncell = 0;
        for (size_t j = 0; j < this->Cells[i].childId.size(); j++)
        {
          if (this->Cells[this->Cells[i].childId[j]].parent == 0)
          {
            datax += this->VectorDataChunks[k].iComponentData[this->Cells[i].childId[j]];
            datay += this->VectorDataChunks[k].jComponentData[this->Cells[i].childId[j]];
            dataz += this->VectorDataChunks[k].kComponentData[this->Cells[i].childId[j]];
            ncell++;
          }
        }
        if (ncell == 0)
        {
          this->VectorDataChunks[k].iComponentData.push_back(0.0);
          this->VectorDataChunks[k].jComponentData.push_back(0.0);
          this->VectorDataChunks[k].kComponentData.push_back(0.0);
        }
        else
        {
          this->VectorDataChunks[k].iComponentData.push_back(datax / (double)ncell);
          this->VectorDataChunks[k].jComponentData.push_back(datay / (double)ncell);
          this->VectorDataChunks[k].kComponentData.push_back(dataz / (double)ncell);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateCellNodes()
{
  for (size_t i = 0; i < this->Cells.size(); i++)
  {
    const vtkIdType id = static_cast<vtkIdType>(i);
    switch (this->Cells[i].type)
    {
      case 1: // Triangle
        this->PopulateTriangleCell(id);
        break;

      case 2: // Tetrahedron
        this->PopulateTetraCell(id);
        break;

      case 3: // Quadrilateral
        this->PopulateQuadCell(id);
        break;

      case 4: // Hexahedral
        this->PopulateHexahedronCell(id);
        break;

      case 5: // Pyramid
        this->PopulatePyramidCell(id);
        break;

      case 6: // Wedge
        this->PopulateWedgeCell(id);
        break;

      case 7: // Polyhedron
        this->PopulatePolyhedronCell(id);
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateTriangleCell(int i)
{
  this->Cells[i].nodes.resize(3);
  if (this->Faces[this->Cells[i].faces[0]].c0 == i)
  {
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[1];
  }

  if (this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[0] &&
    this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[1])
  {
    this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[1]].nodes[0];
  }
  else
  {
    this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[1]].nodes[1];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateTetraCell(int i)
{
  this->Cells[i].nodes.resize(4);

  if (this->Faces[this->Cells[i].faces[0]].c0 == i)
  {
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[1];
    this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[0]].nodes[2];
  }
  else
  {
    this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[1];
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[2];
  }

  if (this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[0] &&
    this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[1] &&
    this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[2])
  {
    this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[1]].nodes[0];
  }
  else if (this->Faces[this->Cells[i].faces[1]].nodes[1] != this->Cells[i].nodes[0] &&
    this->Faces[this->Cells[i].faces[1]].nodes[1] != this->Cells[i].nodes[1] &&
    this->Faces[this->Cells[i].faces[1]].nodes[1] != this->Cells[i].nodes[2])
  {
    this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[1]].nodes[1];
  }
  else
  {
    this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[1]].nodes[2];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateQuadCell(int i)
{
  this->Cells[i].nodes.resize(4);

  if (this->Faces[this->Cells[i].faces[0]].c0 == i)
  {
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells[i].nodes[1] = this->Faces[this->Cells[i].faces[0]].nodes[0];
    this->Cells[i].nodes[0] = this->Faces[this->Cells[i].faces[0]].nodes[1];
  }

  if ((this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[0] &&
        this->Faces[this->Cells[i].faces[1]].nodes[0] != this->Cells[i].nodes[1]) &&
    (this->Faces[this->Cells[i].faces[1]].nodes[1] != this->Cells[i].nodes[0] &&
      this->Faces[this->Cells[i].faces[1]].nodes[1] != this->Cells[i].nodes[1]))
  {
    if (this->Faces[this->Cells[i].faces[1]].c0 == i)
    {
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[1]].nodes[0];
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[1]].nodes[1];
    }
    else
    {
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[1]].nodes[0];
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[1]].nodes[1];
    }
  }
  else if ((this->Faces[this->Cells[i].faces[2]].nodes[0] != this->Cells[i].nodes[0] &&
             this->Faces[this->Cells[i].faces[2]].nodes[0] != this->Cells[i].nodes[1]) &&
    (this->Faces[this->Cells[i].faces[2]].nodes[1] != this->Cells[i].nodes[0] &&
      this->Faces[this->Cells[i].faces[2]].nodes[1] != this->Cells[i].nodes[1]))
  {
    if (this->Faces[this->Cells[i].faces[2]].c0 == i)
    {
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[2]].nodes[0];
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[2]].nodes[1];
    }
    else
    {
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[2]].nodes[0];
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[2]].nodes[1];
    }
  }
  else
  {
    if (this->Faces[this->Cells[i].faces[3]].c0 == i)
    {
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[3]].nodes[0];
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[3]].nodes[1];
    }
    else
    {
      this->Cells[i].nodes[3] = this->Faces[this->Cells[i].faces[3]].nodes[0];
      this->Cells[i].nodes[2] = this->Faces[this->Cells[i].faces[3]].nodes[1];
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateHexahedronCell(int i)
{
  this->Cells[i].nodes.resize(8);

  if (this->Faces[this->Cells[i].faces[0]].c0 == i)
  {
    for (int j = 0; j < 4; j++)
    {
      this->Cells[i].nodes[j] = this->Faces[this->Cells[i].faces[0]].nodes[j];
    }
  }
  else
  {
    for (int j = 3; j >= 0; j--)
    {
      this->Cells[i].nodes[3 - j] = this->Faces[this->Cells[i].faces[0]].nodes[j];
    }
  }

  //  Look for opposite face of hexahedron
  for (int j = 1; j < 6; j++)
  {
    int flag = 0;
    for (int k = 0; k < 4; k++)
    {
      if ((this->Cells[i].nodes[0] == this->Faces[this->Cells[i].faces[j]].nodes[k]) ||
        (this->Cells[i].nodes[1] == this->Faces[this->Cells[i].faces[j]].nodes[k]) ||
        (this->Cells[i].nodes[2] == this->Faces[this->Cells[i].faces[j]].nodes[k]) ||
        (this->Cells[i].nodes[3] == this->Faces[this->Cells[i].faces[j]].nodes[k]))
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      if (this->Faces[this->Cells[i].faces[j]].c1 == i)
      {
        for (int k = 4; k < 8; k++)
        {
          this->Cells[i].nodes[k] = this->Faces[this->Cells[i].faces[j]].nodes[k - 4];
        }
      }
      else
      {
        for (int k = 7; k >= 4; k--)
        {
          this->Cells[i].nodes[k] = this->Faces[this->Cells[i].faces[j]].nodes[7 - k];
        }
      }
    }
  }

  //  Find the face with points 0 and 1 in them.
  int f01[4] = { -1, -1, -1, -1 };
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells[i].nodes[0] == this->Faces[this->Cells[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells[i].nodes[1] == this->Faces[this->Cells[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }
    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces[this->Cells[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f01[k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f01[k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
    }
  }

  //  Find the face with points 0 and 3 in them.
  int f03[4] = { -1, -1, -1, -1 };
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells[i].nodes[0] == this->Faces[this->Cells[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells[i].nodes[3] == this->Faces[this->Cells[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }

    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces[this->Cells[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f03[k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f03[k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // What point is in f01 and f03 besides 0 ... this is point 4
  int p4 = 0;
  for (int k = 0; k < 4; k++)
  {
    if (f01[k] != this->Cells[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (f01[k] == f03[n])
        {
          p4 = f01[k];
        }
      }
    }
  }

  // Since we know point 4 now we check to see if points
  //  4, 5, 6, and 7 are in the correct positions.
  int t[8];
  t[4] = this->Cells[i].nodes[4];
  t[5] = this->Cells[i].nodes[5];
  t[6] = this->Cells[i].nodes[6];
  t[7] = this->Cells[i].nodes[7];
  if (p4 == this->Cells[i].nodes[5])
  {
    this->Cells[i].nodes[5] = t[6];
    this->Cells[i].nodes[6] = t[7];
    this->Cells[i].nodes[7] = t[4];
    this->Cells[i].nodes[4] = t[5];
  }
  else if (p4 == Cells[i].nodes[6])
  {
    this->Cells[i].nodes[5] = t[7];
    this->Cells[i].nodes[6] = t[4];
    this->Cells[i].nodes[7] = t[5];
    this->Cells[i].nodes[4] = t[6];
  }
  else if (p4 == Cells[i].nodes[7])
  {
    this->Cells[i].nodes[5] = t[4];
    this->Cells[i].nodes[6] = t[5];
    this->Cells[i].nodes[7] = t[6];
    this->Cells[i].nodes[4] = t[7];
  }
  // else point 4 was lined up so everything was correct.
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulatePyramidCell(int i)
{
  this->Cells[i].nodes.resize(5);
  //  The quad face will be the base of the pyramid
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if (this->Faces[this->Cells[i].faces[j]].nodes.size() == 4)
    {
      if (this->Faces[this->Cells[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells[i].nodes[k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells[i].nodes[3 - k] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // Just need to find point 4
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if (this->Faces[this->Cells[i].faces[j]].nodes.size() == 3)
    {
      for (int k = 0; k < 3; k++)
      {
        if ((this->Faces[this->Cells[i].faces[j]].nodes[k] != this->Cells[i].nodes[0]) &&
          (this->Faces[this->Cells[i].faces[j]].nodes[k] != this->Cells[i].nodes[1]) &&
          (this->Faces[this->Cells[i].faces[j]].nodes[k] != this->Cells[i].nodes[2]) &&
          (this->Faces[this->Cells[i].faces[j]].nodes[k] != this->Cells[i].nodes[3]))
        {
          this->Cells[i].nodes[4] = this->Faces[this->Cells[i].faces[j]].nodes[k];
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateWedgeCell(int i)
{
  this->Cells[i].nodes.resize(6);

  //  Find the first triangle face and make it the base.
  int base = 0;
  int first = 0;
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if ((this->Faces[this->Cells[i].faces[j]].type == 3) && (first == 0))
    {
      base = this->Cells[i].faces[j];
      first = 1;
    }
  }

  //  Find the second triangle face and make it the top.
  int top = 0;
  int second = 0;
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if ((this->Faces[this->Cells[i].faces[j]].type == 3) && (second == 0) &&
      (this->Cells[i].faces[j] != base))
    {
      top = this->Cells[i].faces[j];
      second = 1;
    }
  }

  // Load Base nodes into the nodes std::vector
  if (this->Faces[base].c0 == i)
  {
    for (int j = 0; j < 3; j++)
    {
      this->Cells[i].nodes[j] = this->Faces[base].nodes[j];
    }
  }
  else
  {
    for (int j = 2; j >= 0; j--)
    {
      this->Cells[i].nodes[2 - j] = this->Faces[base].nodes[j];
    }
  }
  // Load Top nodes into the nodes std::vector
  if (this->Faces[top].c1 == i)
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells[i].nodes[j] = this->Faces[top].nodes[j - 3];
    }
  }
  else
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells[i].nodes[j] = this->Faces[top].nodes[5 - j];
    }
  }

  //  Find the quad face with points 0 and 1 in them.
  int w01[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if (this->Cells[i].faces[j] != base && this->Cells[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf1 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells[i].nodes[0] == this->Faces[this->Cells[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells[i].nodes[1] == this->Faces[this->Cells[i].faces[j]].nodes[k])
        {
          wf1 = 1;
        }
        if ((wf0 == 1) && (wf1 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w01[n] = this->Faces[this->Cells[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  //  Find the quad face with points 0 and 2 in them.
  int w02[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    if (this->Cells[i].faces[j] != base && this->Cells[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf2 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells[i].nodes[0] == this->Faces[this->Cells[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells[i].nodes[2] == this->Faces[this->Cells[i].faces[j]].nodes[k])
        {
          wf2 = 1;
        }
        if ((wf0 == 1) && (wf2 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w02[n] = this->Faces[this->Cells[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  // Point 3 is the point that is in both w01 and w02

  // What point is in f01 and f02 besides 0 ... this is point 3
  int p3 = 0;
  for (int k = 0; k < 4; k++)
  {
    if (w01[k] != this->Cells[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (w01[k] == w02[n])
        {
          p3 = w01[k];
        }
      }
    }
  }

  // Since we know point 3 now we check to see if points
  //  3, 4, and 5 are in the correct positions.
  int t[6];
  t[3] = this->Cells[i].nodes[3];
  t[4] = this->Cells[i].nodes[4];
  t[5] = this->Cells[i].nodes[5];
  if (p3 == this->Cells[i].nodes[4])
  {
    this->Cells[i].nodes[3] = t[4];
    this->Cells[i].nodes[4] = t[5];
    this->Cells[i].nodes[5] = t[3];
  }
  else if (p3 == this->Cells[i].nodes[5])
  {
    this->Cells[i].nodes[3] = t[5];
    this->Cells[i].nodes[4] = t[3];
    this->Cells[i].nodes[5] = t[4];
  }
  // else point 3 was lined up so everything was correct.
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulatePolyhedronCell(int i)
{
  //  We can't set the size on the nodes std::vector because we
  //  are not sure how many we are going to have.
  //  All we have to do here is add the nodes from the faces into
  //  nodes std::vector within the cell.  All we have to check for is
  //  duplicate nodes.
  //

  for (size_t j = 0; j < this->Cells[i].faces.size(); j++)
  {
    for (size_t k = 0; k < this->Faces[this->Cells[i].faces[j]].nodes.size(); k++)
    {
      int flag;
      flag = 0;
      // Is the node already in the cell?
      for (size_t n = 0; n < Cells[i].nodes.size(); n++)
      {
        if (this->Cells[i].nodes[n] == this->Faces[this->Cells[i].faces[j]].nodes[k])
        {
          flag = 1;
        }
      }
      if (flag == 0)
      {
        // No match - insert node into cell.
        this->Cells[i].nodes.push_back(this->Faces[this->Cells[i].faces[j]].nodes[k]);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetData()
{
  if (H5Gget_objinfo(this->FluentDataFile, "/results/1", false, nullptr) == 0)
  {
    int iphase = 1;
    while (
      H5Gget_objinfo(this->FluentDataFile,
        std::string("/results/1/phase-" + std::to_string(iphase)).c_str(), false, nullptr) == 0)
    {
      hid_t group, attr, dset, groupcell, space, dataType;
      group = H5Gopen(this->FluentDataFile,
        std::string("/results/1/phase-" + std::to_string(iphase)).c_str(), H5P_DEFAULT);
      groupcell = H5Gopen(group, "cells", H5P_DEFAULT);

      char* strchar;
      dset = H5Dopen(groupcell, "fields", H5P_DEFAULT);
      space = H5Dget_space(dset);
      dataType = H5Dget_type(dset);
      size_t stringLength = H5Tget_size(dataType);
      strchar = new char[stringLength];
      status = H5Dread(dset, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, strchar);
      status = H5Dclose(dset);
      status = H5Tclose(dataType);
      status = H5Sclose(space);
      std::string str(strchar);
      delete strchar;
      std::vector<std::string> v_str;
      size_t npos = 0;
      while (npos < str.length())
      {
        v_str.push_back(str.substr(npos, str.find(';', npos) - npos));
        npos = str.find(';', npos) + 1;
      }
      for (size_t i = 0; i < v_str.size(); i++)
      {
        std::string strSectionName = v_str[i];
        hid_t groupdata = H5Gopen(groupcell, strSectionName.c_str(), H5P_DEFAULT);
        if (iphase > 1)
          strSectionName =
            std::string("phase_") + std::to_string(iphase) + std::string("-") + strSectionName;

        uint64_t nSections;
        attr = H5Aopen(groupdata, "nSections", H5P_DEFAULT);
        status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
        status = H5Aclose(attr);

        for (int iSection = 0; iSection < static_cast<int>(nSections); iSection++)
        {
          dset = H5Dopen(groupdata, std::to_string(iSection + 1).c_str(), H5P_DEFAULT);

          uint64_t minId, maxId;
          attr = H5Aopen(dset, "minId", H5P_DEFAULT);
          status = H5Aread(attr, H5T_NATIVE_UINT64, &minId);
          status = H5Aclose(attr);
          attr = H5Aopen(dset, "maxId", H5P_DEFAULT);
          status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId);
          status = H5Aclose(attr);

          space = H5Dget_space(dset);
          hid_t ndims = H5Sget_simple_extent_ndims(space);
          std::vector<hsize_t> dims(ndims);
          status = H5Sget_simple_extent_dims(space, dims.data(), nullptr);
          hsize_t total_dim = 1;
          for (hid_t k = 0; k < ndims; k++)
          {
            total_dim *= dims[k];
          }

          // Data precision only in DAT file
          int type_prec = 0;
          hid_t type = H5Dget_type(dset);
          if (H5Tget_precision(type) == 32)
            type_prec = 1;
          status = H5Tclose(type);

          std::vector<double> data(total_dim);
          if (type_prec == 0)
          {
            status = H5Dread(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
          }
          else
          {
            // This could be improved by using datatype and dataspace in HDF5
            // to directly read the float data into double format.
            std::vector<float> dataf(total_dim);
            status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataf.data());
            for (size_t j = 0; j < total_dim; j++)
              data[j] = static_cast<double>(dataf[j]);
          }

          if (ndims == 1)
          {
            this->NumberOfScalars++;
            this->ScalarDataChunks.emplace_back();
            this->ScalarDataChunks.back().variableName = strSectionName;
            for (size_t j = minId; j <= maxId; j++)
            {
              this->ScalarDataChunks.back().scalarData.push_back(data[j - 1]);
            }
          }
          else
          {
            this->NumberOfVectors++;
            this->VectorDataChunks.emplace_back();
            this->VectorDataChunks.back().variableName = strSectionName;
            for (size_t j = minId; j <= maxId; j++)
            {
              this->VectorDataChunks.back().iComponentData.push_back(data[dims[1] * (j - 1)]);
              this->VectorDataChunks.back().jComponentData.push_back(data[dims[1] * (j - 1) + 1]);
              if (ndims == 3)
                this->VectorDataChunks.back().kComponentData.push_back(data[dims[1] * (j - 1) + 2]);
              else
                this->VectorDataChunks.back().kComponentData.push_back(0.0);
            }
          }

          status = H5Tclose(dataType);
          status = H5Sclose(space);
          status = H5Dclose(dset);
        }

        status = H5Gclose(groupdata);
      }

      status = H5Gclose(groupcell);
      status = H5Gclose(group);
      iphase++;
    }
  }
}
VTK_ABI_NAMESPACE_END
