// 
// create iso-surface of the Lorenz attractor
//

#include "vtkMath.h"
#include "vtkShortArray.h"
#include "vtkStructuredPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

float	Pr = 10.0;	// The Lorenz parameters
float	b = 2.667;
float	r = 28.0;
float	x, y, z;	// starting (and current) x, y, z 
float	h = 0.01;	// integration step size 
int	resolution=200;	// slice resolution 
int	iter = 10000000;	// number of iterations 
float	xmin = -30.0;	// x, y, z range for voxels 
float	xmax = 30.0;
float	ymin = -30.0;
float	ymax = 30.0;
float	zmin = -10.0;
float	zmax = 60.0;

int randomMode = 1;
float xIncr, yIncr, zIncr;
short	*slice;

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  int	i, j;
  float	xx, yy, zz;
  short	xxx, yyy, zzz;
  int	sliceSize;

  short *s;
  void options(int, char**);
  int numPts, index;

  // take a stab at an integration step size
  xIncr = resolution / (xmax - xmin);
  yIncr = resolution / (ymax - ymin);
  zIncr = resolution / (zmax - zmin);

  printf ("The Lorenz Attractor\n");
  printf ("	Pr = %f\n", Pr);
  printf ("	b = %f\n", b);
  printf ("	r = %f\n", r);
  printf ("	integration step size = %f\n", h);
  printf ("	slice resolution = %d\n", resolution);
  printf ("	# of iterations = %d\n", iter);
  printf ("	specified range:\n");
  printf ("		x: %f, %f\n", xmin, xmax);
  printf ("		y: %f, %f\n", ymin, ymax);
  printf ("		z: %f, %f\n", zmin, zmax);

  x = vtkMath::Random(xmin,xmax);
  y = vtkMath::Random(ymin,ymax);
  z = vtkMath::Random(zmin,zmax);
  printf ("	starting at %f, %f, %f\n", x, y, z);

  // allocate memory for the slices
  sliceSize = resolution * resolution;
  numPts = sliceSize * resolution;
  vtkScalars *scalars = vtkScalars::New(VTK_SHORT);
  s = ((vtkShortArray *)scalars->GetData())->WritePointer(0,numPts);
  for (i=0; i < numPts; i++) s[i] = 0;

  printf ("	integrating...\n");
  for (j = 0; j < iter; j++) 
    {
    // integrate to next time step
    xx = x + h * Pr * (y - x);
    yy = y + h * (x * (r - z) - y);
    zz = z + h * (x * y - (b * z));

    x = xx; y = yy; z = zz;

    // calculate voxel index
    if (x < xmax && x > xmin && y < ymax && y > ymin && z < zmax && z > zmin) 
      {
      xxx = (short) ((float)(xx - xmin) * xIncr);
      yyy = (short) ((float)(yy - ymin) * yIncr);
      zzz = (short) ((float)(zz - zmin) * zIncr);
      index = xxx + yyy*resolution + zzz*sliceSize;
      s[index] += 1;
      }
    }

  vtkStructuredPoints *volume = vtkStructuredPoints::New();
    volume->GetPointData()->SetScalars(scalars);
    volume->SetDimensions(resolution,resolution,resolution);
    volume->SetOrigin(xmin,ymin,zmin);
    volume->SetSpacing((xmax-xmin)/resolution, (ymax-ymin)/resolution,
                       (zmax-zmin)/resolution);

  printf ("	contouring...\n");
  // do the graphics dance
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // create iso-surface
  vtkContourFilter *contour = vtkContourFilter::New();
    contour->SetInput(volume);
    contour->SetValue(0,50);

  // create mapper
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    mapper->SetInput(contour->GetOutput());
    mapper->ScalarVisibilityOff();

  // create actor
  vtkActor *actor = vtkActor::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.6863,0.9333,0.9333);

  renderer->AddActor(actor);
      renderer->SetBackground(1,1,1);
  
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  scalars->Delete();
  volume->Delete();
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  contour->Delete();
  mapper->Delete();
  actor->Delete();
}

