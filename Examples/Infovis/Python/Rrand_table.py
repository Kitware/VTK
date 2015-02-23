#!/usr/bin/env python

# Python example script that uses the Gnu R random table source to create an output
# table with 5 columns and 30 rows.  The columns are filled random numbers drawn
# from a Normal, Poisson, Chi-square, Uniform, and Binomial distribution, respecively.

# VTK must be built with VTK_USE_GNU_R turned on for this example to work!

from vtk import *

if __name__ == "__main__":

  # Define parameters for each distribution. Integers for each distribution type are
  # defined in the C++ header file VTK/Graphics/vtkRRandomTableSource.h

  NORMAL = 17
  mean_nd = 5.0
  sd_nd = 2.5

  POISSON = 4
  lambda_pd = 3.0

  CHISQUARE = 18
  k_csd = 3.0

  UNIFORM = 14
  lb_ud = 5.0
  ub_ud = 100.0

  BINOMIAL = 16
  nt_bd = 100
  ps_bd = 0.2

  # Create R random table source
  tablesource = vtkRRandomTableSource()

  # Define distribution type form each output column of the table
  tablesource.SetStatisticalDistributionForColumn(NORMAL,mean_nd,sd_nd,0.0,"Normal",0)
  tablesource.SetStatisticalDistributionForColumn(POISSON,lambda_pd,0.0,0.0,"Poisson",1)
  tablesource.SetStatisticalDistributionForColumn(CHISQUARE,k_csd,0.0,0.0,"Chi-Square",2)
  tablesource.SetStatisticalDistributionForColumn(UNIFORM,lb_ud,ub_ud,0.0,"Uniform",3)
  tablesource.SetStatisticalDistributionForColumn(BINOMIAL,nt_bd,ps_bd,0.0,"Binomial",4)

  tablesource.SetNumberOfRows(30)

  # Update table source output and print the table to the output
  tablesource.Update()
  table = tablesource.GetOutput()
  table.Dump(30)



