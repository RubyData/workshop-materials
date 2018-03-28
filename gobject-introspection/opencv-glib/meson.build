project('opencv-glib',    # プロジェクトのID
        'c', 'cpp',       # このプロジェクトではCとC++を使う
        version: '1.0.0', # プロジェクトのバージョン
        # ライセンスは3条項BSDライセンス。OpenCVと合わせた。
        # プロジェクトに合わせて変更する。
        license: 'BSD-3-Clause')

# APIのバージョン。プロジェクトのメジャーバージョンと合わせるとよい。
# GObject Introspectionで公開するAPIで使う。
api_version = '1.0'
# 共有ライブラリーのバージョン。
# libopoencv-glib.so.1.0.0の最後の「1.0.0」の部分が共有ライブラリーのバージョン。
library_version = '1.0.0'

# Mesonが提供するGLib関連の便利機能を使う。
# GNOMEはGLibを特に活用しているプロジェクト。この便利機能にはGLib関連だけ
# ではなく、GNOME関連の便利機能も含まれているので'glib'ではなく'gnome'に
# なっている。
gnome = import('gnome')
# Mesonが提供するpkg-config関連の便利機能を使う。
# pkg-configはライブラリーを見つけるための便利ツール。
pkgconfig = import('pkgconfig')

# トップディレクトリーをヘッダーファイルの検索パスに設定する準備。
# ここではまだ設定していない。opencv-glib/以下で実際に設定する。
root_inc = include_directories('.')

# opencv-glib/ディレクトリーのmeson.buildも処理する。
subdir('opencv-glib')

# ninja testでtest/run-test.shを実行する。
run_test = find_program('test/run-test.sh')
test('unit test', run_test)
