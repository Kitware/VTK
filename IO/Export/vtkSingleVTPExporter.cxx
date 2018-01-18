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
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkXMLPolyDataWriter.h"

vtkStandardNewMacro(vtkSingleVTPExporter);

vtkSingleVTPExporter::vtkSingleVTPExporter()
{
  this->FilePrefix = nullptr;
}

vtkSingleVTPExporter::~vtkSingleVTPExporter()
{
  delete [] this->FilePrefix;
}

void vtkSingleVTPExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkActor *anActor, *aPart;

  // make sure the user specified a filename
  if ( this->FilePrefix == nullptr)
  {
    vtkErrorMacro(<< "Please specify file prefix to use");
    return;
  }

  // first make sure there is only one renderer in this rendering window
  if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
  {
    vtkErrorMacro(<< "this exporter only supports one renderer per window.");
    return;
  }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for exporting.");
    return;
  }

  std::vector<actorData> actors;

  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  vtkCollectionSimpleIterator ait;
  bool haveTextures = false;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
  {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
    {
      aPart = static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
      if (aPart->GetVisibility() &&
          aPart->GetMapper() &&
          aPart->GetMapper()->GetInput() &&
          vtkPolyDataMapper::SafeDownCast(aPart->GetMapper()))
      {
        aPart->GetMapper()->GetInputAlgorithm()->Update();
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

  // we always produce a rgba texture as output if we have any textures
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  if (haveTextures)
  {
    this->WriteTexture(actors);
  }

  this->WriteVTP(actors);
}

void vtkSingleVTPExporter::WriteVTP(
  std::vector<actorData> &actors)
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
    actorData *ad = &(actors[i]);
    vtkDataArray *norms = static_cast<vtkPolyData *>(
      ad->Actor->GetMapper()->GetInput())->GetPointData()->GetNormals();
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

  int pointOffset = 0;
  for (size_t i = 0; i < actors.size(); ++i)
  {
    actorData *ad = &(actors[i]);
    triFilter->SetInputData(static_cast<vtkPolyData *>(
      ad->Actor->GetMapper()->GetInput()));
    triFilter->Update();
    vtkPolyData *ipd = triFilter->GetOutput();
    vtkPointData *iptd = ipd->GetPointData();

    // copy the points over
    vtkPoints *ipts = ipd->GetPoints();
    vtkIdType inpts = ipts->GetNumberOfPoints();
    for (int j = 0; j < inpts; ++j)
    {
      opts->InsertNextPoint(ipts->GetPoint(j));
    }

    // copy the tcoords over, create if missing
    vtkDataArray *itc = iptd->GetTCoords();
    if (!itc || !ad->Texture)
    {
      for (int j = 0; j < inpts; ++j)
      {
        otcoords->InsertNextTuple2(0.0,0.0);
      }
    }
    else
    {
      int dims[3];
      ad->Texture->GetInput()->GetDimensions(dims);
      double offset[2] = {
        static_cast<double>(ad->ImagePosition[0]) / this->TextureSize[0],
        static_cast<double>(ad->ImagePosition[1]) / this->TextureSize[1]
      };
      double scale[2] = {
        static_cast<double>(dims[0]) / this->TextureSize[0],
        static_cast<double>(dims[1]) / this->TextureSize[1]
      };
      for (int j = 0; j < inpts; ++j)
      {
        double *tmp = itc->GetTuple(j);
        if (tmp[0] > 1.05 || tmp[1] > 1.05 || tmp[0] < -0.05 || tmp[1] < -0.05)
        {
          vtkErrorMacro("Repeating texture found with coords of " << tmp[0] << " " << tmp[1]);
        }
        tmp[0] = vtkMath::Min(tmp[0],1.0);
        tmp[1] = vtkMath::Min(tmp[1],1.0);
        tmp[0] = vtkMath::Max(tmp[0],0.0);
        tmp[1] = vtkMath::Max(tmp[1],0.0);
        otcoords->InsertNextTuple2(
          tmp[0]*scale[0] + offset[0],
          tmp[1]*scale[1] + offset[1]);
      }
    }

    // copy the normals over if we have them
    if (haveNormals)
    {
      vtkFloatArray *otnormals = static_cast<vtkFloatArray *>(
        opd->GetPointData()->GetNormals());
      vtkDataArray *inorm = iptd->GetNormals();
      for (int j = 0; j < inpts; ++j)
      {
        double *tmp = inorm->GetTuple(j);
        otnormals->InsertNextTuple3(tmp[0], tmp[1], tmp[2]);
      }
    }

    // copy the scalars over, create if missing
    vtkDataArray *is = iptd->GetScalars();
    vtkProperty *prop = ad->Actor->GetProperty();
    double *dcolor = prop->GetDiffuseColor();
    double diffuse = prop->GetDiffuse();
    double *acolor = prop->GetAmbientColor();
    double ambient = prop->GetAmbient();
    double opacity = prop->GetOpacity();
    float col[4] = {
      static_cast<float>(
        vtkMath::Max(255.0*(dcolor[0]*diffuse + acolor[0]*ambient),255.0)),
      static_cast<float>(
        vtkMath::Max(255.0*(dcolor[1]*diffuse + acolor[1]*ambient),255.0)),
      static_cast<float>(
        vtkMath::Max(255.0*(dcolor[2]*diffuse + acolor[2]*ambient),255.0)),
      static_cast<float>(opacity*255.0)
    };
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
            double *tmp = itc->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0]*tmp[0],
              col[1]*tmp[0],
              col[2]*tmp[0],
              col[3]);
          }
          break;
        case 2:
          for (int j = 0; j < inpts; ++j)
          {
            double *tmp = itc->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0]*tmp[0],
              col[1]*tmp[0],
              col[2]*tmp[0],
              col[3]*tmp[1]);
          }
          break;
        case 3:
          for (int j = 0; j < inpts; ++j)
          {
            double *tmp = itc->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0]*tmp[0],
              col[1]*tmp[1],
              col[2]*tmp[2],
              col[3]);
          }
          break;
        case 4:
          for (int j = 0; j < inpts; ++j)
          {
            double *tmp = itc->GetTuple(j);
            oscalars->InsertNextTuple4(
              col[0]*tmp[0],
              col[1]*tmp[1],
              col[2]*tmp[2],
              col[3]*tmp[3]);
          }
          break;
      }
    }

    // copy the cells over
    vtkIdType npts;
    vtkIdType *cpts;
    vtkCellArray *ica;

    ica = ipd->GetVerts();
    ica->InitTraversal();
    while(ica->GetNextCell(npts, cpts))
    {
      overts->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        overts->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetLines();
    ica->InitTraversal();
    while(ica->GetNextCell(npts, cpts))
    {
      olines->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        olines->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetPolys();
    ica->InitTraversal();
    while(ica->GetNextCell(npts, cpts))
    {
      opolys->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        opolys->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }
    ica = ipd->GetStrips();
    ica->InitTraversal();
    while(ica->GetNextCell(npts, cpts))
    {
      ostrips->InsertNextCell(npts);
      for (int cp = 0; cp < npts; ++cp)
      {
        ostrips->InsertCellPoint(cpts[cp] + pointOffset);
      }
    }

    // increment values
    pointOffset += ipts->GetNumberOfPoints();
  }

  vtkNew<vtkXMLPolyDataWriter> writer;
  writer->SetDataModeToBinary();
  writer->SetInputData(opd);
  std::string fname = this->FilePrefix;
  fname += ".vtp";
  writer->SetFileName(fname.c_str());
  writer->Write();
}

