/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPowerCrustSurfaceReconstruction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPowerCrustSurfaceReconstruction.cxx - reconstructs surfaces from unorganized point data
// .SECTION Description
// vtkPowerCrustSurfaceReconstruction.cxx reconstructs a surface from unorganized points
// scattered across its surface. The original algorithm is the Power Crust, for full details and
// for the original code, see:
//
// http://www.cs.utexas.edu/users/amenta/powercrust/welcome.html
//
// IMPORTANT: The PowerCrust code was released under the GNU public licence (GPL) - this forbids
// its use for developing commercial products! As a modified version, this port therefore has the
// same restrictions. For more details see the copyrights in the source file, and:
//
// http://www.gnu.org/copyleft/gpl.html
//
// -- The restriction applies only to this class. -- 
//
// The medial surface can be accessed using GetMedialSurface() - remember to call Update() on the
// filter before accessing this, it is not part of the normal VTK pipeline.
//
// This filter is a big improvement on vtkSurfaceReconstructionFilter in almost all cases but it is 
// not as fast.
//
// .SECTION Caveats 
// The algorithm may fail to give a correct reconstruction on surfaces that are not densely
// sampled. In practice it does very well.
//
// An instance of the filter can be used to reconstruct a surface only once. To reconstruct more 
// than one surface, use more instances of the filter.
//
// The exact arithmetic routines are thought to have problems on some platforms, please report any
// problems you encounter.
//
// The orientation of the polygons is not consistent! This can be corrected by 
// vtkPolyDataNormals (ConsistencyOn) but you should be aware of it.
//
// The surface has not been simplified using the routines provided with the distribution, this will
// hopefully come soon.
//
// .SECTION See Also
// vtkSurfaceReconstructionFilter

#ifndef __vtkPowerCrustSurfaceReconstruction_h
#define __vtkPowerCrustSurfaceReconstruction_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkPowerCrustSurfaceReconstruction : public vtkDataSetToPolyDataFilter
{
public:
  static vtkPowerCrustSurfaceReconstruction *New();
  vtkTypeRevisionMacro(vtkPowerCrustSurfaceReconstruction,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the medial surface of the reconstructed surface.
  vtkPolyData* GetMedialSurface() { return this->medial_surface; }

  // Description:
  // This error function allows our ported code to report error messages neatly.
  // This is not for external use. 
  void Error(const char *message);

protected:
  vtkPowerCrustSurfaceReconstruction();
  ~vtkPowerCrustSurfaceReconstruction();

  // Description:
  // the main function that does the work
  void Execute();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  void ExecuteInformation();

  vtkPolyData *medial_surface;

private:
  vtkPowerCrustSurfaceReconstruction(const vtkPowerCrustSurfaceReconstruction&);  // Not implemented.
  void operator=(const vtkPowerCrustSurfaceReconstruction&);  // Not implemented.
};

#endif


