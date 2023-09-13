set(unit_tests
  testManager.cxx
  testToken.cxx
  testTypeContainer.cxx
)

token_unit_tests(
  LABEL "Basic"
  SOURCES ${unit_tests}
  LIBRARIES token
)
