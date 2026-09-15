#!/usr/bin/env bash
# Test suite for resumec: golden-file codegen test, one negative test per
# semantic rule, an AST/parser isolation check, and a PDF smoke test.
# A C unit-test framework would be overkill at this scope, so this is a
# plain shell script built around `diff` and exit-code/substring checks,
# as called for in the project spec.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/bin/resumec"
TESTS_DIR="$ROOT/tests"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

PASS=0
FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found -- run 'make' first" >&2
    exit 1
fi

echo "== golden-file test =="
"$BIN" "$TESTS_DIR/golden/sample1.resume" --tex-only -o "$TMP_DIR/sample1.pdf" \
    >"$TMP_DIR/stdout.txt" 2>"$TMP_DIR/stderr.txt"
if [ ! -f "$TMP_DIR/sample1.tex" ]; then
    fail "codegen did not produce sample1.tex (stderr: $(cat "$TMP_DIR/stderr.txt"))"
elif diff -u "$TESTS_DIR/golden/sample1.tex" "$TMP_DIR/sample1.tex" >"$TMP_DIR/diff.txt"; then
    pass "generated .tex byte-for-byte matches tests/golden/sample1.tex"
else
    fail "generated .tex differs from tests/golden/sample1.tex"
    cat "$TMP_DIR/diff.txt"
fi

echo
echo "== lexer/parser isolation (--dump-ast) =="
AST_OUT="$("$BIN" "$TESTS_DIR/golden/sample1.resume" --dump-ast)"
if echo "$AST_OUT" | grep -q 'name=Ananya Mehta' \
    && echo "$AST_OUT" | grep -q 'skills (3): \[C++, Python, Java\]' \
    && echo "$AST_OUT" | grep -q 'college=VIT cgpa=9.30'; then
    pass "--dump-ast shows the expected AST shape, independent of semantic/IR/codegen"
else
    fail "--dump-ast output did not match expectations:"
    echo "$AST_OUT"
fi

echo
echo "== negative tests (one per semantic rule) =="

check_negative() {
    local name="$1" file="$2" expect_substr="$3"
    local out status
    out="$("$BIN" "$file" --tex-only -o "$TMP_DIR/neg.pdf" 2>&1)"
    status=$?
    if [ "$status" -eq 0 ]; then
        fail "$name: expected a non-zero exit code, got 0"
        return
    fi
    if echo "$out" | grep -qF "$expect_substr"; then
        pass "$name: exits non-zero and reports \"$expect_substr\""
    else
        fail "$name: diagnostic did not contain \"$expect_substr\" (got: $out)"
    fi
}

check_negative "bad CGPA"        "$TESTS_DIR/invalid_cgpa.resume"    "out of valid range"
check_negative "malformed email" "$TESTS_DIR/invalid_email.resume"   "invalid email format"
check_negative "duplicate skill" "$TESTS_DIR/duplicate_skill.resume" "duplicate skill"
check_negative "missing name"    "$TESTS_DIR/missing_name.resume"    "must have a 'name'"

echo
echo "== PDF smoke test =="
PDFLATEX=""
if command -v pdflatex >/dev/null 2>&1; then
    PDFLATEX="pdflatex"
elif [ -x /Library/TeX/texbin/pdflatex ]; then
    PDFLATEX="/Library/TeX/texbin/pdflatex"
fi

if [ -n "$PDFLATEX" ]; then
    "$BIN" "$TESTS_DIR/golden/sample1.resume" -o "$TMP_DIR/smoke.pdf" \
        >"$TMP_DIR/smoke.out" 2>&1
    status=$?
    if [ "$status" -eq 0 ] && [ -s "$TMP_DIR/smoke.pdf" ]; then
        pass "full pipeline (lex..codegen..pdflatex) produced a non-empty PDF"
    else
        fail "full pipeline did not produce a non-empty PDF"
        cat "$TMP_DIR/smoke.out"
    fi
else
    echo "  SKIP: no pdflatex found on this machine -- install a LaTeX distribution"
    echo "        (e.g. 'brew install --cask basictex') to exercise this test."
fi

echo
echo "-------------------------------------------"
echo "passed: $PASS   failed: $FAIL"
[ "$FAIL" -eq 0 ]
