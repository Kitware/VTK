/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToDataSetFilter.h
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
// .NAME vtkDataObjectToDataSetFilter - map field data to concrete dataset
// .SECTION Description
// vtkDataObjectToDataSetFilter is an class that maps a data object (i.e., a field)
// into a concrete dataset, i.e., gives structure to the field by defining a
// geometry and topology.
//
// To use this filter you associate components in the input field data with
// portions of the output dataset. (A component is an array of values from
// the field.) For example, you would specify x-y-z points by assigning 
// components from the field for the x, then y, then z values of the points.
// You may also have to specify component ranges (for each z-y-z) to make 
// sure that the number of x, y, and z values is the same. Also, you may 
// want to normalize the components which helps distribute the data 
// uniformly. Once you've setup the filter to combine all the pieces of 
// data into a specified dataset (the geometry, topology, point and cell 
// data attributes), the various output methods (e.g., GetPolyData()) are
// used to retrieve the final product.
//
// This filter is often used in conjunction with
// vtkFieldDataToAttributeDataFilter.  vtkFieldDataToAttributeDataFilter
// takes field data and transforms it into attribute data (e.g., point and
// cell data attributes such as scalars and vectors).  To do this, use this
// filter which constructs a concrete dataset and passes the input data
// object field data to its output. and then use
// vtkFieldDataToAttributeDataFilter to generate the attribute data associated
// with the dataset.

// .SECTION Caveats
// Make sure that the data you extract is consistent. That is, if you have N
// points, extract N x, y, and z components. Also, all the information
// necessary to define a dataset must be given. For example, vtkPolyData
// requires points at a minumum; vtkStructuredPoints requires setting the
// dimensions; vtkStructuredGrid requires defining points and dimensions;
// vtkUnstructuredGrid requires setting points; and vtkRectilinearGrid
// requires that you define the x, y, and z-coordinate arrays (by specifying
// points) as well as the dimensions.
//
// If you wish to create a dataset of just points (i.e., unstructured points
// dataset), create vtkPolyData consisting of points. There will be no cells 
// in such a dataset.

// .SECTION See Also
// vtkDataObject vtkFieldData vtkDataSet vtkPolyData vtkStructuredPoints 
// vtkStructuredGrid vtkUnstructuredGrid vtkRectilinearGrid
// vtkDataSetAttributes vtkScalars vtkDataArray

#ifndef __vtkDataObjectToDataSetFilter_h
#define __vtkDataObjectToDataSetFilter_h

#include "vtkSource.h"
#include "vtkFieldData.h"

class vtkDataSet;
class vtkPointSet;
class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkUnstructuredGrid;
class vtkCellArray;

