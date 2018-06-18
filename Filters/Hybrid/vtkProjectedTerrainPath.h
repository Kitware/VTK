/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTerrainPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProjectedTerrainPath
 * @brief   project a polyline onto a terrain
 *
 * vtkProjectedTerrainPath projects an input polyline onto a terrain. (The
 * terrain is defined by a 2D height image and is the second input to the
 * filter.) The polyline projection is controlled via several modes as
 * follows. 1) Simple mode projects the polyline points onto the terrain,
 * taking into account the height offset instance variable. 2) Non-occluded
 * mode insures that no parts of the polyline are occluded by the terrain
 * (e.g. a line passes through a mountain). This may require recursive
 * subdivision of the polyline. 3) Hug mode insures that the polyine points
 * remain within a constant distance from the surface. This may also require
 * recursive subdivision of the polyline. Note that both non-occluded mode
 * and hug mode also take into account the height offset, so it is possible
 * to create paths that hug terrain a certain distance above it. To use this
 * filter, define two inputs: 1) a polyline, and 2) an image whose scalar
 * values represent a height field. Then specify the mode, and the height
 * offset to use.
 *
 * An description of the algorithm is as follows. The filter begins by
 * projecting the polyline points to the image (offset by the specified
 * height offset).  If the mode is non-occluded or hug, then the maximum
 * error along each line segment is computed and placed into a priority
 * queue. Each line segment is then split at the point of maximum error, and
 * the two new line segments are evaluated for maximum error. This process
 * continues until the line is not occluded by the terrain (non-occluded
 * mode) or satisfies the error on variation from the surface (hug
 * mode). (Note this process is repeated for each polyline in the
 * input. Also, the maximum error is computed in two parts: a maximum
 * positive error and maximum negative error. If the polyline is above the
 * terrain--i.e., the height offset is positive--in non-occluded or hug mode
 * all negative errors are eliminated. If the polyline is below the
 * terrain--i.e., the height offset is negative--in non-occluded or hug mode
 * all positive errors are eliminated.)
 *
 * @warning
 * This algorithm requires the entire input image to be in memory, hence it
 * may not work for extremely large images.
 *
 * @warning
 * The input height image is assumed to be positioned in the x-y plane so the
 * scalar value is the z-coordinate, height value.
 *
 * @warning
 * A priority queue is used so that the 1) the total number of line segments
 * can be controlled, and 2) the algorithm can terminate when the errors in
 * the queue are less than the specified error tolerance.
 *
 * @sa
 * vtkGreedyTerrainDecimation
*/

#ifndef vtkProjectedTerrainPath_h
#define vtkProjectedTerrainPath_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPriorityQueue;
class vtkImageData;
class vtkEdgeList;
class vtkPoints;

class VTKFILTERSHYBRID_EXPORT vtkProjectedTerrainPath : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for printing and determining type information.
   */
  vtkTypeMacro(vtkProjectedTerrainPath,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Instantiate the class.
   */
  static vtkProjectedTerrainPath* New();

  //@{
  /**
   * Specify the second input (the terrain) onto which the polyline(s) should be projected.
   * Note: This assigns a data object as the input terrain.
   * To establish a pipeline connection, use
   * SetSourceConnection() method.
   */
  void SetSourceData(vtkImageData *source);
  vtkImageData *GetSource();
  //@}

  /**
   * Specify the second input (the terrain) onto which the polyline(s) should be projected.
   * Note: vtkImageData* is required
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  enum {SIMPLE_PROJECTION=0,NONOCCLUDED_PROJECTION,HUG_PROJECTION};

  //@{
  /**
   * Determine how to control the projection process. Simple projection
   * just projects the original polyline points. Non-occluded projection
   * insures that the polyline does not intersect the terrain surface.
   * Hug projection is similar to non-occulded projection except that
   * produces a path that is nearly parallel to the terrain (within the
   * user specified height tolerance).
   */
  vtkSetClampMacro(ProjectionMode,int,SIMPLE_PROJECTION,HUG_PROJECTION);
  vtkGetMacro(ProjectionMode,int);
  void SetProjectionModeToSimple()
    {this->SetProjectionMode(SIMPLE_PROJECTION);}
  void SetProjectionModeToNonOccluded()
    {this->SetProjectionMode(NONOCCLUDED_PROJECTION);}
  void SetProjectionModeToHug()
    {this->SetProjectionMode(HUG_PROJECTION);}
  //@}

  //@{
  /**
   * This is the height above (or below) the terrain that the projected
   * path should be. Positive values indicate distances above the terrain;
   * negative values indicate distances below the terrain.
   */
  vtkSetMacro(HeightOffset,double);
  vtkGetMacro(HeightOffset,double);
  //@}

  //@{
  /**
   * This is the allowable variation in the altitude of the path
   * with respect to the variation in the terrain. It only comes
   * into play if the hug projection mode is enabled.
   */
  vtkSetClampMacro(HeightTolerance,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(HeightTolerance,double);
  //@}

  //@{
  /**
   * This instance variable can be used to limit the total number of line
   * segments created during subdivision. Note that the number of input line
   * segments will be the minimum number that cab be output.
   */
  vtkSetClampMacro(MaximumNumberOfLines,vtkIdType,1,VTK_ID_MAX);
  vtkGetMacro(MaximumNumberOfLines,vtkIdType);
  //@}

protected:
  vtkProjectedTerrainPath();
  ~vtkProjectedTerrainPath() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  // Supporting methods
  void GetImageIndex(double x[3], double loc[2], int ij[2]);
  double GetHeight(double loc[2], int ij[2]);
  void ComputeError(vtkIdType edgeId);
  void RemoveOcclusions();
  void HugTerrain();
  void SplitEdge(vtkIdType eId, double t);

  //ivars that the API addresses
  int       ProjectionMode;
  double    HeightOffset;
  double    HeightTolerance;
  vtkIdType MaximumNumberOfLines;

  //Bookkeeping arrays
  int          Dimensions[3];
  int          Extent[6];
  double       Origin[3];
  double       Spacing[3];
  vtkDataArray *Heights;
  vtkPoints    *Points;
  vtkIdType    NumLines;

  //Errors above/below terrain. In both instances, negative values are
  //inserted because the priority queue puts smallest values on top.
  vtkPriorityQueue *PositiveLineError; //errors above terrain
  vtkPriorityQueue *NegativeLineError; //errors below terrain

  //This is a PIMPL'd vector representing edges
  vtkEdgeList *EdgeList;

private:
  vtkProjectedTerrainPath(const vtkProjectedTerrainPath&) = delete;
  void operator=(const vtkProjectedTerrainPath&) = delete;

};

#endif
