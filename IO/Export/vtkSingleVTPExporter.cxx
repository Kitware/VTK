/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSingleVTPExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSingleVTPExporter.h"

#include <map>

#include "vtkActorCollection.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLPolyDataWriter.h"

vtkStandardNewMacro(vtkSingleVTPExporter);

vtkSingleVTPExporter::vtkSingleVTPExporter()
{
  this->FilePrefix = nullptr;
}

vtkSingleVTPExporter::~vtkSingleVTPExporter()
{
  delete[] this->FilePrefix;
}

void vtkSingleVTPExporter::SetFileName(const char* fn)
{
  std::string prefix = fn;
  if (prefix.size() > 4 && prefix.substr(prefix.size() - 4, 4) == ".vtp")
  {
    prefix = prefix.substr(0, prefix.size() - 4);
    this->SetFilePrefix(prefix.c_str());
  }
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

}

void vtkSingleVTPExporter::WriteData()
{
  vtkRenderer* ren;

  // make sure the user specified a filename
  if (this->FilePrefix == nullptr)
  {
    vtkErrorMacro(<< "Please specify file prefix to use");
    return;
  }

  std::vector<actorData> actors;
  bool haveTextures = false;

  vtkRendererCollection* rc = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator rit;
  for (rc->InitTraversal(rit); (ren = rc->GetNextRenderer(rit));)
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
            if (pd)
            {
              actorData ad;
              ad.Actor = aPart;
              ad.Texture = aPart->GetTexture();
              if (ad.Texture)
              {
                haveTextures = true;
              }
              actors.push_back(ad);
            }
          }
        }
      }
    }
  }

  // we always produce a rgba texture as output if we have any textures
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  if (haveTextures)
  {
    this->WriteTexture(actors);
  }

  this->WriteVTP(actors);
}

// process an input triangle and generate one or more output triangles
// based on texture coordinates.
void vtkSingleVTPExporter::ProcessTriangle(const vtkIdType* pts, vtkPolyData* opd)
{
  vtkCellArray* newPolys = opd->GetPolys();
  vtkPoints* opts = opd->GetPoints();
  vtkPointData* optd = opd->GetPointData();
  vtkDataArray* otc = opd->GetPointData()->GetTCoords();

  double tcoord[3][3];
  otc->GetTuple(pts[0], tcoord[0]);
  otc->GetTuple(pts[1], tcoord[1]);
  otc->GetTuple(pts[2], tcoord[2]);
  double min[2];
  min[0] = tcoord[0][0];
  min[1] = tcoord[0][1];
  bool outside = false;
  for (int i = 0; i < 3; ++i)
  {
    if (tcoord[i][0] < min[0])
    {
      min[0] = tcoord[i][0];
    }
    if (tcoord[i][1] < min[1])
    {
      min[1] = tcoord[i][1];
    }
    if (tcoord[i][0] < 0.0 || tcoord[i][0] > 1.5 || tcoord[i][1] < 0.0 || tcoord[i][1] > 1.5)
    {
      outside = true;
    }
  }

  // Step 1 if a triangle is already fine (with no
  // texture coordinates outside the allowed range)
  // then just pass it to the output cell array with no new
  // point data as none is needed.
  if (!outside)
  {
    newPolys->InsertNextCell(3, pts);
    return;
  }

  // copy the points so we can adjust the tcoords
  double oplocs[3][3];
  opts->GetPoint(pts[0], oplocs[0]);
  opts->GetPoint(pts[1], oplocs[1]);
  opts->GetPoint(pts[2], oplocs[2]);

  // adjust the tcoord range to start between 0-1
  int adjust[2] = { static_cast<int>(-floor(min[0])), static_cast<int>(-floor(min[1])) };
  for (int i = 0; i < 3; i++)
  {
    tcoord[i][0] += adjust[0];
    tcoord[i][1] += adjust[1];
  }

  // does adjusting solve the issue? If so add 3 points and return
  outside = false;
  for (int i = 0; i < 3; ++i)
  {
    if (tcoord[i][0] < 0.0 || tcoord[i][0] > 1.5 || tcoord[i][1] < 0.0 || tcoord[i][1] > 1.5)
    {
      outside = true;
    }
  }

  // Step 2 - if a simple shifting of the texture coordinate
  // works, then add new points and data for the shifted values
  // and insert a triangle using those shifted points
  if (!outside)
  {
    vtkIdType cptids[3];
    for (int i = 0; i < 3; i++)
    {
      cptids[i] = opts->InsertNextPoint(oplocs[i]);
      optd->CopyData(optd, pts[i], cptids[i]);
      otc->SetTuple(cptids[i], tcoord[i]);
    }
    newPolys->InsertNextCell(3, cptids);
    return;
  }

  // Step 3, neither of the above worked so instead
  // subdivide the triangle into 4 and recurse
  // add 3 points and interpolated data for them
  vtkIdType nptids[3];
  for (int i = 0; i < 3; i++)
  {
    nptids[i] = opts->InsertNextPoint((oplocs[i][0] + oplocs[(i + 1) % 3][0]) * 0.5,
      (oplocs[i][1] + oplocs[(i + 1) % 3][1]) * 0.5, (oplocs[i][2] + oplocs[(i + 1) % 3][2]) * 0.5);
    optd->InterpolateEdge(optd, nptids[i], pts[i], pts[(i + 1) % 3], 0.5);
  }
  vtkIdType newtri[3];
  newtri[0] = pts[0];
  newtri[1] = nptids[0];
  newtri[2] = nptids[2];
  this->ProcessTriangle(newtri, opd);
  newtri[0] = pts[1];
  newtri[1] = nptids[1];
  newtri[2] = nptids[0];
  this->ProcessTriangle(newtri, opd);
  newtri[0] = pts[2];
  newtri[1] = nptids[2];
  newtri[2] = nptids[1];
  this->ProcessTriangle(newtri, opd);
  this->ProcessTriangle(nptids, opd);
}

