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

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkActorCollection.h"
#include "vtkActor2DCollection.h"
#include "vtkCamera.h"
#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkCoordinate.h"
#include "vtkFreeTypeTools.h"
#include "vtkGL2PSContextDevice2D.h"
#include "vtkGL2PSUtilities.h"
#include "vtkIntArray.h"
#include "vtkMapper2D.h"
#include "vtkMathTextActor.h"
#include "vtkMathTextActor3D.h"
#include "vtkMathTextUtilities.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkProp.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkStdString.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkVolume.h"
#include "vtkVolumeCollection.h"
#include "vtk_gl2ps.h"

#include <vector>

vtkStandardNewMacro(vtkGL2PSExporter)
vtkCxxSetObjectMacro(vtkGL2PSExporter, RasterExclusions, vtkProp3DCollection)

vtkGL2PSExporter::vtkGL2PSExporter()
{
  this->RasterExclusions = NULL;
  this->FilePrefix = NULL;
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
  this->WriteTimeStamp = 1;
  this->PixelData = NULL;
}

vtkGL2PSExporter::~vtkGL2PSExporter()
{
  this->SetRasterExclusions(NULL);
  if ( this->FilePrefix )
    {
    delete [] this->FilePrefix;
    }
  if ( this->Title )
    {
    delete [] this->Title;
    }
  if (this->PixelData)
    {
    delete [] this->PixelData;
    }
}

