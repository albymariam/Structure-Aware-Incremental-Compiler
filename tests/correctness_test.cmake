set(work "${CMAKE_CURRENT_BINARY_DIR}/incppbuild-correctness")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}/include" "${work}/src")

file(GLOB headers "${PROJECT_SOURCE_DIR}/examples/sample_project/include/*")
file(GLOB sources "${PROJECT_SOURCE_DIR}/examples/sample_project/src/*")
file(COPY ${headers} DESTINATION "${work}/include")
file(COPY ${sources} DESTINATION "${work}/src")

set(source_files
    "${work}/src/main.cpp"
    "${work}/src/login.cpp"
    "${work}/src/payment.cpp"
    "${work}/src/database.cpp")
set(clean_executable "${work}/program_clean.exe")
set(incremental_executable "${work}/program_incremental.exe")

execute_process(
    COMMAND "${CXX_COMPILER}" -std=c++20 -static "-I${work}/include"
            ${source_files} -o "${clean_executable}"
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "Clean build failed: ${result}")
endif()

execute_process(
    COMMAND "${clean_executable}"
    OUTPUT_FILE "${work}/output_clean.txt"
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "Clean executable failed: ${result}")
endif()

file(READ "${work}/src/payment.cpp" payment)
string(REPLACE "subtotal" "intermediateTotal" payment "${payment}")
file(WRITE "${work}/src/payment.cpp" "${payment}")

execute_process(
    COMMAND "${CXX_COMPILER}" -std=c++20 -static "-I${work}/include"
            ${source_files} -o "${incremental_executable}"
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "Incremental build failed: ${result}")
endif()

execute_process(
    COMMAND "${incremental_executable}"
    OUTPUT_FILE "${work}/output_incremental.txt"
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "Incremental executable failed: ${result}")
endif()

file(READ "${work}/output_clean.txt" clean_output)
file(READ "${work}/output_incremental.txt" incremental_output)
if(NOT clean_output STREQUAL incremental_output)
    message(FATAL_ERROR "Clean and incremental executable output differs")
endif()
file(REMOVE_RECURSE "${work}")