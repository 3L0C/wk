#!/usr/bin/env bash
# Auto-discovery test runner for wk
# Parses @test/@expect directives from .wks files

set -uo pipefail

# Counters
PASS=0
FAIL=0
SKIP=0
TOTAL=0

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

# Options
FILTER=""
VERBOSE=false
UPDATE_SNAPSHOTS=false

# Log directory
LOG_DIR="tests/logs"
mkdir -p "$LOG_DIR"
TEST_LOG="$LOG_DIR/test_run_$(date +%Y%m%d_%H%M%S).log"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
    --filter)
        FILTER="$2"
        shift 2
        ;;
    -v | --verbose)
        VERBOSE=true
        shift
        ;;
    --update-snapshots | -u)
        UPDATE_SNAPSHOTS=true
        shift
        ;;
    -h | --help)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --filter PATTERN      Only run tests matching PATTERN"
        echo "  -v, --verbose         Show verbose output"
        echo "  -u, --update-snapshots  Update transpile snapshots"
        echo "  -h, --help            Show this help"
        exit 0
        ;;
    *)
        echo "Unknown option: $1"
        exit 1
        ;;
    esac
done

log() {
    echo "$1" >>"$TEST_LOG"
}

log_and_print() {
    echo -e "$1"
    echo -e "$1" | sed 's/\x1b\[[0-9;]*m//g' >>"$TEST_LOG"
}

# Discover all test files
discover_tests() {
    local dirs=("tests/runtime" "tests/transpile" "tests/error/compile" "tests/error/runtime")
    for dir in "${dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            find "$dir" -name "*.wks" -type f 2>/dev/null
        fi
    done | sort
}

