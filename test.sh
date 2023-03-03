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
assert 4 "return 4;"
assert 20 "foo = 4;bar = 2 + 3;return foo * bar;"
assert 6 "a=4;if(1<2)a=6;a;"
echo OK