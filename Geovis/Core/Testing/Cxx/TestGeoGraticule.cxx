#include "vtkGeoGraticule.h"

#include "vtkActor.h"
#include "vtkGeoProjection.h"
#include "vtkGeoTransform.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTransformFilter.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkWindowToImageFilter.h"

#define vtkCreateMacro(type, obj) \
  vtkSmartPointer<type> obj = vtkSmartPointer<type>::New()

int TestGeoGraticule( int argc, char* argv[] )
{
  int latLevel = 2;
  int lngLevel = 2;
  const char* pname = "rouss";
  vtkCreateMacro( vtkGeoGraticule, ggr );
  vtkCreateMacro( vtkGeoTransform, xfm );
  vtkCreateMacro( vtkGeoProjection, gcs );
  vtkCreateMacro( vtkGeoProjection, pcs );
  vtkCreateMacro( vtkTransformFilter, xff );
  vtkCreateMacro( vtkXMLPolyDataReader, pdr );
  vtkCreateMacro( vtkTransformFilter, xf2 );
  vtkCreateMacro( vtkPolyDataMapper, mapper );
  vtkCreateMacro( vtkPolyDataMapper, mapper2 );
  vtkCreateMacro( vtkActor, actor );
  vtkCreateMacro( vtkActor, actor2 );

  ggr->SetGeometryType( vtkGeoGraticule::POLYLINES );
  ggr->SetLatitudeLevel( latLevel );
  ggr->SetLongitudeLevel( lngLevel );
  ggr->SetLongitudeBounds( -180, 180 );
  ggr->SetLatitudeBounds( -90, 90 );

  // gcs defaults to latlong.
  pcs->SetName( pname );
  pcs->SetCentralMeridian( 0. );
  xfm->SetSourceProjection( gcs );
  xfm->SetDestinationProjection( pcs );
  xff->SetInputConnection( ggr->GetOutputPort() );
  xff->SetTransform( xfm );
  mapper->SetInputConnection( xff->GetOutputPort() );
  actor->SetMapper( mapper );

  char* input_file = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "/Data/political.vtp");
  pdr->SetFileName(input_file);

  xf2->SetTransform( xfm );
  xf2->SetInputConnection( pdr->GetOutputPort() );
  mapper2->SetInputConnection( xf2->GetOutputPort() );
  actor2->SetMapper( mapper2 );

  vtkCreateMacro( vtkRenderWindow, win );
  win->SetMultiSamples(0);
  vtkCreateMacro( vtkRenderer, ren );
  vtkCreateMacro( vtkRenderWindowInteractor, iren );
  win->SetInteractor( iren );
  win->AddRenderer( ren );
  ren->AddActor( actor );
  ren->AddActor( actor2 );

  win->Render();
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    }

  delete [] input_file;

  return !retVal;
};
