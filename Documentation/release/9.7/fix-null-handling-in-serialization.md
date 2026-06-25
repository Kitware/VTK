# Pass through nullptr objects as JSON null in serialization


`null` is now consistently used for `nullptr` in all serialization contexts. See https://json-schema.org/understanding-json-schema/reference/null.

Previously, `nullptr` objects were not being correctly represented as JSON `null` in the states output by the VTK serialization API.
Before this fix, if you had a VTK data structure that contained `nullptr` references, those references would be omitted in the serialized JSON output. Likewise, when deserializing, `null` values in JSON were not being correctly interpreted as `nullptr` in VTK objects. This inconsistency could lead to data loss or misinterpretation when saving and loading VTK objects.

You can now expect that any `nullptr` object in VTK data structures will be serialized as `null` in JSON format. Method calls that accept or return `nullptr` were also updated to handle `null` values appropriately.
