/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeMoments.cxx

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
#include "vtkComputeMoments.h"

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkResampleWithDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkMomentsHelper.h"
#include "vtkMomentsTensor.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Dense)
#include <vector>

#include "vtk_kissfft.h"
#include VTK_KISSFFT_HEADER(kiss_fft.h)
#include VTK_KISSFFT_HEADER(tools/kiss_fftnd.h)

/**
 * standard vtk new operator
 */
vtkStandardNewMacro(vtkComputeMoments);

/**
 * constructior setting defaults
 */
vtkComputeMoments::vtkComputeMoments()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->Dimension = 0;
  this->FieldRank = 0;
  this->Order = 2;
  this->Radii = std::vector<double>(0);
  this->RelativeRadii = std::vector<double>(0);
  for (int k = 5; k > 2; --k)
  {
    RelativeRadii.push_back(1. / pow(2.0, k));
  }
  this->NumberOfIntegrationSteps = 5;
  this->NameOfPointData = "no name set by user";
  this->Extent = 0;
  this->UseFFT = true;
}

/**
 * destructor
 */
vtkComputeMoments::~vtkComputeMoments() {}

std::string vtkComputeMoments::GetStringTensorIndices(size_t index,
  int dimension,
  int order,
  int fieldRank)
{
  return vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(
    index, dimension, order, fieldRank);
};

/**
 * the agorithm has two input ports
 * port 0 is the dataset of which the moments are computed
 * port 1 is the grid at whose locations the moments are computed. if not set, the original grid is
 * chosen
 */
int vtkComputeMoments::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

/**
 * the agorithm generates a field of vtkImageData storing the moments. It will
 * have numberOfFields scalar arrays in its pointdata it has the same dimensions
 * and topology as the second inputport
 */
int vtkComputeMoments::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

/** standard vtk print function
 * @param os: the way how to print
 * @param indent: how far to the right the text shall appear
 */
void vtkComputeMoments::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Hier: vtkComputeMoments::PrintSelf\n";
  os << indent << "Dimension =  " << this->Dimension << "\n";
  os << indent << "FieldRank =  " << this->FieldRank << "\n";
  os << indent << "Order =  " << this->Order << "\n";
  os << indent << "Radii =  ";
  for (size_t i = 0; i < this->Radii.size(); ++i)
  {
    os << std::scientific << setprecision(10) << this->Radii.at(i) << " ";
  }
  os << "\n";
  os << indent << "NumberOfIntegrationSteps =  " << this->NumberOfIntegrationSteps << "\n";
  os << indent << "NameOfPointData =  " << this->NameOfPointData << "\n";
  os << indent << "UseFFT? " << this->UseFFT << "\n";

  this->Superclass::PrintSelf(os, indent);
}

/**
 * Set the radii of the integration.
 */
void vtkComputeMoments::SetRadii(const std::vector<double>& radii)
{
  this->Radii = radii;
  this->RelativeRadii.resize(0);
  for (size_t i = 0; i < radii.size(); ++i)
  {
    this->RelativeRadii.push_back(radii.at(i) / this->Extent);
  }
};

/**
 * Set the relative radii of the integration, i.e. radius / min extent of the dataset.
 */
void vtkComputeMoments::SetRelativeRadii(const std::vector<double>& relativeRadii)
{
  this->RelativeRadii = relativeRadii;
  this->Radii.resize(0);
  for (size_t i = 0; i < relativeRadii.size(); ++i)
  {
    this->Radii.push_back(relativeRadii.at(i) * this->Extent);
  }
};

/**
 * Set the different integration radii from the field as constant length array
 * for python wrapping
 * @param radiiArray: array of size 10 containing the radii. if less radii are
 * desired, fill the remaining entries with zeros
 */
void vtkComputeMoments::SetRadiiArray(double radiiArray[10])
{
  this->Radii.resize(0);
  this->RelativeRadii.resize(0);
  for (int i = 0; i < 10; ++i)
  {
    if (radiiArray[i] == 0)
    {
      break;
    }
    else
    {
      this->Radii.push_back(radiiArray[i]);
      this->RelativeRadii.push_back(radiiArray[i] / this->Extent);
    }
  }
}

