/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPolyDataSource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vtkPolyDataSource is an abstract class whose subclasses generate polygonal
// data.

// .SECTION See Also
// vtkPolyDataReader vtkAxes vtkBYUReader vtkConeSource vtkCubeSource
// vtkCursor3D vtkCyberReader vtkCylinderSource vtkDiskSource vtkLineSource
// vtkMCubesReader vtkOutlineSource vtkPlaneSource vtkPointSource vtkSTLReader
// vtkSphereSource vtkTextSource vtkUGFacetReader vtkVectorText

#ifndef __vtkPolyDataSource_h
#define __vtkPolyDataSource_h

#include "vtkSource.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkPolyDataSource : public vtkSource
{
public:
  static vtkPolyDataSource *New();
  const char *GetClassName() {return "vtkPolyDataSource";};

  // Description:
  // Get the output of this source.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx)
    {return (vtkPolyData *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataSource();
  ~vtkPolyDataSource() {};
  vtkPolyDataSource(const vtkPolyDataSource&) {};
  void operator=(const vtkPolyDataSource&) {};
  
  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtents method.
  int ExecutePiece;
  int ExecuteNumberOfPieces;
};

#endif