void vtkGL2PSExporter::WriteData()
{
  // make sure the user specified a file prefix
  if (this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify a file prefix to use");
    return;
    }

  // Get the renderers. We'll be walking through them a lot later.
  vtkRendererCollection *renCol = this->RenderWindow->GetRenderers();

  // Grab props that need special handling for vector output
  vtkNew<vtkPropCollection> contextActorCol;
  this->GetVisibleContextActors(contextActorCol.GetPointer(), renCol);
  vtkNew<vtkCollection> specialPropCol;
  this->RenderWindow->CaptureGL2PSSpecialProps(specialPropCol.GetPointer());

  // Setup information that GL2PS will need to export the scene:
  GLint options = static_cast<GLint>(this->GetGL2PSOptions());
  GLint sort = static_cast<GLint>(this->GetGL2PSSort());
  GLint format = static_cast<GLint>(this->GetGL2PSFormat());
  int *winsize = this->RenderWindow->GetSize();
  GLint viewport[4] = {0, 0, static_cast<GLint>(winsize[0]),
                       static_cast<GLint>(winsize[1])};

  // Create the file.
  char *fName = new char [strlen(this->FilePrefix) + 5];
  sprintf(fName, "%s.%s", this->FilePrefix, this->GetFileExtension());
  FILE *fpObj = fopen(fName, "wb");
  if (!fpObj)
    {
    vtkErrorMacro(<< "unable to open file: " << fName);
    return;
    }

  // Store the "properly" rendered image's pixel data for special actors that
  // need to copy bitmaps into the output (e.g. paraview's scalar bar actor)
  this->PixelDataSize[0] = viewport[2];
  this->PixelDataSize[1] = viewport[3];
  this->RenderWindow->Render();
  delete this->PixelData;
  this->PixelData =
      new float[this->PixelDataSize[0] * this->PixelDataSize[1] * 3];
  glReadPixels(0, 0, this->PixelDataSize[0], this->PixelDataSize[1], GL_RGB,
               GL_FLOAT, this->PixelData);

  // Turn off special props -- these will be handled separately later.
  vtkPropCollection *propCol;
  for (specialPropCol->InitTraversal();
       (propCol = vtkPropCollection::SafeDownCast(
          specialPropCol->GetNextItemAsObject()));)
    {
    this->SetPropVisibilities(propCol, 0);
    }
  this->SetPropVisibilities(contextActorCol.GetPointer(), 0);

  // Write out a raster image without the 2d actors before switching to feedback
  // mode
  float *rasterImage = NULL;
  // Store visibility of actors/volumes if rasterizing.
  vtkNew<vtkIntArray> volVis;
  vtkNew<vtkIntArray> actVis;
  vtkNew<vtkIntArray> act2dVis;
  if (this->Write3DPropsAsRasterImage)
    {
    vtkDebugMacro(<<"Rasterizing 3D geometry.")
    this->SavePropVisibility(renCol, volVis.GetPointer(), actVis.GetPointer(),
                             act2dVis.GetPointer());
    this->Turn2DPropsOff(renCol);

    int numpix= winsize[0]*winsize[1]*3;
    rasterImage = new float [numpix];
    int offscreen = this->RenderWindow->GetOffScreenRendering();

    this->RenderWindow->OffScreenRenderingOn();
    this->RenderWindow->Render();
    unsigned char *charpixels = this->RenderWindow->GetPixelData(
          0, 0, winsize[0] - 1, winsize[1] - 1, 1);

    for (int i=0; i<numpix; i++)
      {
      rasterImage[i] = (static_cast<float>(charpixels[i])/255.0);
      }
    delete [] charpixels;
    this->RenderWindow->SetOffScreenRendering(offscreen);
    // Render after switching to/from offscreen render but before initializing
    // gl2ps, otherwise the renderwindow will switch gl contexts and switch out
    // of feedback mode.
    this->RenderWindow->Render();
    }

  // Disable depth peeling. It uses textures that turn into large opaque quads
  // in the output, and gl2ps sorts primitives itself anyway.
  vtkRenderer *ren;
  std::vector<bool> origDepthPeeling;
  origDepthPeeling.reserve(renCol->GetNumberOfItems());
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    origDepthPeeling.push_back(ren->GetUseDepthPeeling() != 0);
    ren->UseDepthPeelingOff();
    }

  vtkDebugMacro(<<"Writing file using GL2PS");

  // Call gl2ps to generate the file.
  int buffsize = 0;
  int state = GL2PS_OVERFLOW;
  while(state == GL2PS_OVERFLOW)
    {
    buffsize += 2048*2048;
    gl2psBeginPage(this->Title ? this->Title : "VTK GL2PS Export", "VTK",
                   viewport, format, sort, options, GL_RGBA, 0,
                   NULL, 0, 0, 0, buffsize, fpObj, fName);

    if (!this->WriteTimeStamp)
      gl2psDisable(GL2PS_TIMESTAMP);

    // Render non-specialized geometry by either passing in the raster image or
    // rendering into the feedback buffer.
    if (this->Write3DPropsAsRasterImage)
      {
      // Dump the rendered image without 2d actors as a raster image.
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      glRasterPos3f(-1.0, -1.0, 1.0);
      gl2psDrawPixels(winsize[0], winsize[1], 0, 0, GL_RGB,
                      GL_FLOAT, rasterImage);
      glPopMatrix();

      // Render the 2d actors alone in a vector graphic format.
      this->RestorePropVisibility(renCol, volVis.GetPointer(),
                                  actVis.GetPointer(), act2dVis.GetPointer());
      this->Turn3DPropsOff(renCol);
      this->RenderWindow->Render();
      }
    else
      {
      this->RenderWindow->Render();
      }

    // Render props that require special handling (text, etc)
    this->DrawSpecialProps(specialPropCol.GetPointer(), renCol);

    // Render context 2D stuff
    this->DrawContextActors(contextActorCol.GetPointer(), renCol);

    state = gl2psEndPage();
    }
  fclose(fpObj);

  // Clean up:
  // Re-enable depth peeling if needed
  for (int i = 0; i < static_cast<int>(origDepthPeeling.size()); ++i)
    {
    vtkRenderer::SafeDownCast(renCol->GetItemAsObject(i))->SetUseDepthPeeling(
          origDepthPeeling[i] ? 1 : 0);
    }
  if (this->Write3DPropsAsRasterImage)
    {
    // Reset the visibility.
    this->RestorePropVisibility(renCol, volVis.GetPointer(),
                                actVis.GetPointer(), act2dVis.GetPointer());
    // free memory
    delete [] rasterImage;
    }
  // Turn the special props back on
  for (specialPropCol->InitTraversal();
       (propCol = vtkPropCollection::SafeDownCast(
          specialPropCol->GetNextItemAsObject()));)
    {
    this->SetPropVisibilities(propCol, 1);
    }
  // Turn context actors back on
  this->SetPropVisibilities(contextActorCol.GetPointer(), 1);
  // Re-render the scene to show all actors.
  this->RenderWindow->Render();
  delete[] fName;
  delete[] this->PixelData;
  this->PixelData = NULL;

  vtkDebugMacro(<<"Finished writing file using GL2PS");
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

