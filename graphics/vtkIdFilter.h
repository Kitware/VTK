/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.h
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
// .NAME vtkIdFilter - generate scalars or field data from point and cell ids
// .SECTION Description
// vtkIdFilter is a filter to that generates scalars or field data
// using cell and point ids. That is, the point attribute data scalars
// or field data are generated from the point ids, and the cell
// attribute data scalars or field data are generated from the the
// cell ids.
//
// Typically this filter is used with vtkLabeledDataMapper (and possibly
// vtkSelectVisiblePoints) to create labels for points and cells, or labels
// for the point or cell data scalar values.

#ifndef __vtkIdFilter_h
#define __vtkIdFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkIdFilter : public vtkDataSetToDataSetFilter 
{
public:

// Description:
// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
  vtkIdFilter();

  static vtkIdFilter *New() {return new vtkIdFilter;};
  const char *GetClassName() {return "vtkIdFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/disable the generation of point ids.
  vtkSetMacro(PointIds,int);
  vtkGetMacro(PointIds,int);
  vtkBooleanMacro(PointIds,int);

  // Description:
  // Enable/disable the generation of point ids.
  vtkSetMacro(CellIds,int);
  vtkGetMacro(CellIds,int);
  vtkBooleanMacro(CellIds,int);

  // Description:
  // Set/Get the flag which controls whether to generate scalar data
  // or field data. If this flag is off, scalar data is generated.
  // Otherwise, field data is generated.
  vtkSetMacro(FieldData,int);
  vtkGetMacro(FieldData,int);
  vtkBooleanMacro(FieldData,int);

protected:
  void Execute();

  int PointIds;
  int CellIds;
  int FieldData;

};

#endif


