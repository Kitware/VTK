/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCastToConcrete.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCastToConcrete - works around type-checking limitations
// .SECTION Description
// vtkCastToConcrete is a filter that works around type-checking limitations
// in the filter classes. Some filters generate abstract types on output, 
// and cannot be connected to the input of filters requiring a concrete
// input type. For example, vtkElevationFilter generates vtkDataSet for output,
// and cannot be connected to vtkDecimate, because vtkDecimate requires 
// vtkPolyData as input. This is true even though (in this example) the input 
// to vtkElevationFilter is of type vtkPolyData, and you know the output of 
// vtkElevationFilter is the same type as its input.
//
// vtkCastToConcrete performs run-time checking to insure that output type
// is of the right type. An error message will result if you try to cast
// an input type improperly. Otherwise, the filter performs the appropriate
// cast and returns the data.

// .SECTION Caveats
// You must specify the input before you can get the output. Otherwise an
// error results.

// .SECTION See Also
// vtkDataSetToDataSetFilter vtkPointSetToPointSetFilter

#ifndef __vtkCastToConcrete_h
#define __vtkCastToConcrete_h

#include "vtkDataSetFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

class VTK_EXPORT vtkCastToConcrete : public vtkDataSetFilter
{

public:
  vtkCastToConcrete();
  ~vtkCastToConcrete();
  char *GetClassName() {return "vtkCastToConcrete";};

  // filter interface (special pass-thru)
  void Update();

  vtkDataSet *GetOutput();
  vtkPolyData *GetPolyDataOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

protected:
  void Execute(); //insures compatibility; satisfies abstract api in vtkFilter
  
  // objects used in the casting process
  vtkPolyData *PolyData;
  vtkStructuredPoints *StructuredPoints;
  vtkStructuredGrid *StructuredGrid;
  vtkUnstructuredGrid *UnstructuredGrid;
};

#endif
