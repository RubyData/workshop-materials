#include <memory>

#include <opencv2/imgcodecs.hpp>

#include <opencv-glib/image.h>
#include <opencv-glib/image-error.h>

G_BEGIN_DECLS

/**
 * SECTION: image
 * @title: Image class
 * @include: opencv-glib/opencv-glib.h
 *
 * #GCVImage is a special matrix class for image.
 *
 * Since: 1.0.0
 */

// 親クラスがGObjectではなくGCVMatrixなのでG_TYPE_OBJECTではなく
// GCV_TYPE_MATRIXを指定している。
// `cv::Mat`を保存する場所はGCVMatrixが持っているので、GCVImageは
// プライベート領域は必要ない。そのため、G_DEFINE_TYPE_WITH_PRIVATE
// ではなくてG_DEFINE_TYPEを使っている。
G_DEFINE_TYPE(GCVImage, gcv_image, GCV_TYPE_MATRIX)

// オブジェクトが作られたときに呼ばれる。
// 今回は何もすることがないので空。
static void
gcv_image_init(GCVImage *object)
{
}

// クラスを初期化するときに呼ばれる。
// GCVMatrixでやっている処理で十分で、GCVImageですることはないので空。
static void
gcv_image_class_init(GCVImageClass *klass)
{
}

// gcv_image_newの本体。
// ここのコメントはドキュメントだけではなく、GObject Introspectionに
// とっても大事な情報なのでこのフォーマットで書くこと。
//
// 「@error: ...」は定形でこれでよい。
//
// 「Returns:」に「 (nullable):」を付けてNULLを返すこともあると
// GObject Introspectionに伝える。
/**
 * gcv_image_new:
 * @filename: The filename to be read.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 *
 * It reads an image from file. Image format is determined by the
 * content, not by the extension of the filename.
 *
 * Returns: A newly read #GCVImage.
 *
 * Since: 1.0.0
 */
GCVImage *
gcv_image_new(const gchar *filename, GError **error)
{
  // OpenCVが提供しているcv::imread()で画像ファイルを読み込む。
  auto cv_matrix_raw = cv::imread(filename, cv::IMREAD_UNCHANGED);
  // 読み込みに失敗すると空になっている。
  if (cv_matrix_raw.empty()) {
    // エラーを設定。errorがNULLの場合は何もしない便利関数。
    g_set_error(error,
                // エラードメイン
                GCV_IMAGE_ERROR,
                // エラーコード
                GCV_IMAGE_ERROR_READ,
                // エラーメッセージ。printfのフォーマットを使える。
                "Failed to read image: %s", filename);
    return NULL; // エラーのときはNULLを返す。
  }
  // cv_matrix_rawはcv::Matなのでstd::shared_ptr<cv::Mat>にする。
  auto cv_matrix = std::make_shared<cv::Mat>(cv_matrix_raw);
  // cv::mat（OpenCV）からGCVImage（OpenCV GLib）を作成。
  // g_object_newでGObject *型のオブジェクトを作成。
  auto image = g_object_new(GCV_TYPE_IMAGE,
                             "matrix", &cv_matrix,
                             NULL);
  // GObject *をGCVImage *にキャスト。
  // このインライン関数はヘッダーでG_DECLARE_DERIVABLE_TYPEを
  // 呼んだときに定義されていた。
  return GCV_IMAGE(image);
}

G_END_DECLS
