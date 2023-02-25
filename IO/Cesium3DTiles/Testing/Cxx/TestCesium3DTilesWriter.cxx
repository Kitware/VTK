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

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkCesium3DTilesWriter.h"
#include "vtkCityGMLReader.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkGLTFReader.h"
#include "vtkIncrementalOctreeNode.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOBJReader.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTesting.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <exception>
#include <set>
#include <sstream>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

using namespace vtksys;
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
  std::string textureFileName = fileNoExt + ".png";
  return SystemTools::FileExists(textureFileName, true /*isFile*/) ? textureFileName : "";
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadOBJBuildings(int numberOfBuildings, int vtkNotUsed(lod),
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
    if (!textureFileName.empty())
    {
      SetField(polyData, "texture_uri", textureFileName.c_str());
    }
    auto building = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    building->SetBlock(0, polyData);
    root->SetBlock(root->GetNumberOfBlocks(), building);
  }
  return root;
}

vtkSmartPointer<vtkPolyData> ReadOBJMesh(int numberOfBuildings, int vtkNotUsed(lod),
  const std::vector<std::string>& files, std::array<double, 3>& fileOffset)
{
  vtkNew<vtkAppendPolyData> append;
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
    append->AddInputDataObject(polyData);
  }
  append->Update();
  return append->GetOutput();
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadCityGMLBuildings(int numberOfBuildings, int lod,
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
std::map<std::string, ReaderType> READER = { { ".obj", ReadOBJBuildings },
  { ".gml", ReadCityGMLBuildings } };

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

void tiler(const std::vector<std::string>& input, int inputType, bool addColor,
  const std::string& output, bool contentGLTF, int numberOfBuildings, int buildingsPerTile, int lod,
  const std::vector<double>& inputOffset, bool saveTiles, bool saveTextures, std::string crs,
  const int utmZone, char utmHemisphere)
{
  vtkSmartPointer<vtkMultiBlockDataSet> mbData;
  vtkSmartPointer<vtkPolyData> polyData;
  std::vector<std::string> files = getFiles(input);
  if (files.empty())
  {
    throw std::runtime_error("No valid input files");
  }
  vtkLog(INFO, "Parsing " << files.size() << " files...");

  std::array<double, 3> fileOffset = { { 0, 0, 0 } };
  if (inputType == vtkCesium3DTilesWriter::Buildings || inputType == vtkCesium3DTilesWriter::Mesh)
  {
    mbData = READER[SystemTools::GetFilenameExtension(files[0])](
      numberOfBuildings, lod, files, fileOffset);
  }
  else /*Points*/
  {
    polyData = ReadOBJMesh(numberOfBuildings, lod, files, fileOffset);
    if (addColor)
    {
      vtkNew<vtkUnsignedCharArray> rgb;
      rgb->SetNumberOfComponents(3);
      rgb->SetNumberOfTuples(3);
      std::array<unsigned char, 3> a;
      a = { { 255, 0, 0 } };
      rgb->SetTypedTuple(0, a.data());
      a = { { 0, 255, 0 } };
      rgb->SetTypedTuple(1, a.data());
      a = { { 0, 0, 255 } };
      rgb->SetTypedTuple(2, a.data());
      rgb->SetName("rgb");
      polyData->GetPointData()->SetScalars(rgb);
    }
  }
  std::transform(fileOffset.begin(), fileOffset.end(), inputOffset.begin(), fileOffset.begin(),
    std::plus<double>());
  std::string textureBaseDirectory = SystemTools::GetFilenamePath(files[0]);

  vtkNew<vtkCesium3DTilesWriter> writer;
  if (inputType == vtkCesium3DTilesWriter::Buildings || inputType == vtkCesium3DTilesWriter::Mesh)
  {
    writer->SetInputDataObject(mbData);
  }
  else
  {
    writer->SetInputDataObject(polyData);
  }
  writer->SetContentGLTF(contentGLTF);
  writer->SetInputType(inputType);
  writer->SetDirectoryName(output.c_str());
  writer->SetTextureBaseDirectory(textureBaseDirectory.c_str());
  writer->SetOffset(fileOffset.data());
  writer->SetSaveTextures(saveTextures);
  writer->SetNumberOfFeaturesPerTile(buildingsPerTile);
  writer->SetSaveTiles(saveTiles);
  if (crs.empty())
  {
    std::ostringstream ostr;
    ostr << "+proj=utm +zone=" << utmZone << (utmHemisphere == 'S' ? "+south" : "");
    crs = ostr.str();
  }
  writer->SetCRS(crs.c_str());
  writer->Write();
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
    outputPoints->GetPoint(i, outputPoint.data());
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

bool JsonEqual(nlohmann::json& l, nlohmann::json& r) noexcept
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
      if (l.type() == nlohmann::json::value_t::number_float ||
        r.type() == nlohmann::json::value_t::number_float)
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
      nlohmann::json::iterator itL = l.begin();
      nlohmann::json::iterator itR = r.begin();
      while (itL != l.end() && itR != r.end())
      {
        if (itL.key() != itR.key())
        {
          return false;
        }
        if (!JsonEqual(itL.value(), itR.value()))
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
      nlohmann::json::iterator itL = l.begin();
      nlohmann::json::iterator itR = r.begin();
      while (itL != l.end() && itR != r.end())
      {
        if (!JsonEqual(*itL, *itR))
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
  catch (nlohmann::json::exception& e)
  {
    std::cerr << "json::exception: " << e.what() << std::endl;
  }
  return false;
}

std::array<std::array<double, 3>, 3> triangleJacksonville = {
  { { { 799099.7216079829959199, -5452032.6613515587523580, 3201501.3033391013741493 } },
    { { 797899.9930383440805599, -5452124.7368548354133964, 3201444.7161126118153334 } },
    { { 797971.0970941731939092, -5452573.6701772613450885, 3200667.5626786206848919 } } }
};

nlohmann::json ReadTileset(const std::string& fileName)
{
  vtksys::ifstream fileStream(fileName.c_str());
  if (fileStream.fail())
  {
    std::ostringstream ostr;
    ostr << "Cannot open: " << fileName << std::endl;
    throw std::runtime_error(ostr.str());
  }
  nlohmann::json tilesetJson = nlohmann::json::parse(fileStream);
  return tilesetJson;
}

void TestJacksonvilleBuildings(const std::string& dataRoot, const std::string& tempDirectory)
{
  std::cout << "Test jacksonville buildings" << std::endl;
  tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/jacksonville-triangle.obj" } },
    vtkCesium3DTilesWriter::Buildings, false /*addColor*/, tempDirectory + "/jacksonville-3dtiles",
    true /*contentGLTF*/, 1, 1, 2, std::vector<double>{ { 0, 0, 0 } }, true /*saveTiles*/,
    false /*saveTextures*/, "", 17, 'N');
  std::string gltfFile = tempDirectory + "/jacksonville-3dtiles/0/0.gltf";
  if (TrianglesDiffer(triangleJacksonville, gltfFile))
  {
    throw std::runtime_error("Triangles differ: " + gltfFile);
  }
  std::string baselineFile = dataRoot + "/Data/3DTiles/jacksonville-tileset.json";
  std::string testFile = tempDirectory + "/jacksonville-3dtiles/tileset.json";
  nlohmann::json baseline = ReadTileset(baselineFile);
  nlohmann::json test = ReadTileset(testFile);
  if (!JsonEqual(baseline, test))
  {
    std::ostringstream ostr;
    ostr << "Error: different tileset than expected:" << std::endl
         << baselineFile << std::endl
         << testFile << std::endl;
    throw std::runtime_error(ostr.str());
  }
}

void TestJacksonvillePoints(
  const std::string& dataRoot, const std::string& tempDirectory, bool contentGLTF)
{
  std::string destDir =
    tempDirectory + "/jacksonville-3dtiles-points-" + (contentGLTF ? "gltf" : "pnts");
  std::cout << "Test jacksonville points " << (contentGLTF ? "gltf" : "pnts") << std::endl;
  tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/jacksonville-triangle.obj" } },
    vtkCesium3DTilesWriter::Points, false /*addColor*/, destDir, contentGLTF, 3, 3, 2,
    std::vector<double>{ { 0, 0, 0 } }, true /*saveTiles*/, false /*saveTextures*/, "", 17, 'N');
  std::string gltfFile = tempDirectory + "/jacksonville-3dtiles-points-gltf/0/0.gltf";
  if (contentGLTF && TrianglesDiffer(triangleJacksonville, gltfFile))
  {
    throw std::runtime_error("Triangles differ: " + gltfFile);
  }
}

void TestJacksonvilleColorPoints(
  const std::string& dataRoot, const std::string& tempDirectory, bool contentGLTF)
{
  std::string destDir =
    tempDirectory + "/jacksonville-3dtiles-colorpoints-" + (contentGLTF ? "gltf" : "pnts");
  std::cout << "Test jacksonville color points " << (contentGLTF ? "gltf" : "pnts") << std::endl;
  tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/jacksonville-triangle.obj" } },
    vtkCesium3DTilesWriter::Points, true /*addColor*/, destDir, contentGLTF, 3, 3, 2,
    std::vector<double>{ { 0, 0, 0 } }, true /*saveTiles*/, false /*saveTextures*/, "", 17, 'N');
  std::string gltfFile = tempDirectory + "/jacksonville-3dtiles-colorpoints-gltf/0/0.gltf";
  if (contentGLTF && TrianglesDiffer(triangleJacksonville, gltfFile))
  {
    throw std::runtime_error("Triangles differ: " + gltfFile);
  }
}

