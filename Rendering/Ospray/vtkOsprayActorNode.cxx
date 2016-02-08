/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayActorNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayActorNode.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCollectionIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkSmartPointer.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"
#include "ospray/api/Device.h"
#include "ospray/common/OSPCommon.h"
#include "include/ospray/version.h"

#include <map>

//============================================================================

namespace vtkosp {

  class Vec3 {
  public:
    Vec3(float x, float y, float z) {
      vals[0] = x;
      vals[1] = y;
      vals[2] = z;
    }
    float operator[](unsigned int i) const { return vals[i]; }
    float x() { return vals[0]; }
    float y() { return vals[1]; }
    float z() { return vals[2]; }

    float vals[3];
  };

  class MyGeom {
  public:
    //TODO: Right now we make at most 4 if that is always true
    //optimize by replacing this vector with a fixed size array
    std::vector<OSPGeometry> geoms;
    ~MyGeom()
    {
      std::vector<OSPGeometry>::iterator it = geoms.begin();
      while (it != geoms.end())
        {
        ospRelease((OSPGeometry)*it);
        it++;
        }
      geoms.clear();
    }
    void Add(OSPGeometry geo)
    {
      geoms.push_back(geo);
    }
    void AddMyselfTo(OSPModel oModel)
    {
      std::vector<OSPGeometry>::iterator it = geoms.begin();
      while (it != geoms.end())
        {
        ospAddGeometry(oModel, *it);
        it++;
        }
    }
  };

  void PointNormalsOntoIndexes(
    std::vector<unsigned int> &indexArray,
    vtkDataArray *vNormals,
    ospray::vec3fa *&normals)
  {
    int numNormals = vNormals->GetNumberOfTuples();
    normals = new ospray::vec3fa[numNormals];
    for (int i = 0; i < numNormals; i++)
      {
      double *vNormal = vNormals->GetTuple(i);
      normals[i] = ospray::vec3fa(vNormal[0],vNormal[1],vNormal[2]);
      }
  }

  void CellColorMaterials(
    vtkUnsignedCharArray *vColors,
    OSPRenderer oRenderer,
    std::vector<OSPMaterial> &ospMaterials)
  {
    int numColors = vColors->GetNumberOfTuples();
    for (int i = 0; i < numColors; i++)
      {
      double *color = vColors->GetTuple(i);
      OSPMaterial oMaterial;
#if OSPRAY_VERSION_MAJOR == 0 && OSPRAY_VERSION_MINOR < 9
      oMaterial = ospNewMaterial(oRenderer,"OBJMaterial");
#else
      oMaterial = ospNewMaterial(oRenderer,"RayTraceMaterial");
#endif
      float diffusef[] = {color[0]/255.0,
                          color[1]/255.0,
                          color[2]/255.0};
      float specularf[] = {0,0,0};
      ospSet3fv(oMaterial,"Kd",diffusef);
      ospSet3fv(oMaterial,"Ks",specularf);
      ospSet1f(oMaterial,"Ns",0);
      ospSet1f(oMaterial,"d",1.0);
      ospCommit(oMaterial);
      ospMaterials.push_back(oMaterial);
      }
  }

}

//============================================================================
vtkStandardNewMacro(vtkOsprayActorNode);

//----------------------------------------------------------------------------
vtkOsprayActorNode::vtkOsprayActorNode()
{
  this->OSPMeshes = NULL;
}

//----------------------------------------------------------------------------
vtkOsprayActorNode::~vtkOsprayActorNode()
{
  delete (vtkosp::MyGeom*)this->OSPMeshes;
}

//----------------------------------------------------------------------------
void vtkOsprayActorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsprayActorNode::RenderSelf()
{
}

namespace {

  //CreateXIndexBuffer's adapted from vtkOpenGLIndexBufferObject
  //TODO: rule of three if this is copied anywhere else in VTK
  //----------------------------------------------------------------------------
  void CreatePointIndexBuffer(vtkCellArray *cells,
                              std::vector<unsigned int> &indexArray)
  {
    //TODO: restore the preallocate and append to offset features I omitted
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    if (!cells->GetNumberOfCells())
      {
      return;
      }
    for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
      {
      for (int i = 0; i < npts; ++i)
        {
        indexArray.push_back(static_cast<unsigned int>(*(indices++)));
        }
      }
    }

