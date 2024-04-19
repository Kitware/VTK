// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkJSONSceneExporter.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkArchiver.h"
#include "vtkCamera.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkExporter.h"
#include "vtkImageData.h"
#include "vtkImageResize.h"
#include "vtkJPEGWriter.h"
#include "vtkJSONDataSetWriter.h"
#include "vtkMapper.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkMoleculeToAtomBallFilter.h"
#include "vtkMoleculeToBondStickFilter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataNormals.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkQuadricClustering.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkScalarsToColors.h"
#include "vtkTexture.h"
#include "vtkVolume.h"
#include "vtkVolumeCollection.h"
#include "vtkVolumeProperty.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkJSONSceneExporter);

//------------------------------------------------------------------------------

vtkJSONSceneExporter::vtkJSONSceneExporter()
{
  this->FileName = nullptr;
  this->WriteTextures = false;
  this->WriteTextureLODs = false;
  this->TextureLODsBaseSize = 100000;
  this->TextureLODsBaseUrl = nullptr;
  this->WritePolyLODs = false;
  this->PolyLODsBaseSize = 100000;
  this->PolyLODsBaseUrl = nullptr;
}

//------------------------------------------------------------------------------

vtkJSONSceneExporter::~vtkJSONSceneExporter()
{
  this->SetFileName(nullptr);
  this->SetTextureLODsBaseUrl(nullptr);
  this->SetPolyLODsBaseUrl(nullptr);
}

//------------------------------------------------------------------------------

