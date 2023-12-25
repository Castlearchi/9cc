        #!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o - -w
ret3() { return 3; }
ret5() { return 5; }
add(x, y) { return x+y; }
sub(x, y) { return x-y; }

add6(a, b, c, d, e, f) {
  return a+b+c+d+e+f;
}

void alloc4(int **p, int w, int x, int y, int z) {
  *p = (int*)malloc(4 * sizeof(int));
  (*p)[0] = w;
  (*p)[1] = x;
  (*p)[2] = y;
  (*p)[3] = z;
}
EOF

assert() {
  expected="$1"
  input="$2"

  echo "$input" | ./9cc - > tmp.s || error "$input" 
  cc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    error "$input"
  fi
}

error() {
  input="$1"
  echo "Fail $input"
  ./9cc "$input"
  exit
}

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 25 'int main() { return 5+20; }'
assert 1 'int main() { return 5-4; }'
assert 21 'int main() { return 5+20-4; }'
assert 50 'int main() { return 5+20+15+10; }'
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

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 20 'int main() { int a; a=3; int z; z=5; int y; y=2; int w; w=10;return a+z+y+w; }'
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

assert 3 'int main() { int i; int j; i = 2; j = 1; j = i + j; return j;}'
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
assert 3 'int main() { int x; int *y; x=3; y=&x; return *y; }'
assert 3 'int main() { int x; int *y;  int **z; x=3; y=&x;z=&y; return **z; }'
assert 5 'int main() { int x; x=3; int *y; y=&x; *y=5; return x; }'

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
assert 3 'int main() { return ret(3); } int ret(int x) { return x; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 0 'int main() { int x; x = 0; int y; y = add2(3,4);return x*y; } int add2(int x, int y) { return x+y; }'
assert 70 'int main() { int x; x = 10; int y; y = add2(3,4);return x*y; } int add2(int x, int y) { return x+y; }'

assert 3 'int main() { int x; int *y; y = &x; *y = 3;return x;}'
assert 3 'int main() { int x; int *y; y = &x; x = 3; return *y;}'
assert 3 'int main() { int x;int *y;int **z; z = &y; y = &x; **z = 3; return x;}'
assert 10 'int main() { int x; de10(&x); return x;} int de10(int *y) { *y = 10; }'

assert 1 '
int main() {
int *p;
alloc4(&p, 1, 2, 4, 8);
return *p;
}'

assert 1 '
int main() {
int *p;
alloc4(&p, 1, 2, 4, 8);
int *q;
q = p;
return *q;
}'

assert 2 '
int main() {
int *p;
alloc4(&p, 1, 2, 4, 8);
int *q;
q = p + 1;
return *q;
}'


assert 4 '
int main() {
int *p;
alloc4(&p, 1, 2, 4, 8);
int *q;
q = p + 2;
return *q;
}'

assert 8 '
int main() {
int *p;
alloc4(&p, 1, 2, 4, 8);
int *q;
q = p + 3;
return *q;
}'

assert 4 'int main() { int x; return sizeof(x); }'
assert 4 'int main() { int x; return sizeof(x + 3); }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 8 'int main() { int *x; return sizeof(x + 5); }'
assert 8 'int main() { int x; return sizeof(&x); }'
assert 4 'int main() { int *x; return sizeof(*x); }'
assert 8 'int main() { int *x; return sizeof(&x); }'
assert 4 'int main() { int **x; return sizeof(**x); }'
assert 8 'int main() { int **x; return sizeof(x + 5); }'
assert 4 'int main() { return sizeof(1); }'
assert 4 'int main() { return sizeof(sizeof(2)); }'

assert 1 'int main() {int a[2];int *p;*a = 1;p=a;return *p;}'
assert 3 '
int main() {
int a[2];
*a = 1;
*(a + 1) = 2;
int *p;
p = a;
return *p + *(p + 1);
}'

assert 2 'int main() { int a[2]; a[0]=2; return *a;}'
assert 2 'int main() { int a[2]; a[0]=2; return a[0];}'
assert 3 'int main() { int a[2]; a[1]=3; return a[1];}'
assert 6 'int main() { int a[2]; a[0]=2; a[1]=3; return a[0]*a[1];}'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
assert 2 'int x; int y; int z; int main() { x=3; y=4; z=5; return x+y-z; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'

assert 0 'int main() { char x; x=0; return x; }'
assert 5 'int main() { char x; char y; x=1; y=4; return x+y; }'
assert 3 'int main() { char x; char y; x=-1; y=4; return x+y; }'
assert 4 'int main() { char x[3]; x[0]=-3; x[1]=1; return x[1]-x[0]; }'
assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 97 'int main() { return "a"[0]; }'
assert 1 'int main() { return sizeof(""); }'

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'

assert 34 'tests/fibonacci'
echo OK