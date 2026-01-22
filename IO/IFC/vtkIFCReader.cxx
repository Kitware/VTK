// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIFCReader.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMaterial.h"

#include "vtksys/SystemTools.hxx"

#include <array>
#include <sstream>
#include <string>
#include <thread>

#include <ifcparse/IfcFile.h>

#include <ifcgeom/ConversionSettings.h>
#include <ifcgeom/IfcGeomRepresentation.h>
#include <ifcgeom/Iterator.h>
#include <ifcgeom/taxonomy.h>
// this would be better defined using IfcOpenShellConfig.cmake but its not
#define IFOPSH_WITH_OPENCASCADE
#include <ifcgeom/hybrid_kernel.h>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
double intensity(double r, double g, double b)
{
  return r * 0.2126 + g * 0.7152 + b * 0.0722;
}

double intensity(double* rgb)
{
  return intensity(rgb[0], rgb[1], rgb[2]);
}

void setMaterial(vtkPolyData* polyData, ifcopenshell::geometry::taxonomy::style& material)
{
  double dark = 0.1;
  double diffuseIntensity = intensity(material.diffuse.components().data());
  double surfaceIntensity = intensity(material.surface.components().data());
  // vtkLog(INFO, "intensity diffuse: " << intensityDiffuse << " surface: "
  //        << intensitySurface);
  // vtkLog(INFO, "color diffuse: (" <<
  //        material.diffuse.r() << ", " <<
  //        material.diffuse.g() << ", " <<
  //        material.diffuse.b() << ") surface: (" <<
  //        material.surface.r() << ", " <<
  //        material.surface.g() << ", " <<
  //        material.surface.b() << ")");
  ifcopenshell::geometry::taxonomy::colour color;
  if (material.use_surface_color)
  {
    if (surfaceIntensity < dark && diffuseIntensity > dark)
    {
      // we override!
      color = material.diffuse;
    }
    else
    {
      color = material.surface;
    }
  }
  else
  {
    if (diffuseIntensity < dark && surfaceIntensity > dark)
    {
      // we override
      color = material.surface;
    }
    else
    {
      color = material.diffuse;
    }
  }
  vtkPolyDataMaterial::SetField(
    polyData, vtkPolyDataMaterial::GetDiffuseColor(), color.components().data(), 3);
  if (material.specular && !material.use_surface_color)
  {
    vtkPolyDataMaterial::SetField(
      polyData, vtkPolyDataMaterial::GetSpecularColor(), material.specular.components().data(), 3);
  }
  if (material.has_specularity())
  {
    vtkPolyDataMaterial::SetField(
      polyData, vtkPolyDataMaterial::GetShininess(), &material.specularity, 1);
  }
  if (material.has_transparency())
  {
    vtkPolyDataMaterial::SetField(
      polyData, vtkPolyDataMaterial::GetTransparency(), &material.transparency, 1);
  }
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkIFCReader);

