## Multithread vtkExtractSelection

vtkExtractSelection's ExtractSelectedCells and ExtractSelectionPoints has been multithreaded.

Also, vtkSelection's Union and Subtract have been improved by using internal iterators.

Finally. vtkSelectionNode's EqualProperties has been improved by using static_cast instead of SafeDownCast, only when
it's needed.
