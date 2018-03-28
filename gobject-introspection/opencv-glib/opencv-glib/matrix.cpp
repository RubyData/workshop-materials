// C++11のstd::shared_ptrを使うためのヘッダーファイルを読み込む。
// std::shared_ptrについては後述する。
#include <memory>

// cv::Matを使うためのヘッダーファイルを読み込む。
#include <opencv2/core/mat.hpp>

// 自分で定義したヘッダーファイルを読み込む。
// #include "..."とせずにユーザーが使うのと同じ使い方にしておいた方が
// 問題が早く見つかってよい。
#include <opencv-glib/matrix.h>

// ヘッダーファイルでも使っていたマクロ。
// CのAPIとして公開するものはソースの中でもこのマクロで囲む。
G_BEGIN_DECLS

// GCVMatrixクラスの説明。
// GTK-Docのフォーマットを使う。
// @includeで指定しているopencv-glib/opencv-glib.hはユーザーが
// #includeに何を指定すればよいかという情報。まだ
// opencv-glib/opencv-glib.hは作成していない。後で作成する。
/**
 * SECTION: matrix
 * @title: Matrix class
 * @include: opencv-glib/opencv-glib.h
 *
 * #GCVMatrix is a matrix class.
 *
 * Since: 1.0.0
 */

// GCVMatrixオブジェクトのインスタンス変数を管理するプライベート領域。
// 名前は#{クラス名}Privateにする。
typedef struct {
  // GCVMatrixはOpenCVのcv::Matをラップするのでcv::Matを持っておく。
  // C++の場合はC++11以降で使えるstd::shared_ptrを使うのが便利。
  // std::shared_ptrを使うと複数箇所で同じオブジェクトを参照できるようになる。
  // Cの場合はポインターにしておけば十分なことがほとんど。
  std::shared_ptr<cv::Mat> matrix;
} GCVMatrixPrivate;

// クラスの実体を定義。
// プライベート領域を使わないときはG_DEFINE_TYPEでよい。
// 今回のように使うときはG_DEFINE_TYPE_WITH_PRIVATEをを使う。
// 引数はそれぞれ次の意味。ヘッダーファイルの
// G_DECLARE_DERIVABLE_TYPEと合わせること。
//
//   * GCVMatrix: モジュール名を含んだクラス名
//   * gcv_matrix: このクラス用の関数のプレフィックス
//   * G_TYPE_OBJECT: 親クラスの型情報
G_DEFINE_TYPE_WITH_PRIVATE(GCVMatrix, gcv_matrix, G_TYPE_OBJECT)

// オブジェクトからプライベート領域を取得する便利マクロ。
// #{大文字のプレフィックス}_GET_PRIVATEという名前で定義するのが習慣。
#define GCV_MATRIX_GET_PRIVATE(obj)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                               GCV_TYPE_MATRIX,         \
                               GCVMatrixPrivate))

// GObjectの「プロパティー」機能のための定数。
// 後で使う。
enum {
  // 値を1から使うために入れているだけのダミーの値。
  PROP_0,
  // 「matrix」プロパティーのID（数値）に名前を付けているだけ。
  PROP_MATRIX
};

// #{プレフィックス}_finalizeはオブジェクトが破棄されるときに呼ばれる。
// （呼ばれるように少し後で登録する。）
static void
gcv_matrix_finalize(GObject *object)
{
  // プライベート領域を取得。
  // C++11以降はautoと書くと明示的に型を書かなくてもよいので便利。
  auto priv = GCV_MATRIX_GET_PRIVATE(object);

  // std::shard_ptrを使っているときはnullptrを代入すると破棄できる。
  // ここでラップしているcv::Matを破棄しないとメモリーリークするので注意。
  priv->matrix = nullptr;

  // 親クラスのfinalizeを呼ぶ。
  // 必ず呼ぶこと。
  G_OBJECT_CLASS(gcv_matrix_parent_class)->finalize(object);
}

