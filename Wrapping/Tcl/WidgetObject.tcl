# These procs allow widgets to behave like objects with their own
# state variables of processing objects.


# generate a "unique" name for a widget variable
proc GetWidgetVariable {widget varName} {
   regsub -all {\.} $widget "_" base

   return "$varName$base"
}

# returns an object which will be associated with a widget
# A convienience method that creates a name for you  
# based on the widget name and varible value/
proc NewWidgetObject {widget type varName} {
   set var "[GetWidgetVariable $widget $varName]_Object"
   # create the vtk object
   $type $var

   # It is better to keep interface consistent
   # setting objects as variable values, and NewWidgetObject.
   SetWidgetVariableValue $widget $varName $var

   return $var
}

# obsolete!!!!!!!
# returns the same thing as GetWidgetVariableValue
proc GetWidgetObject {widget varName} {
   puts "Warning: obsolete call: GetWidgetObject"
   puts "Please use GetWidgetVariableValue"
   return "[GetWidgetVariable $widget $varName]_Object"
}

# sets the value of a widget variable
proc SetWidgetVariableValue {widget varName value} {
   set var [GetWidgetVariable $widget $varName]
   global $var
   set $var $value
}

# This proc has alway eluded me.
proc GetWidgetVariableValue {widget varName} {
   set var [GetWidgetVariable $widget $varName]
   global $var
   set temp ""
   catch {eval "set temp [format {$%s} $var]"}

   return $temp
}