void vtkGL2PSExporter::SavePropVisibility(vtkRendererCollection *renCol,
                                          vtkIntArray *volVis,
                                          vtkIntArray *actVis,
                                          vtkIntArray *act2dVis)
{
  int nRen = renCol->GetNumberOfItems();
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkActor2DCollection *a2Col;
  vtkVolume *vol;
  vtkActor *act;
  vtkActor2D *act2d;
  int j;

  volVis->SetNumberOfComponents(nRen);
  actVis->SetNumberOfComponents(nRen);
  act2dVis->SetNumberOfComponents(nRen);

  renCol->InitTraversal();
  for (int i=0; i<nRen; ++i)
    {
    ren = renCol->GetNextItem();
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
    a2Col = ren->GetActors2D();

    volVis->SetNumberOfTuples(vCol->GetNumberOfItems());
    for (vCol->InitTraversal(), j=0; (vol = vCol->GetNextVolume()); ++j)
      {
      volVis->SetComponent(i, j, vol->GetVisibility());
      }

    actVis->SetNumberOfTuples(aCol->GetNumberOfItems());
    for (aCol->InitTraversal(), j=0; (act = aCol->GetNextActor()); ++j)
      {
      actVis->SetComponent(i, j, act->GetVisibility());
      }

    act2dVis->SetNumberOfTuples(a2Col->GetNumberOfItems());
    for (a2Col->InitTraversal(), j=0; (act2d = a2Col->GetNextActor2D());
         ++j)
      {
      act2dVis->SetComponent(i, j, act2d->GetVisibility());
      }
    }
}

void vtkGL2PSExporter::RestorePropVisibility(vtkRendererCollection *renCol,
                                             vtkIntArray *volVis,
                                             vtkIntArray *actVis,
                                             vtkIntArray *act2dVis)
{
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkActor2DCollection *a2Col;
  vtkVolume *vol;
  vtkActor *act;
  vtkActor2D *act2d;
  int j;
  int nRen = renCol->GetNumberOfItems();

  renCol->InitTraversal();
  for (int i=0; i<nRen; ++i)
    {
    ren = renCol->GetNextItem();
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
    a2Col = ren->GetActors2D();

    for (vCol->InitTraversal(), j=0; (vol = vCol->GetNextVolume()); ++j)
      {
      vol->SetVisibility(static_cast<int>(volVis->GetComponent(i, j)));
      }

    for (aCol->InitTraversal(), j=0; (act = aCol->GetNextActor()); ++j)
      {
      act->SetVisibility(static_cast<int>(actVis->GetComponent(i, j)));
      }

    for (a2Col->InitTraversal(), j=0; (act2d = a2Col->GetNextActor2D());
         ++j)
      {
      act2d->SetVisibility(static_cast<int>(act2dVis->GetComponent(i, j)));
      }
    }
}

void vtkGL2PSExporter::Turn3DPropsOff(vtkRendererCollection *renCol)
{
  vtkRenderer *ren;
  vtkVolumeCollection *vCol;
  vtkActorCollection *aCol;
  vtkVolume *vol;
  vtkActor *act;

  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    vCol = ren->GetVolumes();
    aCol = ren->GetActors();
    for (vCol->InitTraversal(); (vol = vCol->GetNextVolume());)
      {
      if (!this->Write3DPropsAsRasterImage ||
          !this->RasterExclusions ||
          !this->RasterExclusions->IsItemPresent(vol))
        {
        vol->SetVisibility(0);
        }
      }

    for (aCol->InitTraversal(); (act = aCol->GetNextActor());)
      {
      if (!this->Write3DPropsAsRasterImage ||
          !this->RasterExclusions ||
          !this->RasterExclusions->IsItemPresent(act))
        {
        act->SetVisibility(0);
        }
      }
    }
}

