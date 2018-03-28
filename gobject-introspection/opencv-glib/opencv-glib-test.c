/* インストールしたヘッダーファイルを読み込む。 */
#include <opencv-glib/matrix.h>

int
main(void)
{
  GCVMatrix *matrix;

  /* 実装した関数を使ってGCVMatrixを作成。 */
  matrix = gcv_matrix_new();
  /* 作成する機能しか実装していないのですぐに破棄。 */
  g_object_unref(matrix);

  return 0;
}
