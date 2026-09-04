#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only

set -e

echo "=== BSTE Runtime Resilience Host Tests ==="

# Locations of harness scripts relative to execroot
HARNESS_BIN="private/devices/google/common/kleaf/impl/bste/harness/bste_test.sh"
RUNNER_BIN="private/devices/google/common/kleaf/impl/bste/harness/bste_test_runner.sh"
COMMON_BIN="private/devices/google/common/kleaf/impl/bste/harness/common.sh"
HELPERS_BIN="private/devices/google/common/kleaf/impl/bste/harness/test_helpers.sh"

# Create a local test sandbox workspace
SANDBOX_DIR="$(mktemp -d)"
echo "Created sandbox: ${SANDBOX_DIR}"

PKG_DIR="${SANDBOX_DIR}/pkg"
mkdir -p "${PKG_DIR}/harness"
mkdir -p "${PKG_DIR}/test_suites"

# Set custom TMPDIR for state isolation
export TMPDIR="${SANDBOX_DIR}/tmp"
mkdir -p "${TMPDIR}"

cp "${HARNESS_BIN}" "${PKG_DIR}/harness/bste_test.sh"
cp "${RUNNER_BIN}" "${PKG_DIR}/harness/bste_test_runner.sh"
cp "${COMMON_BIN}" "${PKG_DIR}/harness/common.sh"
cp "${HELPERS_BIN}" "${PKG_DIR}/harness/test_helpers.sh"
chmod +x "${PKG_DIR}/harness/"*.sh

cd "${PKG_DIR}"
BSTE="./harness/bste_test.sh"

function cleanup {
  echo "Cleaning up sandbox..."
  rm -rf "${SANDBOX_DIR}"
}
trap cleanup EXIT

# ------------------------------------------------------------------------------
# Test: Execution Isolation (Subshell Scoping)
# ------------------------------------------------------------------------------
echo ">>> Testing Execution Isolation..."
mkdir -p "${PKG_DIR}/test_suites/iso_suite"
cat << 'EOF' > "${PKG_DIR}/test_suites/iso_suite/test.sh"
GLOBAL_VAR="ORIGINAL"
suite_init() {
  echo "Iso suite initialized"
}

test_case_1() {
  # Modify it in child subshell
  GLOBAL_VAR="MODIFIED"
  LEAK_VAR="LEAKED"
}

test_case_2() {
  # Verify original value persists and no leak from test_case_1
  if [ "${GLOBAL_VAR}" != "ORIGINAL" ]; then
    echo "FAIL: GLOBAL_VAR changed!"
    return 1
  fi
  if [ -n "${LEAK_VAR}" ]; then
    echo "FAIL: LEAK_VAR was seen!"
    return 1
  fi
  return 0
}
EOF

cat << 'EOF' > "${PKG_DIR}/test_suites/iso_suite/bste_test_sources.sh"
. ./test.sh
EOF

cat << 'EOF' > "${PKG_DIR}/test_suites/iso_suite/bste_test_metadata.sh"
BSTE_TEST_CASES=("test_case_1" "test_case_2")
BSTE_SUITE_INIT="suite_init"
EOF

${BSTE} run --detach iso_suite

# Wait for completion
MAX_WAIT=15
while [ ${MAX_WAIT} -gt 0 ]; do
  if grep -q "result=PASSED" "${TMPDIR}/bste_run/current/summary.txt" 2>/dev/null; then
    break
  fi
  sleep 1
  MAX_WAIT=$((MAX_WAIT - 1))
done

# Verify success
if grep -q "phase:iso_suite.test_case_2:TEST_CASE=OK" "${TMPDIR}/bste_run/current/summary.txt" && \
   grep -q "result:iso_suite.test_case_2=PASSED" "${TMPDIR}/bste_run/current/summary.txt"; then
  echo ">>> Execution Isolation SUCCESS"
