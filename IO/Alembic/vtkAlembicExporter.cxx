// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlembicExporter.h"

#include <cstdio>
#include <memory>
#include <sstream>

#include "vtkAssemblyPath.h"
#include "vtkBase64OutputStream.h"
#include "vtkCamera.h"
#include "vtkCellArrayIterator.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkExtractVOI.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

using namespace Alembic::AbcGeom; // Contains Abc, AbcCoreAbstract

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAlembicExporter);

vtkAlembicExporter::vtkAlembicExporter()
{
  this->FileName = nullptr;
}

vtkAlembicExporter::~vtkAlembicExporter()
{
  delete[] this->FileName;
}

namespace
{

vtkPolyData* findPolyData(vtkDataObject* input)
{
  // do we have polydata?
  vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
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

void WriteMesh(
  OArchive& archive, vtkPolyData* pd, vtkActor* aPart, const char* /*fileName*/, size_t index)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData* tris = trif->GetOutput();

  // If the actor has a transform, the mesh should have an OXForm parent node
  // Always use a transform, even if matrix is identity.
  vtkMatrix4x4* amat = aPart->GetMatrix();
  // We need to convert row-major to column-major, with transpose.
  vtkNew<vtkMatrix4x4> transpose;
  vtkMatrix4x4::Transpose(amat, transpose);
  double matData[4][4];
  vtkMatrix4x4::DeepCopy((double*)matData, transpose);
  Abc::M44d camMatrix(matData);

  // set the transform in an alembic node.
  std::ostringstream strm;
  strm << "xform_" << index;
  std::string xformName = strm.str();
  OXform xform(OObject(archive, kTop), xformName);
  XformSample xformSamp;
  XformOp transop(kMatrixOperation, kMatrixHint);
  xformSamp.addOp(transop, camMatrix);
  xform.getSchema().set(xformSamp);

  std::ostringstream().swap(strm);
  strm << "mesh_" << index;
  std::string name = strm.str();
  // Create a PolyMesh output class.
  OPolyMesh meshObj(xform, name);
  OPolyMeshSchema& mesh = meshObj.getSchema();

  // write the point locations
  vtkDataArray* pointData = nullptr;
  {
    pointData = tris->GetPoints()->GetData();
    // Alembic polymesh does not support doubles so handle that
    if (pointData->GetDataType() == VTK_DOUBLE)
    {
      pointData = vtkFloatArray::New();
      pointData->DeepCopy(tris->GetPoints()->GetData());
    }
  }

  // if we have vertex colors then retrieve them
  aPart->GetMapper()->SetInterpolateScalarsBeforeMapping(false);
  aPart->GetMapper()->MapScalars(tris, 1.0);
  vtkSmartPointer<vtkUnsignedCharArray> vertColor(aPart->GetMapper()->GetColorMapColors());

  // Generate tcoord by changing mapper settings:
  aPart->GetMapper()->SetInterpolateScalarsBeforeMapping(true);
  aPart->GetMapper()->MapScalars(tris, 1.0);
  // if we have tcoords then write them out
  // first check for colortcoords
  vtkFloatArray* tcoords = aPart->GetMapper()->GetColorCoordinates();
  if (!tcoords)
  {
    tcoords = vtkFloatArray::SafeDownCast(tris->GetPointData()->GetTCoords());
  }
  if (tcoords)
  {
    if (tcoords->GetNumberOfComponents() != 2)
    {
      vtkWarningWithObjectMacro(nullptr, "Ignoring texture coords without 2 components.");
      tcoords = nullptr;
    }
  }

  // gather a list of cell arrays to export.
  std::vector<vtkCellArray*> cellsToExport;
  // write out the verts
  if (tris->GetVerts() && tris->GetVerts()->GetNumberOfCells())
  {
    cellsToExport.push_back(tris->GetVerts());
  }

  // write out the lines
  if (tris->GetLines() && tris->GetLines()->GetNumberOfCells())
  {
    cellsToExport.push_back(tris->GetLines());
  }

  // write out the triangles
  if (tris->GetPolys() && tris->GetPolys()->GetNumberOfCells())
  {
    cellsToExport.push_back(tris->GetPolys());
  }

  // Int32ArraySample contains `int32_t`, so we have to cast vtkIdType.
  std::vector<vtkTypeInt32> ia;
  std::vector<vtkTypeInt32> counts;
  for (auto* cellArray : cellsToExport)
  {
    auto cellIter = vtk::TakeSmartPointer(cellArray->NewIterator());
    vtkNew<vtkIdList> cell;
    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      cellIter->GetCurrentCell(cell);
      for (vtkIdType i = 0; i < cell->GetNumberOfIds(); i++)
      {
        ia.push_back(static_cast<vtkTypeInt32>(cell->GetId(i)));
      }
      counts.push_back(static_cast<vtkTypeInt32>(cell->GetNumberOfIds()));
    }
  }
  // set texture coords, if present.
  OV2fGeomParam::Sample uvsamp;
  if (tcoords)
  {
    uvsamp.setVals(
      V2fArraySample((const V2f*)tcoords->GetVoidPointer(0), tcoords->GetNumberOfTuples()));
    // this means per-vertex, vtkFaceVaryingScope means per-vertex-per-face.
    uvsamp.setScope(kVertexScope);
  }

