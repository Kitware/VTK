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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRange.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTriangleFilter.h"

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

void WriteValues(vtkCellArray *ca, vtkBase64OutputStream *ostr)
{
  vtkIdType npts;
  vtkIdType *indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts,indx); )
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      ostr->Write(reinterpret_cast<char *>(&value), 4);
    }
  }
}

void WriteValues(vtkCellArray *ca, ofstream &myFile)
{
  vtkIdType npts;
  vtkIdType *indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts,indx); )
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      myFile.write(reinterpret_cast<char *>(&value), 4);
    }
  }
}

void WriteValues(vtkPoints *ca, vtkBase64OutputStream *ostr)
{
  double pt[3];
  float fpt[3];
  for (int i = 0; i < ca->GetNumberOfPoints(); ++i)
  {
    ca->GetPoint(i, pt);
    fpt[0] = pt[0];
    fpt[1] = pt[1];
    fpt[2] = pt[2];
    ostr->Write(reinterpret_cast<char *>(fpt), 12);
  }
}

void WriteValues(vtkPoints *ca, ofstream &myFile)
{
  double pt[3];
  float fpt[3];
  for (int i = 0; i < ca->GetNumberOfPoints(); ++i)
  {
    ca->GetPoint(i, pt);
    fpt[0] = pt[0];
    fpt[1] = pt[1];
    fpt[2] = pt[2];
    myFile.write(reinterpret_cast<char *>(fpt),12);
  }
}