void vtkJSONSceneExporter::WriteDataObject(
  ostream& os, vtkDataObject* dataObject, vtkActor* actor = nullptr, vtkVolume* volume = nullptr)
{
  // Skip if nothing to process
  if (dataObject == nullptr)
  {
    return;
  }

  // Handle Dataset
  if (dataObject->IsA("vtkDataSet"))
  {
    std::string texturesString;
    std::string renderingSetup;

    if (actor)
    {
      if (this->WriteTextures && actor->GetTexture())
      {
        // Write out the textures, add it to the textures string
        texturesString += this->WriteTexture(actor->GetTexture());
      }

      if (this->WriteTextureLODs && actor->GetTexture())
      {
        // Write out the texture LODs, add it to the textures string
        texturesString += this->WriteTextureLODSeries(actor->GetTexture());
      }
      renderingSetup = this->ExtractActorRenderingSetup(actor);
    }
    else if (volume)
    {
      renderingSetup = this->ExtractVolumeRenderingSetup(volume);
    }
    std::string addOnMeta = renderingSetup + texturesString + "\n";
    std::string dsMeta =
      this->WriteDataSet(vtkDataSet::SafeDownCast(dataObject), addOnMeta.c_str());
    if (!dsMeta.empty())
    {
      os << dsMeta;
    }
    return;
  }

  // Handle composite
  if (dataObject->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(dataObject);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(composite->NewIterator());
    iter->SkipEmptyNodesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      this->WriteDataObject(os, iter->GetCurrentDataObject(), actor, volume);
      iter->GoToNextItem();
    }

    return;
  }

  // Handle molecule
  if (dataObject->IsA("vtkMolecule"))
  {
    vtkMolecule* molecule = vtkMolecule::SafeDownCast(dataObject);

    // Create tubes for each bond
    vtkNew<vtkMoleculeToBondStickFilter> stickFilter;
    stickFilter->SetInputDataObject(molecule);
    stickFilter->Update();

    // Create spheres for each atom
    vtkNew<vtkMoleculeToAtomBallFilter> ballFilter;
    ballFilter->SetInputDataObject(molecule);

    if (actor)
    {
      // Retrieve radius type and scale factor from mapper
      vtkMoleculeMapper* mapper = vtkMoleculeMapper::SafeDownCast(actor->GetMapper());

      switch (mapper->GetAtomicRadiusType())
      {
        case vtkMoleculeMapper::CovalentRadius:
        {
          ballFilter->SetRadiusSource(vtkMoleculeToAtomBallFilter::CovalentRadius);
          break;
        }
        case vtkMoleculeMapper::VDWRadius:
        {
          ballFilter->SetRadiusSource(vtkMoleculeToAtomBallFilter::VDWRadius);
          break;
        }
        case vtkMoleculeMapper::UnitRadius:
        {
          ballFilter->SetRadiusSource(vtkMoleculeToAtomBallFilter::UnitRadius);
          break;
        }
        default:
        {
          // Default to Van Der Waals
          ballFilter->SetRadiusSource(vtkMoleculeToAtomBallFilter::VDWRadius);
          break;
        }
      }

      ballFilter->SetRadiusScale(mapper->GetAtomicRadiusScaleFactor());
    }
    else
    {
      // Set default radius type and scale
      ballFilter->SetRadiusSource(vtkMoleculeToAtomBallFilter::VDWRadius);
      ballFilter->SetRadiusScale(0.3);
    }

    // Reduce resolution when the number of atoms is high
    // The threshold value has been arbitrarily chosen
    if (molecule->GetNumberOfAtoms() > 100)
    {
      ballFilter->SetResolution(20);
    }

    // Create vertex normals for a smoother appearance
    vtkNew<vtkPolyDataNormals> normalFilter;
    normalFilter->SetInputConnection(ballFilter->GetOutputPort());
    normalFilter->Update();

    // Write tubes and spheres
    this->WriteDataObject(os, stickFilter->GetOutput(), actor, volume);
    this->WriteDataObject(os, normalFilter->GetOutput(), actor, volume);
  }
}

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::ExtractColorTransferFunctionSetup(
  vtkColorTransferFunction* function)
{
  std::stringstream configuration;

  bool useAboveRangeColor = function->GetUseAboveRangeColor();
  bool useBelowRangeColor = function->GetUseBelowRangeColor();
  int colorSpace = function->GetColorSpace();
  double aboveRangeColor[3] = { 0 };
  double belowRangeColor[3] = { 0 };
  double nanColor[3] = { 0 };
  function->GetAboveRangeColor(aboveRangeColor);
  function->GetBelowRangeColor(belowRangeColor);
  function->GetNanColor(nanColor);

  vtkIdType numberOfNodes = function->GetSize();
  constexpr const char* INDENT = "            ";
  configuration << INDENT << "  \"useAboveRangeColor\": " << (useAboveRangeColor ? "true" : "false")
                << ",\n"
                << INDENT << "  \"useBelowRangeColor\": " << (useBelowRangeColor ? "true" : "false")
                << ",\n"
                << INDENT << "  \"colorSpace\": " << colorSpace << ",\n";
  if (useAboveRangeColor)
  {
    configuration << INDENT << "  \"aboveRangeColor\": [" << aboveRangeColor[0] << ", "
                  << aboveRangeColor[1] << ", " << aboveRangeColor[2] << "],\n";
  }
  if (useBelowRangeColor)
  {
    configuration << INDENT << "  \"belowRangeColor\": [" << belowRangeColor[0] << ", "
                  << belowRangeColor[1] << ", " << belowRangeColor[2] << "],\n";
  }
  configuration << INDENT << "  \"nanColor\": [" << nanColor[0] << ", " << nanColor[1] << ", "
                << nanColor[2] << "],\n";
  configuration << INDENT << "  \"nodes\": [\n";
  for (vtkIdType nodeId = 0; nodeId < numberOfNodes; ++nodeId)
  {
    double node[6];
    function->GetNodeValue(nodeId, node);
    configuration << INDENT << "    [";
    for (int i = 0; i < 6; ++i)
    {
      configuration << node[i] << (i == 5 ? "]" : ", ");
    }
    if (nodeId < numberOfNodes - 1)
    {
      configuration << ",";
    }
    configuration << "\n";
  }
  configuration << INDENT << "  ]\n";
  return configuration.str();
}

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::ExtractPiecewiseFunctionSetup(vtkPiecewiseFunction* function)
{
  bool clamping = function->GetClamping();
  vtkIdType numberOfPoints = function->GetSize();
  constexpr const char* INDENT = "            ";
  std::stringstream configuration;
  configuration << INDENT << "  \"clamping\": " << (clamping ? "true" : "false") << ",\n";
  configuration << INDENT << "  \"points\": [\n";
  for (vtkIdType pointId = 0; pointId < numberOfPoints; ++pointId)
  {
    double point[4];
    function->GetNodeValue(pointId, point);
    configuration << INDENT << "    [";
    for (int i = 0; i < 4; ++i)
    {
      configuration << point[i] << (i < 3 ? ", " : "");
    }
    configuration << "]";
    if (pointId < numberOfPoints - 1)
    {
      configuration << ",";
    }
    configuration << "\n";
  }
  configuration << INDENT << "  ]\n";
  return configuration.str();
}

