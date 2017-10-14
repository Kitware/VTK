/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXPolyDataMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOptiXPolyDataMapperNode.h"

#include "vtkProperty.h"
#include "vtkActor.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOptiXActorNode.h"
#include "vtkOptiXRendererNode.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkOptiXPtxLoader.h"

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>

#include <map>

//============================================================================

//------------------------------------------------------------------------------
class vtkOptiXPolyDataMapperNode::Geom
{
//A cache for the OptiX meshes we make for this actor.
//When something else in the scene changes but this actor doesn't we
//reuse instead of recreating. RendererNode has a higher level cache
//that prevent spatial sorting when nothing changes other than camera.
public:
  std::vector<optix::GeometryInstance> gis;
  std::vector<optix::Buffer>           buffers;
  std::vector<optix::TextureSampler>   samplers;

  Geom()
  {
  }

  ~Geom()
  {
    for (std::vector<optix::GeometryInstance>::iterator it = gis.begin();
      it != gis.end();
      ++it)
    {
      (*it)->getGeometry()->destroy();
      for (unsigned int i = 0; i < (*it)->getMaterialCount(); ++i)
      {
        (*it)->getMaterial(i)->destroy();
      }
      (*it)->destroy();
    }

    for (std::vector<optix::Buffer>::iterator it = buffers.begin();
      it != buffers.end();
      ++it)
    {
      (*it)->destroy();
    }

    for (std::vector<optix::TextureSampler>::iterator it = samplers.begin();
      it != samplers.end();
      ++it)
    {
      (*it)->destroy();
    }
  }

  void Add( optix::GeometryInstance gi )
  {
    gis.push_back( gi );
  }

  void AddBuffer( optix::Buffer buffer )
  {
    buffers.push_back( buffer );
  }

  void AddSampler(optix::TextureSampler sampler)
  {
    samplers.push_back(sampler);
  }

  void AddMyselfTo( optix::GeometryGroup geom_group )
  {
    for( std::vector<optix::GeometryInstance>::iterator it = gis.begin();
      it != gis.end();
      ++it )
    {
      geom_group->addChild( *it );
    }
  }
};

//------------------------------------------------------------------------------
optix::Buffer vtkDataArrayToBuffer3( vtkDataArray* vdata,
  optix::Context ctx,
  vtkOptiXPolyDataMapperNode::Geom* my_geom )
{
  const int num_tuples = vdata ? vdata->GetNumberOfTuples() : 0;
  optix::Buffer buff = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT3,
    num_tuples
    );

  optix::float3* buff_data = reinterpret_cast<optix::float3*>( buff->map() );
  for (int i = 0; i < num_tuples; i++)
  {
    const double* tuple = vdata->GetTuple(i);
    buff_data[i] = optix::make_float3 (
      static_cast<float>(tuple[0]),
      static_cast<float>(tuple[1]),
      static_cast<float>(tuple[2])
      );
  }
  buff->unmap();

  my_geom->AddBuffer(buff);

  return buff;
}

//------------------------------------------------------------------------------
float MapThroughPWF(double in, vtkPiecewiseFunction *scaleFunction)
{
  double out = in;
  if (!scaleFunction)
  {
    out = in;
  }
  else
  {
    out = scaleFunction->GetValue(in);
  }
  return static_cast<float>(out);
}

void ToOptiXTextureData(vtkImageData *vColorTextureMap,
  int numTextureCoordinates,
  float *textureCoordinates,
  unsigned int numPointValueTextureCoords,
  float *pointValueTextureCoords,
  int xSize,
  int ySize,
  optix::Buffer& texCoord_buffer,
  optix::Buffer& texColor_buffer)
{
  //Texture coordinates
  optix::float2* tc =
    reinterpret_cast<optix::float2*>(texCoord_buffer->map());

  if (numPointValueTextureCoords)
  {
    //using 1D texture for point value LUT

    for (size_t i = 0; i < numPointValueTextureCoords; i++)
    {
      tc[i] = optix::make_float2(pointValueTextureCoords[i], 0);
    }
  }
  else if (numTextureCoordinates)
  {
    //2d texture mapping
    float *itc = textureCoordinates;
    for (size_t i = 0; i < numTextureCoordinates; i += 2)
    {
      float t1, t2;
      t1 = *itc;
      itc++;
      t2 = *itc;
      itc++;
      tc[i / 2] = optix::make_float2(t1, t2);
    }
  }

  texCoord_buffer->unmap();

  //Texture colors
  optix::uchar4* texColors =
    reinterpret_cast<optix::uchar4*>(texColor_buffer->map());

  unsigned char *ichars =
    (unsigned char *)vColorTextureMap->GetScalarPointer();
  int comps = vColorTextureMap->GetNumberOfScalarComponents();
  optix::uchar4* ot = texColors;
  for (int i = 0; i <= xSize; i++)
  {
    for (int j = 0; j <= ySize; j++)
    {
      *ot = optix::make_uchar4(ichars[0], ichars[1], ichars[2], 255);
      ++ot;
      ichars += comps;
    }
  }

  texColor_buffer->unmap();
}

