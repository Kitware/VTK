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
}

void vtkGL2PSExporter::WriteData()
{
  FILE *fpObj;
  char *fName;
  GLint format = GL2PS_EPS;
  GLint sort;
  GLint options = GL2PS_NONE;
  int buffsize = 0;
  int state = GL2PS_OVERFLOW;
  GLint viewport[4];
  int *winsize = this->RenderWindow->GetSize();

  vtkRendererCollection *renCol = this->RenderWindow->GetRenderers();
  vtkPropCollection *specialPropCol =
      this->RenderWindow->CaptureGL2PSSpecialProps();
  vtkPropCollection *contextActorCol = this->GetVisibleContextActors(renCol);
  // Stores visibility of actors/volumes.
  vtkIntArray *volVis = vtkIntArray::New();
  vtkIntArray *actVis = vtkIntArray::New();
  vtkIntArray *act2dVis = vtkIntArray::New();
  float *floatpixels = 0; // for writing 3d props as an image.

  // Setting this to the entire window size for now.
  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = winsize[0];
  viewport[3] = winsize[1];

  // make sure the user specified a file prefix
  if (this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify a file prefix to use");
    return;
    }

  // Set the options based on user's choices.
  if (this->Sort == NO_SORT)
    {
    sort = GL2PS_NO_SORT;
    }
  else if (this->Sort == SIMPLE_SORT)
    {
    sort = GL2PS_SIMPLE_SORT;
    }
  else
    {
    sort = GL2PS_BSP_SORT;
    }

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

  // Setup the file.
  fName = new char [strlen(this->FilePrefix) + 5] ;
  if (this->FileFormat == PS_FILE)
    {
    sprintf(fName, "%s.ps", this->FilePrefix);
    format = GL2PS_PS;
    }
  else if (this->FileFormat == EPS_FILE)
    {
    sprintf(fName, "%s.eps", this->FilePrefix);
    format = GL2PS_EPS;
    }
  else if (this->FileFormat == PDF_FILE)
    {
    sprintf(fName, "%s.pdf", this->FilePrefix);
    format = GL2PS_PDF;
    }
  else if (this->FileFormat == TEX_FILE)
    {
    sprintf(fName, "%s.tex", this->FilePrefix);
    format = GL2PS_TEX;
    }
  else if (this->FileFormat == SVG_FILE)
    {
    sprintf(fName, "%s.svg", this->FilePrefix);
    format = GL2PS_SVG;
    }

  fpObj = fopen(fName, "wb");
  if (!fpObj)
    {
    vtkErrorMacro(<< "unable to open file: " << fName);
    return;
    }

  // Turn off text actors -- these will be handled separately later.
  this->SetPropVisibilities(specialPropCol, 0);
  this->SetPropVisibilities(contextActorCol, 0);

  // Write out a raster image without the 2d actors before switching to feedback
  // mode
  if (this->Write3DPropsAsRasterImage)
    {
    this->SavePropVisibility(renCol, volVis, actVis, act2dVis);
    this->Turn2DPropsOff(renCol);

    int numpix= winsize[0]*winsize[1]*3;
    floatpixels = new float [numpix];
    unsigned char *charpixels;
    int offscreen = this->RenderWindow->GetOffScreenRendering();

    this->RenderWindow->OffScreenRenderingOn();
    this->RenderWindow->Render();
    charpixels = this->RenderWindow->GetPixelData(0, 0, winsize[0] - 1,
                                                  winsize[1] - 1, 1);

    for (int i=0; i<numpix; i++)
      {
      floatpixels[i] = (static_cast<float>(charpixels[i])/255.0);
      }
    delete [] charpixels;
    this->RenderWindow->SetOffScreenRendering(offscreen);
    // Render after switching to/from offscreen render but before initializing
    // gl2ps, otherwise the renderwindow will switch gl contexts and switch out
    // of feedback mode.
    this->RenderWindow->Render();
    }

  // Writing the file using GL2PS.
  vtkDebugMacro(<<"Writing file using GL2PS");

  // Call gl2ps to generate the file.
  while(state == GL2PS_OVERFLOW)
    {
    buffsize += 2048*2048;
    gl2psBeginPage(this->Title ? this->Title : "VTK GL2PS Export", "VTK",
                   viewport, format, sort, options, GL_RGBA, 0,
                   NULL, 0, 0, 0, buffsize, fpObj, fName);

    if (!this->WriteTimeStamp)
      gl2psDisable(GL2PS_TIMESTAMP);

    if (this->Write3DPropsAsRasterImage)
      {
      // Dump the rendered image without 2d actors as a raster image.
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      glRasterPos3f(-1.0, -1.0, 1.0);
      gl2psDrawPixels(winsize[0], winsize[1], 0, 0, GL_RGB,
                      GL_FLOAT, floatpixels);
      glPopMatrix();
      // Render the 2d actors alone in a vector graphic format.
      this->RestorePropVisibility(renCol, volVis, actVis, act2dVis);
      this->Turn3DPropsOff(renCol);
      this->RenderWindow->Render();
      }
    else
      {
      this->RenderWindow->Render();
      }

    // Add the strings from the text actors individually
    vtkProp *prop = 0;
    for (specialPropCol->InitTraversal();
         (prop = specialPropCol->GetNextProp());)
      {
      // What sort of special prop is it?
      if (vtkActor2D *act2d = vtkActor2D::SafeDownCast(prop))
        {
        if (vtkTextActor *textAct = vtkTextActor::SafeDownCast(act2d))
          {
          this->DrawTextActor(textAct, renCol);
          }
        else if (vtkMathTextActor *textAct =
                 vtkMathTextActor::SafeDownCast(act2d))
          {
          this->DrawMathTextActor(textAct, renCol);
          }
        else if (vtkMapper2D *map2d = act2d->GetMapper())
          {
          if (vtkTextMapper *textMap = vtkTextMapper::SafeDownCast(map2d))
            {
            this->DrawTextMapper(textMap, act2d, renCol);
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
      else if (vtkMathTextActor3D *textAct =
               vtkMathTextActor3D::SafeDownCast(prop))
        {
        this->DrawMathTextActor3D(textAct, renCol);
        }
      else if (vtkTextActor3D *textAct =
               vtkTextActor3D::SafeDownCast(prop))
        {
        this->DrawTextActor3D(textAct, renCol);
        }
      else // Some other prop
        {
        continue;
        }
      }

    // Render ContextActors. Iterate through all actors again instead of using
    // the collected actors (contextActorCol), since we need to know which
    // actors belong to which renderers.
    vtkRenderer *ren;
    vtkNew<vtkContext2D> context;
    vtkNew<vtkGL2PSContextDevice2D> gl2psDevice;
    for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
      {
      vtkCollection *pCol = ren->GetViewProps();
      vtkContextActor *act;
      gl2psDevice->Begin(ren);
      for (pCol->InitTraversal();
           (act = vtkContextActor::SafeDownCast(pCol->GetNextItemAsObject()));)
        {
        if (act)
          {
          context->Begin(gl2psDevice.GetPointer());
          act->SetVisibility(1);
          act->GetScene()->SetGeometry(ren->GetSize());
          act->GetScene()->Paint(context.GetPointer());
          act->SetVisibility(0);
          context->End();
          }
        }
      gl2psDevice->End();
      }

    state = gl2psEndPage();
    }
  fclose(fpObj);

  // Clean up.
  if (this->Write3DPropsAsRasterImage)
    {
    // Reset the visibility.
    this->RestorePropVisibility(renCol, volVis, actVis, act2dVis);
    // memory
    delete [] floatpixels;
    // Re-render the scene to show all actors.
    this->RenderWindow->Render();
    }
  this->SetPropVisibilities(specialPropCol, 1);
  this->SetPropVisibilities(contextActorCol, 1);
  delete[] fName;
  volVis->Delete();
  actVis->Delete();
  act2dVis->Delete();
  specialPropCol->Delete();
  contextActorCol->Delete();

  vtkDebugMacro(<<"Finished writing file using GL2PS");
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

vtkPropCollection *
vtkGL2PSExporter::GetVisibleContextActors(vtkRendererCollection *renCol)
{
  vtkPropCollection *result = vtkPropCollection::New();

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
  return result;
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

void vtkGL2PSExporter::DrawTextActor(vtkTextActor *textAct,
                                     vtkRendererCollection *renCol)
{
  const char *string = textAct->GetInput();
  vtkCoordinate *coord = textAct->GetActualPositionCoordinate();
  vtkTextProperty *tprop = textAct->GetTextProperty();
  int *winsize = this->RenderWindow->GetSize();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  // Add a string for each renderer
  vtkRenderer *ren = 0;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    // convert coordinate to normalized display
    GLdouble *textPos = coord->GetComputedDoubleDisplayValue(
          vtkRenderer::SafeDownCast(ren));
    textPos[0] = (2.0 * textPos[0] / winsize[0]) - 1.0;
    textPos[1] = (2.0 * textPos[1] / winsize[1]) - 1.0;
    textPos[2] = -1.0;
    vtkGL2PSUtilities::DrawString(string, tprop, textPos);
    }
  glPopMatrix();
}

void vtkGL2PSExporter::DrawTextActor3D(vtkTextActor3D *textAct,
                                       vtkRendererCollection *renCol)
{
  const char *string = textAct->GetInput();
  vtkNew<vtkTextProperty> tprop;
  int *winsize = this->RenderWindow->GetSize();
  tprop->ShallowCopy(textAct->GetTextProperty());
  tprop->SetJustificationToLeft(); // Ignored by textactor3d
  tprop->SetVerticalJustificationToBottom(); // Ignored by textactor3d

  // Get path
  vtkNew<vtkPath> path;
  vtkFreeTypeTools::GetInstance()->StringToPath(tprop.GetPointer(),
                                                vtkStdString(string),
                                                path.GetPointer());

  // Extract gl transform info
  vtkMatrix4x4 *actorMatrix = textAct->GetMatrix();
  vtkNew<vtkMatrix4x4> projectionMatrix;
  vtkNew<vtkMatrix4x4> modelviewMatrix;
  vtkNew<vtkMatrix4x4> transformMatrix;
  double glMatrix[16];
  double viewport[4];
  double depthRange[2];

  double *textBounds = textAct->GetBounds();
  double rasterPos[3] = {(textBounds[1] + textBounds[0]) * 0.5,
                         (textBounds[3] + textBounds[2]) * 0.5,
                         (textBounds[5] + textBounds[4]) * 0.5};

  double scale[2] = {1.0, 1.0};
  double *dcolor = tprop->GetColor();
  unsigned char color[3] = {static_cast<unsigned char>(dcolor[0]*255),
                            static_cast<unsigned char>(dcolor[1]*255),
                            static_cast<unsigned char>(dcolor[2]*255)};
  double winsized[2] = {static_cast<double>(winsize[0]),
                        static_cast<double>(winsize[1])};

  vtkSmartPointer<vtkPoints> origPoints = path->GetPoints();
  vtkRenderer *ren = 0;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    // Setup gl matrices
    ren->GetActiveCamera()->Render(ren);

    // Build transformation matrix
    glGetDoublev(GL_PROJECTION_MATRIX, glMatrix);
    projectionMatrix->DeepCopy(glMatrix);
    projectionMatrix->Transpose();
    glGetDoublev(GL_MODELVIEW_MATRIX, glMatrix);
    modelviewMatrix->DeepCopy(glMatrix);
    modelviewMatrix->Transpose();
    vtkMatrix4x4::Multiply4x4(projectionMatrix.GetPointer(),
                              modelviewMatrix.GetPointer(),
                              transformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                              actorMatrix,
                              transformMatrix.GetPointer());

    glGetDoublev(GL_VIEWPORT, viewport);
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    const double halfWidth = viewport[2] * 0.5;
    const double halfHeight = viewport[3] * 0.5;
    const double zFactor1 = (depthRange[1] - depthRange[0]) * 0.5;
    const double zFactor2 = (depthRange[1] + depthRange[0]) * 0.5;

    vtkNew<vtkPoints> newPoints;
    newPoints->DeepCopy(origPoints);
    double point[4];
    double translation[2] = {0.0, 0.0};
    for (vtkIdType i = 0; i < path->GetNumberOfPoints(); ++i)
      {
      newPoints->GetPoint(i, point);
      point[3] = 1.0;
      // Convert path to clip coordinates:
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

    vtkGL2PSUtilities::DrawPath(path.GetPointer(), rasterPos, winsized,
                                translation, scale, 0.0, color);
    glMatrixMode(GL_PROJECTION_MATRIX);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW_MATRIX);
    glPopMatrix();
    }
}


void vtkGL2PSExporter::DrawTextMapper(vtkTextMapper *textMap,
                                      vtkActor2D *textAct,
                                      vtkRendererCollection *renCol)
{
  const char *string = textMap->GetInput();
  vtkCoordinate *coord = textAct->GetActualPositionCoordinate();
  vtkTextProperty *tprop = textMap->GetTextProperty();
  int *winsize = this->RenderWindow->GetSize();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  // Add a string for each renderer
  vtkRenderer *ren = 0;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    // convert coordinate to normalized display
    GLdouble *textPos = coord->GetComputedDoubleDisplayValue(
          vtkRenderer::SafeDownCast(ren));
    textPos[0] = (2.0 * textPos[0] / winsize[0]) - 1.0;
    textPos[1] = (2.0 * textPos[1] / winsize[1]) - 1.0;
    textPos[2] = -1.0;
    vtkGL2PSUtilities::DrawString(string, tprop, textPos);
    }
  glPopMatrix();
}

void vtkGL2PSExporter::DrawMathTextActor(vtkMathTextActor *textAct,
                                         vtkRendererCollection *renCol)
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

  // Add a path for each renderer
  vtkRenderer *ren = 0;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    ren->GetActiveCamera()->Render(ren);
    int *textPos = coord->GetComputedDisplayValue(ren);
    double textPosd[2] = {static_cast<double>(textPos[0]),
                          static_cast<double>(textPos[1])};
    vtkGL2PSUtilities::DrawPath(path.GetPointer(), rasterPos, winsized,
                                textPosd, scale, tprop->GetOrientation(),
                                color);
    glMatrixMode(GL_PROJECTION_MATRIX);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW_MATRIX);
    glPopMatrix();
    }
}

