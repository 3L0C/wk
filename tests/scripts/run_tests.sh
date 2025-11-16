#!/usr/bin/env bash
# Test runner for wk

SUCCESS_COUNT=0
FAILURE_COUNT=0
TOTAL_TESTS=0

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Test log directory
LOG_DIR="tests/logs"
mkdir -p "$LOG_DIR"

# Log file for this test run
TEST_LOG="$LOG_DIR/test_run_$(date +%Y%m%d_%H%M%S).log"
echo "Starting tests at $(date)" > "$TEST_LOG"

# Create a function to run a test
run_test() {
    local test_file="$1"
    local key_sequence="$2"
    local expected_file="$3"
    local test_name
    test_name="$(basename "$test_file")"

    ((TOTAL_TESTS++))

    echo "==== Testing $test_name with key sequence '$key_sequence' ====" | tee -a "$TEST_LOG"

    # Sanitize key sequence for filename (replace spaces and special chars with underscores)
    local sanitized_keys
    sanitized_keys=$(echo "$key_sequence" | tr ' -' '__')
    local output_file="$LOG_DIR/${test_name}_${sanitized_keys}_output.txt"

    ./wk --key-chords "$test_file" --press "$key_sequence" > "$output_file" 2>&1
    local exit_code=$?

    echo "Command: ./wk --key-chords $test_file --press \"$key_sequence\"" >> "$TEST_LOG"
    echo "Exit code: $exit_code" >> "$TEST_LOG"

    if [[ -f "$expected_file" ]]; then
        # Compare output with expected output
        if diff -u "$expected_file" "$output_file" >> "$TEST_LOG"; then
            echo -e "${GREEN}PASS${NC}: $test_name ($key_sequence)" | tee -a "$TEST_LOG"
            ((SUCCESS_COUNT++))
            return 0
        else
            echo -e "${RED}FAIL${NC}: $test_name ($key_sequence) - Output doesn't match expected" | tee -a "$TEST_LOG"
            echo "Expected output (from $expected_file):" | tee -a "$TEST_LOG"
            tee -a "$TEST_LOG" < "$expected_file"
            echo "Actual output:" | tee -a "$TEST_LOG"
            tee -a "$TEST_LOG" < "$output_file"
            ((FAILURE_COUNT++))
            return 1
        fi
    else
        # If no expected file exists, check exit code
        if [[ $exit_code -eq 0 ]]; then
            echo -e "${GREEN}PASS${NC}: $test_name ($key_sequence) - Executed successfully" | tee -a "$TEST_LOG"
            # Save the output as expected for future runs
            cp "$output_file" "${expected_file}"
            echo "Created new expected output file: $expected_file" | tee -a "$TEST_LOG"
            ((SUCCESS_COUNT++))
            return 0
        else
            echo -e "${RED}FAIL${NC}: $test_name ($key_sequence) - Non-zero exit code: $exit_code" | tee -a "$TEST_LOG"
            echo "Output:" | tee -a "$TEST_LOG"
            tee -a "$TEST_LOG" < "$output_file"
            ((FAILURE_COUNT++))
            return 1
        fi
    fi
}

run_error_test() {
    local test_file="$1"
    local expected_error="$2"
    local test_name
    test_name="$(basename "$test_file")"

    ((TOTAL_TESTS++))

    echo "==== Testing error handling for $test_name ====" | tee -a "$TEST_LOG"

    # Run wk with the test file
    local output_file="$LOG_DIR/${test_name}_error_output.txt"

    # Run the command, expecting it to fail
    ./wk --key-chords "$test_file" > "$output_file" 2>&1
    local exit_code=$?

    # Log the command and exit code
    echo "Command: ./wk --key-chords $test_file" >> "$TEST_LOG"
    echo "Exit code: $exit_code" >> "$TEST_LOG"

    # Check for expected error
    if [[ $exit_code -ne 0 ]] && grep -q "$expected_error" "$output_file"; then
        echo -e "${GREEN}PASS${NC}: $test_name - Found expected error" | tee -a "$TEST_LOG"
        ((SUCCESS_COUNT++))
        return 0
    else
        echo -e "${RED}FAIL${NC}: $test_name - Did not find expected error: '$expected_error'" | tee -a "$TEST_LOG"
        echo "Output:" | tee -a "$TEST_LOG"
        tee -a "$TEST_LOG" < "$output_file"
        ((FAILURE_COUNT++))
        return 1
    fi
}

