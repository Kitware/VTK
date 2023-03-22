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

// Hide VTK_DEPRECATED_IN_9_0_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkFLUENTCFFReader.h"
#include "fstream"
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
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFLUENTCFFReader);

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

// Structures
struct vtkFLUENTCFFReader::Cell
{
  int type;
  int zone;
  std::vector<int> faces;
  int parent;
  int child;
  std::vector<int> nodes;
  std::vector<int> childId;
};

struct vtkFLUENTCFFReader::Face
{
  int type;
  unsigned int zone;
  std::vector<int> nodes;
  int c0;
  int c1;
  int periodicShadow;
  int parent;
  int child;
  int interfaceFaceParent;
  int interfaceFaceChild;
  int ncgParent;
  int ncgChild;
};

struct vtkFLUENTCFFReader::ScalarDataChunk
{
  std::string variableName;
  vtkIdType zoneId;
  std::vector<double> scalarData;
};

struct vtkFLUENTCFFReader::VectorDataChunk
{
  std::string variableName;
  vtkIdType zoneId;
  std::vector<double> iComponentData;
  std::vector<double> jComponentData;
  std::vector<double> kComponentData;
};

struct vtkFLUENTCFFReader::stdString
{
  std::string value;
};
struct vtkFLUENTCFFReader::intVector
{
  std::vector<int> value;
};
struct vtkFLUENTCFFReader::doubleVector
{
  std::vector<double> value;
};
struct vtkFLUENTCFFReader::stringVector
{
  std::vector<std::string> value;
};
struct vtkFLUENTCFFReader::cellVector
{
  std::vector<Cell> value;
};
struct vtkFLUENTCFFReader::faceVector
{
  std::vector<Face> value;
};
struct vtkFLUENTCFFReader::stdMap
{
  std::map<int, std::string> value;
};
struct vtkFLUENTCFFReader::scalarDataVector
{
  std::vector<ScalarDataChunk> value;
};
struct vtkFLUENTCFFReader::vectorDataVector
{
  std::vector<VectorDataChunk> value;
};
struct vtkFLUENTCFFReader::intVectorVector
{
  std::vector<std::vector<int>> value;
};

