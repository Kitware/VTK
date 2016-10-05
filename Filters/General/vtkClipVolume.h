/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClipVolume
 * @brief   clip volume data with user-specified implicit function or input scalar data
 *
 * vtkClipVolume is a filter that clips volume data (i.e., vtkImageData)
 * using either: any subclass of vtkImplicitFunction or the input scalar
 * data. The clipping operation cuts through the cells of the
 * dataset--converting 3D image data into a 3D unstructured grid--returning
 * everything inside of the specified implicit function (or greater than the
 * scalar value). During the clipping the filter will produce pieces of a
 * cell. (Compare this with vtkExtractGeometry or vtkGeometryFilter, which
 * produces entire, uncut cells.) The output of this filter is a 3D
 * unstructured grid (e.g., tetrahedra or other 3D cell types).
 *
 * To use this filter, you must decide if you will be clipping with an
 * implicit function, or whether you will be using the input scalar data.  If
 * you want to clip with an implicit function, you must first define and then
 * set the implicit function with the SetClipFunction() method. Otherwise,
 * you must make sure input scalar data is available. You can also specify a
 * scalar value, which is used to decide what is inside and outside of the
 * implicit function. You can also reverse the sense of what inside/outside
 * is by setting the InsideOut instance variable. (The cutting algorithm
 * proceeds by computing an implicit function value or using the input scalar
 * data for each point in the dataset. This is compared to the scalar value
 * to determine inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the portion of the volume that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * The filter will produce an unstructured grid of entirely tetrahedra or a
 * mixed grid of tetrahedra and other 3D cell types (e.g., wedges). Control
 * this behavior by setting the Mixed3DCellGeneration. By default the
 * Mixed3DCellGeneration is on and a combination of cell types will be
 * produced. Note that producing mixed cell types is a faster than producing
 * only tetrahedra.
 *
 * @warning
 * This filter is designed to function with 3D structured points. Clipping
 * 2D images should be done by converting the image to polygonal data
 * and using vtkClipPolyData,
 *
 * @sa
 * vtkImplicitFunction vtkClipPolyData vtkGeometryFilter vtkExtractGeometry
*/

#ifndef vtkClipVolume_h
#define vtkClipVolume_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCellData;
class vtkDataArray;
class vtkIdList;
class vtkImplicitFunction;
class vtkMergePoints;
class vtkOrderedTriangulator;
class vtkPointData;
class vtkIncrementalPointLocator;
class vtkPoints;
class vtkUnstructuredGrid;
class vtkCell;
class vtkTetra;
class vtkCellArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;

class VTKFILTERSGENERAL_EXPORT vtkClipVolume : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkClipVolume,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; and generate clip scalars turned off.
   */
  static vtkClipVolume *New();

  //@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with scalars). The
   * default value is 0.0.
   */
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag. When off, a vertex is considered inside the
   * implicit function if its value is greater than the Value ivar. When
   * InsideOutside is turned on, a vertex is considered inside the implicit
   * function if its implicit function value is less than or equal to the
   * Value ivar.  InsideOut is off by default.
   */
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);
  //@}

  //@{
  /**
   * Specify the implicit function with which to perform the clipping. If you
   * do not define an implicit function, then the input scalar data will be
   * used for clipping.
   */
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the
   * input scalar data. If you enable this flag but do not provide an
   * implicit function an error will be reported.
   */
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);
  //@}

  //@{
  /**
   * Control whether a second output is generated. The second output
   * contains the unstructured grid that's been clipped away.
   */
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);
  //@}

  /**
   * Return the clipped output.
   */
  vtkUnstructuredGrid *GetClippedOutput();

  //@{
  /**
   * Control whether the filter produces a mix of 3D cell types on output, or
   * whether the output cells are all tetrahedra. By default, a mixed set of
   * cells (e.g., tetrahedra and wedges) is produced. (Note: mixed type
   * generation is faster and less overall data is generated.)
   */
  vtkSetMacro(Mixed3DCellGeneration,int);
  vtkGetMacro(Mixed3DCellGeneration,int);
  vtkBooleanMacro(Mixed3DCellGeneration,int);
  //@}

  //@{
  /**
   * Set the tolerance for merging clip intersection points that are near
   * the corners of voxels. This tolerance is used to prevent the generation
   * of degenerate tetrahedra.
   */
  vtkSetClampMacro(MergeTolerance,double,0.0001,0.25);
  vtkGetMacro(MergeTolerance,double);
  //@}

  //@{
  /**
   * Set / Get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator and clip function.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkClipVolume(vtkImplicitFunction *cf=NULL);
  ~vtkClipVolume() VTK_OVERRIDE;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  void ClipTets(double value, vtkTetra *clipTetra, vtkDataArray *clipScalars,
                vtkDataArray *cellScalars, vtkIdList *tetraIds,
                vtkPoints *tetraPts, vtkPointData *inPD, vtkPointData *outPD,
                vtkCellData *inCD, vtkIdType cellId, vtkCellData *outCD,
                vtkCellData *clippedCD, int insideOut);
  void ClipVoxel(double value, vtkDataArray *cellScalars, int flip,
                 double origin[3], double spacing[3], vtkIdList *cellIds,
                 vtkPoints *cellPts, vtkPointData *inPD, vtkPointData *outPD,
                 vtkCellData *inCD, vtkIdType cellId, vtkCellData *outCD,
                 vtkCellData *clippedCD);

  vtkImplicitFunction *ClipFunction;
  vtkIncrementalPointLocator     *Locator;
  int                  InsideOut;
  double                Value;
  int                  GenerateClipScalars;
  double                MergeTolerance;
  int                  Mixed3DCellGeneration;
  int                  GenerateClippedOutput;
  vtkUnstructuredGrid *ClippedOutput;

private:
  vtkOrderedTriangulator *Triangulator;

  // Used temporarily to pass data around
  vtkIdType             NumberOfCells;
  vtkCellArray          *Connectivity;
  vtkUnsignedCharArray  *Types;
  vtkIdTypeArray        *Locations;
  vtkIdType             NumberOfClippedCells;
  vtkCellArray          *ClippedConnectivity;
  vtkUnsignedCharArray  *ClippedTypes;
  vtkIdTypeArray        *ClippedLocations;

private:
  vtkClipVolume(const vtkClipVolume&) VTK_DELETE_FUNCTION;
  void operator=(const vtkClipVolume&) VTK_DELETE_FUNCTION;
};

#endif
