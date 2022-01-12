/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Test3DTilesWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCesium3DTilesWriter.h"
#include "vtkTesting.h"
#include <vtkCellData.h>
#include <vtkCityGMLReader.h>
#include <vtkCompositeDataIterator.h>
#include <vtkDataObject.h>
#include <vtkDirectory.h>
#include <vtkDoubleArray.h>
#include <vtkGLTFReader.h>
#include <vtkIncrementalOctreeNode.h>
#include <vtkIncrementalOctreePointLocator.h>
#include <vtkLogger.h>
#include <vtkMathUtilities.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <array>
#include <set>
#include <sstream>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

using namespace vtksys;
using namespace nlohmann;
//------------------------------------------------------------------------------
void SetField(vtkDataObject* obj, const char* name, const char* value)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
    fd = newfd;
  }
  vtkNew<vtkStringArray> sa;
  sa->SetNumberOfTuples(1);
  sa->SetValue(0, value);
  sa->SetName(name);
  fd->AddArray(sa);
}

//------------------------------------------------------------------------------
std::array<double, 3> ReadOBJOffset(const char* comment)
{
  std::array<double, 3> translation = { 0, 0, 0 };
  if (comment)
  {
    std::istringstream istr(comment);
    std::array<std::string, 3> axesNames = { "x", "y", "z" };
    for (int i = 0; i < 3; ++i)
    {
      std::string axis;
      std::string s;
      istr >> axis >> s >> translation[i];
      if (istr.fail())
      {
        vtkLog(WARNING, "Cannot read axis " << axesNames[i] << " from comment.");
      }
      if (axis != axesNames[i])
      {
        vtkLog(WARNING, "Invalid axis " << axesNames[i] << ": " << axis);
      }
    }
  }
  else
  {
    vtkLog(WARNING, "nullptr comment.");
  }
  return translation;
}

