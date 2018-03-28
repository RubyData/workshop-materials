#pragma once

// GCVMatrixのサブクラスを作るのでmatrix.hを読み込む。
#include <opencv-glib/matrix.h>

G_BEGIN_DECLS

// GCVImageの型情報を取得するための便利マクロ。
// GCV_TYPE_MATRIXと同じ位置付け。
#define GCV_TYPE_IMAGE (gcv_image_get_type())
// 親クラスがGObjectではなくGCVMatrixになっているところがポイント。
// 他はGCVMatrixのときと同様。
G_DECLARE_DERIVABLE_TYPE(GCVImage,
                         gcv_image,
                         GCV,
                         IMAGE,
                         GCVMatrix)
struct _GCVImageClass
{
  // 親クラスがGObjectではなくGCVMatrixなので、
  // GObjectClassではなくGCVMatrixClassになっている。
  GCVMatrixClass parent_class;
};

// 指定した名前の画像ファイルを読み込んでGCVImageを作る。
// エラーが発生したらerrorに格納する。
GCVImage *gcv_image_new(const gchar *filename, GError **error);

G_END_DECLS
