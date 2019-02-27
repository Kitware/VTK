/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGLTFExporter.h"

#include <memory>
#include <sstream>

#include "vtk_jsoncpp.h"

#include "vtkAssemblyPath.h"
#include "vtkBase64OutputStream.h"
#include "vtkCamera.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/SystemTools.hxx"

vtkStandardNewMacro(vtkGLTFExporter);

vtkGLTFExporter::vtkGLTFExporter()
{
  this->FileName = nullptr;
  this->InlineData = false;
}

vtkGLTFExporter::~vtkGLTFExporter()
{
  delete [] this->FileName;
}

namespace {

vtkPolyData *findPolyData(vtkDataObject* input)
{
  // do we have polydata?
  vtkPolyData *pd = vtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  vtkCompositeDataSet *cd = vtkCompositeDataSet::SafeDownCast(input);
  if (cd)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        return pd;
      }
    }
  }
  return nullptr;
}

void WriteValues(vtkDataArray *ca, ofstream &myFile)
{
  myFile.write(reinterpret_cast<char *>(
    ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples()*
      ca->GetNumberOfComponents()*
        ca->GetElementComponentSize());
}

void WriteValues(vtkDataArray *ca, vtkBase64OutputStream *ostr)
{
  ostr->Write(reinterpret_cast<char *>(
    ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples()*
      ca->GetNumberOfComponents()*
        ca->GetElementComponentSize());
}

void WriteBufferAndView(vtkDataArray *inda, const char *fileName, bool inlineData,
  Json::Value &buffers,
  Json::Value &bufferViews
)
{
  vtkDataArray *da = inda;

  // gltf does not support doubles so handle that
  if (inda->GetDataType() == VTK_DOUBLE)
  {
    da = vtkFloatArray::New();
    da->DeepCopy(inda);
  }

  // if inline then base64 encode the data
  std::string result;
  if (inlineData)
  {
    result = "data:application/octet-stream;base64,";
    std::ostringstream toString;
    vtkNew<vtkBase64OutputStream> ostr;
    ostr->SetStream(&toString);
    ostr->StartWriting();
    WriteValues(da, ostr);
    ostr->EndWriting();
    result += toString.str();
  }
  else
  {
    // otherwise write binary files
    std::ostringstream toString;
    toString
      << "buffer"
      << da->GetMTime()
      << ".bin";
    result = toString.str();

    std::string fullPath =
      vtksys::SystemTools::GetFilenamePath(fileName);
    fullPath += "/";
    fullPath += result;

    // now write the data
    ofstream myFile(fullPath, ios::out | ios::binary);

    WriteValues(da, myFile);
    myFile.close();
  }

  Json::Value buffer;
  Json::Value view;

  unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
  unsigned int byteLength = da->GetElementComponentSize() * count;
  buffer["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  buffer["uri"] = result;
  buffers.append(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  bufferViews.append(view);

  // delete double to float conversion array
  if (da != inda)
  {
    da->Delete();
  }
}

void WriteBufferAndView(vtkCellArray *ca, const char *fileName, bool inlineData,
  Json::Value &buffers,
  Json::Value &bufferViews
)
{
  vtkUnsignedIntArray *ia = vtkUnsignedIntArray::New();
  vtkIdType npts;
  vtkIdType *indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts,indx); )
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      ia->InsertNextValue(value);
    }
  }

  WriteBufferAndView(ia, fileName, inlineData, buffers, bufferViews);
  ia->Delete();
}


// gltf uses hard coded numbers to represent data types
// they match the definitions from gl.h but for your convenience
// some of the common values we use are listed below to make
// the code more readable without including gl.h

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601

