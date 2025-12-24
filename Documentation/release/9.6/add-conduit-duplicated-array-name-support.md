# Add support for cell array and point array with the same name in Conduit

VTK now correctly converts a data object with two arrays that have the same name to a Conduit node.
The two arrays (points and cells) are renamed to arrayName_vertex and arrayName_element in the Conduit node only when a name conflict occurs.
Otherwise, array names are not changed. The original name can be found in `field/myArray/display_name`.
Thus, storing both ghost points and ghost cells in a Conduit node is now possible.