void vtkGL2PSExporter::Turn2DPropsOff(vtkRendererCollection *renCol)
{
  vtkRenderer *ren;
  vtkActor2DCollection *a2Col;
  vtkActor2D *act2d;

  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    a2Col = ren->GetActors2D();
    for (a2Col->InitTraversal(); (act2d = a2Col->GetNextActor2D());)
      {
      act2d->SetVisibility(0);
      }
    }
  if (this->Write3DPropsAsRasterImage && this->RasterExclusions)
    {
    vtkProp3D *prop;
    for (this->RasterExclusions->InitTraversal();
         (prop = this->RasterExclusions->GetNextProp3D());)
      {
      prop->SetVisibility(0);
      }
    }
}

void vtkGL2PSExporter::GetVisibleContextActors(vtkPropCollection *result,
                                               vtkRendererCollection *renCol)
{
  assert("valid pointers" && result && renCol);
  result->RemoveAllItems();
  vtkRenderer *ren;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    vtkCollection *pCol = ren->GetViewProps();
    vtkContextActor *act;
    for (pCol->InitTraversal();
         (act = vtkContextActor::SafeDownCast(pCol->GetNextItemAsObject()));)
      {
      if (!act || !act->GetVisibility())
        {
        continue;
        }
      if (!result->IsItemPresent(act))
        {
        result->AddItem(act);
        }
      }
    }
}

void vtkGL2PSExporter::SetPropVisibilities(vtkPropCollection *col, int vis)
{
  vtkProp *prop;
  for (col->InitTraversal();
       (prop = vtkProp::SafeDownCast(col->GetNextItemAsObject()));)
    {
    prop->SetVisibility(vis);
  }
}

void vtkGL2PSExporter::DrawSpecialProps(vtkCollection *specialPropCol,
                                        vtkRendererCollection *renCol)
{
  // Iterate through the renderers and the prop collections together:
  assert("renderers and special prop collections match" &&
         renCol->GetNumberOfItems() == specialPropCol->GetNumberOfItems());
  for (int i = 0; i < renCol->GetNumberOfItems(); ++i)
    {
    vtkPropCollection *propCol = vtkPropCollection::SafeDownCast(
          specialPropCol->GetItemAsObject(i));
    vtkRenderer *ren = vtkRenderer::SafeDownCast(renCol->GetItemAsObject(i));

    // Setup the GL matrices for this renderer. This pushes the MV matrix,
    // (and resets w/o pushing proj?) so we'll need to pop it later. We push the
    // projection matrix ourselves.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    ren->GetActiveCamera()->Render(ren);

    // Draw special props
    vtkProp *prop = 0;
    for (propCol->InitTraversal(); (prop = propCol->GetNextProp());)
      {
      // What sort of special prop is it?
      if (vtkActor2D *act2d = vtkActor2D::SafeDownCast(prop))
        {
        if (vtkTextActor *textAct = vtkTextActor::SafeDownCast(act2d))
          {
          this->DrawTextActor(textAct, ren);
          }
        else if (vtkMathTextActor *mathTextAct =
                 vtkMathTextActor::SafeDownCast(act2d))
          {
          this->DrawMathTextActor(mathTextAct, ren);
          }
        else if (vtkMapper2D *map2d = act2d->GetMapper())
          {
          if (vtkTextMapper *textMap = vtkTextMapper::SafeDownCast(map2d))
            {
            this->DrawTextMapper(textMap, act2d, ren);
            }
          else // Some other mapper2D
            {
            continue;
            }
          }
        else // Some other actor2D
          {
          continue;
          }
        }
      else if (vtkMathTextActor3D *mathTextAct3D =
               vtkMathTextActor3D::SafeDownCast(prop))
        {
        this->DrawMathTextActor3D(mathTextAct3D, ren);
        }
      else if (vtkTextActor3D *textAct3D =
               vtkTextActor3D::SafeDownCast(prop))
        {
        this->DrawTextActor3D(textAct3D, ren);
        }
      else // Some other prop
        {
        continue;
        }
      }
    // Pop MV matrices (This was pushed by vtkOpenGLCamera::Render earlier).
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }
}

