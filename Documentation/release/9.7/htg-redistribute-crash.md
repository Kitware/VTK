## vtkHyperTreeGridRedistribute: fix a crash when the input has no valid trees

Fix a crash that happened in the HTG redistribute filter when the input HyperTreeGrid had no valid tree in any rank, which can happen when every tree is masked in every process.
