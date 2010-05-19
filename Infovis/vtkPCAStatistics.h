/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPCAStatistics.h

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
// .NAME vtkPCAStatistics - A class for principal component analysis
//
// .SECTION Description
// This class derives from the multi-correlative statistics algorithm and
// uses the covariance matrix and Cholesky decomposition computed by it.
// However, when it finalizes the statistics in Learn mode, the PCA class
// computes the SVD of the covariance matrix in order to obtain its eigenvectors.
//
// In the assess mode, the input data are
// - projected into the basis defined by the eigenvectors,
// - the energy associated with each datum is computed,
// - or some combination thereof.
// Additionally, the user may specify some threshold energy or
// eigenvector entry below which the basis is truncated. This allows
// projection into a lower-dimensional state while minimizing (in a 
// least squares sense) the projection error.
//
//
// .SECTION Thanks
// Thanks to David Thompson, Philippe Pebay and Jackson Mayo from
// Sandia National Laboratories for implementing this class.

#ifndef __vtkPCAStatistics_h
#define __vtkPCAStatistics_h

#include "vtkMultiCorrelativeStatistics.h"

class VTK_INFOVIS_EXPORT vtkPCAStatistics : public vtkMultiCorrelativeStatistics
{
public:
  vtkTypeMacro(vtkPCAStatistics,vtkMultiCorrelativeStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  static vtkPCAStatistics* New();

  //BTX
  // Description:
  // Methods by which the covariance matrix may be normalized.
  enum NormalizationType
    {
    NONE,               //!< The covariance matrix should be used as computed.
    TRIANGLE_SPECIFIED, //!< Normalize cov(i,j) by V(i,j) where V is supplied by the user.
    DIAGONAL_SPECIFIED, //!< Normalize cov(i,j) by sqrt(V(i)*V(j)) where V is supplied by the user.
    DIAGONAL_VARIANCE,  //!< Normalize cov(i,j) by sqrt(cov(i,i)*cov(j,j)).
    NUM_NORMALIZATION_SCHEMES //!< The number of normalization schemes.
    };

  // Description:
  // These are the enumeration values that SetBasisScheme() accepts and GetBasisScheme returns.
  enum ProjectionType
    {
    FULL_BASIS,         //!< Use all entries in the basis matrix
    FIXED_BASIS_SIZE,   //!< Use the first N entries in the basis matrix
    FIXED_BASIS_ENERGY, //!< Use consecutive basis matrix entries whose energies sum to at least T
    NUM_BASIS_SCHEMES   //!< The number of schemes (not a valid scheme).
    };
  //ETX

  // Description:
  // This determines how (or if) the covariance matrix \a cov is normalized before PCA.
  //
  // When set to NONE, no normalization is performed. This is the default.
  //
  // When set to TRIANGLE_SPECIFIED, each entry cov(i,j) is divided by V(i,j).
  // The list V of normalization factors must be set using the SetNormalization method
  // before the filter is executed.
  //
  // When set to DIAGONAL_SPECIFIED, each entry cov(i,j) is divided by sqrt(V(i)*V(j)).
  // The list V of normalization factors must be set using the SetNormalization method
  // before the filter is executed.
  //
  // When set to DIAGONAL_VARIANCE, each entry cov(i,j) is divided by sqrt(cov(i,i)*cov(j,j)).
  // <b>Warning</b>: Although this is accepted practice in some fields,
  // some people think you should not turn this option on unless there
  // is a good physically-based reason for doing so. Much better instead
  // to determine how component magnitudes should be compared using
  // physical reasoning and use DIAGONAL_SPECIFIED, TRIANGLE_SPECIFIED, or
  // perform some pre-processing to shift and scale input data columns
  // appropriately than to expect magical results from a shady normalization hack.
  vtkSetMacro(NormalizationScheme,int);
  vtkGetMacro(NormalizationScheme,int);
  virtual void SetNormalizationSchemeByName( const char* sname );
  virtual const char* GetNormalizationSchemeName( int scheme );

  // Description:
  // These methods allow you to set/get values used to normalize the covariance matrix before PCA.
  // The normalization values apply to all requests, so you do not specify a single
  // vector but a 3-column table.
  //
  // The first two columns contain the names of columns from input 0 and the third column contains
  // the value to normalize the corresponding entry in the covariance matrix.
  // The table must always have 3 columns even when the NormalizationScheme is DIAGONAL_SPECIFIED.
  // When only diagonal entries are to be used, only table rows where the first two columns are
  // identical to one another will be employed.
  // If there are multiple rows specifying different values for the same pair of columns,
  // the entry nearest the bottom of the table takes precedence.
  //
  // These functions are actually convenience methods that set/get the third input of the filter.
  // Because the table is the third input, you may use other filters to produce a table of
  // normalizations and have the pipeline take care of updates.
  //
  // Any missing entries will be set to 1.0 and a warning issued.
  // An error will occur if the third input to the filter is not set and the
  // NormalizationScheme is DIAGONAL_SPECIFIED or TRIANGLE_SPECIFIED.
  virtual vtkTable* GetSpecifiedNormalization();
  virtual void SetSpecifiedNormalization( vtkTable* );

  // Description:
  // This variable controls the dimensionality of output tuples in Assess mode.
  // Consider the case where you have requested a PCA on D columns.
  //
  // When set to vtkPCAStatistics::FULL_BASIS, the entire set of basis vectors
  // is used to derive new coordinates for each tuple being assessed.
  // In this mode, you are guaranteed to have output tuples of the same dimension
  // as the input tuples. (That dimension is D, so there will be D additional
  // columns added to the table for the request.)
  //
  // When set to vtkPCAStatistics::FIXED_BASIS_SIZE, only the first N basis vectors
  // are used to derive new coordinates for each tuple being assessed.
  // In this mode, you are guaranteed to have output tuples of dimension min(N,D).
  // You must set N prior to assessing data using the SetFixedBasisSize() method.
  // When N < D, this turns the PCA into a projection (instead of change of basis).
  //
  // When set to vtkPCAStatistics::FIXED_BASIS_ENERGY, the number of basis vectors
  // used to derive new coordinates for each tuple will be the minimum number
  // of columns N that satisfy
  // \f[
  //   \frac{\sum_{i=1}^{N} \lambda_i}{\sum_{i=1}^{D} \lambda_i} < T
  // \f]
  // You must set T prior to assessing data using the SetFixedBasisEnergy() method.
  // When T < 1, this turns the PCA into a projection (instead of change of basis).
  //
  // By default BasisScheme is set to vtkPCAStatistics::FULL_BASIS.
  vtkSetMacro(BasisScheme,int);
  vtkGetMacro(BasisScheme,int);
  virtual const char* GetBasisSchemeName( int schemeIndex );
  virtual void SetBasisSchemeByName( const char* schemeName );

  // Description:
  // The number of basis vectors to use. See SetBasisScheme() for more information.
  // When FixedBasisSize <= 0 (the default), the fixed basis size scheme is equivalent to the full basis scheme.
  vtkSetMacro(FixedBasisSize,int);
  vtkGetMacro(FixedBasisSize,int);

  // Description:
  // The minimum energy the new basis should use, as a fraction. See SetBasisScheme() for more information.
  // When FixedBasisEnergy >= 1 (the default), the fixed basis energy scheme is equivalent to the full basis scheme.
  vtkSetClampMacro(FixedBasisEnergy,double,0.,1.);
  vtkGetMacro(FixedBasisEnergy,double);

//BTX
  // Description:
  // A convenience method (in particular for access from other applications) to 
  // set parameter values.
  // Return true if setting of requested parameter name was excuted, false otherwise.
  virtual bool SetParameter( const char* parameter,
                             int index,
                             vtkVariant value );
//ETX

protected:
  vtkPCAStatistics();
  ~vtkPCAStatistics();

  // Description:
  // This algorithm accepts a vtkTable containing normalization values for
  // its fourth input (port 3).
  // We override FillInputPortInformation to indicate this.
  virtual int FillInputPortInformation( int port, vtkInformation* info );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkDataObject* inMeta );

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable*, 
                       vtkDataObject*, 
                       vtkTable* );

  //BTX  
  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* inData, 
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
  //ETX

  int NormalizationScheme;
  int BasisScheme;
  int FixedBasisSize;
  double FixedBasisEnergy;

  //BTX
  static const char* BasisSchemeEnumNames[NUM_BASIS_SCHEMES + 1];
  static const char* NormalizationSchemeEnumNames[NUM_NORMALIZATION_SCHEMES + 1];
  //ETX

private:
  vtkPCAStatistics( const vtkPCAStatistics& ); // Not implemented
  void operator = ( const vtkPCAStatistics& );  // Not implemented
};

#endif // __vtkPCAStatistics_h