//------------------------------------------------------------------------------
std::string vtkJSONSceneExporter::ExtractVolumeRenderingSetup(vtkVolume* volume)
{
  vtkVolumeProperty* property = volume->GetProperty();

  double* p3dPosition = volume->GetPosition();
  double* p3dScale = volume->GetScale();
  double* p3dOrigin = volume->GetOrigin();
  double* p3dRotateWXYZ = volume->GetOrientationWXYZ();

  int interpolationType = property->GetInterpolationType();

  vtkTypeBool independentComponents = property->GetIndependentComponents();
  int shade = property->GetShade();
  double ambient = property->GetAmbient();
  double diffuse = property->GetDiffuse();
  double specular = property->GetSpecular();
  double specularPower = property->GetSpecularPower();

  constexpr const char* INDENT = "      ";
  std::stringstream renderingConfig;
  renderingConfig << ",\n"
                  << "\"volume\": {\n"
                  << INDENT << "  \"origin\": [" << p3dOrigin[0] << ", " << p3dOrigin[1] << ", "
                  << p3dOrigin[2] << "],\n"
                  << INDENT << "  \"scale\": [" << p3dScale[0] << ", " << p3dScale[1] << ", "
                  << p3dScale[2] << "],\n"
                  << INDENT << "  \"position\": [" << p3dPosition[0] << ", " << p3dPosition[1]
                  << ", " << p3dPosition[2] << "]\n"
                  << INDENT << "},\n"
                  << INDENT << "\"volumeRotation\": [" << p3dRotateWXYZ[0] << ", "
                  << p3dRotateWXYZ[1] << ", " << p3dRotateWXYZ[2] << ", " << p3dRotateWXYZ[3]
                  << "],\n"
                  << INDENT << "\"mapper\": {},\n"
                  << INDENT << "\"property\": {\n"
                  << INDENT << "  \"interpolationType\": " << interpolationType << ",\n"
                  << INDENT
                  << "  \"independentComponents\": " << (independentComponents ? "true" : "false")
                  << ",\n"
                  << INDENT << "  \"ambient\": " << ambient << ",\n"
                  << INDENT << "  \"diffuse\": " << diffuse << ",\n"
                  << INDENT << "  \"specular\": " << specular << ",\n"
                  << INDENT << "  \"specularPower\": " << specularPower << ",\n"
                  << INDENT << "  \"shade\": " << shade << ",\n"
                  << INDENT << "  \"components\": [\n";
  for (vtkIdType component = 0; component < VTK_MAX_VRCOMP; ++component)
  {
    renderingConfig << INDENT << "  {\n";
    int colorChannels = property->GetColorChannels(component);
    renderingConfig << INDENT << "    \"colorChannels\": " << colorChannels << ",\n";
    if (colorChannels == 3)
    {
      renderingConfig << INDENT << "    \"rgbTransferFunction\":\n"
                      << INDENT << "    {\n"
                      << this->ExtractColorTransferFunctionSetup(
                           property->GetRGBTransferFunction(component))
                      << INDENT << "    },\n";
    }
    else if (colorChannels == 1)
    {
      renderingConfig << INDENT << "    \"grayTransferFunction\":\n"
                      << INDENT << "    {\n"
                      << this->ExtractPiecewiseFunctionSetup(
                           property->GetGrayTransferFunction(component))
                      << INDENT << "    },\n";
    }
    renderingConfig << INDENT << "    \"scalarOpacity\":\n"
                    << INDENT << "    {\n"
                    << this->ExtractPiecewiseFunctionSetup(property->GetScalarOpacity(component))
                    << INDENT << "    },\n";
    double scalarOpacityUnitDistance = property->GetScalarOpacityUnitDistance(component);
    renderingConfig << INDENT << "    \"scalarOpacityUnitDistance\": " << scalarOpacityUnitDistance
                    << "\n"
                    << INDENT << "  }";
    if (component < VTK_MAX_VRCOMP - 1)
    {
      renderingConfig << ",";
    }
    renderingConfig << "\n";
  }
  renderingConfig << INDENT << "  ]\n" << INDENT << "}\n";
  return renderingConfig.str();
}

