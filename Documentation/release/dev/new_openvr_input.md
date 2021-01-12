# Update the OpenVR input model to support actions

OpenVR has updated its input model to be action based and now supports
action binding and customization within the OpenVR user interface. This
includes controller labeling and user configuration. As such, we reworked
the device based event model to also support actions.

Part of this change involved updates to vtkEventData.h to add the notion of
"Any" as a device or input so that event handlers could look for a select
event from any input or any device for example. This also resulted in a
number of new event ids such as menu3d, clip3d, etc and the old event data
model used a new subclass for each id. We changed this to instead use a
method to set the event id type.

Once these changes were done we updated widget event code to be based off a
select3d event as opposed to a specific hardware device such as right
trigger. We updated event dispatch code to better handle event id. And
update widget event processing as needed to track move events based on the
original device that initiated to action.

If you have your own custom 3D event handling code or widgets, then you can
look at the changes made in this MR
https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7557#e8d22b8c27ce72ddec1110556087c6bd8d15fbec
to see how to update your code. The distance widget is a good example to
work from.
