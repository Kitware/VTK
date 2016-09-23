/*=========================================================================

Program:   Visualization Toolkit
Module:    TestRRandomTableSource.cxx

-------------------------------------------------------------------------
Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkRCalculatorFilter.h>
#include <vtkSmartPointer.h>
#include <vtkRRandomTableSource.h>
#include <vtkDescriptiveStatistics.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{

#define test_expression(expression)                                     \
  {                                                                     \
    if(!(expression))                                                   \
    {                                                                 \
      std::ostringstream buffer;                                 \
      buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
      throw std::runtime_error(buffer.str());                        \
    }                                                                 \
  }

  bool doubleEquals(double left, double right, double epsilon) {
    return (fabs(left - right) < epsilon);
  }

}

int TestRRandomTableSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
  {
    double mean_nd = 5.0;
    double sd_nd = 2.5;
    double lambda_pd = 3.0;
    double k_csd = 3.0;
    double lb_ud = 5.0;
    double ub_ud = 100.0;
    double nt_bd = 100;
    double ps_bd = 0.2;
    vtkRRandomTableSource* rts = vtkRRandomTableSource::New();
    vtkDescriptiveStatistics* dsf = vtkDescriptiveStatistics::New();
    rts->SetNumberOfRows(100000);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,mean_nd,sd_nd,0.0,"Normal",0);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::POISSON,lambda_pd,0.0,0.0,"Poisson",1);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::CHISQUARE,k_csd,0.0,0.0,"Chi-Square",2);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::UNIF,lb_ud,ub_ud,0.0,"Uniform",3);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::BINOMIAL,nt_bd,ps_bd,0.0,"Binomial",4);
    dsf->SetInputConnection(rts->GetOutputPort());
    dsf->AddColumn("Normal");
    dsf->AddColumn("Poisson");
    dsf->AddColumn("Chi-Square");
    dsf->AddColumn("Uniform");
    dsf->AddColumn("Binomial");
    dsf->SetLearnOption( true );
    dsf->SetDeriveOption( true );
    dsf->Update();
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( dsf->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
    vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
    vtkTable* outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );

    for ( vtkIdType r = 0; r < outputPrimary->GetNumberOfRows(); ++ r )
    {
      if(!strcmp(outputPrimary->GetValueByName(r, "Variable").ToString(),"Normal"))
      {
        test_expression(doubleEquals(outputPrimary->GetValueByName(r,"Mean").ToDouble(),mean_nd,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Standard Deviation").ToDouble(),sd_nd,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Skewness").ToDouble(),0.0,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Kurtosis").ToDouble(),0.0,1.0));
      }
      else if(!strcmp(outputPrimary->GetValueByName(r, "Variable").ToString(),"Poisson"))
      {
        test_expression(doubleEquals(outputPrimary->GetValueByName(r,"Mean").ToDouble(),lambda_pd,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Standard Deviation").ToDouble(),sqrt(lambda_pd),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Skewness").ToDouble(),1.0/sqrt(lambda_pd),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Kurtosis").ToDouble(),1.0/lambda_pd,1.0));
      }
      else if(!strcmp(outputPrimary->GetValueByName(r, "Variable").ToString(),"Chi-Square"))
      {
        test_expression(doubleEquals(outputPrimary->GetValueByName(r,"Mean").ToDouble(),k_csd,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Standard Deviation").ToDouble(),sqrt(2.0*k_csd),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Skewness").ToDouble(),sqrt(8.0/k_csd),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Kurtosis").ToDouble(),12.0/k_csd,2.0));
      }
      else if(!strcmp(outputPrimary->GetValueByName(r, "Variable").ToString(),"Uniform"))
      {
        test_expression(doubleEquals(outputPrimary->GetValueByName(r,"Mean").ToDouble(),0.5*(lb_ud+ub_ud),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Standard Deviation").ToDouble(),sqrt((1.0/12.0)*pow((ub_ud-lb_ud),2)),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Skewness").ToDouble(),0.0,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Kurtosis").ToDouble(),-(6.0/5.0),1.0));
      }
      else if(!strcmp(outputPrimary->GetValueByName(r, "Variable").ToString(),"Binomial"))
      {
        test_expression(doubleEquals(outputPrimary->GetValueByName(r,"Mean").ToDouble(),nt_bd*ps_bd,1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Standard Deviation").ToDouble(),sqrt(nt_bd*ps_bd*(1.0 - ps_bd)),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Skewness").ToDouble(),(1.0 - 2.0*ps_bd)/sqrt(nt_bd*ps_bd*(1.0 - ps_bd)),1.0));
        test_expression(doubleEquals(outputDerived->GetValueByName(r,"Kurtosis").ToDouble(),(1.0 - 6.0*ps_bd*(1.0 - ps_bd))/(nt_bd*ps_bd*(1.0 - ps_bd)),1.0));
      }
    }

    dsf->Delete();
    rts->Delete();
    return 0;
  }

  catch( std::exception& e )
  {
    cerr << e.what()
         << "\n";
    return 1;
  }
}