// for an input polydata with texture coordinates handle any
// triangles with repeating textures. Basically calls
// process triangle for each input triangle.
vtkPolyData* vtkSingleVTPExporter::FixTextureCoordinates(vtkPolyData* ipd)
{
  // do we have tcoords and are they out of range, if not just return
  vtkPolyData* opd = vtkPolyData::New();
  vtkNew<vtkPoints> opts;
  opts->SetDataTypeToDouble();
  opts->DeepCopy(ipd->GetPoints());
  opd->SetPoints(opts);
  vtkPointData* iptd = ipd->GetPointData();
  vtkPointData* optd = opd->GetPointData();
  optd->CopyAllOn();
  optd->InterpolateAllocate(iptd, ipd->GetPoints()->GetNumberOfPoints());
  optd->CopyData(iptd, 0, ipd->GetPoints()->GetNumberOfPoints(), 0);

  vtkCellArray* newPolys = nullptr;
  if (ipd->GetPolys()->GetNumberOfCells() > 0)
  {
    vtkCellArray* cells = ipd->GetPolys();
    const vtkIdType* pts = nullptr;
    vtkIdType npts;
    newPolys = vtkCellArray::New();
    newPolys->AllocateEstimate(cells->GetNumberOfCells(), 3);
    opd->SetPolys(newPolys);
    vtkIdList* ptIds = vtkIdList::New();
    ptIds->Allocate(VTK_CELL_SIZE);

    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      // does this triangle go outside of 0 to 1.5 in tcoords?
      this->ProcessTriangle(pts, opd);
    }
    newPolys->Delete();
    ptIds->Delete();
  }

  return opd;
}

