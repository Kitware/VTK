/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStreamingStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkStreamingStatistics - A class for using the statistics filters
// in a streaming mode.
//
// .SECTION Description
// A class for using the statistics filters in a streaming mode or perhaps
// an "online, incremental, push" mode.
//
// .SECTION Thanks
// Thanks to the Universe for unfolding in a way that allowed this class
// to be implemented, also Godzilla for not crushing my computer.

#ifndef vtkStreamingStatistics_h
#define vtkStreamingStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkDataObjectCollection;
class vtkMultiBlockDataSet;
class vtkStatisticsAlgorithm;
class vtkTable;

class VTKFILTERSSTATISTICS_EXPORT vtkStreamingStatistics : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkStreamingStatistics, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkStreamingStatistics* New();

  // Description:
  // enumeration values to specify input port types
  enum InputPorts
    {
    INPUT_DATA = 0,         //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1,   //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2         //!< Port 2 is for a priori models
    };

  // Description:
  // enumeration values to specify output port types
  enum OutputIndices
    {
    OUTPUT_DATA  = 0,       //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1,       //!< Output 1 contains any generated model
    OUTPUT_TEST  = 2        //!< Output 2 contains result of statistical test(s)
    };

  virtual void SetStatisticsAlgorithm(vtkStatisticsAlgorithm*);

protected:
  vtkStreamingStatistics();
  ~vtkStreamingStatistics();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* );

private:
  vtkStreamingStatistics( const vtkStreamingStatistics& ); // Not implemented
  void operator = ( const vtkStreamingStatistics& );   // Not implemented

  // Internal statistics algorithm to care for and feed
  vtkStatisticsAlgorithm* StatisticsAlgorithm;

  // Internal model that gets aggregated
  vtkMultiBlockDataSet* InternalModel;
};

#endif
