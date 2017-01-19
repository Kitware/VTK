/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestValuePasses.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering a scene using ValuePasses to draw arrays as
// decipherable colors. In interactive mode, hit the 'c' key to cycle
// between standard and value rendered colormaps.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"


#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDefaultPass.h"
#include "vtkDataSetAttributes.h"
#include "vtkElevationFilter.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkImageSinusoidSource.h"
#include "vtkInformation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkValuePasses.h"
#include "vtkValuePainter.h"

#include "vtkCameraPass.h"
#include "vtkDefaultPass.h"
#include "vtkLightsPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkSequencePass.h"

static vtkProp * prop = NULL;

const char TestValuePassesEventLog[] =
  "# StreamVersion 1\n"
  "LeftButtonPressEvent 148 258 0 0 0 0 0\n"
  "StartInteractionEvent 148 258 0 0 0 0 0\n"
  "KeyPressEvent 177 231 0 0 99 1 c\n"
  "CharEvent 177 231 0 0 99 1 c\n"
  "KeyReleaseEvent 177 231 0 0 99 1 c\n"
  "KeyPressEvent 177 231 0 0 99 1 c\n"
  "CharEvent 177 231 0 0 99 1 c\n"
  "KeyReleaseEvent 177 231 0 0 99 1 c\n"
  "KeyPressEvent 177 231 0 0 99 1 c\n"
  "CharEvent 177 231 0 0 99 1 c\n"
  "KeyReleaseEvent 177 231 0 0 99 1 c\n"
  "KeyPressEvent 177 231 0 0 97 1 a\n"
  "CharEvent 177 231 0 0 97 1 a\n"
  "KeyReleaseEvent 177 231 0 0 97 1 a\n"
  "KeyPressEvent 177 231 0 0 97 1 a\n"
  "CharEvent 177 231 0 0 97 1 a\n"
  "KeyReleaseEvent 177 231 0 0 97 1 a\n"
  "KeyPressEvent 177 231 0 0 97 1 a\n"
  "CharEvent 177 231 0 0 97 1 a\n"
  "KeyReleaseEvent 177 231 0 0 97 1 a\n"
  "KeyPressEvent 177 231 0 0 97 1 a\n"
  "CharEvent 177 231 0 0 97 1 a\n"
  "KeyReleaseEvent 177 231 0 0 97 1 a\n"
  "KeyPressEvent 177 231 0 0 97 1 a\n"
  "CharEvent 177 231 0 0 97 1 a\n"
  "KeyReleaseEvent 177 231 0 0 97 1 a\n"
  ;