class VTK_EXPORT vtkDataObjectToDataSetFilter : public vtkSource
{
public:
  vtkDataObjectToDataSetFilter();
  ~vtkDataObjectToDataSetFilter();
  static vtkDataObjectToDataSetFilter *New() {return new vtkDataObjectToDataSetFilter;};
  const char *GetClassName() {return "vtkDataObjectToDataSetFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline. (Method interface inherited from vtkSource.)
  void Update();

  // Description:
  // Set the input to the filter.
  virtual void SetInput(vtkDataObject *input);
  vtkDataObject *GetInput() {return this->Input;};

  // Description:
  // Control what type of data is generated for output.
  vtkSetMacro(DataSetType,int);
  vtkGetMacro(DataSetType,int);
  void SetDataSetTypeToPolyData() {
    this->SetDataSetType(VTK_POLY_DATA);};
  void SetDataSetTypeToStructuredPoints() {
    this->SetDataSetType(VTK_STRUCTURED_POINTS);};
  void SetDataSetTypeToStructuredGrid() {
    this->SetDataSetType(VTK_STRUCTURED_GRID);};
  void SetDataSetTypeToRectilinearGrid() {
    this->SetDataSetType(VTK_RECTILINEAR_GRID);};
  void SetDataSetTypeToUnstructuredGrid() {
    this->SetDataSetType(VTK_UNSTRUCTURED_GRID);};

  // Description:
  // Get the output in different forms. The particular method invoked
  // should be consistent with the SetDataSetType() method. (Note:
  // GetOutput() will always return a type consistent with 
  // SetDataSetType(). Also, GetOutput() will return NULL if the filter
  // aborted due to inconsistent data.)
  vtkDataSet *GetOutput();
  vtkPolyData *GetPolyDataOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();

  // Description:
  // Define the component of the field to be used for the x, y, and z values
  // of the points. Note that the parameter comp must lie between (0,2) and
  // refers to the x-y-z (i.e., 0,1,2) components of the points. To define
  // the field component to use you can specify an array name and the
  // component in that array. The (min,max) values are the range of data in
  // the component you wish to extract. (This method should be used for
  // vtkPolyData, vtkUnstructuredGrid, vtkStructuredGrid, and
  // vtkRectilinearGrid.) A convenience method, SetPointComponent(),is also
  // provided which does not require setting the (min,max) component range or
  // the normalize flag (normalize is set to DefaulatNormalize value).
  void SetPointComponent(int comp, char *arrayName, int arrayComp,
                         int min, int max, int normalize);
  void SetPointComponent(int comp, char *arrayName, int arrayComp)
    {this->SetPointComponent(comp, arrayName, arrayComp, -1, -1, this->DefaultNormalize);};
  const char *GetPointComponentArrayName(int comp);
  int GetPointComponentArrayComponent(int comp);
  int GetPointComponentMinRange(int comp);
  int GetPointComponentMaxRange(int comp);
  int GetPointComponentNormailzeFlag(int comp);
  
  // Description:
  // Define cell connectivity when creating vtkPolyData. You can define
  // vertices, lines, polygons, and/or triangle strips via these methods.
  // These methods are similar to those for defining points, except
  // that no normalization of the data is possible. Basically, you need to
  // define an array of values that (for each cell) includes the number of 
  // points per cell, and then the cell connectivity. (This is the vtk file 
  // format described in in the textbook or User's Guide.)
  void SetVertsComponent(char *arrayName, int arrayComp, int min, int max);
  void SetVertsComponent(char *arrayName, int arrayComp)
    {this->SetVertsComponent(arrayName, arrayComp, -1, -1);};
  const char *GetVertsComponentArrayName();
  int GetVertsComponentArrayComponent();
  int GetVertsComponentMinRange();
  int GetVertsComponentMaxRange();
  void SetLinesComponent(char *arrayName, int arrayComp, int min, int max);
  void SetLinesComponent(char *arrayName, int arrayComp)
    {this->SetLinesComponent(arrayName, arrayComp, -1, -1);};
  const char *GetLinesComponentArrayName();
  int GetLinesComponentArrayComponent();
  int GetLinesComponentMinRange();
  int GetLinesComponentMaxRange();
  void SetPolysComponent(char *arrayName, int arrayComp, int min, int max);
  void SetPolysComponent(char *arrayName, int arrayComp)
    {this->SetPolysComponent(arrayName, arrayComp, -1, -1);};
  const char *GetPolysComponentArrayName();
  int GetPolysComponentArrayComponent();
  int GetPolysComponentMinRange();
  int GetPolysComponentMaxRange();
  void SetStripsComponent(char *arrayName, int arrayComp, int min, int max);
  void SetStripsComponent(char *arrayName, int arrayComp)
    {this->SetStripsComponent(arrayName, arrayComp, -1, -1);};
  const char *GetStripsComponentArrayName();
  int GetStripsComponentArrayComponent();
  int GetStripsComponentMinRange();
  int GetStripsComponentMaxRange();
  
  // Description:
  // Define cell types and cell connectivity when creating unstructured grid
  // data.  These methods are similar to those for defining points, except
  // that no normalization of the data is possible. Basically, you need to
  // define an array of cell types (an integer value per cell), and another
  // array consisting (for each cell) of a number of points per cell, and
  // then the cell connectivity. (This is the vtk file format described in 
  // in the textbook or User's Guide.)
  void SetCellTypeComponent(char *arrayName, int arrayComp,
                            int min, int max);
  void SetCellTypeComponent(char *arrayName, int arrayComp)
    {this->SetCellTypeComponent(arrayName, arrayComp, -1, -1);};
  const char *GetCellTypeComponentArrayName();
  int GetCellTypeComponentArrayComponent();
  int GetCellTypeComponentMinRange();
  int GetCellTypeComponentMaxRange();
  void SetCellConnectivityComponent(char *arrayName, int arrayComp,
                                    int min, int max);
  void SetCellConnectivityComponent(char *arrayName, int arrayComp)
    {this->SetCellConnectivityComponent(arrayName, arrayComp, -1, -1);};
  const char *GetCellConnectivityComponentArrayName();
  int GetCellConnectivityComponentArrayComponent();
  int GetCellConnectivityComponentMinRange();
  int GetCellConnectivityComponentMaxRange();
  
  // Description:
  // Set the default Normalize() flag for those methods setting a default
  // Normalize value (e.g., SetPointComponent).
  vtkSetMacro(DefaultNormalize,int);
  vtkGetMacro(DefaultNormalize,int);
  vtkBooleanMacro(DefaultNormalize,int);

  // Decription:
  // Specify the dimensions to use if generating a dataset that requires
  // dimensions specification (vtkStructuredPoints, vtkStructuredGrid,
  // vtkRectilinearGrid).
  vtkSetVector3Macro(Dimensions,int);
  vtkGetVectorMacro(Dimensions,int,3);
  
  // Decription:
  // Specify the origin to use if generating a dataset whose origin
  // can be set (i.e., a vtkStructuredPoints dataset).
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);
  
