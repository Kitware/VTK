/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockSortHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief Collection of comparison functions for std::sort.
 *
 */

#ifndef vtkBlockSortHelper_h
#define vtkBlockSortHelper_h

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkRenderer.h"


namespace vtkBlockSortHelper
{

  /**
   *  operator() for back-to-front sorting.
   *
   *  Use as the 'comp' parameter of std::sort.
   *
   */
  struct BackToFront
  {
    vtkRenderer* Renderer;
    double CameraPosition[4];

    //----------------------------------------------------------------------------
    BackToFront(vtkRenderer* ren, vtkMatrix4x4* volMatrix)
    {
      this->Renderer = ren;

      vtkCamera* cam = ren->GetActiveCamera();
      double camWorldPos[4];

      cam->GetPosition(camWorldPos);
      camWorldPos[3] = 1.0;

      // Transform the camera position to the volume (dataset) coordinate system.
      vtkNew<vtkMatrix4x4> InverseVolumeMatrix;
      InverseVolumeMatrix->DeepCopy(volMatrix);
      InverseVolumeMatrix->Invert();
      InverseVolumeMatrix->MultiplyPoint(camWorldPos, CameraPosition);
    };

    //----------------------------------------------------------------------------
    bool operator() (vtkImageData* first, vtkImageData* second)
    {
      double center[3];
      double bounds[6];

      first->GetBounds(bounds);
      this->ComputeCenter(bounds, center);
      double const dist1 = vtkMath::Distance2BetweenPoints(center,
        this->CameraPosition);

      second->GetBounds(bounds);
      this->ComputeCenter(bounds, center);
      double const dist2 = vtkMath::Distance2BetweenPoints(center,
        this->CameraPosition);

      return dist2 < dist1;
    };

    //----------------------------------------------------------------------------
    inline void ComputeCenter(double const* bounds, double* center)
    {
      center[0] = bounds[0] + std::abs(bounds[1] - bounds[0]) / 2.0;
      center[1] = bounds[2] + std::abs(bounds[3] - bounds[2]) / 2.0;
      center[2] = bounds[4] + std::abs(bounds[5] - bounds[4]) / 2.0;
    };
  };
}

#endif // vtkBlockSortHelper_h
// VTK-HeaderTest-Exclude: vtkBlockSortHelper.h
