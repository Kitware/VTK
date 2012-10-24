/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuaternion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSetGet.h"
#include "vtkQuaternion.h"
#include "vtkMathUtilities.h"

// Pre-declarations of the test functions
static int TestQuaternionSetGet();
static int TestQuaternionNormalization();
static int TestQuaternionConjugationAndInversion();
static int TestQuaternionRotation();
static int TestQuaternionMatrixConversions();
static int TestQuaternionConversions();
static int TestQuaternionSlerp();

//----------------------------------------------------------------------------
int TestQuaternion(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  retVal += TestQuaternionSetGet();
  retVal += TestQuaternionNormalization();
  retVal += TestQuaternionConjugationAndInversion();
  retVal += TestQuaternionRotation();
  retVal += TestQuaternionMatrixConversions();
  retVal += TestQuaternionConversions();
  retVal += TestQuaternionSlerp();

  return retVal;
}

// Test if the access and set methods are valids
//----------------------------------------------------------------------------
int TestQuaternionSetGet() //use of vtkQuaternionf for this test
{
  int retVal = 0;
  //
  // Test out the general vector data types, give nice API and great memory use
  vtkQuaternionf qf(1.0f);
  float zeroArrayf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  qf.Set(zeroArrayf[0], zeroArrayf[1], zeroArrayf[2], zeroArrayf[3]);

  if (sizeof(qf) != sizeof(zeroArrayf))
    {
    // The two should be the same size and memory layout - error out if not
    std::cerr << "vtkQuaternionf should be the same size as float[4]."
        << std::endl
        << "sizeof(vtkQuaternionf) = " << sizeof(qf) << std::endl
        << "sizeof(float[4]) = " << sizeof(zeroArrayf) << std::endl;
    ++retVal;
    }
  if (qf.GetSize() != 4)
    {
    std::cerr << "Incorrect size of vtkQuaternionf, should be 4, but is "
        << qf.GetSize() << std::endl;
    ++retVal;
    }

  //
  // Test out vtkQuaternionf and ensure the various access methods are the same
  qf.Set(0.0f, 6.0f, 9.0f, 15.0f);
  if (qf.GetW() != qf[0]
      || !vtkMathUtilities::FuzzyCompare<float>(qf.GetW(), 0.0f))
    {
    std::cerr << "qf.GetW() should equal qf.GetData()[0] which should equal 0."
      << "\nqf.W() = " << qf.GetW() << std::endl
      << "qf[0] = " << qf[0] << std::endl;
    ++retVal;
    }
  if (qf.GetX() != qf[1]
      || !vtkMathUtilities::FuzzyCompare<float>(qf.GetX(), 6.0f))
    {
    std::cerr << "qf.GetX() should equal qf.GetData()[1] "
      << "which should equal 6.0. \nqf.GetX() = " << qf.GetX() << std::endl
      << "qf[1] = " << qf[1] << std::endl;
    ++retVal;
    }
  if (qf.GetY() != qf[2]
      || !vtkMathUtilities::FuzzyCompare<float>(qf.GetY(), 9.0f))
    {
    std::cerr << "qf.GetY() should equal qf.GetData()[2]"
      <<" which should equal 9.0.\nqf.GetY() = " << qf.GetY() <<std:: endl
      << "qf[2] = " << qf[2] << std::endl;
    ++retVal;
    }
  if (qf.GetZ() != qf[3]
      || !vtkMathUtilities::FuzzyCompare<float>(qf.GetZ(), 15.0f))
    {
    std::cerr << "qf.GetZ() should equal qf.GetData()[3] "
      << "which should equal 15.0.\nqf.Z() = " << qf.GetZ() << std::endl
      << "qf[3] = " << qf[3] << std::endl;
    ++retVal;
    }

  //
  // Assign the data to an float array and ensure the two ways of
  // referencing are the same.
  float *floatPtr = qf.GetData();
  for (int i = 0; i < 3; ++i)
    {
    if (qf[i] != floatPtr[i] || qf(i) != qf[i])
      {
      std::cerr << "Error: qf[i] != floatPtr[i]" << std::endl
          << "qf[i] = " << qf[i] << std::endl
          << "floatPtr[i] = " << floatPtr[i] << std::endl;
      ++retVal;
      }
    }

  //To and from float[4]
  float setArray[4] = {1.0, -38.0, 42.0, 0.0001};
  qf.Set(setArray);
  if (!qf.Compare( vtkQuaternionf(1.0, -38.0, 42.0, 0.0001), 0.0001))
    {
    std::cerr << "Error vtkQuaterniond::Set(float[4]) failed: "
      << qf << std::endl;
    ++retVal;
    }

  float arrayToCompare[4];
  qf.Get(arrayToCompare);
  for (int i = 0; i < 4; ++i)
    {
    if (!vtkMathUtilities::FuzzyCompare(setArray[i], arrayToCompare[i]))
      {
      std::cerr << "Error vtkQuaterniond::Get(float[4]) failed: "
        << setArray[i] << "!= " << arrayToCompare[i] << std::endl;
      ++retVal;
      }
    }

  return retVal;
}

