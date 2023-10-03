#!/bin/bash

cc="$1"

ntest=0
npass=0

assert() {
  ((ntest++))
  echo "$1" | "$cc" - &>/dev/null
  status="$?"

  if [ "$status" -eq 0 ]; then
    echo "NG: $1 => (succeeded)" 1>&2
    echo "=====> expected: $2:$3: $4" 1>&2
  else
    mapfile -t v </tmp/9cc_err_info
    if [ "${v[0]}" = "$2" ] && [ "${v[1]}" = "$3" ] && [ "${v[2]}" = "$4" ]; then
      echo "OK: $1 => $2:$3: $4"
      ((npass++))
    else
      echo "NG: $1 => ${v[0]}:${v[1]}: ${v[2]}" 1>&2
      echo "=====> expected: $2:$3: $4" 1>&2
    fi
  fi

  rm -f /tmp/9cc_err_info
}

assert 'int main() { return 0 }' 1 23 "';'ではありません"
assert 'int main() { 12.3.4; }' 1 18 '小数点が多すぎます'
assert 'int main() { 0x12.3; }' 1 16 '浮動小数点数で10進数以外の表記はできません'
assert 'int main() { 42lL; }' 1 16 '不正なサフィックスです'
assert 'int main() { 12.3fl; }' 1 18 '不正なサフィックスです'
assert 'int main() { "foo; }' 1 21 \''"'\''がありません'
assert '/* comment' 1 1 'コメントが閉じられていません'
assert 'int main() { '\''a" }' 1 16 \'\'\''ではありません'
assert 'あ' 1 1 'トークナイズできません'

assert '#hoge' 1 2 '不正なディレクティブです'
assert '#include foo' 1 10 '"ファイル名" ではありません'
assert '#if 1.2
#endif' 1 5 '浮動小数点数は使えません'
assert '#if 1' 1 2 '対応する#endifがありません'
assert '#endif' 1 2 '対応する#ifがありません'

assert 'enum E { 42 };' 1 10 '識別子ではありません'

assert 'int main() { int *p; int *q; p+q; }' 1 31 'ポインタ同士の加算はできません'
assert 'int main() { int *p; 4-p; }' 1 23 '数値からポインタ値を引くことはできません'

assert 'int main() { int a[]; }' 1 18 '配列の要素数が指定されていません'
assert 'long s="foo";' 1 8 '初期化子の型が不正です'

assert 'int main() { case 1:; }' 1 14 'ここでcase文を使用することはできません'
assert 'int main() { switch(1){ case 1:;case 1:; } }' 1 33 '重複したcase文'
assert 'int main() { default:; }' 1 14 'ここでdefault文を使用することはできません'
assert 'int main() { switch(1){ default:; default:; } }' 1 35 '重複したdefault文'
assert 'int main() { break; }' 1 14 'ここでbreak文を使用することはできません'
assert 'int main() { continue; }' 1 14 'ここでcontinue文を使用することはできません'
assert 'int main() { goto L; M:; }' 1 19 'そのようなラベルはありません'

assert 'int n=3; int a[n]' 1 16 '定数式ではありません'

assert 'void x;' 1 6 'void型の変数を宣言することはできません'
assert 'int main() { void x; }' 1 19 'void型の変数を宣言することはできません'
assert 'const const int x;' 1 7 'constを複数指定することはできません'
assert 'volatile volatile int x;' 1 10 'volatileを複数指定することはできません'
assert 'restrict int x' 1 1 'ここでrestrictを使うことはできません'
assert 'int *const const p;' 1 12 'constを複数指定することはできません'
assert 'int *volatile volatile p;' 1 15 'volatileを複数指定することはできません'
assert 'int *restrict restrict P;' 1 15 'restrictを複数指定することはできません'
assert 'int f(static int x) {}' 1 7 'ここで記憶クラス指定子を使うことはできません'
assert 'static static int x;' 1 8 '記憶クラス指定子を複数指定することはできません'
assert 'int int x;' 1 5 '型の指定が正しくありません'