  //----------------------------------------------------------------------------
  void CreateLineIndexBuffer(vtkCellArray *cells,
                             std::vector<unsigned int> &indexArray)
  {
    //TODO: restore the preallocate and append to offset features I omitted
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    if (!cells->GetNumberOfCells())
      {
      return;
      }
    for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
      {
      for (int i = 0; i < npts-1; ++i)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[i]));
        indexArray.push_back(static_cast<unsigned int>(indices[i+1]));
        }
      }
  }

  //----------------------------------------------------------------------------
  void CreateTriangleLineIndexBuffer(vtkCellArray *cells,
                                     std::vector<unsigned int> &indexArray)
  {
    //TODO: restore the preallocate and append to offset features I omitted
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    if (!cells->GetNumberOfCells())
      {
      return;
      }
    for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
      {
      for (int i = 0; i < npts; ++i)
        {
        indexArray.push_back(static_cast<unsigned int>(indices[i]));
        indexArray.push_back(static_cast<unsigned int>
                             (indices[i < npts-1 ? i+1 : 0]));
        }
      }
  }

  //----------------------------------------------------------------------------
  void CreateTriangleIndexBuffer(vtkCellArray *cells, vtkPoints *points,
                                 std::vector<unsigned int> &indexArray)
  {
    //TODO: restore the preallocate and append to offset features I omitted
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    if (!cells->GetNumberOfCells())
      {
      return;
      }

    // the folowing are only used if we have to triangulate a polygon
    // otherwise they just sit at NULL
    vtkPolygon *polygon = NULL;
    vtkIdList *tris = NULL;
    vtkPoints *triPoints = NULL;

    for (cells->InitTraversal(); cells->GetNextCell(npts, indices); )
      {
      // ignore degenerate triangles
      if (npts < 3)
        {
        continue;
        }

      // triangulate needed
      if (npts > 3)
        {
        // special case for quads, penta, hex which are common
        if (npts == 4)
          {
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[1]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          }
        else if (npts == 5)
          {
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[1]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          indexArray.push_back(static_cast<unsigned int>(indices[4]));
          }
        else if (npts == 6)
          {
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[1]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[2]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          indexArray.push_back(static_cast<unsigned int>(indices[0]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          indexArray.push_back(static_cast<unsigned int>(indices[5]));
          indexArray.push_back(static_cast<unsigned int>(indices[3]));
          indexArray.push_back(static_cast<unsigned int>(indices[4]));
          indexArray.push_back(static_cast<unsigned int>(indices[5]));
          }
        else // 7 sided polygon or higher, do a full smart triangulation
          {
          if (!polygon)
            {
            polygon = vtkPolygon::New();
            tris = vtkIdList::New();
            triPoints = vtkPoints::New();
            }

          vtkIdType *triIndices = new vtkIdType[npts];
          triPoints->SetNumberOfPoints(npts);
          for (int i = 0; i < npts; ++i)
            {
            int idx = indices[i];
            triPoints->SetPoint(i, points->GetPoint(idx));
            triIndices[i] = i;
            }
          polygon->Initialize(npts, triIndices, triPoints);
          polygon->Triangulate(tris);
          for (int j = 0; j < tris->GetNumberOfIds(); ++j)
            {
            indexArray.push_back(static_cast<unsigned int>
                                 (indices[tris->GetId(j)]));
            }
          delete [] triIndices;
          }
        }
      else
        {
        indexArray.push_back(static_cast<unsigned int>(*(indices++)));
        indexArray.push_back(static_cast<unsigned int>(*(indices++)));
        indexArray.push_back(static_cast<unsigned int>(*(indices++)));
        }
      }
    if (polygon)
      {
      polygon->Delete();
      tris->Delete();
      triPoints->Delete();
      }
  }

  //----------------------------------------------------------------------------
  void CreateStripIndexBuffer(vtkCellArray *cells,
                              std::vector<unsigned int> &indexArray,
                              bool wireframeTriStrips)
  {
    if (!cells->GetNumberOfCells())
      {
      return;
      }
    vtkIdType      *pts = 0;
    vtkIdType      npts = 0;

    size_t triCount = cells->GetNumberOfConnectivityEntries()
      - 3*cells->GetNumberOfCells();
    size_t targetSize = wireframeTriStrips ? 2*(triCount*2+1)
      : triCount*3;
    indexArray.reserve(targetSize);

    if (wireframeTriStrips)
      {
      for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
        {
        indexArray.push_back(static_cast<unsigned int>(pts[0]));
        indexArray.push_back(static_cast<unsigned int>(pts[1]));
        for (int j = 0; j < npts-2; ++j)
          {
          indexArray.push_back(static_cast<unsigned int>(pts[j]));
          indexArray.push_back(static_cast<unsigned int>(pts[j+2]));
          indexArray.push_back(static_cast<unsigned int>(pts[j+1]));
          indexArray.push_back(static_cast<unsigned int>(pts[j+2]));
          }
        }
      }
    else
      {
      for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
        {
        for (int j = 0; j < npts-2; ++j)
          {
          indexArray.push_back(static_cast<unsigned int>(pts[j]));
          indexArray.push_back(static_cast<unsigned int>(pts[j+1+j%2]));
          indexArray.push_back(static_cast<unsigned int>(pts[j+1+(j+1)%2]));
          }
        }
      }
  }

  //----------------------------------------------------------------------------
  OSPMaterial VTKToOSPTexture(vtkImageData *vColorTextureMap,
                              OSPRenderer oRenderer)
  {
    int xsize = vColorTextureMap->GetExtent()[1];
    int ysize = vColorTextureMap->GetExtent()[3];
    unsigned char *ichars =
      (unsigned char *)vColorTextureMap->GetScalarPointer();
    unsigned char *ochars = new unsigned char[(xsize+1)*(ysize+1)*3];
    unsigned char *oc = ochars;
    int comps = vColorTextureMap->GetNumberOfScalarComponents();
    int tups = vColorTextureMap->GetPointData()->GetScalars()->GetNumberOfTuples();
    int cntr=0;
    for (int i = 0; i <= xsize; i++)
      {
      for (int j = 0; j <= ysize; j++)
        {
        oc[0] = ichars[0];
        oc[1] = ichars[1];
        oc[2] = ichars[2];
        oc+=3;
        ichars+=comps;
        }
      }
    osp::Texture2D *t2d;
    t2d = (osp::Texture2D*)ospNewTexture2D
      (xsize+1,
       ysize+1,
       OSP_UCHAR3,
       ochars,
       OSP_TEXTURE_FILTER_NEAREST);

    OSPMaterial ospMaterial;
#if OSPRAY_VERSION_MAJOR == 0 && OSPRAY_VERSION_MINOR < 9
    ospMaterial = ospNewMaterial(oRenderer,"OBJMaterial");
#else
    ospMaterial = ospNewMaterial(oRenderer,"RayTraceMaterial");
#endif
    ospSetObject(ospMaterial, "map_Kd", ((OSPTexture2D)(t2d)));
    ospCommit(t2d);
    ospCommit(ospMaterial);
    return ospMaterial;
  }

  //----------------------------------------------------------------------------
  OSPGeometry RenderAsSpheres(OSPModel oModel,
                              std::vector<unsigned int> &indexArray,
                              ospray::vec3fa *vertices,
                              int numColors,
                              ospray::vec4f *colors,
                              int numMaterials,
                              OSPData materialData,
                              int primsize,
                              OSPMaterial oMaterial
                              )
  {
    OSPGeometry ospMesh = ospNewGeometry("spheres");
    float *mdata = new float[4*indexArray.size()];
    int *idata = (int*)mdata;
    for (size_t i = 0; i < indexArray.size(); i++)
      {
      mdata[i*4+0] = (float)vertices[indexArray[i]].x;
      mdata[i*4+1] = (float)vertices[indexArray[i]].y;
      mdata[i*4+2] = (float)vertices[indexArray[i]].z;
      int mat = 0;
      if (numMaterials && primsize>0)
        {
        mat = i/primsize;
        }
      else if (numColors)
        {
        mat = indexArray[i];
        }
      idata[i*4+3] = mat;
      }
    OSPData _colors = NULL;
    if (numColors)
      {
      _colors = ospNewData(numColors, OSP_FLOAT4, &colors[0]);
      }
    OSPData _mdata = ospNewData(indexArray.size()*4, OSP_FLOAT, mdata);
    ospSetObject(ospMesh, "spheres", _mdata);
    ospSet1i(ospMesh, "bytes_per_sphere", 4*sizeof(float));
    ospSet1i(ospMesh, "offset_center", 0*sizeof(float));

    ospSet1f(ospMesh, "radius", 0.05); //TODO: make a function of point size and mesh/camera bounds
    ospSet1i(ospMesh, "offset_radius", -1); //TODO: connect to an array

    if (numMaterials && primsize>0)
      {
      //per cell color
      ospSet1i(ospMesh, "offset_materialID", 3*sizeof(float));
      ospSetData(ospMesh, "materialList", materialData);
      }
    else if (numColors)
      {
      //per point color
      ospSet1i(ospMesh, "offset_colorID", 3*sizeof(float));
      ospSetData(ospMesh, "color", _colors);
      }
    else
      {
      //actor color
      ospSetMaterial(ospMesh, oMaterial);
      }

    ospAddGeometry(oModel, ospMesh);
    ospCommit(ospMesh);
    ospRelease(_colors);
    ospRelease(_mdata);
    delete[] mdata;

    return ospMesh;
  }

  //----------------------------------------------------------------------------
  OSPGeometry RenderAsCylinders(OSPModel oModel,
                                std::vector<unsigned int> &indexArray,
                                ospray::vec3fa *vertices,
                                int numColors,
                                ospray::vec4f *colors,
                                int numMaterials,
                                OSPData materialData,
                                int primsize,
                                OSPMaterial oMaterial)
  {
    OSPGeometry ospMesh = ospNewGeometry("cylinders");
    float *mdata = new float[indexArray.size()/2*7];
    int *idata = (int*)mdata;
    for (size_t i = 0; i < indexArray.size()/2; i++)
      {
      mdata[i*7+0] = (float)vertices[indexArray[i*2+0]].x;
      mdata[i*7+1] = (float)vertices[indexArray[i*2+0]].y;
      mdata[i*7+2] = (float)vertices[indexArray[i*2+0]].z;
      mdata[i*7+3] = (float)vertices[indexArray[i*2+1]].x;
      mdata[i*7+4] = (float)vertices[indexArray[i*2+1]].y;
      mdata[i*7+5] = (float)vertices[indexArray[i*2+1]].z;
      int mat = 0;
      if (numMaterials && primsize>0)
        {
        mat = i;
        if (primsize==3)
          {
          mat = i/3;
          }
        }
      else if (numColors)
        {
        mat = indexArray[i*2];
        }
      idata[i*7+6] = mat;
      }
    OSPData _colors = NULL;
    if (numColors)
      {
      _colors = ospNewData(numColors, OSP_FLOAT4, &colors[0]);
      }
    OSPData _mdata = ospNewData(indexArray.size()/2*7, OSP_FLOAT, mdata);
    ospSetData(ospMesh, "cylinders", _mdata);
    ospSet1i(ospMesh, "bytes_per_cylinder", 7*sizeof(float));
    ospSet1i(ospMesh, "offset_v0", 0);
    ospSet1i(ospMesh, "offset_v1", 3*sizeof(float));

    ospSet1f(ospMesh, "radius", 0.02); //TODO: make a function of line width and mesh/camera bounds
    ospSet1i(ospMesh, "offset_radius", -1); //TODO: connect to an array

    if (numMaterials && primsize > 0)
      {
      //per cell color
      ospSet1i(ospMesh, "offset_materialID", 6*sizeof(float));
      ospSetData(ospMesh, "materialList", materialData);
      }
    else if (numColors)
      {
      //per point color
      ospSet1i(ospMesh, "offset_colorID", 6*sizeof(float));
      ospSetData(ospMesh, "color", _colors);
      }
    else
      {
      //per actor color
      ospSetMaterial(ospMesh, oMaterial);
      }

    ospAddGeometry(oModel, ospMesh);
    ospCommit(ospMesh);
    ospRelease(_colors);
    ospRelease(_mdata);
    delete[] mdata;

    return ospMesh;
  }

  //----------------------------------------------------------------------------
  OSPGeometry RenderAsTriangles(OSPRenderer oRenderer,
                                OSPModel oModel,
                                OSPData position,
                                std::vector<unsigned int> &indexArray,
                                int numNormals,
                                ospray::vec3fa *normals,
                                int numColors,
                                ospray::vec4f *colors,
                                int num_colorCoordinates,
                                float *point_textureCoordinates,
                                int num_textureCoordinates,
                                float *textureCoordinates,
                                vtkImageData *vColorTextureMap,
                                int numMaterials,
                                OSPData materialData,
                                OSPMaterial oMaterial)
  {
    OSPGeometry ospMesh = ospNewGeometry("trianglemesh");
    ospSetData(ospMesh, "position", position);

    size_t numTriangles = indexArray.size() / 3;
    ospray::vec3i *triangles = (ospray::vec3i *)embree::alignedMalloc
      (sizeof(ospray::vec3i) * numTriangles);
    for (size_t i = 0, mi = 0; i < numTriangles; i++, mi += 3)
      {
      triangles[i] = embree::Vec3i(indexArray[mi + 0],
                                   indexArray[mi + 1],
                                   indexArray[mi + 2]);
      }
    OSPData index = ospNewData(numTriangles, OSP_INT3, &triangles[0]);
    embree::alignedFree(triangles);
    ospSetData(ospMesh, "index", index);

    OSPData _normals = NULL;
    if (numNormals)
      {
      _normals = ospNewData(numNormals, OSP_FLOAT3A, &normals[0]);
      ospSetData(ospMesh, "vertex.normal", _normals);
      }

    bool _hastm = false;
    ospray::vec2f *tc = NULL;
    if (num_colorCoordinates || num_textureCoordinates)
      {
      _hastm = true;

      if (num_colorCoordinates)
        {
        tc = (ospray::vec2f *)embree::alignedMalloc
          (sizeof(ospray::vec2f) * num_colorCoordinates);
        for (size_t i = 0; i < num_colorCoordinates; i++)
          {
          tc[i] = ospray::vec2f(point_textureCoordinates[i],0);
          }
        OSPData tcs = ospNewData(num_colorCoordinates, OSP_FLOAT2, &tc[0]);
        ospSetData(ospMesh, "vertex.texcoord", tcs);
        }
      else if (num_textureCoordinates)
        {
        tc = (ospray::vec2f *)embree::alignedMalloc
          (sizeof(ospray::vec2f) * num_textureCoordinates/2);
        float *itc = textureCoordinates;
        for (size_t i = 0; i < num_textureCoordinates; i+=2)
          {
          float t1,t2;
          t1 = *itc;
          itc++;
          t2 = *itc;
          itc++;
          tc[i/2] = ospray::vec2f(t1,t2);
          }
        OSPData tcs = ospNewData(num_textureCoordinates/2, OSP_FLOAT2, &tc[0]);
        ospSetData(ospMesh, "vertex.texcoord", tcs);
        }
      OSPMaterial ospMaterial = VTKToOSPTexture(vColorTextureMap,oRenderer);
      ospSetMaterial(ospMesh, ospMaterial);
      }
    embree::alignedFree(tc);

    OSPData _cmats = NULL;
    OSPData _colors = NULL;
    if (!_hastm)
      {
      int *ids;
      if (numMaterials)
        {
        ids = new int[numTriangles];
        for (size_t i = 0; i < numTriangles; i++)
          {
          ids[i] = i;
          }
        _cmats = ospNewData(numTriangles, OSP_INT, &ids[0]);
        ospSetData(ospMesh, "prim.materialID", _cmats);
        ospSetData(ospMesh, "materialList", materialData);
        }
      else if (numColors)
        {
        _colors = ospNewData(numColors, OSP_FLOAT4, &colors[0]);
        ospSetData(ospMesh, "vertex.color", _colors);
        }
      else
        {
        ospSetMaterial(ospMesh, oMaterial);
        }
      }

    ospAddGeometry(oModel, ospMesh);
    ospCommit(ospMesh);
    ospCommit(oModel); //TODO: crashes without yet others don't, why?
    ospRelease(index);
    ospRelease(_normals);
    ospRelease(_colors);
    ospRelease(_cmats);
    //delete[] ids;
    return ospMesh;
  }
}

//----------------------------------------------------------------------------
void vtkOsprayActorNode::ORenderPoly(void *renderer, void *model,
                                     vtkActor *act, vtkPolyData * poly)
{
  OSPRenderer oRenderer = (OSPRenderer) renderer;
  OSPModel oModel = (OSPModel) model;
  vtkosp::MyGeom *myMeshes = (vtkosp::MyGeom*)this->OSPMeshes;

  if (this->RenderTime > act->GetMTime() &&
      this->RenderTime > poly->GetMTime())
    {
    myMeshes->AddMyselfTo(oModel);
    return;
    }
  delete myMeshes;
  myMeshes = new vtkosp::MyGeom();
  this->OSPMeshes = myMeshes;

  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  std::vector<unsigned int> pIndexArray;
  prims[1] =  poly->GetLines();
  std::vector<unsigned int> lIndexArray;
  prims[2] =  poly->GetPolys();
  std::vector<unsigned int> tIndexArray;
  prims[3] =  poly->GetStrips();
  std::vector<unsigned int> sIndexArray;

  CreatePointIndexBuffer(prims[0], pIndexArray);
  switch (this->Representation)
    {
    case VTK_POINTS:
      {
      CreatePointIndexBuffer(prims[1], lIndexArray);
      CreatePointIndexBuffer(prims[2], tIndexArray);
      CreatePointIndexBuffer(prims[3], sIndexArray);
      break;
      }
    case VTK_WIREFRAME:
      {
      CreateLineIndexBuffer(prims[1], lIndexArray);
      CreateTriangleLineIndexBuffer(prims[2], tIndexArray);
      CreateStripIndexBuffer(prims[3], sIndexArray, true);
      break;
      }
    default:
      {
      CreateLineIndexBuffer(prims[1], lIndexArray);
      CreateTriangleIndexBuffer(prims[2], poly->GetPoints(), tIndexArray);
      CreateStripIndexBuffer(prims[3], sIndexArray, false);
      }
    }

  std::vector<vtkosp::Vec3> _vertices;
  vtkSmartPointer<vtkMatrix4x4> m = vtkSmartPointer<vtkMatrix4x4>::New();
  act->GetMatrix(m);
  //see if we can avoid some work
  double Ident[4][4] = {{1.0,0.0,0.0,0.0},
                        {0.0,1.0,0.0,0.0},
                        {0.0,0.0,1.0,0.0},
                        {0.0,0.0,0.0,1.0}};
  bool ident = true;
  double *iptr = &Ident[0][0];
  double *mptr = &m->Element[0][0];
  for (int i = 0; i < 16; i++)
    {
    if (*iptr != *mptr)
      {
      ident = false;
      }
    }
  double inPos[4];
  inPos[3] = 1.0;
  double transPos[4];
  for (int i = 0; i < poly->GetNumberOfPoints(); i++)
    {
    double *pos = poly->GetPoints()->GetPoint(i);
    bool wasNan = false;
    int fixIndex = i - 1;
    do
      {
      wasNan = false;
      for (int j = 0; j < 3; j++)
        {
        if (std::isnan(pos[j]))
          {
          wasNan = true;
          }
        }
      if (wasNan && fixIndex >= 0)
        {
        pos = poly->GetPoints()->GetPoint(fixIndex--);
        }
      } while (wasNan == true && fixIndex >= 0);
    if (ident)
      {
      _vertices.push_back(vtkosp::Vec3(pos[0], pos[1], pos[2]));
      }
    else
      {
      inPos[0] = pos[0];
      inPos[1] = pos[1];
      inPos[2] = pos[2];
      m->MultiplyPoint(inPos, transPos);
      //alternatively use an OSPRay instance of something like that
      _vertices.push_back(vtkosp::Vec3(transPos[0], transPos[1], transPos[2]));
      }
    }

  OSPMaterial oMaterial;
#if OSPRAY_VERSION_MAJOR == 0 && OSPRAY_VERSION_MINOR < 9
  oMaterial = ospNewMaterial(oRenderer,"OBJMaterial");
#else
  oMaterial = ospNewMaterial(oRenderer,"RayTraceMaterial");
#endif
  float diffusef[] = {(float)this->DiffuseColor[0],
                      (float)this->DiffuseColor[1],
                      (float)this->DiffuseColor[2]};
  float specularf[] = {(float)this->SpecularColor[0],
                       (float)this->SpecularColor[1],
                       (float)this->SpecularColor[2]};
  ospSet3fv(oMaterial,"Kd",diffusef);
  ospSet3fv(oMaterial,"Ks",specularf);
  ospSet1f(oMaterial,"Ns",float(this->SpecularPower*.5));
  ospSet1f(oMaterial,"d",float(this->Opacity));
  ospCommit(oMaterial);

  //direct colors
  int cellFlag;
  std::vector<OSPMaterial> ospMaterials;
  int num_materials = 0;
  ospray::vec4f *point_colors = NULL;
  int num_colors = 0;
  float *point_textureCoordinates = NULL;
  int num_colorCoordinates = 0;
  act->GetMapper()->MapScalars(1.0, cellFlag);
  vtkUnsignedCharArray *vColors = act->GetMapper()->Colors;
  vtkFloatArray *vColorCoordinates = act->GetMapper()->ColorCoordinates;
  vtkImageData* vColorTextureMap = act->GetMapper()->ColorTextureMap;
  if (vColors)
    {
    if (cellFlag)
      {
      vtkosp::CellColorMaterials(vColors, oRenderer, ospMaterials);
      num_materials = ospMaterials.size();
      }
    else
      {
      num_colors = vColors->GetNumberOfTuples();
      point_colors = new ospray::vec4f[num_colors];
      for (int i = 0; i < num_colors; i++)
        {
        unsigned char *color = vColors->GetPointer(4 * i);
        point_colors[i] = ospray::vec4f(color[0] / 255.0,
                                  color[1] / 255.0,
                                  color[2] / 255.0,
                                  1);
        }
      }
    }
  else
    {
    if (vColorCoordinates && vColorTextureMap)
      {
      num_colorCoordinates = vColorCoordinates->GetNumberOfTuples();
      point_textureCoordinates = new float[num_colorCoordinates];
      float *tc = vColorCoordinates->GetPointer(0);
      for (int i = 0; i < num_colorCoordinates; i++)
        {
        point_textureCoordinates[i] = *tc;
        tc+=2;
        }
      }
    }
  OSPData materialData =
    ospNewData(ospMaterials.size(), OSP_OBJECT, &ospMaterials[0]);
  ospCommit(materialData);

  vtkTexture *texture = act->GetTexture();
  float *textureCoordinates = NULL;
  int num_textureCoordinates = 0;
  if (texture)
    {
    vtkDataArray *da = poly->GetPointData()->GetTCoords();
    num_textureCoordinates = da->GetNumberOfTuples();
    textureCoordinates = new float[num_textureCoordinates*2];
    float *tp = textureCoordinates;
    for (int i=0; i<num_textureCoordinates; i++)
      {
      *tp = (float)da->GetTuple(i)[0];
      tp++;
      *tp = (float)da->GetTuple(i)[1];
      tp++;
      }
    vColorTextureMap = vtkImageData::SafeDownCast
      (texture->GetInput());
    num_textureCoordinates = num_textureCoordinates*2;
    }
  //points
  size_t numPositions = _vertices.size();
  ospray::vec3fa *vertices = (ospray::vec3fa *)embree::alignedMalloc
    (sizeof(ospray::vec3fa) * numPositions);
  for (size_t i = 0; i < numPositions; i++)
    {
    vertices[i] =
      ospray::vec3fa(_vertices[i].x(),
                     _vertices[i].y(),
                     _vertices[i].z());
    }
  OSPData position = ospNewData(numPositions, OSP_FLOAT3A, &vertices[0]);
  ospCommit(position);

  //create an ospray mesh for the vertex cells
  if (pIndexArray.size())
    {
    myMeshes
      ->Add(RenderAsSpheres(oModel,
                            pIndexArray, vertices,
                            num_colors, point_colors,
                            num_materials, materialData, 1,
                            oMaterial));
    }


  //create an ospray mesh for the line cells
  if (lIndexArray.size())
    {
    //format depends on representation style
    if (this->Representation == VTK_POINTS)
      {
      myMeshes
        ->Add(RenderAsSpheres(oModel,
                              lIndexArray, vertices,
                              num_colors, point_colors,
                              num_materials, materialData, 2,
                              oMaterial));
      }
    else
      {
      myMeshes
        ->Add(RenderAsCylinders(oModel,
                                lIndexArray, vertices,
                                num_colors, point_colors,
                                num_materials, materialData, 2,
                                oMaterial));
      }
    }

  //create an ospray mesh for the polygon cells
  if (tIndexArray.size())
    {
    //format depends on representation style
    switch (this->Representation)
      {
      case VTK_POINTS:
        {
        myMeshes
          ->Add(RenderAsSpheres(oModel,
                                tIndexArray, vertices,
                                num_colors, point_colors,
                                num_materials, materialData, 3,
                                oMaterial));
        break;
        }
      case VTK_WIREFRAME:
        {
        myMeshes
          ->Add(RenderAsCylinders(oModel,
                                  tIndexArray, vertices,
                                  num_colors, point_colors,
                                  num_materials, materialData, 3,
                                  oMaterial));
        break;
        }
      default:
        {
        ospray::vec3fa *normals = NULL;
        int numNormals = 0;
        if (this->Interpolation != VTK_FLAT)
          {
          vtkDataArray *vNormals = poly->GetPointData()->GetNormals();
          if (vNormals)
            {
            vtkosp::PointNormalsOntoIndexes
              (tIndexArray, vNormals, normals);
            numNormals = tIndexArray.size();
            }
          }
        myMeshes
          ->Add(RenderAsTriangles(oRenderer, oModel,
                                  position, tIndexArray,
                                  numNormals, normals,
                                  num_colors, point_colors,
                                  num_colorCoordinates, point_textureCoordinates,
                                  num_textureCoordinates, textureCoordinates,
                                  vColorTextureMap,
                                  num_materials, materialData,
                                  oMaterial));
        delete[] normals;
        }
      }
    }

  if (sIndexArray.size())
    {
    switch (this->Representation)
      {
      case VTK_POINTS:
        {
        myMeshes
          ->Add(RenderAsSpheres(oModel,
                                sIndexArray, vertices,
                                num_colors, point_colors,
                                num_materials, materialData, -1,
                                oMaterial));
        break;
        }
      case VTK_WIREFRAME:
        {
        myMeshes
          ->Add(RenderAsCylinders(oModel,
                                  sIndexArray, vertices,
                                  num_colors, point_colors,
                                  num_materials, materialData, -1,
                                  oMaterial));
        break;
        }
      default:
        {
        ospray::vec3fa *normals = NULL;
        int numNormals = 0;
        if (this->Interpolation != VTK_FLAT)
          {
          vtkDataArray *vNormals = poly->GetPointData()->GetNormals();
          if (vNormals)
            {
            vtkosp::PointNormalsOntoIndexes
              (sIndexArray, vNormals, normals);
            numNormals = sIndexArray.size();
            }
          }
        myMeshes
          ->Add(RenderAsTriangles(oRenderer, oModel,
                                  position, sIndexArray,
                                  numNormals, normals,
                                  num_colors, point_colors,
                                  num_colorCoordinates, point_textureCoordinates,
                                  num_textureCoordinates, textureCoordinates,
                                  vColorTextureMap,
                                  num_materials, materialData,
                                  oMaterial));
        delete[] normals;
        }
      }
    }
  ospRelease(position);
  embree::alignedFree(vertices);
  ospMaterials.clear();
  delete[] point_colors;
  delete[] point_textureCoordinates;
}

//----------------------------------------------------------------------------
void vtkOsprayActorNode::ORender(void *renderer, void *model)
{
  if (this->Visibility == false)
    {
    delete (vtkosp::MyGeom*)this->OSPMeshes;
    return;
    }

  vtkActor *act = (vtkActor*)this->GetRenderable();
  vtkPolyData *poly = (vtkPolyData*)(act->GetMapper()->GetInput());
  if (poly)
    {
    this->ORenderPoly(renderer, model, act, poly);
    }
  else
    {
    vtkMapper *cpdm = act->GetMapper();
    vtkDataObject * dobj = cpdm->GetInputDataObject(0, 0);
    if (dobj)
      {
      vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast
        (cpdm->GetInputDataObject(0, 0));
      if (input)
        {
        vtkCompositeDataIterator*dit = input->NewIterator();
        dit->SkipEmptyNodesOn();
        while(!dit->IsDoneWithTraversal())
          {
          poly = vtkPolyData::SafeDownCast(input->GetDataSet(dit));
          if (poly)
            {
            this->ORenderPoly(renderer, model, act, poly);
            }
          dit->GoToNextItem();
          }
        dit->Delete();
        }
      }
    }
  this->RenderTime.Modified();
}
