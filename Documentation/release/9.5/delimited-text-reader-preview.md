# Delimited Text Reader Preview

vtkDelimitedTextReader offers a `GetPreview` method to inspect the file
content.  During the `RequestInformation` pass, it reads the
`PreviewNumberOfLines` first lines of the input file.  By default, nothing is
done and Preview is empty.

This is useful to configure the reader options (like `XXXDelimiters` or
`HaveHeaders`) before the `RequestData` pass.
