// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers rendering of a text actor with depth peeling.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "TestSingleTextActorInternal.h"

#include "vtkNew.h"
#include "vtkTextActor.h"

int TestTextActorDepthPeeling(int argc, char* argv[])
{
  vtkNew<vtkTextActor> actor;
  actor->SetInput(TestTextActor::InputText().c_str());
  actor->SetDisplayPosition(150, 150);

  return TestTextActor::CreatePipeline(argc, argv, actor, actor->GetTextProperty(), true);
}
