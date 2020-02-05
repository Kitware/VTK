/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLagrangianParticle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianParticle.h"

#include "vtkDoubleArray.h"
#include "vtkLagrangianThreadedData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStaticCellLocator.h"

#include <iostream>

int TestLagrangianParticle(int, char*[])
{
  int nvar = 7;
  int seedId = 0;

  vtkNew<vtkDoubleArray> vel;
  vel->SetNumberOfComponents(3);
  double velTmp[3] = { 17, 17, 17 };
  vel->InsertNextTuple(velTmp);

  vtkNew<vtkPointData> pd;
  pd->AddArray(vel);
  vtkIdType particleCounter = 0;

  std::unique_ptr<vtkLagrangianThreadedData> data(new vtkLagrangianThreadedData);
  vtkNew<vtkGenericCell> cell;
  vtkLagrangianParticle part(nvar, seedId, particleCounter, seedId, 0, pd, 8, 3);
  part.SetThreadedData(data.get());
  particleCounter++;
  if (nvar != part.GetNumberOfVariables())
  {
    std::cerr << "Incorrect number of variables" << std::endl;
    return EXIT_FAILURE;
  }
  double* p = part.GetPrevEquationVariables();
  double* x = part.GetEquationVariables();
  double* f = part.GetNextEquationVariables();
  if (p != part.GetPrevPosition())
  {
    std::cerr << "Prev Position and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (x != part.GetPosition())
  {
    std::cerr << "Position and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (f != part.GetNextPosition())
  {
    std::cerr << "Next Position and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (p + 3 != part.GetPrevVelocity())
  {
    std::cerr << "Prev Velocity and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (x + 3 != part.GetVelocity())
  {
    std::cerr << "Velocity and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (f + 3 != part.GetNextVelocity())
  {
    std::cerr << "Next Velocity and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (p + 6 != part.GetPrevUserVariables())
  {
    std::cerr << "Prev User and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (x + 6 != part.GetUserVariables())
  {
    std::cerr << "User and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (f + 6 != part.GetNextUserVariables())
  {
    std::cerr << "Next User and Equation variables should be the same" << std::endl;
    return EXIT_FAILURE;
  }
  if (part.GetNumberOfVariables() != 7)
  {
    std::cerr << "Unexpected Number of user variables" << std::endl;
    return EXIT_FAILURE;
  }
  if (part.GetNumberOfUserVariables() != 0)
  {
    std::cerr << "Unexpected Number of user variables" << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < nvar; i++)
  {
    x[i] = -i;
    f[i] = i;
  }

  double& stepTime = part.GetStepTimeRef();
  stepTime = 2.13;

  std::unique_ptr<vtkLagrangianParticle> part2(part.NewParticle(particleCounter));
  particleCounter++;
  std::unique_ptr<vtkLagrangianParticle> part3(part2->CloneParticle());
  part.MoveToNextPosition();

  double* x2 = part2->GetEquationVariables();
  double* x3 = part3->GetEquationVariables();
  double* p2 = part2->GetPrevEquationVariables();
  double* p3 = part3->GetPrevEquationVariables();
  double* f2 = part2->GetNextEquationVariables();
  double* f3 = part3->GetNextEquationVariables();
  double m3 = part3->GetPositionVectorMagnitude();

  for (int i = 0; i < nvar; i++)
  {
    if (x[i] != i || x2[i] != i || x3[i] != i)
    {
      std::cerr << "Incorrect equation variables: " << x[i] << " " << x2[i] << " " << x3[i] << " "
                << i << std::endl;
      return EXIT_FAILURE;
    }
    if (f[i] != 0 || f2[i] != 0 || f3[i] != 0)
    {
      std::cerr << "Incorrect next equation variables" << std::endl;
      return EXIT_FAILURE;
    }
    if (p[i] != -i || p2[i] != -i || p3[i] != -i)
    {
      std::cerr << "Incorrect prev equation variables" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (std::abs(m3 - 2.23606797749979) > 10e-6)
  {
    std::cerr << "Unexpected Position Vector Magnitude" << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetNumberOfSteps() != 1 || part2->GetNumberOfSteps() != 1 ||
    part3->GetNumberOfSteps() != 1)
  {
    std::cerr << "Incorrect Number of step" << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetSeedId() != seedId || part2->GetSeedId() != seedId || part3->GetSeedId() != seedId)
  {
    std::cerr << "Incorrect SeedId : " << part.GetSeedId() << " " << part2->GetSeedId() << " "
              << part3->GetSeedId() << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetId() != 0)
  {
    std::cerr << "Incorrect Id in part : " << part.GetId() << std::endl;
    return EXIT_FAILURE;
  }
  if (part2->GetId() != 1 || part3->GetId() != 1)
  {
    std::cerr << "Incorrect Id in part2 or part3: " << part2->GetId() << " " << part3->GetId()
              << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetParentId() != -1)
  {
    std::cerr << "Incorrect Parent Id in part : " << part.GetParentId() << std::endl;
    return EXIT_FAILURE;
  }
  part.SetParentId(0);
  if (part.GetParentId() != 0)
  {
    std::cerr << "SetParentId does not seem to work" << std::endl;
    return EXIT_FAILURE;
  }

  if (part2->GetParentId() != 0 || part3->GetParentId() != 0)
  {
    std::cerr << "Incorrect Parent Id in part2 or part3" << part2->GetParentId() << " "
              << part3->GetParentId() << std::endl;
    return EXIT_FAILURE;
  }
  if (part.GetSeedData()->GetArray(0)->GetComponent(part.GetSeedArrayTupleIndex(), 0) != 17 ||
    part2->GetSeedData()->GetArray(0)->GetComponent(part2->GetSeedArrayTupleIndex(), 0) != 17 ||
    part3->GetSeedData()->GetArray(0)->GetComponent(part3->GetSeedArrayTupleIndex(), 0) != 17)
  {
    std::cerr << "Incorrect Seed data " << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkStaticCellLocator> locator;
  vtkNew<vtkPolyData> poly;
  int cellId = 17;
  double pos[3] = { 0, 0, 0 };
  part.SetLastCell(locator, poly, cellId, pos);
  if (part.GetLastLocator() != locator || part.GetLastDataSet() != poly ||
    part.GetLastCellId() != cellId)
  {
    std::cerr << "Incorrect LastCellId or LastDataSet or LastLocator" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_TERMINATED);
  if (part.GetTermination() != vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_TERMINATED)
  {
    std::cerr << "Incorrect Termination" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_TERMINATED);
  if (part.GetInteraction() != vtkLagrangianParticle::SURFACE_INTERACTION_TERMINATED)
  {
    std::cerr << "Incorrect Interaction" << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetIntegrationTime() != 2.13 || part2->GetIntegrationTime() != 2.13 ||
    part3->GetIntegrationTime() != 2.13)
  {
    std::cerr << "Incorrect Step Time or Integration Time: " << part.GetIntegrationTime() << " "
              << part2->GetIntegrationTime() << " " << part3->GetIntegrationTime() << std::endl;
    return EXIT_FAILURE;
  }

  if (part.GetPrevIntegrationTime() != 0 || part2->GetPrevIntegrationTime() != 0 ||
    part3->GetPrevIntegrationTime() != 0)
  {
    std::cerr << "Incorrect Prev Integration Time" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetIntegrationTime(7.13);
  if (part.GetIntegrationTime() != 7.13)
  {
    std::cerr << "SetIntegrationTime does not seem to work" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetUserFlag(17);
  if (part.GetUserFlag() != 17)
  {
    std::cerr << "UserFlag does not seem to work" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetPInsertPreviousPosition(true);
  if (!part.GetPInsertPreviousPosition())
  {
    std::cerr << "PInsertPreviousPosition does not seem to work" << std::endl;
    return EXIT_FAILURE;
  }

  part.SetPManualShift(true);
  if (!part.GetPManualShift())
  {
    std::cerr << "PManualShift does not seem to work" << std::endl;
    return EXIT_FAILURE;
  }

  part.PrintSelf(std::cout, vtkIndent(0));

  particleCounter = 0;
  vtkLagrangianParticle part4(nvar, seedId, particleCounter, seedId, 0, pd, 8, 17);
  part4.SetThreadedData(data.get());
  particleCounter++;
  std::unique_ptr<vtkLagrangianParticle> part5(vtkLagrangianParticle::NewInstance(
    nvar, seedId, particleCounter, seedId, 0.17, pd, 8, 7, 17, 0.13));
  particleCounter++;
  if (part4.GetId() != 0)
  {
    std::cerr << "Incorrect Id in part4 : " << part4.GetId() << ". Particle Id problems."
              << std::endl;
    return EXIT_FAILURE;
  }
  if (part5->GetId() != 1)
  {
    std::cerr << "Incorrect Id in part5 : " << part5->GetId() << ". Particle Id problems."
              << std::endl;
    return EXIT_FAILURE;
  }
  if (part5->GetNumberOfSteps() != 17)
  {
    std::cerr << "Incorrect NumberOfSteps in part5." << std::endl;
    return EXIT_FAILURE;
  }
  if (part5->GetIntegrationTime() != 0.17)
  {
    std::cerr << "Incorrect Integration Time in part5." << std::endl;
    return EXIT_FAILURE;
  }
  if (part5->GetPrevIntegrationTime() != 0.13)
  {
    std::cerr << "Incorrect Previous Integration Time in part5." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
