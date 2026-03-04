// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGLTFImporter.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <iostream>

namespace
{
std::string expectedScene = R"(<?xml version="1.0"?>
<assembly type="vtkDataAssembly" version="1.0" id="0">
  <Bishop_W2 id="1" label="Bishop_W2">
    <Bishop_Shared id="2" parent_node_name="Bishop_W2" flat_actor_id="0" label="Bishop_Shared" />
  </Bishop_W2>
  <Bishop_W1 id="3" label="Bishop_W1">
    <Bishop_Shared id="4" parent_node_name="Bishop_W1" flat_actor_id="1" label="Bishop_Shared" />
  </Bishop_W1>
  <Bishop_B2 id="5" label="Bishop_B2">
    <Bishop_Shared id="6" parent_node_name="Bishop_B2" flat_actor_id="2" label="Bishop_Shared" />
  </Bishop_B2>
  <Bishop_B1 id="7" label="Bishop_B1">
    <Bishop_Shared id="8" parent_node_name="Bishop_B1" flat_actor_id="3" label="Bishop_Shared" />
  </Bishop_B1>
  <Knight_W2 id="9" label="Knight_W2">
    <Knight_Shared id="10" parent_node_name="Knight_W2" flat_actor_id="4" label="Knight_Shared" />
  </Knight_W2>
  <Knight_W1 id="11" label="Knight_W1">
    <Knight_Shared id="12" parent_node_name="Knight_W1" flat_actor_id="5" label="Knight_Shared" />
  </Knight_W1>
  <Knight_B2 id="13" label="Knight_B2">
    <Knight_Shared id="14" parent_node_name="Knight_B2" flat_actor_id="6" label="Knight_Shared" />
  </Knight_B2>
  <Knight_B1 id="15" label="Knight_B1">
    <Knight_Shared id="16" parent_node_name="Knight_B1" flat_actor_id="7" label="Knight_Shared" />
  </Knight_B1>
  <Castle_W2 id="17" label="Castle_W2">
    <Castle_Shared id="18" parent_node_name="Castle_W2" flat_actor_id="8" label="Castle_Shared" />
  </Castle_W2>
  <Castle_W1 id="19" label="Castle_W1">
    <Castle_Shared id="20" parent_node_name="Castle_W1" flat_actor_id="9" label="Castle_Shared" />
  </Castle_W1>
  <Castle_B2 id="21" label="Castle_B2">
    <Castle_Shared id="22" parent_node_name="Castle_B2" flat_actor_id="10" label="Castle_Shared" />
  </Castle_B2>
  <Castle_B1 id="23" label="Castle_B1">
    <Castle_Shared id="24" parent_node_name="Castle_B1" flat_actor_id="11" label="Castle_Shared" />
  </Castle_B1>
  <Pawn_Body_B8 id="25" label="Pawn_Body_B8">
    <Pawn_Body_Shared id="26" parent_node_name="Pawn_Body_B8" flat_actor_id="12" label="Pawn_Body_Shared" />
    <Pawn_Top_B8 id="27" label="Pawn_Top_B8">
      <Pawn_Top_Shared id="28" parent_node_name="Pawn_Top_B8" flat_actor_id="13" label="Pawn_Top_Shared" />
    </Pawn_Top_B8>
  </Pawn_Body_B8>
  <Pawn_Body_B7 id="29" label="Pawn_Body_B7">
    <Pawn_Body_Shared id="30" parent_node_name="Pawn_Body_B7" flat_actor_id="14" label="Pawn_Body_Shared" />
    <Pawn_Top_B7 id="31" label="Pawn_Top_B7">
      <Pawn_Top_Shared id="32" parent_node_name="Pawn_Top_B7" flat_actor_id="15" label="Pawn_Top_Shared" />
    </Pawn_Top_B7>
  </Pawn_Body_B7>
  <Pawn_Body_B6 id="33" label="Pawn_Body_B6">
    <Pawn_Body_Shared id="34" parent_node_name="Pawn_Body_B6" flat_actor_id="16" label="Pawn_Body_Shared" />
    <Pawn_Top_B6 id="35" label="Pawn_Top_B6">
      <Pawn_Top_Shared id="36" parent_node_name="Pawn_Top_B6" flat_actor_id="17" label="Pawn_Top_Shared" />
    </Pawn_Top_B6>
  </Pawn_Body_B6>
  <Pawn_Body_B5 id="37" label="Pawn_Body_B5">
    <Pawn_Body_Shared id="38" parent_node_name="Pawn_Body_B5" flat_actor_id="18" label="Pawn_Body_Shared" />
    <Pawn_Top_B5 id="39" label="Pawn_Top_B5">
      <Pawn_Top_Shared id="40" parent_node_name="Pawn_Top_B5" flat_actor_id="19" label="Pawn_Top_Shared" />
    </Pawn_Top_B5>
  </Pawn_Body_B5>
  <Pawn_Body_B4 id="41" label="Pawn_Body_B4">
    <Pawn_Body_Shared id="42" parent_node_name="Pawn_Body_B4" flat_actor_id="20" label="Pawn_Body_Shared" />
    <Pawn_Top_B4 id="43" label="Pawn_Top_B4">
      <Pawn_Top_Shared id="44" parent_node_name="Pawn_Top_B4" flat_actor_id="21" label="Pawn_Top_Shared" />
    </Pawn_Top_B4>
  </Pawn_Body_B4>
  <Pawn_Body_B3 id="45" label="Pawn_Body_B3">
    <Pawn_Body_Shared id="46" parent_node_name="Pawn_Body_B3" flat_actor_id="22" label="Pawn_Body_Shared" />
    <Pawn_Top_B3 id="47" label="Pawn_Top_B3">
      <Pawn_Top_Shared id="48" parent_node_name="Pawn_Top_B3" flat_actor_id="23" label="Pawn_Top_Shared" />
    </Pawn_Top_B3>
  </Pawn_Body_B3>
  <Pawn_Body_B2 id="49" label="Pawn_Body_B2">
    <Pawn_Body_Shared id="50" parent_node_name="Pawn_Body_B2" flat_actor_id="24" label="Pawn_Body_Shared" />
    <Pawn_Top_B2 id="51" label="Pawn_Top_B2">
      <Pawn_Top_Shared id="52" parent_node_name="Pawn_Top_B2" flat_actor_id="25" label="Pawn_Top_Shared" />
    </Pawn_Top_B2>
  </Pawn_Body_B2>
  <Pawn_Body_B1 id="53" label="Pawn_Body_B1">
    <Pawn_Body_Shared id="54" parent_node_name="Pawn_Body_B1" flat_actor_id="26" label="Pawn_Body_Shared" />
    <Pawn_Top_B1 id="55" label="Pawn_Top_B1">
      <Pawn_Top_Shared id="56" parent_node_name="Pawn_Top_B1" flat_actor_id="27" label="Pawn_Top_Shared" />
    </Pawn_Top_B1>
  </Pawn_Body_B1>
  <Pawn_Body_W8 id="57" label="Pawn_Body_W8">
    <Pawn_Body_Shared id="58" parent_node_name="Pawn_Body_W8" flat_actor_id="28" label="Pawn_Body_Shared" />
    <Pawn_Top_W8 id="59" label="Pawn_Top_W8">
      <Pawn_Top_Shared id="60" parent_node_name="Pawn_Top_W8" flat_actor_id="29" label="Pawn_Top_Shared" />
    </Pawn_Top_W8>
  </Pawn_Body_W8>
  <Pawn_Body_W7 id="61" label="Pawn_Body_W7">
    <Pawn_Body_Shared id="62" parent_node_name="Pawn_Body_W7" flat_actor_id="30" label="Pawn_Body_Shared" />
    <Pawn_Top_W7 id="63" label="Pawn_Top_W7">
      <Pawn_Top_Shared id="64" parent_node_name="Pawn_Top_W7" flat_actor_id="31" label="Pawn_Top_Shared" />
    </Pawn_Top_W7>
  </Pawn_Body_W7>
  <Pawn_Body_W6 id="65" label="Pawn_Body_W6">
    <Pawn_Body_Shared id="66" parent_node_name="Pawn_Body_W6" flat_actor_id="32" label="Pawn_Body_Shared" />
    <Pawn_Top_W6 id="67" label="Pawn_Top_W6">
      <Pawn_Top_Shared id="68" parent_node_name="Pawn_Top_W6" flat_actor_id="33" label="Pawn_Top_Shared" />
    </Pawn_Top_W6>
  </Pawn_Body_W6>
  <Pawn_Body_W5 id="69" label="Pawn_Body_W5">
    <Pawn_Body_Shared id="70" parent_node_name="Pawn_Body_W5" flat_actor_id="34" label="Pawn_Body_Shared" />
    <Pawn_Top_W5 id="71" label="Pawn_Top_W5">
      <Pawn_Top_Shared id="72" parent_node_name="Pawn_Top_W5" flat_actor_id="35" label="Pawn_Top_Shared" />
    </Pawn_Top_W5>
  </Pawn_Body_W5>
  <Pawn_Body_W4 id="73" label="Pawn_Body_W4">
    <Pawn_Body_Shared id="74" parent_node_name="Pawn_Body_W4" flat_actor_id="36" label="Pawn_Body_Shared" />
    <Pawn_Top_W4 id="75" label="Pawn_Top_W4">
      <Pawn_Top_Shared id="76" parent_node_name="Pawn_Top_W4" flat_actor_id="37" label="Pawn_Top_Shared" />
    </Pawn_Top_W4>
  </Pawn_Body_W4>
  <Pawn_Body_W3 id="77" label="Pawn_Body_W3">
    <Pawn_Body_Shared id="78" parent_node_name="Pawn_Body_W3" flat_actor_id="38" label="Pawn_Body_Shared" />
    <Pawn_Top_W3 id="79" label="Pawn_Top_W3">
      <Pawn_Top_Shared id="80" parent_node_name="Pawn_Top_W3" flat_actor_id="39" label="Pawn_Top_Shared" />
    </Pawn_Top_W3>
  </Pawn_Body_W3>
  <Pawn_Body_W2 id="81" label="Pawn_Body_W2">
    <Pawn_Body_Shared id="82" parent_node_name="Pawn_Body_W2" flat_actor_id="40" label="Pawn_Body_Shared" />
    <Pawn_Top_W2 id="83" label="Pawn_Top_W2">
      <Pawn_Top_Shared id="84" parent_node_name="Pawn_Top_W2" flat_actor_id="41" label="Pawn_Top_Shared" />
    </Pawn_Top_W2>
  </Pawn_Body_W2>
  <Pawn_Body_W1 id="85" label="Pawn_Body_W1">
    <Pawn_Body_Shared id="86" parent_node_name="Pawn_Body_W1" flat_actor_id="42" label="Pawn_Body_Shared" />
    <Pawn_Top_W1 id="87" label="Pawn_Top_W1">
      <Pawn_Top_Shared id="88" parent_node_name="Pawn_Top_W1" flat_actor_id="43" label="Pawn_Top_Shared" />
    </Pawn_Top_W1>
  </Pawn_Body_W1>
  <Chessboard id="89" label="Chessboard">
    <Chessboard id="90" parent_node_name="Chessboard" flat_actor_id="44" label="Chessboard" />
  </Chessboard>
  <Queen_W id="91" label="Queen_W">
    <Queen_Shared id="92" parent_node_name="Queen_W" flat_actor_id="45" label="Queen_Shared" />
  </Queen_W>
  <Queen_B id="93" label="Queen_B">
    <Queen_Shared id="94" parent_node_name="Queen_B" flat_actor_id="46" label="Queen_Shared" />
  </Queen_B>
  <King_W id="95" label="King_W">
    <King_Shared id="96" parent_node_name="King_W" flat_actor_id="47" label="King_Shared" />
  </King_W>
  <King_B id="97" label="King_B">
    <King_Shared id="98" parent_node_name="King_B" flat_actor_id="48" label="King_Shared" />
  </King_B>
</assembly>
)";
}

int TestGLTFImporterHierarchy(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  auto hierarchy = importer->GetSceneHierarchy();
  if (hierarchy == nullptr || hierarchy->GetNumberOfChildren(0) == 0)
  {
    std::cerr << "ERROR: scene hierarchy cannot be null!\n";
    return 1;
  }
  const std::string serializedXML = hierarchy->SerializeToXML(vtkIndent(2));
  if (serializedXML != expectedScene)
  {
    std::cerr << "ERROR: generated scene hierarchy doesn't match expected result!\n";
    std::cerr << "Generated: \n"
              << serializedXML << "---------\n"
              << "Expected: \n"
              << expectedScene << "\n---------\n";
    return 1;
  }

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
