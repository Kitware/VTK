#include "vtkBooleanTexture.h"
#include "vtkTexture.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkQuadric.h"
#include "vtkSphereSource.h"
#include "vtkImplicitTextureCoords.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

float positions[][3] = {
	{-4,4,0}, {-2,4,0}, {0,4,0}, {2,4,0},
	{-4,2,0}, {-2,2,0}, {0,2,0}, {2,2,0},
	{-4,0,0}, {-2,0,0}, {0,0,0}, {2,0,0},
	{-4,-2,0}, {-2,-2,0}, {0,-2,0}, {2,-2,0}
};

vtkBooleanTexture *makeBooleanTexture (int, int, int);

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  int i;
  vtkBooleanTexture *aBoolean[16];
  vtkTexture *aTexture[16];
  vtkActor *anActor[16];

  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // define two elliptical cylinders
  vtkQuadric *quadric1 = vtkQuadric::New();
      quadric1->SetCoefficients (1,2,0,0,0,0,0,0,0,-.07);

  vtkQuadric *quadric2 = vtkQuadric::New();
      quadric2->SetCoefficients (2,1,0,0,0,0,0,0,0,-.07);

  // create a sphere for all to use
  vtkSphereSource *aSphere = vtkSphereSource::New();
    aSphere->SetPhiResolution(100); aSphere->SetThetaResolution(100); 

  // create texture coordianates for all
  vtkImplicitTextureCoords *tcoords = vtkImplicitTextureCoords::New();
      tcoords->SetInput(aSphere->GetOutput());
      tcoords->SetRFunction(quadric1);
      tcoords->SetSFunction(quadric2);

  vtkDataSetMapper *aMapper = vtkDataSetMapper::New();
      aMapper->SetInput (tcoords->GetOutput ());

  // create a mapper, sphere and texture map for each case
  for (i = 0; i < 16; i++) 
    {
    aBoolean[i] = makeBooleanTexture (i, 256, 1);

    aTexture[i] = vtkTexture::New();
      aTexture[i]->SetInput (aBoolean[i]->GetOutput());
      aTexture[i]->InterpolateOff();
      aTexture[i]->RepeatOff ();
    anActor[i] = vtkActor::New();
      anActor[i]->SetMapper (aMapper);
      anActor[i]->SetTexture (aTexture[i]);
      anActor[i]->SetPosition ( &positions[i][0]);
      anActor[i]->SetScale (2.0, 2.0, 2.0);
    aren->AddActor (anActor[i]);
    }

  aren->SetBackground (0.4392,0.5020,0.5647);
  aren->GetActiveCamera()->Zoom(1.4);
  renWin->DoubleBufferOff();
  renWin->SetSize(300,300);

  // interact with data
  //renWin->SetFileName("plate42.ppm");
  renWin->Render();
  //renWin->SaveImageAsPPM();

  SAVEIMAGE( renWin );
  
  iren->Start();

  // Clean up
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  quadric1->Delete();
  quadric2->Delete();
  aSphere->Delete();
  tcoords->Delete();
  aMapper->Delete();
  for (i=0; i<16; i++)
    {
    aBoolean[i]->Delete();
    aTexture[i]->Delete();
    anActor[i]->Delete();
    }
}

static unsigned char solid[] = {255, 255};
static unsigned char clear[] = {255, 0};
static unsigned char edge[] =  {0, 255};

vtkBooleanTexture *makeBooleanTexture (int caseNumber, int resolution, int thickness)
{
  vtkBooleanTexture *booleanTexture = vtkBooleanTexture::New();

  booleanTexture->SetXSize (resolution);
  booleanTexture->SetYSize (resolution);
  booleanTexture->SetThickness (thickness);

  switch (caseNumber) 
    {
    case 0:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (solid);
      booleanTexture->SetOnIn (solid);
      booleanTexture->SetOnOut (solid);
      booleanTexture->SetInOn (solid);
      booleanTexture->SetOutOn (solid);
      break;
    case 1:	/* cut inside 1 */
      booleanTexture->SetInIn (clear);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (solid);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (solid);
      break;
    case 2:	/* cut outside 1 */
      booleanTexture->SetInIn (solid);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (solid);
      booleanTexture->SetInOn (solid);
      booleanTexture->SetOutOn (edge);
      break;
    case 3:	/* cut all 1 */
      booleanTexture->SetInIn (clear);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (clear);
      booleanTexture->SetOnOut (solid);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (edge);
      break;
    case 4:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (solid);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (solid);
      break;
    case 5:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (clear);
      booleanTexture->SetOutOn (solid);
      break;
    case 6:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (edge);
      break;
    case 7:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutOut (solid);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (clear);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (clear);
      booleanTexture->SetOutOn (edge);
      break;
    case 8:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (solid);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (solid);
      booleanTexture->SetOutOn (edge);
      break;
    case 9:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (edge);
      break;
    case 10:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (solid);
      booleanTexture->SetOutOn (clear);
      break;
    case 11:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetInOut (solid);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (clear);
      booleanTexture->SetOnOut (edge);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (clear);
      break;
    case 12:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (solid);
      booleanTexture->SetOnOut (clear);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (edge);
      break;
    case 13:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutIn (solid);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (clear);
      booleanTexture->SetInOn (clear);
      booleanTexture->SetOutOn (edge);
      break;
    case 14:
      booleanTexture->SetInIn (solid);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (edge);
      booleanTexture->SetOnIn (edge);
      booleanTexture->SetOnOut (clear);
      booleanTexture->SetInOn (edge);
      booleanTexture->SetOutOn (clear);
      break;
    case 15:
      booleanTexture->SetInIn (clear);
      booleanTexture->SetInOut (clear);
      booleanTexture->SetOutIn (clear);
      booleanTexture->SetOutOut (clear);
      booleanTexture->SetOnOn (clear);
      booleanTexture->SetOnIn (clear);
      booleanTexture->SetOnOut (clear);
      booleanTexture->SetInOn (clear);
      booleanTexture->SetOutOn (clear);
      break;
    }

  return booleanTexture;
}

