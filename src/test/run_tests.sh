#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Compiler flags matching CMakeLists.txt
CXX_FLAGS="-mcpu=cortex-m0plus -mthumb -std=c++17 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
    -D__SAMD21E18A__ -I${PROJECT_ROOT} -I${PROJECT_ROOT}/src \
    -I${PROJECT_ROOT}/src/libs/cmsis/Core/Include \
    -I${PROJECT_ROOT}/src/libs/cmsis/samd21a/include"

PASS=0
FAIL=0
TESTS=()

run_test() {
    local name="$1"
    local file="$2"
    local expect_success="$3"  # "yes" or "no"
    
    TESTS+=("$name")
    
    echo -n "  $name ... "
    
    if output=$(arm-none-eabi-g++ $CXX_FLAGS "$SCRIPT_DIR/$file" -c -o /dev/null 2>&1); then
        compiled=true
    else
        compiled=false
    fi
    
    if [ "$expect_success" = "yes" ]; then
        if $compiled; then
            echo "PASS"
            PASS=$((PASS + 1))
        else
            echo "FAIL (expected success but compilation failed)"
            echo "$output" | sed 's/^/    /'
            FAIL=$((FAIL + 1))
        fi
    else
        if ! $compiled; then
            # Check that the error message is meaningful (not just a missing header)
            if echo "$output" | grep -q "static assertion failed"; then
                echo "PASS"
                PASS=$((PASS + 1))
            else
                echo "FAIL (compiled failed but no static_assert message)"
                echo "$output" | sed 's/^/    /'
                FAIL=$((FAIL + 1))
            fi
        else
            echo "FAIL (expected failure but compilation succeeded)"
            FAIL=$((FAIL + 1))
        fi
    fi
}

echo ""
echo "=== Compile-time validation tests ==="
echo ""

# Good configs - should compile successfully
run_test "good_i2c"         "good_i2c.cpp"       "yes"
run_test "good_adc"         "good_adc.cpp"       "yes"
run_test "good_pwm"         "good_pwm.cpp"       "yes"
run_test "good_chain"       "good_chain.cpp"     "yes"
run_test "adjacent_pins"    "adjacent_pins.cpp"  "yes"

# Bad configs - should fail with static_assert messages
run_test "bad_i2c"          "bad_i2c.cpp"         "no"
run_test "bad_adc"          "bad_adc.cpp"         "no"
run_test "bad_adc_read"     "bad_adc_read.cpp"    "no"
run_test "bad_pwm"          "bad_pwm.cpp"         "no"
run_test "bad_pwm_duty"     "bad_pwm_duty.cpp"    "no"

# Mixed valid/invalid pins — SCL good, SDA bad (should catch SDA error)
run_test "mixed_sda_bad"    "mixed_pins.cpp"      "no"

# Mixed valid/invalid pins — SCL bad, SDA good (should catch SCL error)
run_test "mixed_scl_bad"    "mixed_sda_good.cpp"  "no"

# Swapped/different SERCOM - PA17 is SERCOM1 but PA04 is SERCOM0, should fail with same-SERCOM error
run_test "swapped_sercom"   "swapped_sercom.cpp"  "no"

echo ""
echo "Results: $PASS passed, $FAIL failed out of ${#TESTS[@]} tests"
echo ""

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
