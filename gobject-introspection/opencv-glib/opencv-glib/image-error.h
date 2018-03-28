#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

// ここのコメントはなくてもいいけど、ドキュメントが生成できるので
// 書いておいた方がよい。
// GCVImageErrorがエラーの名前。
// エラーコードはGCV_IMAGE_ERROR_XXXのように共通のプレフィックスをつける。
// 共通のプレフィクスをつけると自動でコード名を生成してくれる。
// 今回のケースだと「read」、「write」、「unknown」というコード名を生成しれくれる。
/**
 * GCVImageError:
 * @GCV_IMAGE_ERROR_READ: Image read error.
 * @GCV_IMAGE_ERROR_RITE: Image write error.
 * @GCV_IMAGE_ERROR_UNKNOWN: Unknown error.
 *
 * Image related errors.
 *
 * Since: 1.0.0
 */
typedef enum {
  GCV_IMAGE_ERROR_READ,
  GCV_IMAGE_ERROR_WRITE,
  GCV_IMAGE_ERROR_UNKNOWN,
} GCVImageError;

// エラードメインを返すマクロ。
// 次のフォーマットにする習慣になっている。
//
//   #{大文字のモジュール名}_#{大文字のエラー名}という習慣がある。
#define GCV_IMAGE_ERROR (gcv_image_error_quark())

// エラードメインを返す関数。
GQuark gcv_image_error_quark(void);

G_END_DECLS