run_transpile_test() {
    local test_file="$1"
    local expected_file="$2"
    local test_name
    test_name="$(basename "$test_file" .wks)"

    ((TOTAL_TESTS++))

    echo "==== Testing transpile for $test_name ====" | tee -a "$TEST_LOG"

    local output_file="$LOG_DIR/${test_name}_transpiled.h"

    # Run transpile
    ./wk --transpile "$test_file" > "$output_file" 2>&1
    local exit_code=$?

    echo "Command: ./wk --transpile $test_file" >> "$TEST_LOG"
    echo "Exit code: $exit_code" >> "$TEST_LOG"

    # Check exit code first
    if [[ $exit_code -ne 0 ]]; then
        echo -e "${RED}FAIL${NC}: $test_name - Transpile failed with exit code $exit_code" | tee -a "$TEST_LOG"
        echo "Output:" | tee -a "$TEST_LOG"
        tee -a "$TEST_LOG" < "$output_file"
        ((FAILURE_COUNT++))
        return 1
    fi

    if [[ -f "$expected_file" ]]; then
        # Compare output with expected output
        if diff -u "$expected_file" "$output_file" >> "$TEST_LOG" 2>&1; then
            echo -e "${GREEN}PASS${NC}: $test_name - Transpile output matches expected" | tee -a "$TEST_LOG"
            ((SUCCESS_COUNT++))
            return 0
        else
            echo -e "${RED}FAIL${NC}: $test_name - Transpile output doesn't match expected" | tee -a "$TEST_LOG"
            echo "See $TEST_LOG for diff details" | tee -a "$TEST_LOG"
            ((FAILURE_COUNT++))
            return 1
        fi
    else
        # If no expected file exists, create it
        echo -e "${GREEN}PASS${NC}: $test_name - Transpile succeeded (created expected file)" | tee -a "$TEST_LOG"
        mkdir -p "$(dirname "$expected_file")"
        cp "$output_file" "$expected_file"
        echo "Created new expected output file: $expected_file" | tee -a "$TEST_LOG"
        ((SUCCESS_COUNT++))
        return 0
    fi
}

# Run all valid tests
echo "Running valid tests..." | tee -a "$TEST_LOG"
echo "======================" | tee -a "$TEST_LOG"

# Basic example tests
run_test "tests/fixtures/valid/basic_test.wks" "a" "tests/expected/basic_test_a.txt"
run_test "tests/fixtures/valid/basic_test.wks" "p b" "tests/expected/basic_test_p_b.txt"

# Chord array tests
run_test "tests/fixtures/valid/chord_array_test.wks" "x" "tests/expected/chord_array_test_x.txt"
run_test "tests/fixtures/valid/chord_array_test.wks" "y" "tests/expected/chord_array_test_y.txt"
run_test "tests/fixtures/valid/chord_array_test.wks" "z" "tests/expected/chord_array_test_z.txt"

# Chord expression tests
run_test "tests/fixtures/valid/chord_expression_test.wks" "a" "tests/expected/chord_expression_test_a.txt"
run_test "tests/fixtures/valid/chord_expression_test.wks" "g" "tests/expected/chord_expression_test_g.txt"

# Preprocessor tests
run_test "tests/fixtures/valid/preprocessor_test.wks" "a" "tests/expected/preprocessor_test_a.txt"
run_test "tests/fixtures/valid/preprocessor_test.wks" "p b" "tests/expected/preprocessor_test_p_b.txt"
run_test "tests/fixtures/valid/preprocessor_test.wks" "b" "tests/expected/preprocessor_test_b.txt"

# Hook tests (sync hooks only - async hooks have non-deterministic output)
run_test "tests/fixtures/valid/hook_test.wks" "a" "tests/expected/hook_test_a.txt"
run_test "tests/fixtures/valid/hook_test.wks" "b" "tests/expected/hook_test_b.txt"
run_test "tests/fixtures/valid/hook_test.wks" "c" "tests/expected/hook_test_c.txt"

# Sorting tests
run_test "tests/fixtures/valid/sorted_test.wks" "a" "tests/expected/sorted_test_a.txt"
run_test "tests/fixtures/valid/sorted_test.wks" "b" "tests/expected/sorted_test_b.txt"
run_test "tests/fixtures/valid/sorted_with_ignore_sort_test.wks" "a" "tests/expected/sorted_with_ignore_sort_test_a.txt"
run_test "tests/fixtures/valid/sorted_with_ignore_sort_test.wks" "b" "tests/expected/sorted_with_ignore_sort_test_b.txt"