/**
 * Get the different integration radii from the field as constant length array
 * for python wrapping
 * @param radiiArray: array of size 10 containing the radii. if less radii are
 * desired, fill the remaining entries with zeros
 */
void vtkComputeMoments::GetRadiiArray(double relativeRadiiArray[10])
{
  for (int i = 0; i < 10; ++i)
  {
    relativeRadiiArray[i] = 0;
  }
  for (size_t i = 0; i < this->Radii.size(); ++i)
  {
    relativeRadiiArray[i] = this->RelativeRadii.at(i);
  }
}

/**
 * Set the different relative integration radii from the field as constant
 * length array for python wrapping
 * @param radiiArray: array of size 10 containing the radii. if less radii are
 * desired, fill the remaining entries with zeros
 */
void vtkComputeMoments::SetRelativeRadiiArray(double relativeRadiiArray[10])
{
  this->Radii.resize(0);
  this->RelativeRadii.resize(0);
  for (int i = 0; i < 10; ++i)
  {
    if (relativeRadiiArray[i] == 0)
    {
      break;
    }
    else
    {
      this->Radii.push_back(relativeRadiiArray[i] * this->Extent);
      this->RelativeRadii.push_back(relativeRadiiArray[i]);
    }
  }
}

/**
 * Get the different relative integration radii from the field as constant
 * length array for python wrapping
 * @param radiiArray: array of size 10 containing the radii. if less radii are
 * desired, fill the remaining entries with zeros
 */
void vtkComputeMoments::GetRelativeRadiiArray(double radiiArray[10])
{
  for (int i = 0; i < 10; ++i)
  {
    radiiArray[i] = 0;
  }
  for (size_t i = 0; i < this->Radii.size(); ++i)
  {
    radiiArray[i] = this->Radii.at(i);
  }
}

/**
 * Find out the dimension and the date type of the field dataset.
 * @param field: function of which the moments are computed
 */
void vtkComputeMoments::InterpretField(vtkImageData* field)
{
  if (field->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkErrorMacro("The field does not contain any pointdata.");
    return;
  }
  if (this->NameOfPointData == "no name set by user")
  {
    this->NameOfPointData = field->GetPointData()->GetArrayName(0);
  }
  if (field->GetPointData()->GetArray(this->NameOfPointData.c_str()) == NULL)
  {
    vtkErrorMacro(
      "The field does not contain an array by the set name of " << this->NameOfPointData.c_str());
    return;
  }

  // dimension
  double bounds[6];
  field->GetBounds(bounds);
  if (bounds[5] - bounds[4] < 1e-10)
  {
    this->Dimension = 2;
  }
  else
  {
    this->Dimension = 3;
  }

  // extent
  this->Extent = bounds[1] - bounds[0];
  this->Extent = std::min(this->Extent, bounds[3] - bounds[2]);
  if (this->Dimension == 3)
  {
    this->Extent = std::min(this->Extent, bounds[5] - bounds[4]);
  }

  // default radii are 1/32, 1/16, 1/8 of the minimal spacial extent
  if (this->Radii.size() == 0)
  {
    for (int k = 5; k > 2; --k)
    {
      Radii.push_back(this->Extent / pow(2.0, k));
    }
  }

  // FieldRank, i.e. scalars, vectors, or matrices
  int numberOfComponents =
    field->GetPointData()->GetArray(this->NameOfPointData.c_str())->GetNumberOfComponents();
  if (numberOfComponents == 1)
  {
    this->FieldRank = 0;
  }
  else if (numberOfComponents == 2 || numberOfComponents == 3)
  {
    this->FieldRank = 1;
  }
  else if (numberOfComponents == 4 || numberOfComponents == 6 || numberOfComponents == 9)
  {
    this->FieldRank = 2;
  }
  else
  {
    vtkErrorMacro("field pointdata's number of components does not correspond "
                  "to 2D or 3D scalars, vectors, or matrices.");
    return;
  }
}

/**
 * Make sure that the user has not entered weird values.
 * @param field: function of which the moments are computed
 */
