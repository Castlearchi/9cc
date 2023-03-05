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
assert 3 "if(2)return 3;"
assert 6 "a=4;if(1<2)a=6;a;"
assert 7 "num=9;if(3>4)num=num-1;else num=num-2;"
assert 5 "i=0;while(i<5) i=i+1; return i;"
assert 6 "for(i=0;i<3;i = i+1) i = 2 * i;return i;"
assert 3 "for(i=0;i<3;) i = 1 + i;return i;"
echo OK