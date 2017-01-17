// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSLACReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkPSLACReader
 *
 *
 *
 * Extends the vtkSLACReader to read in partitioned pieces.  Due to the nature
 * of the data layout, this reader only works in a data parallel mode where
 * each process in a parallel job simultaneously attempts to read the piece
 * corresponding to the local process id.
 *
*/

#ifndef vtkPSLACReader_h
#define vtkPSLACReader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkSLACReader.h"

class vtkMultiProcessController;

class VTKIOPARALLEL_EXPORT vtkPSLACReader : public vtkSLACReader
{
public:
  vtkTypeMacro(vtkPSLACReader, vtkSLACReader);
  static vtkPSLACReader *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The controller used to communicate partition data.  The number of pieces
   * requested must agree with the number of processes, the piece requested must
   * agree with the local process id, and all process must invoke
   * ProcessRequests of this filter simultaneously.
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *);
  //@}

protected:
  vtkPSLACReader();
  ~vtkPSLACReader() VTK_OVERRIDE;

  vtkMultiProcessController *Controller;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

  int CheckTetrahedraWinding(int meshFD) VTK_OVERRIDE;
  int ReadConnectivity(int meshFD, vtkMultiBlockDataSet *surfaceOutput,
                               vtkMultiBlockDataSet *volumeOutput) VTK_OVERRIDE;
  int ReadCoordinates(int meshFD, vtkMultiBlockDataSet *output) VTK_OVERRIDE;
  int ReadMidpointCoordinates(int meshFD, vtkMultiBlockDataSet *output,
                                      MidpointCoordinateMap &map) VTK_OVERRIDE;
  int ReadMidpointData(int meshFD, vtkMultiBlockDataSet *output,
                               MidpointIdMap &map) VTK_OVERRIDE;
  int RestoreMeshCache(vtkMultiBlockDataSet *surfaceOutput,
                               vtkMultiBlockDataSet *volumeOutput,
                               vtkMultiBlockDataSet *compositeOutput) VTK_OVERRIDE;
  int ReadFieldData(const int *modeFDArray,
                            int numModeFDs,
                            vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  int ReadTetrahedronInteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity) VTK_OVERRIDE;
  int ReadTetrahedronExteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity) VTK_OVERRIDE;

  int MeshUpToDate() VTK_OVERRIDE;

  /**
   * Reads point data arrays.  Called by ReadCoordinates and ReadFieldData.
   */
  vtkSmartPointer<vtkDataArray> ReadPointDataArray(int ncFD, int varId) VTK_OVERRIDE;

  class vtkInternal;
  vtkInternal *Internal;

  //@{
  /**
   * The number of pieces and the requested piece to load.  Synonymous with
   * the number of processes and the local process id.
   */
  int NumberOfPieces;
  int RequestedPiece;
  //@}

  /**
   * The number of points defined in the mesh file.
   */
  vtkIdType NumberOfGlobalPoints;

  /**
   * The number of midpoints defined in the mesh file
   */
  vtkIdType NumberOfGlobalMidpoints;

  //@{
  /**
   * The start/end points read by the given process.
   */
  vtkIdType StartPointRead(int process) {
    return process*(this->NumberOfGlobalPoints/this->NumberOfPieces + 1);
  }
  vtkIdType EndPointRead(int process) {
    vtkIdType result = this->StartPointRead(process+1);
    if (result > this->NumberOfGlobalPoints) result=this->NumberOfGlobalPoints;
    return result;
  }
  //@}

  //@{
  /**
   * Piece information from the last call.
   */
  int NumberOfPiecesCache;
  int RequestedPieceCache;
  //@}

private:
  vtkPSLACReader(const vtkPSLACReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPSLACReader &) VTK_DELETE_FUNCTION;
};

#endif //vtkPSLACReader_h
