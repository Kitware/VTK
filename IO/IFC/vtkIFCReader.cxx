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
#include <ifcgeom/ConversionSettings.h>
#include <ifcgeom/IfcGeomRepresentation.h>
#include <sstream>
#include <string>

#include <ifcgeom/Iterator.h>
#include <ifcparse/IfcFile.h>
// this would be better defined using IfcOpenShellConfig.cmake but its not
#define IFOPSH_WITH_OPENCASCADE
#include <ifcgeom/hybrid_kernel.h>

// Schemas available: 2x3;4;4x1;4x2;4x3;4x3_tc1;4x3_add1;4x3_add2

// For BOOST_PP_SEQ_FOR_EACH and BOOST_PP_STRINGIZE preprocessor macro
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/program_options.hpp>

// Include all possible schema types that could be parsed
#include "ifcparse/Ifc2x3.h"
#include "ifcparse/Ifc4.h"
#include "ifcparse/Ifc4x1.h"
#include "ifcparse/Ifc4x2.h"
#include "ifcparse/Ifc4x3.h"
/**
 * IFC4X3_TC1 refers to the first Technical Corrigendum (TC1)
 *
 * Purpose: A Technical Corrigendum primarily serves to correct minor
 * technical problems, fix typos, and improve the documentation of an
 * existing standard, without expanding its scope or functionality.
 */
#include "ifcparse/Ifc4x3_tc1.h"
/**
 * Current Standard: The latest official and internationally
 * accredited version of the standard is IFC 4.3.2.0 (also referred to
 * simply as IFC 4.3), which supersedes previous development versions
 * like IFC4x3 Add1 and Add2.
 */
#include "ifcparse/Ifc4x3_add1.h"
#include "ifcparse/Ifc4x3_add2.h"

#include <thread>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// Enumerate through all IFC schemas you want to be able to process
#define IFC_SCHEMA_SEQ (4x3_add2)(4x3_add1)(4x3)(4x2)(4x1)(4)(2x3)
#define EXPAND_AND_CONCATENATE(elem) Ifc##elem
#define PROCESS_FOR_SCHEMA(r, data, elem)                                                          \
  if (schema_version == BOOST_PP_STRINGIZE(elem))                                                  \
  {                                                                                                \
    parseIfc<EXPAND_AND_CONCATENATE(elem)>(file);                                                  \
  }                                                                                                \
  else

template <typename Schema>
void parseIfc(IfcParse::IfcFile& file)
{
  const typename Schema::IfcProduct::list::ptr elements =
    file.instances_by_type<typename Schema::IfcProduct>();
  for (typename Schema::IfcProduct::list::it it = elements->begin(); it != elements->end(); ++it)
  {
    typename Schema::IfcProduct* ifcProduct = *it;
    // TODO: Do something with ifcProduct
  }
}

void process(const std::string& schema_version, IfcParse::IfcFile& file)
{
  // Syntactic sugar for iterating through all IFC schemas and passing them to main processing
  // method
  BOOST_PP_SEQ_FOR_EACH(PROCESS_FOR_SCHEMA, , IFC_SCHEMA_SEQ)
  { // The final else to catch unhandled schema version
    throw std::invalid_argument("IFC Schema " + schema_version + " not supported");
  }
}

void saveMaterial(vtkPolyData* polyData, ifcopenshell::geometry::taxonomy::style::ptr material)
{
  std::array<double, 3> diffuse, specular;
  double specularity, transparency;
  if (material->use_surface_color)
  {
    diffuse = { material->surface.r(), material->surface.g(), material->surface.b() };
    specular = { material->surface.r(), material->surface.g(), material->surface.b() };
    specularity = 0;
    transparency = 0;
  }
  else
  {
    diffuse = { material->diffuse.r(), material->diffuse.g(), material->diffuse.b() };
    specular = { material->specular.r(), material->specular.g(), material->specular.b() };
    specularity = std::isnan(material->specularity) ? 1 : material->specularity;
    transparency = std::isnan(material->transparency) ? 0 : material->transparency;
  }
  vtkPolyDataMaterial::SetField(polyData, vtkPolyDataMaterial::DIFFUSE_COLOR, diffuse.data(), 3);
  vtkPolyDataMaterial::SetField(polyData, vtkPolyDataMaterial::SPECULAR_COLOR, specular.data(), 3);
  vtkPolyDataMaterial::SetField(polyData, vtkPolyDataMaterial::SHININESS, &specularity, 1);
  vtkPolyDataMaterial::SetField(polyData, vtkPolyDataMaterial::TRANSPARENCY, &transparency, 1);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkIFCReader);

//------------------------------------------------------------------------------
vtkIFCReader::vtkIFCReader()
{
  this->FileName = nullptr;
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
    schema_version = schema_version.substr(3);
    std::transform(schema_version.begin(), schema_version.end(), schema_version.begin(),
      [](unsigned char c) { return std::tolower(c); });
    process(schema_version, file);
    ifcopenshell::geometry::Settings settings;
    // no need to use the transform
    settings.get<ifcopenshell::geometry::settings::UseWorldCoords>().value = true;
    settings.get<ifcopenshell::geometry::settings::OutputDimensionality>().value =
      ifcopenshell::geometry::CURVES_SURFACES_AND_SOLIDS;
    settings.get<ifcopenshell::geometry::settings::IteratorOutput>().value =
      ifcopenshell::geometry::TRIANGULATED;

    unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<IfcGeom::filter_t> filter_funcs;
    std::set<std::string> entities = { "IfcSpace", "IfcOpeningElement" };
    IfcGeom::entity_filter entity_filter;
    entity_filter.entity_names = entities;
    filter_funcs.emplace_back(boost::ref(entity_filter));

    IfcGeom::Iterator iterator(
      ifcopenshell::geometry::kernels::construct(&file, "opencascade", settings), settings, &file,
      filter_funcs, numThreads);
    if (!iterator.initialize())
    {
      throw std::logic_error("No geometrical elements found or none successfully converted");
    }
    unsigned int i = 0;
    const unsigned int DEFAULT_NUMBER_OF_ENTIITIES = 512;
    output->SetNumberOfPartitionedDataSets(DEFAULT_NUMBER_OF_ENTIITIES);
    do
    {
      // reallocate if needed
      if (i >= output->GetNumberOfPartitionedDataSets())
      {
        output->SetNumberOfPartitionedDataSets(output->GetNumberOfPartitionedDataSets() * 2);
      }
      IfcGeom::Element* element = iterator.get();
      vtkLog(INFO, "Name: " << element->name());
      vtkLog(INFO, "Type: " << element->type());
      // if (element->name() != "Terrain_Final")
      // {
      //   continue;
      // }
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
        saveMaterial(polyData, materials[0]);
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
              saveMaterial(polyData, materials[*mid0]);
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
    } while (iterator.next());
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
  if (!vtksys::SystemTools::FileExists(this->FileName, true))
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
