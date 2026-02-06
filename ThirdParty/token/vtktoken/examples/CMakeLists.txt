set(examples
  basic
  containers
  switch-case
)

foreach (example ${examples})
  add_executable(${example} ${example}.cxx)
  target_link_libraries(${example}
    PRIVATE
      token
  )
  add_test(NAME "example-${example}" COMMAND ${example})
endforeach()