//------------------------------------------------------------------------------
void RenderAsSpheres(optix::Context ctx,
  optix::Material matl,
  std::vector<double> vertices,
  unsigned int numVertices,
  std::vector<unsigned int> &indexArray,
  std::vector<unsigned int> &rIndexArray,
  float pointSize,
  vtkDataArray* scaleArray,
  vtkPiecewiseFunction *scaleFunction,
  vtkImageData *vColorTextureMap,
  int numTextureCoordinates,
  float *textureCoordinates,
  unsigned int numPointColors,
  optix::float4* PointColors, // Point and cell colors of length numVertices
  unsigned int numCellColors,
  optix::float3* CellColors,
  unsigned int numPointValueTextureCoords,
  float *pointValueTextureCoords,
  vtkOptiXPolyDataMapperNode::Geom* my_geom,
  vtkOptiXPtxLoader* ptxLoader
  )
{
  assert(numPointColors == 0 || numPointColors == numVertices);
  assert(indexArray.size() == rIndexArray.size());

  // Sphere center and scale buffer
  optix::Buffer sphere_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT4,
    indexArray.size()
    );
  my_geom->AddBuffer(sphere_buffer);

  optix::float4* sphere_data =
    reinterpret_cast<optix::float4*>(sphere_buffer->map());
  for (size_t i = 0; i < numVertices; i++)
  {
    float scale = 1.0f;
    if (scaleArray != nullptr)
    {
      scale = MapThroughPWF(*scaleArray->GetTuple(i), scaleFunction);
    }

    sphere_data[i] = optix::make_float4(
      static_cast<float>(vertices[i * 3 + 0]),
      static_cast<float>(vertices[i * 3 + 1]),
      static_cast<float>(vertices[i * 3 + 2]),
      scale
      );
  }
  sphere_buffer->unmap();

  // Geometry
  optix::Geometry geometry = ctx->createGeometry();
  geometry->setPrimitiveCount(numVertices);
  geometry->setIntersectionProgram(
    ptxLoader->SphereIsectProgram
    );
  geometry->setBoundingBoxProgram(
    ptxLoader->SphereBoundsProgram
    );

  geometry["spheres"]->setBuffer(sphere_buffer);

  //texture for material
  optix::Buffer texCoord_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT2,
    (numPointValueTextureCoords ?
      numPointValueTextureCoords : numTextureCoordinates / 2)
  );
  my_geom->AddBuffer(texCoord_buffer);

  int xsize = vColorTextureMap ? vColorTextureMap->GetExtent()[1] : 0;
  int ysize = vColorTextureMap ? vColorTextureMap->GetExtent()[3] : 0;

  optix::Buffer texColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_UNSIGNED_BYTE4,
    xsize + 1, ysize + 1
  );
  my_geom->AddBuffer(texColor_buffer);

  optix::TextureSampler tex_sampler = ctx->createTextureSampler();
  my_geom->AddSampler(tex_sampler);

  tex_sampler->setWrapMode(0, RT_WRAP_CLAMP_TO_EDGE);
  tex_sampler->setWrapMode(1, RT_WRAP_CLAMP_TO_EDGE);
  tex_sampler->setFilteringModes(RT_FILTER_LINEAR,
    RT_FILTER_LINEAR,
    RT_FILTER_NONE);
  tex_sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
  tex_sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);
  tex_sampler->setMaxAnisotropy(1.0f);

  tex_sampler->setBuffer(texColor_buffer);

  bool _hastm = false;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    ToOptiXTextureData(vColorTextureMap,
      numTextureCoordinates,
      textureCoordinates,
      numPointValueTextureCoords,
      pointValueTextureCoords,
      xsize,
      ysize,
      texCoord_buffer,
      texColor_buffer);
  }

  geometry["texcoords"]->setBuffer(texCoord_buffer);
  matl["colorTexture"]->setTextureSampler(tex_sampler);

  bool hasCellColorBuffer = !_hastm && numCellColors != 0;
  optix::Buffer cellColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT3,
    (hasCellColorBuffer ? numVertices : 0)
    );
  my_geom->AddBuffer(cellColor_buffer);

  bool hasVertexColorBuffer = !hasCellColorBuffer && (numPointColors != 0);
  optix::Buffer vertColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT4,
    (hasVertexColorBuffer ? numPointColors : 0)
    );
  my_geom->AddBuffer(vertColor_buffer);

  if (hasCellColorBuffer)
  {
    optix::float3* vc =
      reinterpret_cast<optix::float3*>(cellColor_buffer->map());

    for (size_t i = 0; i < indexArray.size(); i++)
    {
      unsigned int vertIndex = indexArray[i];
      unsigned int rCellIndex = rIndexArray[i];
      assert(vertIndex < numVertices);
      assert(rCellIndex < numCellColors);

      vc[vertIndex] = CellColors[rCellIndex];
    }

    cellColor_buffer->unmap();
  }
  else if (hasVertexColorBuffer)
  {
    optix::float4* vc =
      reinterpret_cast<optix::float4*>(vertColor_buffer->map());

    memcpy(vc, PointColors, sizeof(PointColors[0])*numPointColors);

    vertColor_buffer->unmap();
  }

  matl["texture_enabled"]->setInt(_hastm);
  matl["cellcolors_enabled"]->setInt(hasCellColorBuffer);
  matl["vertexcolors_enabled"]->setInt(hasVertexColorBuffer);
  matl["cellcolors"]->setBuffer(cellColor_buffer);
  geometry["vertexcolors"]->setBuffer(vertColor_buffer);
  geometry["sphere_radius"]->setFloat((scaleArray != nullptr) ? 1.0f : pointSize);

  my_geom->Add(ctx->createGeometryInstance(geometry, &matl, &matl + 1));
}

