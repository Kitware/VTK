
# A method to make an instance of a class with a unique name.
proc new {className} {
   set counterName [format {%sCounter} $className]
   global $counterName
   if {[info exists $counterName] == 0} {
      set $counterName 0
   }
   # Choose a name that is not being used
   set instanceName [format {%s%d} $className [incr $counterName]]
   while {[info commands $instanceName] != ""} {
      set instanceName [format {%s%d} $className [incr $counterName]]
   }
   # make the vtk object
   $className $instanceName

   return $instanceName
}