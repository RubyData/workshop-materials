// ヘッダーの重複読み込みを防ぐ。
// 昔は次のように書いていたが、イマドキ#pragma onceを
// サポートしていないコンパイラーはないので#pragma onceを
// 使うこと。
//
//   #include OPENCV_GLIB_MATRIX_H
//   #ifndef OPENCV_GLIB_MATRIX_H
//   ...（ここにヘッダーの本体を書く）...
//   #endif
#pragma once

// GObjectの機能を使うためのヘッダーファイル
#include <glib-object.h>

// GLibが提供しているマクロ。
// 最後のG_BEGIN_DECLSとペアで使う。
// ここで囲んだ部分はC++のシンボルではなくCのシンボルになる。
// OpenCV GLibではCのAPIを提供するので必ずこのマクロを使う。
//
// 実体は次のことをしている。
//
// // ここがG_BEGIN_DECLS相当
// #ifdef __cplusplus
//   extern "C" {
// #endif
// ...（ここがAPI定義本体）...
// // ここがG_END_DECLS相当
// #ifdef __cplusplus
//   }
// #endif
G_BEGIN_DECLS

// OpenCV GLibのMatrixクラスの型情報を取得するための便利マクロ。
// GObjectを使ったライブラリーは次のようなフォーマットで
// 型情報を取得するマクロを提供するのが習慣になっている。
//
//   #{大文字のモジュール名}_TYPE_#{大文字のクラス名}
#define GCV_TYPE_MATRIX (gcv_matrix_get_type())

// GObjectが提供しているクラス定義のための便利マクロ。
// 引数はそれぞれ次の意味。
//
//   * GCVMatrix: モジュール名を含んだクラス名
//   * gcv_matrix: このクラス用の関数のプレフィックス
//   * GCV: 大文字のモジュール名
//   * MATRIX: 大文字のクラス名（モジュール名なし）
//   * GObject: 親クラス名
//
// 大文字の名前を明示的に指定しているのは、Cではマクロで大文字小文字
// を変換する機能がないから。
G_DECLARE_DERIVABLE_TYPE(GCVMatrix,
                         gcv_matrix,
                         GCV,
                         MATRIX,
                         GObject)
// GCVMatrixクラスのデータ。
// 最初のメンバーは親クラスにする。
// これはCで継承を実現するためのテクニック。
// 子オブジェクトを親オブジェクトにキャストして扱えるようになる。
struct _GCVMatrixClass
{
  // G_DECLARE_DERIVABLE_TYPE()で親クラスとしてGObjectを指定したので
  // 最後にClassをつけたGObjectClassを指定する。
  GObjectClass parent_class;

  // サブクラスでメソッドをオーバーライドする場合は、
  // ここに関数ポインターを追加する。
};

// コンストラクター。
// #{プレフィックス}_newとするのが習慣。
GCVMatrix *gcv_matrix_new(void);
// 行列が空なら真をメソッド。
gboolean gcv_matrix_is_empty(GCVMatrix *matrix);

// G_BEGIN_DECLSに対応するマクロ。
G_END_DECLS