// Test the normalize and normalized functions.
//----------------------------------------------------------------------------
int TestQuaternionNormalization() //This test use vtkQuaterniond
{
  int retVal = 0;

  vtkQuaterniond normy(1.0, 2.0, 3.0, 4.0);
  vtkQuaterniond normed = normy.Normalized();
  if (!normed.Compare(
        vtkQuaterniond(0.182574, 0.365148, 0.547723, 0.730297),
        0.0001))
    {
    std::cerr << "Error vtkQuaterniond::Normalized() failed: "
      << normed << std::endl;
    ++retVal;
    }
  normy.Normalize();
  if (!normy.Compare(normed, 0.0001))
    {
    std::cerr << "Error vtkQuaterniond::Normalize() failed: "
      << normy << std::endl;
    }
  if (!vtkMathUtilities::FuzzyCompare(normy.Norm(), 1.0, 0.0001))
    {
    std::cerr << "Normalized length should always be ~= 1.0, value is "
         << normy.Norm() << std::endl;
    ++retVal;
    }

  return retVal;
}

// This tests the conjugation and inversion at the same time.
// Since inversion depends on normalization, this will probably fail
// if TestQuaternionNormalisation() fails.
//----------------------------------------------------------------------------
int TestQuaternionConjugationAndInversion() //this test uses vtkQuaternionf
{
  int retVal = 0;

  //
  // Test conjugate and inverse  at the same time.
  // [inv(q) = conj(q)/norm2(q)]
  vtkQuaternionf toConjugate(2.0f);
  vtkQuaternionf conjugate = toConjugate.Conjugated();
  if (!conjugate.Compare(
        vtkQuaternionf(2.0f, -2.0f, -2.0f, -2.0f),
        0.0001))
    {
    std::cerr << "Error vtkQuaternionf::Conjugated() failed: "
      << conjugate << std::endl;
    ++retVal;
    }
  float squaredNorm = conjugate.SquaredNorm();
  vtkQuaternionf invToConjugate = conjugate / squaredNorm;
  if (!invToConjugate.Compare(
        vtkQuaternionf(0.125f, -0.125f, -0.125f, -0.125f),
        0.0001))
    {
    std::cerr << "Error vtkQuaternionf Divide by Scalar() failed: "
      << invToConjugate << std::endl;
    ++retVal;
    }

  vtkQuaternionf shouldBeIdentity = invToConjugate * toConjugate;
  vtkQuaternionf identity;
  identity.ToIdentity();
  if (!shouldBeIdentity.Compare(identity, 0.0001))
    {
    std::cerr << "Error vtkQuaternionf multiplication failed: "
      << shouldBeIdentity << std::endl;
    ++retVal;
    }
  toConjugate.Invert();
  if (!invToConjugate.Compare(toConjugate, 0.0001))
    {
    std::cerr << "Error vtkQuaternionf::Inverse failed: "
      << toConjugate << std::endl;
    ++retVal;
    }
  shouldBeIdentity.Invert();
  if (!shouldBeIdentity.Compare(identity, 0.0001))
    {
    std::cerr << "Error vtkQuaternionf::Inverse failed: "
      << shouldBeIdentity << std::endl;
    ++retVal;
    }

  return retVal;
}

// Test the rotations
//----------------------------------------------------------------------------
int TestQuaternionRotation() //this test uses vtkQuaterniond
{
  int retVal = 0;

  //
  //Test rotations
  vtkQuaterniond rotation;
  rotation.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(10.0),
    1.0, 1.0, 1.0);

  if (!rotation.Compare(
        vtkQuaterniond(0.996195, 0.0290519, 0.0290519, 0.0290519),
        0.0001))
    {
    std::cerr << "Error vtkQuaterniond::SetRotation Angle()"
      <<" and Axis() failed: "<< rotation << std::endl;
    ++retVal;
    }

  vtkQuaterniond secondRotation;
  secondRotation.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(-20.0),
    1.0, -1.0, 1.0);
  if (!secondRotation.Compare(
        vtkQuaterniond(0.984808, -0.0578827, 0.0578827, -0.0578827),
        0.0001))
    {
    std::cerr << "Error vtkQuaterniond::SetRotation Angle()"
      <<" and Axis() failed: "<< secondRotation << std::endl;
    ++retVal;
    }

  vtkQuaterniond resultRotation = rotation * secondRotation;
  double axis[3];
  double supposedAxis[3] = {-0.338805, 0.901731, -0.2685};
  double angle = resultRotation.GetRotationAngleAndAxis(axis);

  if (!vtkMathUtilities::FuzzyCompare(axis[0], supposedAxis[0], 0.0001)
    || !vtkMathUtilities::FuzzyCompare(axis[1], supposedAxis[1], 0.0001)
    || !vtkMathUtilities::FuzzyCompare(axis[2], supposedAxis[2], 0.0001))
    {
    std::cerr << "Error vtkQuaterniond::GetRotationAxis() failed: "
      << axis[0] << "  " << axis[1] << "  " << axis[2] << std::endl;
    ++retVal;
    }
  if (!vtkMathUtilities::FuzzyCompare(
        vtkMath::DegreesFromRadians(angle), 11.121, 0.0001))
    {
    std::cerr << "Error vtkQuaterniond::GetRotationAngle() failed: "
      << vtkMath::DegreesFromRadians(angle) << std::endl;
    ++retVal;
    }

  return retVal;
}

