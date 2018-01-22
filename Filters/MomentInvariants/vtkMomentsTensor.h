/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentsTensor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/**
 * @class   vtkMomentsTensor
 * @brief   helper class that stores a tensor of arbitrary rank and dimension
 *
 * A tensor in general is a multidimensional array its rank gives the number
 * of indices to reference its entries the number of entries is
 * dimension^rank the application of this class is the moment invariant
 * pattern detection the theory and the algorithm is described in Roxana
 * Bujack and Hans Hagen: "Moment Invariants for Multi-Dimensional Data"
 * http://www.informatik.uni-leipzig.de/~bujack/2017TensorDagstuhl.pdf for
 * this purpose, we split the rank into fieldrank and moment rank the class
 * mainly first multiplies and the contracts moment tensors and tracks these
 * processes.
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkMomentsTensor_h
#define vtkMomentsTensor_h
#ifndef __VTK_WRAP__
#ifndef VTK_WRAPPING_CXX

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <vtkMath.h>

#include <iostream>
#include <vector>

class vtkMomentsTensor
{
private:
  /**
   * Dimension of the tensor can for example be 2 or 3
   */
  size_t m_dimension;

  /**
   * rank of this tensor, i.e. number of indices to reference its entries.
   * for uncontracted tensor it is: m_momentRank + m_FieldRank
   */
  size_t m_rank;

  /**
   * Fieldrank of the tensor is 0 for scalars, 1 for vectors, 3 for matrices
   */
  size_t m_fieldRank;

  /**
   * Order of the moment tensor
   */
  size_t m_momentRank;

  /**
   * data vector of size dim^rank with T_ijk at position i + dim * j + dim^2 * k
   */
  std::vector<double> m_data;

  /**
   * optional outer tensor product information that produced it
   */
  std::vector<size_t> m_productInfo;

  /**
   * optional contraction information that produced it
   */
  std::vector<size_t> m_contractionInfo;

  /**
   * check if indices are within bounds
   * @param indices: vector of indices that identify the component of the tensor
   */
  void validate(const std::vector<size_t>& indices)
  {
    if (indices.size() > m_rank)
    {
      vtkGenericWarningMacro("indices too long.");
    }
    for (size_t i = 0; i < indices.size(); ++i)
    {
      if (indices.at(i) > m_dimension)
      {
        vtkGenericWarningMacro("index too big.");
      }
    }
  }

  /**
   * change the position of index j with index k
   * T'_ijlkm = sum_{j=k} T_ikljm (i,l,m are multiindices)
   * @param j: index to be transposed
   * @param k: index to be transposed
   */
  inline vtkMomentsTensor transpose(size_t j, size_t k)
  {
    if (j == k)
    {
      return *this;
    }
    if (j > m_rank || k > m_rank)
    {
      vtkGenericWarningMacro("index too big.");
    }
    vtkMomentsTensor transposition(*this);
    for (size_t i = 0; i < m_data.size(); ++i)
    {
      std::vector<size_t> indices = getIndices(i);
      size_t temp = indices.at(j);
      indices.at(j) = indices.at(k);
      indices.at(k) = temp;
      transposition.set(getIndex(indices), m_data.at(i));
    }
    return transposition;
  }

  /**
   * set contraction information after contraction has just produced a new tensor
   * @param parentInfo: contraction information  of the tensor that produces the new tensor through
   * contraction
   * @param j: index that is contracted
   * @param k: index that is contracted
   */
  inline void setContractionInfo(const std::vector<size_t>& parentInfo, size_t i, size_t j)
  {
    m_contractionInfo = parentInfo;
    m_contractionInfo.push_back(i);
    m_contractionInfo.push_back(j);
  }

  /**
   * we can generate vectors from second order tensors using the eigenvectors
   * the, the contractionInfo contains which eigenvector it is
   * @param parentInfo: contraction information  of the tensor that produces the new tensor through
   * contraction
   * @param i: index of the eigenvector
   */
  inline void setContractionInfo(const std::vector<size_t>& parentInfo, size_t i)
  {
    m_contractionInfo = parentInfo;
    m_contractionInfo.push_back(i);
  }

  /**
   * after tensors are multiplied, we store which tensors produced them
   * @param productInfo1: product information of the tensor that produces the new tensor through
   * multiplication
   * @param productInfo2: product information of the tensor that produces the new tensor through
   * multiplication
   */
  inline void setProductInfo(const std::vector<size_t>& productInfo1, const std::vector<size_t>& productInfo2)
  {
    m_productInfo = productInfo1;
    m_productInfo.insert(m_productInfo.end(), productInfo2.begin(), productInfo2.end());
  }

  /**
   * if a tensor is not created through multiplication, we just pass down the information of its
   * parent
   * @param parentInfo: product information of the tensor that produces the new tensor
   */
  inline void setProductInfo(const std::vector<size_t>& parentInfo) { m_productInfo = parentInfo; }