void vtkGL2PSExporter::DrawMathTextActor3D(vtkMathTextActor3D *textAct,
                                           vtkRendererCollection *renCol)
{
  const char *string = textAct->GetInput();
  vtkNew<vtkTextProperty> tprop;
  int *winsize = this->RenderWindow->GetSize();
  tprop->ShallowCopy(textAct->GetTextProperty());
  tprop->SetOrientation(0); // Ignored in mathtextactor3d
  tprop->SetJustificationToLeft(); // Ignored in mathtextactor3d
  tprop->SetVerticalJustificationToBottom(); // Ignored in mathtextactor3d

  // Get path
  vtkNew<vtkPath> path;
  vtkMathTextUtilities::GetInstance()->StringToPath(string,
                                                    path.GetPointer(),
                                                    tprop.GetPointer());

  // Extract gl transform info
  vtkMatrix4x4 *actorMatrix = textAct->GetMatrix();
  vtkNew<vtkMatrix4x4> projectionMatrix;
  vtkNew<vtkMatrix4x4> modelviewMatrix;
  vtkNew<vtkMatrix4x4> transformMatrix;
  double glMatrix[16];
  double viewport[4];
  double depthRange[2];

  double *textBounds = textAct->GetBounds();
  double rasterPos[3] = {(textBounds[1] + textBounds[0]) * 0.5,
                       (textBounds[3] + textBounds[2]) * 0.5,
                       (textBounds[5] + textBounds[4]) * 0.5};

  double scale[2] = {1.0, 1.0};
  double *dcolor = tprop->GetColor();
  unsigned char color[3] = {static_cast<unsigned char>(dcolor[0]*255),
                            static_cast<unsigned char>(dcolor[1]*255),
                            static_cast<unsigned char>(dcolor[2]*255)};
  double winsized[2] = {static_cast<double>(winsize[0]),
                        static_cast<double>(winsize[1])};

  vtkSmartPointer<vtkPoints> origPoints = path->GetPoints();
  vtkRenderer *ren = 0;
  for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
    // Setup gl matrices
    ren->GetActiveCamera()->Render(ren);

    // Build transformation matrix
    glGetDoublev(GL_PROJECTION_MATRIX, glMatrix);
    projectionMatrix->DeepCopy(glMatrix);
    projectionMatrix->Transpose();
    glGetDoublev(GL_MODELVIEW_MATRIX, glMatrix);
    modelviewMatrix->DeepCopy(glMatrix);
    modelviewMatrix->Transpose();
    vtkMatrix4x4::Multiply4x4(projectionMatrix.GetPointer(),
                              modelviewMatrix.GetPointer(),
                              transformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                              actorMatrix,
                              transformMatrix.GetPointer());

    glGetDoublev(GL_VIEWPORT, viewport);
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    const double halfWidth = viewport[2] * 0.5;
    const double halfHeight = viewport[3] * 0.5;
    const double zFactor1 = (depthRange[1] - depthRange[0]) * 0.5;
    const double zFactor2 = (depthRange[1] + depthRange[0]) * 0.5;

    vtkNew<vtkPoints> newPoints;
    newPoints->DeepCopy(origPoints);
    double point[4];
    double translation[2] = {0.0, 0.0};
    for (vtkIdType i = 0; i < path->GetNumberOfPoints(); ++i)
      {
      newPoints->GetPoint(i, point);
      point[3] = 1.0;
      // Convert path to clip coordinates:
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

    vtkGL2PSUtilities::DrawPath(path.GetPointer(), rasterPos, winsized,
                                translation, scale, 0.0, color);
    glMatrixMode(GL_PROJECTION_MATRIX);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW_MATRIX);
    glPopMatrix();
    }
}
