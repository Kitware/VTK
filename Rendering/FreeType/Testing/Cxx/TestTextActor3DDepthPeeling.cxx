// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests vtkTextActor3D with depth peeling.
// As this actor uses vtkImageActor underneath, it also tests vtkImageActor
// with depth peeling.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "TestSingleTextActorInternal.h"
#include "vtkTextActor3D.h"

int TestTextActor3DDepthPeeling(int argc, char* argv[])
{
  vtkNew<vtkTextActor3D> actor;
  actor->SetInput(TestTextActor::InputText().c_str());
  actor->SetPosition(3, 4, 5);

  return TestTextActor::CreatePipeline(argc, argv, actor, actor->GetTextProperty(), true);
}