# Special keys tests
run_test "tests/fixtures/valid/special_keys_test.wks" "TAB" "tests/expected/special_keys_test_tab.txt"
run_test "tests/fixtures/valid/special_keys_test.wks" "ESC" "tests/expected/special_keys_test_esc.txt"
run_test "tests/fixtures/valid/special_keys_test.wks" "VolDown" "tests/expected/special_keys_test_vol_down.txt"

# Unicode tests
run_test "tests/fixtures/valid/unicode_test.wks" "👍" "tests/expected/unicode_test_thumbs_up.txt"
run_test "tests/fixtures/valid/unicode_test.wks" "ä" "tests/expected/unicode_test_umlaut.txt"

# Variable (:var) macro tests
run_test "tests/fixtures/valid/var_test.wks" "a" "tests/expected/var_test_a.txt"
run_test "tests/fixtures/valid/var_test.wks" "b" "tests/expected/var_test_b.txt"
run_test "tests/fixtures/valid/var_test.wks" "c" "tests/expected/var_test_c.txt"
run_test "tests/fixtures/valid/var_test.wks" "d" "tests/expected/var_test_d.txt"
run_test "tests/fixtures/valid/var_test.wks" "e" "tests/expected/var_test_e.txt"
run_test "tests/fixtures/valid/var_test.wks" "f" "tests/expected/var_test_f.txt"

# Variable with include tests
run_test "tests/fixtures/valid/var_include_test/main.wks" "a" "tests/expected/var_include_main_a.txt"
run_test "tests/fixtures/valid/var_include_test/main.wks" "b" "tests/expected/var_include_main_b.txt"
run_test "tests/fixtures/valid/var_include_test/main.wks" "c" "tests/expected/var_include_main_c.txt"

# Meta-variable tests (variable names with interpolations)
run_test "tests/fixtures/valid/meta_var_test.wks" "a" "tests/expected/meta_var_test_a.txt"
run_test "tests/fixtures/valid/meta_var_test.wks" "b" "tests/expected/meta_var_test_b.txt"
run_test "tests/fixtures/valid/meta_var_test.wks" "c" "tests/expected/meta_var_test_c.txt"
run_test "tests/fixtures/valid/meta_var_test.wks" "d" "tests/expected/meta_var_test_d.txt"
run_test "tests/fixtures/valid/meta_var_test.wks" "e" "tests/expected/meta_var_test_e.txt"
run_test "tests/fixtures/valid/meta_var_test.wks" "f" "tests/expected/meta_var_test_f.txt"

# Preprocessor directive variable interpolation tests
run_test "tests/fixtures/valid/preprocessor_vars_test.wks" "a" "tests/expected/preprocessor_vars_test_a.txt"
run_test "tests/fixtures/valid/preprocessor_vars_test.wks" "b" "tests/expected/preprocessor_vars_test_b.txt"
run_test "tests/fixtures/valid/preprocessor_vars_test.wks" "c" "tests/expected/preprocessor_vars_test_c.txt"
run_test "tests/fixtures/valid/preprocessor_vars_test.wks" "d" "tests/expected/preprocessor_vars_test_d.txt"
run_test "tests/fixtures/valid/preprocessor_vars_test.wks" "e" "tests/expected/preprocessor_vars_test_e.txt"

# Complex variable interpolation tests
run_test "tests/fixtures/valid/var_complex_test.wks" "a" "tests/expected/var_complex_test_a.txt"
run_test "tests/fixtures/valid/var_complex_test.wks" "b" "tests/expected/var_complex_test_b.txt"
run_test "tests/fixtures/valid/var_complex_test.wks" "c" "tests/expected/var_complex_test_c.txt"
run_test "tests/fixtures/valid/var_complex_test.wks" "d" "tests/expected/var_complex_test_d.txt"
run_test "tests/fixtures/valid/var_complex_test.wks" "e" "tests/expected/var_complex_test_e.txt"
run_test "tests/fixtures/valid/var_complex_test.wks" "f" "tests/expected/var_complex_test_f.txt"

