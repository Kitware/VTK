/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkCollectGraph
 * @brief   Collect distributed graph.
 *
 * This filter has code to collect a graph from across processes onto vertex 0.
 * Collection can be turned on or off using the "PassThrough" flag.
*/

#ifndef vtkCollectGraph_h
#define vtkCollectGraph_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkMultiProcessController;
class vtkSocketController;

class VTKFILTERSPARALLEL_EXPORT vtkCollectGraph : public vtkGraphAlgorithm
{
public:
  static vtkCollectGraph *New();
  vtkTypeMacro(vtkCollectGraph, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * By defualt this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * When this filter is being used in client-server mode,
   * this is the controller used to communicate between
   * client and server.  Client should not set the other controller.
   */
  virtual void SetSocketController(vtkSocketController*);
  vtkGetObjectMacro(SocketController, vtkSocketController);
  //@}

  //@{
  /**
   * To collect or just copy input to output. Off (collect) by default.
   */
  vtkSetMacro(PassThrough, int);
  vtkGetMacro(PassThrough, int);
  vtkBooleanMacro(PassThrough, int);
  //@}

  enum {
    DIRECTED_OUTPUT,
    UNDIRECTED_OUTPUT,
    USE_INPUT_TYPE
  };

  //@{
  /**
   * Directedness flag, used to signal whether the output graph is directed or undirected.
   * DIRECTED_OUTPUT expects that this filter is generating a directed graph.
   * UNDIRECTED_OUTPUT expects that this filter is generating an undirected graph.
   * DIRECTED_OUTPUT and UNDIRECTED_OUTPUT flags should only be set on the client
   * filter.  Server filters should be set to USE_INPUT_TYPE since they have valid
   * input and the directedness is determined from the input type.
   */
  vtkSetMacro(OutputType, int);
  vtkGetMacro(OutputType, int);
  //@}

protected:
  vtkCollectGraph();
  ~vtkCollectGraph();

  int PassThrough;
  int OutputType;

  // Data generation method
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkMultiProcessController *Controller;
  vtkSocketController *SocketController;

private:
  vtkCollectGraph(const vtkCollectGraph&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCollectGraph&) VTK_DELETE_FUNCTION;
};

#endif