std::string vtkJSONSceneExporter::ExtractActorRenderingSetup(vtkActor* actor)
{
  vtkMapper* mapper = actor->GetMapper();
  // int scalarVisibility = mapper->GetScalarVisibility();
  const char* colorArrayName = mapper->GetArrayName();
  int colorMode = mapper->GetColorMode();
  int scalarMode = mapper->GetScalarMode();

  vtkProperty* property = actor->GetProperty();
  int representation = property->GetRepresentation();
  double* colorToUse = property->GetDiffuseColor();
  if (representation == 1)
  {
    colorToUse = property->GetColor();
  }
  int pointSize = property->GetPointSize();
  float opacity = property->GetOpacity();
  int edgeVisibility = property->GetEdgeVisibility();

  double* p3dPosition = actor->GetPosition();
  double* p3dScale = actor->GetScale();
  double* p3dOrigin = actor->GetOrigin();
  double* p3dRotateWXYZ = actor->GetOrientationWXYZ();

  constexpr const char* INDENT = "      ";
  std::stringstream renderingConfig;
  renderingConfig << ",\n"
                  << INDENT << "\"actor\": {\n"
                  << INDENT << "  \"origin\": [" << p3dOrigin[0] << ", " << p3dOrigin[1] << ", "
                  << p3dOrigin[2] << "],\n"
                  << INDENT << "  \"scale\": [" << p3dScale[0] << ", " << p3dScale[1] << ", "
                  << p3dScale[2] << "],\n"
                  << INDENT << "  \"position\": [" << p3dPosition[0] << ", " << p3dPosition[1]
                  << ", " << p3dPosition[2] << "]\n"
                  << INDENT << "},\n"
                  << INDENT << "\"actorRotation\": [" << p3dRotateWXYZ[0] << ", "
                  << p3dRotateWXYZ[1] << ", " << p3dRotateWXYZ[2] << ", " << p3dRotateWXYZ[3]
                  << "],\n"
                  << INDENT << "\"mapper\": {\n"
                  << INDENT << "  \"colorByArrayName\": \"" << colorArrayName << "\",\n"
                  << INDENT << "  \"colorMode\": " << colorMode << ",\n"
                  << INDENT << "  \"scalarMode\": " << scalarMode << "\n"
                  << INDENT << "},\n"
                  << INDENT << "\"property\": {\n"
                  << INDENT << "  \"representation\": " << representation << ",\n"
                  << INDENT << "  \"edgeVisibility\": " << edgeVisibility << ",\n"
                  << INDENT << "  \"diffuseColor\": [" << colorToUse[0] << ", " << colorToUse[1]
                  << ", " << colorToUse[2] << "],\n"
                  << INDENT << "  \"pointSize\": " << pointSize << ",\n"
                  << INDENT << "  \"opacity\": " << opacity << "\n"
                  << INDENT << "}";

  return renderingConfig.str();
}

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::GetTemporaryPath() const
{
  return std::string(this->FileName) + ".pvtmp";
}

std::string vtkJSONSceneExporter::CurrentDataSetPath() const
{
  std::stringstream path;
  path << this->GetTemporaryPath() << "/" << this->DatasetCount + 1;
  return vtksys::SystemTools::ConvertToOutputPath(path.str());
}

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::WriteDataSet(vtkDataSet* dataset, const char* addOnMeta = nullptr)
{
  if (!dataset)
  {
    return "";
  }

  std::string dsPath = this->CurrentDataSetPath();
  ++this->DatasetCount;

  auto* polyData = vtkPolyData::SafeDownCast(dataset);
  vtkSmartPointer<vtkPolyData> newDataset;
  std::string polyLODsConfig;
  if (this->WritePolyLODs && polyData)
  {
    newDataset = this->WritePolyLODSeries(polyData, polyLODsConfig);
    // Write the smallest poly LOD to the vtkjs file
    dataset = newDataset.Get();
  }

  vtkNew<vtkJSONDataSetWriter> dsWriter;
  dsWriter->SetInputData(dataset);
  dsWriter->GetArchiver()->SetArchiveName(dsPath.c_str());
  dsWriter->Write();

  if (!dsWriter->IsDataSetValid())
  {
    this->DatasetCount--;
    return "";
  }

  std::stringstream meta;
  if (this->DatasetCount > 1)
  {
    meta << ",\n";
  }
  else
  {
    meta << "\n";
  }
  constexpr const char* INDENT = "    ";
  meta << INDENT << "{\n"
       << INDENT << "  \"name\": \"" << this->DatasetCount << "\",\n"
       << INDENT << "  \"type\": \"vtkHttpDataSetReader\",\n"
       << INDENT << "  \"vtkHttpDataSetReader\": { \"url\": \"" << this->DatasetCount << "\" }";

  if (addOnMeta != nullptr)
  {
    meta << addOnMeta;
  }

  meta << polyLODsConfig;
  meta << INDENT << "}";

  return meta.str();
}

//------------------------------------------------------------------------------

