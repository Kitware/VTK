/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Medical3.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// 
// This example reads a volume dataset, extracts two isosurfaces that
// represent the skin and bone, creates three orthogonal planes 
// (saggital, axial, coronal), and displays them.
//
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkStripper.h"
#include "vtkLookupTable.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkProperty.h"
#include "vtkPolyDataNormals.h"
#include "vtkContourFilter.h"
#include "vtkImageData.h"
#include "vtkImageClip.h"
#include "vtkImageMapToColors.h"
#include "vtkImageActor.h"

int main (int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return 1;
    }

  // Create the renderer, the render window, and the interactor. The
  // renderer draws into the render window, the interactor enables
  // mouse- and keyboard-based interaction with the data within the
  // render window.
  //
  vtkRenderer *aRenderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aRenderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // The following reader is used to read a series of 2D slices (images)
  // that compose the volume. The slice dimensions are set, and the
  // pixel spacing. The data Endianness must also be specified. The
  // reader usese the FilePrefix in combination with the slice number to
  // construct filenames using the format FilePrefix.%d. (In this case
  // the FilePrefix is the root name of the file: quarter.)
  vtkVolume16Reader *v16 = vtkVolume16Reader::New();
    v16->SetDataDimensions(64,64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix (argv[1]);
    v16->SetImageRange(1, 93);
    v16->SetDataSpacing (3.2, 3.2, 1.5);

  // An isosurface, or contour value of 500 is known to correspond to
  // the skin of the patient. Once generated, a vtkPolyDataNormals
  // filter is is used to create normals for smooth surface shading
  // during rendering.  The triangle stripper is used to create triangle
  // strips from the isosurface; these render much faster on may
  // systems.
  vtkContourFilter *skinExtractor = vtkContourFilter::New();
    skinExtractor->SetInput( v16->GetOutput());
    skinExtractor->SetValue(0, 500);
  vtkPolyDataNormals *skinNormals = vtkPolyDataNormals::New();
    skinNormals->SetInput(skinExtractor->GetOutput());
    skinNormals->SetFeatureAngle(60.0);
  vtkStripper *skinStripper = vtkStripper::New();
    skinStripper->SetInput(skinNormals->GetOutput());
  vtkPolyDataMapper *skinMapper = vtkPolyDataMapper::New();
    skinMapper->SetInput(skinStripper->GetOutput());
    skinMapper->ScalarVisibilityOff();
  vtkActor *skin = vtkActor::New();
    skin->SetMapper(skinMapper);
    skin->GetProperty()->SetDiffuseColor(1, .49, .25);
    skin->GetProperty()->SetSpecular(.3);
    skin->GetProperty()->SetSpecularPower(20);

  // An isosurface, or contour value of 1150 is known to correspond to
  // the skin of the patient. Once generated, a vtkPolyDataNormals
  // filter is is used to create normals for smooth surface shading
  // during rendering.  The triangle stripper is used to create triangle
  // strips from the isosurface; these render much faster on may
  // systems.
  vtkContourFilter *boneExtractor = vtkContourFilter::New();
    boneExtractor->SetInput((vtkDataSet *) v16->GetOutput());
    boneExtractor->SetValue(0, 1150);
  vtkPolyDataNormals *boneNormals = vtkPolyDataNormals::New();
    boneNormals->SetInput(boneExtractor->GetOutput());
    boneNormals->SetFeatureAngle(60.0);
  vtkStripper *boneStripper = vtkStripper::New();
    boneStripper->SetInput(boneNormals->GetOutput());
  vtkPolyDataMapper *boneMapper = vtkPolyDataMapper::New();
    boneMapper->SetInput(boneStripper->GetOutput());
    boneMapper->ScalarVisibilityOff();
  vtkActor *bone = vtkActor::New();
    bone->SetMapper(boneMapper);
    bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

  // An outline provides context around the data.
  //
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInput((vtkDataSet *) v16->GetOutput());
  vtkPolyDataMapper *mapOutline = vtkPolyDataMapper::New();
    mapOutline->SetInput(outlineData->GetOutput());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(0,0,0);

  // Now we are creating three orthogonal planes passing through the
  // volume. Each plane uses a different texture map and therefore has
  // diferent coloration.

  // Start by creatin a black/white lookup table.
  vtkLookupTable *bwLut = vtkLookupTable::New();
    bwLut->SetTableRange (0, 2000);
    bwLut->SetSaturationRange (0, 0);
    bwLut->SetHueRange (0, 0);
    bwLut->SetValueRange (0, 1);

  // Now create a lookup table that consists of the full hue circle
  // (from HSV).
  vtkLookupTable *hueLut = vtkLookupTable::New();
    hueLut->SetTableRange (0, 2000);
    hueLut->SetHueRange (0, 1);
    hueLut->SetSaturationRange (1, 1);
    hueLut->SetValueRange (1, 1);

  // Finally, create a lookup table with a single hue but having a range
  // in the saturation of the hue.
  vtkLookupTable *satLut = vtkLookupTable::New();
    satLut->SetTableRange (0, 2000);
    satLut->SetHueRange (.6, .6);
    satLut->SetSaturationRange (0, 1);
    satLut->SetValueRange (1, 1);

  // Create the first of the three planes. We want to avoid duplicating
  // data, so we clip the extent of the data to the plane that we want.
  // Then just this plane is mapped through a lookup table. The
  // vtkImageActor is a type of vtkProp that conveniently displays an
  // image on a single quadrilateral plane. It does this using texture
  // mapping and as a result is quite fast. (Note: the input image has
  // to be unsigned char values, which the vtkImageMapToColors
  // produces.)
  vtkImageClip *saggitalSection = vtkImageClip::New();
    saggitalSection->SetInput (v16->GetOutput());
    saggitalSection->SetOutputWholeExtent(32,32, 0,63, 0, 92);
  vtkImageMapToColors *saggitalColors = vtkImageMapToColors::New();
    saggitalColors->SetInput(saggitalSection->GetOutput());
    saggitalColors->SetLookupTable(bwLut);
  vtkImageActor *saggital = vtkImageActor::New();
    saggital->SetInput(saggitalColors->GetOutput());
    saggital->SetDisplayExtent(32,32, 0,63, 0,92);

  // Create the second (axial) plane of the three planes. We use the
  // same approach as before except that the extent differs.
  vtkImageClip *axialSection = vtkImageClip::New();
    axialSection->SetInput (v16->GetOutput());
    axialSection->SetOutputWholeExtent(0,63, 0,63, 46,46);
  vtkImageMapToColors *axialColors = vtkImageMapToColors::New();
    axialColors->SetInput(axialSection->GetOutput());
    axialColors->SetLookupTable(hueLut);
  vtkImageActor *axial = vtkImageActor::New();
    axial->SetInput(axialColors->GetOutput());
    axial->SetDisplayExtent(0,63, 0,63, 46,46);

  // Create the third (coronal) plane of the three planes. We use 
  // the same approach as before except that the extent differs.
  vtkImageClip *coronalSection = vtkImageClip::New();
    coronalSection->SetInput (v16->GetOutput());
    coronalSection->SetOutputWholeExtent(0,63, 32,32, 0, 92);
  vtkImageMapToColors *coronalColors = vtkImageMapToColors::New();
    coronalColors->SetInput(coronalSection->GetOutput());
    coronalColors->SetLookupTable(satLut);
  vtkImageActor *coronal = vtkImageActor::New();
    coronal->SetInput(coronalColors->GetOutput());
    coronal->SetDisplayExtent(0,63, 32,32, 0,92);

  // It is convenient to create an initial view of the data. The
  // FocalPoint and Position form a vector direction. Later on
  // (ResetCamera() method) this vector is used to position the camera
  // to look at the data in this direction.
  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);
    aCamera->ComputeViewPlaneNormal();

  // Actors are added to the renderer. 
  aRenderer->AddActor(outline);
  aRenderer->AddActor(saggital);
  aRenderer->AddActor(axial);
  aRenderer->AddActor(coronal);
  aRenderer->AddActor(axial);
  aRenderer->AddActor(coronal);
  aRenderer->AddActor(skin);
  aRenderer->AddActor(bone);

  // Turn off bone for this example.
  bone->VisibilityOff();

  // Set skin to semi-transparent.
  skin->GetProperty()->SetOpacity(0.5);

  // An initial camera view is created.  The Dolly() method moves 
  // the camera towards the FocalPoint, thereby enlarging the image.
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

  // interact with data
  iren->Initialize();
  iren->Start(); 

  return 0;
}