void vtkGL2PSExporter::DrawTextActor(vtkTextActor *textAct, vtkRenderer *ren)
{
  const char *string = textAct->GetInput();
  vtkCoordinate *coord = textAct->GetActualPositionCoordinate();
  vtkTextProperty *tprop = textAct->GetTextProperty();

  this->DrawViewportTextOverlay(string, tprop, coord, ren);
}

void vtkGL2PSExporter::DrawTextActor3D(vtkTextActor3D *textAct,
                                       vtkRenderer *)
{
  // Get path
  const char *string = textAct->GetInput();
  vtkNew<vtkPath> path;
  vtkNew<vtkTextProperty> tprop;
  tprop->ShallowCopy(textAct->GetTextProperty());
  tprop->SetJustificationToLeft(); // Ignored by textactor3d
  tprop->SetVerticalJustificationToBottom(); // Ignored by textactor3d
  vtkFreeTypeTools::GetInstance()->StringToPath(tprop.GetPointer(),
                                                vtkStdString(string),
                                                path.GetPointer());

  // Get actor info
  vtkMatrix4x4 *actorMatrix = textAct->GetMatrix();
  double *actorBounds = textAct->GetBounds();
  double *dcolor = tprop->GetColor();
  unsigned char actorColor[3] = {static_cast<unsigned char>(dcolor[0]*255),
                                 static_cast<unsigned char>(dcolor[1]*255),
                                 static_cast<unsigned char>(dcolor[2]*255)};

  this->Draw3DPath(path.GetPointer(), actorMatrix, actorBounds, actorColor);
}

void vtkGL2PSExporter::DrawTextMapper(vtkTextMapper *textMap,
                                      vtkActor2D *textAct,
                                      vtkRenderer *ren)
{
  const char *string = textMap->GetInput();
  vtkCoordinate *coord = textAct->GetActualPositionCoordinate();
  vtkTextProperty *tprop = textMap->GetTextProperty();

  this->DrawViewportTextOverlay(string, tprop, coord, ren);
}

void vtkGL2PSExporter::DrawMathTextActor(vtkMathTextActor *textAct,
                                         vtkRenderer *ren)
{
  const char *string = textAct->GetInput();
  vtkCoordinate *coord = textAct->GetActualPositionCoordinate();
  vtkTextProperty *tprop = textAct->GetTextProperty();
  int *winsize = this->RenderWindow->GetSize();

  vtkNew<vtkPath> path;
  double scale[2] = {1.0,1.0};
  vtkMathTextUtilities::GetInstance()->StringToPath(string,
                                                    path.GetPointer(),
                                                    tprop);
  double *dcolor = tprop->GetColor();
  unsigned char color[3] = {static_cast<unsigned char>(dcolor[0]*255),
                            static_cast<unsigned char>(dcolor[1]*255),
                            static_cast<unsigned char>(dcolor[2]*255)};
  double winsized[2] = {static_cast<double>(winsize[0]),
                        static_cast<double>(winsize[1])};

  double *actorBounds = textAct->GetBounds();
  double rasterPos[3] = {actorBounds[1] - actorBounds[0],
                         actorBounds[3] - actorBounds[2],
                         actorBounds[5] - actorBounds[4]};

  int *textPos = coord->GetComputedDisplayValue(ren);
  double textPosd[2] = {static_cast<double>(textPos[0]),
                        static_cast<double>(textPos[1])};
  vtkGL2PSUtilities::DrawPath(path.GetPointer(), rasterPos, winsized,
                              textPosd, scale, tprop->GetOrientation(),
                              color);
}

void vtkGL2PSExporter::DrawMathTextActor3D(vtkMathTextActor3D *textAct,
                                           vtkRenderer *)
{
  // Get path
  vtkNew<vtkPath> path;
  vtkNew<vtkTextProperty> tprop;
  const char *string = textAct->GetInput();
  tprop->ShallowCopy(textAct->GetTextProperty());
  tprop->SetOrientation(0); // Ignored in mathtextactor3d
  tprop->SetJustificationToLeft(); // Ignored in mathtextactor3d
  tprop->SetVerticalJustificationToBottom(); // Ignored in mathtextactor3d

  vtkMathTextUtilities::GetInstance()->StringToPath(string,
                                                    path.GetPointer(),
                                                    tprop.GetPointer());

  // Get actor info
  vtkMatrix4x4 *actorMatrix = textAct->GetMatrix();
  double *actorBounds = textAct->GetBounds();
  double *dcolor = tprop->GetColor();
  unsigned char actorColor[3] = {static_cast<unsigned char>(dcolor[0]*255),
                                 static_cast<unsigned char>(dcolor[1]*255),
                                 static_cast<unsigned char>(dcolor[2]*255)};

  this->Draw3DPath(path.GetPointer(), actorMatrix, actorBounds, actorColor);
}

