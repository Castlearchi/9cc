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

file_debug() {
  input="$1"

  ./9cc "$input" > tmp.s
  cc -c tmp.s -o tmp.o
  cc -c test/test.c -o test/test.o
  cc -o tmp tmp.o test/test.o
  if [[ -f tmp ]]; then
    ./tmp
  else
    echo "Compilation error"
  fi
}

assert 5 "main(){return 5;}"
assert 20 "main(){a = 5; b = 4;return a*b;}"
assert 4 "main(){a=3;if(a){a=4;}return a;}"
assert 10 "main(){a=10;return a;}sub_func(a, b){return (a - b);}"
assert 34 "func(){return 34;}main(){a=3;return func();}"
assert 3 "sub_func(a, b){return (a - b);}main(){a=3;return a;}"
assert 68 "sub_func(a, b){return (a - b);}main(){return sub_func(100, 32);}"

echo OK