assert 'int main() { int x; y; }' 1 21 '定義されていない変数です'
assert 'int A; enum { A };' 1 15 'そのような識別子はすでに存在します'
assert 'int x; int x;' 1 12 'そのような識別子はすでに存在します'
assert 'int f() { int x; int x; }' 1 22 'そのような識別子はすでに存在します'
assert 'int f(int x) { int x; }' 1 20 'そのような識別子はすでに存在します'
assert 'struct T { int a; int a; };' 1 23 '同名のメンバがすでに存在します'
assert 'union U { int a; int a; };' 1 22 '同名のメンバがすでに存在します'

assert 'int f(int n) { __builtin_va_list ap; __builtin_va_start(ap,n); }' 1 56 '固定長引数関数で使うことはできません'
assert 'int f(int n, ...) { char* ap; __builtin_va_arg(ap, int); }' 1 48 'va_list型ではありません'
assert 'int f(int n, ...) { __builtin_va_list ap; int ap2; __builtin_va_copy(ap2,ap); }' 1 70 'va_list型ではありません'
assert 'int f(int n, ...) { int ap; __builtin_va_list ap2; __builtin_va_copy(ap2,ap); }' 1 74 'va_list型ではありません'
assert 'int a; int main() { a(); }' 1 21 '関数ではありません'
assert 'int f() { return; }' 1 11 '非void関数では値を返す必要があります'
assert 'void f() { return 0; }' 1 12 'void関数から値を返すことはできません'

assert 'struct E; enum E;' 1 16 '前回の宣言と型が一致しません'
assert 'enum E { A }; enum E { B };' 1 20 '列挙体の再定義はできません'
assert 'union T; struct T;' 1 17 '前回の宣言と型が一致しません'
assert 'struct T{int a;}; struct T{long a;};' 1 26 '構造体の再定義はできません'
assert 'enum U; union U;' 1 15 '前回の宣言と型が一致しません'
assert 'union T{int a;}; union T{long a;};' 1 24 '共用体の再定義はできません'
assert 'int main() { int x; x.a; }' 1 21 '構造体/共用体ではありません'
assert 'struct T{int a;}; int main() { struct T x; x.b; }' 1 46 'そのようなメンバは存在しません'
assert 'struct T; struct T x[3];' 1 21 '配列の要素の型が不完全です'
assert 'struct T; struct T f() {}' 1 20 '戻り値の型が不完全です'
assert 'struct T; int f(struct T x) {}' 1 26 'パラメータの型が不完全です'
assert 'struct T {struct T a;};' 1 20 'メンバの型が不完全です'
assert 'struct T x;' 1 10 '不完全な型の変数宣言です'
assert 'struct T; int main() { struct T x; }' 1 33 '不完全な型の変数宣言です'

assert 'int main() { int const x=42; x=7; }' 1 30 '読み取り専用の変数です'
assert 'int main() { int n=42; int *const p; p=&n; }' 1 38 '読み取り専用の変数です'
assert 'int main() { int n=42; int const *p; *p=64; }' 1 38 '読み取り専用の変数です'
assert 'int main() { const int a[3][2]; a[1][0]=42; }' 1 33 '読み取り専用の変数です'
assert 'struct T { int a; char b; }; int main() { const struct T x={}; x.b=42; }' 1 65 '読み取り専用の変数です'
assert 'typedef struct T { int a; char b; } T; int main() { const T x={}; x.b=42; }' 1 68 '読み取り専用の変数です'
assert 'typedef struct T { const int a; char b; } T; int main() { T x={}; x.a=42; }' 1 68 '読み取り専用の変数です'
assert 'struct T { const int a; char b; }; int main() { struct T x={}; x.a=42; }' 1 65 '読み取り専用の変数です'
assert 'int main() { int a[3]; a=42; }' 1 24 '左辺値ではありません'
assert 'int main() { int p; *p; }' 1 21 '参照外しできません'
assert 'int main() { void *p; *p; }' 1 23 "'void *'型の参照外しはできません"
assert 'int main() { ({ int x; }); }' 1 15 'voidを返すことはできません'

assert 'int main() { 2=42; }' 1 14 '代入の左辺値が変数ではありません'

assert 'int main() { 0.1&0.1; }' 1 17 '不正なオペランド'

echo
echo "[$npass/$ntest]"
if [ "$npass" -eq "$ntest" ]; then
  echo "All tests passed."
  echo
else
  exit 1
fi