public:
  /**
   * empty constructor
   */
  vtkMomentsTensor()
    : m_dimension(0)
    , m_rank(0)
    , m_fieldRank(0)
    , m_momentRank(0)
  {
  }

  /**
   * constructor allocating the data vector
   * @param dimension: e.g. 2D or 3D
   * @param rank: rank of the tensor, i.e. number of indices to reference its entries
   * @param fieldRank: 0 for scalars, 1 for vectors, 3 for matrices
   */
  vtkMomentsTensor(size_t dimension, size_t rank, size_t fieldRank)
    : m_dimension(dimension)
    , m_rank(rank)
    , m_fieldRank(fieldRank)
    , m_momentRank(rank - fieldRank)
  {
    m_data = std::vector<double>(pow(dimension, rank), 0);
    m_productInfo.push_back(rank);
  }

  /**
   * copy constructor
   */
  vtkMomentsTensor(vtkMomentsTensor* tensor)
    : m_dimension(tensor->getDimension())
    , m_rank(tensor->getRank())
    , m_fieldRank(tensor->getFieldRank())
    , m_momentRank(tensor->getMomentRank())
  {
    m_data = tensor->getData();
    m_contractionInfo = tensor->getContractionInfo();
    m_productInfo = tensor->getProductInfo();
  }

  /**
   * constructor allocating the data vector
   * @param dimension: e.g. 2D or 3D
   * @param rank: rank of the tensor, i.e. number of indices to reference its entries
   * @param fieldRank: 0 for scalars, 1 for vectors, 3 for matrices
   * @param momentRank: order of the moment tensor
   */
  vtkMomentsTensor(size_t dimension, size_t rank, size_t fieldRank, size_t momentRank)
    : m_dimension(dimension)
    , m_rank(rank)
    , m_fieldRank(fieldRank)
    , m_momentRank(momentRank)
  {
    m_data = std::vector<double>(pow(dimension, rank), 0);
    m_productInfo.push_back(rank);
  }

  /**
   * copy constructor from eigen matrix
   */
  vtkMomentsTensor(Eigen::MatrixXd data)
    : m_dimension(data.rows())
    , m_rank(data.cols())
    , m_fieldRank(0)
    , m_momentRank(data.rows())
  {
    m_data = std::vector<double>(pow(m_dimension, m_rank), 0);
    for (size_t i = 0; i < this->size(); ++i)
    {
      this->set(i, data(i));
    }
  }

  /**
   * inverse function to getIndices
   * @param indices: vector of tensor indices that identify an entry
   * @return the place in the flat c++ std vector
   */
  inline size_t getIndex(const std::vector<size_t>& indices)
  {
    validate(indices);
    size_t index = 0;
    for (size_t i = 0; i < indices.size(); ++i)
    {
      index += pow(m_dimension, i) * indices.at(i);
    }
    // cout<<index<<endl;
    return index;
  }

  /**
   * get rank of the tensor, i.e. number of indices to reference its entries
   */
  inline size_t getRank() { return (m_rank); }

  /**
   * get field rank, 0 for scalars, 1 for vectors, 3 for matrices
   */
  inline size_t getFieldRank() { return (m_fieldRank); }

  /**
   * get moment rank, order of the moment tensor
   */
  inline size_t getMomentRank() { return (m_momentRank); }

  /**
   * get dimension, e.g. 2D or 3D
   */
  inline size_t getDimension() { return (m_dimension); }

  /**
   * get data vector
   */
  inline std::vector<double> getData() { return m_data; }

  /**
   * get the vector with the indices that indicate which tensor this one was produced from through
   * contraction
   */
  inline std::vector<size_t> getContractionInfo() { return m_contractionInfo; }

  /**
   * get the vector with the indices that indicate which tensor this one was produced from through
   * multiplication
   */
  inline std::vector<size_t> getProductInfo() { return m_productInfo; }

  /**
   * get the number of entries of this tensor
   */
  inline size_t size() { return m_data.size(); }

  /**
   * inverse function to getIndex
   * @param index: the place in the flat c++ std vector
   * @return: vector of tensor indices that identify an entry
   */
  inline std::vector<size_t> getIndices(size_t index)
  {
    std::vector<size_t> indices(m_rank, 0);
    for (size_t i = 0; i < m_rank; ++i)
    {
      indices.at(i) = (int(int(index) / int(pow(m_dimension, i)))) % int(m_dimension);
    }
    // cout<<"index:"<<index<<"\t"<<getIndex(indices)<<endl;
    return indices;
  }

  /**
   * this function adds the numbers of zeros, ones, and twos in the tensor index
   * it is used to determine the tensor's properties under reflection
   * iff the sum is odd, the sign of the entry changes
   * @return the vector with the added values
   */
  inline std::vector<size_t> getIndexSum(size_t index)
  {
    std::vector<size_t> sum(m_dimension, 0);
    std::vector<size_t> indices = getIndices(index);
    for (size_t i = 0; i < m_rank; ++i)
    {
      sum.at(indices.at(i))++;
    }
    return sum;
  }

  /**
   * the moment tensors have two types of indices fieldIndices and momentIndices
   * fieldIndices of length fieldRank refer to the components of the original data (3 for vector, 9
   * for matrix) momentIndices of length momentRank refer to the basis function
   * @param index: the place in the flat c++ std vector
   * @return: the indices that correspond to the basis function
   */
  inline std::vector<size_t> getMomentIndices(size_t index)
  {
    if (m_contractionInfo.size() > 0)
    {
      vtkGenericWarningMacro("This tensor is a contraction and has no clearly separated indices.");
    }
    if (m_productInfo.size() > 1)
    {
      vtkGenericWarningMacro("This tensor is a product and has no clearly separated indices.");
    }
    std::vector<size_t> indices(m_momentRank, 0);
    for (size_t i = 0; i < m_momentRank; ++i)
    {
      indices.at(i) = (int(int(index) / int(pow(m_dimension, i)))) % int(m_dimension);
    }
    return indices;
  }

  /**
   * the moment tensors have two types of indices fieldIndices and momentIndices
   * fieldIndices of length fieldRank refer to the components of the original data (3 for vector, 9
   * for matrix) momentIndices of length momentRank refer to the basis function
   * @param index: the place in the flat c++ std vector
   * @return: the indices that correspond to the function components
   */
  inline std::vector<size_t> getFieldIndices(size_t index)
  {
    if (m_contractionInfo.size() > 0)
    {
      vtkGenericWarningMacro("This tensor is a contraction and has no clearly separated indices.");
    }
    if (m_productInfo.size() > 1)
    {
      vtkGenericWarningMacro("This tensor is a product and has no clearly separated indices.");
    }
    if (m_fieldRank + m_momentRank != m_rank)
    {
      vtkGenericWarningMacro("m_fieldRank + m_momentRank != m_rank.");
    }
    std::vector<size_t> indices(m_fieldRank, 0);
    for (size_t i = 0; i < m_fieldRank; ++i)
    {
      indices.at(i) =
        (int(int(index) / int(pow(m_dimension, i + m_momentRank)))) % int(m_dimension);
    }
    return indices;
  }

  /**
   * IMPORTANT
   * this function forms the bridge to vtk
   * it translates the order of the fieldIndices to vtk's order as used by vtkDoubleArray.SetTuple()
   * there the values are stored as follows:
   * 2D and 3D: scalar: 0, vector: 0,1,2 matrix: 00,10,20,01,11,21,02,12,22
   * 2D has zeros in the indices containing a 2
   * @param index: the place in the flat c++ std vector of this class
   * @return: the corresponding index as used by vtkDoubleArray.SetTuple().
   */
  inline size_t getFieldIndex(size_t index)
  {
    size_t fieldIndex = 0;
    for (size_t j = 0; j < m_fieldRank; ++j)
    {
      fieldIndex += getFieldIndices(index).at(j) * size_t(pow(3, m_fieldRank - j - 1));
    }
    return fieldIndex;
  }

  /**
   * Get data entry for given tensor indices
   */
  inline double get(const std::vector<size_t>& indices) { return m_data.at(getIndex(indices)); }

  /**
   * Get data entry for given flat c++ vector index
   */
  inline double get(size_t index) { return m_data.at(index); }

  /**
   * Set data entry for given tensor indices
   */
  inline void set(const std::vector<size_t>& indices, double value)
  {
    m_data.at(getIndex(indices)) = value;
  }

  /**
   * Set data entry for given tensor indices
   */
  inline void set(size_t index, double value) { m_data.at(index) = value; }

  /**
   * Set the whole data vector
   */
  inline void set(const std::vector<double>& data) { m_data = data; }

  /**
   * This function produces a tensor contraction of the last two indices
   * T'_i = sum_{j=k} T_ijk (i is multiindex)
   * for arbitrary contractions, use the transpose function and move the indices there prior to
   * calling this function
   * @return the contracted tensor. its rank is two lower than the rank of its parent
   */
  inline vtkMomentsTensor contract()
  {
    if (m_rank < 2)
    {
      vtkGenericWarningMacro("rank too small for contraction.");
    }
    vtkMomentsTensor contraction(m_dimension, m_rank - 2, m_fieldRank, m_momentRank);
    for (size_t i = 0; i < pow(m_dimension, m_rank - 2); ++i)
    {
      double value = 0;
      for (size_t j = 0; j < m_dimension; ++j)
      {
        //                    cout<<"nu "<<i + pow( m_dimension, m_rank - 2 ) * j  + pow(
        //                    m_dimension, m_rank - 1 ) * j<<"\t"; for( size_t k = 0; k <
        //                    getIndices(i).size() ; ++k )
        //                    {
        //                        cout<<getIndices(i + pow( m_dimension, m_rank - 2 ) * j  + pow(
        //                        m_dimension, m_rank - 1 ) * j).at(k)<<"\t";
        //                    }
        //                    cout<<m_data.at( i + pow( m_dimension, m_rank - 2 ) * j  + pow(
        //                    m_dimension, m_rank - 1 ) * j )<<endl;
        value += m_data.at(i + pow(m_dimension, m_rank - 2) * j + pow(m_dimension, m_rank - 1) * j);
      }
      contraction.set(i, value);
    }
    return contraction;
  }

  /**
   * This function produces a tensor contraction of the indices i and k
   * T'_i = sum_{j=k} T_ijk (i is multiindex)
   * we use the transpose function and move the indices i and k to the end and then call the
   * contract() function
   * @param: i index for contraction
   * @param: k index for contraction
   * @return the contracted tensor. its rank is two lower than the rank of its parent
   */
  inline vtkMomentsTensor contract(size_t i, size_t j)
  {
    if (m_rank < 2)
    {
      vtkGenericWarningMacro("rank too small for contraction.");
    }
    if (i >= j)
    {
      vtkGenericWarningMacro("indices for contracion are not asending.");
    }

    vtkMomentsTensor contr1 = transpose(i, m_rank - 2);
    vtkMomentsTensor contr2;
    if (j == m_rank - 2)
    {
      contr2 = contr1.transpose(i, m_rank - 1);
    }
    else
    {
      contr2 = contr1.transpose(j, m_rank - 1);
    }
    vtkMomentsTensor contr3 = contr2.contract();
    contr3.setContractionInfo(this->m_contractionInfo, i, j);
    contr3.setProductInfo(this->m_productInfo, std::vector<size_t>(0));

    return contr3;
  }

  /**
   * This function produces a tensor contraction of the indices stored in contractor
   * @param contractor: list of indices to be contracted
   * @return the contracted tensor. its rank is length(contractor) lower than the rank of its parent
   */
  inline vtkMomentsTensor contract(std::vector<size_t> contractor)
  {
    if (contractor.size() == 0)
    {
      return *this;
    }
    if (m_rank < contractor.size())
    {
      vtkGenericWarningMacro("rank too small for contraction.");
    }

    vtkMomentsTensor contraction = *this;
    for (size_t i = 0; i < contractor.size() - 1; i = i + 2)
    {
      contraction = contraction.contract(contractor.at(i), contractor.at(i + 1));
    }

    // index pairs correspond to contractions. a potential last single index refers to the
    // eigenvector
    if (contractor.size() % 2 == 1)
    {
      if (contraction.getRank() != 2)
      {
        vtkGenericWarningMacro("only rank two can have eigenvectors.");
      }
      // contraction.eigenVectors().at( contractor.back() ).print();
      if (contractor.back() < getDimension())
      {
        return contraction.eigenVectors().at(contractor.back());
      }
      else
      {
        vtkMomentsTensor eigenVector =
          contraction.eigenVectors().at(contractor.back() - getDimension());
        for (size_t i = 0; i < eigenVector.size(); ++i)
        {
          eigenVector.set(i, -eigenVector.get(i));
        }
        return eigenVector;
      }
    }
    return contraction;
  }

  /**
   * This function prints the properties of this tensor
   */
  inline void print()
  {
    cout << " m_rank=" << m_rank << " m_fieldRank=" << m_fieldRank
         << " m_momentRank=" << m_momentRank << " m_data.size()=" << m_data.size()
         << " m_contractionInfo=";
    for (size_t i = 0; i < m_contractionInfo.size(); ++i)
    {
      cout << m_contractionInfo.at(i) << " ";
    }
    cout << " m_productInfo=";
    for (size_t i = 0; i < m_productInfo.size(); ++i)
    {
      cout << m_productInfo.at(i) << " ";
    }
    cout << endl;
    for (size_t i = 0; i < this->size(); ++i)
    {
      cout << i << "\t";
      for (size_t j = 0; j < getIndices(i).size(); ++j)
      {
        cout << getIndices(i).at(j) << "\t";
      }
      //                cout<<" sum=";
      //                for( size_t j = 0; j < getDimension() ; ++j )
      //                {
      //                    cout<<getIndexSum(i).at(j)<<"\t";
      //                }
      //                cout<<endl;
      //                cout<<" momentIndices=";
      //                for( size_t j = 0; j < getMomentIndices(i).size() ; ++j )
      //                {
      //                    cout<<getMomentIndices(i).at(j)<<"\t";
      //                }
      //                cout<<endl;
      //                cout<<" fieldIndices=";
      //                for( size_t j = 0; j < getFieldIndices(i).size() ; ++j )
      //                {
      //                    cout<<getFieldIndices(i).at(j)<<"\t";
      //                }
      //                cout<<" fieldIndex="<<getFieldIndex(i)<<endl;
      //
      //                cout<<endl;
      cout << m_data.at(i) << endl;
    }
  }

  /**
   * This function produces a list of all possible 2-index combinations, which is used to generate
   * all possible contractions
   * @return vector of the contracted tensors. their rank is 2 lower than the rank of the parent
   */
  inline std::vector<vtkMomentsTensor> contractAll()
  {
    std::vector<vtkMomentsTensor> contractions;
    for (size_t i = 0; i < m_rank - 1; ++i)
    {
      for (size_t j = i + 1; j < m_rank; ++j)
      {
        contractions.push_back(contract(i, j));
      }
    }
    return contractions;
  }

  /**
   * This function computes the Euclidean norm of the tensor
   * @return Euclidean norm of the tensor
   */
  inline double norm()
  {
    double norm = 0;
    for (size_t i = 0; i < this->size(); ++i)
    {
      norm += pow(m_data.at(i), 2);
    }
    return (sqrt(norm));
  }

  /**
   * for vector and matrix fields, the last indizes don't belong to the moment-indices, but the
   * fieldIndex the number of zeros in the moment-indices equals p in the basis function x^p*y^q*z^r
   * the number of one in a tensor-multiindex equals q in the basis function x^p*y^q*z^r
   * the number of twos in a tensor-multiindex equals r in the basis function x^p*y^q*z^r
   * @param index: the place in the flat c++ std vector
   * @return: vector of summed tensor indices corresponds to p, q, r
   */
  inline std::vector<size_t> getOrders(size_t index)
  {
    // cout<<"getOrders"<<index<<endl;
    std::vector<size_t> orders(m_dimension);
    std::vector<size_t> indices = getMomentIndices(index);
    for (size_t i = 0; i < m_momentRank; ++i)
    {
      orders.at(indices.at(i))++;
    }
    return orders;
  }

  /**
   * rotate the tensor
   * @param rotMat: rotation matrix that is applied
   * @return: rotated tensor
   */
  inline vtkMomentsTensor rotate(Eigen::MatrixXd rotMat)
  {
    vtkMomentsTensor rotation(this);
    double summand, factor;
    std::vector<size_t> indicesI, indicesJ;
    for (size_t i = 0; i < this->size(); ++i)
    {
      summand = 0;
      indicesI = this->getIndices(i);
      for (size_t j = 0; j < this->size(); ++j)
      {
        factor = 1;
        indicesJ = this->getIndices(j);
        for (size_t k = 0; k < indicesI.size(); ++k)
        {
          factor *= rotMat(indicesI.at(k), indicesJ.at(k));
          //                        cout<< indicesI.at( k )<<"\t"<< indicesJ.at( k ) <<"\t"<<rotMat(
          //                        indicesI.at( k ), indicesJ.at( k ) )<<endl;
        }
        summand += factor * this->get(indicesJ);
        //                    cout<<"factor="<<factor<<endl;
        //                    cout<<"summand="<<summand<<endl;
      }
      rotation.set(i, summand);
    }
    return rotation;
  }

  /**
   * multiply two tensors.
   * the rank of the result is the sum of the ranks of its parents
   * @param tensor1: factor
   * @param tensor2: factor
   * @return: tensor product
   */
  static inline vtkMomentsTensor tensorProduct(vtkMomentsTensor tensor1, vtkMomentsTensor tensor2)
  {
    if (tensor1.getDimension() != tensor2.getDimension())
    {
      vtkGenericWarningMacro("only tensor with the same dimension can be multiplied.");
    }
    vtkMomentsTensor product(tensor1.getDimension(),
      tensor1.getRank() + tensor2.getRank(),
      tensor1.getFieldRank() + tensor2.getFieldRank());
    product.setProductInfo(tensor1.getProductInfo(), tensor2.getProductInfo());
    std::vector<size_t> indices1;
    std::vector<size_t> indices2;
    for (size_t i = 0; i < tensor1.size(); ++i)
    {
      for (size_t j = 0; j < tensor2.size(); ++j)
      {
        indices1 = tensor1.getIndices(i);
        indices2 = tensor2.getIndices(j);
        indices1.insert(indices1.end(), indices2.begin(), indices2.end());
        product.set(indices1, tensor1.get(i) * tensor2.get(j));
      }
    }
    return product;
  }

  /**
   * sum two tensors.
   * the rank of the result is the same as the rank of its parents
   * @param tensor1: summand
   * @param tensor2: summand
   * @return: tensor sum
   */
  static inline vtkMomentsTensor tensorSum(vtkMomentsTensor tensor1,
    vtkMomentsTensor tensor2,
    double a,
    double b)
  {
    if (tensor1.getDimension() != tensor2.getDimension())
    {
      vtkGenericWarningMacro("only tensors with the same dimension can be added.");
    }
    if (tensor1.getRank() != tensor2.getRank())
    {
      vtkGenericWarningMacro("only tensors with the same rank can be added.");
    }
    if (tensor1.getFieldRank() != tensor2.getFieldRank())
    {
      vtkGenericWarningMacro("only tensors with the same fieldrank can be added.");
    }
    vtkMomentsTensor sum(tensor1.getDimension(), tensor1.getRank(), tensor1.getFieldRank());
    sum.setProductInfo(std::vector<size_t>(0));
    for (size_t i = 0; i < tensor1.size(); ++i)
    {
      sum.set(i, a * tensor1.get(i) + b * tensor2.get(i));
    }
    return sum;
  }

  /**
   * Euclidean distance between two tensors.
   * @param tensor1: tensor
   * @param tensor2: tensor
   * @return: tensor distance
   */
  static inline double tensorDistance(vtkMomentsTensor tensor1, vtkMomentsTensor tensor2)
  {
    return vtkMomentsTensor::tensorSum(tensor1, tensor2, 1, -1).norm();
  }

  /**
   * convenience function converting a rank 1 tensor into a vector
   * @return: vector
   */
  inline Eigen::VectorXd getVector()
  {
    if (getRank() != 1)
    {
      print();
      vtkGenericWarningMacro("only tensors of rank 1 can be vectors.");
    }
    Eigen::VectorXd vector(getDimension());
    for (size_t d = 0; d < getDimension(); ++d)
    {
      vector[d] = get(d);
    }
    return vector;
  }

  /**
   * convenience function converting a rank 2 tensor into a matrix
   * @return: matrix
   */
  inline Eigen::MatrixXd getMatrix()
  {
    if (getRank() != 2)
    {
      vtkGenericWarningMacro("only tensors of rank 2 can be matrices.");
    }
    Eigen::MatrixXd matrix(getDimension(), getDimension());
    for (size_t i = 0; i < this->size(); ++i)
    {
      matrix(i) = get(i);
    }
    return matrix;
  }

  /**
   * this function produces the anti-symmetric Levi-Civita tensor
   * @return: Levi-Civita tensor
   */
  static inline vtkMomentsTensor getEpsilon(size_t dimension)
  {
    vtkMomentsTensor epsilon(dimension, dimension, 0);
    if (dimension == 2)
    {
      static const double arr[] = { 0, -1, 1, 0 };
      epsilon.set(std::vector<double>(arr, arr + sizeof(arr) / sizeof(arr[0])));
    }
    else if (dimension == 3)
    {
      static const double arr[] = {
        0, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0
      };
      epsilon.set(std::vector<double>(arr, arr + sizeof(arr) / sizeof(arr[0])));
    }
    else
    {
      vtkGenericWarningMacro("dimension has to be 2 or 3.");
    }
    return epsilon;
  }

  /**
   * this function computes the eigenvectors of a rank 2 tensor
   * they are not normalized, but weighted with how distinct their corresponding eigenvalues are
   * e.g. if an eigenvalue appears twice, the corresponding vectors will be weighted with zero
   * @return: vector with the weighted eigenvectors
   */
  std::vector<vtkMomentsTensor> eigenVectors()
  {
    if (getDimension() != 3 && getDimension() != 2)
    {
      vtkGenericWarningMacro("dimension has to be 2 or 3.");
    }

    std::vector<vtkMomentsTensor> eigenVectors(getDimension());
    // we take the symmetric part only to guarantee the existence of real eigenvectors
    Eigen::EigenSolver<Eigen::MatrixXd> es(0.5 * (getMatrix() + getMatrix().transpose()));
    // sort eigenvalues by real part
    std::vector<size_t> eigenValueOrder(getDimension(), 1);
    for (size_t d = 0; d < getDimension(); ++d)
    {
      if (real(es.eigenvalues()[eigenValueOrder.at(0)]) < real(es.eigenvalues()[d]) - 1e-10)
      {
        eigenValueOrder.at(0) = d;
      }
      if (real(es.eigenvalues()[eigenValueOrder.at(getDimension() - 1)]) >
        real(es.eigenvalues()[d]) + 1e-10)
      {
        eigenValueOrder.at(getDimension() - 1) = d;
      }
    }
    if (getDimension() == 3)
    {
      eigenValueOrder.at(1) = 3 - eigenValueOrder.at(0) - eigenValueOrder.at(2);
    }

    for (size_t d = 0; d < getDimension() - 1; ++d)
    {
      if (real(es.eigenvalues()[eigenValueOrder.at(d)]) <
        real(es.eigenvalues()[eigenValueOrder.at(d + 1)]) - 1e-10)
      {
        vtkGenericWarningMacro("eigenvalues not sorted: " << es.eigenvalues());
      }
    }

    // weigh eigenvector by how distinguished its corresponding eigenvalue is
    for (size_t d = 0; d < getDimension(); ++d)
    {
      double factor = 1000;
      for (size_t e = 0; e < getDimension(); ++e)
      {
        if (eigenValueOrder.at(d) != e)
        {
          factor = std::min(factor,
            std::abs(real(es.eigenvalues()[eigenValueOrder.at(d)]) - real(es.eigenvalues()[e])));
        }
      }
      //                // use the version of the eigenvector that points to the positive axis that
      //                does not vanish. not stable for( size_t e = 0; e < getDimension(); ++e )
      //                {
      //                    if( abs( es.pseudoEigenvectors().col( eigenValueOrder.at( d ) )[ e ] ) >
      //                    1e-3 )
      //                    {
      //                        if( es.pseudoEigenvectors().col( eigenValueOrder.at( d ) )[ e ] < 0
      //                        )
      //                        {
      //                            factor *= -1;
      //                        }
      //                        break;
      //                    }
      //                }

      // in 2D, the weight from before is always the same for both, so also weigh it by its
      // eigenvalue
      if (getDimension() == 2)
      {
        factor *= real(es.eigenvalues()[eigenValueOrder.at(d)]);
      }
      eigenVectors.at(d) =
        vtkMomentsTensor(es.pseudoEigenvectors().col(eigenValueOrder.at(d)) * factor);
      eigenVectors.at(d).setContractionInfo(getContractionInfo(), d);
      eigenVectors.at(d).setProductInfo(getProductInfo());
    }
    //#ifdef DEBUG
    //            cout<<"eigenVectors()"<<endl;
    //            cout<<es.eigenvalues()<<endl;
    //            cout<<es.pseudoEigenvectors()<<endl;
    //            for( size_t d = 0; d < getDimension(); ++d )
    //            {
    //                cout<<eigenValueOrder.at( d )<<endl;
    //            }
    //#endif
    return eigenVectors;
  }

  /**
   * if we have a rank 1 tensor that was made from an eigenvector, its sign is arbitrary
   * that means for the standard position, we need to take it and its negative into account
   * this function generates the negative
   * its contractionInfo is encoded as the eigenvector index plus the dimension
   * e.g. the largest 3D EV will have a 0 andits negative a 3
   */
  inline void otherEV()
  {
    m_contractionInfo.back() = m_contractionInfo.back() + m_dimension;
    for (size_t i = 0; i < m_data.size(); ++i)
    {
      set(i, -get(i));
    }
  }
};

#endif // VTK_WRAPPING_CXX
#endif // __VTK_WRAP__
#endif // vtkMomentsTensor_h
// VTK-HeaderTest-Exclude: vtkMomentsTensor.h
