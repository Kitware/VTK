/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataObjectFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "vtkDataObjectSource.h"
#include "vtkDataSet.h"
#include "vtkCollection.h"

class VTK_GRAPHICS_EXPORT vtkDataSetToDataObjectFilter : public vtkDataObjectSource
{
public:
  vtkTypeMacro(vtkDataSetToDataObjectFilter,vtkDataObjectSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the object to transform all data into a data object.
  static vtkDataSetToDataObjectFilter *New();

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

  // Description:
  // Cast input to DataSet.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();

protected:
  vtkDataSetToDataObjectFilter();
  ~vtkDataSetToDataObjectFilter();
  vtkDataSetToDataObjectFilter(const vtkDataSetToDataObjectFilter&);
  void operator=(const vtkDataSetToDataObjectFilter&);

  void Execute(); //generate output data
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  int Geometry;
  int Topology;
  int PointData;
  int CellData;
  int FieldData;

};

#endif