void TestJacksonvilleMesh(const std::string& dataRoot, const std::string& tempDirectory)
{
  std::string destDir = tempDirectory + "/jacksonville-3dtiles-mesh";
  std::cout << "Test jacksonville mesh" << std::endl;
  tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/jacksonville-triangle.obj" } },
    vtkCesium3DTilesWriter::Mesh, false /*addColor*/, destDir, true /*contentGLTF*/, 3, 3, 2,
    std::vector<double>{ { 0, 0, 0 } }, true /*saveTiles*/, false /*saveTextures*/, "", 17, 'N');
  std::string gltfFile = tempDirectory + "/jacksonville-3dtiles-mesh/0/0.gltf";
  if (TrianglesDiffer(triangleJacksonville, gltfFile))
  {
    throw std::runtime_error("Triangles differ: " + gltfFile);
  }
}

void TestBerlinBuildings(const std::string& dataRoot, const std::string& tempDirectory)
{
  std::array<std::array<double, 3>, 3> in;
  std::cout << "Test berlin buildings (citygml)" << std::endl;
  tiler(std::vector<std::string>{ { dataRoot + "/Data/3DTiles/berlin-triangle.gml" } },
    vtkCesium3DTilesWriter::Buildings, false /*addColor*/, tempDirectory + "/berlin-3dtiles",
    true /*contentGLTF*/, 1, 1, 2, std::vector<double>{ { 0, 0, 0 } }, true /*saveTiles*/,
    false /*saveTextures*/, "", 33, 'N');
  in = { { { { 3782648.3888294636271894, 894381.1232001162134111, 5039949.8578473944216967 } },
    { { 3782647.9758559409528971, 894384.6010377000784501, 5039955.8512009736150503 } },
    { { 3782645.8996075680479407, 894380.4562150554265827, 5039951.8311523543670774 } } } };
  if (TrianglesDiffer(in, tempDirectory + "/berlin-3dtiles/0/0.gltf"))
  {
    throw std::runtime_error("Triangles differ failure");
  }
  std::string basefname = dataRoot + "/Data/3DTiles/berlin-tileset.json";
  nlohmann::json baseline = ReadTileset(basefname);
  std::string testfname = tempDirectory + "/berlin-3dtiles/tileset.json";
  nlohmann::json test = ReadTileset(testfname);
  if (!JsonEqual(baseline, test))
  {
    std::ostringstream ostr;
    ostr << "Error: different tileset than expected" << std::endl
         << basefname << std::endl
         << testfname << std::endl;
    throw std::runtime_error(ostr.str());
  }
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
  try
  {
    TestJacksonvilleBuildings(dataRoot, tempDirectory);
    TestBerlinBuildings(dataRoot, tempDirectory);

    TestJacksonvillePoints(dataRoot, tempDirectory, false /*contentGLTF*/);
    TestJacksonvillePoints(dataRoot, tempDirectory, true /*contentGLTF*/);
    TestJacksonvilleColorPoints(dataRoot, tempDirectory, false /*contentGLTF*/);
    TestJacksonvilleColorPoints(dataRoot, tempDirectory, true /*contentGLTF*/);

    TestJacksonvilleMesh(dataRoot, tempDirectory);
  }
  catch (std::runtime_error& e)
  {
    vtkLog(ERROR, << e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