// #{プレフィックス}_set_propertyはプロパティーを設定するときに呼ばれる。
// （呼ばれるように少し後で登録する。）
static void
gcv_matrix_set_property(GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
  auto priv = GCV_MATRIX_GET_PRIVATE(object);

  switch (prop_id) {
  // 「matrix」プロパティーを設定したとき
  case PROP_MATRIX:
    // プロパティーの値をプライベート領域に保存。
    // std::shard_ptrを使うときはこのやり方になる。
    priv->matrix =
      *static_cast<std::shared_ptr<cv::Mat> *>(g_value_get_pointer(value));
    break;
  default:
    // 未知のプロパティーを指定されたときの処理。
    // 定形のコード。
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

// #{プレフィックス}_initはオブジェクトが作られたときに呼ばれる。
// 今回は何もすることがないので空。
static void
gcv_matrix_init(GCVMatrix *object)
{
}

// #{プレフィックス}_class_initはクラスを初期化するときに呼ばれる。
// フックの登録とプロパティーの登録をする。
static void
gcv_matrix_class_init(GCVMatrixClass *klass)
{
  GParamSpec *spec;

  auto gobject_class = G_OBJECT_CLASS(klass);

  // #{プレフィックス}_finalizeが呼ばれるようにする。
  gobject_class->finalize     = gcv_matrix_finalize;
  // #{プレフィックス}_set_propertyが呼ばれるようにする。
  gobject_class->set_property = gcv_matrix_set_property;

  // 「matrix」プロパティーを作成。
  spec = g_param_spec_pointer("matrix", // プロパティー名
                              "Matrix", // プロパティーのニックネーム
                              // プロパティーの説明
                              "The raw std::shared<cv::Mat> *",
                              // オブジェクト作成時のみ設定でき、
                              // 読み込みはできない設定。
                              // CのAPIからC++のオブジェクトを
                              // 触らせたくないから。
                              static_cast<GParamFlags>(G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  // 「matrix」プロパティーを登録。
  g_object_class_install_property(gobject_class, PROP_MATRIX, spec);
}

// gcv_matrix_newの本体。
// ここのコメントはドキュメントだけではなく、GObject Introspectionに
// とっても大事な情報なのでこのフォーマットで書くこと。
/**
 * gcv_matrix_new:
 *
 * Returns: A newly created empty #GCVMatrix.
 *
 * Since: 1.0.0
 */
GCVMatrix *
gcv_matrix_new(void)
{
  // cv::Matオブジェクトの作成。
  auto cv_matrix = std::make_shared<cv::Mat>();
  // cv::Mat（OpenCV）からGCVMatrix（OpenCV GLib）を作成。
  // g_object_newでGObject *型のオブジェクトを作成。
  auto matrix = g_object_new(GCV_TYPE_MATRIX, // GCVMatrixの型情報。
                             // 「matrix」プロパティーにcv::Matを設定。
                             "matrix", &cv_matrix,
                             NULL);
  // GObject *をGCVMatrix *にキャスト。
  // このインライン関数はヘッダーでG_DECLARE_DERIVABLE_TYPEを
  // 呼んだときに定義されていた。
  return GCV_MATRIX(matrix);
}

// gcv_matrix_is_emptyの本体。
// ここのコメントはドキュメントだけではなく、GObject Introspectionに
// とっても大事な情報なのでこのフォーマットで書くこと。
//
// @matrixは「matrix」という名前の引数のドキュメントに対応する。
// すべての引数について説明を書かないといけない。
//
//
// 真偽値を返す関数の「Returns:」はこのように「%TRUE if ..., %FALSE otherwise.」
// というフレーズを使うのが便利。説明の書き方はGLibのリファレンスマニュアルを
// 参考にするとよい。
/**
 * gcv_matrix_is_empty:
 * @matrix: A #GCVMatrix
 *
 * Returns: %TRUE if the matrix is empty, %FALSE otherwise.
 *
 * Since: 1.0.0
 */
gboolean
gcv_matrix_is_empty(GCVMatrix *matrix)
{
  // 最初にNULLチェックなど引数のバリデーションをするのもアリ。
  // プライベート領域を取得。
  auto priv = GCV_MATRIX_GET_PRIVATE(matrix);
  // cv::Matにあるemptyメソッドを利用。
  return priv->matrix->empty();
}

G_END_DECLS
