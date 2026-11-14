# Runs im3e_shader_compiler on an invalid input, and fails unless the tool returns non-zero and prints the expected
# error. A script is needed as the PASS_REGULAR_EXPRESSION test property ignores the exit code.
#
# Usage: cmake -DTOOL=<tool> -DINPUT=<.slang> -DOUTPUT_DIR=<folder> -DFUNCTION=<name> -DEXPECTED_ERROR=<regex>
#              -P expect_compiler_error.cmake

foreach(VARIABLE TOOL INPUT OUTPUT_DIR FUNCTION EXPECTED_ERROR)
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "Missing -D${VARIABLE}")
    endif()
endforeach()

execute_process(
    COMMAND "${TOOL}" "${INPUT}"
        --header "${OUTPUT_DIR}/shader.h"
        --source "${OUTPUT_DIR}/shader.cpp"
        --depfile "${OUTPUT_DIR}/shader.d"
        --function "${FUNCTION}"
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE OUTPUT
)

if(RESULT EQUAL 0)
    message(FATAL_ERROR "Expected the tool to fail, but it succeeded. Output:\n${OUTPUT}")
endif()

if(NOT OUTPUT MATCHES "${EXPECTED_ERROR}")
    message(FATAL_ERROR "Expected an error matching \"${EXPECTED_ERROR}\" (exit code ${RESULT}). Output:\n${OUTPUT}")
endif()
