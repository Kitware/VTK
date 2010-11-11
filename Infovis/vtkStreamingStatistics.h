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

#ifndef __vtkStreamingStatistics_h
#define __vtkStreamingStatistics_h

#include "vtkStatisticsAlgorithm.h"
#include "vtkSetGet.h"
class vtkDataObjectCollection;
class vtkMultiBlockDataSet;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkStreamingStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkStreamingStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkStreamingStatistics* New();

  virtual void SetStatisticsAlgorithm(vtkStatisticsAlgorithm*);

protected:
  vtkStreamingStatistics();
  ~vtkStreamingStatistics();

  // Description:
  // Placeholder for abstract method
  virtual void Aggregate( vtkDataObjectCollection*, vtkMultiBlockDataSet* ) {};

  // Description:
  // Placeholder for abstract method
  virtual void Learn(vtkTable*, vtkTable*, vtkMultiBlockDataSet*) {};

  // Description:
  // Placeholder for abstract method
  virtual void Derive(vtkMultiBlockDataSet*) {};

  // Description:
  // Placeholder for abstract method
  virtual void Assess(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) {};

  // Description:
  // Placeholder for abstract method
  virtual void Test(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) {};

  //BTX
  // Description:
  // Placeholder for abstract method
  virtual void SelectAssessFunctor( vtkTable* vtkNotUsed(outData),
                                    vtkDataObject* vtkNotUsed(inMeta),
                                    vtkStringArray* vtkNotUsed(rowNames),
                                    AssessFunctor*& vtkNotUsed(dfunc)) {};
  //ETX

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* );

private:
  vtkStreamingStatistics( const vtkStreamingStatistics& ); // Not implemented
  void operator = ( const vtkStreamingStatistics& );   // Not implemented

  // Internal statistics algorithm to care for and feed
  vtkStatisticsAlgorithm *StatisticsAlgorithm;

  // Internal model that gets aggregated
  vtkMultiBlockDataSet *internalModel;
};

#endif