void vtkJSONSceneExporter::WriteLookupTable(const char* name, vtkScalarsToColors* lookupTable)
{
  if (lookupTable == nullptr)
  {
    return;
  }

  vtkDiscretizableColorTransferFunction* dctfn =
    vtkDiscretizableColorTransferFunction::SafeDownCast(lookupTable);
  if (dctfn != nullptr)
  {
    constexpr const char* INDENT = "    ";
    std::stringstream lutJSON;
    lutJSON << "{\n"
            << INDENT << "  \"clamping\": " << (dctfn->GetClamping() ? "true" : "false") << ",\n"
            << INDENT << "  \"colorSpace\": " << dctfn->GetColorSpace() << ",\n"
            << INDENT << "  \"hSVWrap\": " << (dctfn->GetHSVWrap() ? "true" : "false") << ",\n"
            << INDENT << "  \"alpha\": " << dctfn->GetAlpha() << ",\n"
            << INDENT << "  \"vectorComponent\": " << dctfn->GetVectorComponent() << ",\n"
            << INDENT << "  \"vectorSize\": " << dctfn->GetVectorSize() << ",\n"
            << INDENT << "  \"vectorMode\": " << dctfn->GetVectorMode() << ",\n"
            << INDENT << "  \"indexedLookup\": " << dctfn->GetIndexedLookup() << ",\n"
            << INDENT << "  \"nodes\": [";

    // Fill nodes
    vtkIdType nbNodes = dctfn->GetSize();
    double node[6];
    for (vtkIdType i = 0; i < nbNodes; i++)
    {
      dctfn->GetNodeValue(i, node);
      if (i > 0)
      {
        lutJSON << ",";
      }

      lutJSON << "\n"
              << INDENT << INDENT << "[" << node[0] << ", " << node[1] << ", " << node[2] << ", "
              << node[3] << ", " << node[4] << ", " << node[5] << "]";
    }

    // Close node list
    lutJSON << "\n      ]\n    }";

    // Store in map...
    this->LookupTables[name] = lutJSON.str();
  }
}

//------------------------------------------------------------------------------
void vtkJSONSceneExporter::WritePropCollection(
  vtkPropCollection* props, std::ostream& sceneComponents)
{
  vtkIdType nbProps = props->GetNumberOfItems();
  for (vtkIdType rpIdx = 0; rpIdx < nbProps; rpIdx++)
  {
    vtkProp* prop = vtkProp::SafeDownCast(props->GetItemAsObject(rpIdx));
    // Skip non-visible actors
    if (!prop || !prop->GetVisibility())
    {
      continue;
    }

    // Skip actors with no geometry
    vtkActor* actor = vtkActor::SafeDownCast(prop);
    if (actor)
    {
      vtkMapper* mapper = actor->GetMapper();

      vtkDataObject* dataObject = mapper->GetInputDataObject(0, 0);
      this->WriteDataObject(sceneComponents, dataObject, actor);
      this->WriteLookupTable(mapper->GetArrayName(), mapper->GetLookupTable());
    }
  }
}

//------------------------------------------------------------------------------
void vtkJSONSceneExporter::WriteVolumeCollection(
  vtkVolumeCollection* volumes, std::ostream& sceneComponents)
{
  vtkVolume* volume = nullptr;
  volumes->InitTraversal();
  while ((volume = volumes->GetNextVolume()))
  {
    // Skip non-visible actors
    if (!volume || !volume->GetVisibility())
    {
      continue;
    }

    vtkAbstractVolumeMapper* mapper = volume->GetMapper();
    vtkDataObject* dataObject = mapper->GetInputDataObject(0, 0);
    this->WriteDataObject(sceneComponents, dataObject, nullptr, volume);
  }
}