else
  echo ">>> Execution Isolation FAILURE"
  cat "${TMPDIR}/bste_run/current/summary.txt"
  exit 1
fi

# ------------------------------------------------------------------------------
# Test: Session Locking & Assertive Recovery
# ------------------------------------------------------------------------------
echo ">>> Testing Session Locking & Assertive Recovery..."

# Mock a dead session
DEAD_RUN="${TMPDIR}/bste_run/runs/dead_run"
mkdir -p "${DEAD_RUN}"
rm -f "${TMPDIR}/bste_run/current"
ln -s "runs/dead_run" "${TMPDIR}/bste_run/current"

cat << 'EOF' > "${DEAD_RUN}/state.txt"
RUN_ID=dead_run
PID=9999999
STATUS=RUNNING
PHASE=TEST_CASE
SUITE=mock_suite
CASE=mock_case
EOF
touch "${DEAD_RUN}/summary.txt"

# Running `status` should recover it
echo "Triggering recovery..."
${BSTE} status >/dev/null || true

# Check recovery status
RECOVERED_STATUS=$(grep "^STATUS=" "${DEAD_RUN}/state.txt" | cut -d'=' -f2)
if [ "${RECOVERED_STATUS}" != "CRASHED" ]; then
  echo ">>> Session Locking FAILURE: Session was not marked CRASHED"
  exit 1
fi
if ! grep -q "result=CRASHED" "${DEAD_RUN}/summary.txt"; then
  echo ">>> Session Locking FAILURE: summary.txt missing crash status"
  exit 1
fi

# Now ensure new run succeeds
${BSTE} run --detach iso_suite >/dev/null
NEW_RID=$(grep "^RUN_ID=" "${TMPDIR}/bste_run/current/state.txt" | cut -d'=' -f2)
if [ "${NEW_RID}" = "dead_run" ]; then
  echo ">>> Session Locking FAILURE: Failed to start new run."
  exit 1
fi
echo ">>> Session Locking SUCCESS"

# Wait for current run to complete so it doesn't block next
MAX_WAIT=15
while [ ${MAX_WAIT} -gt 0 ]; do
  if grep -q "result=PASSED" "${TMPDIR}/bste_run/current/summary.txt" 2>/dev/null; then
    break
  fi
  sleep 1
  MAX_WAIT=$((MAX_WAIT - 1))
done

# ------------------------------------------------------------------------------
# Test: Atomic Scoreboard & Persistence (Hard Crash Attribution)
# ------------------------------------------------------------------------------
echo ">>> Testing Atomic Scoreboard & Persistence..."
mkdir -p "${PKG_DIR}/test_suites/crash_suite"
cat << 'EOF' > "${PKG_DIR}/test_suites/crash_suite/test.sh"
sleepy_case() {
  echo "Sleeping..."
  sleep 60
}
EOF
cat << 'EOF' > "${PKG_DIR}/test_suites/crash_suite/bste_test_sources.sh"
. ./test.sh
EOF
cat << 'EOF' > "${PKG_DIR}/test_suites/crash_suite/bste_test_metadata.sh"
BSTE_TEST_CASES=("sleepy_case")
EOF

# Start it
${BSTE} run --detach crash_suite >/dev/null

# Wait until phase is TEST_CASE
MAX_WAIT=10
CRASH_PID=""
while [ ${MAX_WAIT} -gt 0 ]; do
  CUR_PHASE=$(grep "^PHASE=" "${TMPDIR}/bste_run/current/state.txt" | cut -d'=' -f2)
  if [ "${CUR_PHASE}" = "TEST_CASE" ]; then
    CRASH_PID=$(grep "^PID=" "${TMPDIR}/bste_run/current/state.txt" | cut -d'=' -f2)
    if [ -n "${CRASH_PID}" ]; then break; fi
  fi
  sleep 1
  MAX_WAIT=$((MAX_WAIT - 1))
done

