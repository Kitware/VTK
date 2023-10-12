// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeoProjection.h"
#include "vtkGeoTransform.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"

#include <algorithm>
#include <array>
#include <iterator>

int TestGeoProjection(int, char*[])
{
  int np = vtkGeoProjection::GetNumberOfProjections();
  cout << "Supported projections:\n";
  for (int i = 0; i < np; ++i)
  {
    cout << "Projection: " << vtkGeoProjection::GetProjectionName(i) << "\n";
    cout << "\t" << vtkGeoProjection::GetProjectionDescription(i) << "\n";
  }
  cout << "-------\n";
  vtkGeoProjection* proj = vtkGeoProjection::New();
  const char* projName = "rouss";
  proj->SetName(projName);
  cout << projName << " is " << proj->GetDescription() << "\n";
  proj->Delete();

  std::array<double, 3> galatiCart{ 3960080.027008516, 2102195.367671419, 4521336.196173832 };
  const std::array<double, 3> galatiLonLatExpected{ 27.96144955485114, 45.43337341871766,
    84.56871610693634 };
  const int zoneExpected = 35;
  std::array<double, 3> galatiLonLat;
  vtkNew<vtkGeoTransform> transform;
  transform->SetTransformZCoordinate(true);
  transform->SetSourceProjection("+proj=cart");
  transform->SetDestinationProjection("+proj=lonlat");
  transform->InternalTransformPoint(galatiCart.data(), galatiLonLat.data());
  int zone = vtkGeoTransform::ComputeUTMZone(galatiLonLat[0], galatiLonLat[1]);
  if (std::equal(galatiLonLat.begin(), galatiLonLat.end(), galatiLonLatExpected.begin(),
        [](double first, double second) { return vtkMathUtilities::NearlyEqual(first, second); }) &&
    zone == zoneExpected)
  {
    return EXIT_SUCCESS;
  }
  else
  {
    std::cout << "Cart -> LonLat conversion resulted in unexpected result:\n";
    std::copy(
      galatiLonLat.begin(), galatiLonLat.end(), std::ostream_iterator<double>(std::cout, " "));
    std::cout << " zone=" << zone;
    std::cout << "\nexpected:\n";
    std::copy(galatiLonLatExpected.begin(), galatiLonLatExpected.end(),
      std::ostream_iterator<double>(std::cout, " "));
    std::cout << " zoneExpected=" << zoneExpected;
    return EXIT_FAILURE;
  }
}
