#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 100 "100;"
assert 22 "5 * 6 - 8;"
assert 3 "a = 3;a;"
assert 5 "a = 4;b = 5;b;"
assert 14 "a = 3;b = 5 * 6 - 8;a + b / 2;"

echo OK