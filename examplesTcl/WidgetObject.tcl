# These procs allow widgets to behave like objects with their own
# state variables of processing objects.


# generate a "unique" name for a widget variable
proc GenerateWidgetVariable {widget varName} {
   regsub -all {\.} $widget "_" base

   return "$varName$base"
}

# returns an object which will be associated with a widget
# no error checking
proc NewWidgetObject {widget type varName} {
   set var [GenerateWidgetVariable $widget $varName]
   # create the vtk object
   $type $var

   return $var
}

# returns the name of an object previously created by NewWidgetObject
proc GetWidgetObject {widget varName} {
   return [GenerateWidgetVariable $widget $varName]
}

# sets the value of a widget variable
proc SetWidgetVariableValue {widget varName value} {
   set var [GenerateWidgetVariable $widget $varName]
   global $var
   set $var $value
}

# This proc has alway eluded me.
proc GetWidgetVariableValue {widget varName} {
   set var [GenerateWidgetVariable $widget $varName]
   global $var
   set temp ""
   catch {eval "set temp [format {$%s} $var]"}

   return $temp
}


