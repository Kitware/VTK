/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceReconstructionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton (MINORI Project, Dental and Medical
             Informatics, Eastman Dental Institute, London, UK) who
             developed and contributed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkSurfaceReconstructionFilter - reconstructs a surface from unorganized points
// .SECTION Description
// 
// vtkSurfaceReconstructionFilter takes a list of points assumed to lie on
// the surface of a solid 3D object. A signed measure of the distance to the
// surface is computed and sampled on a regular grid. The grid can then be
// contoured at zero to extract the surface. The default values for
// neighborhood size and sample spacing should give reasonable results for
// most uses but can be set if desired. This procedure is based on the PhD
// work of Hugues Hoppe: http://www.research.microsoft.com/-hoppe

#ifndef __vtkSurfaceReconstructionFilter_h
#define __vtkSurfaceReconstructionFilter_h

#include "vtkDataSetToStructuredPointsFilter.h"

class VTK_EXPORT vtkSurfaceReconstructionFilter : public vtkDataSetToStructuredPointsFilter
{
public:
  vtkTypeMacro(vtkSurfaceReconstructionFilter,vtkDataSetToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with NeighborhoodSize=20.
  static vtkSurfaceReconstructionFilter* New();

  // Description: 
  // Specify the number of neighbors each point has, used for estimating the
  // local surface orientation.  The default value of 20 should be OK for
  // most applications, higher values can be specified if the spread of
  // points is uneven. Values as low as 10 may yield adequate results for
  // some surfaces. Higher values cause the algorithm to take longer. Higher
  // values will cause errors on sharp boundaries.
  vtkGetMacro(NeighborhoodSize,int);
  vtkSetMacro(NeighborhoodSize,int);

  // Description: 
  // Specify the spacing of the 3D sampling grid. If not set, a
  // reasonable guess will be made.
  vtkGetMacro(SampleSpacing,float);
  vtkSetMacro(SampleSpacing,float);

protected:
  vtkSurfaceReconstructionFilter();
  ~vtkSurfaceReconstructionFilter() {};
  vtkSurfaceReconstructionFilter(const vtkSurfaceReconstructionFilter&) {};
  void operator=(const vtkSurfaceReconstructionFilter&) {};

  void Execute();

  int NeighborhoodSize;
  float SampleSpacing;
};

#endif

