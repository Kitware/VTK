## Add translation mode to vtkResliceCursorWidgetLineRepresentation

Add a binding on Alt+LeftClick on the ResliceCursorWidget to allow the user to translate a single axis.
Add a TranslateAction to the ResliceCursorWidget to trigger this behavior.
Modify the ResliceCursorLineRepresentation so that the ManipulationMode drives which type of interaction is triggered.