void vtkGL2PSExporter::DrawViewportTextOverlay(const char *string,
                                               vtkTextProperty *tprop,
                                               vtkCoordinate *coord,
                                               vtkRenderer *ren)
{
  // Figure out the viewport information
  int *winsize = this->RenderWindow->GetSize();
  double *viewport = ren->GetViewport();
  int viewportPixels[4] = {static_cast<int>(viewport[0] * winsize[0]),
                           static_cast<int>(viewport[1] * winsize[1]),
                           static_cast<int>(viewport[2] * winsize[0]),
                           static_cast<int>(viewport[3] * winsize[1])};
  int viewportSpread[2] = {viewportPixels[2] - viewportPixels[0],
                           viewportPixels[3] - viewportPixels[1]};

  // Get viewport coord
  double *textPos2 = coord->GetComputedDoubleViewportValue(ren);

  // convert to NDC with z on the near plane
  double textPos[3];
  textPos[0] = (2. * textPos2[0] / static_cast<double>(viewportSpread[0])) - 1.;
  textPos[1] = (2. * textPos2[1] / static_cast<double>(viewportSpread[1])) - 1.;
  textPos[2] = -1.;

  // Setup the GL state to render into the viewport
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glViewport(viewportPixels[0], viewportPixels[1],
             viewportSpread[0], viewportSpread[1]);

  vtkGL2PSUtilities::DrawString(string, tprop, textPos);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}