// Define interaction style
class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
  private:
  vtkOpenGLRenderer *GLRenderer;
  vtkRenderPass *VCamera;
  vtkRenderPass *Values;
  vtkRenderPass *NormalC;
  vtkAlgorithm *Alg;
  unsigned int Counter;

  public:

    static KeyPressInteractorStyle* New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    KeyPressInteractorStyle()
    {
      this->Counter = 0;
      this->SetPipelineControlPoints(NULL,NULL,NULL,NULL,NULL);
    }

    void SetPipelineControlPoints(vtkOpenGLRenderer *g,
                                  vtkRenderPass *vc,
                                  vtkRenderPass *v,
                                  vtkRenderPass *n,
                                  vtkAlgorithm *p)
    {
      this->GLRenderer = g;
      this->VCamera = vc;
      this->Values = v;
      this->NormalC = n;
      this->Alg = p;
    }

    void OnKeyPress() VTK_OVERRIDE
    {
      if (this->GLRenderer == NULL)
      {
        return;
      }

      // Get the keypress
      vtkRenderWindowInteractor *rwi = this->Interactor;
      std::string key = rwi->GetKeySym();

      if(key == "c")
      {
        vtkRenderPass * current = this->GLRenderer->GetPass();
        if (current == NULL)
        {
          cerr << "Value (multipass) rendering" << endl;
          this->GLRenderer->SetPass(this->VCamera);
          this->GLRenderer->GetRenderWindow()->Render();
        }
        else if (current == this->VCamera)
        {
          cerr << "Normal (multipass) rendering" << endl;
          this->GLRenderer->SetPass(this->NormalC);
          this->GLRenderer->GetRenderWindow()->Render();
        }
        else if (current == this->NormalC)
        {
          cerr << "Hardcoded rendering" << endl;
          this->GLRenderer->SetPass(NULL);
          this->GLRenderer->GetRenderWindow()->Render();
        }
      }

      if(key == "a")
      {
        vtkRenderPass * current = this->GLRenderer->GetPass();
        if (current == this->VCamera)
        {
          vtkDataSet *ds = vtkDataSet::SafeDownCast(this->Alg->GetOutputDataObject(0));
          if (ds)
          {
#define DO_INFO
#ifdef DO_INFO
            cerr << "Change array through actor's info" << endl;
#else
            cerr << "Change array through passes' API" << endl;
#endif
            this->Counter++;
            unsigned int na = 0;
            vtkFieldData *fd;
            fd = ds->GetPointData();
            for (int i = 0; i < fd->GetNumberOfArrays(); i++)
            {
              na += fd->GetArray(i)->GetNumberOfComponents();
            }
            fd = ds->GetCellData();
            for (int i = 0; i < fd->GetNumberOfArrays(); i++)
            {
              na += fd->GetArray(i)->GetNumberOfComponents();
            }
            int c = this->Counter % na;

            int mode;
            int a = -1;
            for (int f = 0; f < 2; f++)
            {
              if (f == 0)
              {
                mode = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
                fd = ds->GetPointData();
              }
              else
              {
                mode = VTK_SCALAR_MODE_USE_CELL_FIELD_DATA;
                fd = ds->GetCellData();
              }
              for (int i = 0; i < fd->GetNumberOfArrays(); i++)
              {
                vtkDataArray *array = fd->GetArray(i);
                for (int j = 0; j < array->GetNumberOfComponents(); j++)
                {
                  a++;
                  if (c == a)
                  {
                    cerr << "Draw " << ((mode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)?"point ":"cell ")
                         << i << "," << j << " " << fd->GetArray(i)->GetName() << endl;
#ifdef DO_INFO
                    //send it via information keys
                    vtkInformation *iv = vtkInformation::New();
                    iv->Set(vtkValuePainter::SCALAR_MODE(), mode);
# if 1
                    iv->Set(vtkValuePainter::ARRAY_NAME(), fd->GetArray(i)->GetName()); //by names
# else
                    iv->Set(vtkValuePainter::ARRAY_ID(), i); //by index
# endif
                    iv->Set(vtkValuePainter::ARRAY_COMPONENT(), j);
                    prop->SetPropertyKeys(iv);
                    iv->Delete();
#else
                    //use direct access to the renderpass API
                    vtkValuePasses *vp = (vtkValuePasses*)this->Values;
                    vp->SetInputArrayToProcess(mode, i);
                    vp->SetInputComponentToProcess(j);
#endif
                  }
                }
              }
            }
            this->GLRenderer->GetRenderWindow()->Render();
          }
        }
      }

      // Forward events
      vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

};
vtkStandardNewMacro(KeyPressInteractorStyle);

// Make sure to have a valid OpenGL context current on the calling thread
// before calling it.
bool MesaHasVTKBug8135(vtkRenderWindow *renwin)
{
  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renwin);

  vtkOpenGLExtensionManager *extmgr
    = context->GetExtensionManager();

  return (extmgr->DriverIsMesa() && !extmgr->DriverVersionAtLeast(7,3));
}

