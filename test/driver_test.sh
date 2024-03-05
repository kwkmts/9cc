#!/bin/bash

tmp=$(mktemp -d /tmp/9cc-driver-test-XXXXXX)
trap 'rm -rf $tmp' EXIT
echo >"$tmp"/empty.c

check() {
  if [ "$?" -eq 0 ]; then
    echo "OK: $1"
  else
    echo "NG: $1"
    exit 1
  fi
}

# デフォルト出力ファイル
rm -f "$tmp"/empty.s
(cd "$tmp" && "$OLDPWD"/9cc "$tmp"/empty.c)
[ -f "$tmp"/empty.s ]
check "default output file"

# -o <ファイル>
rm -f "$tmp"/empty.s
./9cc -o "$tmp"/empty.s "$tmp"/empty.c
[ -f "$tmp"/empty.s ]
check "-o <file>"

# -o<ファイル>
rm -f "$tmp"/empty.s
./9cc -o"$tmp"/empty.s "$tmp"/empty.c
[ -f "$tmp"/empty.s ]
check "-o<file>"

# -E
echo '#define M foo
M' >"$tmp"/out
./9cc -E "$tmp"/out | grep -q foo
check -E

echo '#define M foo
M' >"$tmp"/out1
./9cc -E -o "$tmp"/out2 "$tmp"/out1
grep -q foo "$tmp"/out2
check "-E -o"

# -I
mkdir "$tmp"/include
echo foo >"$tmp"/include/I_opt_test
echo '#include "I_opt_test"' | ./9cc -I"$tmp"/include -E - | grep -q foo
check -I

# -D
echo FOO | ./9cc -D FOO -E - | grep -q 1
check -D

echo FOO | ./9cc -DFOO=BAR -E - | grep -q BAR
check "-D<macro>=<value>"

# -U
echo FOO | ./9cc -D FOO=BAR -U FOO -E - | grep -q FOO
check -U

# -include
echo foo >"$tmp"/out.h
echo bar | ./9cc -include "$tmp"/out.h -E -o- - | grep -q -z 'foo.*bar'
check -include

# ハッシュマップのテスト
./9cc -hashmap-test
check "hashmap test"