if [ -z "${CRASH_PID}" ]; then
  echo ">>> Atomic Scoreboard FAILURE: Timeout waiting for TEST_CASE"
  exit 1
fi

echo "Hard killing runner (PID ${CRASH_PID})..."
pkill -9 -s "${CRASH_PID}"
sleep 1

# Invoke status to trigger recovery
${BSTE} status --raw > /dev/null || true

SUMM_FILE="${TMPDIR}/bste_run/current/summary.txt"
if grep -q "phase:crash_suite.sleepy_case:TEST_CASE=CRASHED" "${SUMM_FILE}" && \
   grep -q "result:crash_suite.sleepy_case=CRASHED" "${SUMM_FILE}"; then
  echo ">>> Atomic Scoreboard SUCCESS"
else
  echo ">>> Atomic Scoreboard FAILURE: Crash not attributed to phase correctly."
  cat "${SUMM_FILE}"
  exit 1
fi

# ------------------------------------------------------------------------------
# Test: Process Group Management
# ------------------------------------------------------------------------------
echo ">>> Testing Process Group Management..."
mkdir -p "${PKG_DIR}/test_suites/kill_suite"
cat << 'EOF' > "${PKG_DIR}/test_suites/kill_suite/test.sh"
orphan_test() {
  # Launch an orphan grandchild process
  sleep 100 &
  CHILD_PID=$!
  echo "${CHILD_PID}" > "${OUT}/orphan.pid"
  echo "Started orphan ${CHILD_PID}"
  sleep 100
}
EOF
cat << 'EOF' > "${PKG_DIR}/test_suites/kill_suite/bste_test_sources.sh"
. ./test.sh
EOF
cat << 'EOF' > "${PKG_DIR}/test_suites/kill_suite/bste_test_metadata.sh"
BSTE_TEST_CASES=("orphan_test")
EOF

# We must ensure there is no ACTIVE session first (recover current if needed)
${BSTE} status >/dev/null || true

${BSTE} run --detach kill_suite >/dev/null

# Wait for pid record
ORPHAN_PID=""
MAX_WAIT=10
while [ ${MAX_WAIT} -gt 0 ]; do
  CUR_RUN=$(readlink -f "${TMPDIR}/bste_run/current" 2>/dev/null)
  PID_F="${CUR_RUN}/test_suites/kill_suite/orphan_test/out/orphan.pid"
  if [ -f "${PID_F}" ]; then
    ORPHAN_PID=$(cat "${PID_F}")
    break
  fi
  sleep 1
  MAX_WAIT=$((MAX_WAIT - 1))
done

if [ -z "${ORPHAN_PID}" ]; then
  echo ">>> Process Group Management FAILURE: Orphan PID not created."
  exit 1
fi

echo "Orphan process detected with PID ${ORPHAN_PID}."
if ! kill -0 "${ORPHAN_PID}" 2>/dev/null; then
  echo ">>> Process Group Management FAILURE: Orphan already dead prematurely."
  exit 1
fi

echo "Invoking bste kill..."
${BSTE} kill >/dev/null
sleep 2

if kill -0 "${ORPHAN_PID}" 2>/dev/null; then
  echo ">>> Process Group Management FAILURE: Orphan PID ${ORPHAN_PID} survived kill!"
  kill -9 "${ORPHAN_PID}" 2>/dev/null
  exit 1
else
  # Verify kill status in summary.txt
  SUMM_FILE="${TMPDIR}/bste_run/current/summary.txt"
  if grep -q "result:kill_suite.orphan_test=KILLED" "${SUMM_FILE}" && \
       grep -q "phase:kill_suite.orphan_test:TEST_CASE=KILLED" "${SUMM_FILE}" && \
       grep -q "result=KILLED" "${SUMM_FILE}"; then
    echo ">>> Process Group Management SUCCESS: Orphan cleaned up and status logged."
  else
    echo ">>> Process Group Management FAILURE: Kill status not logged correctly."
    cat "${SUMM_FILE}"
    exit 1
  fi
