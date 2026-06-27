add_library(billing STATIC src/BillingEngine.cpp)
target_include_directories(billing PUBLIC include)
target_compile_features(billing PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(billing PRIVATE /W4 /WX)
else()
    target_compile_options(billing PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
