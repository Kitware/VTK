/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesDomain.h

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

/// \file vtkQtChartSeriesDomain.h
/// \date March 3, 2008

#ifndef _vtkQtChartSeriesDomain_h
#define _vtkQtChartSeriesDomain_h

#include "vtkQtChartExport.h"

class vtkQtChartAxisDomain;
class vtkQtChartSeriesDomainInternal;


class VTKQTCHART_EXPORT vtkQtChartSeriesDomain
{
public:
  vtkQtChartSeriesDomain();
  vtkQtChartSeriesDomain(const vtkQtChartSeriesDomain &other);
  ~vtkQtChartSeriesDomain();

  const vtkQtChartAxisDomain &getXDomain() const;
  vtkQtChartAxisDomain &getXDomain();

  const vtkQtChartAxisDomain &getYDomain() const;
  vtkQtChartAxisDomain &getYDomain();

  vtkQtChartSeriesDomain &operator=(const vtkQtChartSeriesDomain &other);

private:
  vtkQtChartSeriesDomainInternal *Internal;
};

#endif
