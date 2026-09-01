## Add score based class choice for object factory preferences

VTK object factory preferences now chooses the VTK class based on a score.

The overrides attributes are now all compared against the preferences and a score is computed for all available class overrides. The class with the highest score is chosen for instantiation. If multiple classes have the same highest score, the preference value order determines the priority.

VTK documentation has been changed in the [runtime settings section](https://docs.vtk.org/en/latest/advanced/runtime_settings.html#runtime-settings) to explain the new preference system.
