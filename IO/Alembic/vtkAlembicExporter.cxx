// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlembicExporter.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

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
#include "vtkStringFormatter.h"
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

namespace
{

vtkPolyData* FindPolyData(vtkDataObject* input)
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

// True if both arrays have identical shape and byte content.
bool TextureContentsEqual(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b)
{
  if (!a || !b)
  {
    return false;
  }
  if (a->GetNumberOfTuples() != b->GetNumberOfTuples() ||
    a->GetNumberOfComponents() != b->GetNumberOfComponents())
  {
    return false;
  }
  vtkIdType n = a->GetNumberOfTuples() * a->GetNumberOfComponents();
  if (n == 0)
  {
    return true;
  }
  return std::memcmp(a->GetPointer(0), b->GetPointer(0), static_cast<size_t>(n)) == 0;
}

} // end anonymous namespace

// Holds state that must persist across multiple WriteData() calls when
// accumulating timesteps into a single archive (i.e. between Start() and
// Finish()). Alembic objects (OXform/OPolyMesh/OCamera) must be created
// exactly once and then reused: creating a second "mesh_0" under the same
// parent on a later frame would create a second, sibling object rather than
// add a time sample to the existing one.
class vtkAlembicExporter::vtkAlembicExporterInternals
{
public:
  void Reset()
  {
    this->Archive = nullptr;
    this->SampleTimes.clear();
    this->FirstFrameMeshCount = 0;
    this->FrameIndex = 0;
    this->ActorXforms.clear();
    this->ActorMeshes.clear();
    this->ActorHasColor.clear();
    this->ActorColorParams.clear();
    this->PreviousTextureData.clear();
    this->HasCamera = false;
    this->CameraXform = OXform();
    this->Camera = OCamera();
  }

  void WriteMesh(bool isFirstFrame, vtkPolyData* pd, vtkActor* aPart, size_t index)
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

    // The xform/mesh pair for a given index is created exactly once and
    // reused on subsequent frames: Alembic objects, like USD prims, can't be
    // structurally added to an already-created schema, only their per-sample
    // data can vary via repeated set() calls.
    bool needNewObjects =
      isFirstFrame || this->ActorXforms.size() <= index || !this->ActorXforms[index].valid();
    if (needNewObjects)
    {
      if (this->ActorXforms.size() <= index)
      {
        this->ActorXforms.resize(index + 1);
        this->ActorMeshes.resize(index + 1);
        this->ActorHasColor.resize(index + 1, false);
        this->ActorColorParams.resize(index + 1);
      }

      std::string xform_name = "xform_" + vtk::to_string(index);
      this->ActorXforms[index] = OXform(OObject(*this->Archive, kTop), xform_name);

      std::string mesh_name = "mesh_" + vtk::to_string(index);
      this->ActorMeshes[index] = OPolyMesh(this->ActorXforms[index], mesh_name);
    }

    OXform& xform = this->ActorXforms[index];
    OPolyMeshSchema& mesh = this->ActorMeshes[index].getSchema();

    // set the transform in an alembic node.
    XformSample xformSamp;
    XformOp transop(kMatrixOperation, kMatrixHint);
    xformSamp.addOp(transop, camMatrix);
    xform.getSchema().set(xformSamp);

