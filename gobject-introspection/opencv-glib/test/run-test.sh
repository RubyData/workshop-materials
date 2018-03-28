#!/bin/sh

# テストディレクトリーの絶対パス。
test_dir="$(cd $(dirname $0); pwd)"
# ビルドディレクトリーの絶対パス。
build_dir="$(cd .; pwd)"

# 実装があるディレクトリー名。
module="opencv-glib"

# 自動でビルドを実行。
if [ -f "build.ninja" ]; then
  ninja || exit $?
fi

# ビルドディレクトリー内の.soがあるディレクトリーを
# LD_LIBRARY_PATH環境変数に追加。
export LD_LIBRARY_PATH="${build_dir}/${module}:${LD_LIBRARY_PATH}"

# ビルドディレクトリー内の.typelibがあるディレクトリーを
# GI_TYPELIB_PATH環境変数に追加。
export GI_TYPELIB_PATH="${build_dir}/${module}:${GI_TYPELIB_PATH}"

${test_dir}/run-test.rb "$@"