void vtkSingleVTPExporter::WriteTexture(
  std::vector<actorData> &actors)
{
  // used to keep track of textures that are used by multiple actors
  std::map<vtkTexture *, actorData> knownTextures;

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
        totalPixels += (dims[0]*dims[1]);
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
    actorData *ad = &(actors[i]);
    if (ad->Texture)
    {
      // have we already see this texture?
      auto kti = knownTextures.find(ad->Texture);
      if (kti == knownTextures.end())
      {
        int dims[3];
        ad->Texture->GetInput()->GetDimensions(dims);

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
  unsigned char *opos = static_cast<unsigned char *>(
    oimage->GetScalarPointer(0, 0, 0));
  for (int ypos = 0; ypos < this->TextureSize[1]; ++ypos)
  {
    for (int xpos = 0; xpos < this->TextureSize[0]*4; ++xpos)
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
        vtkImageData *iimage = ad.Texture->GetInput();
        int dims[3];
        iimage->GetDimensions(dims);
        int iextent[6];
        iimage->GetExtent(iextent);
        // where to put the data
        int xpos = ad.ImagePosition[0];
        int ypos = ad.ImagePosition[1];
        int outExt[6] = {xpos, xpos + dims[0] - 1, ypos, ypos+dims[1] - 1, 0, 0};
        opos = static_cast<unsigned char *>(
          oimage->GetScalarPointer(xpos, ypos, 0));
        unsigned char *ipos = static_cast<unsigned char *>(
          iimage->GetScalarPointer(iextent[0], iextent[2], iextent[4]));

        vtkIdType outIncX, outIncY, outIncZ;
        int iNumComp = iimage->GetNumberOfScalarComponents();

        // Get increments to march through data
        oimage->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

        // copy the image
        for (int y = 0; y < dims[1]; ++y)
        {
          switch (iNumComp)
          {
            case 1:
            for (int x = 0; x < dims[0]; ++x)
            {
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
            for (int x = 0; x < dims[0]; ++x)
            {
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
            for (int x = 0; x < dims[0]; ++x)
            {
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
            for (int x = 0; x < dims[0]*4; ++x)
            {
              *opos = *ipos;
              opos++;
              ipos++;
            }
            break;
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
  this->Superclass::PrintSelf(os,indent);

  if (this->FilePrefix)
  {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
  }
  else
  {
    os << indent << "FilePrefix: (null)\n";
  }
}
