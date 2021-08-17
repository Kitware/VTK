## Add translation mode to vtkResliceCursorWidgetLineRepresentation

Added the function `SetTranslationMode(bool isTranslationMode)` to the vtkResliceCursorWidgetLineRepresentation, as well as a bool member TranslationMode (false by default).
Modified the ResliceCursorWidget so that using Alt + Left Click allows the user to translate a single axis.