// Test the matrix conversions
//----------------------------------------------------------------------------
int TestQuaternionMatrixConversions() //this test uses vtkQuaternionf
{
  int retVal = 0;

  vtkQuaternionf quat;
  float M[3][3];
  M[0][0] = 0.98420;    M[0][1] = 0.17354;    M[0][2] = 0.03489;
  M[1][0] = -0.17327;   M[1][1] = 0.90415;    M[1][2] = 0.39049;
  M[2][0] = 0.03621;    M[2][1] = -0.39037;   M[2][2] = 0.91994;
  quat.FromMatrix3x3(M);

  if (!(quat.Compare(
          vtkQuaternionf(-0.975744, 0.200069, 0.000338168, 0.0888578),
          0.001)))
    {
    std::cerr << "Error vtkQuaternionf FromMatrix3x3 failed: "
      << quat << std::endl;
    ++retVal;
    }

  //an easy one, just to make sure !
  float newM[3][3];
  quat.ToMatrix3x3(newM);
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (!vtkMathUtilities::FuzzyCompare(M[i][j], newM[i][j], 0.001f))
        {
        std::cerr << "Error vtkQuaternionf ToMatrix3x3 failed: "
          << M[i][j] <<" != " << newM[i][j] << std::endl;
        ++retVal;
        }
      }
    }

  //Rotate -23 degrees around X
  M[0][0] = 1.0;  M[0][1] = 0.0;      M[0][2] = 0.0;
  M[1][0] = 0.0;  M[1][1] = 0.92050;  M[1][2] = 0.39073;
  M[2][0] = 0.0;  M[2][1] = -0.39073; M[2][2] = 0.92050;
  //Let's also make the quaternion
  quat.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(-23.0),
    1.0, 0.0, 0.0);

  //just in case, it makes another test
  vtkQuaternionf newQuat;
  newQuat.FromMatrix3x3(M);
  if (!(newQuat.Compare(quat, 0.00001)))
    {
    std::cerr << "Error vtkQuaternionf FromMatrix3x3 failed: "
      << newQuat <<" != " << quat << std::endl;
    ++retVal;
    }

  //And compare again !
  quat.ToMatrix3x3(newM);
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (!vtkMathUtilities::FuzzyCompare( M[i][j], newM[i][j], 0.001f))
        {
        std::cerr << "Error vtkQuaternionf ToMatrix3x3 failed: "
          << M[i][j] <<" != " << newM[i][j] << std::endl;
        ++retVal;
        }
      }
    }

  return retVal;
}

// Test the quaternion's conversions
//----------------------------------------------------------------------------
int TestQuaternionConversions() //this test uses vtkQuaterniond
{
  int retVal = 0;
  vtkQuaterniond quat(15.0, -3.0, 2.0, 0.001);

  // Logarithm
  vtkQuaterniond logQuat;
  logQuat = quat.UnitLog();
  if (!(logQuat.Compare(vtkQuaterniond(0, -0.378151, 0.252101, 0.00012),
                        0.00001)))
    {
    std::cerr << "Error vtkQuaterniond UnitLogQuaternion() failed: "
      << logQuat << std::endl;
    ++retVal;
    }

  // Exponential
  vtkQuaterniond expQuat = quat.UnitExp();
  if (!(expQuat.Compare(vtkQuaterniond(0.89075, -0.37815, 0.25210, 0.00012),
                        0.00001)))
    {
    std::cerr << "Error vtkQuaterniond UnitExpQuaternion() failed: "
      << expQuat << std::endl;
    ++retVal;
    }

  //To VTK
  vtkQuaterniond vtkQuat = quat.NormalizedWithAngleInDegrees();
  if (!(vtkQuat.Compare(
          vtkQuaterniond(55.709, -0.194461, 0.129641, 6.48204e-005),
          0.00001)))
    {
    std::cerr << "Error vtkQuaterniond UnitForVTKQuaternion() failed: "
      << vtkQuat << std::endl;
    ++retVal;
    }

  return retVal;
}

// Test the quaternion's slerp
//----------------------------------------------------------------------------
int TestQuaternionSlerp() //this test uses vtkQuaternionf
{
  int retVal = 0;

  vtkQuaternionf q0, q1;
  q0.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(-2.0),
                             -3.0, 2.0, 1.0);
  q1.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(183.0),
                             10.0, 0.0, 1.0);
  vtkQuaternionf halfQ1 = q0.Slerp(0.5, q1);
  halfQ1.Normalize();

  if (!(halfQ1.Compare(vtkQuaternionf(0.99444, 0.104907, -0.00254, 0.00883),
                        0.00001)))
    {
    std::cerr << "Error vtkQuaternionf Slerp() failed: "
      << halfQ1 << std::endl;
    ++retVal;
    }

  return retVal;
}
