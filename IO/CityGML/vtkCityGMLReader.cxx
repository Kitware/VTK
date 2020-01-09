/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCityGMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCityGMLReader.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCollection.h"
#include "vtkContourTriangulator.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTriangle.h"
#include "vtkTriangleFilter.h"
#include "vtk_pugixml.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class vtkCityGMLReader::Implementation
{
public:
  enum PolygonType
  {
    NONE,
    TEXTURE,
    MATERIAL
  };

public:
  Implementation(vtkCityGMLReader* reader, int lod, int useTransparencyAsOpacity)
  {
    this->Initialize(reader, lod, useTransparencyAsOpacity);
  }

  void Initialize(vtkCityGMLReader* reader, int lod, int useTransparencyAsOpacity)
  {
    this->Reader = reader;
    this->LOD = lod;
    this->UseTransparencyAsOpacity = useTransparencyAsOpacity;
    this->PolyIdToTextureCoordinates.clear();
    this->PolyIdToMaterialIndex.clear();
    this->Materials.clear();
    InitializeImplicitGeometry();
  }

  void CacheParameterizedTexture(pugi::xml_document& doc)
  {
    std::ostringstream ostr;
    ostr << "//app:Appearance//app:ParameterizedTexture";
    auto xtextureNode = doc.select_nodes(ostr.str().c_str());
    for (auto it = xtextureNode.begin(); it != xtextureNode.end(); ++it)
    {
      TextureInfo info;
      auto textureNode = it->node();
      info.ImageURI = textureNode.child("app:imageURI");

      auto targetNode = textureNode.child("app:target");
      while (targetNode)
      {
        auto texCoordList = targetNode.first_child();
        for (auto textureCoordinates : texCoordList.children())
        {
          info.TextureCoordinates = textureCoordinates;
          const char* polyId = textureCoordinates.attribute("ring").value();
          // in the Berlin 3D dataset
          // https://www.businesslocationcenter.de/en/economic-atlas/download-portal/
          // app:textureCoordinates ring attribute is prefixed by a #,
          // while gml:LinearRing gml:id attribute is not
          if (polyId[0] == '#')
          {
            ++polyId;
          }
          this->PolyIdToTextureCoordinates[polyId] = info;
        }
        targetNode = targetNode.next_sibling("app:target");
      }
    }
  }

  void CacheX3DMaterial(pugi::xml_document& doc)
  {
    std::ostringstream ostr;
    ostr << "//app:Appearance//app:X3DMaterial";
    auto xnodes = doc.select_nodes(ostr.str().c_str());
    for (auto xnode : xnodes)
    {
      auto node = xnode.node();
      node = node.first_child();
      Material material;
      for (; std::string(node.name()) != "app:target"; node = node.next_sibling())
      {
        const char* value = node.child_value();
        std::istringstream iss(value);
        std::array<float, 3> color;
        for (size_t i = 0; i < color.size(); ++i)
        {
          iss >> color[i];
        }
        if (std::string(node.name()) == "app:diffuseColor")
        {
          material.Diffuse = color;
        }
        else if (std::string(node.name()) == "app:specularColor")
        {
          material.Specular = color;
        }
        else if (std::string(node.name()) == "app:transparency")
        {
          float transparency = this->UseTransparencyAsOpacity ? 1 - color[0] : color[0];
          material.Transparency = transparency;
        }
      }
      this->Materials.push_back(material);
      size_t indexMaterial = this->Materials.size() - 1;
      for (; node; node = node.next_sibling())
      {
        const char* id = node.child_value();
        // remove the # in front of the id.
        this->PolyIdToMaterialIndex[id + 1] = indexMaterial;
      }
    }
  }

  void InitializeImplicitGeometry()
  {
    this->RelativeGeometryIdToDataSet.clear();
    if (!this->RelativeGeometryDataSets)
    {
      this->RelativeGeometryDataSets = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    }
    this->RelativeGeometryDataSets->Initialize();
  }

  void CacheImplicitGeometry(pugi::xml_document& doc, const char* gmlNamespace, const char* feature)
  {
    auto xmultiSurface = doc.select_nodes((std::string("//") + gmlNamespace + ":" + feature + "/" +
      gmlNamespace + ":lod" + std::to_string(this->LOD) +
      "ImplicitRepresentation/"
      "core:ImplicitGeometry/core:relativeGMLGeometry/gml:MultiSurface")
                                            .c_str());
    for (auto it = xmultiSurface.begin(); it != xmultiSurface.end(); ++it)
    {
      pugi::xml_node node = it->node();
      const char* id = node.attribute("gml:id").value();
      this->ReadMultiSurface(node, this->RelativeGeometryDataSets);
      this->RelativeGeometryIdToDataSet[id] = this->RelativeGeometryDataSets->GetBlock(
        this->RelativeGeometryDataSets->GetNumberOfBlocks() - 1);
    }
  }

  void ReadImplicitGeometry(
    const pugi::xml_node& implicitGeometryNode, vtkMultiBlockDataSet* output, const char* element)
  {
    std::istringstream iss(implicitGeometryNode.child("core:transformationMatrix").child_value());
    double m[16];
    for (int i = 0; i < 16; ++i)
    {
      iss >> m[i];
    }
    vtkNew<vtkMatrix4x4> matrix;
    matrix->DeepCopy(m);
    const char* posString = implicitGeometryNode.child("core:referencePoint")
                              .child("gml:Point")
                              .child("gml:pos")
                              .child_value();
    iss.str(posString);
    vtkNew<vtkTransform> transform;
    transform->PostMultiply();
    transform->Concatenate(matrix);
    if (*posString)
    {
      double t[3];
      for (int i = 0; i < 3; ++i)
      {
        iss >> t[i];
      }
      transform->Translate(t);
    }
    pugi::xml_node relativeGeometryNode = implicitGeometryNode.child("core:relativeGMLGeometry");
    const char* href = relativeGeometryNode.attribute("xlink:href").value();
    const char* id = nullptr;
    if (*href == 0)
    {
      pugi::xml_node multiSurfaceNode = relativeGeometryNode.child("gml:MultiSurface");
      id = multiSurfaceNode.attribute("gml:id").value();
    }
    else
    {
      id = href + 1; // href is prefixed by a #.
    }
    auto it = this->RelativeGeometryIdToDataSet.find(id);
    if (it == this->RelativeGeometryIdToDataSet.end())
    {
      vtkWarningWithObjectMacro(this->Reader, << "Cannot find cached multi surface for id=" << id);
      return;
    }
    vtkNew<vtkTransformFilter> transformFilter;
    transformFilter->SetTransform(transform);
    transformFilter->SetInputDataObject(it->second);
    transformFilter->Update();
    vtkDataObject* obj = transformFilter->GetOutputDataObject(0);
    this->SetField(obj, "element", element);
    output->SetBlock(output->GetNumberOfBlocks(), obj);
  }

  void ReadImplicitGeometry(pugi::xml_document& doc, vtkMultiBlockDataSet* output,
    const char* gmlNamespace, const char* feature)
  {
    vtkNew<vtkMultiBlockDataSet> b;
    this->SetField(b, "element", "grp:CityObjectGroup");
    auto ximplicitGeometry =
      doc.select_nodes((std::string("//") + gmlNamespace + ":" + feature + "/" + gmlNamespace +
        ":" + "lod" + std::to_string(this->LOD) + "ImplicitRepresentation/core:ImplicitGeometry")
                         .c_str());
    for (auto it = ximplicitGeometry.begin(); it != ximplicitGeometry.end(); ++it)
    {
      this->ReadImplicitGeometry(
        it->node(), b, (std::string(gmlNamespace) + ":" + feature).c_str());
    }
    if (b->GetNumberOfBlocks())
    {
      output->SetBlock(output->GetNumberOfBlocks(), b);
    }
  }

  bool IsNewPolygonNeeded(PolygonType polygonType, size_t materialIndex,
    std::unordered_map<size_t, vtkPolyData*>& materialIndexToPolyData, const std::string& imageURI,
    std::unordered_map<std::string, vtkPolyData*>& imageURIToPolyData)
  {
    switch (polygonType)
    {
      case PolygonType::MATERIAL:
        return materialIndexToPolyData.find(materialIndex) == materialIndexToPolyData.end();
      case PolygonType::NONE:
      case PolygonType::TEXTURE:
      default:
        // for NONE imageURI is empty string
        return imageURIToPolyData.find(imageURI) == imageURIToPolyData.end();
    }
  }

  void SavePolygon(PolygonType polygonType, size_t materialIndex,
    std::unordered_map<size_t, vtkPolyData*>& materialIndexToPolyData, const std::string& imageURI,
    std::unordered_map<std::string, vtkPolyData*>& imageURIToPolyData, vtkPolyData* polyData)
  {
    switch (polygonType)
    {
      case PolygonType::MATERIAL:
        materialIndexToPolyData[materialIndex] = polyData;
        break;
      case PolygonType::NONE:
      case PolygonType::TEXTURE:
      default:
        // for NONE imageURI is empty string
        imageURIToPolyData[imageURI] = polyData;
        break;
    }
  }

  vtkPolyData* GetPolygon(PolygonType polygonType, size_t materialIndex,
    std::unordered_map<size_t, vtkPolyData*>& materialIndexToPolyData, const std::string& imageURI,
    std::unordered_map<std::string, vtkPolyData*>& imageURIToPolyData)
  {
    switch (polygonType)
    {
      case PolygonType::MATERIAL:
        return materialIndexToPolyData[materialIndex];
      case PolygonType::NONE:
      case PolygonType::TEXTURE:
      default:
        // for NONE imageURI is empty string
        return imageURIToPolyData[imageURI];
    }
  }

  PolygonType GetPolygonInfo(const char* id, const char* exteriorId, size_t* index,
    std::string* imageURI, std::string* tcoordsString)
  {
    if (GetPolygonTextureInfo(exteriorId, imageURI, tcoordsString))
      return PolygonType::TEXTURE;
    if (GetPolygonMaterialInfo(id, index))
      return PolygonType::MATERIAL;
    return PolygonType::NONE;
  }

  bool GetPolygonMaterialInfo(const char* id, size_t* index)
  {
    auto materialIndexIt = this->PolyIdToMaterialIndex.find(id);
    bool hasMaterial = false;
    if (materialIndexIt != this->PolyIdToMaterialIndex.end())
    {
      hasMaterial = true;
      *index = materialIndexIt->second;
    }
    return hasMaterial;
  }

  /**
   * Return true if the texture is found, false otherwise
   */
  bool GetPolygonTextureInfo(
    const char* exteriorId, std::string* imageURI, std::string* tcoordsString)
  {
    bool hasTexture = false;
    if (this->PolyIdToTextureCoordinates.find(exteriorId) != this->PolyIdToTextureCoordinates.end())
    {
      hasTexture = true;
      TextureInfo info = this->PolyIdToTextureCoordinates[exteriorId];
      *imageURI = info.ImageURI.child_value();
      *tcoordsString = info.TextureCoordinates.child_value();
    }
    return hasTexture;
  }

  /**
   * Return true if the texture is found, false otherwise
   */
  vtkIdType TCoordsFromString(const std::string& textureCoordinates, vtkDoubleArray* output)
  {
    std::istringstream iss(textureCoordinates);
    float textureValue[2];
    vtkIdType count = 0;
    for (iss >> textureValue[0] >> textureValue[1]; !iss.fail();
         iss >> textureValue[0] >> textureValue[1])
    {
      output->InsertTuple(output->GetNumberOfTuples(), textureValue);
      ++count;
    }
    // first point is repeated in the last position
    count = count - 1;
    output->SetNumberOfTuples(output->GetNumberOfTuples() - 1);
    return count;
  }

  void ReadLinearRingPolygon(pugi::xml_node nodeRing, vtkPoints* points, vtkCellArray* polys)
  {
    vtkIdType i = 0;
    vtkNew<vtkPolygon> poly;
    vtkIdList* polyPointIds = poly->GetPointIds();
    pugi::xml_node posList = nodeRing.child("gml:posList");
    if (posList)
    {
      std::istringstream iss(posList.child_value());
      bool validPoint = true;
      do
      {
        double p[3] = { 0., 0., 0. };
        for (vtkIdType j = 0; j < 3; ++j)
        {
          iss >> p[j];
          if (iss.fail())
          {
            if (j)
            {
              std::ostringstream oss;
              oss << "Number of values have to be multiple of three. Extra " << j
                  << " values. See: " << posList.child_value();
              throw std::runtime_error(oss.str());
            }
            else
            {
              std::ostringstream oss;
              double* pFirst = points->GetPoint(0);
              double* pLast = points->GetPoint(polyPointIds->GetNumberOfIds() - 1);
              if (!vtkMathUtilities::FuzzyCompare(pFirst[0], pLast[0]) ||
                !vtkMathUtilities::FuzzyCompare(pFirst[1], pLast[1]) ||
                !vtkMathUtilities::FuzzyCompare(pFirst[2], pLast[2]))
              {
                oss << "gml:posList: First point (" << pFirst[0] << ", " << pFirst[1] << ", "
                    << pFirst[2] << ") is not equal with last point (" << pLast[0] << ", "
                    << pLast[1] << ", " << pLast[2] << "). File may be corrupted.";
                throw std::runtime_error(oss.str());
              }
            }
            validPoint = false;
            break;
          }
        }
        if (validPoint)
        {
          points->InsertNextPoint(p);
          polyPointIds->InsertId(i, points->GetNumberOfPoints() - 1);
          ++i;
        }
      } while (validPoint);
      // gml:posList repeates the last point in a
      // polygon (there are n points). We only need the first n - 1.
      polyPointIds->SetNumberOfIds(polyPointIds->GetNumberOfIds() - 1);
      points->SetNumberOfPoints(points->GetNumberOfPoints() - 1);
      polys->InsertNextCell(poly);
    }
    else
    {
      vtkIdType n = std::distance(nodeRing.begin(), nodeRing.end());
      polyPointIds->SetNumberOfIds(n - 1);
      // go over all gml:pos children
      for (pugi::xml_node pos : nodeRing.children())
      {
        // Part-1-Terrain-WaterBody-Vegetation-V2.gml repeates the last point in a
        // polygon (there are n points). We only read the first n - 1.
        if (i == n - 1)
        {
          break;
        }
        std::istringstream iss(pos.child_value());
        double p[3];
        for (vtkIdType j = 0; j < 3; ++j)
        {
          iss >> p[j];
        }
        points->InsertNextPoint(p);
        polyPointIds->SetId(i, points->GetNumberOfPoints() - 1);
        ++i;
      }
      polys->InsertNextCell(poly);
    }
  }

  void ReadLinearRingLines(pugi::xml_node nodeRing, vtkPoints* points, vtkCellArray* lines)
  {
    pugi::xml_node posList = nodeRing.child("gml:posList");
    if (posList)
    {
      vtkNew<vtkLine> line;
      vtkIdType i = 1;
      std::istringstream iss(posList.child_value());
      bool validPoint = true;
      double p[3] = { 0., 0., 0. };
      for (vtkIdType j = 0; j < 3; ++j)
      {
        iss >> p[j];
        if (iss.fail())
        {
          std::ostringstream oss;
          oss << "Number of values have to be multiple of three. Extra " << j
              << " values. See: " << posList.child_value();
          throw std::runtime_error(oss.str());
        }
      }
      points->InsertNextPoint(p);
      vtkIdType firstPointIndex = points->GetNumberOfPoints() - 1;
      do
      {
        std::fill(p, p + 3, 0);
        for (vtkIdType j = 0; j < 3; ++j)
        {
          iss >> p[j];
          if (iss.fail())
          {
            if (j)
            {
              std::ostringstream oss;
              oss << "Number of values have to be multiple of three. Extra " << j
                  << " values. See: " << posList.child_value();
              throw std::runtime_error(oss.str());
            }
            validPoint = false;
            break;
          }
        }
        if (validPoint)
        {
          line->GetPointIds()->SetId(0, points->GetNumberOfPoints() - 1);
          points->InsertNextPoint(p);
          line->GetPointIds()->SetId(1, points->GetNumberOfPoints() - 1);
          lines->InsertNextCell(line);
          ++i;
        }
      } while (validPoint);
      // first point is repeated in the last position
      // one point less
      points->SetNumberOfPoints(points->GetNumberOfPoints() - 1);
      // point the last point of the last cell to the first point
      vtkNew<vtkIdList> cell;
      lines->GetCellAtId(lines->GetNumberOfCells() - 1, cell);
      cell->SetId(1, firstPointIndex);
      lines->ReplaceCellAtId(lines->GetNumberOfCells() - 1, cell);
    }
    else
    {
      std::array<double, 3> p;
      // Part-1-Terrain-WaterBody-Vegetation-V2.gml repeates the first point at the end
      vtkIdType n = std::distance(nodeRing.begin(), nodeRing.end());

      auto it = nodeRing.begin();
      {
        std::istringstream iss(it->child_value());
        for (size_t j = 0; j < p.size(); ++j)
        {
          iss >> p[j];
        }
      }
      points->InsertNextPoint(&p[0]);
      vtkIdType firstPointIndex = points->GetNumberOfPoints() - 1;
      vtkIdType i = 1;
      for (++it; it != nodeRing.end(); ++it, ++i)
      {
        vtkNew<vtkLine> line;
        // the last point is the same as the first point
        if (i < n - 1)
        {
          std::istringstream iss(it->child_value());
          for (size_t j = 0; j < p.size(); ++j)
          {
            iss >> p[j];
          }
        }
        line->GetPointIds()->SetId(0, points->GetNumberOfPoints() - 1);
        if (i < n - 1)
        {
          points->InsertNextPoint(&p[0]);
          line->GetPointIds()->SetId(1, points->GetNumberOfPoints() - 1);
        }
        else
        {
          line->GetPointIds()->SetId(1, firstPointIndex);
        }
        lines->InsertNextCell(line);
      }
    }
  }

  /**
   * Used for setting breakpoints when debugging.
   */
  void ParseGMLId(const char* idC, std::vector<int>* components)
  {
    std::string id(idC);
    size_t uPrev = id.find_first_of("_-"), u;
    char* strEnd;
    while ((u = id.find_first_of("_-", uPrev + 1)) != std::string::npos)
    {
      int value = std::strtol(id.substr(uPrev + 1, u - uPrev - 1).c_str(), &strEnd, 16);
      components->push_back(value);
      uPrev = u;
    }
    u = id.size();
    int value = std::strtol(id.substr(uPrev + 1, u - uPrev - 1).c_str(), &strEnd, 16);
    components->push_back(value);
  }

  static void SetField(vtkDataObject* obj, const char* name, const char* value)
  {
    vtkFieldData* fd = obj->GetFieldData();
    if (!fd)
    {
      vtkNew<vtkFieldData> newfd;
      obj->SetFieldData(newfd);
    }
    vtkNew<vtkStringArray> sa;
    sa->SetNumberOfTuples(1);
    sa->SetValue(0, value);
    sa->SetName(name);
    fd->AddArray(sa);
  }

  static void SetField(vtkDataObject* obj, const char* name, float* value, vtkIdType size)
  {
    vtkFieldData* fd = obj->GetFieldData();
    if (!fd)
    {
      vtkNew<vtkFieldData> newfd;
      obj->SetFieldData(newfd);
    }
    vtkNew<vtkFloatArray> da;
    da->SetNumberOfTuples(size);
    for (vtkIdType i = 0; i < size; ++i)
    {
      da->SetValue(i, value[i]);
    }
    da->SetName(name);
    fd->AddArray(da);
  }

  /**
   * This can read gml:MultiSurface and gml:CompositeSurface with texture
   * read from app:ParameterizedTexture
   */
  void ReadMultiSurface(const pugi::xml_node& multiSurfaceNode, vtkMultiBlockDataSet* output)
  {
    // A multi surface can have several materials and several textures.
    // We create a polydata for each material and texture and one for
    // no material and texture.
    std::unordered_map<size_t, vtkPolyData*> materialIndexToPolyData;
    std::unordered_map<std::string, vtkPolyData*> imageURIToPolyData;
    size_t polyDataCount = 0;
    // used to prevent polydata from being deleted until the end of the function
    vtkNew<vtkCollection> polyDataList;

    auto xpoly = multiSurfaceNode.select_nodes("gml:surfaceMember/gml:Polygon");
    const char* exteriorId = nullptr;
    for (auto it = xpoly.begin(); it != xpoly.end(); ++it)
    {
      pugi::xml_node nodePolygon = it->node();
      const char* id = nodePolygon.attribute("gml:id").value();
      pugi::xml_node nodeInterior = nodePolygon.child("gml:interior");

      std::string imageURI;
      std::string tcoordsString;
      size_t materialIndex = 0;
      pugi::xml_node nodeExteriorRing = nodePolygon.child("gml:exterior").child("gml:LinearRing");
      pugi::xml_attribute gmlIdAttribute = nodeExteriorRing.attribute("gml:id");
      exteriorId = gmlIdAttribute.value();

      // fill in texture coordinates for the this polygon
      PolygonType polygonType =
        this->GetPolygonInfo(id, exteriorId, &materialIndex, &imageURI, &tcoordsString);
      if (IsNewPolygonNeeded(
            polygonType, materialIndex, materialIndexToPolyData, imageURI, imageURIToPolyData))
      {
        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkPoints> points;
        points->SetDataType(VTK_DOUBLE);
        vtkNew<vtkCellArray> cells;
        if (gmlIdAttribute)
        {
          this->SetField(polyData, "gml_id", exteriorId);
        }
        polyData->SetPoints(points);
        nodeInterior ? polyData->SetLines(cells) : polyData->SetPolys(cells);
        switch (polygonType)
        {
          case PolygonType::TEXTURE:
            this->SetField(polyData, "texture_uri", imageURI.c_str());
            break;
          case PolygonType::MATERIAL:
          {
            Material material = this->Materials[materialIndex];
            this->SetField(polyData, "diffuse_color", &material.Diffuse[0], 3);
            this->SetField(polyData, "specular_color", &material.Specular[0], 3);
            this->SetField(polyData, "transparency", &material.Transparency, 1);
            break;
          }
          case PolygonType::NONE:
          default:
            // no fields to set
            break;
        }
        SavePolygon(polygonType, materialIndex, materialIndexToPolyData, imageURI,
          imageURIToPolyData, polyData);
        ++polyDataCount;
        polyDataList->AddItem(polyData);
      }
      vtkPolyData* polyData = GetPolygon(
        polygonType, materialIndex, materialIndexToPolyData, imageURI, imageURIToPolyData);
      vtkIdType exteriorTcoordsCount = 0;

      vtkNew<vtkPolyData> exteriorContour;
      vtkNew<vtkPoints> exteriorPoints;
      exteriorPoints->SetDataType(VTK_DOUBLE);
      vtkNew<vtkCellArray> exteriorCells;
      exteriorContour->SetPoints(exteriorPoints);
      bool hasTexture = (polygonType == PolygonType::TEXTURE);
      if (hasTexture)
      {
        vtkNew<vtkDoubleArray> exteriorTcoords;
        exteriorTcoords->SetNumberOfComponents(2);
        exteriorTcoords->SetName("tcoords");
        exteriorContour->GetPointData()->SetTCoords(exteriorTcoords);
        exteriorTcoordsCount = this->TCoordsFromString(tcoordsString, exteriorTcoords);
      }

      if (nodeInterior)
      {
        this->ReadLinearRingLines(nodeExteriorRing, exteriorPoints, exteriorCells);
        exteriorContour->SetLines(exteriorCells);

        // Read the interior rings
        vtkNew<vtkPolyData> interiorContour;
        vtkNew<vtkPoints> interiorPoints;
        interiorPoints->SetDataType(VTK_DOUBLE);
        vtkNew<vtkCellArray> interiorCells;
        vtkNew<vtkDoubleArray> interiorTCoords;
        interiorTCoords->SetNumberOfComponents(2);
        interiorTCoords->SetName("tcoords");
        interiorContour->SetPoints(interiorPoints);
        interiorContour->SetLines(interiorCells);
        interiorContour->GetPointData()->SetTCoords(interiorTCoords);
        // exterior and all interior polygons have texture
        while (nodeInterior)
        {
          auto nodeInteriorRing = nodeInterior.child("gml:LinearRing");
          const char* interiorId = nodeInteriorRing.attribute("gml:id").value();
          std::string interiorImageURI;
          std::string interiorTCoordsString;
          bool interiorHasTexture =
            this->GetPolygonTextureInfo(interiorId, &interiorImageURI, &interiorTCoordsString);
          if (hasTexture != interiorHasTexture)
          {
            vtkWarningWithObjectMacro(this->Reader,
              << "Exterior (" << hasTexture << ") and interior (" << interiorHasTexture
              << ") polygons have different texture specifications: " << exteriorId << ", "
              << interiorId);
            hasTexture = false;
          }
          if (hasTexture)
          {
            this->TCoordsFromString(interiorTCoordsString, interiorTCoords);
          }
          this->ReadLinearRingLines(nodeInteriorRing, interiorPoints, interiorCells);
          nodeInterior = nodeInterior.next_sibling("gml:interior");
        }

        if (!hasTexture)
        {
          interiorContour->GetPointData()->RemoveArray("tcoords");
          polyData->GetPointData()->RemoveArray("tcoords");
        }
        else
        {
          if (exteriorTcoordsCount != exteriorPoints->GetNumberOfPoints())
          {
            vtkWarningWithObjectMacro(this->Reader,
              << "Tcoords count (" << exteriorTcoordsCount << ") does not match point count ("
              << exteriorPoints->GetNumberOfPoints() << "): " << exteriorId);
          }
        }

        // compute transform to rotate to XY plane
        vtkNew<vtkPolygon> exteriorPolygon;
        exteriorPolygon->Initialize(exteriorPoints->GetNumberOfPoints(), exteriorPoints);
        double exteriorPolygonNormal[3];
        exteriorPolygon->ComputeNormal(exteriorPoints, exteriorPolygonNormal);
        double zAxis[3] = { 0, 0, 1 };
        double rotationAxis[3];
        vtkMath::Cross(exteriorPolygonNormal, zAxis, rotationAxis);
        double angleRad = vtkMath::AngleBetweenVectors(exteriorPolygonNormal, zAxis);
        double angle = vtkMath::DegreesFromRadians(angleRad);
        vtkNew<vtkTransform> transform;
        transform->RotateWXYZ(angle, rotationAxis);

        vtkNew<vtkAppendPolyData> append;
        append->AddInputData(exteriorContour);
        append->AddInputData(interiorContour);

        vtkNew<vtkTransformFilter> transformFilter;
        transformFilter->SetTransform(transform);
        transformFilter->SetInputConnection(append->GetOutputPort());
        // make sure all points have the same Z
        transformFilter->Update();
        vtkPointSet* xyPoly = transformFilter->GetOutput();
        vtkPoints* xyPoints = xyPoly->GetPoints();
        double p[3];
        xyPoints->GetPoint(0, p);
        double referenceZ = p[2];
        for (vtkIdType pointId = 1; pointId < xyPoints->GetNumberOfPoints(); ++pointId)
        {
          xyPoints->GetPoint(pointId, p);
          p[2] = referenceZ;
          xyPoints->SetPoint(pointId, p);
        }

        vtkNew<vtkContourTriangulator> triangulator;
        triangulator->SetInputConnection(transformFilter->GetOutputPort());

        vtkNew<vtkTransformFilter> transformBackFilter;
        transformBackFilter->SetTransform(transform->GetInverse());
        transformBackFilter->SetInputConnection(triangulator->GetOutputPort());
        transformBackFilter->Update();
        vtkPolyData* polyWithHoles = vtkPolyData::SafeDownCast(transformBackFilter->GetOutput());

        vtkNew<vtkAppendPolyData> appendPolyWithHoles;
        appendPolyWithHoles->AddInputData(polyData);
        appendPolyWithHoles->AddInputData(polyWithHoles);
        appendPolyWithHoles->Update();
        vtkPolyData* newPolyData = vtkPolyData::SafeDownCast(appendPolyWithHoles->GetOutput());

        SavePolygon(polygonType, materialIndex, materialIndexToPolyData, imageURI,
          imageURIToPolyData, newPolyData);
        polyDataList->AddItem(newPolyData);
      }
      else
      {
        this->ReadLinearRingPolygon(nodeExteriorRing, exteriorPoints, exteriorCells);
        exteriorContour->SetPolys(exteriorCells);
        if (exteriorTcoordsCount != exteriorPoints->GetNumberOfPoints())
        {
          if (hasTexture)
          {
            vtkWarningWithObjectMacro(this->Reader,
              << "Tcoords count (" << exteriorTcoordsCount << ") does not match point count ("
              << exteriorPoints->GetNumberOfPoints() << "): " << exteriorId);
            // fill in with the last texcoord value
            if (exteriorTcoordsCount < exteriorPoints->GetNumberOfPoints())
            {
              vtkDataArray* exteriorTcoords = exteriorContour->GetPointData()->GetTCoords();
              double* lastTex = exteriorTcoords->GetTuple(exteriorTcoords->GetNumberOfTuples());
              for (int i = 0; i < exteriorPoints->GetNumberOfPoints() - exteriorTcoordsCount; ++i)
              {
                exteriorTcoords->InsertTuple(exteriorTcoords->GetNumberOfTuples(), lastTex);
              }
            }
          }
        }

        // polygon can be concave
        vtkNew<vtkTriangleFilter> triangulate;
        triangulate->SetInputDataObject(exteriorContour);

        vtkNew<vtkAppendPolyData> append;
        append->AddInputData(polyData);
        append->AddInputConnection(triangulate->GetOutputPort());
        append->Update();
        vtkPolyData* newPolyData = vtkPolyData::SafeDownCast(append->GetOutput());
        SavePolygon(polygonType, materialIndex, materialIndexToPolyData, imageURI,
          imageURIToPolyData, newPolyData);
        polyDataList->AddItem(newPolyData);
      }
    }

    if (polyDataCount > 1)
    {
      vtkNew<vtkMultiBlockDataSet> b;
      for (const auto& p : imageURIToPolyData)
      {
        vtkPolyData* data = p.second;
        b->SetBlock(b->GetNumberOfBlocks(), data);
      }
      for (auto p : materialIndexToPolyData)
      {
        vtkPolyData* data = p.second;
        b->SetBlock(b->GetNumberOfBlocks(), data);
      }
      output->SetBlock(output->GetNumberOfBlocks(), b);
    }
    else if (polyDataCount == 1)
    {
      vtkPolyData* data = nullptr;
      if (!imageURIToPolyData.empty())
      {
        data = imageURIToPolyData.begin()->second;
      }
      else if (!materialIndexToPolyData.empty())
      {
        data = materialIndexToPolyData.begin()->second;
      }
      else
      {
        vtkWarningWithObjectMacro(
          this->Reader, << "One poly data which is neither texture nor material.");
      }
      output->SetBlock(output->GetNumberOfBlocks(), data);
    }
  }

  void ReadMultiSurfaceGroup(pugi::xml_document& doc, vtkMultiBlockDataSet* output,
    const char* gmlNamespace, const char* feature, float progressStart, float progressEnd,
    int maximumNumberOfNodes = std::numeric_limits<int>::max())
  {
    std::ostringstream ostr;
    std::string element = std::string(gmlNamespace) + ":" + feature;
    ostr << "//" << element;
    auto nodes = doc.select_nodes(ostr.str().c_str());
    int size = std::distance(nodes.begin(), nodes.end());
    int i = 0;
    for (auto featureNode : nodes)
    {
      vtkNew<vtkMultiBlockDataSet> groupBlock;
      ostr.str("");
      ostr << "descendant::" << gmlNamespace
           << ":lod" + std::to_string(this->LOD) + "Geometry/gml:MultiSurface |"
           << "descendant::" << gmlNamespace
           << ":lod" + std::to_string(this->LOD) + "MultiSurface/gml:MultiSurface";

      auto xMultiSurface = featureNode.node().select_nodes(ostr.str().c_str());
      for (auto it = xMultiSurface.begin(); it != xMultiSurface.end(); ++it)
      {
        pugi::xml_node node = it->node();
        this->ReadMultiSurface(node, groupBlock);
      }
      if (groupBlock->GetNumberOfBlocks())
      {
        output->SetBlock(output->GetNumberOfBlocks(), groupBlock);
        this->SetField(groupBlock, "element", element.c_str());
        pugi::xml_attribute gmlIdAttribute = featureNode.node().attribute("gml:id");
        auto gmlId = gmlIdAttribute.value();
        if (gmlId)
        {
          this->SetField(groupBlock, "gml_id", gmlId);
        }
      }
      ++i;
      if (i >= maximumNumberOfNodes)
      {
        break;
      }
      if (i % 1024 == 0)
      {
        this->Reader->UpdateProgress(progressStart + (progressEnd - progressStart) * i / size);
      }
    }
  }

  void ReadReliefFeature(pugi::xml_document& doc, vtkMultiBlockDataSet* output)
  {
    vtkNew<vtkPoints> points;
    points->SetDataType(VTK_DOUBLE);
    vtkNew<vtkCellArray> polys;

    pugi::xpath_node_set xrelief;
    xrelief = doc.select_nodes(("//dem:ReliefFeature//dem:TINRelief[number(child::dem:lod) = " +
      std::to_string(this->LOD) + "]//gml:TriangulatedSurface")
                                 .c_str());
    for (auto itSurface = xrelief.begin(); itSurface != xrelief.end(); ++itSurface)
    {
      pugi::xpath_node_set xtriangle;
      xtriangle = itSurface->node().select_nodes("//gml:Triangle//gml:LinearRing/gml:posList");

      vtkNew<vtkTriangle> triangle;
      for (auto it = xtriangle.begin(); it != xtriangle.end(); ++it)
      {
        pugi::xml_node node = it->node();
        std::istringstream iss(node.child_value());
        // Part-1-Terrain-WaterBody-Vegetation-V2.gml repeates the last point in a
        // triangle (there are 4 points). We only read the first 3.
        for (vtkIdType i = 0; i < 3; ++i)
        {
          double p[3];
          for (vtkIdType j = 0; j < 3; ++j)
          {
            iss >> p[j];
          }
          points->InsertNextPoint(p);
          triangle->GetPointIds()->SetId(i, points->GetNumberOfPoints() - 1);
        }
        polys->InsertNextCell(triangle);
      }

      if (points->GetNumberOfPoints())
      {
        vtkNew<vtkPolyData> polyData;
        polyData->SetPoints(points);
        polyData->SetPolys(polys);
        this->SetField(polyData, "element", "dem:ReliefFeature");
        output->SetBlock(output->GetNumberOfBlocks(), polyData);
      }
    }
  }

  void ReadWaterBody(pugi::xml_document& doc, vtkMultiBlockDataSet* output)
  {
    vtkNew<vtkMultiBlockDataSet> b;
    this->SetField(b, "element", "wtr:WaterBody");
    auto xWaterSurface = doc.select_nodes(("//wtr:WaterBody//wtr:WaterSurface/wtr:lod" +
      std::to_string(this->LOD) + "Surface/gml:CompositeSurface")
                                            .c_str());
    this->ReadMultiSurface(xWaterSurface.begin()->node(), b);
    auto xWaterGroundSurface = doc.select_nodes(("//wtr:WaterBody//wtr:WaterGroundSurface/wtr:lod" +
      std::to_string(this->LOD) + "Surface/gml:CompositeSurface")
                                                  .c_str());
    this->ReadMultiSurface(xWaterGroundSurface.begin()->node(), b);
    if (b->GetNumberOfBlocks())
    {
      output->SetBlock(output->GetNumberOfBlocks(), b);
    }
  }

private:
  struct TextureInfo
  {
    TextureInfo() {}
    pugi::xml_node ImageURI;
    pugi::xml_node TextureCoordinates;
  };

  struct Material
  {
    Material()
    {
      std::fill(this->Diffuse.begin(), this->Diffuse.end(), 1.0);
      std::fill(this->Specular.begin(), this->Specular.end(), 1.0);
      this->Transparency = 1.0;
    }
    std::array<float, 3> Diffuse;
    std::array<float, 3> Specular;
    float Transparency;
  };

private:
  vtkCityGMLReader* Reader;
  int LOD;
  int UseTransparencyAsOpacity;
  // map from polyid to (app:imageURI, app:textureCoordinates)
  std::unordered_map<std::string, TextureInfo> PolyIdToTextureCoordinates;
  std::unordered_map<std::string, size_t> PolyIdToMaterialIndex;
  std::vector<Material> Materials;
  std::unordered_map<std::string, vtkDataObject*> RelativeGeometryIdToDataSet;
  // used to store the datasets
  vtkSmartPointer<vtkMultiBlockDataSet> RelativeGeometryDataSets;
};

