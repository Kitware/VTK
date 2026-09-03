## vtkDataAssembly node paths are unique selectors

`vtkDataAssembly` no longer requires a node name to be unique among its siblings. Names are often derived from a file
and passed through `vtkDataAssembly::MakeValidNodeName`, which can turn two distinct names into the same valid one, and
rejecting the second of them made readers drop nodes. `AddNode` and `AddNodes` now accept such a name.

Since a name-based path can then match several nodes, `vtkDataAssembly::GetNodePath` qualifies its result with the node
id when the path is ambiguous, for example `/Root/Blocks/element[@id='7']`. The returned path always selects exactly the
node you asked for, so you can pass it to `SelectNodes`. For a node whose path is already unambiguous, which is every
node in an assembly with unique sibling names, you get the plain path as before.

Two things to keep in mind when using the result:

* It is a selector, not a name. It may contain characters that `IsNodeNameValid` rejects, so do not use it where a node
  name is expected, such as deriving a group name in a file format.

* Node ids are assigned in `AddNode` order, so an id-qualified path is only meaningful for that assembly and for
  assemblies built the same way. If you need a path that survives structural changes, build a selector from a `label`
  attribute instead.
