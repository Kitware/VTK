//=======test program to make sure cell data is passed through properly=======
//
// Thanks to: Paul Hsieh, pahsied@usgs.gov 

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPoints.h"
#include "vtkScalars.h"
#include "vtkGeometryFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
    // Create rendering stuff
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // Create structured points data set
    vtkStructuredPoints *sp = vtkStructuredPoints::New();
    sp->SetDimensions(3, 3, 1);
    sp->SetOrigin(0, 0, 0);
    sp->SetSpacing(1, 1, 1);

    // Create cell scalars
    vtkScalars *scalars = vtkScalars::New();
    for (int i=0; i<4; i++)
      {
      scalars->InsertNextScalar(i*0.33f);
      }	
    sp->GetCellData()->SetScalars(scalars);

    // Render the data set by vtkPolyDataMapper via vtkGeometryFilter
    // (works in vtk 3.1)
    vtkGeometryFilter *geom = vtkGeometryFilter::New();
    geom->SetInput(sp);
    vtkPolyDataMapper *mapper1 = vtkPolyDataMapper::New();
    mapper1->SetInput(geom->GetOutput());
    mapper1->SetScalarModeToUseCellData();
    vtkActor *actor1 = vtkActor::New();
    actor1->SetMapper(mapper1);
    ren->AddActor(actor1);

    // Render the same data set by vtkDataSetMapper
    // (doesn't work in vtk 3.1 but works in vtk 2.3)
    vtkDataSetMapper *mapper2 = vtkDataSetMapper::New();
    mapper2->SetInput(sp);
    mapper2->SetScalarModeToUseCellData();

    vtkActor *actor2 = vtkActor::New();
    actor2->SetMapper(mapper2);
    actor2->AddPosition(4, 0, 0);
    ren->AddActor(actor2);

    renWin->Render();

    SAVEIMAGE( renWin );

    iren->Start();

    ren->Delete();
    renWin->Delete();
    iren->Delete();
    sp->Delete();
    geom->Delete();
    mapper1->Delete();
    actor1->Delete();
    mapper2->Delete();
    actor2->Delete();

    exit(1);
}
