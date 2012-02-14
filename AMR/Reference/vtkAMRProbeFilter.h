/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRProbeFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRProbeFilter.h -- Probes the user supplied set of points and
// returns only the blocks that contain these points.
//
// .SECTION Description
// A concrete instance of the vtkMultiBlockDataSetAlgorithm that provides
// functionality to probe and return the blocks that contain the user-supplied
// set of points(or particles). The filter accepts two inputs. The first input
// consists of the AMR dataset to be probed, i.e., an instance of the
// vtkHierarchicalBoxDataSet. The second input consists of the set of points
// to be probed. The output is a multi-block dataset consisting of all the
// blocks that contain the given input points. Note, for each point only the
// highest resolution block that contains the point is chosen.

#ifndef VTKAMRPROBEFILTER_H_
#define VTKAMRPROBEFILTER_H_

#include "vtkMultiBlockDataSetAlgorithm.h"
#include <set> // For C++ STL set

class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkPointSet;
class vtkInformation;
class vtkIndent;
class vtkInformationVector;

class VTK_AMR_EXPORT vtkAMRProbeFilter : public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRProbeFilter *New();
    vtkTypeMacro(vtkAMRProbeFilter,vtkMultiBlockDataSetAlgorithm);
    void PrintSelf( std::ostream& oss, vtkIndent indent);

    // Description:
    // Sets the AMR dataset
    void SetAMRDataSet( vtkHierarchicalBoxDataSet *amrds );

    // Description:
    // Sets the probe points
    void SetProbePoints( vtkPointSet *probes );

  protected:
    vtkAMRProbeFilter();
    virtual ~vtkAMRProbeFilter();

    // Description:
    // Extracts the AMR blocks into a multi-block dataset instance.
    void ExtractAMRBlocks(
      vtkMultiBlockDataSet *mbds,
      vtkHierarchicalBoxDataSet *amrds,
      std::set< unsigned int > &blocksToExtract );

    // Description:
    // Determines if a point is within a block at a given level
    bool FindPointInLevel(
        double x, double y, double z,
        int levelIdx, vtkHierarchicalBoxDataSet *amrds, int &blockId );

    // Description:
    // Determines if a point is within an AMR block given the x,y,z coordinates
    // and the level and ID of the target block.
    bool PointInAMRBlock(
        double x, double y, double z,
        int levelIdx, int blockIdx, vtkHierarchicalBoxDataSet *amrds );

    // Description:
    // Given a user-supplied pointset, this method finds the blocks
    // that contain these points from the input AMR dataset, amrds,
    // and stores them to the output block dataset.
    void ProbeAMR(
        vtkPointSet *probes, vtkHierarchicalBoxDataSet *amrds,
        vtkMultiBlockDataSet *mbds );

    // Standard pipeline routines
    virtual int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

  private:
    vtkAMRProbeFilter( const vtkAMRProbeFilter&); // Not implemented
    void operator=(const vtkAMRProbeFilter&); // Not implemented
};

#endif /* VTKAMRPROBEFILTER_H_ */