//------------------------------------------------------------------------------
vtkIFCReader::vtkIFCReader()
{
  this->FileName = nullptr;
  this->NumberOfThreads = 8;
  this->IncludeCurves = false;
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkIFCReader::~vtkIFCReader()
{
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkIFCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//------------------------------------------------------------------------------
int vtkIFCReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  try
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo)
    {
      throw std::runtime_error("Invalid output information object");
    }
    auto output = vtkPartitionedDataSetCollection::GetData(outputVector);
    if (!this->FileName || std::string(this->FileName).empty())
    {
      throw std::runtime_error("Invalid input filename: nullptr or empty");
    }
    if (!vtksys::SystemTools::FileExists(this->FileName, true))
    {
      throw std::runtime_error(std::string("Filename does not exist: ") + this->FileName);
    }

    IfcParse::IfcFile file(this->FileName);
    if (!file.good())
    {
      throw std::runtime_error(std::string("Unable to parse") + this->FileName);
    }
    auto schema_version = file.schema()->name();
    vtkLog(INFO, "File schema: " << schema_version << " threads: " << this->NumberOfThreads);
    ifcopenshell::geometry::Settings settings;
    // no need to use the transform
    settings.get<ifcopenshell::geometry::settings::UseWorldCoords>().value = true;
    settings.get<ifcopenshell::geometry::settings::OutputDimensionality>().value =
      (this->IncludeCurves ? ifcopenshell::geometry::CURVES_SURFACES_AND_SOLIDS
                           : ifcopenshell::geometry::SURFACES_AND_SOLIDS);
    settings.get<ifcopenshell::geometry::settings::IteratorOutput>().value =
      ifcopenshell::geometry::TRIANGULATED;
    // Try to get the reader to work as fast as IfcConvert to glb which
    // runs in about 30s for 'Viadotto Acerno bridge.ifc' compared with about
    // 2min for the reader.
    // We copied the same settings as IfcConvert but that did not speed up
    // the reader.
    settings.get<ifcopenshell::geometry::settings::MesherLinearDeflection>().value = 0.001;
    settings.get<ifcopenshell::geometry::settings::MesherAngularDeflection>().value = 0.5;
    settings.get<ifcopenshell::geometry::settings::PrecisionFactor>().value = 1;
    settings.get<ifcopenshell::geometry::settings::BooleanAttempt2d>().value = true;
    settings.get<ifcopenshell::geometry::settings::ApplyDefaultMaterials>().value = true;
    settings.get<ifcopenshell::geometry::settings::FunctionStepParam>().value = 0.5;
    settings.get<ifcopenshell::geometry::settings::CgalSmoothAngleDegrees>().value = -1;
    settings.get<ifcopenshell::geometry::settings::CircleSegments>().value = 16;

    std::vector<IfcGeom::filter_t> filter_funcs;
    std::set<std::string> entities = {
      "IfcSpace", "IfcOpeningElement",

      // "IfcAlignment",
      // "IfcAlignmentSegment",
      // "IfcAnnotation",
      // "IfcBeam",
      // "IfcBearing",
      // "IfcColumn",
      // "IfcCourse",
      // "IfcDiscreteAccessory",
      // "IfcEarthworksFill",
      // "IfcFooting",
      // //"IfcGeographicElement",
      // "IfcMechanicalFastener",
      // "IfcMember",
      // "IfcPavement",
      // "IfcPile",
      // "IfcRailing",
      // "IfcReferent",
      // "IfcSlab",
      // "IfcSurfaceFeature",
      // "IfcWall",
    };
    IfcGeom::entity_filter entity_filter;
    entity_filter.entity_names = entities;
    filter_funcs.emplace_back(boost::ref(entity_filter));
    // vtkLog(INFO, "Construct IfcGeom::Iterator ...");
    IfcGeom::Iterator iterator(
      ifcopenshell::geometry::kernels::construct(&file, "opencascade", settings), settings, &file,
      filter_funcs, this->NumberOfThreads);
    if (!iterator.initialize())
    {
      throw std::logic_error("No geometrical elements found or none successfully converted");
    }
    unsigned int i = 0;
    const unsigned int DEFAULT_NUMBER_OF_ENTIITIES = 512;
    output->SetNumberOfPartitionedDataSets(DEFAULT_NUMBER_OF_ENTIITIES);
    // vtkLog(INFO, "Iterate using IfcGeom::Iterator ...");
    bool hasElements;
    do
    {
      // reallocate if needed
      if (i >= output->GetNumberOfPartitionedDataSets())
      {
        // vtkLog(INFO, "Realocate NumberOfPartitionedDataSets to: "
        //        << output->GetNumberOfPartitionedDataSets() * 2);
        output->SetNumberOfPartitionedDataSets(output->GetNumberOfPartitionedDataSets() * 2);
      }
      IfcGeom::Element* element = iterator.get();
      // vtkLog(INFO, "Name: " << element->name());
      // vtkLog(INFO, "Type: " << element->type());
      // vtkLog(INFO, "i: " << i);
      const IfcGeom::TriangulationElement* shape =
        static_cast<const IfcGeom::TriangulationElement*>(element);
      const IfcGeom::Representation::Triangulation& geom = shape->geometry();
      auto& verts = geom.verts();
      auto& edges = geom.edges();
      auto& faces = geom.faces();
      auto& materialIds = geom.material_ids();
      auto& materials = geom.materials();
      // points that can be shared between several PolyData
      vtkNew<vtkDoubleArray> pointData;
      pointData->SetNumberOfComponents(3);
      pointData->SetNumberOfTuples(verts.size() / 3);
      std::copy(verts.begin(), verts.end(), pointData->GetPointer(0));
      vtkNew<vtkPoints> points;
      points->SetData(pointData);
      if (materials.size() <= 1)
      {
        ifcopenshell::geometry::taxonomy::style material =
          materials.empty() ? ifcopenshell::geometry::taxonomy::style() : *materials[0];
        // one material so we need one polydata
        vtkNew<vtkPolyData> polyData;
        polyData->SetPoints(points);
        vtkNew<vtkIdTypeArray> connectivity;
        vtkNew<vtkCellArray> cellArray;
        if (!faces.empty())
        {
          connectivity->SetNumberOfTuples(faces.size());
          std::copy(faces.begin(), faces.end(), connectivity->GetPointer(0));
          cellArray->SetData(3, connectivity);
          polyData->SetPolys(cellArray);
          // if edges are not empty
          // we already added the faces so do nothing
        }
        else if (!edges.empty())
        {
          connectivity->SetNumberOfTuples(edges.size());
          std::copy(edges.begin(), edges.end(), connectivity->GetPointer(0));
          cellArray->SetData(2, connectivity);
          polyData->SetLines(cellArray);
        }
        output->SetNumberOfPartitions(i, 1);
        output->SetPartition(i, 0, polyData);
        setMaterial(polyData, material);
      }
      else
      {
        // we create one polydata for each material
        std::vector<int>::const_iterator fid0;
        int stride;
        if (!faces.empty())
        {
          stride = 3;
          fid0 = faces.begin();
        }
        else
        {
          stride = 2;
          fid0 = edges.begin();
        }
        auto mid0 = materialIds.begin();
        auto mid1 = mid0;
        while (true)
        {
          mid1++;
          if ((mid1 == materialIds.end()) || (*mid1 != *mid0))
          {
            vtkNew<vtkPolyData> polyData;
            vtkNew<vtkIdTypeArray> connectivity;
            vtkNew<vtkCellArray> cellArray;
            polyData->SetPoints(points);
            auto n = std::distance(mid0, mid1);
            auto fid1 = fid0 + n * stride;
            connectivity->SetNumberOfTuples(n * stride);
            std::copy(fid0, fid1, connectivity->GetPointer(0));
            cellArray->SetData(stride, connectivity);
            if (stride == 2)
            {
              polyData->SetLines(cellArray);
            }
            else if (stride == 3)
            {
              polyData->SetPolys(cellArray);
            }
            output->SetNumberOfPartitions(i, output->GetNumberOfPartitions(i) + 1);
            output->SetPartition(i, output->GetNumberOfPartitions(i) - 1, polyData);
            if (*mid0 >= 0)
            {
              setMaterial(polyData, *materials[*mid0]);
            }
            if (mid1 == materialIds.end())
            {
              break;
            }
            mid0 = mid1;
            fid0 = fid1;
          }
        }
      }
      ++i;
      hasElements = iterator.next();
      int progress = iterator.progress();
      this->UpdateProgress(static_cast<double>(progress) / 100);
    } while (hasElements);
    output->SetNumberOfPartitionedDataSets(i);
    vtkLog(INFO, "Finished " << i << " partitioned datasets");
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< e.what());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkIFCReader::CanReadFile(const char* filename)
{
  if (!filename || std::string(filename).empty())
  {
    return 0;
  }
  if (!vtksys::SystemTools::FileExists(filename, true))
  {
    return 0;
  }
  IfcParse::IfcFile file(filename);
  if (!file.good())
  {
    vtkErrorMacro("Unable to parse" << filename);
    return 0;
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
