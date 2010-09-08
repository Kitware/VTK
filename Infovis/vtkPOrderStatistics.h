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
#include <vtkstd/vector> // STL Header
//ETX

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
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

 protected:
  vtkPOrderStatistics();
  ~vtkPOrderStatistics();

//BTX
  // Description:
  // Pack all entries of a order table in:
  // 1. a single string for all realizations of variables, and
  // 2. a single vector for the corresponding keys and cardinalities
  bool Pack( vtkTable* orderTab,
             vtkStdString& xPacked,
             vtkstd::vector<vtkIdType>& kcValues );

  // Description:
  // Reduce the collection of local order tables to the global one
  bool Reduce( vtkIdType& xSizeTotal,
               char* xPacked_g,
               vtkStdString& xPacked_l,
               vtkIdType& kcSizeTotal,
               vtkIdType*  kcValues_g,
               vtkstd::vector<vtkIdType>& kcValues_l );

  // Description:
  // Broadcast reduced order table to all processes
  bool Broadcast( vtkIdType xSizeTotal,
                  vtkStdString& xPacked,
                  vtkstd::vector<vtkStdString>& xValues,
                  vtkIdType kcSizeTotal,
                  vtkstd::vector<vtkIdType>& kcValues,
                  vtkIdType reduceProc );
//ETX

  vtkMultiProcessController* Controller;
 private:
  vtkPOrderStatistics(const vtkPOrderStatistics&); // Not implemented.
  void operator=(const vtkPOrderStatistics&); // Not implemented.
};

#endif
