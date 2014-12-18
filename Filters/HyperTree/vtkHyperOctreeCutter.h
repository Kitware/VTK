/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeCutter - Cut vtkHyperOctree with user-specified
// implicit function
// .SECTION Description
// vtkHyperOctreeCutter is a filter to cut through data using any subclass of
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
//
// In VTK, cutting means reducing a cell of dimension N to a cut surface
// of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
// vtkPlane implicit function) will generate triangles. (In comparison,
// clipping takes a N dimensional cell and creates N dimension primitives.)
//
// vtkHyperOctreeCutter is generally used to "slice-through" a dataset,
// generating a surface that can be visualized. It is also possible to use
// vtkHyperOctreeCutter to do a form of volume rendering. vtkHyperOctreeCutter
// does this by generating multiple cut surfaces (usually planes) which are
// ordered (and rendered) from back-to-front. The surfaces are set translucent
// to give a volumetric rendering effect.
//
// Note that data can be cut using either 1) the scalar values associated
// with the dataset or 2) an implicit function associated with this class.
// By default, if an implicit function is set it is used to cut the data
// set, otherwise the dataset scalars are used to perform the cut.

// .SECTION See Also
// vtkImplicitFunction vtkHyperOctree

#ifndef vtkHyperOctreeCutter_h
#define vtkHyperOctreeCutter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods

#include "vtkCutter.h" // for VTK_SORT_BY_VALUE and VTK_SORT_BY_CELL

//#define VTK_SORT_BY_VALUE 0
//#define VTK_SORT_BY_CELL 1
// This does not really belong here,  ut it is for a temporary
// fix until this filter can be converted to geernate unstructured grids.
//#define VTK_NUMBER_OF_CELL_TYPES 68

class vtkImplicitFunction;
class vtkIncrementalPointLocator;
class vtkHyperOctree;
class vtkOrderedTriangulator;
class vtkHyperOctreeCursor;
class vtkTetra;
class vtkDataSetAttributes;
class vtkHyperOctreeClipCutPointsGrabber;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeCutter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeCutter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0; and
  // generating cut scalars turned off.
  static vtkHyperOctreeCutter *New();

  // Description:
  // Set a particular contour value at contour number i. The index i ranges
  // between 0<=i<NumberOfContours.
  void SetValue(int i, double value)
    {this->ContourValues->SetValue(i,value);}

  // Description:
  // Get the ith contour value.
  double GetValue(int i)
    {return this->ContourValues->GetValue(i);}

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  double *GetValues()
    {return this->ContourValues->GetValues();}

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(double *contourValues)
    {this->ContourValues->GetValues(contourValues);}

  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number)
    {this->ContourValues->SetNumberOfContours(number);}

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours()
    {return this->ContourValues->GetNumberOfContours();}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double range[2])
    {this->ContourValues->GenerateValues(numContours, range);}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  // Description:
  // Override GetMTime because we delegate to vtkContourValues and refer to
  // vtkImplicitFunction.
  unsigned long GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be
  // interpolated from the implicit function values, and not the input scalar
  // data.
  vtkSetMacro(GenerateCutScalars,int);
  vtkGetMacro(GenerateCutScalars,int);
  vtkBooleanMacro(GenerateCutScalars,int);

  // Description:
  // Specify a spatial locator for merging points. By default,
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Set the sorting order for the generated polydata. There are two
  // possibilities:
  //   Sort by value = 0 - This is the most efficient sort. For each cell,
  //      all contour values are processed. This is the default.
  //   Sort by cell = 1 - For each contour value, all cells are processed.
  //      This order should be used if the extracted polygons must be rendered
  //      in a back-to-front or front-to-back order. This is very problem
  //      dependent.
  // For most applications, the default order is fine (and faster).
  //
  // Sort by cell is going to have a problem if the input has 2D and 3D cells.
  // Cell data will be scrambled becauses with
  // vtkPolyData output, verts and lines have lower cell ids than triangles.
  vtkSetClampMacro(SortBy,int,VTK_SORT_BY_VALUE,VTK_SORT_BY_CELL);
  vtkGetMacro(SortBy,int);
  void SetSortByToSortByValue()
    {this->SetSortBy(VTK_SORT_BY_VALUE);}
  void SetSortByToSortByCell()
    {this->SetSortBy(VTK_SORT_BY_CELL);}

  // Description:
  // Return the sorting procedure as a descriptive character string.
  const char *GetSortByAsString()
    {
      if ( this->SortBy == VTK_SORT_BY_VALUE )
        {
        return "SortByValue";
        }
      else
        {
        return "SortByCell";
        }
    }

  // Description:
  // Create default locator. Used to create one when none is specified. The
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkHyperOctreeCutter(vtkImplicitFunction *cf=NULL);
  ~vtkHyperOctreeCutter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Description:
  // Cut the sub-hierarchy pointed by cursor.
  // \pre cursor_exists: cursor!=0
  // \pre positive_level: level>=0
  void CutNode(vtkHyperOctreeCursor *cursor,
               int level,
               double bounds[6]);

  vtkImplicitFunction *CutFunction;


  vtkIncrementalPointLocator *Locator;
  int SortBy;
  vtkContourValues *ContourValues;
  int GenerateCutScalars;

  vtkHyperOctree *Input;
  vtkPolyData *Output;


  vtkCellArray *NewVerts;
  vtkCellArray *NewLines;
  vtkCellArray *NewPolys;

  vtkDataSetAttributes *InCD;
  vtkCellData *OutCD;
  vtkPointData *OutPD;
  vtkOrderedTriangulator *Triangulator;
  vtkHyperOctreeCursor *Sibling; // to avoid allocation in the loop

  int Iter; // iterate over contour values in case of VTK_SORT_BY_CELL


  vtkDoubleArray *CellScalars;
  vtkTetra *Tetra;
  vtkDoubleArray *TetScalars;

  vtkPoints *Pts;
  vtkPolygon *Polygon;

  vtkIdType CellTypeCounter[65536]; // up-to-65536 points per octant
  vtkIdType TotalCounter;
  vtkIdType TemplateCounter; // record the number of octants that succceed
  // to use the template triangulator

  // in VTK_SORT_BY_VALUE case, rejection test need to combine all values.
  int *AllLess;
  int *AllGreater;
  vtkHyperOctreeClipCutPointsGrabber *Grabber;

private:
  vtkHyperOctreeCutter(const vtkHyperOctreeCutter&);  // Not implemented.
  void operator=(const vtkHyperOctreeCutter&);  // Not implemented.
};

#endif
