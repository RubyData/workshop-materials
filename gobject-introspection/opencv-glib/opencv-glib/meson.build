# ソースファイルのリスト。
# ソースファイルを追加したら増やしていく。
# Rakefileのように*.cppのようなパターンで指定する機能はない。
# これはMesonは高速に動くことを大事にしているから。
# パターンを使えるようにするとどうしても速度が落ちてしまうので
# あえてサポートしていない。
sources = files(
  'image.cpp',
  'image-error.cpp',
  'matrix.cpp',
)

# ヘッダーファイルのリスト。
# ヘッダーファイルを追加したら増やしていく。
headers = files(
  'image.h',
  'image-error.h',
  'matrix.h',
)

# ヘッダーファイルを#{prefix}/include/opencv-glib/以下にインストールする。
# meson.project_name()はトップレベルのmeson.buildのproject()で
# 指定したプロジェクトID。今回のケースでは'opencv-glib'になる。
install_headers(headers, subdir: meson.project_name())

# enumをGObjectでのオブジェクトにするための実装を自動生成する。
enums = gnome.mkenums('enums',
                      # enumの検出対象となるヘッダーファイル。
                      sources: headers,
                      # クラス名のプレフィクス。
                      # gnome.generate_girで指定するものと同じ。
                      identifier_prefix: 'GCV',
                      # 関数名などのプレフィックス。
                      # gnome.generate_girで指定するものと同じ。
                      symbol_prefix: 'gcv',
                      # Cのソースを生成するために使うテンプレート。
                      c_template: 'enums.c.template',
                      # ヘッダーファイルを生成するために使うテンプレート。
                      h_template: 'enums.h.template',
                      # ヘッダーファイルをインストールする。
                      install_header: true,
                      # ヘッダーファイルのインストール先のディレクトリー。
                      # #{prefix}/include/opencv-glib/enums.hに
                      # インストールしたいのでmeson.project_name()を
                      # 使っている。
                      # meson.project_name()はトップレベルのmeson.buildの
                      # project()で指定したプロジェクトID。
                      # 今回のケースでは'opencv-glib'になる。
                      install_dir: join_paths(get_option('includedir'),
                                              meson.project_name()))

# 依存しているライブラリーのリスト。
# pkg-configで見つけられるライブラリーだと楽。
dependencies = [
  # OpenCVはopencvという名前でpkg-configで見つけられる。
  # macOSの場合は'opencv4'と記述する(homebrew利用時)
  dependency('opencv'),
  # GObjectは必ず依存関係に含める。
  # GObject Introspectionに対応するには必須だから。
  dependency('gobject-2.0'),
]
# libopencv-glib.soをビルドする設定。
libopencv_glib = library(# ライブラリー名。
                         # GNU/Linuxではlib#{ライブラリー名}.soという名前の
                         # 共有ライブラリーをビルドする。
                         'opencv-glib',
                         # ライブラリーのソース。
                         sources: sources + enums,
                         # ライブラリーをインストールする。
                         install: true,
                         # このライブラリーが依存するライブラリー。
                         dependencies: dependencies,
                         # ビルド時に検索するヘッダーファイルのディレクトリー。
                         include_directories: [
                           # トップレベルのディレクトリー。
                           # この変数はトップレベルのmeson.buildで定義していた。
                           root_inc,
                         ],
                         # 共有ライブラリーのバージョン。
                         # GNU/Linuxでは
                         # libopencv-glib.so.#{バージョン}
                         # というファイルを作る。
                         # library_versionはトップレベルのmeson.buildで'1.0.0'と
                         # 定義しているのでliboepncv-glib.so.1.0.0となる。
                         version: library_version)

# GObject Introspection用のファイルを生成する。
gnome.generate_gir(# GObject Introspection対応させるライブラリー。
                   # library('opencv-glib', ...)の戻り値。
                   libopencv_glib,
                   # ソースファイルとヘッダーファイル。
                   sources: sources + headers + enums,
                   # GObject Introspectionの世界でのネームスペース。
                   # ソースコード中ではOpenCVと重複しないようにGCVと
                   # Gを付けたが、GObject Introspectionの世界にOpenCV
                   # はないので、ここではつける必要はない。
                   namespace: 'CV',
                   # GObject Introspectionの世界でのバージョン。
                   # GObject Introspectionの世界では、異なるバージョンが
                   # 同時に存在できるので、APIが変わったときはここを変える
                   # ことで古いバージョンと共存できる。
                   # api_versionはトップレベルのmeson.buildで定義していた。
                   # 今回は'1.0'になっている。
                   # '#{メジャーバージョン}.#{マイナーバージョン}'という
                   # フォーマットにする習慣がある。
                   nsversion: api_version,
                   # クラス名のプレフィックス。
                   identifier_prefix: 'GCV',
                   # 関数名などのプレフィックス。
                   symbol_prefix: 'gcv',
                   # pkg-configでのパッケージ名。
                   # まだpkg-configに対応していないが後で対応するので
                   # 指定しておく。
                   export_packages: 'opencv-glib',
                   # 依存しているGObject Introspectionの世界のライブラリー。
                   # GObject Introspectionの世界のライブラリーの名前は
                   # '#{ネームスペース}-#{メジャーバージョン}.#{マイナーバージョン}'
                   # というフォーマットになっている。
                   includes: [
                     'GObject-2.0',
                   ],
                   # インストールする。
                   install: true,
                   # ビルド時の警告をすべて表示する。
                   extra_args: [
                     '--warn-all',
                   ])

# Mesonが提供するpkg-config用の便利機能を使う。
pkg = import('pkgconfig')
# .pcファイルを生成し、インストールする。
pkg.generate(# #{プロジェクトID}.pcというファイル名にする。
             # meson.project_name()はトップレベルのmeson.buildの
             # project()で指定したプロジェクトID。
             # 今回のケースでは'opencv-glib'になるので、
             # ファイル名はopencv-glib.pcになる。
             filebase: meson.project_name(),
             # パッケージ名。
             name: 'OpenCV GLib',
             # パッケージの説明。
             description: 'C API for OpenCV based on GLib',
             # パッケージのバージョン。
             # meson.project_version()はトップレベルのmeson.buildの
             # project()で指定したバージョン。
             # 今回のケースでは'1.0.0'になる。
             version: meson.project_version(),
             # 依存しているpkg-configのパッケージのリスト。
             requires: ['gobject-2.0', 'opencv'],
             # このパッケージをユーザーが使うときにリンクするライブラリー。
             libraries: [libopencv_glib])