# Determine test type from file path
get_test_type() {
    local file="$1"
    if [[ "$file" == tests/transpile/* ]]; then
        echo "transpile"
    elif [[ "$file" == tests/error/* ]]; then
        echo "error"
    else
        echo "runtime"
    fi
}

# Parse test definitions from a file using awk for robustness
# Output format: TYPE<TAB>KEYS<TAB>EXPECT<TAB>DESC
# TYPE is one of: test, error, skip, transpile
parse_tests() {
    local file="$1"
    local test_type="$2"

    awk -v test_type="$test_type" '
    BEGIN {
        in_header = 1
        in_multiline = 0
        current_keys = ""
        current_expect = ""
        current_desc = ""
        file_expect_error = ""
        skip_reason = ""
    }

    # Stop parsing header at first non-comment, non-empty line
    /^[^#]/ && !/^[[:space:]]*$/ {
        in_header = 0
        # Emit any pending test before we exit header
        if (current_keys != "" || current_expect != "") {
            if (skip_reason != "") {
                print "skip\t" current_keys "\t" skip_reason "\t" current_desc
            } else if (current_expect ~ /^ERROR:/) {
                print "error\t" current_keys "\t" current_expect "\t" current_desc
            } else {
                print "test\t" current_keys "\t" current_expect "\t" current_desc
            }
            current_keys = ""
            current_expect = ""
            current_desc = ""
            skip_reason = ""
        }
    }

    !in_header { next }

    # @desc: file description (ignored for now)
    /^# @desc:/ { next }

    # @skip: reason
    /^# @skip: / {
        skip_reason = substr($0, 10)
        next
    }

    # @expect-error: (file-level for compile errors)
    /^# @expect-error: / && current_keys == "" {
        file_expect_error = "ERROR:" substr($0, 18)
        next
    }

    # @test: "keys" -> description
    /^# @test: "/ {
        # Emit previous test
        if (current_keys != "" || current_expect != "") {
            if (skip_reason != "") {
                print "skip\t" current_keys "\t" skip_reason "\t" current_desc
            } else if (current_expect ~ /^ERROR:/) {
                print "error\t" current_keys "\t" current_expect "\t" current_desc
            } else {
                print "test\t" current_keys "\t" current_expect "\t" current_desc
            }
            skip_reason = ""
        }

        # Parse new test
        match($0, /"([^"]+)"/, arr)
        current_keys = arr[1]
        current_expect = ""
        current_desc = ""
        in_multiline = 0

        # Check for -> description
        if (match($0, /->[ ]*(.+)$/, desc_arr)) {
            current_desc = desc_arr[1]
        }
        next
    }

    # @expect: single line
    /^# @expect: / {
        current_expect = substr($0, 12)
        in_multiline = 0
        next
    }

    # @expect-multiline: start
    /^# @expect-multiline:/ {
        in_multiline = 1
        current_expect = ""
        next
    }

    # @expect-error: for specific test
    /^# @expect-error: / && current_keys != "" {
        current_expect = "ERROR:" substr($0, 18)
        in_multiline = 0
        next
    }

    # Multiline continuation: # |content
    # Use \\n as escape sequence for newlines (decoded by bash)
    /^# \|/ && in_multiline {
        line_content = substr($0, 4)
        if (current_expect == "") {
            current_expect = line_content
        } else {
            current_expect = current_expect "\\n" line_content
        }
        next
    }

    # Empty line ends multiline
    /^[[:space:]]*$/ {
        in_multiline = 0
        next
    }

    # Non-continuation comment ends multiline
    /^#/ && !/^# \|/ {
        in_multiline = 0
    }

    END {
        # Emit final test if exists
        if (current_keys != "" || current_expect != "") {
            if (skip_reason != "") {
                print "skip\t" current_keys "\t" skip_reason "\t" current_desc
            } else if (current_expect ~ /^ERROR:/) {
                print "error\t" current_keys "\t" current_expect "\t" current_desc
            } else {
                print "test\t" current_keys "\t" current_expect "\t" current_desc
            }
        }

        # Handle file-level error (compile errors without @test)
        if (test_type == "error" && file_expect_error != "" && current_keys == "") {
            print "error\t\t" file_expect_error "\t"
        }

        # Handle transpile (no keys)
        if (test_type == "transpile" && current_keys == "") {
            print "transpile\t\t\t"
        }
    }
    ' "$file"
}

# Run a single runtime test
run_runtime_test() {
    local file="$1"
    local keys="$2"
    local expect="$3"

    local output
    local exit_code=0
    output=$(./wk --key-chords "$file" --press "$keys" 2>&1) || exit_code=$?

    log "Command: ./wk --key-chords $file --press \"$keys\""
    log "Exit code: $exit_code"
    log "Output: $output"

    # Decode escaped newlines from parser
    local decoded_expect
    decoded_expect=$(printf '%b' "$expect")

    # Normalize: remove trailing newline for comparison
    local normalized_output="${output%$'\n'}"
    local normalized_expect="${decoded_expect%$'\n'}"

    if [[ "$normalized_output" == "$normalized_expect" ]]; then
        return 0
    else
        if $VERBOSE; then
            echo "    Expected: '$normalized_expect'"
            echo "    Got: '$normalized_output'"
        fi
        return 1
    fi
}

# Run an error test
run_error_test() {
    local file="$1"
    local keys="$2"
    local expect="$3"

    local output
    local exit_code=0

    if [[ -n "$keys" ]]; then
        output=$(./wk --key-chords "$file" --press "$keys" 2>&1) || exit_code=$?
        log "Command: ./wk --key-chords $file --press \"$keys\""
    else
        output=$(./wk --key-chords "$file" 2>&1) || exit_code=$?
        log "Command: ./wk --key-chords $file"
    fi

    log "Exit code: $exit_code"
    log "Output: $output"

    local expected_error="${expect#ERROR:}"
    if [[ $exit_code -ne 0 ]] && echo "$output" | grep -qF "$expected_error"; then
        return 0
    else
        if $VERBOSE; then
            echo "    Expected error: $expected_error"
            echo "    Got (exit=$exit_code): $output"
        fi
        return 1
    fi
}

# Run a transpile test with snapshot comparison
run_transpile_test() {
    local file="$1"
    local name
    name=$(basename "$file" .wks)
    local snapshot="tests/snapshots/transpile/${name}.h"

    local output
    local exit_code=0
    output=$(./wk --transpile "$file" 2>&1) || exit_code=$?

    log "Command: ./wk --transpile $file"
    log "Exit code: $exit_code"

    if [[ $exit_code -ne 0 ]]; then
        if $VERBOSE; then
            echo "    Transpile failed with exit code $exit_code"
            echo "    Output: $output"
        fi
        return 1
    fi

    # Update snapshot if requested
    if $UPDATE_SNAPSHOTS; then
        mkdir -p "$(dirname "$snapshot")"

        if [[ -f "$snapshot" ]]; then
            # Check if there's actually a difference
            if diff -q <(printf '%s\n' "$output") "$snapshot" >/dev/null 2>&1; then
                echo "    No changes: $snapshot"
                return 0
            fi

            # Show diff and ask for confirmation
            echo ""
            echo "=== Snapshot diff for $name ==="
            diff -u "$snapshot" <(printf '%s\n' "$output") | head -50
            echo ""

            # Check if we have a TTY for interactive input
            if [[ -t 0 ]] || [[ -e /dev/tty ]]; then
                echo -n "Update this snapshot? [y/N] "
                read -r response </dev/tty 2>/dev/null || response="n"
                if [[ "$response" =~ ^[Yy]$ ]]; then
                    printf '%s\n' "$output" >"$snapshot"
                    echo "    Updated snapshot: $snapshot"
                    log "Updated snapshot: $snapshot"
                else
                    echo "    Skipped: $snapshot"
                    log "Skipped snapshot update: $snapshot"
                fi
            else
                echo "    Non-interactive mode: skipping update (run interactively to update)"
                log "Skipped snapshot update (non-interactive): $snapshot"
            fi
        else
            # New snapshot - show preview and confirm
            echo ""
            echo "=== New snapshot: $name ==="
            printf '%s\n' "$output" | head -30
            echo "... (truncated)"
            echo ""

            # Check if we have a TTY for interactive input
            if [[ -t 0 ]] || [[ -e /dev/tty ]]; then
                echo -n "Create this snapshot? [y/N] "
                read -r response </dev/tty 2>/dev/null || response="n"
                if [[ "$response" =~ ^[Yy]$ ]]; then
                    printf '%s\n' "$output" >"$snapshot"
                    echo "    Created snapshot: $snapshot"
                    log "Created snapshot: $snapshot"
                else
                    echo "    Skipped: $snapshot"
                    log "Skipped snapshot creation: $snapshot"
                fi
            else
                echo "    Non-interactive mode: skipping creation (run interactively to create)"
                log "Skipped snapshot creation (non-interactive): $snapshot"
            fi
        fi
        return 0
    fi

    # Compare against snapshot
    if [[ -f "$snapshot" ]]; then
        if diff -q <(printf '%s\n' "$output") "$snapshot" >/dev/null 2>&1; then
            return 0
        else
            if $VERBOSE; then
                echo "    Snapshot mismatch. Diff:"
                diff -u "$snapshot" <(printf '%s\n' "$output") | head -20
            fi
            log "Snapshot mismatch for $file"
            return 1
        fi
    else
        # No snapshot exists - create it
        mkdir -p "$(dirname "$snapshot")"
        printf '%s\n' "$output" >"$snapshot"
        echo "    Created new snapshot: $snapshot"
        log "Created snapshot: $snapshot"
        return 0
    fi
}

# Main test loop
main() {
    echo "Running wk tests..."
    echo "==================="
    log "Starting tests at $(date)"

    local test_files
    test_files=$(discover_tests)

    if [[ -z "$test_files" ]]; then
        echo "No test files found in tests/{runtime,transpile,error}/"
        echo "Run the migration script first or add test files."
        exit 0
    fi

    for file in $test_files; do
        # Apply filter if specified
        if [[ -n "$FILTER" ]] && [[ "$file" != *"$FILTER"* ]]; then
            continue
        fi

        local test_type
        test_type=$(get_test_type "$file")
        local file_basename
        file_basename=$(basename "$file" .wks)

        log "=== Testing $file ==="

        # Parse and run tests from this file
        while IFS=$'\t' read -r type keys expect desc; do
            [[ -z "$type" ]] && continue

            ((TOTAL++))

            # Build test name
            local test_name="$file_basename"
            [[ -n "$keys" ]] && test_name+=" [$keys]"
            [[ -n "$desc" ]] && test_name+=" - $desc"

            # Handle skip
            if [[ "$type" == "skip" ]]; then
                log_and_print "${YELLOW}SKIP${NC}: $test_name ($expect)"
                ((SKIP++))
                continue
            fi

            # Run appropriate test type
            local result=0
            case "$type" in
            test)
                run_runtime_test "$file" "$keys" "$expect" || result=1
                ;;
            error)
                run_error_test "$file" "$keys" "$expect" || result=1
                ;;
            transpile)
                run_transpile_test "$file" || result=1
                ;;
            esac

            if [[ $result -eq 0 ]]; then
                log_and_print "${GREEN}PASS${NC}: $test_name"
                ((PASS++))
            else
                log_and_print "${RED}FAIL${NC}: $test_name"
                ((FAIL++))
            fi

        done < <(parse_tests "$file" "$test_type")
    done

    # Print summary
    echo ""
    echo "==================="
    echo "Test Summary:"
    echo "Total tests: $TOTAL"
    echo -e "${GREEN}Passed${NC}: $PASS"
    if [[ $FAIL -gt 0 ]]; then
        echo -e "${RED}Failed${NC}: $FAIL"
    fi
    if [[ $SKIP -gt 0 ]]; then
        echo -e "${YELLOW}Skipped${NC}: $SKIP"
    fi
    echo ""
    echo "Log file: $TEST_LOG"

    log "Test Summary: $PASS passed, $FAIL failed, $SKIP skipped"

    [[ $FAIL -gt 0 ]] && exit 1
    exit 0
}

main
