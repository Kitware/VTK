## Fix HyperTree initialization routine

`vtkHyperTree::Initialize` used to incorrectly set up a internal data structure, causing incorrect iterations indices for `vtkHyperTreeGridNonOrientedUnlimitedSuperCursor::ToChild`.

This fixes the Gradient filter results using "Unlimited" mode for HTG, which now gives correct values.