fi

# ------------------------------------------------------------------------------
# Test: Rich Test API & Structured Results (GoogleTest style)
# ------------------------------------------------------------------------------
echo ">>> Testing Rich Test API & Structured Results..."
mkdir -p "${PKG_DIR}/test_suites/api_suite"
cat << 'EOF' > "${PKG_DIR}/test_suites/api_suite/test.sh"
# Use ksh function syntax to test function name resolution
function test_success {
  # Test state sharing via TEST_OUT
  echo "state_data" > "${TEST_OUT}/shared_state.txt"
  pass "custom pass message"
}

function test_failure {
  assert_eq "expected_val" "actual_val" "eq message"
}

function test_skip {
  skip "skip message"
}

function test_error {
  # Test state sharing via SUITE_OUT
  echo "suite_state" > "${SUITE_OUT}/suite_shared.txt"
  error "explicit error message"
}

function test_asserts {
  assert_true "test 1 -eq 1" "should be true"
  assert_false "test 1 -eq 2" "should be false"
  assert_ne "val1" "val2" "should be different"
  pass "asserts ok"
}
EOF

cat << 'EOF' > "${PKG_DIR}/test_suites/api_suite/bste_test_sources.sh"
alias pass='bste_test_pass "test.sh" "${LINENO}"'
alias fail='bste_test_fail "test.sh" "${LINENO}"'
alias skip='bste_test_skip "test.sh" "${LINENO}"'
alias error='bste_test_error "test.sh" "${LINENO}"'
alias assert_true='bste_test_assert_true "test.sh" "${LINENO}"'
alias assert_false='bste_test_assert_false "test.sh" "${LINENO}"'
alias assert_eq='bste_test_assert_eq "test.sh" "${LINENO}"'
alias assert_ne='bste_test_assert_ne "test.sh" "${LINENO}"'
source test.sh
EOF

cat << 'EOF' > "${PKG_DIR}/test_suites/api_suite/bste_test_metadata.sh"
BSTE_TEST_CASES=("test_success" "test_failure" "test_skip" "test_error" "test_asserts")
EOF

# We must recover previous session first
${BSTE} status >/dev/null || true

# Run it
${BSTE} run --detach api_suite >/dev/null

# Wait for completion
MAX_WAIT=15
while [ ${MAX_WAIT} -gt 0 ]; do
  if grep -q "result=" "${TMPDIR}/bste_run/current/summary.txt" 2>/dev/null; then
    break
  fi
  sleep 1
  MAX_WAIT=$((MAX_WAIT - 1))
done

# Verify summary.txt contents
SUMM_FILE="${TMPDIR}/bste_run/current/summary.txt"
echo "=== summary.txt ==="
cat "${SUMM_FILE}"

# Assertions for summary.txt
# Test case results
grep -q "result:api_suite.test_success=PASSED" "${SUMM_FILE}"
grep -q "result:api_suite.test_failure=FAILED" "${SUMM_FILE}"
grep -q "result:api_suite.test_skip=SKIPPED" "${SUMM_FILE}"
grep -q "result:api_suite.test_error=ERROR" "${SUMM_FILE}"
grep -q "result:api_suite.test_asserts=PASSED" "${SUMM_FILE}"

# Suite result
grep -q "result:api_suite=ERROR" "${SUMM_FILE}"

# Phase status (all OK except test_error which is ERROR)
grep -q "phase:api_suite.test_success:TEST_CASE=OK" "${SUMM_FILE}"
grep -q "phase:api_suite.test_failure:TEST_CASE=OK" "${SUMM_FILE}"
grep -q "phase:api_suite.test_skip:TEST_CASE=OK" "${SUMM_FILE}"
grep -q "phase:api_suite.test_error:TEST_CASE=ERROR" "${SUMM_FILE}"

