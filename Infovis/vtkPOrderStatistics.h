/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPOrderStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkPOrderStatistics - A class for parallel univariate order statistics
// .SECTION Description
// vtkPOrderStatistics is vtkOrderStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each
// individual data points on the node that owns it.

// .NOTE: It is assumed that the keys in the histogram table be contained in the set {0,...,n-1}
// of successive integers, where n is the number of rows of the summary table.
// If this requirement is not fulfilled, then the outcome of the parallel update of order
// tables is unpredictable but will most likely be a crash.
// Note that this requirement is consistent with the way histogram tables are constructed
// by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
// then you do not have to worry about this requirement.

// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.

#ifndef __vtkPOrderStatistics_h
#define __vtkPOrderStatistics_h

#include "vtkOrderStatistics.h"

//BTX
#include <map> // STL Header
//ETX

class vtkIdTypeArray;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTK_INFOVIS_EXPORT vtkPOrderStatistics : public vtkOrderStatistics
{
 public:
  static vtkPOrderStatistics* New();
  vtkTypeMacro(vtkPOrderStatistics, vtkOrderStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Execute the parallel calculations required by the Learn option.
  virtual void Learn( vtkTable*,
                      vtkTable*,
                      vtkMultiBlockDataSet* );

 protected:
  vtkPOrderStatistics();
  ~vtkPOrderStatistics();

//BTX
  // Description:
  // Reduce the collection of local histograms to the global one for data inputs
  bool Reduce( vtkIdTypeArray*,
               vtkDataArray* );

  // Description:
  // Reduce the collection of local histograms to the global one for string inputs
  bool Reduce( vtkIdTypeArray*,
               vtkIdType&,
               char*,
               std::map<vtkStdString,vtkIdType>& );

  // Description:
  // Broadcast reduced histogram to all processes in the case of string inputs
  bool Broadcast( std::map<vtkStdString,vtkIdType>&,
                  vtkIdTypeArray*,
                  vtkStringArray*,
                  vtkIdType );
//ETX

  vtkMultiProcessController* Controller;
 private:
  vtkPOrderStatistics(const vtkPOrderStatistics&); // Not implemented.
  void operator=(const vtkPOrderStatistics&); // Not implemented.
};

#endif