//------------------------------------------------------------------------------
void RenderAsCylinders(optix::Context ctx,
  optix::Material matl,
  std::vector<double> vertices,
  unsigned int numVertices,
  std::vector<unsigned int> &indexArray,
  std::vector<unsigned int> &rIndexArray,
  float lineWidth,
  vtkDataArray* scaleArray,
  vtkPiecewiseFunction *scaleFunction,
  vtkImageData *vColorTextureMap,
  int numTextureCoordinates,
  float *textureCoordinates,
  unsigned int numPointColors,
  optix::float4* PointColors, // Point and cell colors of length numVertices
  unsigned int numCellColors,
  optix::float3* CellColors,
  unsigned int numPointValueTextureCoords,
  float *pointValueTextureCoords,
  vtkOptiXPolyDataMapperNode::Geom* my_geom,
  vtkOptiXPtxLoader* ptxLoader
  )
{
  assert(numPointColors == 0 || numPointColors == numVertices);

  // Triangle buffer
  const size_t numLines = indexArray.size() / 2;
  optix::Buffer line_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_INT2,
    numLines
    );
  my_geom->AddBuffer(line_buffer);

  optix::int2* line_buffer_data =
    reinterpret_cast<optix::int2*>(line_buffer->map());
  for (size_t i = 0; i < numLines; i++)
  {
    line_buffer_data[i] = optix::make_int2(
      static_cast<int>(indexArray[i * 2 + 0]),
      static_cast<int>(indexArray[i * 2 + 1])
      );
  }
  line_buffer->unmap();

  // Vertex buffer
  optix::Buffer vert_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT4,
    numVertices
    );
  my_geom->AddBuffer(vert_buffer);

  optix::float4* vert_buffer_data =
    reinterpret_cast<optix::float4*>(vert_buffer->map());
  for (size_t i = 0; i < numVertices; i++)
  {
    float scale = 1.0f;
    if (scaleArray != nullptr)
    {
      scale = MapThroughPWF(*scaleArray->GetTuple(i), scaleFunction);
    }

    vert_buffer_data[i] = optix::make_float4(
      static_cast<float>(vertices[i * 3 + 0]),
      static_cast<float>(vertices[i * 3 + 1]),
      static_cast<float>(vertices[i * 3 + 2]),
      scale
      );
  }
  vert_buffer->unmap();

  // Geometry
  optix::Geometry geometry = ctx->createGeometry();
  geometry->setPrimitiveCount((unsigned int)(numLines));
  geometry->setIntersectionProgram(
    ptxLoader->CylinderIsectProgram
    );
  geometry->setBoundingBoxProgram(
    ptxLoader->CylinderBoundsProgram
    );

  geometry["vertices"]->setBuffer(vert_buffer);
  geometry["lines"]->setBuffer(line_buffer);

  //texture for material
  optix::Buffer texCoord_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT2,
    (numPointValueTextureCoords ?
      numPointValueTextureCoords : numTextureCoordinates / 2)
  );
  my_geom->AddBuffer(texCoord_buffer);

  int xsize = vColorTextureMap ? vColorTextureMap->GetExtent()[1] : 0;
  int ysize = vColorTextureMap ? vColorTextureMap->GetExtent()[3] : 0;

  optix::Buffer texColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_UNSIGNED_BYTE4,
    xsize + 1, ysize + 1
  );
  my_geom->AddBuffer(texColor_buffer);

  optix::TextureSampler tex_sampler = ctx->createTextureSampler();
  my_geom->AddSampler(tex_sampler);

  tex_sampler->setWrapMode(0, RT_WRAP_CLAMP_TO_EDGE);
  tex_sampler->setWrapMode(1, RT_WRAP_CLAMP_TO_EDGE);
  tex_sampler->setFilteringModes(RT_FILTER_LINEAR,
    RT_FILTER_LINEAR,
    RT_FILTER_NONE);
  tex_sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
  tex_sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);
  tex_sampler->setMaxAnisotropy(1.0f);

  tex_sampler->setBuffer(texColor_buffer);

  bool _hastm = false;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    ToOptiXTextureData(vColorTextureMap,
      numTextureCoordinates,
      textureCoordinates,
      numPointValueTextureCoords,
      pointValueTextureCoords,
      xsize,
      ysize,
      texCoord_buffer,
      texColor_buffer);
  }

  geometry["texcoords"]->setBuffer(texCoord_buffer);
  matl["colorTexture"]->setTextureSampler(tex_sampler);

  bool hasCellColorBuffer = !_hastm && numCellColors != 0;
  optix::Buffer cellColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT3,
    (hasCellColorBuffer ? numLines : 0)
    );
  my_geom->AddBuffer(cellColor_buffer);

  bool hasVertexColorBuffer = !hasCellColorBuffer && (numPointColors != 0);
  optix::Buffer vertColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT4,
    (hasVertexColorBuffer ? numPointColors : 0)
    );
  my_geom->AddBuffer(vertColor_buffer);

  if (hasCellColorBuffer)
  {
    optix::float3* vc =
      reinterpret_cast<optix::float3*>(cellColor_buffer->map());

    for (size_t i = 0; i < numLines; i++)
    {
      unsigned int rCellIndex = rIndexArray[i*2];
      assert(rCellIndex < numCellColors);

      vc[i] = CellColors[rCellIndex];
    }

    cellColor_buffer->unmap();
  }
  else if (hasVertexColorBuffer)
  {
    optix::float4* vc =
      reinterpret_cast<optix::float4*>(vertColor_buffer->map());

    memcpy(vc, PointColors, sizeof(PointColors[0])*numPointColors);

    vertColor_buffer->unmap();
  }

  // General parameters
  matl["texture_enabled"]->setInt(_hastm);
  matl["cellcolors_enabled"]->setInt(hasCellColorBuffer);
  matl["vertexcolors_enabled"]->setInt(hasVertexColorBuffer);
  matl["cellcolors"]->setBuffer(cellColor_buffer);
  geometry["vertexcolors"]->setBuffer(vertColor_buffer);
  geometry["cylinder_radius"]->setFloat(
    (scaleArray != nullptr) ? 1.0f : lineWidth );

  my_geom->Add(ctx->createGeometryInstance(geometry, &matl, &matl + 1));
}