//------------------------------------------------------------------------------
std::string GetOBJTextureFileName(const std::string& file)
{
  std::string fileNoExt = SystemTools::GetFilenameWithoutExtension(file);
  return fileNoExt + ".png";
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadOBJFiles(int numberOfBuildings, int vtkNotUsed(lod),
  const std::vector<std::string>& files, std::array<double, 3>& fileOffset)
{
  auto root = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  for (size_t i = 0; i < files.size() && i < static_cast<size_t>(numberOfBuildings); ++i)
  {
    vtkNew<vtkOBJReader> reader;
    reader->SetFileName(files[i].c_str());
    reader->Update();
    if (i == 0)
    {
      fileOffset = ReadOBJOffset(reader->GetComment());
    }
    auto polyData = reader->GetOutput();
    std::string textureFileName = GetOBJTextureFileName(files[i]);
    SetField(polyData, "texture_uri", textureFileName.c_str());
    auto building = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    building->SetBlock(0, polyData);
    root->SetBlock(root->GetNumberOfBlocks(), building);
  }
  return root;
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadCityGMLFiles(int numberOfBuildings, int lod,
  const std::vector<std::string>& files, std::array<double, 3>& fileOffset)
{
  if (files.size() > 1)
  {
    vtkLog(WARNING, "Can only process one CityGML file for now.");
  }
  vtkNew<vtkCityGMLReader> reader;
  reader->SetFileName(files[0].c_str());
  reader->SetNumberOfBuildings(numberOfBuildings);
  reader->SetLOD(lod);
  reader->Update();
  vtkSmartPointer<vtkMultiBlockDataSet> root = reader->GetOutput();
  if (!root)
  {
    vtkLog(ERROR, "Expecting vtkMultiBlockDataSet");
    return nullptr;
  }
  std::fill(fileOffset.begin(), fileOffset.end(), 0);
  return root;
}

//------------------------------------------------------------------------------
using ReaderType = vtkSmartPointer<vtkMultiBlockDataSet> (*)(int numberOfBuildings, int lod,
  const std::vector<std::string>& files, std::array<double, 3>& fileOffset);
std::map<std::string, ReaderType> READER = { { ".obj", ReadOBJFiles },
  { ".gml", ReadCityGMLFiles } };

//------------------------------------------------------------------------------
bool isSupported(const char* file)
{
  std::string ext = SystemTools::GetFilenameExtension(file);
  return READER.find(ext) != READER.end();
}

//------------------------------------------------------------------------------
std::vector<std::string> getFiles(const std::vector<std::string>& input)
{
  std::vector<std::string> files;
  for (const std::string& name : input)
  {
    if (SystemTools::FileExists(name.c_str(), false /*isFile*/))
    {
      if (SystemTools::FileIsDirectory(name))
      {
        // add all supported files from the directory
        vtkNew<vtkDirectory> dir;
        if (!dir->Open(name.c_str()))
        {
          vtkLog(WARNING, "Cannot open directory: " << name);
        }
        for (int i = 0; i < dir->GetNumberOfFiles(); ++i)
        {
          const char* file = dir->GetFile(i);
          if (!SystemTools::FileIsDirectory(file) && isSupported(file))
          {
            files.push_back(name + "/" + file);
          }
        }
      }
      else
      {
        files.push_back(name);
      }
    }
    else
    {
      vtkLog(WARNING, "No such file or directory: " << name);
    }
  }
  return files;
}

//------------------------------------------------------------------------------
struct Input
{
  Input()
  {
    this->Data = nullptr;
    std::fill(Offset.begin(), Offset.end(), 0);
  }
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  std::array<double, 3> Offset;
};

Input tiler(const std::vector<std::string>& input, const std::string& output, int numberOfBuildings,
  int buildingsPerTile, int lod, const std::vector<double>& inputOffset, bool saveTiles,
  bool saveTextures, std::string crs, const int utmZone, char utmHemisphere)
{
  Input ret;
  std::vector<std::string> files = getFiles(input);
  if (files.empty())
  {
    vtkLog(ERROR, "No valid input files");
    return ret;
  }
  vtkLog(INFO, "Parsing " << files.size() << " files...")

    std::array<double, 3>
      fileOffset = { { 0, 0, 0 } };
  ret.Data =
    READER[SystemTools::GetFilenameExtension(files[0])](numberOfBuildings, lod, files, fileOffset);
  std::transform(fileOffset.begin(), fileOffset.end(), inputOffset.begin(), fileOffset.begin(),
    std::plus<double>());
  ret.Offset = fileOffset;

  std::string texturePath = SystemTools::GetFilenamePath(files[0]);

  vtkNew<vtkCesium3DTilesWriter> writer;
  writer->SetInputDataObject(ret.Data);
  writer->SetDirectoryName(output.c_str());
  writer->SetTexturePath(texturePath.c_str());
  writer->SetOffset(&fileOffset[0]);
  writer->SetSaveTextures(saveTextures);
  writer->SetNumberOfBuildingsPerTile(buildingsPerTile);
  writer->SetSaveTiles(saveTiles);
  if (crs.empty())
  {
    std::ostringstream ostr;
    ostr << "+proj=utm +zone=" << utmZone << (utmHemisphere == 'S' ? "+south" : "");
    crs = ostr.str();
  }
  writer->SetCRS(crs.c_str());
  writer->Write();
  return ret;
}

bool TrianglesDiffer(std::array<std::array<double, 3>, 3>& in, std::string gltfFileName)
{
  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(gltfFileName.c_str());
  reader->Update();
  vtkMultiBlockDataSet* mbOutput = reader->GetOutput();
  auto it = vtk::TakeSmartPointer(mbOutput->NewIterator());
  vtkPolyData* output = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
  if (!output)
  {
    std::cerr << "Cannot read output data" << std::endl;
    return true;
  }
  vtkPoints* outputPoints = output->GetPoints();
  for (int i = 0; i < 3; ++i)
  {
    std::array<double, 3> outputPoint;
    outputPoints->GetPoint(i, &outputPoint[0]);
    for (size_t j = 0; j < in[i].size(); ++j)
    {
      if (!vtkMathUtilities::NearlyEqual(in[i][j], outputPoint[j], 0.001))
      {
        std::cerr << "input point: " << std::fixed << std::setprecision(16) << in[i][j]
                  << " differ than output point: " << outputPoint[j] << " at position: " << j
                  << std::endl;
        return true;
      }
    }
  }
  return false;
}

bool jsonEqual(json& l, json& r) noexcept
{
  try
  {
    if (l.is_null() && r.is_null())
    {
      return true;
    }
    else if (l.is_boolean() && r.is_boolean())
    {
      return l == r;
    }
    else if (l.is_string() && r.is_string())
    {
      return l == r;
    }
    else if (l.is_number() && r.is_number())
    {
      if (l.type() == json::value_t::number_float || r.type() == json::value_t::number_float)
      {
        return vtkMathUtilities::NearlyEqual(l.get<double>(), r.get<double>());
      }
      else
      {
        return l == r;
      }
    }
    else if (l.is_object() && r.is_object())
    {
      json::iterator itL = l.begin();
      json::iterator itR = r.begin();
      while (itL != l.end() && itR != r.end())
      {
        if (itL.key() != itR.key())
        {
          return false;
        }
        if (!jsonEqual(itL.value(), itR.value()))
        {
          return false;
        }
        ++itL;
        ++itR;
      }
      if (itL != l.end() || itR != r.end())
      {
        return false;
      }
      return true;
    }
    else if (l.is_array() && r.is_array())
    {
      json::iterator itL = l.begin();
      json::iterator itR = r.begin();
      while (itL != l.end() && itR != r.end())
      {
        if (!jsonEqual(*itL, *itR))
        {
          return false;
        }
        ++itL;
        ++itR;
      }
      if (itL != l.end() || itR != r.end())
      {
        return false;
      }
      return true;
    }
  }
  catch (json::exception& e)
  {
    std::cerr << "json::exception: " << e.what() << std::endl;
  }
  return false;
}

int TestCesium3DTilesWriter(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  if (!testHelper->IsFlagSpecified("-T"))
  {
    std::cerr << "Error: -T /path/to/temp_directory was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDirectory = testHelper->GetTempDirectory();

  auto ret =
    tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/jacksonville-triangle.obj" } },
      tempDirectory + "/jacksonville-3dtiles", 1, 1, 2, std::vector<double>{ { 0, 0, 0 } },
      true /*saveTiles*/, false /*saveTextures*/, "", 17, 'N');
  if (!ret.Data)
  {
    return EXIT_FAILURE;
  }
  std::array<std::array<double, 3>, 3> in = {
    { { { 799099.7216079829959199, -5452032.6613515587523580, 3201501.3033391013741493 } },
      { { 797899.9930383440805599, -5452124.7368548354133964, 3201444.7161126118153334 } },
      { { 797971.0970941731939092, -5452573.6701772613450885, 3200667.5626786206848919 } } }
  };
  if (TrianglesDiffer(in, tempDirectory + "/jacksonville-3dtiles/0/0.gltf"))
  {
    return EXIT_FAILURE;
  }
  std::string basefname = dataRoot + "/Data/3DTiles/jacksonville-tileset.json";
  vtksys::ifstream baselineFile(basefname.c_str());
  if (baselineFile.fail())
  {
    std::cerr << "Cannot open: " << basefname << std::endl;
    return EXIT_FAILURE;
  }
  json baseline = json::parse(baselineFile);
  std::string testfname = tempDirectory + "/jacksonville-3dtiles/tileset.json";
  vtksys::ifstream testFile(testfname.c_str());
  if (testFile.fail())
  {
    std::cerr << "Cannot open: " << testfname << std::endl;
    return EXIT_FAILURE;
  }
  json test = json::parse(testFile);
  if (!jsonEqual(baseline, test))
  {
    std::cerr << "Jacksonville data produced a different tileset than expected:" << std::endl
              << basefname << std::endl
              << testfname << std::endl;
    return EXIT_FAILURE;
  }
  ret = tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/berlin-triangle.gml" } },
    tempDirectory + "/berlin-3dtiles", 1, 1, 2, std::vector<double>{ { 0, 0, 0 } },
    true /*saveTiles*/, false /*saveTextures*/, "", 33, 'N');
  if (!ret.Data)
  {
    return EXIT_FAILURE;
  }
  in = { { { { 3782648.3888294636271894, 894381.1232001162134111, 5039949.8578473944216967 } },
    { { 3782647.9758559409528971, 894384.6010377000784501, 5039955.8512009736150503 } },
    { { 3782645.8996075680479407, 894380.4562150554265827, 5039951.8311523543670774 } } } };
  if (TrianglesDiffer(in, tempDirectory + "/berlin-3dtiles/0/0.gltf"))
  {
    return EXIT_FAILURE;
  }
  baselineFile.close();
  basefname = dataRoot + "/Data/3DTiles/berlin-tileset.json";
  baselineFile.open(basefname.c_str());
  if (baselineFile.fail())
  {
    std::cerr << "Cannot open: " << basefname << std::endl;
    return EXIT_FAILURE;
  }
  baseline = json::parse(baselineFile);
  testFile.close();
  testfname = tempDirectory + "/berlin-3dtiles/tileset.json";
  testFile.open(testfname.c_str());
  if (testFile.fail())
  {
    std::cerr << "Cannot open: " << testfname << std::endl;
    return EXIT_FAILURE;
  }
  test = json::parse(testFile);
  if (!jsonEqual(baseline, test))
  {
    std::cerr << "Berlin data produced a different tileset than expected" << std::endl
              << basefname << std::endl
              << testfname << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