  // Decription:
  // Specify the spacing to use if generating a dataset whose spacing 
  // can be set (i.e., a vtkStructuredPoints dataset).
  vtkSetVector3Macro(Spacing,float);
  vtkGetVectorMacro(Spacing,float,3);
  
  // Decription:
  // Alternative methods to specify the dimensions, spacing, and origin for those
  // datasets requiring this information. You need to specify the name of an array;
  // the component of the array, and the range of the array (min,max). These methods
  // will override the information given by the previous methods.
  void SetDimensionsComponent(char *arrayName, int arrayComp, int min, int max);
  void SetDimensionsComponent(char *arrayName, int arrayComp)
    {this->SetDimensionsComponent(arrayName, arrayComp, -1, -1);};
  void SetSpacingComponent(char *arrayName, int arrayComp, int min, int max);
  void SetSpacingComponent(char *arrayName, int arrayComp)
    {this->SetSpacingComponent(arrayName, arrayComp, -1, -1);};
  void SetOriginComponent(char *arrayName, int arrayComp, int min, int max);
  void SetOriginComponent(char *arrayName, int arrayComp)
    {this->SetOriginComponent(arrayName, arrayComp, -1, -1);};
  
protected:
  void Execute(); //generate output data

  vtkDataObject *Input;
  char Updating;

  // objects used to support the retrieval of output after mapping
  vtkPolyData *PolyData;
  vtkStructuredPoints *StructuredPoints;
  vtkStructuredGrid *StructuredGrid;
  vtkUnstructuredGrid *UnstructuredGrid;
  vtkRectilinearGrid *RectilinearGrid;
  
  // control flags used to generate the output dataset
  int DataSetType; //the type of dataset to generate
  
  // Support definition of points
  char *PointArrays[3]; //the name of the arrays
  int PointArrayComponents[3]; //the array components used for x-y-z
  int PointComponentRange[3][2]; //the range of the components to use
  int PointNormalize[3]; //flags control normalization

  // These define cells for vtkPolyData
  char *VertsArray; //the name of the array
  int VertsArrayComponent; //the array component
  int VertsComponentRange[2]; //the range of the components to use

  char *LinesArray; //the name of the array
  int LinesArrayComponent; //the array component used for cell types
  int LinesComponentRange[2]; //the range of the components to use

  char *PolysArray; //the name of the array
  int PolysArrayComponent; //the array component
  int PolysComponentRange[2]; //the range of the components to use

  char *StripsArray; //the name of the array
  int StripsArrayComponent; //the array component
  int StripsComponentRange[2]; //the range of the components to use

  // Used to define vtkUnstructuredGrid datasets
  char *CellTypeArray; //the name of the array
  int CellTypeArrayComponent; //the array component used for cell types
  int CellTypeComponentRange[2]; //the range of the components to use

  char *CellConnectivityArray; //the name of the array
  int CellConnectivityArrayComponent; //the array components used for cell connectivity
  int CellConnectivityComponentRange[2]; //the range of the components to use

  // helper methods (and attributes) to construct datasets
  void SetArrayName(char* &name, char *newName);
  int ConstructPoints(vtkPointSet *ps);
  int ConstructPoints(vtkRectilinearGrid *rg);
  int ConstructCells(vtkPolyData *pd);
  int ConstructCells(vtkUnstructuredGrid *ug);
  vtkCellArray *ConstructCellArray(vtkDataArray *da, int comp, int compRange[2]);

  // Default value for normalization
  int DefaultNormalize;

  // Couple of different ways to specify dimensions, spacing, and origin.
  int Dimensions[3];
  float Origin[3];
  float Spacing[3];
  
  char *DimensionsArray; //the name of the array
  int DimensionsArrayComponent; //the component of the array used for dimensions
  int DimensionsComponentRange[2]; //the ComponentRange of the array for the dimensions
  
  char *OriginArray; //the name of the array
  int OriginArrayComponent; //the component of the array used for Origins
  int OriginComponentRange[2]; //the ComponentRange of the array for the Origins
  
  char *SpacingArray; //the name of the array
  int SpacingArrayComponent; //the component of the array used for Spacings
  int SpacingComponentRange[2]; //the ComponentRange of the array for the Spacings
  
  void ConstructDimensions();
  void ConstructSpacing();
  void ConstructOrigin();
  
};

#endif