//------------------------------------------------------------------------------
void vtkJSONSceneExporter::WriteData()
{
  this->DatasetCount = 0;
  this->TextureStrings.clear();
  this->TextureLODStrings.clear();
  this->FilesToZip.clear();

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  std::string tmpPath = this->GetTemporaryPath();

  if (!vtksys::SystemTools::MakeDirectory(tmpPath))
  {
    vtkErrorMacro(<< "Cannot create directory " << tmpPath);
    return;
  }

  vtkRenderer* renderer = this->GetActiveRenderer();
  if (!renderer)
  {
    renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }
  vtkCamera* cam = renderer->GetActiveCamera();

  std::stringstream sceneComponents;
  vtkPropCollection* renProps = renderer->GetViewProps();

  this->WritePropCollection(renProps, sceneComponents);
  this->WriteVolumeCollection(renderer->GetVolumes(), sceneComponents);

  std::stringstream sceneJsonFile;
  sceneJsonFile << "{\n"
                << "  \"version\": 1.0,\n"
                << "  \"background\": [" << renderer->GetBackground()[0] << ", "
                << renderer->GetBackground()[1] << ", " << renderer->GetBackground()[2] << "],\n"
                << "  \"camera\": {\n"
                << "    \"focalPoint\": [" << cam->GetFocalPoint()[0] << ", "
                << cam->GetFocalPoint()[1] << ", " << cam->GetFocalPoint()[2] << "],\n"
                << "    \"position\": [" << cam->GetPosition()[0] << ", " << cam->GetPosition()[1]
                << ", " << cam->GetPosition()[2] << "],\n"
                << "    \"viewUp\": [" << cam->GetViewUp()[0] << ", " << cam->GetViewUp()[1] << ", "
                << cam->GetViewUp()[2] << "]\n"
                << "  },\n"
                << "  \"centerOfRotation\": [" << cam->GetFocalPoint()[0] << ", "
                << cam->GetFocalPoint()[1] << ", " << cam->GetFocalPoint()[2] << "],\n"
                << "  \"scene\": [" << sceneComponents.str() << "\n  ],\n"
                << "  \"lookupTables\": {\n";

  // Inject lookup table
  size_t nbLuts = this->LookupTables.size();
  for (auto const& lut : this->LookupTables)
  {
    sceneJsonFile << "    \"" << lut.first << "\": " << lut.second << (--nbLuts ? "," : "") << "\n";
  }

  sceneJsonFile << "  }\n"
                << "}\n";

  // Write meta-data file
  std::stringstream scenePath;
  scenePath << tmpPath << "/index.json";

  vtksys::ofstream file;
  file.open(scenePath.str().c_str(), ios::out);
  file << sceneJsonFile.str();
  file.close();

  if (vtksys::SystemTools::FileExists(this->FileName))
  {
    vtksys::SystemTools::RemoveFile(this->FileName);
  }

  int result = std::rename(tmpPath.c_str(), this->FileName);

  if (result != 0)
  {
    vtkErrorMacro("Cannot rename temporary file.");
    return;
  }
}

namespace
{

//------------------------------------------------------------------------------

size_t getFileSize(const std::string& path)
{
  // TODO: This function gives me slightly different sizes than what my
  // filesystem gives me. Find out why.
  // For instance, I get 240MB for a 230MB file.
  // Maybe we can say "it's close enough" for now, though...
  vtksys::SystemTools::Stat_t stat_buf;
  int res = vtksys::SystemTools::Stat(path, &stat_buf);
  if (res < 0)
  {
    std::cerr << "Failed to get size of file " << path << std::endl;
    return 0;
  }

  return stat_buf.st_size;
}

} // end anon namespace

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::WriteTexture(vtkTexture* texture)
{
  // If this texture has already been written, just re-use the one
  // we have.
  if (this->TextureStrings.find(texture) != this->TextureStrings.end())
  {
    return this->TextureStrings[texture];
  }

  std::string path = this->CurrentDataSetPath();

  // Make sure it exists
  if (!vtksys::SystemTools::MakeDirectory(path))
  {
    vtkErrorMacro(<< "Cannot create directory " << path);
    return "";
  }

  path += "/texture.jpg";
  path = vtksys::SystemTools::ConvertToOutputPath(path);

  vtkSmartPointer<vtkImageData> image = texture->GetInput();

  vtkNew<vtkJPEGWriter> writer;
  writer->SetFileName(path.c_str());
  writer->SetInputDataObject(image);
  writer->Write();

  constexpr const char* INDENT = "      ";
  std::stringstream config;
  config << ",\n" << INDENT << "\"texture\": \"" << this->DatasetCount + 1 << "/texture.jpg\"";
  this->TextureStrings[texture] = config.str();
  return config.str();
}

//------------------------------------------------------------------------------

