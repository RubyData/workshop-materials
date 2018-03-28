#include <opencv-glib/image-error.h>

G_BEGIN_DECLS

/**
 * SECTION: image-error
 * @title: GCVImageError
 * @short_description: Image Error
 *
 * #GCVImageError provides image related error codes.
 */

// エラードメインを定義する便利マクロ。
//
// 第一引数は次のフォーマットにする習慣になっている。
//
//   #{ハイフンつなぎのエラー名}-quark」
//
// エラーIDになるので重複しないように注意。
//
// 第二引数はアンダースコアつなぎのエラー名を指定する。
// 以下の関数を定義してくれる。
//
//   #{アンダースコアつなぎのエラー名}_quark
G_DEFINE_QUARK(gcv-image-error-quark, gcv_image_error)

G_END_DECLS