  if (!ia.empty())
  {
    OPolyMeshSchema::Sample meshSamp(
      V3fArraySample((const V3f*)pointData->GetVoidPointer(0), pointData->GetNumberOfTuples()),
      Int32ArraySample(ia.data(), ia.size()), Int32ArraySample(counts.data(), counts.size()),
      uvsamp, ON3fGeomParam::Sample());
    mesh.set(meshSamp);
  }

  if (vertColor)
  {
    OCompoundProperty arbParams = mesh.getArbGeomParams();
    // Convert to floats with values between 0 and 1.
    std::vector<float> rgbaAsFloat;
    rgbaAsFloat.resize(vertColor->GetNumberOfTuples() * vertColor->GetNumberOfComponents());
    size_t counter = 0;
    for (vtkIdType i = 0; i < vertColor->GetNumberOfTuples(); i++)
    {
      for (vtkIdType j = 0; j < vertColor->GetNumberOfComponents(); j++)
      {
        rgbaAsFloat[counter] = ((float)vertColor->GetTypedComponent(i, j) / 255.);
        counter++;
      }
    }

    C4fArraySample valSamp((const C4f*)rgbaAsFloat.data(), vertColor->GetNumberOfTuples());

    // "rgba" is a magic name for some Alembic imports, 3DSMax
    OC4fGeomParam color(arbParams, "rgba", false, kVertexScope, 1);
    OC4fGeomParam::Sample colorSamp(valSamp, kVertexScope);

    color.set(colorSamp);
  }
}

void WriteCamera(OArchive& archive, vtkRenderer* ren)
{
  // setup the camera transform
  // Get the camera's transform in world coords:
  vtkTransform* trans = ren->GetActiveCamera()->GetModelViewTransformObject();
  vtkNew<vtkMatrix4x4> inv;
  trans->GetInverse(inv);
  // Convert right-handed to left-handed, by swapping Y.
  // inv->Scale(1, -1, 1);
  // Instead we need to convert row-major to column-major, with transpose.
  vtkNew<vtkMatrix4x4> transpose;
  vtkMatrix4x4::Transpose(inv, transpose);
  double matData[4][4];
  vtkMatrix4x4::DeepCopy((double*)matData, transpose);
  Abc::M44d camMatrix(matData);

  // set the transform in an alembic node.
  OXform xform(OObject(archive, kTop), "camXform");
  XformSample xformSamp;
  XformOp transop(kMatrixOperation, kMatrixHint);
  xformSamp.addOp(transop, camMatrix);
  xform.getSchema().set(xformSamp);

  vtkCamera* cam = ren->GetActiveCamera();
  CameraSample samp;

  OCamera camObj(xform, "cam");
  OCameraSchema camSchema = camObj.getSchema();
  samp.setNearClippingPlane(cam->GetClippingRange()[0]);
  samp.setFarClippingPlane(cam->GetClippingRange()[1]);
  camSchema.set(samp);

  // TODO translate the FOV into something for Alembic
  // Parallel projection may not be available - it's non-physical.
  // if (cam->GetParallelProjection())
}