# Verify state sharing via SUITE_OUT and TEST_OUT
CUR_RUN="${TMPDIR}/bste_run/current"
if [ ! -f "${CUR_RUN}/test_suites/api_suite/test_success/out/shared_state.txt" ] || \
   [ "$(cat "${CUR_RUN}/test_suites/api_suite/test_success/out/shared_state.txt")" != \
     "state_data" ]; then
  echo ">>> Rich Test API FAILURE: TEST_OUT state sharing failed"
  exit 1
fi
if [ ! -f "${CUR_RUN}/test_suites/api_suite/out/suite_shared.txt" ] || \
   [ "$(cat "${CUR_RUN}/test_suites/api_suite/out/suite_shared.txt")" != "suite_state" ]; then
  echo ">>> Rich Test API FAILURE: SUITE_OUT state sharing failed"
  exit 1
fi

# Verify session.log contents (GoogleTest format and messages)
SESSION_LOG="${CUR_RUN}/session.log"
echo "=== session.log ==="
cat "${SESSION_LOG}"

# Verify RUN and completion lines
grep -q "\[ RUN      \] api_suite.test_success (TEST_CASE)" "${SESSION_LOG}"
grep -q "\[       OK \] api_suite.test_success (TEST_CASE)" "${SESSION_LOG}"

grep -q "\[ RUN      \] api_suite.test_failure (TEST_CASE)" "${SESSION_LOG}"
grep -q "\[  FAILED  \] api_suite.test_failure (TEST_CASE)" "${SESSION_LOG}"

grep -q "\[ RUN      \] api_suite.test_skip (TEST_CASE)" "${SESSION_LOG}"
grep -q "\[  SKIPPED \] api_suite.test_skip (TEST_CASE)" "${SESSION_LOG}"

grep -q "\[ RUN      \] api_suite.test_error (TEST_CASE)" "${SESSION_LOG}"
grep -q "\[  ERROR   \] api_suite.test_error (TEST_CASE)" "${SESSION_LOG}"

# Verify helper messages and line numbers are printed BEFORE completion brackets
# test_success calls pass on line 5 - should be quiet (no prints)
! grep -q "test.sh:5:" "${SESSION_LOG}"
! grep -q "custom pass message" "${SESSION_LOG}"

# test_failure calls assert_eq on line 9
grep -q "test.sh:9: fail" "${SESSION_LOG}"
grep -q "Expected equality of these values:" "${SESSION_LOG}"
grep -q "  Expected: expected_val" "${SESSION_LOG}"
grep -q "  Actual:   actual_val" "${SESSION_LOG}"

# test_skip calls skip on line 13
grep -q "test.sh:13: skip" "${SESSION_LOG}"
grep -q "skip message" "${SESSION_LOG}"

# test_error calls error on line 19
grep -q "test.sh:19: error" "${SESSION_LOG}"
grep -q "explicit error message" "${SESSION_LOG}"

# Verify bste_test summary output
echo "=== bste_test summary ==="
${BSTE} summary

# Verify summary output format
RESULT_OUT=$(${BSTE} summary)
echo "${RESULT_OUT}" | grep -q "\[ PASSED  \] api_suite.test_success"
echo "${RESULT_OUT}" | grep -q "\[ FAILED  \] api_suite.test_failure"
echo "${RESULT_OUT}" | grep -q "\[ SKIPPED \] api_suite.test_skip"
echo "${RESULT_OUT}" | grep -q "\[ ERROR   \] api_suite.test_error"
echo "${RESULT_OUT}" | grep -q "\[ ERROR   \] api_suite$"
echo "${RESULT_OUT}" | grep -q "Summary: 2 passed, 1 failed, 1 skipped, 1 errors"

echo ">>> Rich Test API SUCCESS"

# ------------------------------------------------------------------------------
# Final Conclusion
# ------------------------------------------------------------------------------
echo ">>> ALL RUNTIME TESTS PASSED SUCCESSFULLY <<<"
exit 0
