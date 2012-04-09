/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTable.h

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
// .NAME vtkThresholdTable - Thresholds table rows.
//
// .SECTION Description
// vtkThresholdTable uses minimum and/or maximum values to threshold
// table rows based on the values in a particular column.
// The column to threshold is specified using SetInputArrayToProcess(0, ...).

#ifndef __vtkThresholdTable_h
#define __vtkThresholdTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include "vtkVariant.h" // For vtkVariant arguments

class VTKINFOVISCORE_EXPORT vtkThresholdTable : public vtkTableAlgorithm
{
public:
  static vtkThresholdTable* New();
  vtkTypeMacro(vtkThresholdTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum {
    ACCEPT_LESS_THAN = 0,
    ACCEPT_GREATER_THAN = 1,
    ACCEPT_BETWEEN = 2,
    ACCEPT_OUTSIDE = 3
    };
  //ETX

  // Description:
  // The mode of the threshold filter.  Options are:
  // ACCEPT_LESS_THAN (0) accepts rows with values < MaxValue;
  // ACCEPT_GREATER_THAN (1) accepts rows with values > MinValue;
  // ACCEPT_BETWEEN (2) accepts rows with values > MinValue and < MaxValue;
  // ACCEPT_OUTSIDE (3) accepts rows with values < MinValue or > MaxValue.
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);

  // Description:
  // The minimum value for the threshold.
  // This may be any data type stored in a vtkVariant.
  virtual void SetMinValue(vtkVariant v)
    {
    this->MinValue = v;
    this->Modified();
    }
  virtual vtkVariant GetMinValue()
    {
    return this->MinValue;
    }

  // Description:
  // The maximum value for the threshold.
  // This may be any data type stored in a vtkVariant.
  virtual void SetMaxValue(vtkVariant v)
    {
    this->MaxValue = v;
    this->Modified();
    }
  virtual vtkVariant GetMaxValue()
    {
    return this->MaxValue;
    }

  // Description:
  // Criterion is rows whose scalars are between lower and upper thresholds
  // (inclusive of the end values).
  void ThresholdBetween(vtkVariant lower, vtkVariant upper);

  // Description:
  // The minimum value for the threshold as a double.
  void SetMinValue(double v)
  {
    this->SetMinValue(vtkVariant(v));
  }

  // Description:
  // The maximum value for the threshold as a double.
  void SetMaxValue(double v)
  {
    this->SetMaxValue(vtkVariant(v));
  }

  // Description:
  // Criterion is rows whose scalars are between lower and upper thresholds
  // (inclusive of the end values).
  void ThresholdBetween(double lower, double upper)
  {
    this->ThresholdBetween(vtkVariant(lower),vtkVariant(upper));
  }

protected:
  vtkThresholdTable();
  ~vtkThresholdTable();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkVariant MinValue;
  vtkVariant MaxValue;
  int Mode;

private:
  vtkThresholdTable(const vtkThresholdTable&); // Not implemented
  void operator=(const vtkThresholdTable&);   // Not implemented
};

#endif