vtkStandardNewMacro(vtkCityGMLReader);

//----------------------------------------------------------------------------
vtkCityGMLReader::vtkCityGMLReader()
{
  this->FileName = nullptr;
  this->LOD = 3;
  this->UseTransparencyAsOpacity = false;
  this->Impl = new Implementation(this, this->LOD, this->UseTransparencyAsOpacity);
  this->SetNumberOfInputPorts(0);
  this->NumberOfBuildings = std::numeric_limits<int>::max();
}

//----------------------------------------------------------------------------
vtkCityGMLReader::~vtkCityGMLReader()
{
  delete this->Impl;
  delete[] this->FileName;
}

//----------------------------------------------------------------------------
int vtkCityGMLReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  this->Impl->Initialize(this, this->LOD, this->UseTransparencyAsOpacity);
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(this->FileName);
  this->UpdateProgress(0.2);

  if (!result)
  {
    std::ostringstream ostr;
    ostr << "XML [" << this->FileName << "] parsed with errors: " << result.description()
         << ". Error offset: " << result.offset << "]\n\n";
    vtkErrorMacro(<< ostr.str());
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  try
  {
    this->Impl->CacheParameterizedTexture(doc);
    this->Impl->CacheX3DMaterial(doc);
    this->UpdateProgress(0.3);
    this->Impl->ReadReliefFeature(doc, output);
    this->Impl->ReadWaterBody(doc, output);
    this->Impl->CacheImplicitGeometry(doc, "veg", "SolitaryVegetationObject");
    this->Impl->ReadImplicitGeometry(doc, output, "veg", "SolitaryVegetationObject");
    this->Impl->InitializeImplicitGeometry();
    this->UpdateProgress(0.4);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "brid", "Bridge", 0.4, 0.425);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "tun", "Tunnel", 0.425, 0.45);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "tran", "Railway", 0.45, 0.475);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "tran", "Road", 0.475, 0.5);
    this->UpdateProgress(0.5);
    this->Impl->ReadMultiSurfaceGroup(
      doc, output, "bldg", "Building", 0.5, 0.875, this->NumberOfBuildings);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "frn", "CityFurniture", 0.875, 0.9);
    this->UpdateProgress(0.9);
    this->Impl->CacheImplicitGeometry(doc, "frn", "CityFurniture");
    this->Impl->ReadImplicitGeometry(doc, output, "frn", "CityFurniture");
    this->Impl->InitializeImplicitGeometry();
    this->Impl->ReadMultiSurfaceGroup(doc, output, "gen", "GenericCityObject", 0.9, 0.95);
    this->Impl->ReadMultiSurfaceGroup(doc, output, "luse", "LandUse", 0.95, 1.0);
  }
  catch (pugi::xpath_exception& e)
  {
    vtkErrorMacro(<< "XPath Error:  " << e.what());
    return 0;
  }
  catch (std::runtime_error& e)
  {
    vtkErrorMacro(<< "Error:  " << e.what());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCityGMLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
