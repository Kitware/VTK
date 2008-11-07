#include "vtkGeoProjection.h"

int TestGeoProjection( int argc, char* argv[] )
{
  const char* projName = argc > 1 ? argv[1] : "rouss";
  int np = vtkGeoProjection::GetNumberOfProjections();
  cout << "Supported projections:\n";
  for ( int i = 0; i < np; ++ i )
    {
    cout << "Projection: " << vtkGeoProjection::GetProjectionName( i ) << "\n";
    cout << "\t" << vtkGeoProjection::GetProjectionDescription( i ) << "\n";
    }
  cout << "-------\n";
  vtkGeoProjection* proj = vtkGeoProjection::New();
  proj->SetName( projName );
  cout << projName << " is " << proj->GetDescription() << "\n";
  proj->Delete();
  return 0;
}