//------------------------------------------------------------------------------
void RenderAsTriangles(
  optix::Context ctx,
  optix::Material matl,
  std::vector<double> vertices,
  unsigned int numVertices,
  vtkDataArray* vNormals,
  vtkImageData *vColorTextureMap,
  int numTextureCoordinates,
  float *textureCoordinates,
  unsigned int numPointColors,
  optix::float4* PointColors,
  unsigned int numCellColors,
  optix::float3* CellColors,
  unsigned int numPointValueTextureCoords,
  float *pointValueTextureCoords,
  std::vector<unsigned int> &indexArray,
  std::vector<unsigned int> &rIndexArray,
  vtkOptiXPolyDataMapperNode::Geom* my_geom,
  vtkOptiXPtxLoader* ptxLoader
  )
{
  assert(numPointColors == 0 || numPointColors == numVertices);

  // Triangle buffer
  const size_t numTriangles = indexArray.size() / 3;
  optix::Buffer tri_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_INT3,
    numTriangles
    );
  my_geom->AddBuffer( tri_buffer );

  optix::int3* tri_buffer_data =
    reinterpret_cast<optix::int3*>( tri_buffer->map() );
  for( size_t i = 0; i < numTriangles; i++ )
  {
    tri_buffer_data[i] = optix::make_int3(
      static_cast<int>(indexArray[i*3+0]),
      static_cast<int>(indexArray[i*3+1]),
      static_cast<int>(indexArray[i*3+2])
      );
  }
  tri_buffer->unmap();

  // Vertex buffer
  optix::Buffer vert_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT3,
    numVertices);
  my_geom->AddBuffer(vert_buffer);

  optix::float3* vert_buffer_data =
    reinterpret_cast<optix::float3*>(vert_buffer->map());
  for (size_t i = 0; i < numVertices; i++)
  {
    vert_buffer_data[i] = optix::make_float3(
      static_cast<float>(vertices[i * 3 + 0]),
      static_cast<float>(vertices[i * 3 + 1]),
      static_cast<float>(vertices[i * 3 + 2])
      );
  }
  vert_buffer->unmap();

  // Normal buffer
  optix::Buffer norm_buffer = vtkDataArrayToBuffer3(vNormals, ctx, my_geom);

  // Generate geometry
  optix::Geometry geometry = ctx->createGeometry();
  geometry->setPrimitiveCount( (unsigned int)(numTriangles) );
  geometry->setIntersectionProgram(
    ptxLoader->TriangleIsectProgram
    );
  geometry->setBoundingBoxProgram(
    ptxLoader->TriangleBoundsProgram
    );
  geometry[ "vertices"  ]->setBuffer( vert_buffer );
  geometry[ "normals"   ]->setBuffer( norm_buffer );
  geometry[ "triangles" ]->setBuffer( tri_buffer );

  //send the texture map and texture coordiantes over
  optix::Buffer texCoord_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_FLOAT2,
    (numPointValueTextureCoords ?
      numPointValueTextureCoords : numTextureCoordinates / 2)
    );
  my_geom->AddBuffer(texCoord_buffer);

  int xsize = vColorTextureMap ? vColorTextureMap->GetExtent()[1] : 0;
  int ysize = vColorTextureMap ? vColorTextureMap->GetExtent()[3] : 0;

  optix::Buffer texColor_buffer = ctx->createBuffer(
    RT_BUFFER_INPUT,
    RT_FORMAT_UNSIGNED_BYTE4,
    xsize+1, ysize+1
    );
  my_geom->AddBuffer(texColor_buffer);

  optix::TextureSampler tex_sampler = ctx->createTextureSampler();
  my_geom->AddSampler(tex_sampler);

  tex_sampler->setWrapMode( 0, RT_WRAP_CLAMP_TO_EDGE );
  tex_sampler->setWrapMode( 1, RT_WRAP_CLAMP_TO_EDGE );
  tex_sampler->setFilteringModes( RT_FILTER_LINEAR,
    RT_FILTER_LINEAR,
    RT_FILTER_NONE);
  tex_sampler->setIndexingMode( RT_TEXTURE_INDEX_NORMALIZED_COORDINATES );
  tex_sampler->setReadMode( RT_TEXTURE_READ_NORMALIZED_FLOAT );
  tex_sampler->setMaxAnisotropy( 1.0f );

  tex_sampler->setBuffer(texColor_buffer);

  bool _hastm = false;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    ToOptiXTextureData(vColorTextureMap,
      numTextureCoordinates,
      textureCoordinates,
      numPointValueTextureCoords,
      pointValueTextureCoords,
      xsize,
      ysize,
      texCoord_buffer,
      texColor_buffer);
  }

  geometry["texcoords"]->setBuffer(texCoord_buffer);
  matl["colorTexture"]->setTextureSampler(tex_sampler);

  bool hasCellColorBuffer = !_hastm && numCellColors;
  optix::Buffer cellColor_buffer =
    ctx->createBuffer(
      RT_BUFFER_INPUT,
      RT_FORMAT_FLOAT3,
      (hasCellColorBuffer ? numTriangles : 0)
    );
  my_geom->AddBuffer(cellColor_buffer);

  bool hasVertexColorBuffer = !hasCellColorBuffer && numPointColors;
  optix::Buffer vertColor_buffer =
    ctx->createBuffer(
      RT_BUFFER_INPUT,
      RT_FORMAT_FLOAT4,
      (hasVertexColorBuffer ? numPointColors : 0)
    );
  my_geom->AddBuffer(vertColor_buffer);

  if (hasCellColorBuffer)
  {
    optix::float3* vc =
      reinterpret_cast<optix::float3*>(cellColor_buffer->map());

    for (size_t i = 0; i < numTriangles; i++)
    {
      assert(rIndexArray[i * 3 + 0] < numCellColors);
      vc[i] = CellColors[rIndexArray[i * 3 + 0]];
    }

    cellColor_buffer->unmap();
  }
  else if (hasVertexColorBuffer)
  {
    optix::float4* vc =
      reinterpret_cast<optix::float4*>(vertColor_buffer->map());

    memcpy(vc, PointColors, sizeof(PointColors[0])*numPointColors);

    vertColor_buffer->unmap();
  }

  matl["texture_enabled"]->setInt(_hastm);
  matl["cellcolors_enabled"]->setInt(hasCellColorBuffer);
  matl["vertexcolors_enabled"]->setInt(hasVertexColorBuffer);
  matl["cellcolors"]->setBuffer(cellColor_buffer);
  geometry["vertexcolors"]->setBuffer(vertColor_buffer);

  my_geom->Add( ctx->createGeometryInstance( geometry, &matl, &matl+1 ) );
}