void vtkComputeMoments::CheckValidity(vtkImageData*)
{
  if (this->Order < 0 || this->Order > 5)
  {
    vtkErrorMacro("The order must be between 0 and 5.");
    return;
  }
  if (this->NumberOfIntegrationSteps < 0)
  {
    vtkErrorMacro("The number of integration steps must be positive.");
    return;
  }
  if (this->Radii.size() == 0)
  {
    vtkErrorMacro("The Radii must be positive.");
  }
  for (size_t i = 0; i < this->Radii.size(); ++i)
  {
    if (this->Radii.at(i) <= 0)
    {
      vtkErrorMacro("The Radii must be positive.  It is " << Radii.at(i));
      return;
    }
  }
}

/**
 * Build the output dataset.
 * @param grid: the uniform grid on which the moments are computed
 * @param output: this vtkImageData has the same topology as grid and will
 * contain numberOfFields scalar fields, each containing one moment at all
 * positions
 */
void vtkComputeMoments::BuildOutput(vtkImageData* grid, vtkImageData* output)
{
  output->CopyStructure(grid);
  // compute number of output moment fields per radius
  this->NumberOfBasisFunctions = 0;
  for (int k = 0; k < this->Order + 1; ++k)
  {
    this->NumberOfBasisFunctions += pow(this->Dimension, k + this->FieldRank);
  }
  this->NumberOfFields = this->NumberOfBasisFunctions * this->Radii.size();

  // vector of arrays for the moments. the name is the tensor indices
  for (size_t k = 0; k < this->Radii.size(); ++k)
  {
    for (size_t i = 0; i < this->NumberOfBasisFunctions; ++i)
    {
      vtkNew<vtkDoubleArray> array;
      std::string fieldName = "radius" + std::to_string(this->Radii.at(k)) + "index" +
        vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(
          i, this->Dimension, this->Order, this->FieldRank)
          .c_str();
      array->SetName(fieldName.c_str());
      array->SetNumberOfTuples(grid->GetNumberOfPoints());
      output->GetPointData()->AddArray(array);
    }
  }
}

/**
 * This Method is the main part that computes the moments.
 * @param radiusIndex: index of this radius in the radii vector
 * @param grid: the uniform grid on which the moments are computed
 * @param field: function of which the moments are computed
 * @param output: this vtkImageData has the same topology as grid and will contain numberOfFields
 * scalar fields, each containing one moment at all positions
 */
