/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextureIO.h"

#include "vtkTextureObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkDataSetWriter.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkPixelExtent.h"
#include "vtkPixelTransfer.h"
#include <cstddef>
#include <deque>
#include <sstream>

using std::string;
using std::deque;
using std::ostringstream;

//----------------------------------------------------------------------------
static
vtkFloatArray *DownloadTexture(
        vtkTextureObject *texture,
        const unsigned int *sub)
{
  int tt = texture->GetVTKDataType();
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();
  unsigned int tnc = texture->GetComponents();

  vtkPixelExtent texExt(0U, tw-1U, 0U, th-1U);
  vtkPixelExtent subExt(texExt);
  if (sub)
  {
    subExt.SetData(sub);
  }

  vtkFloatArray *ta = vtkFloatArray::New();
  ta->SetNumberOfComponents(tnc);
  ta->SetNumberOfTuples(subExt.Size());
  ta->SetName("tex");
  float *pTa = ta->GetPointer(0);

  vtkPixelBufferObject *pbo = texture->Download();

  vtkPixelTransfer::Blit(
        texExt,
        subExt,
        subExt,
        subExt,
        tnc,
        tt,
        pbo->MapPackedBuffer(),
        tnc,
        VTK_FLOAT,
        pTa);

  pbo->UnmapPackedBuffer();
  pbo->Delete();

  return ta;
}

//----------------------------------------------------------------------------
void vtkTextureIO::Write(
        const char *filename,
        vtkTextureObject *texture,
        const unsigned int *subset,
        const double *origin)
{
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();

  vtkPixelExtent subExt(0U, tw-1U, 0U, th-1U);
  if (subset)
  {
    subExt.SetData(subset);
  }

  int dataExt[6]={0,0, 0,0, 0,0};
  subExt.CellToNode();
  subExt.GetData(dataExt);

  double dataOrigin[6]={0,0, 0,0, 0,0};
  if (origin)
  {
    dataOrigin[0] = origin[0];
    dataOrigin[1] = origin[1];
  }

  vtkFloatArray *ta = DownloadTexture(texture, subset);

  vtkImageData *id = vtkImageData::New();
  id->SetExtent(dataExt);
  id->SetOrigin(dataOrigin);
  id->GetCellData()->AddArray(ta);
  ta->Delete();

  vtkDataSetWriter *w = vtkDataSetWriter::New();
  cerr << "writing to: " << filename << endl;
  w->SetFileName(filename);
  w->SetInputData(id);
  w->Write();

  id->Delete();
  w->Delete();
}

//----------------------------------------------------------------------------
void vtkTextureIO::Write(
        const char *filename,
        vtkTextureObject *texture,
        const deque<vtkPixelExtent> &exts,
        const double *origin)
{
  int n = static_cast<int>(exts.size());
  if (n == 0)
  {
    //vtkGenericWarningMacro("Empty extents nothing will be written");
    return;
  }
  vtkMultiBlockDataSet *mb = vtkMultiBlockDataSet::New();
  for (int i=0; i<n; ++i)
  {
    vtkPixelExtent ext = exts[i];
    if (ext.Empty()) continue;

    vtkFloatArray *ta = DownloadTexture(texture, ext.GetDataU());

    int dataExt[6]={0,0,0,0,0,0};
    ext.CellToNode();
    ext.GetData(dataExt);

    double dataOrigin[6]={0,0,0,0,0,0};
    if (origin)
    {
      dataOrigin[0] = origin[0];
      dataOrigin[1] = origin[1];
    }

    vtkImageData *id = vtkImageData::New();
    id->SetExtent(dataExt);
    id->SetOrigin(dataOrigin);
    id->GetCellData()->AddArray(ta);
    ta->Delete();

    mb->SetBlock(i, id);
    id->Delete();
  }

  vtkXMLMultiBlockDataWriter *w = vtkXMLMultiBlockDataWriter::New();
  cerr << "writing to: " << filename << endl;
  w->SetFileName(filename);
  w->SetInputData(mb);
  w->Write();
  w->Delete();
  mb->Delete();
}