int TestValuePasses(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

//valuepasses intentionally lacks a camera so that it can cooperate with synch rendererers
  vtkSmartPointer<vtkValuePasses> valuePasses=vtkSmartPointer<vtkValuePasses>::New();
  vtkSmartPointer<vtkCameraPass> cameraPass = vtkSmartPointer<vtkCameraPass>::New();
  cameraPass->SetDelegatePass(valuePasses);

  vtkSmartPointer<vtkCameraPass> normalPasses = vtkSmartPointer<vtkCameraPass>::New();

  vtkSmartPointer<vtkSequencePass> seq = vtkSmartPointer<vtkSequencePass>::New();
  vtkSmartPointer<vtkLightsPass> lightsPass = vtkSmartPointer<vtkLightsPass>::New();
  vtkSmartPointer<vtkDefaultPass> propsPasses = vtkSmartPointer<vtkDefaultPass>::New();
  vtkSmartPointer<vtkRenderPassCollection> passes=vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(lightsPass);
  passes->AddItem(propsPasses);
  seq->SetPasses(passes);
  normalPasses->SetDelegatePass(seq);

  vtkOpenGLRenderer *glRenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  glRenderer->SetPass(cameraPass);

  vtkSmartPointer<vtkImageSinusoidSource> imageSource=vtkSmartPointer<vtkImageSinusoidSource>::New();
  imageSource->SetWholeExtent(0,9,0,9,0,9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  vtkImageData *image=imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkSmartPointer<vtkElevationFilter> ef = vtkSmartPointer<vtkElevationFilter>::New();
  ef->SetInputConnection(imageSource->GetOutputPort());
  const double *bds = image->GetBounds();
  ef->SetLowPoint(bds[0],bds[2],bds[4]);
  ef->SetHighPoint(bds[0],bds[2],bds[5]);

  vtkSmartPointer<vtkPointDataToCellData> p2c = vtkSmartPointer<vtkPointDataToCellData>::New();
  p2c->SetInputConnection(ef->GetOutputPort());
  p2c->PassPointDataOn();

  vtkSmartPointer<vtkDataSetSurfaceFilter> surface=vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

  surface->SetInputConnection(p2c->GetOutputPort());


  vtkSmartPointer<vtkPolyDataMapper> mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkSmartPointer<vtkLookupTable> lut=vtkSmartPointer<vtkLookupTable>::New();
  lut->SetTableRange(range);
  //lut->SetAlphaRange(0.5,0.5);
  lut->SetHueRange(0.2,0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  prop = actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  /*
  vtkInformation *iv = vtkInformation::New();
  iv->Set(vtkValuePainter::SCALAR_MODE(), 4);
  iv->Set(vtkValuePainter::ARRAY_ID(), 1);
  iv->Set(vtkValuePainter::ARRAY_COMPONENT(), 0);
  actor->SetPropertyKeys(iv);
  iv->Delete();
  */

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(400,400);

  // empty scene during OpenGL detection.
  actor->SetVisibility(0);
  renWin->Render();

  int retVal;
  if(MesaHasVTKBug8135(renWin))
  {
    // Mesa will crash if version<7.3
    cout<<"This version of Mesa would crash. Skip the test."<<endl;
    retVal=vtkRegressionTester::PASSED;
  }
  else
  {
    actor->SetVisibility(1);
    renderer->ResetCamera();
    vtkCamera *camera=renderer->GetActiveCamera();
    camera->Azimuth(-40.0);
    camera->Elevation(20.0);
    renWin->Render();

    vtkSmartPointer<KeyPressInteractorStyle> style =
      vtkSmartPointer<KeyPressInteractorStyle>::New();
    style->SetPipelineControlPoints(glRenderer, cameraPass, valuePasses, normalPasses, surface);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(renderer);

    vtkSmartPointer<vtkInteractorEventRecorder> recorder =
      vtkSmartPointer<vtkInteractorEventRecorder>::New();
    recorder->SetInteractor(iren);
#if 0
    recorder->SetFileName("TestValuePassesEvent.log");
    recorder->SetEnabled(1);
    recorder->Record();
    iren->Initialize();
    iren->Start();
    renWin->Render();
    recorder->Stop();
#else
    recorder->ReadFromInputStringOn();
    recorder->SetInputString(TestValuePassesEventLog);
    iren->Initialize();
    renWin->Render();
    recorder->Play();
    recorder->Off();
#endif
    retVal = vtkRegressionTestImage( renWin );
    if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

  }

  return !retVal;
}
