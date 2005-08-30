/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Medical2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example reads a volume dataset, extracts two isosurfaces that
// represent the skin and bone, and then displays them.
//

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkPolyDataMapper.h"
#include "vtkStripper.h"
#include "vtkPolyDataNormals.h"
#include "vtkContourFilter.h"

int main (int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return 1;
    }

  // Create the renderer, the render window, and the interactor. The renderer
  // draws into the render window, the interactor enables mouse- and 
  // keyboard-based interaction with the data within the render window.
  //
  vtkRenderer *aRenderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aRenderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // The following reader is used to read a series of 2D slices (images)
  // that compose the volume. The slice dimensions are set, and the
  // pixel spacing. The data Endianness must also be specified. The reader
  // usese the FilePrefix in combination with the slice number to construct
  // filenames using the format FilePrefix.%d. (In this case the FilePrefix
  // is the root name of the file: quarter.)
  vtkVolume16Reader *v16 = vtkVolume16Reader::New();
    v16->SetDataDimensions(64,64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix (argv[1]);
    v16->SetImageRange(1, 93);
    v16->SetDataSpacing (3.2, 3.2, 1.5);

  // An isosurface, or contour value of 500 is known to correspond to the
  // skin of the patient. Once generated, a vtkPolyDataNormals filter is
  // is used to create normals for smooth surface shading during rendering.
  // The triangle stripper is used to create triangle strips from the
  // isosurface; these render much faster on may systems.
  vtkContourFilter *skinExtractor = vtkContourFilter::New();
    skinExtractor->SetInputConnection(v16->GetOutputPort());
    skinExtractor->SetValue(0, 500);
  vtkPolyDataNormals *skinNormals = vtkPolyDataNormals::New();
    skinNormals->SetInputConnection(skinExtractor->GetOutputPort());
    skinNormals->SetFeatureAngle(60.0);
  vtkStripper *skinStripper = vtkStripper::New();
    skinStripper->SetInputConnection(skinNormals->GetOutputPort());
  vtkPolyDataMapper *skinMapper = vtkPolyDataMapper::New();
    skinMapper->SetInputConnection(skinStripper->GetOutputPort());
    skinMapper->ScalarVisibilityOff();
  vtkActor *skin = vtkActor::New();
    skin->SetMapper(skinMapper);
    skin->GetProperty()->SetDiffuseColor(1, .49, .25);
    skin->GetProperty()->SetSpecular(.3);
    skin->GetProperty()->SetSpecularPower(20);
    skin->GetProperty()->SetOpacity(1.0);

  // An isosurface, or contour value of 1150 is known to correspond to the
  // skin of the patient. Once generated, a vtkPolyDataNormals filter is
  // is used to create normals for smooth surface shading during rendering.
  // The triangle stripper is used to create triangle strips from the
  // isosurface; these render much faster on may systems.
  vtkContourFilter *boneExtractor = vtkContourFilter::New();
    boneExtractor->SetInputConnection(v16->GetOutputPort());
    boneExtractor->SetValue(0, 1150);
  vtkPolyDataNormals *boneNormals = vtkPolyDataNormals::New();
    boneNormals->SetInputConnection(boneExtractor->GetOutputPort());
    boneNormals->SetFeatureAngle(60.0);
  vtkStripper *boneStripper = vtkStripper::New();
    boneStripper->SetInputConnection(boneNormals->GetOutputPort());
  vtkPolyDataMapper *boneMapper = vtkPolyDataMapper::New();
    boneMapper->SetInputConnection(boneStripper->GetOutputPort());
    boneMapper->ScalarVisibilityOff();
  vtkActor *bone = vtkActor::New();
    bone->SetMapper(boneMapper);
    bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

  // An outline provides context around the data.
  //
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInputConnection(v16->GetOutputPort());
  vtkPolyDataMapper *mapOutline = vtkPolyDataMapper::New();
    mapOutline->SetInputConnection(outlineData->GetOutputPort());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(0,0,0);

  // It is convenient to create an initial view of the data. The FocalPoint
  // and Position form a vector direction. Later on (ResetCamera() method)
  // this vector is used to position the camera to look at the data in
  // this direction.
  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);
    aCamera->ComputeViewPlaneNormal();

  // Actors are added to the renderer. An initial camera view is created.
  // The Dolly() method moves the camera towards the FocalPoint,
  // thereby enlarging the image.
  aRenderer->AddActor(outline);
  aRenderer->AddActor(skin);
  aRenderer->AddActor(bone);
  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
  aRenderer->SetBackground(1,1,1);
  renWin->SetSize(640, 480);

  // Note that when camera movement occurs (as it does in the Dolly()
  // method), the clipping planes often need adjusting. Clipping planes
  // consist of two planes: near and far along the view direction. The 
  // near plane clips out objects in front of the plane; the far plane
  // clips out objects behind the plane. This way only what is drawn
  // between the planes is actually rendered.
  aRenderer->ResetCameraClippingRange ();

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
  iren->Initialize();
  iren->Start(); 

  // It is important to delete all objects created previously to prevent
  // memory leaks. In this case, since the program is on its way to
  // exiting, it is not so important. But in applications it is
  // essential.
  v16->Delete();
  skinExtractor->Delete();
  skinNormals->Delete();
  skinStripper->Delete();
  skinMapper->Delete();
  skin->Delete();
  boneExtractor->Delete();
  boneNormals->Delete();
  boneStripper->Delete();
  boneMapper->Delete();
  bone->Delete();
  outlineData->Delete();
  mapOutline->Delete();
  outline->Delete();
  aCamera->Delete();
  aRenderer->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}
