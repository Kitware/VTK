/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Medical3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// 
// This example reads a volume dataset, extracts two isosurfaces that
// represent the skin and bone, creates three orthogonal planes 
// (sagittal, axial, coronal), and displays them.
//
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume16Reader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkCamera.h>
#include <vtkStripper.h>
#include <vtkLookupTable.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkProperty.h>
#include <vtkPolyDataNormals.h>
#include <vtkContourFilter.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageActor.h>
#include <vtkSmartPointer.h>

int main (int argc, char *argv[])
{
  if (argc < 2)
    {
    cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return EXIT_FAILURE;
    }

  // Create the renderer, the render window, and the interactor. The
  // renderer draws into the render window, the interactor enables
  // mouse- and keyboard-based interaction with the data within the
  // render window.
  //
  vtkSmartPointer<vtkRenderer> aRenderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(aRenderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
  aRenderer->SetBackground(.2, .3, .4);
  renWin->SetSize(640, 480);
  
  // The following reader is used to read a series of 2D slices (images)
  // that compose the volume. The slice dimensions are set, and the
  // pixel spacing. The data Endianness must also be specified. The
  // reader uses the FilePrefix in combination with the slice number to
  // construct filenames using the format FilePrefix.%d. (In this case
  // the FilePrefix is the root name of the file: quarter.)
  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64,64);
  v16->SetImageRange(1, 93);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetFilePrefix (argv[1]);
  v16->SetDataSpacing (3.2, 3.2, 1.5);
  v16->Update();

  // An isosurface, or contour value of 500 is known to correspond to
  // the skin of the patient. Once generated, a vtkPolyDataNormals
  // filter is is used to create normals for smooth surface shading
  // during rendering.  The triangle stripper is used to create triangle
  // strips from the isosurface; these render much faster on may
  // systems.
  vtkSmartPointer<vtkContourFilter> skinExtractor =
    vtkSmartPointer<vtkContourFilter>::New();
  skinExtractor->SetInputConnection( v16->GetOutputPort());
  skinExtractor->SetValue(0, 500);
  skinExtractor->Update();

  vtkSmartPointer<vtkPolyDataNormals> skinNormals =
    vtkSmartPointer<vtkPolyDataNormals>::New();
  skinNormals->SetInputConnection(skinExtractor->GetOutputPort());
  skinNormals->SetFeatureAngle(60.0);
  skinNormals->Update();

  vtkSmartPointer<vtkStripper> skinStripper =
    vtkSmartPointer<vtkStripper>::New();
  skinStripper->SetInputConnection(skinNormals->GetOutputPort());
  skinStripper->Update();

  vtkSmartPointer<vtkPolyDataMapper> skinMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  skinMapper->SetInputConnection(skinStripper->GetOutputPort());
  skinMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> skin =
    vtkSmartPointer<vtkActor>::New();
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
  vtkSmartPointer<vtkContourFilter> boneExtractor =
    vtkSmartPointer<vtkContourFilter>::New();
  boneExtractor->SetInputConnection(v16->GetOutputPort());
  boneExtractor->SetValue(0, 1150);

  vtkSmartPointer<vtkPolyDataNormals> boneNormals =
    vtkSmartPointer<vtkPolyDataNormals>::New();
  boneNormals->SetInputConnection(boneExtractor->GetOutputPort());
  boneNormals->SetFeatureAngle(60.0);

  vtkSmartPointer<vtkStripper> boneStripper =
    vtkSmartPointer<vtkStripper>::New();
  boneStripper->SetInputConnection(boneNormals->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> boneMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  boneMapper->SetInputConnection(boneStripper->GetOutputPort());
  boneMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> bone =
    vtkSmartPointer<vtkActor>::New();
  bone->SetMapper(boneMapper);
  bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

  // An outline provides context around the data.
  //
  vtkSmartPointer<vtkOutlineFilter> outlineData =
    vtkSmartPointer<vtkOutlineFilter>::New();
  outlineData->SetInputConnection(v16->GetOutputPort());
  outlineData->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapOutline =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapOutline->SetInputConnection(outlineData->GetOutputPort());

  vtkSmartPointer<vtkActor> outline =
    vtkSmartPointer<vtkActor>::New();
  outline->SetMapper(mapOutline);
  outline->GetProperty()->SetColor(0,0,0);

  // Now we are creating three orthogonal planes passing through the
  // volume. Each plane uses a different texture map and therefore has
  // different coloration.

  // Start by creating a black/white lookup table.
  vtkSmartPointer<vtkLookupTable> bwLut =
    vtkSmartPointer<vtkLookupTable>::New();
  bwLut->SetTableRange (0, 2000);
  bwLut->SetSaturationRange (0, 0);
  bwLut->SetHueRange (0, 0);
  bwLut->SetValueRange (0, 1);
  bwLut->Build(); //effective built

  // Now create a lookup table that consists of the full hue circle
  // (from HSV).
  vtkSmartPointer<vtkLookupTable> hueLut =
    vtkSmartPointer<vtkLookupTable>::New();
  hueLut->SetTableRange (0, 2000);
  hueLut->SetHueRange (0, 1);
  hueLut->SetSaturationRange (1, 1);
  hueLut->SetValueRange (1, 1);
  hueLut->Build(); //effective built

  // Finally, create a lookup table with a single hue but having a range
  // in the saturation of the hue.
  vtkSmartPointer<vtkLookupTable> satLut =
    vtkSmartPointer<vtkLookupTable>::New();
  satLut->SetTableRange (0, 2000);
  satLut->SetHueRange (.6, .6);
  satLut->SetSaturationRange (0, 1);
  satLut->SetValueRange (1, 1);
  satLut->Build(); //effective built

  // Create the first of the three planes. The filter vtkImageMapToColors
  // maps the data through the corresponding lookup table created above.  The
  // vtkImageActor is a type of vtkProp and conveniently displays an image on
  // a single quadrilateral plane. It does this using texture mapping and as
  // a result is quite fast. (Note: the input image has to be unsigned char
  // values, which the vtkImageMapToColors produces.) Note also that by
  // specifying the DisplayExtent, the pipeline requests data of this extent
  // and the vtkImageMapToColors only processes a slice of data.
  vtkSmartPointer<vtkImageMapToColors> sagittalColors =
    vtkSmartPointer<vtkImageMapToColors>::New();
  sagittalColors->SetInputConnection(v16->GetOutputPort());
  sagittalColors->SetLookupTable(bwLut);
  sagittalColors->Update();

  vtkSmartPointer<vtkImageActor> sagittal =
    vtkSmartPointer<vtkImageActor>::New();
  sagittal->SetInput(sagittalColors->GetOutput());
  sagittal->SetDisplayExtent(32,32, 0,63, 0,92);

  // Create the second (axial) plane of the three planes. We use the
  // same approach as before except that the extent differs.
  vtkSmartPointer<vtkImageMapToColors> axialColors =
    vtkSmartPointer<vtkImageMapToColors>::New();
  axialColors->SetInputConnection(v16->GetOutputPort());
  axialColors->SetLookupTable(hueLut);
  axialColors->Update();

  vtkSmartPointer<vtkImageActor> axial =
    vtkSmartPointer<vtkImageActor>::New();
  axial->SetInput(axialColors->GetOutput());
  axial->SetDisplayExtent(0,63, 0,63, 46,46);

  // Create the third (coronal) plane of the three planes. We use 
  // the same approach as before except that the extent differs.
  vtkSmartPointer<vtkImageMapToColors> coronalColors =
    vtkSmartPointer<vtkImageMapToColors>::New();
  coronalColors->SetInputConnection(v16->GetOutputPort());
  coronalColors->SetLookupTable(satLut);
  coronalColors->Update();

  vtkSmartPointer<vtkImageActor> coronal =
    vtkSmartPointer<vtkImageActor>::New();
  coronal->SetInput(coronalColors->GetOutput());
  coronal->SetDisplayExtent(0,63, 32,32, 0,92);

  // It is convenient to create an initial view of the data. The
  // FocalPoint and Position form a vector direction. Later on
  // (ResetCamera() method) this vector is used to position the camera
  // to look at the data in this direction.
  vtkSmartPointer<vtkCamera> aCamera =
    vtkSmartPointer<vtkCamera>::New();
  aCamera->SetViewUp (0, 0, -1);
  aCamera->SetPosition (0, 1, 0);
  aCamera->SetFocalPoint (0, 0, 0);
  aCamera->ComputeViewPlaneNormal();
  aCamera->Azimuth(30.0);
  aCamera->Elevation(30.0);

  // Actors are added to the renderer. 
  aRenderer->AddActor(outline);
  aRenderer->AddActor(sagittal);
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
  
  // Calling Render() directly on a vtkRenderer is strictly forbidden.
  // Only calling Render() on the vtkRenderWindow is a valid call.
  renWin->Render();
  
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);
  
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

  return EXIT_SUCCESS;
}
