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