//------------------------------------------------------------------------------
vtkFLUENTCFFReader::vtkFLUENTCFFReader()
{
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->FileName = nullptr;
  this->NumberOfCells = 0;
  this->NumberOfCellArrays = 0;

  status = H5open();
  if (status < 0)
    vtkErrorMacro("HDF5 library initialisation error");
  status = H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
  this->FluentCaseFile = static_cast<hid_t>(-1);
  this->FluentDataFile = static_cast<hid_t>(-1);

  this->Points = vtkPoints::New();
  this->Triangle = vtkTriangle::New();
  this->Tetra = vtkTetra::New();
  this->Quad = vtkQuad::New();
  this->Hexahedron = vtkHexahedron::New();
  this->Pyramid = vtkPyramid::New();
  this->Wedge = vtkWedge::New();
  this->ConvexPointSet = vtkConvexPointSet::New();

  this->Cells = new cellVector;
  this->Faces = new faceVector;
  this->CellZones = new intVector;
  this->ScalarDataChunks = new scalarDataVector;
  this->VectorDataChunks = new vectorDataVector;

  this->SwapBytes = 0;
  this->GridDimension = 0;
  this->DataPass = 0;
  this->NumberOfScalars = 0;
  this->NumberOfVectors = 0;

  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkFLUENTCFFReader::~vtkFLUENTCFFReader()
{
  this->Points->Delete();
  this->Triangle->Delete();
  this->Tetra->Delete();
  this->Quad->Delete();
  this->Hexahedron->Delete();
  this->Pyramid->Delete();
  this->Wedge->Delete();
  this->ConvexPointSet->Delete();

  delete this->Cells;
  delete this->Faces;
  delete this->CellZones;
  delete this->ScalarDataChunks;
  delete this->VectorDataChunks;
  status = H5close();
  if (status < 0)
    vtkErrorMacro("HDF5 library error");

  this->CellDataArraySelection->Delete();

  delete[] this->FileName;
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName)
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

  // Read data
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
  for (size_t i = 0; i < this->ScalarDataChunks->value.size(); i++)
  {
    this->CellDataArraySelection->AddArray(this->ScalarDataChunks->value[i].variableName.c_str());
  }
  for (size_t i = 0; i < this->VectorDataChunks->value.size(); i++)
  {
    this->CellDataArraySelection->AddArray(this->VectorDataChunks->value[i].variableName.c_str());
  }
  this->NumberOfCells = static_cast<vtkIdType>(this->Cells->value.size());

  output->SetNumberOfBlocks(static_cast<unsigned int>(this->CellZones->value.size()));
  // vtkUnstructuredGrid *Grid[CellZones.size()];

  std::vector<vtkUnstructuredGrid*> grid;
  grid.resize(this->CellZones->value.size());

  for (size_t test = 0; test < this->CellZones->value.size(); test++)
  {
    grid[test] = vtkUnstructuredGrid::New();
  }

  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    size_t location = std::find(this->CellZones->value.begin(), this->CellZones->value.end(),
                        this->Cells->value[i].zone) -
      this->CellZones->value.begin();

    if (this->Cells->value[i].type == 1)
    {
      for (int j = 0; j < 3; j++)
      {
        this->Triangle->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Triangle->GetCellType(), this->Triangle->GetPointIds());
    }
    else if (this->Cells->value[i].type == 2)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Tetra->GetPointIds()->SetId(j, Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Tetra->GetCellType(), this->Tetra->GetPointIds());
    }
    else if (this->Cells->value[i].type == 3)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Quad->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Quad->GetCellType(), this->Quad->GetPointIds());
    }
    else if (this->Cells->value[i].type == 4)
    {
      for (int j = 0; j < 8; j++)
      {
        this->Hexahedron->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->Hexahedron->GetCellType(), this->Hexahedron->GetPointIds());
    }
    else if (this->Cells->value[i].type == 5)
    {
      for (int j = 0; j < 5; j++)
      {
        this->Pyramid->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Pyramid->GetCellType(), this->Pyramid->GetPointIds());
    }
    else if (this->Cells->value[i].type == 6)
    {
      for (int j = 0; j < 6; j++)
      {
        this->Wedge->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Wedge->GetCellType(), this->Wedge->GetPointIds());
    }
    else if (this->Cells->value[i].type == 7)
    {
      this->ConvexPointSet->GetPointIds()->SetNumberOfIds(
        static_cast<vtkIdType>(this->Cells->value[i].nodes.size()));
      for (size_t j = 0; j < this->Cells->value[i].nodes.size(); j++)
      {
        this->ConvexPointSet->GetPointIds()->SetId(
          static_cast<vtkIdType>(j), this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->ConvexPointSet->GetCellType(), this->ConvexPointSet->GetPointIds());
    }
  }
  //  this->Cells->value.clear();

  // Scalar Data
  for (size_t l = 0; l < this->ScalarDataChunks->value.size(); l++)
  {
    if (this->CellDataArraySelection->ArrayIsEnabled(
          this->ScalarDataChunks->value[l].variableName.c_str()))
    {
      for (size_t location = 0; location < this->CellZones->value.size(); location++)
      {
        vtkDoubleArray* v = vtkDoubleArray::New();
        unsigned int i = 0;
        for (size_t m = 0; m < this->ScalarDataChunks->value[l].scalarData.size(); m++)
        {
          if (this->Cells->value[m].zone == this->CellZones->value[location])
          {
            v->InsertValue(
              static_cast<vtkIdType>(i), this->ScalarDataChunks->value[l].scalarData[m]);
            i++;
          }
        }
        v->SetName(this->ScalarDataChunks->value[l].variableName.c_str());
        grid[location]->GetCellData()->AddArray(v);
        v->Delete();
      }
    }
  }
  this->ScalarDataChunks->value.clear();

  // Vector Data
  for (size_t l = 0; l < this->VectorDataChunks->value.size(); l++)
  {
    if (this->CellDataArraySelection->ArrayIsEnabled(
          this->VectorDataChunks->value[l].variableName.c_str()))
    {
      for (size_t location = 0; location < this->CellZones->value.size(); location++)
      {
        vtkDoubleArray* v = vtkDoubleArray::New();
        unsigned int i = 0;
        v->SetNumberOfComponents(3);
        for (size_t m = 0; m < this->VectorDataChunks->value[l].iComponentData.size(); m++)
        {
          if (this->Cells->value[m].zone == this->CellZones->value[location])
          {
            v->InsertComponent(
              static_cast<vtkIdType>(i), 0, this->VectorDataChunks->value[l].iComponentData[m]);
            v->InsertComponent(
              static_cast<vtkIdType>(i), 1, this->VectorDataChunks->value[l].jComponentData[m]);
            v->InsertComponent(
              static_cast<vtkIdType>(i), 2, this->VectorDataChunks->value[l].kComponentData[m]);
            i++;
          }
        }
        v->SetName(this->VectorDataChunks->value[l].variableName.c_str());
        grid[location]->GetCellData()->AddArray(v);
        v->Delete();
      }
    }
  }
  this->VectorDataChunks->value.clear();

  for (size_t addTo = 0; addTo < this->CellZones->value.size(); addTo++)
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
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number Of Cell Zone: " << this->CellZones->value.size() << endl;
  if (this->DataPass == 1)
  {
    os << indent << "List Of Scalar Value : " << this->ScalarDataChunks->value.size() << endl;
    if (this->ScalarDataChunks->value.size() != 0)
    {
      os << indent;
      for (size_t i = 0; i < this->ScalarDataChunks->value.size(); i++)
      {
        os << this->ScalarDataChunks->value[i].variableName;
      }
      os << endl;
    }
    os << indent << "List Of Vector Value : " << this->VectorDataChunks->value.size() << endl;
    if (this->VectorDataChunks->value.size() != 0)
    {
      os << indent;
      for (size_t i = 0; i < this->VectorDataChunks->value.size(); i++)
      {
        os << this->VectorDataChunks->value[i].variableName;
      }
      os << endl;
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTCFFReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
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
bool vtkFLUENTCFFReader::OpenCaseFile(const char* filename)
{
  // Check if the file is HDF5 or exist
  htri_t file_type = H5Fis_hdf5(filename);
  if (file_type != 1)
  {
    vtkErrorMacro("The file " << filename << " does not exist or is not a HDF5 file.");
    return false;
  }
  // Open file with default properties access
  this->FluentCaseFile = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
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
bool vtkFLUENTCFFReader::OpenDataFile(const char* filename)
{
  std::string dfilename(filename);
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
  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    if (this->CellZones->value.empty())
    {
      this->CellZones->value.push_back(this->Cells->value[i].zone);
    }
    else
    {
      int match = 0;
      for (size_t j = 0; j < this->CellZones->value.size(); j++)
      {
        if (this->CellZones->value[j] == this->Cells->value[i].zone)
        {
          match = 1;
        }
      }
      if (match == 0)
      {
        this->CellZones->value.push_back(this->Cells->value[i].zone);
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

  uint64_t *minId, *maxId, *dimension;
  int32_t* Id;

  minId = new uint64_t[nZones];
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId);
  status = H5Dclose(dset);

  maxId = new uint64_t[nZones];
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId);
  status = H5Dclose(dset);

  Id = new int32_t[nZones];
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id);
  status = H5Dclose(dset);

  dimension = new uint64_t[nZones];
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension);
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    double* nodeData = nullptr;
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

    nodeData = new double[gSize];
    status = H5Dread(dset_coords, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, nodeData);
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

    delete nodeData;
  }

  delete minId;
  delete maxId;
  delete Id;
  delete dimension;

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
  this->Cells->value.resize(lastIndex);
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

  uint64_t *minId, *maxId, *dimension;
  int32_t *Id, *cellType, *childZoneId;

  minId = new uint64_t[nZones];
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId);
  status = H5Dclose(dset);

  maxId = new uint64_t[nZones];
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId);
  status = H5Dclose(dset);

  Id = new int32_t[nZones];
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id);
  status = H5Dclose(dset);

  dimension = new uint64_t[nZones];
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension);
  status = H5Dclose(dset);

  cellType = new int32_t[nZones];
  dset = H5Dopen(group, "cellType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, cellType);
  status = H5Dclose(dset);

  childZoneId = new int32_t[nZones];
  dset = H5Dopen(group, "childZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, childZoneId);
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    unsigned int elementType = static_cast<unsigned int>(cellType[iZone]);
    unsigned int zoneId = static_cast<unsigned int>(Id[iZone]);
    unsigned int firstIndex = static_cast<unsigned int>(minId[iZone]);
    unsigned int lastIndex = static_cast<unsigned int>(maxId[iZone]);
    // unsigned int child = static_cast<unsigned int>(childZoneId[iZone]);

    if (elementType == 0)
    {
      int16_t* cellTypeData = nullptr;
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
          cellTypeData = new int16_t[ctype_maxId - ctype_minId + 1];
          dset = H5Dopen(group_ctype, "cell-types", H5P_DEFAULT);
          status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, cellTypeData);
          status = H5Dclose(dset);
          status = H5Gclose(group_ctype);
          break;
        }
        status = H5Gclose(group_ctype);
      }

      if (cellTypeData != nullptr)
      {
        for (unsigned int i = firstIndex; i <= lastIndex; i++)
        {
          this->Cells->value[i - 1].type = static_cast<unsigned int>(cellTypeData[i - ctype_minId]);
          this->Cells->value[i - 1].zone = zoneId;
          this->Cells->value[i - 1].parent = 0;
          this->Cells->value[i - 1].child = 0; // child;
        }

        delete cellTypeData;
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Cells->value[i - 1].type = elementType;
        this->Cells->value[i - 1].zone = zoneId;
        this->Cells->value[i - 1].parent = 0;
        this->Cells->value[i - 1].child = 0; // child;
      }
    }
  }

  delete minId;
  delete maxId;
  delete Id;
  delete dimension;
  delete cellType;
  delete childZoneId;

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
  this->Faces->value.resize(lastIndex);
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

  uint64_t *minId, *maxId, *dimension;
  int32_t *Id, *zoneT, *faceT, *childZoneId, *shadowZoneId, *flags;

  minId = new uint64_t[nZones];
  dset = H5Dopen(group, "minId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, minId);
  status = H5Dclose(dset);

  maxId = new uint64_t[nZones];
  dset = H5Dopen(group, "maxId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, maxId);
  status = H5Dclose(dset);

  Id = new int32_t[nZones];
  dset = H5Dopen(group, "id", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, Id);
  status = H5Dclose(dset);

  dimension = new uint64_t[nZones];
  dset = H5Dopen(group, "dimension", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, dimension);
  status = H5Dclose(dset);

  zoneT = new int32_t[nZones];
  dset = H5Dopen(group, "zoneType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, zoneT);
  status = H5Dclose(dset);

  faceT = new int32_t[nZones];
  dset = H5Dopen(group, "faceType", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, faceT);
  status = H5Dclose(dset);

  childZoneId = new int32_t[nZones];
  dset = H5Dopen(group, "childZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, childZoneId);
  status = H5Dclose(dset);

  shadowZoneId = new int32_t[nZones];
  dset = H5Dopen(group, "shadowZoneId", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, shadowZoneId);
  status = H5Dclose(dset);

  flags = new int32_t[nZones];
  dset = H5Dopen(group, "flags", H5P_DEFAULT);
  status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, flags);
  status = H5Dclose(dset);

  for (uint64_t iZone = 0; iZone < nZones; iZone++)
  {
    unsigned int zoneId = static_cast<unsigned int>(Id[iZone]);
    unsigned int firstIndex = static_cast<unsigned int>(minId[iZone]);
    unsigned int lastIndex = static_cast<unsigned int>(maxId[iZone]);
    // unsigned int child = static_cast<unsigned int>(childZoneId[iZone]);
    // unsigned int shadow = static_cast<unsigned int>(shadowZoneId[iZone]);

    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Faces->value[i - 1].zone = zoneId;
      this->Faces->value[i - 1].periodicShadow = 0; // shadow;
      this->Faces->value[i - 1].parent = 0;
      this->Faces->value[i - 1].child = 0; // child;
      this->Faces->value[i - 1].interfaceFaceParent = 0;
      this->Faces->value[i - 1].ncgParent = 0;
      this->Faces->value[i - 1].ncgChild = 0;
      this->Faces->value[i - 1].interfaceFaceChild = 0;
      // TODO: zoneType ?
    }
  }

  delete minId;
  delete maxId;
  delete Id;
  delete dimension;
  delete zoneT;
  delete faceT;
  delete childZoneId;
  delete shadowZoneId;
  delete flags;

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
    int16_t* nnodes_fnodes;
    uint32_t* nodes_fnodes;
    uint64_t minId_fnodes, maxId_fnodes, nodes_size;
    std::string groupname = std::string("/meshes/1/faces/nodes/" + std::to_string(iSection + 1));
    group = H5Gopen(this->FluentCaseFile, groupname.c_str(), H5P_DEFAULT);

    attr = H5Aopen(group, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minId_fnodes);
    status = H5Aclose(attr);
    attr = H5Aopen(group, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxId_fnodes);
    status = H5Aclose(attr);

    nnodes_fnodes = new int16_t[maxId_fnodes - minId_fnodes + 1];
    dset = H5Dopen(group, "nnodes", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nnodes_fnodes);
    status = H5Dclose(dset);

    dset = H5Dopen(group, "nodes", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &nodes_size);
    status = H5Aclose(attr);
    nodes_fnodes = new uint32_t[nodes_size];
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, nodes_fnodes);
    status = H5Dclose(dset);

    int numberOfNodesInFace = 0;
    uint64_t ptr = minId_fnodes;
    for (unsigned int i = static_cast<unsigned int>(minId_fnodes);
         i <= static_cast<unsigned int>(maxId_fnodes); i++)
    {
      numberOfNodesInFace = static_cast<int>(nnodes_fnodes[i - minId_fnodes]);

      this->Faces->value[i - 1].nodes.resize(numberOfNodesInFace);
      this->Faces->value[i - 1].type = numberOfNodesInFace;

      for (int k = 0; k < numberOfNodesInFace; k++)
      {
        this->Faces->value[i - 1].nodes[k] = static_cast<int>(nodes_fnodes[ptr - 1]) - 1;
        ptr++;
      }
    }

    delete nnodes_fnodes;
    delete nodes_fnodes;
    status = H5Gclose(group);
  }

  // C0 C1
  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/c0", H5P_DEFAULT);
  attr = H5Aopen(group, "nSections", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
  status = H5Aclose(attr);
  for (uint64_t iSection = 0; iSection < nSections; iSection++)
  {
    uint32_t* c0;
    uint64_t minc0, maxc0;

    dset = H5Dopen(group, std::to_string(iSection + 1).c_str(), H5P_DEFAULT);

    attr = H5Aopen(dset, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minc0);
    status = H5Aclose(attr);
    attr = H5Aopen(dset, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxc0);
    status = H5Aclose(attr);

    c0 = new uint32_t[maxc0 - minc0 + 1];
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, c0);
    status = H5Dclose(dset);

    for (unsigned int i = static_cast<unsigned int>(minc0); i <= static_cast<unsigned int>(maxc0);
         i++)
    {
      this->Faces->value[i - 1].c0 = static_cast<int>(c0[i - minc0]) - 1;
      if (this->Faces->value[i - 1].c0 >= 0)
      {
        this->Cells->value[this->Faces->value[i - 1].c0].faces.push_back(i - 1);
      }
    }

    delete c0;
  }
  status = H5Gclose(group);

  group = H5Gopen(this->FluentCaseFile, "/meshes/1/faces/c1", H5P_DEFAULT);
  attr = H5Aopen(group, "nSections", H5P_DEFAULT);
  status = H5Aread(attr, H5T_NATIVE_UINT64, &nSections);
  status = H5Aclose(attr);
  for (size_t i = 0; i < this->Faces->value.size(); i++)
  {
    this->Faces->value[i].c1 = -1;
  }
  for (uint64_t iSection = 0; iSection < nSections; iSection++)
  {
    uint32_t* c1;
    uint64_t minc1, maxc1;

    dset = H5Dopen(group, std::to_string(iSection + 1).c_str(), H5P_DEFAULT);

    attr = H5Aopen(dset, "minId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &minc1);
    status = H5Aclose(attr);
    attr = H5Aopen(dset, "maxId", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &maxc1);
    status = H5Aclose(attr);

    c1 = new uint32_t[maxc1 - minc1 + 1];
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, c1);
    status = H5Dclose(dset);

    for (unsigned int i = static_cast<unsigned int>(minc1); i <= static_cast<unsigned int>(maxc1);
         i++)
    {
      this->Faces->value[i - 1].c1 = static_cast<int>(c1[i - minc1]) - 1;
      if (this->Faces->value[i - 1].c1 >= 0)
      {
        this->Cells->value[this->Faces->value[i - 1].c1].faces.push_back(i - 1);
      }
    }

    delete c1;
  }

  status = H5Gclose(group);
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetPeriodicShadowFaces()
{
  // TODO: Periodic shadow faces has not been tested because no test file available
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetCellOverset()
{
  herr_t s1 = H5Gget_objinfo(this->FluentCaseFile, "/special/Overset_DCI", false, nullptr);
  if (s1 == 0)
  {
    vtkWarningMacro("The overset layout of this CFF file cannot be displayed by this reader.");
    // TODO: Overset has not been tested because no test file available
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

    int32_t *topology;
    topology = new int32_t[nSections];
    status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, topology);
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

      int32_t *ndata;
      ndata = new int32_t[maxId - minId + 1];
      dset = H5Dopen(groupTopo, "ndata", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, ndata);
      status = H5Dclose(dset);

      for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
    i++)
      {
        if (ndata[i - minId] != 4)
          this->Cells->value[i - 1].overset = 1;
      }

      dset = H5Dopen(groupTopo, "data", H5P_DEFAULT);
      uint64_t size_data;
      attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
      status = H5Aread(attr, H5T_NATIVE_UINT64, &size_data);
      status = H5Aclose(attr);
      int8_t *data;
      data = new int8_t[size_data];
      status = H5Dread(dset, H5T_NATIVE_INT8, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
      status = H5Dclose(dset);

      delete ndata;
      delete data;

      status = H5Dclose(dset);
      status = H5Gclose(groupTopo);
    }

    delete topology;
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

    int16_t* nkids;
    uint32_t* kids;
    nkids = new int16_t[maxId - minId + 1];
    dset = H5Dopen(group, "nkids", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nkids);
    status = H5Dclose(dset);

    uint64_t kids_size;
    dset = H5Dopen(group, "kids", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &kids_size);
    status = H5Aclose(attr);
    kids = new uint32_t[kids_size];
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, kids);
    status = H5Dclose(dset);

    uint64_t ptr = 0;
    for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
         i++)
    {
      this->Cells->value[i - 1].parent = 1;
      int numberOfKids = static_cast<int>(nkids[i - minId]);
      this->Cells->value[i - 1].childId.resize(numberOfKids);
      for (int j = 0; j < numberOfKids; j++)
      {
        this->Cells->value[kids[ptr] - 1].child = 1;
        this->Cells->value[i - 1].childId[j] = kids[ptr] - 1;
        ptr++;
      }
    }

    delete nkids;
    delete kids;

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

    int16_t* nkids;
    uint32_t* kids;
    nkids = new int16_t[maxId - minId + 1];
    dset = H5Dopen(group, "nkids", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, nkids);
    status = H5Dclose(dset);

    uint64_t kids_size;
    dset = H5Dopen(group, "kids", H5P_DEFAULT);
    attr = H5Aopen(dset, "chunkDim", H5P_DEFAULT);
    status = H5Aread(attr, H5T_NATIVE_UINT64, &kids_size);
    status = H5Aclose(attr);
    kids = new uint32_t[kids_size];
    status = H5Dread(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, kids);
    status = H5Dclose(dset);

    uint64_t ptr = 0;
    for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
         i++)
    {
      this->Faces->value[i - 1].parent = 1;
      int numberOfKids = static_cast<int>(nkids[i - minId]);
      for (int j = 0; j < numberOfKids; j++)
      {
        this->Faces->value[kids[ptr] - 1].child = 1;
        ptr++;
      }
    }

    delete nkids;
    delete kids;

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

    uint64_t* nciTopology;
    nciTopology = new uint64_t[nData * nZones];
    dset = H5Dopen(group, "nciTopology", H5P_DEFAULT);
    status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, nciTopology);
    status = H5Dclose(dset);

    for (uint64_t iZone = 0; iZone < nZones; iZone++)
    {
      int zoneId = static_cast<int>(nciTopology[iZone * nData]);
      int minId = static_cast<int>(nciTopology[iZone * nData + 1]);
      int maxId = static_cast<int>(nciTopology[iZone * nData + 2]);

      hid_t group_int = H5Gopen(group, std::to_string(zoneId).c_str(), H5P_DEFAULT);

      uint64_t *pf0, *pf1;
      pf0 = new uint64_t[maxId - minId + 1];
      pf1 = new uint64_t[maxId - minId + 1];
      dset = H5Dopen(group_int, "pf0", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, pf0);
      status = H5Dclose(dset);
      dset = H5Dopen(group_int, "pf1", H5P_DEFAULT);
      status = H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, pf1);
      status = H5Dclose(dset);

      for (unsigned int i = static_cast<unsigned int>(minId); i <= static_cast<unsigned int>(maxId);
           i++)
      {
        unsigned int parentId0 = static_cast<unsigned int>(pf0[i - minId]);
        unsigned int parentId1 = static_cast<unsigned int>(pf1[i - minId]);

        this->Faces->value[parentId0 - 1].interfaceFaceParent = 1;
        this->Faces->value[parentId1 - 1].interfaceFaceParent = 1;
        this->Faces->value[i - 1].interfaceFaceChild = 1;
      }

      delete pf0;
      delete pf1;

      status = H5Gclose(group_int);
    }

    delete nciTopology;

    status = H5Gclose(group);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::GetNonconformalGridInterfaceFaceInformation()
{
  // TODO: Non conformal grid interace faces has not been tested because no test file available
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::CleanCells()
{

  std::vector<int> t;
  for (size_t i = 0; i < Cells->value.size(); i++)
  {

    if (((this->Cells->value[i].type == 1) && (this->Cells->value[i].faces.size() != 3)) ||
      ((this->Cells->value[i].type == 2) && (this->Cells->value[i].faces.size() != 4)) ||
      ((this->Cells->value[i].type == 3) && (this->Cells->value[i].faces.size() != 4)) ||
      ((this->Cells->value[i].type == 4) && (this->Cells->value[i].faces.size() != 6)) ||
      ((this->Cells->value[i].type == 5) && (this->Cells->value[i].faces.size() != 5)) ||
      ((this->Cells->value[i].type == 6) && (this->Cells->value[i].faces.size() != 5)))
    {

      // Copy faces
      t.clear();
      for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
      {
        t.push_back(this->Cells->value[i].faces[j]);
      }

      // Clear Faces
      this->Cells->value[i].faces.clear();

      // Copy the faces that are not flagged back into the cell
      for (size_t j = 0; j < t.size(); j++)
      {
        if ((this->Faces->value[t[j]].child == 0) && (this->Faces->value[t[j]].ncgChild == 0) &&
          (this->Faces->value[t[j]].interfaceFaceChild == 0))
        {
          this->Cells->value[i].faces.push_back(t[j]);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateCellTree()
{
  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    // If cell is parent cell -> interpolate data from children
    if (this->Cells->value[i].parent == 1)
    {
      for (size_t k = 0; k < this->ScalarDataChunks->value.size(); k++)
      {
        double data = 0.0;
        int ncell = 0;
        for (size_t j = 0; j < this->Cells->value[i].childId.size(); j++)
        {
          if (this->Cells->value[this->Cells->value[i].childId[j]].parent == 0)
          {
            data += this->ScalarDataChunks->value[k].scalarData[this->Cells->value[i].childId[j]];
            ncell++;
          }
        }
        if (ncell == 0)
          this->ScalarDataChunks->value[k].scalarData.push_back(0.0);
        else
          this->ScalarDataChunks->value[k].scalarData.push_back(data / (double)ncell);
      }
      for (size_t k = 0; k < this->VectorDataChunks->value.size(); k++)
      {
        double datax = 0.0;
        double datay = 0.0;
        double dataz = 0.0;
        int ncell = 0;
        for (size_t j = 0; j < this->Cells->value[i].childId.size(); j++)
        {
          if (this->Cells->value[this->Cells->value[i].childId[j]].parent == 0)
          {
            datax +=
              this->VectorDataChunks->value[k].iComponentData[this->Cells->value[i].childId[j]];
            datay +=
              this->VectorDataChunks->value[k].jComponentData[this->Cells->value[i].childId[j]];
            dataz +=
              this->VectorDataChunks->value[k].kComponentData[this->Cells->value[i].childId[j]];
            ncell++;
          }
        }
        if (ncell == 0)
        {
          this->VectorDataChunks->value[k].iComponentData.push_back(0.0);
          this->VectorDataChunks->value[k].jComponentData.push_back(0.0);
          this->VectorDataChunks->value[k].kComponentData.push_back(0.0);
        }
        else
        {
          this->VectorDataChunks->value[k].iComponentData.push_back(datax / (double)ncell);
          this->VectorDataChunks->value[k].jComponentData.push_back(datay / (double)ncell);
          this->VectorDataChunks->value[k].kComponentData.push_back(dataz / (double)ncell);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateCellNodes()
{
  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    const vtkIdType id = static_cast<vtkIdType>(i);
    switch (this->Cells->value[i].type)
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
  this->Cells->value[i].nodes.resize(3);
  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[1])
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateTetraCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }
  else
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[1] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[2])
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] != this->Cells->value[i].nodes[1] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] != this->Cells->value[i].nodes[2])
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[2];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateQuadCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if ((this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
          this->Cells->value[i].nodes[0] &&
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
          this->Cells->value[i].nodes[1]) &&
    (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
        this->Cells->value[i].nodes[0] &&
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
        this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[1]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
  }
  else if ((this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
               this->Cells->value[i].nodes[0] &&
             this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
               this->Cells->value[i].nodes[1]) &&
    (this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
        this->Cells->value[i].nodes[0] &&
      this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
        this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[2]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
  }
  else
  {
    if (this->Faces->value[this->Cells->value[i].faces[3]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateHexahedronCell(int i)
{
  this->Cells->value[i].nodes.resize(8);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    for (int j = 0; j < 4; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }
  else
  {
    for (int j = 3; j >= 0; j--)
    {
      this->Cells->value[i].nodes[3 - j] =
        this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }

  //  Look for opposite face of hexahedron
  for (int j = 1; j < 6; j++)
  {
    int flag = 0;
    for (int k = 0; k < 4; k++)
    {
      if ((this->Cells->value[i].nodes[0] ==
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[1] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[2] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[3] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]))
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c1 == i)
      {
        for (int k = 4; k < 8; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k - 4];
        }
      }
      else
      {
        for (int k = 7; k >= 4; k--)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[7 - k];
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
      if (this->Cells->value[i].nodes[0] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[1] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }
    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
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
      if (this->Cells->value[i].nodes[0] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[3] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }

    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // What point is in f01 and f03 besides 0 ... this is point 4
  int p4 = 0;
  for (int k = 0; k < 4; k++)
  {
    if (f01[k] != this->Cells->value[i].nodes[0])
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
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  t[6] = this->Cells->value[i].nodes[6];
  t[7] = this->Cells->value[i].nodes[7];
  if (p4 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[5] = t[6];
    this->Cells->value[i].nodes[6] = t[7];
    this->Cells->value[i].nodes[7] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
  }
  else if (p4 == Cells->value[i].nodes[6])
  {
    this->Cells->value[i].nodes[5] = t[7];
    this->Cells->value[i].nodes[6] = t[4];
    this->Cells->value[i].nodes[7] = t[5];
    this->Cells->value[i].nodes[4] = t[6];
  }
  else if (p4 == Cells->value[i].nodes[7])
  {
    this->Cells->value[i].nodes[5] = t[4];
    this->Cells->value[i].nodes[6] = t[5];
    this->Cells->value[i].nodes[7] = t[6];
    this->Cells->value[i].nodes[4] = t[7];
  }
  // else point 4 was lined up so everything was correct.
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulatePyramidCell(int i)
{
  this->Cells->value[i].nodes.resize(5);
  //  The quad face will be the base of the pyramid
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 4)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[3 - k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // Just need to find point 4
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 3)
    {
      for (int k = 0; k < 3; k++)
      {
        if ((this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
              this->Cells->value[i].nodes[0]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[1]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[2]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[3]))
        {
          this->Cells->value[i].nodes[4] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTCFFReader::PopulateWedgeCell(int i)
{
  this->Cells->value[i].nodes.resize(6);

  //  Find the first triangle face and make it the base.
  int base = 0;
  int first = 0;
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) && (first == 0))
    {
      base = this->Cells->value[i].faces[j];
      first = 1;
    }
  }

  //  Find the second triangle face and make it the top.
  int top = 0;
  int second = 0;
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) && (second == 0) &&
      (this->Cells->value[i].faces[j] != base))
    {
      top = this->Cells->value[i].faces[j];
      second = 1;
    }
  }

  // Load Base nodes into the nodes std::vector
  if (this->Faces->value[base].c0 == i)
  {
    for (int j = 0; j < 3; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[base].nodes[j];
    }
  }
  else
  {
    for (int j = 2; j >= 0; j--)
    {
      this->Cells->value[i].nodes[2 - j] = this->Faces->value[base].nodes[j];
    }
  }
  // Load Top nodes into the nodes std::vector
  if (this->Faces->value[top].c1 == i)
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[j - 3];
    }
  }
  else
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[5 - j];
    }
  }

  //  Find the quad face with points 0 and 1 in them.
  int w01[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base && this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf1 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[1] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf1 = 1;
        }
        if ((wf0 == 1) && (wf1 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w01[n] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  //  Find the quad face with points 0 and 2 in them.
  int w02[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base && this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf2 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[2] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf2 = 1;
        }
        if ((wf0 == 1) && (wf2 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w02[n] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
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
    if (w01[k] != this->Cells->value[i].nodes[0])
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
  t[3] = this->Cells->value[i].nodes[3];
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  if (p3 == this->Cells->value[i].nodes[4])
  {
    this->Cells->value[i].nodes[3] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
    this->Cells->value[i].nodes[5] = t[3];
  }
  else if (p3 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[3] = t[5];
    this->Cells->value[i].nodes[4] = t[3];
    this->Cells->value[i].nodes[5] = t[4];
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

  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    for (size_t k = 0; k < this->Faces->value[this->Cells->value[i].faces[j]].nodes.size(); k++)
    {
      int flag;
      flag = 0;
      // Is the node already in the cell?
      for (size_t n = 0; n < Cells->value[i].nodes.size(); n++)
      {
        if (this->Cells->value[i].nodes[n] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          flag = 1;
        }
      }
      if (flag == 0)
      {
        // No match - insert node into cell.
        this->Cells->value[i].nodes.push_back(
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]);
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

          hsize_t* dims;
          space = H5Dget_space(dset);
          hid_t ndims = H5Sget_simple_extent_ndims(space);
          dims = new hsize_t[ndims];
          status = H5Sget_simple_extent_dims(space, dims, nullptr);
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

          double* data;
          if (type_prec == 0)
          {
            data = new double[total_dim];
            status = H5Dread(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
          }
          else
          {
            // This could be improved by using datatype and dataspace in HDF5
            // to directly read the float data into double format.
            float* dataf;
            dataf = new float[total_dim];
            status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataf);
            data = new double[total_dim];
            for (size_t j = 0; j < total_dim; j++)
              data[j] = static_cast<double>(dataf[j]);
            delete dataf;
          }

          if (ndims == 1)
          {
            this->NumberOfScalars++;
            this->ScalarDataChunks->value.resize(this->ScalarDataChunks->value.size() + 1);
            this->ScalarDataChunks->value[this->ScalarDataChunks->value.size() - 1].variableName =
              strSectionName;
            for (size_t j = minId; j <= maxId; j++)
            {
              this->ScalarDataChunks->value[this->ScalarDataChunks->value.size() - 1]
                .scalarData.push_back(data[j - 1]);
            }
          }
          else
          {
            this->NumberOfVectors++;
            this->VectorDataChunks->value.resize(this->VectorDataChunks->value.size() + 1);
            this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1].variableName =
              strSectionName;
            for (size_t j = minId; j <= maxId; j++)
            {
              this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
                .iComponentData.push_back(data[dims[1] * (j - 1)]);
              this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
                .jComponentData.push_back(data[dims[1] * (j - 1) + 1]);
              if (ndims == 3)
                this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
                  .kComponentData.push_back(data[dims[1] * (j - 1) + 2]);
              else
                this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
                  .kComponentData.push_back(0.0);
            }
          }

          delete data;
          delete dims;
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