    // write the point locations
    vtkNew<vtkFloatArray> pointData;
    if (auto floatArray = vtkFloatArray::FastDownCast(tris->GetPoints()->GetData()))
    {
      pointData->ShallowCopy(floatArray);
    }
    else
    {
      pointData->DeepCopy(tris->GetPoints()->GetData());
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
        V2fArraySample((const V2f*)tcoords->GetPointer(0), tcoords->GetNumberOfTuples()));
      // this means per-vertex, vtkFaceVaryingScope means per-vertex-per-face.
      uvsamp.setScope(kVertexScope);
    }

    if (!ia.empty())
    {
      OPolyMeshSchema::Sample meshSamp(
        V3fArraySample((const V3f*)pointData->GetPointer(0), pointData->GetNumberOfTuples()),
        Int32ArraySample(ia.data(), ia.size()), Int32ArraySample(counts.data(), counts.size()),
        uvsamp, ON3fGeomParam::Sample());
      mesh.set(meshSamp);
    }

    if (vertColor)
    {
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

      if (!this->ActorHasColor[index])
      {
        OCompoundProperty arbParams = mesh.getArbGeomParams();
        // "rgba" is a magic name for some Alembic imports, 3DSMax
        this->ActorColorParams[index] = OC4fGeomParam(arbParams, "rgba", false, kVertexScope, 1);
        this->ActorHasColor[index] = true;
      }

      OC4fGeomParam::Sample colorSamp(valSamp, kVertexScope);
      this->ActorColorParams[index].set(colorSamp);
    }
  }

  void WriteCamera(vtkRenderer* ren)
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

    if (!this->HasCamera)
    {
      this->CameraXform = OXform(OObject(*this->Archive, kTop), "camXform");
      this->Camera = OCamera(this->CameraXform, "cam");
      this->HasCamera = true;
    }

    // set the transform in an alembic node.
    XformSample xformSamp;
    XformOp transop(kMatrixOperation, kMatrixHint);
    xformSamp.addOp(transop, camMatrix);
    this->CameraXform.getSchema().set(xformSamp);

    vtkCamera* cam = ren->GetActiveCamera();
    CameraSample samp;

    OCameraSchema camSchema = this->Camera.getSchema();
    samp.setNearClippingPlane(cam->GetClippingRange()[0]);
    samp.setFarClippingPlane(cam->GetClippingRange()[1]);
    camSchema.set(samp);

    // TODO translate the FOV into something for Alembic
    // Parallel projection may not be available - it's non-physical.
    // if (cam->GetParallelProjection())
  }

  size_t WriteTexture(bool started, size_t frameIndex, vtkActor* aPart, const char* fileName,
    size_t index, std::map<vtkUnsignedCharArray*, size_t>& textureMap)
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

      // When accumulating multiple timesteps into a single archive, only
      // write a new texture image if its contents actually changed since the
      // last frame that wrote one for this slot; a static texture shouldn't
      // be duplicated on every frame.
      bool contentChanged = true;
      if (started && index < this->PreviousTextureData.size() && this->PreviousTextureData[index])
      {
        contentChanged = !TextureContentsEqual(this->PreviousTextureData[index], da);
      }

      if (contentChanged)
      {
        // figure out a filename - strip extension, add "_tex0.png"
        std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);
        std::string baseName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);
        std::string fname = filePath + "/" + baseName + "_tex" + vtk::to_string(index);
        if (started)
        {
          fname += "_frame" + vtk::to_string(frameIndex);
        }
        fname += ".png";

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

        if (started)
        {
          if (this->PreviousTextureData.size() <= index)
          {
            this->PreviousTextureData.resize(index + 1);
          }
          vtkNew<vtkUnsignedCharArray> snapshot;
          snapshot->DeepCopy(da);
          this->PreviousTextureData[index] = snapshot;
        }
      }

      textureSource = texIndex;
    }
    else
    {
      textureSource = textureMap[da];
    }
    return textureSource;
  }

  // Finalizes and closes the archive: builds the real TimeSampling from every
  // recorded TimeValue, points every persisted xform/mesh/camera schema at
  // it, then destroys the archive, which is what actually flushes it to disk.
  void FinalizeArchive()
  {
    if (!this->Archive)
    {
      return;
    }

    TimeSampling timeSampling(TimeSamplingType(TimeSamplingType::kAcyclic), this->SampleTimes);
    uint32_t tsIndex = this->Archive->addTimeSampling(timeSampling);

    for (auto& xform : this->ActorXforms)
    {
      if (xform.valid())
      {
        xform.getSchema().setTimeSampling(tsIndex);
      }
    }
    for (auto& meshObj : this->ActorMeshes)
    {
      if (meshObj.valid())
      {
        meshObj.getSchema().setTimeSampling(tsIndex);
      }
    }
    if (this->HasCamera)
    {
      this->CameraXform.getSchema().setTimeSampling(tsIndex);
      this->Camera.getSchema().setTimeSampling(tsIndex);
    }

    this->Archive = nullptr;
  }

  // True if an archive has already been created (i.e. this is not the first
  // WriteData() call of the export).
  bool HasArchive() const { return this->Archive != nullptr; }

  // Creates a new, empty archive at fileName, replacing any existing one.
  void CreateArchive(const char* fileName)
  {
    this->Archive = std::make_unique<OArchive>(Alembic::AbcCoreOgawa::WriteArchive(), fileName);
  }

  // Records a time sample for the current frame; every value recorded since
  // Start() is used to build the archive's final TimeSampling at Finish().
  void AddSampleTime(double timeValue) { this->SampleTimes.push_back(timeValue); }

  // Returns the current frame index and increments it, used to give each
  // frame's texture images distinct filenames (see WriteTexture).
  size_t NextFrameIndex() { return this->FrameIndex++; }

  // Tracks how many meshes were present in the first exported frame so later
  // frames can detect whether a given mesh was created in the initial write.
  size_t GetFirstFrameMeshCount() const { return this->FirstFrameMeshCount; }

  void SetFirstFrameMeshCount(size_t count) { this->FirstFrameMeshCount = count; }

