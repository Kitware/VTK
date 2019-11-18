/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPLYReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPLY.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <vector>

vtkStandardNewMacro(vtkPLYReader);

namespace
{
/**
 * Create an extra point in 'data' with the same coordinates and data as
 * the point at cellPointIndex inside cell. This is to avoid texture artifacts
 * when you have one point with two different texture values (so the latter
 * value override the first. This results in a texture discontinuity which results
 * in artifacts).
 */
vtkIdType duplicateCellPoint(vtkPolyData* data, vtkCell* cell, int cellPointIndex)
{
  // get the old point id
  vtkIdList* pointIds = cell->GetPointIds();
  vtkIdType pointId = pointIds->GetId(cellPointIndex);

  // duplicate that point and all associated data
  vtkPoints* points = data->GetPoints();
  double* point = data->GetPoint(pointId);
  vtkIdType newPointId = points->InsertNextPoint(point);
  for (int i = 0; i < data->GetPointData()->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* a = data->GetPointData()->GetArray(i);
    a->InsertTuple(newPointId, a->GetTuple(pointId));
  }
  // make cell use the new point
  pointIds->SetId(cellPointIndex, newPointId);
  return newPointId;
}

/**
 * Set a newPointId at cellPointIndex inside cell.
 */
void setCellPoint(vtkCell* cell, int cellPointIndex, vtkIdType newPointId)
{
  // get the old point id
  vtkIdList* pointIds = cell->GetPointIds();
  // make cell use the new point
  pointIds->SetId(cellPointIndex, newPointId);
}

/**
 * Compare two points for equality
 */
bool FuzzyEqual(double* f, double* s, double t)
{
  return vtkMathUtilities::FuzzyCompare(f[0], s[0], t) &&
    vtkMathUtilities::FuzzyCompare(f[1], s[1], t) && vtkMathUtilities::FuzzyCompare(f[2], s[2], t);
}
}

// Construct object with merging set to true.
vtkPLYReader::vtkPLYReader()
{
  this->Comments = vtkStringArray::New();
  this->ReadFromInputString = false;
  this->FaceTextureTolerance = 0.000001;
  this->DuplicatePointsForFaceTexture = true;
}

vtkPLYReader::~vtkPLYReader()
{
  this->Comments->Delete();
  this->Comments = nullptr;
}

namespace
{ // required so we don't violate ODR
typedef struct _plyVertex
{
  float x[3]; // the usual 3-space position of a vertex
  float tex[2];
  float normal[3];
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
} plyVertex;

typedef struct _plyFace
{
  unsigned char intensity; // optional face attributes
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
  unsigned char nverts;    // number of vertex indices in list
  int* verts;              // vertex index list
  unsigned char ntexcoord; // number of texcoord in list
  float* texcoord;         // texcoord list
} plyFace;
}

int vtkPLYReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  PlyProperty vertProps[] = {
    { "x", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, x)), 0, 0, 0, 0 },
    { "y", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, x) + sizeof(float)), 0, 0, 0,
      0 },
    { "z", PLY_FLOAT, PLY_FLOAT,
      static_cast<int>(offsetof(plyVertex, x) + sizeof(float) + sizeof(float)), 0, 0, 0, 0 },
    { "u", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, tex)), 0, 0, 0, 0 },
    { "v", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, tex) + sizeof(float)), 0, 0,
      0, 0 },
    { "nx", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, normal)), 0, 0, 0, 0 },
    { "ny", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, normal) + sizeof(float)), 0,
      0, 0, 0 },
    { "nz", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex, normal) + 2 * sizeof(float)),
      0, 0, 0, 0 },
    { "red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex, red)), 0, 0, 0, 0 },
    { "green", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex, green)), 0, 0, 0, 0 },
    { "blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex, blue)), 0, 0, 0, 0 },
    { "alpha", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex, alpha)), 0, 0, 0, 0 },
  };
  PlyProperty faceProps[] = {
    { "vertex_indices", PLY_INT, PLY_INT, static_cast<int>(offsetof(plyFace, verts)), 1, PLY_UCHAR,
      PLY_UCHAR, static_cast<int>(offsetof(plyFace, nverts)) },
    { "intensity", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace, intensity)), 0, 0, 0,
      0 },
    { "red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace, red)), 0, 0, 0, 0 },
    { "green", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace, green)), 0, 0, 0, 0 },
    { "blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace, blue)), 0, 0, 0, 0 },
    { "alpha", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace, alpha)), 0, 0, 0, 0 },
    { "texcoord", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyFace, texcoord)), 1, PLY_UCHAR,
      PLY_UCHAR, static_cast<int>(offsetof(plyFace, ntexcoord)) },
  };

  // open a PLY file for reading
  PlyFile* ply;
  int nelems, numElems, nprops;
  char **elist, *elemName;

  if (this->ReadFromInputString)
  {
    if (!(ply = vtkPLY::ply_open_for_reading_from_string(this->InputString, &nelems, &elist)))
    {
      vtkWarningMacro(<< "Could not open PLY file");
      return 0;
    }
  }
  else
  {
    if (!(ply = vtkPLY::ply_open_for_reading(this->FileName, &nelems, &elist)))
    {
      vtkWarningMacro(<< "Could not open PLY file");
      return 0;
    }
  }

  int numberOfComments = 0;
  char** comments = vtkPLY::ply_get_comments(ply, &numberOfComments);
  this->Comments->Reset();
  for (int i = 0; i < numberOfComments; i++)
  {
    this->Comments->InsertNextValue(comments[i]);
  }

  // Check to make sure that we can read geometry
  PlyElement* elem;
  int index;
  if ((elem = vtkPLY::find_element(ply, "vertex")) == nullptr ||
    vtkPLY::find_property(elem, "x", &index) == nullptr ||
    vtkPLY::find_property(elem, "y", &index) == nullptr ||
    vtkPLY::find_property(elem, "z", &index) == nullptr)
  {
    vtkErrorMacro(<< "Cannot read geometry");
    vtkPLY::ply_close(ply);
  }

  // Check for optional attribute data. We can handle intensity; and the
  // triplet red, green, blue.
  bool intensityAvailable = false;
  vtkSmartPointer<vtkUnsignedCharArray> intensity = nullptr;
  if ((elem = vtkPLY::find_element(ply, "face")) != nullptr &&
    vtkPLY::find_property(elem, "intensity", &index) != nullptr)
  {
    intensity = vtkSmartPointer<vtkUnsignedCharArray>::New();
    intensity->SetName("intensity");
    intensityAvailable = true;
    output->GetCellData()->AddArray(intensity);
    output->GetCellData()->SetActiveScalars("intensity");
  }

  bool rgbCellsAvailable = false;
  bool rgbCellsHaveAlpha = false;
  vtkSmartPointer<vtkUnsignedCharArray> rgbCells = nullptr;
  if ((elem = vtkPLY::find_element(ply, "face")) != nullptr &&
    vtkPLY::find_property(elem, "red", &index) != nullptr &&
    vtkPLY::find_property(elem, "green", &index) != nullptr &&
    vtkPLY::find_property(elem, "blue", &index) != nullptr)
  {
    rgbCellsAvailable = true;
    rgbCells = vtkSmartPointer<vtkUnsignedCharArray>::New();
    if (vtkPLY::find_property(elem, "alpha", &index) != nullptr)
    {
      rgbCells->SetName("RGBA");
      rgbCells->SetNumberOfComponents(4);
      rgbCellsHaveAlpha = true;
    }
    else
    {
      rgbCells->SetName("RGB");
      rgbCells->SetNumberOfComponents(3);
    }
    output->GetCellData()->AddArray(rgbCells);
    output->GetCellData()->SetActiveScalars("RGB");
  }

  bool rgbPointsAvailable = false;
  bool rgbPointsHaveAlpha = false;
  vtkSmartPointer<vtkUnsignedCharArray> rgbPoints = nullptr;
  if ((elem = vtkPLY::find_element(ply, "vertex")) != nullptr)
  {
    if (vtkPLY::find_property(elem, "red", &index) != nullptr &&
      vtkPLY::find_property(elem, "green", &index) != nullptr &&
      vtkPLY::find_property(elem, "blue", &index) != nullptr)
    {
      rgbPointsAvailable = true;
    }
    else if (vtkPLY::find_property(elem, "diffuse_red", &index) != nullptr &&
      vtkPLY::find_property(elem, "diffuse_green", &index) != nullptr &&
      vtkPLY::find_property(elem, "diffuse_blue", &index) != nullptr)
    {
      rgbPointsAvailable = true;
      vertProps[8].name = "diffuse_red";
      vertProps[9].name = "diffuse_green";
      vertProps[10].name = "diffuse_blue";
    }
    if (rgbPointsAvailable)
    {
      rgbPoints = vtkSmartPointer<vtkUnsignedCharArray>::New();
      if (vtkPLY::find_property(elem, "alpha", &index) != nullptr)
      {
        rgbPoints->SetName("RGBA");
        rgbPoints->SetNumberOfComponents(4);
        rgbPointsHaveAlpha = true;
      }
      else
      {
        rgbPoints->SetName("RGB");
        rgbPoints->SetNumberOfComponents(3);
      }
      output->GetPointData()->SetScalars(rgbPoints);
    }
  }

  bool normalPointsAvailable = false;
  vtkSmartPointer<vtkFloatArray> normals = nullptr;
  if ((elem = vtkPLY::find_element(ply, "vertex")) != nullptr &&
    vtkPLY::find_property(elem, "nx", &index) != nullptr &&
    vtkPLY::find_property(elem, "ny", &index) != nullptr &&
    vtkPLY::find_property(elem, "nz", &index) != nullptr)
  {
    normals = vtkSmartPointer<vtkFloatArray>::New();
    normalPointsAvailable = true;
    normals->SetName("Normals");
    normals->SetNumberOfComponents(3);
    output->GetPointData()->SetNormals(normals);
  }

  bool texCoordsPointsAvailable = false;
  vtkSmartPointer<vtkFloatArray> texCoordsPoints = nullptr;
  if ((elem = vtkPLY::find_element(ply, "vertex")) != nullptr)
  {
    if (vtkPLY::find_property(elem, "u", &index) != nullptr &&
      vtkPLY::find_property(elem, "v", &index) != nullptr)
    {
      texCoordsPointsAvailable = true;
    }
    else if (vtkPLY::find_property(elem, "texture_u", &index) != nullptr &&
      vtkPLY::find_property(elem, "texture_v", &index) != nullptr)
    {
      texCoordsPointsAvailable = true;
      vertProps[3].name = "texture_u";
      vertProps[4].name = "texture_v";
    }

    if (texCoordsPointsAvailable)
    {
      texCoordsPoints = vtkSmartPointer<vtkFloatArray>::New();
      texCoordsPoints->SetName("TCoords");
      texCoordsPoints->SetNumberOfComponents(2);
      output->GetPointData()->SetTCoords(texCoordsPoints);
    }
  }

  bool texCoordsFaceAvailable = false;
  if ((elem = vtkPLY::find_element(ply, "face")) != nullptr && !texCoordsPointsAvailable)
  {
    if (vtkPLY::find_property(elem, "texcoord", &index) != nullptr)
    {
      texCoordsFaceAvailable = true;
      texCoordsPoints = vtkSmartPointer<vtkFloatArray>::New();
      texCoordsPoints->SetName("TCoords");
      texCoordsPoints->SetNumberOfComponents(2);
      output->GetPointData()->SetTCoords(texCoordsPoints);
    }
  }
  // Okay, now we can grab the data
  int numPts = 0, numPolys = 0;
  for (int i = 0; i < nelems; i++)
  {
    // get the description of the first element */
    elemName = elist[i];
    vtkPLY::ply_get_element_description(ply, elemName, &numElems, &nprops);

    // if we're on vertex elements, read them in
    if (elemName && !strcmp("vertex", elemName))
    {
      // Create a list of points
      numPts = numElems;
      vtkPoints* pts = vtkPoints::New();
      pts->SetDataTypeToFloat();
      pts->SetNumberOfPoints(numPts);

      // Setup to read the PLY elements
      vtkPLY::ply_get_property(ply, elemName, &vertProps[0]);
      vtkPLY::ply_get_property(ply, elemName, &vertProps[1]);
      vtkPLY::ply_get_property(ply, elemName, &vertProps[2]);

      if (texCoordsPointsAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &vertProps[3]);
        vtkPLY::ply_get_property(ply, elemName, &vertProps[4]);
        texCoordsPoints->SetNumberOfTuples(numPts);
      }

      if (normalPointsAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &vertProps[5]);
        vtkPLY::ply_get_property(ply, elemName, &vertProps[6]);
        vtkPLY::ply_get_property(ply, elemName, &vertProps[7]);
        normals->SetNumberOfTuples(numPts);
      }

      if (rgbPointsAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &vertProps[8]);
        vtkPLY::ply_get_property(ply, elemName, &vertProps[9]);
        vtkPLY::ply_get_property(ply, elemName, &vertProps[10]);
        if (rgbPointsHaveAlpha)
        {
          vtkPLY::ply_get_property(ply, elemName, &vertProps[11]);
        }
        rgbPoints->SetNumberOfTuples(numPts);
      }

      plyVertex vertex;
      for (int j = 0; j < numPts; j++)
      {
        vtkPLY::ply_get_element(ply, (void*)&vertex);
        pts->SetPoint(j, vertex.x);
        if (texCoordsPointsAvailable)
        {
          texCoordsPoints->SetTuple2(j, vertex.tex[0], vertex.tex[1]);
        }
        if (normalPointsAvailable)
        {
          normals->SetTuple3(j, vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        }
        if (rgbPointsAvailable)
        {
          if (rgbPointsHaveAlpha)
          {
            rgbPoints->SetTuple4(j, vertex.red, vertex.green, vertex.blue, vertex.alpha);
          }
          else
          {
            rgbPoints->SetTuple3(j, vertex.red, vertex.green, vertex.blue);
          }
        }
      }
      output->SetPoints(pts);
      pts->Delete();
    } // if vertex

    else if (elemName && !strcmp("face", elemName))
    {
      // texture coordinates
      vtkNew<vtkPoints> texCoords;
      // We store a list of pointIds (that have the same texture coordinates)
      // at the texture index returned by texLocator
      std::vector<std::vector<vtkIdType> > pointIds;
      pointIds.resize(output->GetNumberOfPoints());
      // Used to detect different texture values at a vertex.
      vtkNew<vtkIncrementalOctreePointLocator> texLocator;
      texLocator->SetTolerance(this->FaceTextureTolerance);
      double bounds[] = { 0.0, 1.0, 0.0, 1.0, 0.0, 0.0 };
      texLocator->InitPointInsertion(texCoords, bounds);

      // Create a polygonal array
      numPolys = numElems;
      vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
      polys->AllocateEstimate(numPolys, 3);
      plyFace face;
      vtkIdType vtkVerts[256];

      // Get the face properties
      vtkPLY::ply_get_property(ply, elemName, &faceProps[0]);
      if (intensityAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &faceProps[1]);
        intensity->SetNumberOfComponents(1);
        intensity->SetNumberOfTuples(numPolys);
      }
      if (rgbCellsAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &faceProps[2]);
        vtkPLY::ply_get_property(ply, elemName, &faceProps[3]);
        vtkPLY::ply_get_property(ply, elemName, &faceProps[4]);

        if (rgbCellsHaveAlpha)
        {
          vtkPLY::ply_get_property(ply, elemName, &faceProps[5]);
        }
        rgbCells->SetNumberOfTuples(numPolys);
      }
      if (texCoordsFaceAvailable)
      {
        vtkPLY::ply_get_property(ply, elemName, &faceProps[6]);
        texCoordsPoints->SetNumberOfTuples(numPts);
        if (this->DuplicatePointsForFaceTexture)
        {
          // initialize texture coordinates with invalid value
          for (int j = 0; j < numPts; ++j)
          {
            texCoordsPoints->SetTuple2(j, -1, -1);
          }
        }
      }

      // grab all the face elements
      vtkNew<vtkPolygon> cell;
      for (int j = 0; j < numPolys; j++)
      {
        // grab and element from the file
        vtkPLY::ply_get_element(ply, (void*)&face);
        for (int k = 0; k < face.nverts; k++)
        {
          vtkVerts[k] = face.verts[k];
        }
        free(face.verts); // allocated in vtkPLY::ascii/binary_get_element

        cell->Initialize(face.nverts, vtkVerts, output->GetPoints());
        if (intensityAvailable)
        {
          intensity->SetValue(j, face.intensity);
        }
        if (rgbCellsAvailable)
        {
          if (rgbCellsHaveAlpha)
          {
            rgbCells->SetValue(4 * j, face.red);
            rgbCells->SetValue(4 * j + 1, face.green);
            rgbCells->SetValue(4 * j + 2, face.blue);
            rgbCells->SetValue(4 * j + 3, face.alpha);
          }
          else
          {
            rgbCells->SetValue(3 * j, face.red);
            rgbCells->SetValue(3 * j + 1, face.green);
            rgbCells->SetValue(3 * j + 2, face.blue);
          }
        }
        if (texCoordsFaceAvailable)
        {
          // Test to know if there is a texcoord for every vertex
          if (face.nverts == (face.ntexcoord / 2))
          {
            if (this->DuplicatePointsForFaceTexture)
            {
              for (int k = 0; k < face.nverts; k++)
              {
                // new texture stored at the current face
                float newTex[] = { face.texcoord[k * 2], face.texcoord[k * 2 + 1] };
                // texture stored at vtkVerts[k] point
                float currentTex[2];
                texCoordsPoints->GetTypedTuple(vtkVerts[k], currentTex);
                double newTex3[] = { newTex[0], newTex[1], 0 };
                if (currentTex[0] == -1.0)
                {
                  // newly seen texture coordinates for vertex
                  texCoordsPoints->SetTuple2(vtkVerts[k], newTex[0], newTex[1]);
                  vtkIdType ti;
                  texLocator->InsertUniquePoint(newTex3, ti);
                  pointIds.resize(std::max(ti + 1, static_cast<vtkIdType>(pointIds.size())));
                  pointIds[ti].push_back(vtkVerts[k]);
                }
                else
                {
                  if (!vtkMathUtilities::FuzzyCompare(
                        currentTex[0], newTex[0], this->FaceTextureTolerance) ||
                    !vtkMathUtilities::FuzzyCompare(
                      currentTex[1], newTex[1], this->FaceTextureTolerance))
                  {
                    // different texture coordinate
                    // than stored at point vtkVerts[k]
                    vtkIdType ti;
                    int inserted = texLocator->InsertUniquePoint(newTex3, ti);
                    if (inserted)
                    {
                      // newly seen texture coordinate for vertex
                      // which already has some texture coordinates.
                      vtkIdType dp = duplicateCellPoint(output, cell, k);
                      texCoordsPoints->SetTuple2(dp, newTex[0], newTex[1]);
                      pointIds.resize(std::max(ti + 1, static_cast<vtkIdType>(pointIds.size())));
                      pointIds[ti].push_back(dp);
                    }
                    else
                    {
                      size_t sameTexIndex = 0;
                      if (pointIds[ti].size() > 1)
                      {
                        double first[3];
                        output->GetPoint(vtkVerts[k], first);
                        for (; sameTexIndex < pointIds[ti].size(); ++sameTexIndex)
                        {
                          double second[3];
                          output->GetPoint(pointIds[ti][sameTexIndex], second);
                          if (FuzzyEqual(first, second, this->FaceTextureTolerance))
                          {
                            break;
                          }
                        }
                        if (sameTexIndex == pointIds[ti].size())
                        {
                          // newly seen point for this texture coordinate
                          vtkIdType dp = duplicateCellPoint(output, cell, k);
                          texCoordsPoints->SetTuple2(dp, newTex[0], newTex[1]);
                          pointIds[ti].push_back(dp);
                        }
                      }

                      // texture coordinate already seen before, use the vertex
                      // associated with these texture coordinates
                      vtkIdType vi = pointIds[ti][sameTexIndex];
                      setCellPoint(cell, k, vi);
                    }
                  }
                  // same texture coordinate, nothing to do.
                }
              }
            }
            else
            {
              // if we don't want point duplication we only need to set
              // the texture coordinates
              for (int k = 0; k < face.nverts; k++)
              {
                // new texture stored at the current face
                float newTex[] = { face.texcoord[k * 2], face.texcoord[k * 2 + 1] };
                texCoordsPoints->SetTuple2(vtkVerts[k], newTex[0], newTex[1]);
              }
            }
          }
          else
          {
            vtkWarningMacro(<< "Number of texture coordinates " << face.ntexcoord
                            << " different than number of points " << face.nverts);
          }
          free(face.texcoord);
        }
        polys->InsertNextCell(cell);
      }
      output->SetPolys(polys);
    }

    free(elist[i]); // allocated by ply_open_for_reading
    elist[i] = nullptr;

  }            // for all elements of the PLY file
  free(elist); // allocated by ply_open_for_reading

  vtkDebugMacro(<< "Read: " << numPts << " points, " << numPolys << " polygons");

  // close the PLY file
  vtkPLY::ply_close(ply);

  return 1;
}

int vtkPLYReader::CanReadFile(const char* filename)
{
  FILE* fd = vtksys::SystemTools::Fopen(filename, "rb");
  if (!fd)
    return 0;

  char line[4] = {};
  const char* result = fgets(line, sizeof(line), fd);
  fclose(fd);
  return (result && strncmp(result, "ply", 3) == 0);
}

void vtkPLYReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Comments:\n";
  indent = indent.GetNextIndent();
  for (int i = 0; i < this->Comments->GetNumberOfValues(); ++i)
  {
    os << indent << this->Comments->GetValue(i) << "\n";
  }
}