std::string vtkJSONSceneExporter::WriteTextureLODSeries(vtkTexture* texture)
{
  // If this texture has already been written, just re-use the one
  // we have.
  if (this->TextureLODStrings.find(texture) != this->TextureLODStrings.end())
  {
    return this->TextureLODStrings[texture];
  }

  std::vector<std::string> files;

  std::string name = "texture";
  std::string ext = ".jpg";

  vtkSmartPointer<vtkImageData> image = texture->GetInput();
  int dims[3];
  image->GetDimensions(dims);

  // Write these into the parent directory of our file.
  // This next line also converts the path to unix slashes.
  std::string path = vtksys::SystemTools::GetParentDirectory(this->GetTemporaryPath());
  path += "/";
  path = vtksys::SystemTools::ConvertToOutputPath(path);

  while (true)
  {
    // Name is "<name>_<dataset_number>-<width>x<height><ext>"
    // For example, "texture_1-256x256.jpg"
    std::stringstream full_name;
    full_name << name << "_" << std::to_string(this->DatasetCount + 1) << "-"
              << std::to_string(dims[0]) << "x" << std::to_string(dims[1]) << ext;
    std::string full_path = path + full_name.str();

    vtkNew<vtkJPEGWriter> writer;
    writer->SetFileName(full_path.c_str());
    writer->SetInputDataObject(image);
    writer->Write();

    files.push_back(full_name.str());

    size_t size = getFileSize(full_path);
    if (size <= this->TextureLODsBaseSize || (dims[0] == 1 && dims[1] == 1))
    {
      // We are done...
      break;
    }

    // Shrink the image and go again
    vtkNew<vtkImageResize> shrink;
    shrink->SetInputData(image);
    dims[0] = dims[0] > 1 ? dims[0] / 2 : 1;
    dims[1] = dims[1] > 1 ? dims[1] / 2 : 1;
    shrink->SetOutputDimensions(dims[0], dims[1], 1);
    shrink->Update();
    image = shrink->GetOutput();
  }

  const char* url = this->TextureLODsBaseUrl;
  std::string baseUrl = url ? url : "";

  // Now, write out the config
  constexpr const char* INDENT = "      ";
  std::stringstream config;
  config << ",\n"
         << INDENT << "\"textureLODs\": {\n"
         << INDENT << "  \"baseUrl\": \"" << baseUrl << "\",\n"
         << INDENT << "  \"files\": [\n";

  // Reverse the order of the files so the smallest comes first
  std::reverse(files.begin(), files.end());
  for (size_t i = 0; i < files.size(); ++i)
  {
    config << INDENT << "    \"" << files[i] << "\"";
    if (i != files.size() - 1)
    {
      config << ",\n";
    }
    else
    {
      config << "\n";
    }
  }

  config << INDENT << "  ]\n" << INDENT << "}";

  this->TextureLODStrings[texture] = config.str();
  return config.str();
}

//------------------------------------------------------------------------------