private:
  std::unique_ptr<OArchive> Archive;

  // Every TimeValue seen since Start(), used to build the archive's final
  // TimeSampling at Finish().
  std::vector<double> SampleTimes;

  size_t FirstFrameMeshCount = 0;

  // Counts WriteData() calls since Start(), used to give each frame's
  // texture images distinct filenames (see WriteTexture).
  size_t FrameIndex = 0;

  std::vector<OXform> ActorXforms;
  std::vector<OPolyMesh> ActorMeshes;
  std::vector<bool> ActorHasColor;
  std::vector<OC4fGeomParam> ActorColorParams;

  // Deep copy of the last texture data actually written for each mesh
  // index, used to avoid writing a duplicate PNG when a texture hasn't
  // changed since the previous frame.
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>> PreviousTextureData;

  bool HasCamera = false;
  OXform CameraXform;
  OCamera Camera;
}; // end vtkAlembicExporter::vtkAlembicExporterInternals

vtkStandardNewMacro(vtkAlembicExporter);

vtkAlembicExporter::vtkAlembicExporter()
{
  this->FileName = nullptr;
  this->Internal = new vtkAlembicExporterInternals;
}

vtkAlembicExporter::~vtkAlembicExporter()
{
  delete[] this->FileName;
  delete this->Internal;
}

void vtkAlembicExporter::Start()
{
  this->Internal->Reset();
  this->Started = true;
}

void vtkAlembicExporter::Finish()
{
  if (!this->Started)
  {
    return;
  }

  this->Internal->FinalizeArchive();

  this->Internal->Reset();
  this->Started = false;
}

void vtkAlembicExporter::WriteData()
{
  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // Determine whether this call is part of a multi-frame, single-file
  // export (this->Started), and if so, whether it is the first frame of
  // that sequence, i.e. whether the shared archive still needs to be
  // created.
  bool isFirstFrame = true;
  if (this->Started)
  {
    if (this->Internal->HasArchive())
    {
      isFirstFrame = false;
    }
    else
    {
      this->Internal->CreateArchive(this->FileName);
      isFirstFrame = true;
    }
  }
  else
  {
    // Legacy, single-shot behavior: always a fresh, self-contained archive.
    this->Internal->Reset();
    this->Internal->CreateArchive(this->FileName);
    isFirstFrame = true;
  }

  if (!this->Internal->HasArchive())
  {
    vtkErrorMacro(<< "Failed to create Alembic archive for file: " << this->FileName);
    return;
  }

  size_t frameIndex = this->Internal->NextFrameIndex();
  this->Internal->AddSampleTime(this->TimeValue);

  // support sharing texture maps between actors within a single frame
  std::map<vtkUnsignedCharArray*, size_t> textureMap;

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
            vtkPolyData* pd = FindPolyData(aPart->GetMapper()->GetInputDataObject(0, 0));
            if (pd && pd->GetNumberOfCells() > 0)
            {
              // save and restore prop changed when generating texture coords
              bool saveInterpScalars = aPart->GetMapper()->GetInterpolateScalarsBeforeMapping();
              foundVisibleProp = true;
              this->Internal->WriteMesh(isFirstFrame, pd, aPart, meshCount);
              this->Internal->WriteTexture(
                this->Started, frameIndex, aPart, this->FileName, meshCount, textureMap);
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
      this->Internal->WriteCamera(ren);
    }
  }

  if (this->Started)
  {
    if (isFirstFrame)
    {
      this->Internal->SetFirstFrameMeshCount(meshCount);
    }
    else if (meshCount != this->Internal->GetFirstFrameMeshCount())
    {
      vtkWarningMacro(<< "The number of visible meshes changed from "
                      << this->Internal->GetFirstFrameMeshCount() << " to " << meshCount
                      << " between frames of a single-file animated Alembic export. Alembic "
                         "objects cannot be added or removed once the archive has been "
                         "created; only the geometry of already-created meshes can vary "
                         "between frames.");
    }
  }
  else
  {
    // Legacy behavior: finalize and close the archive immediately.
    this->Internal->FinalizeArchive();
  }
}

void vtkAlembicExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << std::endl;
}
VTK_ABI_NAMESPACE_END