void vtkGL2PSExporter::Draw3DPath(vtkPath *path, vtkMatrix4x4 *actorMatrix,
                                  double actorBounds[4],
                                  unsigned char actorColor[3])
{
  double scale[2] = {1.0, 1.0};
  double translation[2] = {0.0, 0.0};
  double rasterPos[3] = {(actorBounds[1] + actorBounds[0]) * 0.5,
                         (actorBounds[3] + actorBounds[2]) * 0.5,
                         (actorBounds[5] + actorBounds[4]) * 0.5};
  int *winsize = this->RenderWindow->GetSize();
  double winsized[2] = {static_cast<double>(winsize[0]),
                        static_cast<double>(winsize[1])};

  // Build transformation matrix
  double glMatrix[16];
  glGetDoublev(GL_PROJECTION_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> projectionMatrix;
  projectionMatrix->DeepCopy(glMatrix);
  projectionMatrix->Transpose();

  glGetDoublev(GL_MODELVIEW_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> modelviewMatrix;
  modelviewMatrix->DeepCopy(glMatrix);
  modelviewMatrix->Transpose();

  vtkNew<vtkMatrix4x4> transformMatrix;
  vtkMatrix4x4::Multiply4x4(projectionMatrix.GetPointer(),
                            modelviewMatrix.GetPointer(),
                            transformMatrix.GetPointer());
  vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                            actorMatrix,
                            transformMatrix.GetPointer());

  double viewport[4];
  glGetDoublev(GL_VIEWPORT, viewport);
  double depthRange[2];
  glGetDoublev(GL_DEPTH_RANGE, depthRange);

  const double halfWidth = viewport[2] * 0.5;
  const double halfHeight = viewport[3] * 0.5;
  const double zFactor1 = (depthRange[1] - depthRange[0]) * 0.5;
  const double zFactor2 = (depthRange[1] + depthRange[0]) * 0.5;

  vtkSmartPointer<vtkPoints> origPoints = path->GetPoints();
  vtkNew<vtkPoints> newPoints;
  newPoints->DeepCopy(origPoints);
  double point[4];
  for (vtkIdType i = 0; i < path->GetNumberOfPoints(); ++i)
    {
    newPoints->GetPoint(i, point);
    point[3] = 1.0;
    // Convert world to clip coordinates:
    // <out point> = [projection] [modelview] [actor matrix] <in point>
    transformMatrix->MultiplyPoint(point, point);
    // Clip to NDC
    const double invW = 1.0 / point[3];
    point[0] *= invW;
    point[1] *= invW;
    point[2] *= invW;
    // NDC to device:
    point[0] = point[0] * halfWidth + viewport[0] + halfWidth;
    point[1] = point[1] * halfHeight + viewport[1] + halfHeight;
    point[2] = point[2] * zFactor1 + zFactor2;
    newPoints->SetPoint(i, point);
    }

  path->SetPoints(newPoints.GetPointer());

  vtkGL2PSUtilities::DrawPath(path, rasterPos, winsized, translation, scale,
                              0.0, actorColor);
}

void vtkGL2PSExporter::CopyPixels(int copyRect[4], vtkRenderer *ren)
{
  // Figure out the viewport information
  int *winsize = this->RenderWindow->GetSize();
  double *viewport = ren->GetViewport();
  int viewportPixels[4] = {static_cast<int>(viewport[0] * winsize[0]),
                           static_cast<int>(viewport[1] * winsize[1]),
                           static_cast<int>(viewport[2] * winsize[0]),
                           static_cast<int>(viewport[3] * winsize[1])};
  int viewportSpread[2] = {viewportPixels[2] - viewportPixels[0],
                           viewportPixels[3] - viewportPixels[1]};

  // convert to NDC with z on the near plane
  double pos[3];
  pos[0] = (2. * copyRect[0] / static_cast<double>(viewportSpread[0])) - 1.;
  pos[1] = (2. * copyRect[1] / static_cast<double>(viewportSpread[1])) - 1.;
  pos[2] = -1;

  // Setup the GL state to render into the viewport
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glViewport(viewportPixels[0], viewportPixels[1],
             viewportSpread[0], viewportSpread[1]);

  // Copy the relevant rectangle of pixel data memory into a new array.
  float *dest = new float[copyRect[2] * copyRect[3] * 3];
  int destWidth = copyRect[2] * 3;
  int destWidthBytes = destWidth * sizeof(float);
  int sourceWidth = this->PixelDataSize[0] * 3;
  int sourceOffset = copyRect[0] * 3;

  for (int row = 0;
       row < copyRect[3] && // Copy until the top of the copyRect is reached,
       row + copyRect[1] < this->PixelDataSize[1]; // or we exceed the cache
       ++row)
    {
    memcpy(dest + (row * destWidth),
           this->PixelData + ((copyRect[1] + row) * sourceWidth) + sourceOffset,
           destWidthBytes);
    }

  // Inject the copied pixel buffer into gl2ps
  glRasterPos3dv(pos);
  gl2psDrawPixels(copyRect[2], copyRect[3], 0, 0, GL_RGB, GL_FLOAT, dest);

  delete [] dest;

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}

void vtkGL2PSExporter::DrawContextActors(vtkPropCollection *contextActs,
                                         vtkRendererCollection *renCol)
{
  if (contextActs->GetNumberOfItems() != 0)
    {
    vtkNew<vtkContext2D> context;
    vtkNew<vtkGL2PSContextDevice2D> gl2psDevice;

    // Render ContextActors. Iterate through all actors again instead of using
    // the collected actors (contextActs), since we need to know which
    // actors belong to which renderers.
    vtkRenderer *ren;
    for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
      {
      gl2psDevice->Begin(ren);
      context->Begin(gl2psDevice.GetPointer());

      vtkContextActor *act;
      vtkCollection *pCol = ren->GetViewProps();
      for (pCol->InitTraversal();
           (act = vtkContextActor::SafeDownCast(pCol->GetNextItemAsObject()));)
        {
        if (act)
          {
          act->SetVisibility(1);
          act->GetScene()->SetGeometry(ren->GetSize());
          act->GetScene()->Paint(context.GetPointer());
          act->SetVisibility(0);
          }
        }

      context->End();
      gl2psDevice->End();
      }
    }

}