# Wrapper system tests
run_test "tests/fixtures/valid/wrapper_test.wks" "a" "tests/expected/wrapper_test_a.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "b" "tests/expected/wrapper_test_b.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "c" "tests/expected/wrapper_test_c.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "d" "tests/expected/wrapper_test_d.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "p e" "tests/expected/wrapper_test_p_e.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "f" "tests/expected/wrapper_test_f.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "g h" "tests/expected/wrapper_test_g_h.txt"
run_test "tests/fixtures/valid/wrapper_test.wks" "g i" "tests/expected/wrapper_test_g_i.txt"

# Comprehensive wrapper tests (%(wrap_cmd) interpolation + inheritance)
run_test "tests/fixtures/valid/wrapper_comprehensive.wks" "o g" "tests/expected/wrapper_comprehensive_o_g.txt"
run_test "tests/fixtures/valid/wrapper_comprehensive.wks" "o n" "tests/expected/wrapper_comprehensive_o_n.txt"
run_test "tests/fixtures/valid/wrapper_comprehensive.wks" "u c" "tests/expected/wrapper_comprehensive_u_c.txt"

# Duplicate key deduplication tests (last wins)
run_test "tests/fixtures/valid/duplicate_test.wks" "a" "tests/expected/duplicate_test_a.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "b" "tests/expected/duplicate_test_b.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "c" "tests/expected/duplicate_test_c.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "M-x" "tests/expected/duplicate_test_M_x.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "SPC" "tests/expected/duplicate_test_SPC.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "w a" "tests/expected/duplicate_test_w_a.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "w c" "tests/expected/duplicate_test_w_c.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "p x" "tests/expected/duplicate_test_p_x.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "p y" "tests/expected/duplicate_test_p_y.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "t" "tests/expected/duplicate_test_t.txt"
run_test "tests/fixtures/valid/duplicate_test.wks" "C-S-k" "tests/expected/duplicate_test_C_S_k.txt"

# Transpile tests
echo "Running transpile tests..." | tee -a "$TEST_LOG"
echo "======================" | tee -a "$TEST_LOG"

run_transpile_test "tests/fixtures/valid/basic_test.wks" "tests/expected/transpiled/basic_test_transpiled.h"
run_transpile_test "tests/fixtures/valid/chord_array_test.wks" "tests/expected/transpiled/chord_array_test_transpiled.h"
run_transpile_test "tests/fixtures/valid/preprocessor_test.wks" "tests/expected/transpiled/preprocessor_test_transpiled.h"
run_transpile_test "tests/fixtures/valid/wrapper_test.wks" "tests/expected/transpiled/wrapper_test_transpiled.h"

# Error handling tests
echo "Running error tests..." | tee -a "$TEST_LOG"
echo "======================" | tee -a "$TEST_LOG"

run_error_test "tests/fixtures/invalid/circular_include_test.wks" "tests/fixtures/invalid/circular_include_test.wks:2:35: wk does not support circular includes: ':include \"circular_include_test.wks\"'."
run_error_test "tests/fixtures/invalid/var_undefined.wks" "Undefined variable"
run_error_test "tests/fixtures/invalid/var_empty_key.wks" "Variable name resolves to empty string"
run_error_test "tests/fixtures/invalid/var_paren_in_key.wks" "Variable name contains ')'"

# Meta-variable error tests
run_error_test "tests/fixtures/invalid/var_circular.wks" "Undefined variable"
run_error_test "tests/fixtures/invalid/var_meta_invalid_char.wks" "Variable name contains ')'"
run_error_test "tests/fixtures/invalid/var_meta_empty.wks" "Variable name resolves to empty string"
run_error_test "tests/fixtures/invalid/var_meta_undefined.wks" "Undefined variable"

# Print summary
echo "" | tee -a "$TEST_LOG"
echo "======================" | tee -a "$TEST_LOG"
echo "Test Summary:" | tee -a "$TEST_LOG"
echo "Total tests: $TOTAL_TESTS" | tee -a "$TEST_LOG"
echo -e "${GREEN}Passed${NC}: $SUCCESS_COUNT" | tee -a "$TEST_LOG"
echo -e "${RED}Failed${NC}: $FAILURE_COUNT" | tee -a "$TEST_LOG"

# Return non-zero exit code if any tests failed
if [[ $FAILURE_COUNT -gt 0 ]]; then
    exit 1
fi

exit 0
