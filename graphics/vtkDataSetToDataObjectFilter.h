/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataObjectFilter.h
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
// .NAME vtkDataSetToDataObjectFilter - map dataset into data object (i.e., a field)
// .SECTION Description
// vtkDataSetToDataObjectFilter is an class that transforms a dataset into
// data object (i.e., a field). The field will have labeled data arrays
// corresponding to the topology, geometry, field data, and point and cell
// attribute data.
//
// You can control what portions of the dataset are converted into the
// output data object's field data. The instance variables Geometry,
// Topology, FieldData, PointData, and CellData are flags that control
// whether the dataset's geometry (e.g., points, spacing, origin);
// topology (e.g., cell connectivity, dimensions); the field data
// associated with the dataset's superclass data object; the dataset's
// point data attributes; and the dataset's cell data attributes. (Note:
// the data attributes include scalars, vectors, tensors, normals, texture
// coordinates, and field data.)
//
// The names used to create the field data are as follows. For vtkPolyData, 
// "Points", "Verts", "Lines", "Polys", and "Strips". For vtkUnstructuredGrid,
// "Cells" and "CellTypes". For vtkStructuredPoints, "Dimensions", "Spacing", 
// and "Origin". For vtkStructuredGrid, "Points" and "Dimensions". For
// vtkRectilinearGrid, "XCoordinates", "YCoordinates", and "ZCoordinates".
// for point attribute data, "PointScalars", "PointVectors", etc. For cell
// attribute data, "CellScalars", "CellVectors", etc. Field data arrays retain
// their original name.

// .SECTION See Also
// vtkDataObject vtkFieldData vtkDataObjectToDataSetFilter

#ifndef __vtkDataSetToDataObjectFilter_h
#define __vtkDataSetToDataObjectFilter_h

#include "vtkDataSetFilter.h"

class VTK_EXPORT vtkDataSetToDataObjectFilter : public vtkDataSetFilter
{
public:
  vtkDataSetToDataObjectFilter();
  const char *GetClassName() {return "vtkDataSetToDataObjectFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the object to transform all data into a data object.
  static vtkDataSetToDataObjectFilter *New() {
    return new vtkDataSetToDataObjectFilter;};

  // Description:
  // Get the output of this filter as a vtkDataObject.
  vtkDataObject *GetOutput() {return this->Output;};
  
  // Description:
  // Turn on/off the conversion of dataset geometry to a data object.
  vtkSetMacro(Geometry,int);
  vtkGetMacro(Geometry,int);
  vtkBooleanMacro(Geometry,int);

  // Description:
  // Turn on/off the conversion of dataset topology to a data object.
  vtkSetMacro(Topology,int);
  vtkGetMacro(Topology,int);
  vtkBooleanMacro(Topology,int);

  // Description:
  // Turn on/off the conversion of dataset field data to a data object.
  vtkSetMacro(FieldData,int);
  vtkGetMacro(FieldData,int);
  vtkBooleanMacro(FieldData,int);

  // Description:
  // Turn on/off the conversion of dataset point data to a data object.
  vtkSetMacro(PointData,int);
  vtkGetMacro(PointData,int);
  vtkBooleanMacro(PointData,int);

  // Description:
  // Turn on/off the conversion of dataset cell data to a data object.
  vtkSetMacro(CellData,int);
  vtkGetMacro(CellData,int);
  vtkBooleanMacro(CellData,int);

protected:
  void Execute(); //generate output data

  int Geometry;
  int Topology;
  int PointData;
  int CellData;
  int FieldData;
  
};

#endif


