#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o - -w
ret3() { return 3; }
ret5() { return 5; }
add(x, y) { return x+y; }
sub(x, y) { return x-y; }

add6(a, b, c, d, e, f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s || exit
  cc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 25 'int main() { return 5+20; }'
assert 1 'int main() { return 5-4; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return +20-10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 3 'int main() { int a; a=3; return a; }'
assert 8 'int main() { int a; a=3; int z; z=5; return a+z; }'

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 8 'int main() { int a; a=3; int z; z=5; return a+z; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
assert 3 'int main() { int foo; foo=3; return foo; }'
assert 8 'int main() { int foo123; foo123=3; int bar; bar=5; return foo123+bar; }'

assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 2 'int main() { if (1) {return 2;} else{return 3;} return 4; }'
assert 3 'int main() { if (0) {return 2;} else{return 3;} return 4; }'

assert 10 'int main() { int i; i=0; int j; j=0; for (i=0; i<5; i=i+1) j=i+j; return j; }'
assert 55 'int main() { int i; i=0; int j; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) return 3; return 5; }'

assert 10 'int main() { int i; i=0; while(i<10) i=i+1; return i; }'

assert 3 'int main() { {1; {2;} return 3;} }'
assert 5 'int main() { ;;; return 5; }'

assert 10 'int main() { int i; i=0; while(i<10) i=i+1; return i; }'
assert 10 'int main() { int i; i=0; while(i<10) {i=i+1;} return i; }'
assert 11 'int main() { int i; i=10; while(i<=10) {i=i+1;} return i; }'
assert 11 'int main() { int i; i=11; while(i<=10) {i=i+1;} return i; }'
assert 11 'int main() { int i; i=0; while(i<=10) {i=i+1;} return i; }'
assert 55 'int main() { int i; i=0; int j; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 'int main() { int x; x=3; return *&x; }'
assert 3 'int main() { int x; x=3; int y; y=&x; int z; z=&y; return **z; }'
assert 5 'int main() { int x; x=3; int y; y=5; return *(&x-8); }'
assert 3 'int main() { int x; x=3; int y; y=5; return *(&y+8); }'
assert 5 'int main() { int x; x=3; int y; y=&x; *y=5; return x; }'
assert 7 'int main() { int x; x=3; int y; y=5; *(&x-8)=7; return y; }'
assert 7 'int main() { int x; x=3; int y; y=5; *(&y+8)=7; return x; }'

assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 3 'int main() { int x; x=2; return add(1, x); }'
assert 6 'int main() { return add(1,add(2,3)); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 0 'int main() { int x = 0; int y = add2(3,4);return x*y; } int add2(int x, int y) { return x+y; }'
assert 70 'int main() { int x = 10; int y = add2(3,4);return 70; } int add2(int x, int y) { return x+y; }'

echo OK