//============================================================================
vtkStandardNewMacro(vtkOptiXPolyDataMapperNode);

//------------------------------------------------------------------------------
vtkOptiXPolyDataMapperNode::vtkOptiXPolyDataMapperNode()
{
  this->MyGeom = 0;
}

//------------------------------------------------------------------------------
vtkOptiXPolyDataMapperNode::~vtkOptiXPolyDataMapperNode()
{
  delete this->MyGeom;
}

//------------------------------------------------------------------------------
void vtkOptiXPolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOptiXPolyDataMapperNode::RenderPoly(
  vtkOptiXRendererNode* orn,
  vtkOptiXActorNode*    aNode,
  vtkPolyData*          poly,
  double*         ambientColor,
  double*         diffuseColor
  )
{
  optix::Context ctx = orn->GetOptiXContext();
  vtkOptiXPtxLoader* ptxLoader = orn->GetOptiXPtxLoader();

  vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());
  vtkProperty* property = act->GetProperty();

  std::vector<double> vertices;
  vtkPolyDataMapperNode::TransformPoints( act, poly, vertices );
  const size_t numPositions = vertices.size() / 3;

  //
  //make connectivity
  //
  std::vector<unsigned int> vertex_index;
  std::vector<unsigned int> vertex_reverse;
  std::vector<unsigned int> line_index;
  std::vector<unsigned int> line_reverse;
  std::vector<unsigned int> triangle_index;
  std::vector<unsigned int> triangle_reverse;
  std::vector<unsigned int> strip_index;
  std::vector<unsigned int> strip_reverse;
  vtkPDConnectivity conn;
  vtkPolyDataMapperNode::MakeConnectivity(
    poly, property->GetRepresentation(), conn);

  vtkMapper *mapper = act->GetMapper();
  double length = 1.0;
  if (mapper)
  {
    length = mapper->GetLength();
  }
  float pointSize = length / 1000.0 * property->GetPointSize();
  float lineWidth = length / 1000.0 * property->GetLineWidth();
  //finer control over sphere and cylinders sizes
  int enable_scaling =
    vtkOptiXActorNode::GetEnableScaling(act);
  vtkDataArray *scaleArray = nullptr;
  vtkPiecewiseFunction *scaleFunction = nullptr;
  if (enable_scaling && mapper)
  {
    vtkInformation *mapInfo = mapper->GetInformation();
    char *scaleArrayName =
      (char *)mapInfo->Get(vtkOptiXActorNode::SCALE_ARRAY_NAME());
    scaleArray = poly->GetPointData()->GetArray(scaleArrayName);
    scaleFunction =
      vtkPiecewiseFunction::SafeDownCast(
        mapInfo->Get(vtkOptiXActorNode::SCALE_FUNCTION()));
  }

  //
  //per actor material
  //
  optix::Material matl = ctx->createMaterial();
  matl->setClosestHitProgram( 0,
      ptxLoader->ClosestHitProgram
    );
  matl->setAnyHitProgram( 1,
      ptxLoader->AnyHitProgram
    );
  const optix::float3 Ka = optix::make_float3(
    static_cast<float>(ambientColor[0] * property->GetAmbient()),
    static_cast<float>(ambientColor[1] * property->GetAmbient()),
    static_cast<float>(ambientColor[2] * property->GetAmbient())
    );
  const optix::float3 Kd = optix::make_float3(
    static_cast<float>(diffuseColor[0] * property->GetDiffuse()),
    static_cast<float>(diffuseColor[1] * property->GetDiffuse()),
    static_cast<float>(diffuseColor[2] * property->GetDiffuse())
    );
  const float Ns = static_cast<float>(property->GetSpecularPower());
  //const float specAdjust = 2.0f/(2.0f+Ns);
  const optix::float3 Ks = optix::make_float3(
    static_cast<float>(property->GetSpecularColor()[0]*
      property->GetSpecular()/*specAdjust*/),
    static_cast<float>(property->GetSpecularColor()[1]*
      property->GetSpecular()/*specAdjust*/),
    static_cast<float>(property->GetSpecularColor()[2]*
      property->GetSpecular()/*specAdjust*/)
    );
  matl[ "Kd" ]->setFloat( (property->GetDiffuse() == 0.0) ? Ka : Kd ); // Fix similar to OSPRay
  matl[ "Ks" ]->setFloat( Ks );
  matl[ "Ns" ]->setFloat( Ns );

  // Set the occlusion epsilon
  double* polyBounds = poly->GetBounds();
  double maxExtentX = vtkMath::Max<double>(std::abs(polyBounds[0]),
    std::abs(polyBounds[1]));
  double maxExtentY = vtkMath::Max<double>(std::abs(polyBounds[2]),
    std::abs(polyBounds[3]));
  double maxExtentZ = vtkMath::Max<double>(std::abs(polyBounds[4]),
    std::abs(polyBounds[5]));
  float occlusionEps =
    float(std::sqrt(
      maxExtentX*maxExtentX+maxExtentY*maxExtentY+maxExtentZ*maxExtentZ)
      *(1e-5));
  matl["occlusion_epsilon"]->setFloat(occlusionEps);

  // Regular textures and texture coordinates
  vtkTexture *texture = act->GetTexture();
  int numTextureCoordinates = 0;
  float *textureCoordinates = nullptr;
  vtkImageData* vColorTextureMap = nullptr;
  if (texture)
  {
    vtkDataArray *da = poly->GetPointData()->GetTCoords();
    numTextureCoordinates = da->GetNumberOfTuples();
    textureCoordinates = new float[numTextureCoordinates * 2];
    float *tp = textureCoordinates;
    for (int i = 0; i<numTextureCoordinates; i++)
    {
      *tp = static_cast<float>(da->GetTuple(i)[0]);
      tp++;
      *tp = static_cast<float>(da->GetTuple(i)[1]);
      tp++;
    }
    vColorTextureMap = vtkImageData::SafeDownCast(texture->GetInput());
    numTextureCoordinates = numTextureCoordinates * 2;
  }

  //Colors from point and cell arrays
  unsigned int numPointColors = 0;
  optix::float4* pointColors = NULL;
  unsigned int numCellColors = 0;
  optix::float3* cellColors = NULL;
  unsigned int numPointValueTextureCoords = 0;
  float *pointValueTextureCoords = NULL;

  vtkUnsignedCharArray *vColors = NULL;
  vtkFloatArray *vColorCoordinates = NULL;
  vtkImageData* pColorTextureMap = NULL;
  int cellFlag = -1; //mapper tells us which

  if (mapper)
  {
    mapper->MapScalars(poly, 1.0, cellFlag);
    vColors = mapper->GetColorMapColors();
    vColorCoordinates = mapper->GetColorCoordinates();
    pColorTextureMap = mapper->GetColorTextureMap();
  }
  if (vColors)
  {
    if (cellFlag == 2 && mapper->GetFieldDataTupleId() > -1)
    {
      int numComp = vColors->GetNumberOfComponents();
      unsigned char *colorPtr = vColors->GetPointer(0);
      colorPtr = colorPtr + mapper->GetFieldDataTupleId()*numComp;
      // this setting (and all the other scalar colors)
      // really depends on mapper->ScalarMaterialMode
      // but I'm not sure Ka is working currently so leaving it on Kd
      const optix::float3 fdiffusef = optix::make_float3(
        static_cast<float>(colorPtr[0] * property->GetDiffuse() / 255.0f),
        static_cast<float>(colorPtr[1] * property->GetDiffuse() / 255.0f),
        static_cast<float>(colorPtr[2] * property->GetDiffuse() / 255.0f)
        );
      matl["Kd"]->setFloat(fdiffusef);
    }
    else if (cellFlag == 1)
    {
      // Same procedure as vertex colors, but is indexed differently later on.
      // (cellFlag == 1 means vColors is not a vertex color array,
      // may have different length, etc).
      numCellColors = vColors->GetNumberOfTuples();
      cellColors = new optix::float3[numCellColors];
      for (unsigned int i = 0; i < numCellColors; i++)
      {
        unsigned char *color = vColors->GetPointer(4 * i);
        cellColors[i] = optix::make_float3(color[0] / 255.0f,
          color[1] / 255.0f,
          color[2] / 255.0f);
      }
    }
    else if (cellFlag == 0)
    {
      //Vertex colors
      numPointColors = vColors->GetNumberOfTuples();
      pointColors = new optix::float4[numPointColors];
      for (unsigned int i = 0; i < numPointColors; i++)
      {
        unsigned char *color = vColors->GetPointer(4 * i);
        pointColors[i] = optix::make_float4(color[0] / 255.0f,
          color[1] / 255.0f,
          color[2] / 255.0f,
          1.f );
      }
    }
  }
  else
  {
    //1D lut texture with coordinates
    if (vColorCoordinates && pColorTextureMap)
    {
      //color on point interpolated values (subsequently colormapped via 1D LUT)
      numPointValueTextureCoords = vColorCoordinates->GetNumberOfTuples();
      pointValueTextureCoords = new float[numPointValueTextureCoords];
      float *tc = vColorCoordinates->GetPointer(0);
      for (unsigned int i = 0; i < numPointValueTextureCoords; i++)
      {
        pointValueTextureCoords[i] = *tc;
        tc += 2;
      }
      vColorTextureMap = pColorTextureMap;
    }
  }

  //Representations for vertex data
  if (conn.vertex_index.size())
  {
    RenderAsSpheres(ctx,
      matl,
      vertices,
      (unsigned int)(numPositions),
      conn.vertex_index, conn.vertex_reverse,
      pointSize,
      scaleArray,
      scaleFunction,
      vColorTextureMap,
      numTextureCoordinates, textureCoordinates,
      numPointColors, pointColors,
      numCellColors, cellColors,
      numPointValueTextureCoords, pointValueTextureCoords,
      this->MyGeom,
      ptxLoader
      );
  }

  // Representations for line data
  if (conn.line_index.size())
  {
    if (property->GetRepresentation() == VTK_POINTS)
    {
      RenderAsSpheres(ctx,
        matl,
        vertices,
        (unsigned int)(numPositions),
        conn.line_index, conn.line_reverse,
        pointSize,
        scaleArray,
        scaleFunction,
        vColorTextureMap,
        numTextureCoordinates, textureCoordinates,
        numPointColors, pointColors,
        numCellColors, cellColors,
        numPointValueTextureCoords, pointValueTextureCoords,
        this->MyGeom,
        ptxLoader
        );
    }
    else
    {
      RenderAsCylinders(ctx,
        matl,
        vertices,
        (unsigned int)(numPositions),
        conn.line_index, conn.line_reverse,
        lineWidth,
        scaleArray,
        scaleFunction,
        vColorTextureMap,
        numTextureCoordinates, textureCoordinates,
        numPointColors, pointColors,
        numCellColors, cellColors,
        numPointValueTextureCoords, pointValueTextureCoords,
        this->MyGeom,
        ptxLoader
        );
    }
  }

  //Representations for triangle data
  if (conn.triangle_index.size())
  {
    //format depends on representation style
    switch( property->GetRepresentation())
    {
      case VTK_POINTS:
      {
        RenderAsSpheres( ctx,
          matl,
          vertices,
          (unsigned int)(numPositions),
          conn.triangle_index, conn.triangle_reverse,
          pointSize,
          scaleArray,
          scaleFunction,
          vColorTextureMap,
          numTextureCoordinates, textureCoordinates,
          numPointColors, pointColors,
          numCellColors, cellColors,
          numPointValueTextureCoords, pointValueTextureCoords,
          this->MyGeom,
          ptxLoader
          );
        break;
      }
      case VTK_WIREFRAME:
      {
        RenderAsCylinders(ctx,
          matl,
          vertices,
          (unsigned int)(numPositions),
          conn.triangle_index, conn.triangle_reverse,
          lineWidth,
          scaleArray,
          scaleFunction,
          vColorTextureMap,
          numTextureCoordinates, textureCoordinates,
          numPointColors, pointColors,
          numCellColors, cellColors,
          numPointValueTextureCoords, pointValueTextureCoords,
          this->MyGeom,
          ptxLoader
          );
        break;
      }
      default:
      {
        vtkDataArray* vNormals = 0;
        if( property->GetInterpolation() != VTK_FLAT )
        {
          vNormals = poly->GetPointData()->GetNormals();
        }

        RenderAsTriangles(
          ctx,
          matl,
          vertices,
          (unsigned int)(numPositions),
          vNormals,
          vColorTextureMap,
          numTextureCoordinates, textureCoordinates,
          numPointColors, pointColors,
          numCellColors, cellColors,
          numPointValueTextureCoords, pointValueTextureCoords,
          conn.triangle_index,
          conn.triangle_reverse,
          this->MyGeom,
          ptxLoader
          );
      }
    }
  }

  //Representations for strip data
  if (conn.strip_index.size())
  {
    //format depends on representation style
    switch (property->GetRepresentation())
    {
    case VTK_POINTS:
    {
      RenderAsSpheres(ctx,
        matl,
        vertices,
        (unsigned int)(numPositions),
        conn.strip_index, conn.strip_reverse,
        pointSize,
        scaleArray,
        scaleFunction,
        vColorTextureMap,
        numTextureCoordinates, textureCoordinates,
        numPointColors, pointColors,
        numCellColors, cellColors,
        numPointValueTextureCoords, pointValueTextureCoords,
        this->MyGeom,
        ptxLoader
      );
      break;
    }
    case VTK_WIREFRAME:
    {
      RenderAsCylinders(ctx,
        matl,
        vertices,
        (unsigned int)(numPositions),
        conn.strip_index, conn.strip_reverse,
        lineWidth,
        scaleArray,
        scaleFunction,
        vColorTextureMap,
        numTextureCoordinates, textureCoordinates,
        numPointColors, pointColors,
        numCellColors, cellColors,
        numPointValueTextureCoords, pointValueTextureCoords,
        this->MyGeom,
        ptxLoader
      );
      break;
    }
    default:
    {
      vtkDataArray* vNormals = 0;
      if (property->GetInterpolation() != VTK_FLAT)
      {
        vNormals = poly->GetPointData()->GetNormals();
      }

      RenderAsTriangles(
        ctx,
        matl,
        vertices,
        (unsigned int)(numPositions),
        vNormals,
        vColorTextureMap,
        numTextureCoordinates, textureCoordinates,
        numPointColors, pointColors,
        numCellColors, cellColors,
        numPointValueTextureCoords, pointValueTextureCoords,
        conn.strip_index, conn.strip_reverse,
        this->MyGeom,
        ptxLoader
      );
    }
    }
  }

  delete[] pointColors;
  delete[] cellColors;
  delete[] pointValueTextureCoords;
}