void vtkComputeMoments::Compute(size_t radiusIndex,
  vtkImageData* grid,
  vtkImageData* field,
  vtkImageData* output)
{
  // std::cout << "vtkComputeMoments::Compute \n";
  if (this->UseFFT)
  {
    /* Setup imagedata for kernel */
    vtkNew<vtkImageData> kernel;
    kernel->SetOrigin(0, 0, 0);
    kernel->SetSpacing(field->GetSpacing());

    int extArray[6] = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < this->Dimension; i++)
    {
      extArray[2 * i] = -this->Radii.at(radiusIndex) / kernel->GetSpacing()[i];
      extArray[2 * i + 1] = this->Radii.at(radiusIndex) / kernel->GetSpacing()[i];
    }
    kernel->SetExtent(extArray);

    vtkNew<vtkDoubleArray> kernelArray;
    kernelArray->SetName("kernel");
    kernelArray->SetNumberOfComponents(1);
    kernelArray->SetNumberOfTuples(kernel->GetNumberOfPoints());
    kernelArray->Fill(0.0);
    kernel->GetPointData()->SetScalars(kernelArray);

    /* Pad the field data */
    vtkSmartPointer<vtkImageData> paddedField =
      vtkMomentsHelper::padField(field, kernel, this->Dimension, this->NameOfPointData);

    /* Start setting up FFT for padded field data */
    const int* dims = paddedField->GetDimensions();
    int numPs = paddedField->GetNumberOfPoints();

    kiss_fft_cpx* fieldFFT = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs);

    // Create the kissfft forward and inverse plan
    // kiss_fftnd_alloc( dims, ndims, forward/inverse, 0, 0 )
    // dims: array of dimensions
    // ndims: number of dimensions
    // inverse: 0 = forward; 1 = inverse
    kiss_fftnd_cfg forwardFFT;
    kiss_fftnd_cfg inverseFFT;

    forwardFFT = kiss_fftnd_alloc(dims, this->Dimension, 0, 0, 0);
    inverseFFT = kiss_fftnd_alloc(dims, this->Dimension, 1, 0, 0);

    /* Initialize & Execute plan on fieldFFT */
    std::vector<kiss_fft_cpx*> dataComponents;
    int numberOfComponents =
      paddedField->GetPointData()->GetArray(this->NameOfPointData.c_str())->GetNumberOfComponents();
    for (int numComp = 0; numComp < numberOfComponents; numComp++)
    {
      dataComponents.push_back((kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs));

      // Fill in values for fieldFFT
      for (int j = 0; j < numPs; j++)
      {
        fieldFFT[j].r = paddedField->GetPointData()
                          ->GetArray(this->NameOfPointData.c_str())
                          ->GetTuple(j)[numComp];
        fieldFFT[j].i = 0.0;
      }
      kiss_fftnd(forwardFFT, fieldFFT, dataComponents[numComp]);
    }

    /* Start setting up FFT for kernel */
    kiss_fft_cpx* kernelFFT = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs);
    kiss_fft_cpx* k_freq = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs);
    kiss_fft_cpx* tmp_k_freq = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs);

    /* Start setting up IFFT for the result */
    kiss_fft_cpx* result = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * numPs);

    vtkNew<vtkImageData> fft_output;
    this->BuildOutput(field, fft_output);

    /*
     * Initialize kernel for each basis function
     * Initialize kernelFFT from the paddedKernel
     * Apply FFT on the kernelFFT
     * Initialize result from kernelFFT according to the Cross-Correlation Theorem
     * Apply IFFT on the result
     * Populate the output from the result with normalization
     */
    for (size_t i = 0; i < this->NumberOfBasisFunctions; i++)
    {
      double argument[3];
      kernelArray->Fill(0.0);

      std::vector<size_t> indices = vtkMomentsHelper::getTensorIndicesFromFieldIndex(
        i, this->Dimension, this->Order, this->FieldRank); // radiusIndex *
                                                           // this->NumberOfBasisFunctions + i); //
                                                           // given multiple radii this does not give
                                                           // correct answers
      int rank = indices.size();

      // Fill in values for kernel
      for (int ptId = 0; ptId < kernel->GetNumberOfPoints(); ++ptId)
      {
        kernel->GetPoint(ptId, argument);
        if (vtkMath::Norm(argument) <= this->Radii.at(radiusIndex))
        {
          double faktor = 1;
          for (int k = 0; k < rank - this->FieldRank; ++k)
          {
            faktor *= argument[indices.at(k)];
          }
          kernelArray->SetTuple1(ptId, faktor);
        }
      }

      vtkMomentsTensor dummyTensor = vtkMomentsTensor(this->Dimension, rank, this->FieldRank);
      int compIndex = dummyTensor.getFieldIndex(dummyTensor.getIndex(indices));

      // Pad the kernel
      vtkSmartPointer<vtkImageData> paddedKernel = vtkMomentsHelper::padKernel(kernel, paddedField);

      // Initialize & Execute plan on kernelFFT
      for (int j = 0; j < numPs; j++)
      {
        kernelFFT[j].r = paddedKernel->GetPointData()->GetScalars()->GetTuple1(j);
        kernelFFT[j].i = 0.0;
      }
      kiss_fftnd(forwardFFT, kernelFFT, k_freq);

      // Multiply the 2 FFT together according to Cross-Correlation Theorem
      for (int j = 0; j < numPs; j++)
      {
        // Cross-correction: complex conjugate the FFT of kernel
        tmp_k_freq[j].r = (dataComponents[compIndex][j].r * k_freq[j].r +
          dataComponents[compIndex][j].i * k_freq[j].i);
        tmp_k_freq[j].i = (dataComponents[compIndex][j].r * (-k_freq[j].i) +
          k_freq[j].r * dataComponents[compIndex][j].i);
      }
      kiss_fftnd(inverseFFT, tmp_k_freq, result);

      // Set the output array for each corresponding moment array
      const int* tmp = field->GetDimensions();
      std::vector<int> origSize = std::vector<int>(tmp, tmp + 3);

      tmp = paddedField->GetDimensions();
      std::vector<int> paddedSize = std::vector<int>(tmp, tmp + 3);

      // FFTW/KissFFT performs unnormalized FFT & IFFT
      double normalize = 1.0;
      double spacings = 1.0;
      for (int j = 0; j < this->Dimension; j++)
      {
        normalize *= dims[j];
        spacings *= field->GetSpacing()[j];
      }

      for (vtkIdType j = 0; j < field->GetNumberOfPoints(); j++)
      {
        std::vector<int> coord = vtkMomentsHelper::getCoord(j, origSize);

        fft_output->GetPointData()
          ->GetArray(radiusIndex * this->NumberOfBasisFunctions + i)
          ->SetTuple1(
            j, result[vtkMomentsHelper::getArrayIndex(coord, paddedSize)].r / normalize * spacings);
      }
    }

    /* Resample to grid structure if grid is defined */
    vtkNew<vtkResampleWithDataSet> resample;
    resample->SetInputData(grid);
    resample->SetSourceData(fft_output);
    resample->Update();
    vtkImageData* resampleData = vtkImageData::SafeDownCast(resample->GetOutput());

    for (size_t i = 0; i < this->NumberOfBasisFunctions; i++)
    {
      for (vtkIdType j = 0; j < output->GetNumberOfPoints(); j++)
      {
        output->GetPointData()
          ->GetArray(radiusIndex * this->NumberOfBasisFunctions + i)
          ->SetTuple1(j,
            resampleData->GetPointData()
              ->GetArray(radiusIndex * this->NumberOfBasisFunctions + i)
              ->GetTuple1(j));
      }
    }

    /* Cleaning up the fft objects */
    free(fieldFFT);
    free(kernelFFT);
    free(k_freq);
    free(tmp_k_freq);
    free(result);
    free(forwardFFT);
    free(inverseFFT);
  }
  else if (this->NumberOfIntegrationSteps == 0)
  {
    // std::cout << "imageData \n";
    for (int ptId = 0; ptId < grid->GetNumberOfPoints(); ++ptId)
    {
      //      cout<<ptId<<" "<<grid->GetPoint(ptId)[0]<<" "<<grid->GetPoint(ptId)[1]<<"
      //      "<<grid->GetPoint(ptId)[2]<<" "<<field->FindPoint(grid->GetPoint(ptId))<<endl;
      std::vector<vtkMomentsTensor> tensorVector =
        vtkMomentsHelper::allMomentsOrigResImageData(this->Dimension,
          this->Order,
          this->FieldRank,
          this->Radii.at(radiusIndex),
          field->FindPoint(grid->GetPoint(ptId)),
          field,
          this->NameOfPointData);
      // put them into the corresponding array
      for (size_t k = 0; k < tensorVector.size(); ++k)
      {
        for (size_t i = 0; i < tensorVector.at(k).size(); ++i)
        {
          output->GetPointData()
            ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(radiusIndex,
              tensorVector.at(k).getIndices(i),
              this->Dimension,
              this->FieldRank,
              this->NumberOfBasisFunctions))
            ->SetTuple1(ptId, tensorVector.at(k).get(i));
          // if (grid->GetPoint(ptId)[0] == 16 && grid->GetPoint(ptId)[1] == -3)

          //   std::cout << "radius=" << this->Radii.at(radiusIndex) << " fieldIndex="
          //             << vtkMomentsHelper::getFieldIndexFromTensorIndices(radiusIndex,
          //                  tensorVector.at(k).getIndices(i),
          //                  this->Dimension,
          //                  this->FieldRank,
          //                  this->NumberOfBasisFunctions)
          //             << " value=" << tensorVector.at(k).get(i) << "\n";
        }
      }
    }
  }
  else
  {
    // std::cout << "sampling active for stencil, because numberOfIntegratioSteps > 0 \n";
    vtkNew<vtkImageData> stencil;
    vtkMomentsHelper::BuildStencil(stencil,
      this->Radii.at(radiusIndex),
      this->NumberOfIntegrationSteps,
      this->Dimension,
      field,
      this->NameOfPointData);
    for (int ptId = 0; ptId < grid->GetNumberOfPoints(); ++ptId)
    {
      // Get the xyz coordinate of the point in the grid dataset
      double center[3];
      grid->GetPoint(ptId, center);
      if (!vtkMomentsHelper::isCloseToEdge(
            this->Dimension, ptId, this->Radii.at(radiusIndex), grid) &&
        vtkMomentsHelper::CenterStencil(
          center, field, stencil, this->NumberOfIntegrationSteps, this->NameOfPointData))
      {
        // get all the moments
        std::vector<vtkMomentsTensor> tensorVector = vtkMomentsHelper::allMoments(this->Dimension,
          this->Order,
          this->FieldRank,
          this->Radii.at(radiusIndex),
          center,
          stencil,
          this->NameOfPointData);

        // std::vector< vtkMomentsTensor > orthonormalTensorVector = this->orthonormalizeMoments(
        // this->Dimension, tensorVector, this->Radii.at(radiusIndex), stencil );
        //            tensorVector = orthonormalTensorVector;
        //            if(ptId == 100)
        //            {
        //                for( size_t i = 0; i < tensorVector.size(); ++i )
        //                {
        //                    tensorVector.at(i).print();
        //                }
        //                std::vector< vtkMomentsTensor > orthonormalTensorVector =
        //                orthonormalizeMoments( this->Dimension, tensorVector,
        //                this->Radii.at(radiusIndex) ); for( size_t i = 0; i < tensorVector.size();
        //                ++i )
        //                {
        //                    orthonormalTensorVector.at(i).print();
        //                }
        //            }

        // put them into the corresponding array
        for (size_t k = 0; k < tensorVector.size(); ++k)
        {
          for (size_t i = 0; i < tensorVector.at(k).size(); ++i)
          {
            output->GetPointData()
              ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(radiusIndex,
                tensorVector.at(k).getIndices(i),
                this->Dimension,
                this->FieldRank,
                this->NumberOfBasisFunctions))
              ->SetTuple1(ptId, tensorVector.at(k).get(i));
            // std::cout<<"radius="<<this->Radii.at(radiusIndex)<<"
            // fieldIndex="<<vtkMomentsHelper::getFieldIndexFromTensorIndices( radiusIndex,
            // tensorVector.at(k).getIndices(i), this->Dimension, this->FieldRank,
            // this->NumberOfBasisFunctions )<<" value="<<tensorVector.at(k).get(i)<<"\n";
          }
        }
      }
      else
      {
        for (size_t i = 0; i < NumberOfBasisFunctions; ++i)
        {
          output->GetPointData()
            ->GetArray(radiusIndex * NumberOfBasisFunctions + i)
            ->SetTuple1(ptId, 0.0);
        }
      }
      // cout<<ptId<<" center="<<center[0]<<" "<<center[1]<<" "<<center[2]<<"
      // "<<outPD->GetArray(0)->GetTuple(ptId)[0]<<"\n";
    }
  }
  //#ifdef MYDEBUG
  //    std::ostream stream(std::cout.rdbuf());
  //    std::cout<<"output=";
  //    output->PrintSelf(stream, vtkIndent(0));
  //    std::cout<<"\n";
  //#endif
}

int vtkComputeMoments::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector*)
{
  // We need to ask for the whole extent from this input.
  vtkInformation* momentsInfo = inputVector[0]->GetInformationObject(0);

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  momentsInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (momentsInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      momentsInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
      6);
  }

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  vtkInformation* gridInfo = inputVector[1]->GetInformationObject(0);

  gridInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  gridInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (gridInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
      6);
  }

  gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

/** main executive of the program, reads the input, calls the functions, and produces the utput.
 * @param request: ?
 * @param inputVector: the input information
 * @param outputVector: the output information
 */
int vtkComputeMoments::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* fieldInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* gridInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* field = vtkImageData::SafeDownCast(fieldInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* grid;
  if (gridInfo)
  {
    grid = vtkImageData::SafeDownCast(gridInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  else
  {
    grid = field;
  }
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (field)
  {
    this->InterpretField(field);
    this->CheckValidity(field);
    this->BuildOutput(grid, output);
    for (size_t radiusIndex = 0; radiusIndex < this->Radii.size(); ++radiusIndex)
    {
      this->Compute(radiusIndex, grid, field, output);
    }
  }
  return 1;
}