size_t WriteTexture(vtkActor* aPart, const char* fileName, size_t index,
  std::map<vtkUnsignedCharArray*, size_t>& textureMap)
{
  // do we have a texture?
  // aPart->GetMapper()->MapScalars(pd, 1.0); already done in WriteMesh
  vtkImageData* id = aPart->GetMapper()->GetColorTextureMap();
  vtkTexture* t = nullptr;
  if (!id && aPart->GetTexture())
  {
    t = aPart->GetTexture();
    id = t->GetInput();
  }

  size_t textureSource = (size_t)-1;

  vtkUnsignedCharArray* da = nullptr;
  if (id && id->GetPointData()->GetScalars())
  {
    da = vtkUnsignedCharArray::SafeDownCast(id->GetPointData()->GetScalars());
  }
  if (!da)
  {
    return textureSource;
  }

  if (textureMap.find(da) == textureMap.end())
  {
    auto texIndex = index;
    textureMap[da] = texIndex;

    // figure out a filename - strip extension, add "_tex0.png"
    std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);
    std::string baseName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);
    std::ostringstream strm;
    strm << filePath << '/' << baseName << "_tex" << index << ".png";
    std::string fname = strm.str();

    // we don't want the NaN color in the texture file
    vtkNew<vtkTrivialProducer> triv;
    triv->SetOutput(id);

    vtkNew<vtkExtractVOI> extractVOI;
    extractVOI->SetInputConnection(triv->GetOutputPort());
    int extent[6];
    id->GetExtent(extent);
    extent[3] = 0;
    extractVOI->SetVOI(extent);

    // Alembic has no standard way to store image data, so write a separate PNG
    vtkNew<vtkPNGWriter> png;
    png->SetFileName(fname.c_str());
    png->SetCompressionLevel(5);
    png->SetInputConnection(extractVOI->GetOutputPort());
    png->Write();

    textureSource = texIndex;
  }
  else
  {
    textureSource = textureMap[da];
  }
  return textureSource;
}
}

void vtkAlembicExporter::WriteData()
{
  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  std::vector<size_t> topNodes;

  // support sharing texture maps
  std::map<vtkUnsignedCharArray*, size_t> textureMap;

  OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), this->FileName);

  // Alembic objects close themselves automatically when they go out
  // of scope. File is written then.
  size_t meshCount = 0;
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

    vtkPropCollection* pc;
    vtkProp* aProp;
    pc = ren->GetViewProps();
    vtkCollectionSimpleIterator pit;
    bool foundVisibleProp = false;
    for (pc->InitTraversal(pit); (aProp = pc->GetNextProp(pit));)
    {
      if (!aProp->GetVisibility())
      {
        continue;
      }
      vtkNew<vtkActorCollection> ac;
      aProp->GetActors(ac);
      vtkActor* anActor;
      vtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
      {
        vtkAssemblyPath* apath;
        vtkActor* aPart;
        for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
        {
          aPart = static_cast<vtkActor*>(apath->GetLastNode()->GetViewProp());
          if (aPart->GetVisibility() && aPart->GetMapper() &&
            aPart->GetMapper()->GetInputAlgorithm())
          {
            aPart->GetMapper()->GetInputAlgorithm()->Update();
            vtkPolyData* pd = findPolyData(aPart->GetMapper()->GetInputDataObject(0, 0));
            if (pd && pd->GetNumberOfCells() > 0)
            {
              // save and restore prop changed when generating texture coords
              bool saveInterpScalars = aPart->GetMapper()->GetInterpolateScalarsBeforeMapping();
              foundVisibleProp = true;
              WriteMesh(archive, pd, aPart, this->FileName, meshCount);
              WriteTexture(aPart, this->FileName, meshCount, textureMap);
              // TODO, look at the data exported by vtkGLTFExporter, we'd want similar.
              // WriteMaterial(archive, meshCount, oldTextureCount != textures.size(), aPart);
              aPart->GetMapper()->SetInterpolateScalarsBeforeMapping(saveInterpScalars);
              ++meshCount;
            }
          }
        }
      }
    }
    // only write the camera if we had visible nodes
    if (foundVisibleProp)
    {
      WriteCamera(archive, ren);
    }
  }
}

void vtkAlembicExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << std::endl;
}
VTK_ABI_NAMESPACE_END