//------------------------------------------------------------------------------
void vtkOptiXPolyDataMapperNode::CreateNewMeshes()
{
  delete this->MyGeom;
  this->MyGeom = new vtkOptiXPolyDataMapperNode::Geom;
}

//------------------------------------------------------------------------------
void vtkOptiXPolyDataMapperNode::AddGeomToGroup(vtkOptiXRendererNode* orn)
{
  this->MyGeom->AddMyselfTo(orn->GetOptiXGeometryGroup());
}

//------------------------------------------------------------------------------
void vtkOptiXPolyDataMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    // we use a lot of params from our parent
    vtkOptiXActorNode *aNode = vtkOptiXActorNode::SafeDownCast(this->Parent);
    vtkActor *act = vtkActor::SafeDownCast(aNode->GetRenderable());

    if (act->GetVisibility() == false)
    {
      return;
    }

    vtkOptiXRendererNode *orn =
      static_cast<vtkOptiXRendererNode *>(
        this->GetFirstAncestorOfType("vtkOptiXRendererNode"));

    //if there are no changes, just reuse last result
    bool enable_cache = true; //turn off to force rebuilds for debugging
    vtkMTimeType inTime = aNode->GetMTime();
    if (enable_cache && this->RenderTime >= inTime)
    {
      this->AddGeomToGroup(orn);
      return;
    }
    this->RenderTime = inTime;

    //something changed so make new meshes
    this->CreateNewMeshes();

    vtkPolyData* poly = NULL;
    vtkMapper* mapper = act->GetMapper();
    if (mapper)
    {
      poly = (vtkPolyData*)(mapper->GetInput());
    }
    if (poly)
    {
      vtkProperty * property = act->GetProperty();
      this->RenderPoly(
        orn,
        aNode,
        poly,
        property->GetAmbientColor(),
        property->GetDiffuseColor() );
    }

    this->AddGeomToGroup(orn);
  }
}
