// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPDescriptiveStatistics.h"

#include "vtkCommunicator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariant.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPDescriptiveStatistics);
vtkCxxSetObjectMacro(vtkPDescriptiveStatistics, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPDescriptiveStatistics::vtkPDescriptiveStatistics()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPDescriptiveStatistics::~vtkPDescriptiveStatistics()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPDescriptiveStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
void vtkPDescriptiveStatistics::Learn(
  vtkTable* inData, vtkTable* inParameters, vtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // First calculate descriptive statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);

  vtkTable* primaryTab = vtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!primaryTab)
  {
    return;
  }

  vtkIdType nRow = primaryTab->GetNumberOfRows();
  if (!nRow)
  {
    // No statistics were calculated.
    return;
  }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    return;
  }

  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    vtkErrorMacro("No parallel communicator.");
  }

  // (All) gather all sample sizes
  double n_l = primaryTab->GetValueByName(0, "Cardinality").ToDouble(); // Cardinality
  double* n_g = new double[np];
  com->AllGather(&n_l, n_g, 1);

  // Iterate over all parameter rows
  for (int r = 0; r < nRow; ++r)
  {
    // Reduce to global extrema
    double extrema_l[2];
    extrema_l[0] = primaryTab->GetValueByName(r, "Minimum").ToDouble();
    // Collect - max instead of max so a single reduce op. (minimum) can process both extrema at a
    // time
    extrema_l[1] = -primaryTab->GetValueByName(r, "Maximum").ToDouble();

    double extrema_g[2];
    com->AllReduce(extrema_l, extrema_g, 2, vtkCommunicator::MIN_OP);
    primaryTab->SetValueByName(r, "Minimum", extrema_g[0]);
    // max = - min ( - max )
    primaryTab->SetValueByName(r, "Maximum", -extrema_g[1]);

    // (All) gather all local M statistics
    double M_l[4];
    M_l[0] = primaryTab->GetValueByName(r, "Mean").ToDouble();
    M_l[1] = primaryTab->GetValueByName(r, "M2").ToDouble();
    M_l[2] = primaryTab->GetValueByName(r, "M3").ToDouble();
    M_l[3] = primaryTab->GetValueByName(r, "M4").ToDouble();
    double* M_g = new double[4 * np];
    com->AllGather(M_l, M_g, 4);

    // Aggregate all local quadruples of M statistics into global ones
    double ns = n_g[0];
    double mean = M_g[0];
    double mom2 = M_g[1];
    double mom3 = M_g[2];
    double mom4 = M_g[3];

    for (int i = 1; i < np; ++i)
    {
      double ns_l = n_g[i];
      double N = ns + ns_l;

      int o = 4 * i;
      double mean_part = M_g[o];
      double mom2_part = M_g[o + 1];
      double mom3_part = M_g[o + 2];
      double mom4_part = M_g[o + 3];

      double delta = mean_part - mean;
      double delta_sur_N = delta / static_cast<double>(N);
      double delta2_sur_N2 = delta_sur_N * delta_sur_N;

      double ns2 = ns * ns;
      double ns_l2 = ns_l * ns_l;
      double prod_ns = ns * ns_l;

      // ###########
      // # WARNING #
      // ###########
      //
      // The formula from "Formulas for Robust, One-Pass Parallel Computation of Covariances
      // and Arbitrary-Order Statistical Moments" (Philippe Pébay, 2008) for mom4 is WRONG.
      // In particular, the line not involving any lower order moments has a mistake (equation 1.6
      // in the paper).
      // To verify this part of the formula, please refer to equation 3.6 from
      // "Formulas for the computation of higher-order central moments" (Philippe Pébay,
      // Timothy B. Terriberry, Hemanth Kolla and Janine Bennett, 2016).

      mom4 += mom4_part +
        delta2_sur_N2 * delta2_sur_N2 * prod_ns * (ns * ns2 + ns_l * ns_l2) + // Mistake was here
        6. * (ns2 * mom2_part + ns_l2 * mom2) * delta2_sur_N2 +
        4. * (ns * mom3_part - ns_l * mom3) * delta_sur_N;

      mom3 += mom3_part + prod_ns * (ns - ns_l) * delta * delta2_sur_N2 +
        3. * (ns * mom2_part - ns_l * mom2) * delta_sur_N;

      mom2 += mom2_part + prod_ns * delta * delta_sur_N;

      mean += ns_l * delta_sur_N;

      ns = N;
    }

    primaryTab->SetValueByName(r, "Mean", mean);
    primaryTab->SetValueByName(r, "M2", mom2);
    primaryTab->SetValueByName(r, "M3", mom3);
    primaryTab->SetValueByName(r, "M4", mom4);

    // Set global statistics
    primaryTab->SetValueByName(r, "Cardinality", ns);

    // Clean-up
    delete[] M_g;
  }
  delete[] n_g;
}
VTK_ABI_NAMESPACE_END
