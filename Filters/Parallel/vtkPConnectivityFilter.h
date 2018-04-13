/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPConnectivityFilter
 * @brief   Parallel version of vtkConnectivityFilter
 *
 * This class computes connectivity of a distributed data set in parallel.
 *
 * Problem
 * =======
 *
 * Datasets are distributed among ranks in a distributed process (Figure 1).
 * vtkConnectivityFilter already runs in parallel on each piece in a typical
 * VTK application run with MPI, but it does not produce correct results. As
 * Figure 2 shows, distributed pieces of each connected component may end up
 * with different labels.
 *
 * ![Figure 1: Pieces in a distributed data set colored by processor
 * rank.](vtkPConnectivityFilterFigure1.png)
 *
 * ![Figure 2: Left). Incorrect parallel labeling yields three regions instead of
 * two, and contiguous regions have more than one region instead of one. Right).
 * Desired correct labeling.](vtkPConnectivityFilterFigure2.png)
 *
 * The part missing from a fully parallel connectivity filter implementation is
 * the identification of which pieces on different ranks are actually connected.
 * This parallel filter provides that missing piece.
 *
 * Approach
 * ========
 *
 * Run vtkConnectivityFilter on each rank’s piece and resolve the connected
 * pieces afterwards. The implementation uses vtkMultiProcessController to
 * communicate among processes.
 *
 * Input Requirements
 * ==================
 *
 * The vtkPConnectivityFilter requires Ghost Points, which can be generated with
 * the vtkPUnstructuredGridGhostCellsGenerator or vtkDistributedDataFilter.
 *
 * Steps in the vtkPConnectivityFilter
 * -----------------------------------
 *
 * ### High-level steps
 *
 * + Run local connectivity algorithm.
 *
 * + Identify region connections across ranks and create a graph of these links.
 *
 * + Run a connected components algorithm on the graph created in the previous
 *   step to unify the regions across ranks.
 *
 * + Relabel cells and points with their “global” RegionIds.
 *
 * ### Low-level steps
 *
 * + In GenerateData(), invoke the superclass’s GenerateData() method. Make
 * temporary changes to extract all connected regions - we’ll handle the
 * different extraction modes at the end. Example results on 3 ranks are shown
 * in Figure 3 where color indicates RegionId computed by vtkConnectivityFilter.
 *
 * + Check for errors in superclass GenerateData() on any rank and exit the
 * algorithm if any encountered an error-indicating return code.
 *
 * ![Figure 3: Results after vtkConnectivityFilter superclass is called on each
 * piece.](vtkPConnectivityFilterFigure3.png)
 *
 * + AllGather the number of connected RegionIds from each rank and AllGatherv
 * the RegionIds themselves. (Optimization opportunity: We can skip the
 * AllGatherv of RegionIds if they are guaranteed to be contiguous on each
 * rank.)
 *
 * ![Figure 4: Ghost point and associated RegionId
 * exchange.](vtkPConnectivityFilterFigure4.png)
 *
 * + Each rank gathers up ghost points and sends them to all other ranks as well
 * as the RegionId assigned to each point. (Optimization opportunity: We can do
 * a bounding box optimization to limit the sending of ghost points only to
 * potential neighbors.)
 *
 * + Each rank runs through the ghost points and determines which points it owns
 * using a locator object. If a point is found on the local rank, add the
 * RegionId from the remote ghost point to a set associated with the local
 * RegionId. This signifies that the local RegionId is connected to the remote
 * RegionId associated with the ghost point (Optimization opportunity: Instead
 * of using a locator object on the entire dataset piece on the rank, extract
 * the surface and create a locator for the surface instead).
 *
 * + Each rank gathers the local-RegionId-to-remote-RegionId links from all
 * other ranks.
 *
 * + From these links, each rank generates a graph structure of the global
 * links. The graph structure is identical on all ranks. (Optimization
 * opportunity: To reduce communication, this link exchange could be avoided and
 * the graph could be made distributed. This is just more complicated to
 * program, however).
 *
 * ![Figure 5: Connected region graph depicted by black line
 * segments.](vtkPConnectivityFilterFigure5.png)
 *
 * + Run a connected components algorithm that relabels the RegionIds, yielding
 * the full connectivity graph across ranks. Figure 6 shows an example result.
 *
 * + Relabel the remaining RegionIds by a contiguous set of RegionIds (e.g., go
 * from [0, 5, 8, 9] to [0, 1, 2, 3]).
 *
 * ![Figure 6: Connected components of graph linking RegionIds across
 * ranks.](vtkPConnectivityFilterFigure6.png)
 *
 * + From the RegionId graph, relabel points and cells in the output. The result
 * is shown in Figure 7.
 *
 * ![Figure 7: Dataset relabeled with global connected
 * RegionIds.](vtkPConnectivityFilterFigure7.png)
 *
 * + Handle ScalarConnectivy option and ExtractionMode after full region
 * connectivity is determined by identifying the correct RegionId and extracting
 * it by thresholding.
 *
 * Caveats
 * =======
 *
 * This parallel implementation does not support a number of features that the
 * vtkConnectivityFilter class supports, including:
 *
 *   - ScalarConnectivity
 *   - VTK_EXTRACT_POINT_SEEDED_REGIONS extraction mode
 *   - VTK_EXTRACT_CELL_SEEDED_REGIONS extraction mode
 *   - VTK_EXTRACT_SPECIFIED_REGIONS extraction mode
 */

#ifndef vtkPConnectivityFilter_h
#define vtkPConnectivityFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkConnectivityFilter.h"

class VTKFILTERSPARALLEL_EXPORT vtkPConnectivityFilter : public vtkConnectivityFilter
{
public:
  vtkTypeMacro(vtkPConnectivityFilter,vtkConnectivityFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPConnectivityFilter *New();

protected:
  vtkPConnectivityFilter();
  ~vtkPConnectivityFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkPConnectivityFilter(const vtkPConnectivityFilter&) = delete;
  void operator=(const vtkPConnectivityFilter&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