vtkSmartPointer<vtkPolyData> vtkJSONSceneExporter::WritePolyLODSeries(
  vtkPolyData* dataset, std::string& polyLODsConfig)
{
  vtkSmartPointer<vtkPolyData> polyData = dataset;
  std::vector<std::string> files;

  // Write these into the parent directory of our file.
  // This next line also converts the path to unix slashes.
  vtkNew<vtkJSONDataSetWriter> dsWriter;
  std::string path = vtksys::SystemTools::GetParentDirectory(this->GetTemporaryPath()) + "/";
  path = vtksys::SystemTools::ConvertToOutputPath(path);

  // If the new size is not at least 5% different from the old size,
  // stop writing out the LODs, because the difference is too small.
  size_t previousDataSize = 0;
  double minDiffFraction = 0.05;

  const size_t& baseSize = this->PolyLODsBaseSize;
  int count = 0;
  while (true)
  {
    // Squeeze the data, or we won't get an accurate memory size
    polyData->Squeeze();
    auto dataSize = polyData->GetActualMemorySize();
    bool tooSimilar = false;
    if (previousDataSize != 0)
    {
      double fraction = (static_cast<double>(previousDataSize) - dataSize) / previousDataSize;
      if (fabs(fraction) < minDiffFraction)
      {
        tooSimilar = true;
      }
    }
    previousDataSize = dataSize;

    if (static_cast<size_t>(dataSize) * 1000 <= baseSize || tooSimilar)
    {
      // It is either now below the base size, or the size isn't
      // changing much anymore.
      // The latest "polyData" will be written into the .vtkjs directory
      break;
    }

    // Write out the source LOD
    // They are not zipped yet, but they should be zipped by subclasses
    std::string name =
      "sourceLOD_" + std::to_string(this->DatasetCount) + "_" + std::to_string(++count) + ".zip";
    std::string full_path = path + name;
    dsWriter->SetInputData(polyData);
    dsWriter->GetArchiver()->SetArchiveName(full_path.c_str());
    dsWriter->Write();
    files.push_back(name);
    this->FilesToZip.push_back(full_path);

    // Now reduce the size of the data
    double bounds[6];
    polyData->GetBounds(bounds);
    double length = polyData->GetLength();
    double factors[3] = { (bounds[1] - bounds[0]) / length + 0.01,
      (bounds[3] - bounds[2]) / length + 0.01, (bounds[5] - bounds[4]) / length + 0.01 };
    double factorsCube = factors[0] * factors[1] * factors[2];
    // Try to make a good first guess for B
    // TODO: this first guess can probably be improved a lot.
    double B = pow(100 * dataSize / factorsCube, 0.3333);

    vtkNew<vtkQuadricClustering> cc;
    cc->UseInputPointsOn();
    cc->CopyCellDataOn();
    cc->SetInputDataObject(polyData);
    cc->SetAutoAdjustNumberOfDivisions(false);

    // We will try to get the next size to be between 1/3 and 1/5
    // of the original. The goal is to be approximately 1/4.
    auto targetSize = dataSize / 4;
    auto targetMin = dataSize / 5;
    auto targetMax = dataSize / 3;
    int maxAttempts = 100;
    size_t previousSize = 0;

    // If we fail to get to ~1/4 the size for some reason, just use
    // the default divisions. Sometimes, a failure is caused by
    // one of the factors being too big.
    bool useDefaultDivisions = false;

    for (int numAttempts = 0; numAttempts < maxAttempts; ++numAttempts)
    {
      int divs[3];
      for (int i = 0; i < 3; ++i)
      {
        divs[i] = B * factors[i] + 1;
      }

      // The allocated memory is proportional to divs[0] * divs[1] * divs[2].
      // Make sure this product is not too big, or we may run out of memory,
      // or, worse, have an integer overflow.
      // At the time of testing, a product of 1e8 requires more than 10 GB of
      // memory to run. Let's set a limit here for now.
      size_t maxProduct = 1e8;
      if (static_cast<size_t>(divs[0]) * divs[1] * divs[2] > maxProduct)
      {
        // Too big. Just use the defaults.
        useDefaultDivisions = true;
        break;
      }

      cc->SetNumberOfXDivisions(divs[0]);
      cc->SetNumberOfYDivisions(divs[1]);
      cc->SetNumberOfZDivisions(divs[2]);

      try
      {
        cc->Update();
      }
      catch (const std::bad_alloc&)
      {
        // Too many divisions, probably. Just use the defaults.
        useDefaultDivisions = true;
        break;
      }

      // Squeeze the data, or we won't get an accurate memory size
      cc->GetOutput()->Squeeze();
      auto newSize = cc->GetOutput()->GetActualMemorySize();

      if (newSize == previousSize)
      {
        // This is not changing. Just use the default divisions.
        useDefaultDivisions = true;
        break;
      }
      previousSize = newSize;

      if (newSize >= targetMin && newSize <= targetMax)
      {
        // We are within the tolerance!
        break;
      }
      else
      {
        // Figure out the fraction that we are off, and change B
        // accordingly
        double fraction = newSize / static_cast<double>(targetSize);
        B /= pow(fraction, 0.333);
      }
    }

    if (useDefaultDivisions)
    {
      vtkNew<vtkQuadricClustering> defaultCC;
      defaultCC->UseInputPointsOn();
      defaultCC->CopyCellDataOn();
      defaultCC->SetInputDataObject(polyData);
      defaultCC->Update();
      polyData = defaultCC->GetOutput();
    }
    else
    {
      polyData = cc->GetOutput();
    }
  }

  // Write out the config
  const char* url = this->PolyLODsBaseUrl;
  std::string baseUrl = url ? url : "";

  constexpr const char* INDENT = "      ";
  std::stringstream config;
  config << ",\n"
         << INDENT << "\"sourceLODs\": {\n"
         << INDENT << "  \"baseUrl\": \"" << baseUrl << "\",\n"
         << INDENT << "  \"files\": [\n";

  // Reverse the order of the files so the smallest comes first
  std::reverse(files.begin(), files.end());
  for (size_t i = 0; i < files.size(); ++i)
  {
    config << INDENT << "    \"" << files[i] << "\"";
    if (i != files.size() - 1)
    {
      config << ",\n";
    }
    else
    {
      config << "\n";
    }
  }

  config << INDENT << "  ]\n" << INDENT << "}";

  polyLODsConfig = config.str();

  return polyData;
}

//------------------------------------------------------------------------------

void vtkJSONSceneExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
