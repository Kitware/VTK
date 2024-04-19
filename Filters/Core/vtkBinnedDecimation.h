// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBinnedDecimation
 * @brief   reduce the number of triangles in a vtkPolyData mesh
 *
 * vtkBinnedDecimation is a filter to reduce the number of triangles in a
 * triangle mesh represented by vtkPolyData. It is similar to
 * vtkQuadricClustering in concept, although it is performance accelerated:
 * it does not use quadric error metrics to position points in the bins, plus
 * it is threaded. (See vtkQuadricClustering for more information.) It also
 * takes some short cuts in the interest of speed: it limits the binning
 * resolution to no more than 2^31 bins; and it can (optionally) reuse the input
 * points in the output (to save memory and computational costs).
 *
 * A high-level overview of the algorithm is as follows. Points are binned
 * into a regular grid subdivided in the x-y-z directions. The idea is to
 * combine all the points within each bin into a single point which is then
 * used by the output triangles. Four options are available to generate the
 * output points. If the input points are to be reused as the output points,
 * then all points in the same bin simply adopt the coordinates of one of the
 * selected points in the bin (and thus all points in the bin take on the
 * same output point id). Alternatively, if new output points are to be
 * generated, then either one point is selected; the centers of occupied bins
 * can be used as the output point coordinates; or an average position of all
 * points falling into the bin can be used to generate the bin
 * point. Finally, triangles are inserted into the output: triangles whose
 * three, binned points lie in separate bins are sent to the output, while
 * all others are discarded (i.e., triangles with two or more points in the
 * same bin are not sent to the output).
 *
 * To use this filter, specify the divisions defining the spatial subdivision
 * in the x, y, and z directions. Of course you must also specify an input
 * vtkPolyData / filter connection. Higher division levels generally produce
 * results closer to the original mesh. Note that for performance reasons
 * (i.e., related to memory), the maximum divisions in the x-y-z directions
 * is limited in such a way (i.e., proportional scaling of divisions is used)
 * so as to ensure that no more than 2^31 bins are used. Higher divisions have
 * modest impact on the overall performance of the algorithm, although the
 * resolution of the output vtkPolyData is affected significantly (i.e., many
 * more triangles may be generated).
 *
 * @warning
 * This filter can drastically affect mesh topology, i.e., topology is not
 * preserved.
 *
 * @warning
 * This filter and vtkQuadricClustering produce similar results, with
 * vtkQuadricClustering theoretically producing better results. In practice
 * however, vtkBinnedDecimation produces results that are visually close to
 * vtkQuadricClustering at speeds approaching 10-100x faster (depending on
 * the bin divisions, and how the output points are generated), and the
 * algorithm requires much less memory. Note that the API of this filter is a
 * subset of vtkQuadricClustering and can often be used interchangeably with
 * vtkQuadricClustering.
 *
 * @warning
 * Algorithm 4) BIN_CENTERS uses a very different implementation strategy
 * requiring a sort of all points. It scales better as the number of bins
 * increases.
 *
 * @warning
 * For certain types of geometry (e.g., a mostly 2D plane with jitter in the
 * normal direction), this decimator can perform badly. In this situation,
 * set the number of bins in the normal direction to one.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential execution type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkQuadricClustering vtkDecimatePro vtkDecimate vtkQuadricLODActor
 * vtkTriangleFilter
 */