void WriteValues(vtkUnsignedCharArray *ca, ofstream &myFile)
{
  myFile.write(reinterpret_cast<char *>(
    ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples()*4);
}

void WriteValues(vtkUnsignedCharArray *ca, vtkBase64OutputStream *ostr)
{
  ostr->Write(reinterpret_cast<char *>(
    ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples()*4);
}

template <typename T>
std::string WriteBuffer(T *ca, const char *fileName, bool inlineData)
{
  // if inline then base64 encode the data
  if (inlineData)
  {
    std::string result = "data:application/octet-stream;base64,";
    std::ostringstream toString;
    vtkNew<vtkBase64OutputStream> ostr;
    ostr->SetStream(&toString);
    ostr->StartWriting();
    WriteValues(ca, ostr);
    ostr->EndWriting();
    result += toString.str();
    return result;
  }

  // otherwise write binary files
  std::string result;
  std::ostringstream toString;
  toString
    << "buffer"
    << ca->GetMTime()
    << ".bin";
  result = toString.str();

  std::string fullPath =
    vtksys::SystemTools::GetFilenamePath(fileName);
  fullPath += "/";
  fullPath += result;

  // now write the data
  ofstream myFile(fullPath, ios::out | ios::binary);

  WriteValues(ca, myFile);
  myFile.close();

  return result;
}

// gltf uses hard coded numbers to represent data types
// they match the definitions from gl.h but for your convenience
// some of the common values we use are listed below.
//
// 5121 - unsigned char
// 5125 = unsigned int
// 5126 = float
//
void WriteMesh(
  unsigned int &totalAccessors,
  Json::Value &accessors,
  unsigned int &totalBuffers,
  Json::Value &buffers,
  unsigned int &totalBufferViews,
  Json::Value &bufferViews,
  unsigned int &totalMeshes,
  Json::Value &meshes,
  unsigned int &totalNodes,
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

  // write out the mesh
  Json::Value aprim;
  aprim["mode"] = 4;
  Json::Value attribs;

  // write the triangles
  {
    vtkCellArray *da = tris->GetPolys();
    std::string fname = WriteBuffer(da, fileName, inlineData);
    Json::Value buffer;
    // 12 bytes per tri, one tri per 4 entries
    buffer["byteLength"] = static_cast<Json::Value::Int64>(12*da->GetNumberOfCells());
    buffer["uri"] = fname;
    buffers.append(buffer);
    totalBuffers++;

    // write the buffer views
    Json::Value view;
    view["buffer"] = totalBuffers - 1;
    view["byteOffset"] = 0;
    view["byteLength"] = static_cast<Json::Value::Int64>(12*da->GetNumberOfCells());
    bufferViews.append(view);
    totalBufferViews++;

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = totalBufferViews -1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = 5125;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells()*3);
    accessors.append(acc);
    aprim["indices"] = totalAccessors;
    totalAccessors++;
  }

  // write the point locations
  {
    vtkPoints *da = tris->GetPoints();
    std::string fname = WriteBuffer(da, fileName, inlineData);
    Json::Value buffer;
    // 3 floats per point
    buffer["byteLength"] = static_cast<Json::Value::Int64>(12*da->GetNumberOfPoints());
    buffer["uri"] = fname;
    buffers.append(buffer);
    totalBuffers++;

    // write the buffer views
    Json::Value view;
    view["buffer"] = totalBuffers - 1;
    view["byteOffset"] = 0;
    view["byteLength"] = static_cast<Json::Value::Int64>(12*da->GetNumberOfPoints());
    view["byteStride"] = 12;
    bufferViews.append(view);
    totalBufferViews++;

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = totalBufferViews -1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC3";
    acc["componentType"] = 5126;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfPoints());
    double range[6];
    da->GetBounds(range);
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
    accessors.append(acc);
    attribs["POSITION"] = totalAccessors;
    totalAccessors++;
  }

  // if we have vertex colors then write them out
  aPart->GetMapper()->MapScalars(pd, 1.0);
  if (aPart->GetMapper()->GetColorMapColors())
  {
    vtkUnsignedCharArray *da = aPart->GetMapper()->GetColorMapColors();
    std::string fname = WriteBuffer(da, fileName, inlineData);
    Json::Value buffer;
    // 4 uchar per point
    buffer["byteLength"] = static_cast<Json::Value::Int64>(4*da->GetNumberOfTuples());
    buffer["uri"] = fname;
    buffers.append(buffer);
    totalBuffers++;

    // write the buffer views
    Json::Value view;
    view["buffer"] = totalBuffers - 1;
    view["byteOffset"] = 0;
    view["byteLength"] = static_cast<Json::Value::Int64>(4*da->GetNumberOfTuples());
    view["byteStride"] = 4;
    bufferViews.append(view);
    totalBufferViews++;

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = totalBufferViews -1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC4";
    acc["componentType"] = 5121;
    acc["normalized"] = true;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    attribs["COLOR_0"] = totalAccessors;
    accessors.append(acc);
    totalAccessors++;
  }

  aprim["attributes"] = attribs;

  Json::Value amesh;
  Json::Value prims;
  prims.append(aprim);
  amesh["primitives"] = prims;
  meshes.append(amesh);
  totalMeshes++;

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
  child["mesh"] = totalMeshes - 1;
  nodes.append(child);
  totalNodes++;
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

  std::vector<unsigned int> topNodes;
  unsigned int count = 0;
  unsigned int totalNodes = 0;
  unsigned int totalMeshes = 0;
  unsigned int totalBuffers = 0;
  unsigned int totalBufferViews = 0;
  unsigned int totalAccessors = 0;

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

    WriteCamera(cameras, ren);

    Json::Value anode;
    anode["camera"] = count; // camera node
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
            if (pd && pd->GetPolys() && pd->GetNumberOfCells() > 0)
            {
              WriteMesh(
                totalAccessors, accessors,
                totalBuffers, buffers,
                totalBufferViews, bufferViews,
                totalMeshes, meshes,
                totalNodes, nodes,
                pd, aPart, this->FileName, this->InlineData);
              anode["children"].append(totalNodes - 1);
            }
          }
        }
      }
    }
    nodes.append(anode);
    topNodes.push_back(totalNodes);
    totalNodes++;
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

  // Json::Value materials;
  // root["materials"] = materials;

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
