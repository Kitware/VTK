/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSExporter.h"

#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"

#include "vtk_gl2ps.h"

#include <vector>

vtkAbstractObjectFactoryNewMacro(vtkGL2PSExporter)
vtkCxxSetObjectMacro(vtkGL2PSExporter, RasterExclusions, vtkPropCollection)

vtkGL2PSExporter::vtkGL2PSExporter()
{
  this->RasterExclusions = NULL;
  this->FilePrefix = NULL;
  this->BufferSize = 4194304; // 4MB
  this->Title = NULL;
  this->FileFormat = EPS_FILE;
  this->Sort = SIMPLE_SORT;
  this->Compress = 1;
  this->DrawBackground = 1;
  this->SimpleLineOffset = 1;
  this->Silent = 0;
  this->BestRoot = 1;
  this->Text = 1;
  this->Landscape = 0;
  this->PS3Shading = 1;
  this->OcclusionCull = 1;
  this->Write3DPropsAsRasterImage = 0;
  this->TextAsPath = false;
  this->PointSizeFactor = 5.f / 7.f;
  this->LineWidthFactor = 5.f / 7.f;
}

vtkGL2PSExporter::~vtkGL2PSExporter()
{
  this->SetRasterExclusions(NULL);
  delete [] this->FilePrefix;
  delete [] this->Title;
}

int vtkGL2PSExporter::GetGL2PSOptions()
{
  GLint options = GL2PS_NONE;
  if (this->Compress == 1)
  {
    options = options | GL2PS_COMPRESS;
  }
  if (this->DrawBackground == 1)
  {
    options = options | GL2PS_DRAW_BACKGROUND;
  }
  if (this->SimpleLineOffset == 1)
  {
    options = options | GL2PS_SIMPLE_LINE_OFFSET;
  }
  if (this->Silent == 1)
  {
    options = options | GL2PS_SILENT;
  }
  if (this->BestRoot == 1)
  {
    options = options | GL2PS_BEST_ROOT;
  }
  if (this->Text == 0)
  {
    options = options | GL2PS_NO_TEXT;
  }
  if (this->Landscape == 1)
  {
    options = options | GL2PS_LANDSCAPE;
  }
  if (this->PS3Shading == 0)
  {
    options = options | GL2PS_NO_PS3_SHADING;
  }
  if (this->OcclusionCull == 1)
  {
    options = options | GL2PS_OCCLUSION_CULL;
  }
  return static_cast<int>(options);
}

int vtkGL2PSExporter::GetGL2PSSort()
{
  switch (this->Sort)
  {
    default:
      vtkDebugMacro(<<"Invalid sort settings, using NO_SORT.");
      VTK_FALLTHROUGH;
    case NO_SORT:
      return GL2PS_NO_SORT;
    case SIMPLE_SORT:
      return GL2PS_SIMPLE_SORT;
    case BSP_SORT:
      return GL2PS_BSP_SORT;
  }
}

int vtkGL2PSExporter::GetGL2PSFormat()
{
  switch (this->FileFormat)
  {
    default:
      vtkDebugMacro(<<"Invalid output format. Using postscript.");
      VTK_FALLTHROUGH;
    case PS_FILE:
      return GL2PS_PS;
    case EPS_FILE:
      return GL2PS_EPS;
    case PDF_FILE:
      return GL2PS_PDF;
    case TEX_FILE:
      return GL2PS_TEX;
    case SVG_FILE:
      return GL2PS_SVG;
  }
}

const char *vtkGL2PSExporter::GetFileExtension()
{
  switch (this->FileFormat)
  {
    default:
      vtkDebugMacro(<<"Invalid output format. Using postscript.");
      VTK_FALLTHROUGH;
    case PS_FILE:
      return "ps";
    case EPS_FILE:
      return "eps";
    case PDF_FILE:
      return "pdf";
    case TEX_FILE:
      return "tex";
    case SVG_FILE:
      return "svg";
  }
}

void vtkGL2PSExporter::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "FileFormat: "
     << this->GetFileFormatAsString() << "\n";
  os << indent << "Sort: "
     << this->GetSortAsString() << "\n";
  os << indent << "Compress: "
     << (this->Compress ? "On\n" : "Off\n");
  os << indent << "DrawBackground: "
     << (this->DrawBackground ? "On\n" : "Off\n");
  os << indent << "SimpleLineOffset: "
     << (this->SimpleLineOffset ? "On\n" : "Off\n");
  os << indent << "Silent: "
     << (this->Silent ? "On\n" : "Off\n");
  os << indent << "BestRoot: "
     << (this->BestRoot ? "On\n" : "Off\n");
  os << indent << "Text: "
     << (this->Text ? "On\n" : "Off\n");
  os << indent << "Landscape: "
     << (this->Landscape ? "On\n" : "Off\n");
  os << indent << "PS3Shading: "
     << (this->PS3Shading ? "On\n" : "Off\n");
  os << indent << "OcclusionCull: "
     << (this->OcclusionCull ? "On\n" : "Off\n");
  os << indent << "Write3DPropsAsRasterImage: "
     << (this->Write3DPropsAsRasterImage ? "On\n" : "Off\n");
  if (this->RasterExclusions)
  {
    os << indent << "RasterExclusions:\n";
    this->RasterExclusions->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "RasterExclusions: (null)\n";
  }
}