#ifndef vtkBinnedDecimation_h
#define vtkBinnedDecimation_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkBinnedDecimation : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard instantiation, type and print methods.
   */
  static vtkBinnedDecimation* New();
  vtkTypeMacro(vtkBinnedDecimation, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the number of divisions along each axis for the spatial bins.
   * The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
   * NumberOfZDivisions. The filter may choose to ignore large numbers of
   * divisions if the input has few points and AutoAdjustNumberOfDivisions is
   * enabled. Also, the maximum number of divisions is controlled so that no
   * more than 2^31 bins are created. (If bin adjustment due to the limit on
   * the number of bins is necessary, then a proportional scaling of the
   * divisions in the x-y-z directions is used.) This API has been adopted
   * to be consistent with vtkQuadricClustering.
   */
  void SetNumberOfXDivisions(int num);
  void SetNumberOfYDivisions(int num);
  void SetNumberOfZDivisions(int num);
  vtkGetMacro(NumberOfXDivisions, int);
  vtkGetMacro(NumberOfYDivisions, int);
  vtkGetMacro(NumberOfZDivisions, int);
  void SetNumberOfDivisions(int div[3]) { this->SetNumberOfDivisions(div[0], div[1], div[2]); }
  void SetNumberOfDivisions(int div0, int div1, int div2);
  int* GetNumberOfDivisions() VTK_SIZEHINT(3);
  void GetNumberOfDivisions(int div[3]);
  ///@}

  ///@{
  /**
   * Enable automatic adjustment of number of divisions. If disabled, the
   * number of divisions specified by the user is always used (as long as it
   * is valid). The default is On.
   */
  vtkSetMacro(AutoAdjustNumberOfDivisions, bool);
  vtkGetMacro(AutoAdjustNumberOfDivisions, bool);
  vtkBooleanMacro(AutoAdjustNumberOfDivisions, bool);
  ///@}

  ///@{
  /**
   * This is an alternative way to set up the bins.  If you are trying to
   * match boundaries between pieces, then you should use these methods
   * rather than SetNumberOfDivisions(). To use these methods, specify the
   * origin and spacing of the spatial binning.
   */
  void SetDivisionOrigin(double x, double y, double z);
  void SetDivisionOrigin(double o[3]) { this->SetDivisionOrigin(o[0], o[1], o[2]); }
  vtkGetVector3Macro(DivisionOrigin, double);
  void SetDivisionSpacing(double x, double y, double z);
  void SetDivisionSpacing(double s[3]) { this->SetDivisionSpacing(s[0], s[1], s[2]); }
  vtkGetVector3Macro(DivisionSpacing, double);
  ///@}

  ///@{
  /**
   * Four options exist for generating output points. 1) Pass the input
   * points through to the output; 2) select one of the input points in the bin and
   * use that; 3) generate new points at the center of bins (only center bin
   * points used by the output triangles are generated); and 4) generate new
   * points from the average of all points falling into a bin and used by
   * output triangles. Note that 1) can result in many, unused output points,
   * but tends to be fastest for small numbers of bins. This can impact
   * rendering memory usage as all points are typically pushed into the
   * graphics hardware. Options 2-4 produce only points used by the output
   * triangles but generally take longer (for small numbers of bins), with
   * speeds slowing in order from options 2 through 4. In terms of quality,
   * option 4 (BIN_AVERAGES) produces the best output; options 1) and 2)
   * produce decent output, with option 3) (BIN_CENTERS) producing a
   * voxelized-like result (which is quite useful for illustrative purposes).
   * Note that for very large numbers of bins (say number of divisions > 500^3),
   * then algorithm 4) BIN_AVERAGES scales better, i.e., is likely faster and
   * produces better results.
   */
  enum
  {
    INPUT_POINTS = 1,
    BIN_POINTS = 2,
    BIN_CENTERS = 3,
    BIN_AVERAGES = 4
  };
  vtkSetClampMacro(PointGenerationMode, int, INPUT_POINTS, BIN_AVERAGES);
  vtkGetMacro(PointGenerationMode, int);
  void SetPointGenerationModeToUseInputPoints() { this->SetPointGenerationMode(INPUT_POINTS); }
  void SetPointGenerationModeToBinPoints() { this->SetPointGenerationMode(BIN_POINTS); }
  void SetPointGenerationModeToBinCenters() { this->SetPointGenerationMode(BIN_CENTERS); }
  void SetPointGenerationModeToBinAverages() { this->SetPointGenerationMode(BIN_AVERAGES); }
  ///@}

  ///@{
  /**
   * This flag directs the filter to produce output point data from the input
   * point data (on by default). If the ProducePointData is set to
   * INPUT_POINTS, point data is simply passed from input to output (since
   * the points don't change). If the point generation mode is set to
   * BIN_AVERAGES, then the average of all point data values within a bin
   * are associated with the point generated in the bin. If the point
   * generation mode is either BIN_POINTS or BIN_CENTERS, then the point
   * data values from one of the points falling into the bin is used.
   */
  vtkSetMacro(ProducePointData, bool);
  vtkGetMacro(ProducePointData, bool);
  vtkBooleanMacro(ProducePointData, bool);
  ///@}

  ///@{
  /**
   * This flag directs the filter to copy cell data from input to output.
   * This flag is off by default.
   */
  vtkSetMacro(ProduceCellData, bool);
  vtkGetMacro(ProduceCellData, bool);
  vtkBooleanMacro(ProduceCellData, bool);

  ///@}
  /**
   * Return a flag indicating whether large ids were used during
   * execution. The value of this flag is only valid after filter
   * execution. The filter may use a smaller id type unless it must use
   * vtkIdType to represent points and cell ids.
   */
  bool GetLargeIds() { return this->LargeIds; }

protected:
  vtkBinnedDecimation();
  ~vtkBinnedDecimation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int NumberOfXDivisions;
  int NumberOfYDivisions;
  int NumberOfZDivisions;
  int NumberOfDivisions[3];

  // Since there are two ways of specifying the grid, we the the flag below
  // to indicate which the user has set.  When this flag is on, the bin sizes
  // are computed from the DivisionOrigin and DivisionSpacing.
  int ComputeNumberOfDivisions;

  bool AutoAdjustNumberOfDivisions;
  double DivisionOrigin[3];
  double DivisionSpacing[3];
  double Bounds[6];

  int PointGenerationMode;
  bool ProducePointData;
  bool ProduceCellData;
  bool LargeIds;

  // Helper function
  void ConfigureBinning(vtkPolyData* input, vtkIdType numPts);

private:
  vtkBinnedDecimation(const vtkBinnedDecimation&) = delete;
  void operator=(const vtkBinnedDecimation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
