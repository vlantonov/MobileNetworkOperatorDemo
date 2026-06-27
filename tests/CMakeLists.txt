add_executable(test_billing test_billing.cpp)
target_link_libraries(test_billing PRIVATE billing doctest::doctest)
add_test(NAME billing_tests COMMAND test_billing)
