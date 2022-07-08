## Add support for removing layers to selection expansion

Support for removing layers when expanding selection has been
added. The feature itself has been added to vtkExpandMarkedElements
with RemoveSeed and RemoveIntermediateLayers boolean members.
Using these flags, the initial selection seed of the expansion can be removed
as well as the intermediate layers, keeping only the final expansion layer.

The support has also been added to other class in the selection stack,
including vtkSelectionSource, vtkSelector and dedicated information keys
have also been added.
