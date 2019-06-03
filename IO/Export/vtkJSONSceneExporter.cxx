/*=========================================================================

  Program:   VisualizationJSONlkit
  Module:    vtkJSONSceneExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJSONSceneExporter.h"

#include "vtkCamera.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkExporter.h"
#include "vtkJSONDataSetWriter.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkJSONSceneExporter);

// ----------------------------------------------------------------------------

vtkJSONSceneExporter::vtkJSONSceneExporter()
{
  this->FileName = nullptr;
}

// ----------------------------------------------------------------------------

vtkJSONSceneExporter::~vtkJSONSceneExporter()
{
  delete[] this->FileName;
}

// ----------------------------------------------------------------------------

void vtkJSONSceneExporter::WriteDataObject(ostream& os, vtkDataObject* dataObject, vtkActor* actor)
{
  // Skip if nothing to process
  if (dataObject == nullptr)
  {
    return;
  }

  // Handle Dataset
  if (dataObject->IsA("vtkDataSet"))
  {
    std::string renderingSetup = this->ExtractRenderingSetup(actor);
    std::string dsMeta =
      this->WriteDataSet(vtkDataSet::SafeDownCast(dataObject), renderingSetup.c_str());
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
    vtkSmartPointer<vtkCompositeDataIterator> iter = composite->NewIterator();
    iter->SkipEmptyNodesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      this->WriteDataObject(os, iter->GetCurrentDataObject(), actor);
      iter->GoToNextItem();
    }
  }
}

// ----------------------------------------------------------------------------

std::string vtkJSONSceneExporter::ExtractRenderingSetup(vtkActor* actor)
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

  const char* INDENT = "      ";
  std::stringstream renderingConfig;
  renderingConfig
    << ",\n"
    << INDENT << "\"actor\": {\n"
    << INDENT << "  \"origin\": ["
    << p3dOrigin[0] << ", "
    << p3dOrigin[1] << ", "
    << p3dOrigin[2] << "],\n"
    << INDENT << "  \"scale\": ["
    << p3dScale[0] << ", "
    << p3dScale[1] << ", "
    << p3dScale[2] << "],\n"
    << INDENT << "  \"position\": ["
    << p3dPosition[0] << ", "
    << p3dPosition[1] << ", "
    << p3dPosition[2] << "]\n"
    << INDENT << "},\n"
    << INDENT << "\"actorRotation\": ["
    << p3dRotateWXYZ[0] << ", "
    << p3dRotateWXYZ[1] << ", "
    << p3dRotateWXYZ[2] << ", "
    << p3dRotateWXYZ[3] << "],\n"
    << INDENT << "\"mapper\": {\n"
    << INDENT << "  \"colorByArrayName\": \"" << colorArrayName << "\",\n"
    << INDENT << "  \"colorMode\": " << colorMode << ",\n"
    << INDENT << "  \"scalarMode\": " << scalarMode << "\n"
    << INDENT << "},\n"
    << INDENT << "\"property\": {\n"
    << INDENT << "  \"representation\": " << representation << ",\n"
    << INDENT << "  \"edgeVisibility\": " << edgeVisibility << ",\n"
    << INDENT << "  \"diffuseColor\": ["
    << colorToUse[0] << ", "
    << colorToUse[1] << ", "
    << colorToUse[2] << "],\n"
    << INDENT << "  \"pointSize\": " << pointSize << ",\n"
    << INDENT << "  \"opacity\": " << opacity << "\n"
    << INDENT << "}\n";

  return renderingConfig.str();
}

// ----------------------------------------------------------------------------

std::string vtkJSONSceneExporter::WriteDataSet(vtkDataSet* dataset, const char* addOnMeta = nullptr)
{
  if (!dataset)
  {
    return "";
  }

  std::stringstream dsPath;
  dsPath << this->FileName << "/" << (++this->DatasetCount);

  vtkNew<vtkJSONDataSetWriter> dsWriter;
  dsWriter->SetInputData(dataset);
  dsWriter->SetFileName(dsPath.str().c_str());
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
  const char* INDENT = "    ";
  meta << INDENT << "{\n"
       << INDENT << "  \"name\": \"" << this->DatasetCount << "\",\n"
       << INDENT << "  \"type\": \"httpDataSetReader\",\n"
       << INDENT << "  \"httpDataSetReader\": { \"url\": \"" << this->DatasetCount << "\" }";

  if (addOnMeta != nullptr)
  {
    meta << addOnMeta;
  }

  meta << INDENT << "}";

  return meta.str();
}

// ----------------------------------------------------------------------------

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
    const char* INDENT = "    ";
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

      lutJSON
        << "\n" << INDENT << INDENT << "["
        << node[0] << ", "
        << node[1] << ", "
        << node[2] << ", "
        << node[3] << ", "
        << node[4] << ", "
        << node[5] << "]";
    }

    // Close node list
    lutJSON << "\n      ]\n    }";

    // Store in map...
    this->LookupTables[name] = lutJSON.str();
  }
}

// ----------------------------------------------------------------------------

void vtkJSONSceneExporter::WriteData()
{
  this->DatasetCount = 0;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  if (!vtksys::SystemTools::MakeDirectory(this->FileName))
  {
    vtkErrorMacro(<< "Can not create directory " << this->FileName);
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
  vtkIdType nbProps = renProps->GetNumberOfItems();
  for (vtkIdType rpIdx = 0; rpIdx < nbProps; rpIdx++)
  {
    vtkProp* renProp = vtkProp::SafeDownCast(renProps->GetItemAsObject(rpIdx));

    // Skip non-visible actors
    if (!renProp || !renProp->GetVisibility())
    {
      continue;
    }

    // Skip actors with no geometry
    vtkActor* actor = vtkActor::SafeDownCast(renProp);
    if (!actor)
    {
      continue;
    }

    vtkMapper* mapper = actor->GetMapper();
    vtkDataObject* dataObject = mapper->GetInputDataObject(0, 0);
    this->WriteDataObject(sceneComponents, dataObject, actor);
    this->WriteLookupTable(mapper->GetArrayName(), mapper->GetLookupTable());
  }

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
                << "  \"scene\": [" << sceneComponents.str()
                << "\n  ],\n"
                << "  \"lookupTables\": {\n";

  // Inject lookup table
  size_t nbLuts = this->LookupTables.size();
  for (auto const& lut : this->LookupTables)
  {
    sceneJsonFile << "    \"" << lut.first.c_str() << "\": " << lut.second.c_str()
                  << (--nbLuts ? "," : "") << "\n";
  }

  sceneJsonFile << "  }\n"
                << "}\n";

  // Write meta-data file
  std::stringstream scenePath;
  scenePath << this->FileName << "/index.json";

  ofstream file;
  file.open(scenePath.str().c_str(), ios::out);
  file << sceneJsonFile.str().c_str();
  file.close();
}

// ----------------------------------------------------------------------------

void vtkJSONSceneExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
