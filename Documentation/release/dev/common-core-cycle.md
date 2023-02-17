## Eliminate CommonDataModel dependency from CommonCore

The vtkInformationDataObjectKey class previously caused a hidden private
dependency of CommonCore on CommonDataModel.  This dependency has now been
eliminated.