void vtkSingleVTPExporter::WriteVTP(std::vector<actorData>& actors)
{
  // we have to build a big polydata that will have
  //   points
  //   tcoords
  //   4 uchar scalars
  //   and verts/lines/tris/strips

  vtkNew<vtkPolyData> opd;
  vtkNew<vtkPoints> opts;
  opts->SetDataTypeToDouble();
  opd->SetPoints(opts);
  vtkNew<vtkCellArray> overts;
  vtkNew<vtkCellArray> olines;
  vtkNew<vtkCellArray> opolys;
  vtkNew<vtkCellArray> ostrips;
  opd->SetVerts(overts);
  opd->SetLines(olines);
  opd->SetPolys(opolys);
  opd->SetStrips(ostrips);
  vtkNew<vtkFloatArray> otcoords;
  otcoords->SetNumberOfComponents(2);
  opd->GetPointData()->SetTCoords(otcoords);
  vtkNew<vtkUnsignedCharArray> oscalars;
  oscalars->SetNumberOfComponents(4);
  opd->GetPointData()->SetScalars(oscalars);

  vtkNew<vtkTriangleFilter> triFilter;

  // look to see if we have point normals
  // on every piece
  bool haveNormals = true;
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    vtkPolyData* mypd = findPolyData(ad->Actor->GetMapper()->GetInputDataObject(0, 0));
    vtkDataArray* norms = mypd->GetPointData()->GetNormals();
    if (!norms)
    {
      haveNormals = false;
      break;
    }
  }
  if (haveNormals)
  {
    vtkNew<vtkFloatArray> otnormals;
    otnormals->SetNumberOfComponents(3);
    opd->GetPointData()->SetNormals(otnormals);
  }

  // compute the maximum color in case it goes over 1.0
  float maxColor = 1.0;
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    vtkProperty* prop = ad->Actor->GetProperty();
    double* dcolor = prop->GetDiffuseColor();
    double diffuse = prop->GetDiffuse();
    double* acolor = prop->GetAmbientColor();
    double ambient = prop->GetAmbient();
    float col[3] = { static_cast<float>(dcolor[0] * diffuse + acolor[0] * ambient),
      static_cast<float>(dcolor[1] * diffuse + acolor[1] * ambient),
      static_cast<float>(dcolor[2] * diffuse + acolor[2] * ambient) };
    if (col[0] > maxColor)
    {
      maxColor = col[0];
    }
    if (col[1] > maxColor)
    {
      maxColor = col[1];
    }
    if (col[2] > maxColor)
    {
      maxColor = col[2];
    }
  }
  maxColor = 255.0 / maxColor;

  int pointOffset = 0;
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    vtkPolyData* mypd = findPolyData(ad->Actor->GetMapper()->GetInputDataObject(0, 0));
    triFilter->SetInputData(mypd);
    triFilter->Update();
    vtkPolyData* ipd = triFilter->GetOutput();
    vtkPoints* ipts = ipd->GetPoints();
    if (!ipts)
    {
      continue;
    }
    if (ad->Texture &&
      (ad->URange[0] < 0.0 || ad->URange[1] > 1.0 || ad->VRange[0] < 0.0 || ad->VRange[1] > 1.0))
    {
      if (ipts->GetNumberOfPoints() != ipd->GetPointData()->GetTCoords()->GetNumberOfTuples())
      {
        vtkErrorMacro("Bad input data");
      }
      ipd = this->FixTextureCoordinates(ipd);
      ipts = ipd->GetPoints();
    }
    else
    {
      ipd->Register(this);
    }
    vtkPointData* iptd = ipd->GetPointData();

    // copy the points over
    vtkIdType inpts = ipts->GetNumberOfPoints();
    for (int j = 0; j < inpts; ++j)
    {
      opts->InsertNextPoint(ipts->GetPoint(j));
    }

    // copy the tcoords over, create if missing
    vtkDataArray* itc = iptd->GetTCoords();
    if (!itc || !ad->Texture)
    {
      for (int j = 0; j < inpts; ++j)
      {
        otcoords->InsertNextTuple2(0.0, 0.0);
      }
    }
    else
    {
      int dims[3];
      ad->Texture->GetInput()->GetDimensions(dims);
      double offset[2] = { static_cast<double>(ad->ImagePosition[0]) / this->TextureSize[0],
        static_cast<double>(ad->ImagePosition[1]) / this->TextureSize[1] };
      double scale[2] = { static_cast<double>(dims[0]) / this->TextureSize[0],
        static_cast<double>(dims[1]) / this->TextureSize[1] };
      for (int j = 0; j < inpts; ++j)
      {
        double* tmp = itc->GetTuple(j);
        otcoords->InsertNextTuple2(tmp[0] * scale[0] + offset[0], tmp[1] * scale[1] + offset[1]);
      }
    }

    // copy the normals over if we have them
    if (haveNormals)
    {
      vtkFloatArray* otnormals = static_cast<vtkFloatArray*>(opd->GetPointData()->GetNormals());
      vtkDataArray* inorm = iptd->GetNormals();
      for (int j = 0; j < inpts; ++j)
      {
        double* tmp = inorm->GetTuple(j);
        otnormals->InsertNextTuple3(tmp[0], tmp[1], tmp[2]);
      }
    }

    // copy the scalars over, create if missing
    vtkDataArray* is = iptd->GetScalars();
    vtkProperty* prop = ad->Actor->GetProperty();
    double* dcolor = prop->GetDiffuseColor();
    double diffuse = prop->GetDiffuse();
    double* acolor = prop->GetAmbientColor();
    double ambient = prop->GetAmbient();
    double opacity = prop->GetOpacity();
    float col[4] = { static_cast<float>(
                       vtkMath::Min(maxColor * (dcolor[0] * diffuse + acolor[0] * ambient), 255.0)),
      static_cast<float>(
        vtkMath::Min(maxColor * (dcolor[1] * diffuse + acolor[1] * ambient), 255.0)),
      static_cast<float>(
        vtkMath::Min(maxColor * (dcolor[2] * diffuse + acolor[2] * ambient), 255.0)),
      static_cast<float>(opacity * 255.0) };
    if (!is)
    {
      for (int j = 0; j < inpts; ++j)
      {
        oscalars->InsertNextTuple(col);
      }
    }
    else
    {
      switch (is->GetNumberOfComponents())
      {
        case 1:
          for (int j = 0; j < inpts; ++j)
          {
            double* tmp = is->GetTuple(j);
            oscalars->InsertNextTuple4(col[0] * tmp[0], col[1] * tmp[0], col[2] * tmp[0], col[3]);
          }
          break;
        case 2:
          for (int j = 0; j < inpts; ++j)
          {
            double* tmp = is->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0] * tmp[0], col[1] * tmp[0], col[2] * tmp[0], col[3] * tmp[1]);
          }
          break;
        case 3:
          for (int j = 0; j < inpts; ++j)
          {
            double* tmp = is->GetTuple(j);
            oscalars->InsertNextTuple4(col[0] * tmp[0], col[1] * tmp[1], col[2] * tmp[2], col[3]);
          }
          break;
        case 4:
          for (int j = 0; j < inpts; ++j)
          {
            double* tmp = is->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0] * tmp[0], col[1] * tmp[1], col[2] * tmp[2], col[3] * tmp[3]);
          }
          break;
      }
    }

    // copy the cells over
    vtkIdType npts;
    const vtkIdType* cpts;
    vtkCellArray* ica;

    ica = ipd->GetVerts();
    ica->InitTraversal();
    while (ica->GetNextCell(npts, cpts))
    {
      overts->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        overts->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetLines();
    ica->InitTraversal();
    while (ica->GetNextCell(npts, cpts))
    {
      olines->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        olines->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetPolys();
    ica->InitTraversal();
    while (ica->GetNextCell(npts, cpts))
    {
      opolys->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        opolys->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetStrips();
    ica->InitTraversal();
    while (ica->GetNextCell(npts, cpts))
    {
      ostrips->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        ostrips->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }

    // increment values
    pointOffset += ipts->GetNumberOfPoints();
    ipd->UnRegister(this);
  }

  vtkNew<vtkXMLPolyDataWriter> writer;
  writer->SetDataModeToBinary();
  writer->SetInputData(opd);
  std::string fname = this->FilePrefix;
  fname += ".vtp";
  writer->SetFileName(fname.c_str());
  writer->Write();
}

void vtkSingleVTPExporter::WriteTexture(std::vector<actorData>& actors)
{
  // used to keep track of textures that are used by multiple actors
  std::map<vtkTexture*, actorData> knownTextures;

  // look for repeating textures
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    vtkPolyData* mypd = findPolyData(ad->Actor->GetMapper()->GetInputDataObject(0, 0));
    vtkDataArray* tcoords = mypd->GetPointData()->GetTCoords();
    ad->HaveRepeatingTexture = false;
    if (tcoords)
    {
      tcoords->GetRange(ad->URange, 0);
      tcoords->GetRange(ad->VRange, 1);
      if (ad->URange[0] < 0.0 || ad->URange[1] > 1.0 || ad->VRange[0] < 0.0 || ad->VRange[1] > 1.0)
      {
        ad->HaveRepeatingTexture = true;
      }
    }
  }

  // make sure HaveRepeating is shared by all
  // actors that use that texture
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    if (ad->HaveRepeatingTexture)
    {
      for (size_t j = 0; j < actors.size(); ++j)
      {
        actorData* ad2 = &(actors[j]);
        if (ad2->Texture == ad->Texture)
        {
          ad2->HaveRepeatingTexture = true;
        }
      }
    }
  }

  // create an image data and start filling it up
  int maxXDim = 0;
  int totalPixels = 0;
  for (auto ad : actors)
  {
    if (ad.Texture)
    {
      // have we already see this texture?
      auto kti = knownTextures.find(ad.Texture);
      if (kti == knownTextures.end())
      {
        int dims[3];
        ad.Texture->GetInput()->GetDimensions(dims);
        // if repeating then we add 50% on X, Y
        if (ad.HaveRepeatingTexture)
        {
          dims[0] *= 1.5;
          dims[1] *= 1.5;
        }
        totalPixels += (dims[0] * dims[1]);
        if (dims[0] > maxXDim)
        {
          maxXDim = dims[0];
        }
        knownTextures[ad.Texture] = ad;
      }
    }
  }
  // ~= the minimum x dim for a perfectly packed texture
  int minXDim = ceil(sqrt(totalPixels));
  if (minXDim < maxXDim)
  {
    minXDim = maxXDim;
  }

  knownTextures.clear();

  // now start placing textures into the image
  // this pass just computes the sizes so we can allocate and copy just once
  int currY = 0;
  int currX = 1;
  int rowMaxY = 0;
  int imageMaxX = 0;
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData* ad = &(actors[i]);
    if (ad->Texture)
    {
      // have we already see this texture?
      auto kti = knownTextures.find(ad->Texture);
      if (kti == knownTextures.end())
      {
        int dims[3];
        ad->Texture->GetInput()->GetDimensions(dims);
        // if repeating then we add 50% on X, Y
        if (ad->HaveRepeatingTexture)
        {
          dims[0] *= 1.5;
          dims[1] *= 1.5;
        }

        // start a new row?
        if (currX && dims[0] + currX > minXDim)
        {
          currY += rowMaxY;
          rowMaxY = 0;
          currX = 1;
        }
        ad->ImagePosition[0] = currX;
        ad->ImagePosition[1] = currY;
        currX += dims[0];
        if (rowMaxY < dims[1])
        {
          rowMaxY = dims[1];
        }
        if (imageMaxX < currX)
        {
          imageMaxX = currX;
        }
        knownTextures[ad->Texture] = *ad;
      }
      else
      {
        ad->ImagePosition[0] = kti->second.ImagePosition[0];
        ad->ImagePosition[1] = kti->second.ImagePosition[0];
      }
    }
  }

  // resulting image is
  this->TextureSize[0] = imageMaxX + 1;
  this->TextureSize[1] = currY + rowMaxY;

  // allocate the image
  vtkNew<vtkImageData> oimage;
  oimage->SetDimensions(this->TextureSize[0], this->TextureSize[1], 1);
  oimage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  // initialize to white
  unsigned char* opos = static_cast<unsigned char*>(oimage->GetScalarPointer(0, 0, 0));
  for (int ypos = 0; ypos < this->TextureSize[1]; ++ypos)
  {
    for (int xpos = 0; xpos < this->TextureSize[0] * 4; ++xpos)
    {
      *opos = 255;
      opos++;
    }
  }

  knownTextures.clear();

  // now copy the textures into it and store the x, y offsets per texture
  for (auto ad : actors)
  {
    if (ad.Texture)
    {
      // have we already see this texture?
      auto kti = knownTextures.find(ad.Texture);
      if (kti == knownTextures.end())
      {
        vtkImageData* iimage = ad.Texture->GetInput();
        int dims[3];
        iimage->GetDimensions(dims);
        int rdims[3];
        iimage->GetDimensions(rdims);
        // if repeating then we add 50% on X, Y
        if (ad.HaveRepeatingTexture)
        {
          rdims[0] *= 1.5;
          rdims[1] *= 1.5;
        }
        int iextent[6];
        iimage->GetExtent(iextent);
        // where to put the data
        int xpos = ad.ImagePosition[0];
        int ypos = ad.ImagePosition[1];
        int outExt[6] = { xpos, xpos + rdims[0] - 1, ypos, ypos + rdims[1] - 1, 0, 0 };
        opos = static_cast<unsigned char*>(oimage->GetScalarPointer(xpos, ypos, 0));
        unsigned char* ipos =
          static_cast<unsigned char*>(iimage->GetScalarPointer(iextent[0], iextent[2], iextent[4]));

        vtkIdType outIncX, outIncY, outIncZ;
        int iNumComp = iimage->GetNumberOfScalarComponents();

        // Get increments to march through data
        oimage->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

        // copy the image
        for (int y = 0; y < rdims[1]; ++y)
        {
          // when we start repeating on Y reset to the beginning
          // of the texture
          if (y == dims[1])
          {
            ipos = static_cast<unsigned char*>(
              iimage->GetScalarPointer(iextent[0], iextent[2], iextent[4]));
          }
          switch (iNumComp)
          {
            case 1:
              for (int x = 0; x < rdims[0]; ++x)
              {
                if (x == dims[0])
                {
                  ipos -= dims[0];
                }
                *opos = *ipos;
                opos++;
                *opos = *ipos;
                opos++;
                *opos = *ipos;
                opos++;
                *opos = 255;
                opos++;
                ipos++;
              }
              break;
            case 2:
              for (int x = 0; x < rdims[0]; ++x)
              {
                if (x == dims[0])
                {
                  ipos -= dims[0] * 2;
                }
                *opos = *ipos;
                opos++;
                *opos = *ipos;
                opos++;
                *opos = *ipos;
                opos++;
                ipos++;
                *opos = *ipos;
                opos++;
                ipos++;
              }
              break;
            case 3:
              for (int x = 0; x < rdims[0]; ++x)
              {
                if (x == dims[0])
                {
                  ipos -= dims[0] * 3;
                }
                *opos = *ipos;
                opos++;
                ipos++;
                *opos = *ipos;
                opos++;
                ipos++;
                *opos = *ipos;
                opos++;
                ipos++;
                *opos = 255;
                opos++;
              }
              break;
            case 4:
              for (int x = 0; x < rdims[0]; ++x)
              {
                if (x == dims[0])
                {
                  ipos -= dims[0] * 4;
                }
                memcpy(opos, ipos, 4);
                opos += 4;
                ipos += 4;
              }
              break;
          }
          if (rdims[0] > dims[0])
          {
            ipos += (dims[0] * 2 - rdims[0]) * iNumComp;
          }
          opos += outIncY;
        }
        knownTextures[ad.Texture] = ad;
      }
    }
  }

  vtkNew<vtkPNGWriter> writer;
  writer->SetInputData(oimage);
  std::string fname = this->FilePrefix;
  fname += ".png";
  writer->SetFileName(fname.c_str());
  writer->Write();
}

void vtkSingleVTPExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FilePrefix)
  {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
  }
  else
  {
    os << indent << "FilePrefix: (null)\n";
  }
}
