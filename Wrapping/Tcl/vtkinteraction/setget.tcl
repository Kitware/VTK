namespace eval ::vtk {

    namespace export *

    # -------------------------------------------------------------------
    # Some functions that can be used to associate variables to
    # widgets without polluting the global space

    variable gvars

    # Generate a "unique" name for a widget variable

    proc get_widget_variable {widget var_name} {
        variable gvars
        return "gvars($widget,vars,$var_name)"
    }

    # Set the value of a widget variable

    proc set_widget_variable_value {widget var_name value} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        set $var $value
    }

    proc unset_widget_variable {widget var_name} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        if {[info exists $var]} {
            unset $var
        }
    }

    # Get the value of a widget variable ("" if undef)

    proc get_widget_variable_value {widget var_name} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        if {[info exists $var]} {
            return [expr $$var]
        } else {
            return ""
        }
    }

    # Return an object which will be associated with a widget

    proc new_widget_object {widget type var_name} {
        variable gvars
        set var [get_widget_variable $widget "${var_name}_obj"]
        $type $var
        set_widget_variable_value $widget $var_name $var
        return $var
    }
}

# Handle deprecated calls

proc GetWidgetVariable {widget varName} {
    puts stderr "GetWidgetVariable is deprecated. Please use ::vtk::get_widget_variable instead"
    return [::vtk::get_widget_variable $widget $varName]
}

proc SetWidgetVariableValue {widget varName value} {
    puts stderr "SetWidgetVariableValue is deprecated. Please use ::vtk::set_widget_variable_value instead"
    ::vtk::set_widget_variable_value $widget $varName $value
}

proc GetWidgetVariableValue {widget varName} {
    puts stderr "GetWidgetVariableValue is deprecated. Please use ::vtk::get_widget_variable_value instead"
    return [::vtk::get_widget_variable_value $widget $varName]
}

proc NewWidgetObject {widget type varName} {
    puts stderr "NewWidgetObject is deprecated. Please use ::vtk::new_widget_object instead"
    return [::vtk::new_widget_object $widget $type $varName]
}
