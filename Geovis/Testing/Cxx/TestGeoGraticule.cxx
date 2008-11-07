#include "vtkGeoGraticule.h"

#include "vtkGeoProjection.h"
#include "vtkGeoTransform.h"

#include "vtkActor.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataWriter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTransformFilter.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkWindowToImageFilter.h"

int TestGeoGraticule( int argc, char* argv[] )
{
  int latLevel = argc > 1 ? atoi( argv[1] ) : 3;
  int lngLevel = argc > 2 ? atoi( argv[2] ) : 3;
  const char* fname = argc > 3 ? argv[3] : "graticule.vtk";
  const char* pname = argc > 4 ? argv[4] : "rouss";
  vtkGeoGraticule* ggr = vtkGeoGraticule::New();
  vtkPolyDataWriter* wri = vtkPolyDataWriter::New();
  vtkGeoTransform* xfm = vtkGeoTransform::New();
  vtkGeoProjection* gcs = vtkGeoProjection::New();
  vtkGeoProjection* pcs = vtkGeoProjection::New();
  vtkTransformFilter* xff = vtkTransformFilter::New();
  vtkXMLPolyDataReader* pdr = vtkXMLPolyDataReader::New();
  vtkTransformFilter* xf2 = vtkTransformFilter::New();
  vtkPolyDataWriter* wr2 = vtkPolyDataWriter::New();

  ggr->SetGeometryType( vtkGeoGraticule::POLYLINES | vtkGeoGraticule::QUADRILATERALS ); 
  ggr->SetLatitudeLevel( latLevel );
  ggr->SetLongitudeLevel( lngLevel );
  ggr->SetLongitudeBounds( -270, 90 );
  ggr->SetLatitudeBounds( -90, 90 );

  // gcs defaults to latlong.
  pcs->SetName( pname );
  pcs->SetCentralMeridian( -90. );
  xfm->SetSourceProjection( gcs );
  xfm->SetDestinationProjection( pcs );
  xff->SetInputConnection( ggr->GetOutputPort() );
  xff->SetTransform( xfm );

  wri->SetInputConnection( xff->GetOutputPort() );
  wri->SetFileName( fname );
  wri->Write();

  char* input_file = vtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/political.vtp");
  pdr->SetFileName(input_file);

  xf2->SetTransform( xfm );
  xf2->SetInputConnection( pdr->GetOutputPort() );
  wr2->SetInputConnection( xf2->GetOutputPort() );
  wr2->SetFileName( "political.vtk" );
  wr2->Write();

  ggr->Delete();
  wri->Delete();
  xfm->Delete();
  xff->Delete();
  gcs->Delete();
  pcs->Delete();

  pdr->Delete();
  xf2->Delete();
  wr2->Delete();

  delete [] input_file;

  return 0;
};