void WriteMesh(
  Json::Value &accessors,
  Json::Value &buffers,
  Json::Value &bufferViews,
  Json::Value &meshes,
  Json::Value &nodes,
  vtkPolyData *pd,
  vtkActor *aPart,
  const char *fileName,
  bool inlineData
)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData *tris = trif->GetOutput();

  // write the point locations
  int pointAccessor = 0;
  {
    vtkDataArray *da = tris->GetPoints()->GetData();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC3";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    double range[6];
    tris->GetPoints()->GetBounds(range);
    Json::Value mins;
    mins.append(range[0]);
    mins.append(range[2]);
    mins.append(range[4]);
    Json::Value maxs;
    maxs.append(range[1]);
    maxs.append(range[3]);
    maxs.append(range[5]);
    acc["min"] = mins;
    acc["max"] = maxs;
    pointAccessor = accessors.size();
    accessors.append(acc);
  }

  // if we have vertex colors then write them out
  int vertColorAccessor = -1;
  aPart->GetMapper()->MapScalars(tris, 1.0);
  if (aPart->GetMapper()->GetColorMapColors())
  {
    vtkUnsignedCharArray *da = aPart->GetMapper()->GetColorMapColors();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC4";
    acc["componentType"] = GL_UNSIGNED_BYTE;
    acc["normalized"] = true;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    vertColorAccessor = accessors.size();
    accessors.append(acc);
  }

  // if we have tcoords then write them out
  // first check for colortcoords
  int tcoordAccessor = -1;
  vtkFloatArray *tcoords =
    aPart->GetMapper()->GetColorCoordinates();
  if (!tcoords)
  {
    tcoords = vtkFloatArray::SafeDownCast(
      tris->GetPointData()->GetTCoords());
  }
  if (tcoords)
  {
    vtkFloatArray *da = tcoords;
    WriteBufferAndView(tcoords, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "VEC2";
    acc["componentType"] = GL_FLOAT;
    acc["normalized"] = false;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    tcoordAccessor = accessors.size();
    accessors.append(acc);
  }

  // to store the primitives
  Json::Value prims;

  // write out the verts
  if (tris->GetVerts() && tris->GetVerts()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 0;
    Json::Value attribs;

    vtkCellArray *da = tris->GetVerts();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() -1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells());
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the lines
  if (tris->GetLines() && tris->GetLines()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 1;
    Json::Value attribs;

    vtkCellArray *da = tris->GetLines();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() -1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells()*2);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the triangles
  if (tris->GetPolys() && tris->GetPolys()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 4;
    Json::Value attribs;

    vtkCellArray *da = tris->GetPolys();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() -1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells()*3);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  Json::Value amesh;
  amesh["primitives"] = prims;
  meshes.append(amesh);

  // write out an actor
  Json::Value child;
  if (!aPart->GetIsIdentity())
  {
    vtkMatrix4x4 *amat = aPart->GetMatrix();
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        child["matrix"].append(amat->GetElement(j,i));
      }
    }
  }
  child["mesh"] = meshes.size() - 1;
  nodes.append(child);
}

void WriteCamera(Json::Value &cameras, vtkRenderer *ren)
{
  vtkCamera *cam = ren->GetActiveCamera();
  Json::Value acamera;
  Json::Value camValues;
  camValues["znear"] = cam->GetClippingRange()[0];
  camValues["zfar"] = cam->GetClippingRange()[1];
  if (cam->GetParallelProjection())
  {
    acamera["type"] =  "orthographic";
    camValues["xmag"] = cam->GetParallelScale()*ren->GetTiledAspectRatio();
    camValues["ymag"] = cam->GetParallelScale();
    acamera["orthographic"] = camValues;
  }
  else
  {
    acamera["type"] = "perspective";
    camValues["yfov"] = cam->GetViewAngle();
    camValues["aspectRatio"] = ren->GetTiledAspectRatio();
    acamera["perspective"] = camValues;
  }
  cameras.append(acamera);
}

void WriteTexture(
  Json::Value &buffers,
  Json::Value &bufferViews,
  Json::Value &textures,
  Json::Value &samplers,
  Json::Value &images,
  vtkPolyData *pd,
  vtkActor *aPart,
  const char *fileName,
  bool inlineData,
  std::map<vtkUnsignedCharArray *, unsigned int> &textureMap
)
{
  // do we have a texture
  aPart->GetMapper()->MapScalars(pd, 1.0);
  vtkImageData *id = aPart->GetMapper()->GetColorTextureMap();
  vtkTexture *t = nullptr;
  if (!id && aPart->GetTexture())
  {
    t = aPart->GetTexture();
    id = t->GetInput();
  }

  vtkUnsignedCharArray *da = nullptr;
  if (id && id->GetPointData()->GetScalars())
  {
    da = vtkUnsignedCharArray::SafeDownCast(id->GetPointData()->GetScalars());
  }
  if (!da)
  {
    return;
  }

  unsigned int textureSource = 0;

  if (textureMap.find(da) == textureMap.end())
  {
    textureMap[da] = textures.size();

    // flip Y
    vtkNew<vtkTrivialProducer> triv;
    triv->SetOutput(id);
    vtkNew<vtkImageFlip> flip;
    flip->SetFilteredAxis(1);
    flip->SetInputConnection(triv->GetOutputPort());

    // convert to png
    vtkNew<vtkPNGWriter> png;
    png->SetCompressionLevel(0);
    png->SetInputConnection(flip->GetOutputPort());
    png->WriteToMemoryOn();
    png->Write();
    da = png->GetResult();

    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the image
    Json::Value img;
    img["bufferView"] = bufferViews.size() - 1;
    img["mimeType"] = "image/png";
    images.append(img);

    textureSource = images.size() - 1;
  }
  else
  {
    textureSource = textureMap[da];
  }

  // write the sampler
  Json::Value smp;
  smp["magFilter"] = GL_NEAREST;
  smp["minFilter"] = GL_NEAREST;
  smp["wrapS"] = GL_CLAMP_TO_EDGE;
  smp["wrapT"] = GL_CLAMP_TO_EDGE;
  if (t)
  {
    smp["wrapS"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["wrapT"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["magFilter"] = t->GetInterpolate() ? GL_LINEAR :  GL_NEAREST;
    smp["minFilter"] = t->GetInterpolate() ? GL_LINEAR :  GL_NEAREST;
  }
  samplers.append(smp);

  Json::Value texture;
  texture["source"] = textureSource;
  texture["sampler"] = samplers.size() - 1;
  textures.append(texture);
}

void WriteMaterial(
  Json::Value &materials,
  int textureIndex,
  bool haveTexture,
  vtkActor *aPart
)
{
  Json::Value mat;
  Json::Value model;

  if (haveTexture)
  {
    Json::Value tex;
    tex["texCoord"] = 0; // TEXCOORD_0
    tex["index"] = textureIndex;
    model["baseColorTexture"] = tex;
  }

  vtkProperty *prop = aPart->GetProperty();
  double dcolor[3];
  prop->GetDiffuseColor(dcolor);
  model["baseColorFactor"].append(dcolor[0]);
  model["baseColorFactor"].append(dcolor[1]);
  model["baseColorFactor"].append(dcolor[2]);
  model["baseColorFactor"].append(prop->GetOpacity());
  model["metallicFactor"] = prop->GetSpecular();
  model["roughnessFactor"] = 1.0/(1.0 + prop->GetSpecular()*0.2*prop->GetSpecularPower());
  mat["pbrMetallicRoughness"] = model;
  materials.append(mat);
}

}

std::string vtkGLTFExporter::WriteToString()
{
  std::ostringstream result;

  this->WriteToStream(result);

  return result.str();
}

void vtkGLTFExporter::WriteData()
{
  ofstream output;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // try opening the files
  output.open(this->FileName);
  if (!output.is_open())
  {
    vtkErrorMacro("Unable to open file for gltf output.");
    return;
  }

  this->WriteToStream(output);
  output.close();
}

void vtkGLTFExporter::WriteToStream(ostream &output)
{
  Json::Value cameras;
  Json::Value bufferViews;
  Json::Value buffers;
  Json::Value accessors;
  Json::Value nodes;
  Json::Value meshes;
  Json::Value textures;
  Json::Value images;
  Json::Value samplers;
  Json::Value materials;

  std::vector<unsigned int> topNodes;

  // supprt sharing texture maps
  std::map<vtkUnsignedCharArray *, unsigned int> textureMap;

  for (auto ren : vtk::Range(this->RenderWindow->GetRenderers()))
  {
    if (this->ActiveRenderer && ren != this->ActiveRenderer)
    {
      // If ActiveRenderer is specified then ignore all other renderers
      continue;
    }
    if (!ren->GetDraw())
    {
      continue;
    }

    // setup the camera data in case we need to use it later
    Json::Value anode;
    anode["camera"] = cameras.size(); // camera node
    vtkMatrix4x4 *mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        anode["matrix"].append(mat->GetElement(j,i));
      }
    }
    anode["name"] = "Camera Node";

    vtkPropCollection *pc;
    vtkProp *aProp;
    pc = ren->GetViewProps();
    vtkCollectionSimpleIterator pit;
    bool foundVisibleProp = false;
    for (pc->InitTraversal(pit); (aProp = pc->GetNextProp(pit)); )
    {
      if (!aProp->GetVisibility())
      {
        continue;
      }
      vtkNew<vtkActorCollection> ac;
      aProp->GetActors(ac);
      vtkActor *anActor;
      vtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
      {
        vtkAssemblyPath *apath;
        vtkActor *aPart;
        for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
        {
          aPart = static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
          if (aPart->GetVisibility() && aPart->GetMapper() && aPart->GetMapper()->GetInputAlgorithm())
          {
            aPart->GetMapper()->GetInputAlgorithm()->Update();
            vtkPolyData *pd = findPolyData(aPart->GetMapper()->GetInputDataObject(0,0));
            if (pd && pd->GetNumberOfCells() > 0)
            {
              foundVisibleProp = true;
              WriteMesh(accessors, buffers, bufferViews,
                meshes, nodes,
                pd, aPart, this->FileName, this->InlineData);
              anode["children"].append(nodes.size() - 1);
              unsigned int oldTextureCount = textures.size();
              WriteTexture(buffers, bufferViews,
                textures, samplers, images,
                pd, aPart, this->FileName, this->InlineData,
                textureMap);
              meshes[meshes.size()-1]["primitives"][0]["material"] = materials.size();
              WriteMaterial(materials,
                oldTextureCount, oldTextureCount != textures.size(), aPart);
            }
          }
        }
      }
    }

    // only write the camera if we had visible nodes
    if (foundVisibleProp)
    {
      WriteCamera(cameras, ren);
      topNodes.push_back(nodes.size());
      nodes.append(anode);
    }
  }

  Json::Value root;
  Json::Value asset;
  asset["generator"] = "VTK";
  asset["version"] = "2.0";
  root["asset"] = asset;

  root["scene"] = 0;
  root["cameras"] = cameras;
  root["nodes"] = nodes;
  root["meshes"] = meshes;
  root["buffers"] = buffers;
  root["bufferViews"] = bufferViews;
  root["accessors"] = accessors;
  root["images"] = images;
  root["textures"] = textures;
  root["samplers"] = samplers;
  root["materials"] = materials;

  Json::Value ascene;
  ascene["name"] = "Layer 0";
  Json::Value noderefs;
  for (auto i : topNodes)
  {
    noderefs.append(i);
  }
  ascene["nodes"] = noderefs;
  Json::Value scenes;
  scenes.append(ascene);
  root["scenes"] = scenes;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";
  std::unique_ptr<Json::StreamWriter> writer(
      builder.newStreamWriter());
  writer->write(root, &output);
}

void vtkGLTFExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "InlineData: " << this->InlineData << "\n";
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
