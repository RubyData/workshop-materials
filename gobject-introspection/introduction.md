# GObject Introspection入門

この文書ではRubyユーザー向けにGObject Introspectionでバインディングを開発する方法を説明します。

## 背景

RubyにはC・C++で実装されたライブラリーの機能を使えるようにするための仕組みがあります。この仕組みを使うと既存の高速・高機能なライブラリーをRubyから利用できます。これによりRubyの書きやすさとC・C++の速度を組み合わせて開発することができます。

C・C++で実装されたライブラリーの機能をRubyから使えるようにしたRubyのライブラリーのことを「バインディング」と呼びます。

データサイエンスの分野ではC・C++で実装されたライブラリーが多くあります。大量のデータを現実的な時間で扱うために性能が必要になるからです。

まだRubyのライブラリーとして利用できない機能を、新しくRubyでも利用できるようにするアプローチは、主に以下の2つです。

  * 既存のC・C++のライブラリーのバインディングを実装する

  * 1からRubyで実装する

多くの場合、前者の方が実装コスト・性能面で有利です。

よって、C・C++で実装された既存の有用なライブラリーのバインディングを実装することは、データサイエンスの分野でRubyをより活用するための現実的な方法の1つです。

この文書では[GObject Introspection][gobject-introspection]を使ってバインディングを実装する方法を順を追って説明します。この文書の読者が以下の1つ以上の状態になることを目指します。

  * GObject Introspectionを使ってバインディングを実装できる

  * 既存のGObject Introspectionを使ったバインディングを改良・修正できる

なお、読者にはRubyの一般的な知識およびC・C++をなんとなく読める程度の知識があることを前提とします。

## GObject Introspectionと他のバインディング実装方法の違い

GObject IntrospectionはCで実装されたライブラリーのバインディングの実装を強力に支援するライブラリー・ツール群です。C++で実装されたライブラリーでもCでラップすることでGObject Introspectionを利用できます。実際、この文書では[OpenCV][opencv]というC++で実装されたコンピュータービジョン用のライブラリーのバインディングをGObject Introspectionを使って実現する方法を実例として使って説明します。

GObject Introspection自体は2005年に開発が始まったプロダクトです。つまり、10年以上継続的に改良が続いている安心して使えるプロダクトということです。類似のプロダクトに[SWIG][swig]があります。これは1995年から開発がはじまっているので20年以上継続的に改良が続いています。

バインディングを実装する主な方法は次の通りです。

  * Rubyが提供するC APIを利用して、Cでバインディングを実装する

  * [Fiddle][fiddle]を利用して、Rubyでバインディングを実装する

  * SWIGを利用して、Cで実装されたバインディングを自動生成する

  * GObject Introspectionを利用して、実行時にバインディングを自動生成する

単純なCのライブラリーであれば、最初の2つの方法（CまたはRubyでバインディングを実装する方法）が現実的です。そうでない場合は最後の2つの方法（SWIGまたはGObject Introspectionを利用する方法）も検討します。

たとえば、OpenCVは1000以上の関数を提供している複雑なライブラリーなので、最後の2つの方法の採用も検討します。

実際のどの方法を採用するかを判断できるようになるために、それぞれの方法の違いを説明します。

### Rubyが提供するC APIを利用して、Cでバインディングを実装する

まず、「Rubyが提供するC APIを利用して、Cでバインディングを実装する」方法について説明します。

RubyはCでRubyのライブラリーを実装する仕組みを提供しています。この仕組みを使って実装したライブラリーを「拡張ライブラリー」と呼びます。拡張ライブラリーを実装するためにRubyはC APIを提供しています。

Rubyが提供しているC APIには、たとえば、Cの文字列（`char *`）をRubyの文字列（`String`オブジェクト）にする`rb_str_new()`やクラスを定義する`rb_define_class()`などがあります。つまり、Rubyで書いているような処理をCでも書けるということです。

RubyのC APIを使うと、C・C++で実装された既存のライブラリーとRubyの橋渡し部分をCで実装できます。この橋渡し部分を「バインディング」というので、Cでバインディングを書けるということです。

たとえば、疑似乱数整数を返すC関数`rand()`のバインディングは次のようになります。

```c
#include <stdlib.h> /* rand()の定義を読み込む */

#include <ruby.h> /* RubyのC APIを読み込む */

static VALUE /* VALUEはCの世界でRubyのオブジェクトを表す型 */
rb_c_rand(VALUE self)
{
  /* INT2NUM()はCのintからRubyのIntegerオブジェクトを作る */
  return INT2NUM(rand());
}

void
Init_c_rand(void)
{
  /* トップレベルでdef c_rand ... endするのと同じ */
  rb_define_global_function("c_rand", rb_c_rand, 0);
}
```

「Rubyが提供するC APIを利用して、Cでバインディングを実装する」方法のメリットは次の通りです。

  * RubyのC APIは使いやすいので慣れてくれば技術的に難しいところはあまりない

  * 他の方法に比べてオーバーヘッドが少ないので、最も高速な実装になりやすい

一方、デメリットは次の通りです。

  * 各機能をそれぞれ実装することに加え、Cで実装するため、手間がかかる

### Fiddleを利用して、Rubyでバインディングを実装する

次は、「Fiddleを利用して、Rubyでバインディングを実装する」方法について説明します。

RubyにはFiddleという標準ライブラリーがあります。これは[libffi][libffi]という「実行時に任意のCの関数を呼び出す機能」を提供するライブラリーのバインディングです。つまり、Fiddleを使うと、Rubyから（Cのコードを書かずに）任意のCの関数を呼び出せるようになります。なお、[ffi gem][ffi-gem]もFiddleと同様にlibffiのバインディングです。ffi gemを使ってもメリット・デメリットはあまり変わりません。

たとえば、疑似乱数整数を返すC関数`rand()`のバインディングは次のようになります。

```ruby
require "fiddle/import"

module STDLib
  # このモジュールの中でFiddleを使う
  extend Fiddle::Importer
  # rand()を定義しているCで実装されたライブラリーを読み込む
  dlload "libc.so.6"
  # rand()を定義をパースしてRubyから呼び出せるようにする
  extern "int rand(void)"
end

# Cで実装された関数を呼び出す
p STDLib.rand
```

一見、Cのことを知らなくてもRubyだけ知っていれば使えるんじゃないかという気持ちになりますが、そんなことはありません。むしろ、「Rubyが提供するC APIを利用して、Cでバインディングを実装する」方法よりもCのことを知らないと困ることが多いです。

たとえば、`dlload`に指定するライブラリー名で困ることがあります。普通にCのライブラリーとしてリンクして使うときは、ライブラリー名やライブラリーの場所の検出を支援するツールを使えます。たとえば、[pkg-config][pkg-config]がそのようなツールです。このようなツールを使えば具体的なライブラリー名やパスは環境に合わせて自動で見つかるので気にする必要がありません。しかし、Fiddleを使うときは自分でそのあたりのことをケアしないといけないため、pkg-configがやってくれるようなことをRubyで自分で実装する必要があります。

また、構造体や関数の定義で困ることもあります。Cでバインディングを書くときは`#include`するだけでよかったですが、Fiddleを使う場合は`#include`できないので自分でFiddleに構造体や関数の定義を伝える必要があります。アラインメントのことを気にしたり、バージョンによって定義が違う場合のケアをするなど、Cでバインディングを書くときよりもCの知識が必要になります。

なお、C++で実装されたライブラリーにはこの方法を使えません。コンパイラーによって関数やクラスなどの名前をmangleする方法が違ったり、例外やテンプレートなどC++特有の機能があるからです。

「Fiddleを利用して、Rubyでバインディングを実装する」方法のメリットは次の通りです。

  * 単純な機能であればCの知識があまりなくても実装できる

  * コンパイルする必要がない

一方、デメリットは次の通りです。

  * Cで実装されたライブラリーを見つける処理の実装が面倒

  * 少し込み入ったことをしようとするとCの詳しい知識が必要

  * 対応したいCの関数定義を列挙するのが面倒

    * Cのヘッダーファイルをパースする機能を実現（Rubyで実装したり既存のCコンパイラーを利用したり）して自動化しようとする試みはあります。

  * C++で実装されたライブラリーには使えない

  * libffiのオーバーヘッドがそれなりにあるのでCで実装する場合に比べて遅くなる

  * Cの関数をRubyで利用できるようにするだけなので、Rubyから使いやすいAPIをRubyで作る必要がある

### SWIGを利用して、Cで実装されたバインディングを自動生成する

次は、「SWIGを利用して、Cで実装されたバインディングを実装する」方法について説明します。

SWIGはCのヘッダーファイルをパースして関数定義や`enum`などを抽出して「Rubyが提供するC APIを利用して、バインディングを実装したCのコード」を生成します。Fiddleを使う方法では自分で関数を列挙しなければいけませんでしたが、SWIGではそれをしなくてもよいです。

SWIGが生成するのはCのコードなので、コンパイルしないと使えません。Fiddleを使った方法ではコンパイルせずに使えました。

たとえば、次のような`User`オブジェクトにあるとします。

```c
/* user.h */
#pragma once

typedef struct User User;

User *user_new(const char *name, int age);
void user_free(User *user);

const char *user_get_name(User *user);
int user_get_age(User *user);
```

この`User`ライブラリーのバインディングを作るSWIGの入力ファイルは次のようになります。


```swig
// user.i

%module user

%{
#include "user.h"
%}

%include "user.h"
```

次のコマンドを実行すると`user_wrap.c`にCのコードを生成します。

```console
% swig -ruby user.i
```

あとは通常の拡張ライブラリー（RubyのC APIを使ってCで実装したRubyのライブラリー）と同様にビルドします。

このバインディングは次のように使います。

```ruby
require_relative "user.so"

user = User.user_new("Alice", 29)
p User.user_get_name(user) # -> "Alice"
p User.user_get_age(user)  # -> 29
User.user_free(user)
```

このように、SWIGが自動で抽出した関数の名前はCのまま`user_new`や`user_get_name`のようになるので、Rubyからは使いやすくありません。

RubyらしいAPIにするには次のどちらかのやり方があります。

  * SWIGのソースでカスタマイズ

  * RubyでSWIGが生成したAPIをラップ

SWIGで生成したバインディングは手で書いたバインディングよりもオーバーヘッドが大きいことが多いです。といってもlibffiよりは小さいです。

「SWIGを利用して、Cで実装されたバインディングを実装する」方法のメリットは次の通りです。

  * 自動でバインディング対象の関数や`enum`を抽出してくれるのですぐに網羅的に機能を使える

  * libffiよりはオーバーヘッドが小さい

一方、デメリットは次の通りです。

  * 自動抽出で定義されたAPIは使いにくいので、結局人手の作業が必要になる

  * CだけでなくSWIGの知識が必要

### GObject Introspectionを利用して、実行時にバインディングを自動生成する

最後は、「GObject Introspectionを利用して、実行時にバインディングを自動生成する」方法について説明します。

Fiddleを使った方法で関数を自分で列挙しなければいけないのは、Cのライブラリーがどのような関数を提供しているかを自動抽出できないからです。SWIGを使った方法で関数を自動抽出できるのは、ヘッダーファイルから必要な情報を取得しているからです。ライブラリーから直接情報を取得しているわけではありません。

GObject Introspectionを使うと、提供している関数の情報をライブラリー側が提供するようになります。ヘッダーファイルでも提供しているとは言えますが、GObject Introspectionはもっと再利用しやすい形で提供しています。たとえば、`const char *user_get_name(User *user)`という文字列をパースしなくても必要な情報を取得できます。

ヘッダーファイルをパースする方法では、ビルドオプションが違うと違う結果になることがありますが、GObject Introspectionの方法ではそのようなことはありません。

GObject Introspectionのアプローチではプログラムの実行時に関数の情報を使うことも簡単です。GObject Introspectionを使うとlibffiを使って実行時にバインディングを作ることができます。
（SWIGのように実行時ではなくビルド時にバインディングを自動生成することもできます。）

以下に各アプローチの違いを図示しました。

```text
Fiddleのアプローチ：

|<- Cのライブラリーの範囲 ->|    |<- Rubyの範囲 ---------------->|
+--------------+                +-------+
|Cのライブラリー| <------------- |libffi | <- 関数の情報 - 開発者
+--------------+                +-------+

SWIGのアプローチ：

|<- Cのライブラリーの範囲 ->|    |<- Rubyの範囲 ------->|
+--------------+                +---------------------+
|Cのライブラリー| <------------  |自動生成したCのコード |
+--------------+                +---------------------+
                                                     ^
+---------------+                                    |
|ヘッダーファイル| -------------- SWIG - 関数の情報 --+
+---------------+

GObject Introspctionのアプローチ：

|<- Cのライブラリーの範囲 ->|    |<- Rubyの範囲 ---------------->|
+--------------+                +------------------------------+
|Cのライブラリー| <------------- |実行時にlibffiを使ってアクセス |
+--------------+                +------------------------------+
                                     ^
+---------------+                    |
|.typelibファイル| ------------------+
+---------------+
  関数の情報
```

GObject Introspectionが提供する関数の情報はオブジェクト指向な情報も含んでいるのでRubyから自然に使えます。たとえば、SWIGで使った例のようなライブラリーの場合は次のように使えるAPIを実行時に自動で生成します。ビルドする必要はありません。

```ruby
user = User.new("Alice", 29)
p user.name # -> "Alice"
p user.age  # -> 29
```

「GObject Introspectionを利用して、実行時にバインディングを自動生成する」方法のメリットは次の通りです。

  * 実行時にバインディングを自動生成できる

  * 使いやすいAPIを自動生成できる

  * ライブラリー側が持っている関数の情報を提供するので、ライブラリーを更新するだけで、バインディング側を更新しなくても最新の機能を使える

  * Rubyではデータをゼロコピーで再利用するための便利な仕組みを実装済み

一方、デメリットは次の通りです。

  * Fiddleよりもオーバーヘッドが大きい

  * CのライブラリーがGObject Introspectionに対応していない場合はGObject Introspectionに対応したラッパーCライブラリーを開発しないといけない

  * CだけでなくGObject（Cでオブジェクト指向な機能を実現するためのライブラリー）の知識が必要

この文書では以下の点がデータサイエンスの分野で重要なためGObject Introspectionを使った方法を説明します。

  * 使いやすいAPIを自動生成できる

  * データをゼロコピーで再利用しやすい

## GObject Introspection対応ライブラリーの開発方法

GObject Introspection対応ライブラリーはCまたはC++で開発します。このライブラリーのAPIは使いやすいオブジェクト指向なAPIとして設計します。基本的にRubyで使いやすいAPIを設計するときと同様に考えればよいです。ただし、`each`を使ったAPIのようなRuby特有のAPIは含めません。

Cは言語としてオブジェクト指向なAPIをサポートしていません。そのため、[GObject][gobject]というCでオブジェクト指向なAPIの実現をサポートするライブラリーを使います。実は、GObject IntrospectionはGObjectをベースとした仕組みです。

この文書ではOpenCVのGObject Introspection対応ライブラリーを例に説明します。OpenCVがC++で実装されているため、GObject Introspection対応ライブラリーもCではなくC++を使います。GObjectを使う部分についてはCでもC++でも変わらないのでこの文書の説明はCの場合でも有効です。

実際のコードは[OpenCV GLibのリポジトリー][opencv-glib-repository]で確認できます。「OpenCV GObject」ではなく「OpenCV GLib」なのはGObjectは[GLib][glib]というライブラリー内の機能の一部だからです。

### 最初の一歩

まず1つ簡単な機能を実装します。最初に簡単な機能を実装するのがオススメです。理由は、最初はビルドシステムも整備する必要があるからです。最初は機能の実装よりもビルドシステムの整備に注力しましょう。

ここでは、簡単な機能として「`cv::Mat`オブジェクトを作れる」機能だけを実装します。`cv::Mat`はOpenCVが提供する行列クラスです。`cv`はOpenCVが使っているネームスペースです。`Mat`は「Matrix（行列）」を省略したものです。画像は縦横にピクセルが並んだものなので、行列と考えることもできます。そのため、OpenCVではただの行列も画像もすべて`cv::Mat`として統一的に扱っています。OpenCVはコンピュータービジョンのライブラリーなので画像は非常に重要な要素です。つまり、`cv::Mat`は非常に重要なクラスです。

`cv::Mat`を作る方法はいくつもありますが、一番簡単な方法は空の行列を作る方法です。この方法では引数なしで`cv::Mat`を作れます。まずはこの作り方で作れるようにしましょう。

### 最初の簡単な実装

GObject Introspection対応ライブラリーではオブジェクト指向なAPIを設計すると説明していました。CのAPIとしてオブジェクト指向なAPIを設計する場合「モジュール名」を決めます。モジュール名は関数や定数のプレフィックスとして使います。Cにはネームスペース機能がないので、プレフィックスを決めて他のライブラリーと名前が衝突しないようにします。

OpenCVのネームスペースが`cv`なのでOpenCV GLibでは最初にGLibの「G」をつけて`gcv`とします。

モジュール名が決まったのでOpenCVの`cv::Mat`に対応するOpenCV GLibのクラス名を決めます。一般的には同じ名前をつけるのですが、`Mat`のように省略している場合は本当に同じ名前でよいか検討します。省略した名前だと読みにくいコードになりがちなので、よほど長い名前でない限りは省略しない方がよいです。

`Matrix`はそれほど長くないのでOpenCV GLibでは省略せずに`Matrix`とすることにします。

それではヘッダーファイルを作ります。

ファイル名は`opencv-glib/matrix.h`とします。インストールしたときにヘッダーファイルが衝突しないように`opencv-glib/`ディレクトリー以下に配置します。ソース内でもこのディレクトリー構成を維持しておくとインクルードパスを通すときに便利です。

内容は次の通りです。詳細はコメントで説明しています。

```cpp
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

// G_BEGIN_DECLSに対応するマクロ。
G_END_DECLS
```

それでは実装を作ります。ファイル名は`opencv-glib/matrix.cpp`です。

少し長いのですが、ほとんどがよく使うパターンのものなので、何度か作ると慣れるはずです。ドキュメントはコード中のコメントから[GTK-Doc][gtk-doc]というドキュメントツールで生成します。この文書ではGTK-Docをビルドシステムに組み込む方法については省略します。対応するときは[OpenCV GLibのコード][opencv-glib-repository]を参考にしたり、[Red Data Toolsのチャット][red-data-tools-gitter-ja]で相談してください。

```cpp
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

G_END_DECLS
```

### ビルドシステムの整備

実装ができたのでビルドシステムを整備します。

ビルドシステムには[Meson][meson]を使います。それほど有名なビルドシステムではないので聞いたことがないかもしれません。Mesonは2012年に開発が始まった比較的新しいビルドシステムです。

なぜMesonを使うかというとGLibがMesonを使っているからです。GLibが使っているビルドシステムだとGLib関連のサポートが充実しているので使いやすいのです。

実は、Mesonだけではビルドできません。Mesonは他のビルドシステムの設定ファイルを出力し、実際のビルドは他のビルドシステムを使います。GNU/Linuxなら[Ninja][ninja]を使い、WindowsならVisual Studioを使い、macOSならXcodeを使います。これはそれほど特別なアプローチではなく、[GNU Autotools][autotools]や[CMake][cmake]など有名なビルドシステムもこのアプローチです。

Mesonでは各ディレクトリーに`meson.build`という設定ファイルを置きます。

まず、トップレベルの`meson.build`は次のようになります。基本的にプロジェクト全体で有効にしたい情報を設定するだけです。あとで、実装がある`oepncv-glib/`ディレクトリーにも`meson.build`を作るのですが、そこで利用するためにここで設定している項目もあります。一度、`opnecv-glib/meson.build`の中身も確認してから再度見直すと理解が深まるかもしれません。

詳細はコメントで説明します。

```meson
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
```

`opencv-glib/meson.build`は次のようになります。トップレベルの`meson.build`ではプロジェクト全体の設定をしましたが、ここでは前述の実装をビルドするための設定をします。

詳細はコメントで説明します。

```meson
# ソースファイルのリスト。
# ソースファイルを追加したら増やしていく。
# Rakefileのように*.cppのようなパターンで指定する機能はない。
# これはMesonは高速に動くことを大事にしているから。
# パターンを使えるようにするとどうしても速度が落ちてしまうので
# あえてサポートしていない。
sources = files(
  'matrix.cpp',
)

# ヘッダーファイルのリスト。
# ヘッダーファイルを追加したら増やしていく。
headers = files(
  'matrix.h',
)

# ヘッダーファイルを#{prefix}/include/opencv-glib/以下にインストールする。
# meson.project_name()はトップレベルのmeson.buildのproject()で
# 指定したプロジェクトID。今回のケースでは'opencv-glib'になる。
install_headers(headers, subdir: meson.project_name())

# 依存しているライブラリーのリスト。
# pkg-configで見つけられるライブラリーだと楽。
dependencies = [
  # OpenCVはopencvという名前でpkg-configで見つけられる。
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
                         sources: sources,
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
```

これだけではまだGObject Introspectionに対応していませんが、ライブラリーとしてビルドできるようにはなっています。最初の一歩はこれくらいの方がよいでしょう。

それでは、実際にビルドします。Mesonは必ずビルド用のディレクトリーを作らなければいけないことに注意してください。ソースコードと同じディレクトリーでビルドすることはできません。ここではソースコードの1つ上のディレクトリーに`opencv-glib.build`というディレクトリーを作ることにします。また、インストール先は`/tmp/local`にします。デフォルトでは`/usr/local`になりますが、一般ユーザーはここに書き込めません。開発時は`/tmp/local`や`~/local`などの一般ユーザーで書き込み可能なディレクトリーの方が便利です。

```console
% rm -rf ../opencv-glib.build   # 念のためビルドディレクトリーを削除
% meson ../opencv-glib.build \
   --prefix=/tmp/local          # MesonでNinja用のファイルを生成
% ninja -C ../opencv-glib.build         # Ninjaでビルド
% ninja -C ../opencv-glib.build install # インストール
```

これで以下のようにファイルがインストールされます。

  * `/tmp/local/include/opencv-glib/matrix.h`
  * `/tmp/local/lib/x86_64-linux-gnu/libopencv-glib.so.1.0.0`

`/tmp/local/lib/libopencv-glib.so.1.0.0`ではなく`x86_64-linux-gnu`になっているのは、Debian GNU/Linuxでは複数のアーキテクチャー（x86やamd64など）を同じシステムにインストールできるようになっているためです。`lib/`以下にライブラリーをインストールすると異なるアーキテクチャーでファイル名が衝突するため`lib/`の下にアーキテクチャー毎にサブディレクトリーを作っています。

なお、次のようにMesonに`--libdir=lib`オプションを指定することでサブディレクトリーを作らなくなります。

```console
% meson ../opencv-glib.build \
   --prefix=/tmp/local \
   --libdir=lib
```

開発用のインストールでは複数のアーキテクチャーをサポートする必要はないので、以降の説明では`--libdir=lib`を指定した前提で説明します。

### 動作確認

試しにインストールしたライブラリーを使ってみましょう。

次の内容の`opencv-glib-test.c`を作成します。

```c
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
```

次のようにビルドします。

```console
% gcc -o opencv-glib-test \
    -I/tmp/local/include \
    $(pkg-config --cflags gobject-2.0) \
    opencv-glib-test.c \
    -L/tmp/local/lib \
    -lopencv-glib \
    $(pkg-config --libs gobject-2.0)
```

実行してクラッシュせずに正常終了すれば問題ありません。現時点では`GCVMatrix`を作成する機能しかないのでこれ以上の確認方法はありません。

注意点は`LD_LIBRARY_PATH`環境変数を指定しないといけないことです。今回は開発用ということでシステム標準ではない場所にインストールしているため、`.so`を探す場所を指定しないといけません。

具体的には次のように実行します。

```console
% LD_LIBRARY_PATH=/tmp/local/lib ./opencv-glib-test
%
```

実装したものをビルドしてインストールして使えるようになりました。無事に最初の一歩を踏み出せましたね！

### GObject Introspection対応

それではGObject Introspectionに対応しましょう。GObject Introspectionに対応するとバインディングを実行時に自動生成できるようになるので、opencv-glibのテストをRubyで書けるようになります。この文書では、まずはGObject Introspectionに対応させ、その後Rubyでテストを書きます。

GObject Introspectionに対応させるには`opencv-glib/meson.build`を変更します。パラメーターが多いですが、次のように[`gnome.generate_gir`][gnome-generate-gir]を呼ぶだけです。

```meson
# GObject Introspection用のファイルを生成する。
gnome.generate_gir(# GObject Introspection対応させるライブラリー。
                   # library('opencv-glib', ...)の戻り値。
                   libopencv_glib,
                   # ソースファイルとヘッダーファイル。
                   sources: sources + headers,
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
```

それではビルドしてインストールしましょう。`meson`コマンドを手動で再実行する必要はありません。`meson.build`が変更されたら自動で再実行します。

```console
% ninja -C ../opencv-glib.build install
```

それではRubyから使ってみましょう。RubyでGObject Introspection対応ライブラリーを使うには[gobject-introspection gem][gobject-introspection-gem]を使います。

まずはgobject-introspection gemをインストールします。

```console
% sudo gem install gobject-introspection
```

次の内容の`opencv-glib-test.rb`を作成します。

```ruby
require "gi"

CV = GI.load("CV")
p CV::Matrix.new
```

GObject Introspection対応ライブラリーのバインディングを実行時に自動生成するには`.typelib`ファイルを見つけられないといけません。今回は開発用にインストール先を`/tmp/local`にしたので、OpenCV GLibの`.typelib`ファイルはシステム標準の場所にはありません。そのため、明示的に指定する必要があります。

`.typelib`ファイルの場所は`GI_TYPELIB_PATH`環境変数で指定します。今回はライブラリーのインストール先が`/tmp/local/lib`なので`.typelib`ファイルは`/tmp/local/lib/girepository-1.0/`以下にあります。

また、`.so`ファイルを見つけるために`LD_LIBRARY_PATH`環境変数も指定しなければいけないことに注意してください。

次のように`GI_TYPELIB_PATH`環境変数と`LD_LIBRARY_PATH`環境変数を指定して実行します。

```console
% GI_TYPELIB_PATH=/tmp/local/lib/girepository-1.0 \
    LD_LIBRARY_PATH=/tmp/local/lib \
    ruby opencv-glib-test.rb
#<CV::Matrix:0x560de825d6a8 ptr=0x560de83e4b90>
```

バインディングを実行時に自動生成してRubyで`GCVMatrix`オブジェクトを作れました。

なお、Ruby以外でもgobject-introspection gem相当のライブラリーがある言語では同様のことができます。たとえば、PythonやLuaでも同様のことができます。

以下はPythonの例です。

```python
import gi
gi.require_version("CV", "1.0")
from gi.repository import CV

matrix = CV.Matrix.new()
print(matrix)
```

[PyGObject][pygobject]を[インストール][pygobject-install]したら、次のように環境変数を指定すると実行できます。

```console
% GI_TYPELIB_PATH=/tmp/local/lib/girepository-1.0 \
   LD_LIBRARY_PATH=/tmp/local/lib \
   python3 opencv-glib-test.py
<CV.Matrix object at 0x7f4e2cb55a20 (GCVMatrix at 0x1706cd0)>
```

以下はLuaの例です。

```lua
local lgi = require 'lgi'
local CV = lgi.CV

local matrix = CV.Matrix.new()
print(matrix)
```

Luaでは[LGI][lgi]を使います。

```console
% sudo luarocks install lgi
```

次のように環境変数を指定すると実行できます。

```console
% GI_TYPELIB_PATH=/tmp/local/lib/girepository-1.0 \
   LD_LIBRARY_PATH=/tmp/local/lib \
   lua opencv-glib-test.lua
lgi.obj 0x5628d4ea54a0:CV.Matrix(GCVMatrix)
```

このようにGObject Introspectionに対応するとRuby以外の言語からも簡単に使えるようになります。このことのメリットは「Ruby以外の言語の人たちと共同でライブラリーをメンテナンスできる」ことです。Rubyコミュニティーに閉じないのでより開発を推進しやすくなります。

### Rubyでのテスト作成

GObject Introspectionに対応でき、Rubyでバインディングを自動生成できることを確認できたので、Rubyでテストを作成します。この文書では[test-unit][test-unit-gem]を使ってテストを開発します。

まず、test-unitをインストールします。

```console
% sudo gem install test-unit
```

次の内容の`test/test-matrix.rb`を作成します。オブジェクトを作れることだけを確認しています。


```ruby
require "test-unit"

require "gi"
CV = GI.load("CV")

class MatrixText < Test::Unit::TestCase
  test(".new") do
    assert_nothing_raised do
      CV::Matrix.new
    end
  end
end
```

次のように実行します。

```console
% GI_TYPELIB_PATH=/tmp/local/lib/girepository-1.0 \
    LD_LIBRARY_PATH=/tmp/local/lib \
    ruby test/test-matrix.rb
Loaded suite test/test-matrix
Started
.
Finished in 0.000701797 seconds.
--------------------------------------------------------------------------------
1 tests, 1 assertions, 0 failures, 0 errors, 0 pendings, 0 omissions, 0 notifications
100% passed
--------------------------------------------------------------------------------
1424.91 tests/s, 1424.91 assertions/s
```

Rubyでテストを書けましたね！

開発中は頻繁にテストを実行したいので、インストールしないとテストできないと不便です。インストールしなくてもテストできるように便利シェルスクリプトを作ります。

次の内容の`test/run-test.sh`を作成します。

```shell
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

ruby ${test_dir}/test-matrix.rb "$@"
```

このシェルスクリプトはビルドディレクトリー内から実行することを想定しています。つまり、次のように実行するということです。

```console
% (cd ../opencv-glib.build && ../opencv-glib/test/run-test.sh)
ninja: no work to do.
Loaded suite /tmp/opencv-glib/test/test-matrix
Started
.
Finished in 0.00067677 seconds.
--------------------------------------------------------------------------------
1 tests, 1 assertions, 0 failures, 0 errors, 0 pendings, 0 omissions, 0 notifications
100% passed
--------------------------------------------------------------------------------
1477.61 tests/s, 1477.61 assertions/s
```

環境変数を指定しなくてよくなりスッキリしましたね！

ただ、ディレクトリーを移動するのが面倒です。Ninjaから起動できるようにしましょう。

トップレベルの`meson.build`に次の内容を追加します。

```meson
# ninja testでtest/run-test.shを実行する。
run_test = find_program('test/run-test.sh')
test('unit test', run_test)
```

これで`ninja test`でテストを実行できるようになりました。

```console
% ninja -C ../opencv-glib.build test
ninja: Entering directory `../opencv-glib.build'
[0/1] Running all tests.
1/1 unit test                               OK       0.12 s

OK:         1
FAIL:       0
SKIP:       0
TIMEOUT:    0

Full log written to /tmp/opencv-glib.build/meson-logs/testlog.txt
```

そっけない表示になりましたが、テストが失敗すると次のように詳細が表示されるので大丈夫です。

```console
% ninja -C ../opencv-glib.build test
ninja: Entering directory `../opencv-glib.build'
[0/1] Running all tests.
1/1 unit test                               FAIL     0.13 s

OK:         0
FAIL:       1
SKIP:       0
TIMEOUT:    0


The output from the failed tests:

1/1 unit test                               FAIL     0.13 s

--- command ---
/tmp/opencv-glib/test/run-test.sh
--- stdout ---
ninja: no work to do.
Loaded suite /tmp/opencv-glib/test/test-matrix
Started
F
===============================================================================
Failure: test: .new(MatrixText):
  Exception raised:
  ArgumentError(<wrong number of arguments (1 for 0)>)
/tmp/opencv-glib/test/test-matrix.rb:8:in `block in <class:MatrixText>'
      5: 
      6: class MatrixText < Test::Unit::TestCase
      7:   test(".new") do
  =>  8:     assert_nothing_raised do
      9:       CV::Matrix.new(1)
     10:     end
     11:   end
===============================================================================

Finished in 0.003703944 seconds.
-------------------------------------------------------------------------------
1 tests, 1 assertions, 1 failures, 0 errors, 0 pendings, 0 omissions, 0 notifications
0% passed
-------------------------------------------------------------------------------
269.98 tests/s, 269.98 assertions/s
-------

Full log written to /tmp/opencv-glib.build/meson-logs/testlog.txt
FAILED: meson-test 
/usr/bin/python3 -u /usr/bin/meson test --no-rebuild --print-errorlogs
ninja: build stopped: subcommand failed.
```

ビルドもインストールもテストもすべて`ninja`経由で実行できる方が開発しやすい人は`ninja test`を使ってください。`ninja`経由の場合はすべて統一した操作になります。

`ninja`経由にこだわらない人は`test/run-test.sh`を直接実行してください。`test/run-test.sh`を直接実行しても自動で`ninja`でビルドするので機能的には変わりません。

この文書では`ninja test`を使います。統一した操作の方が読者がわかりやすいからです。

### 簡単な機能を追加

自動テストもできて一通り開発するための基盤が揃ったので機能を追加します。まずは行列が空かどうかを確認するだけの簡単な機能を追加します。

まずは、ヘッダーファイル`opencv-glib/matrix.h`に関数定義を追加します。`gboolean`はGLibが提供している真偽値のための型です。第一引数を`GCVMatrix *`にするとGObject Introspectionは`GCVMatrix`のメソッドと認識してくれます。

```c
// 行列が空なら真をメソッド。
gboolean gcv_matrix_is_empty(GCVMatrix *matrix);
```

実装は次のようになります。

```cpp
// 実装はG_BEGIN_DECLSとG_END_DECLSの間に置くこと。
// そうしないとCのシンボルとして扱われない。
G_BEGIN_DECLS
// ...

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

// ...
G_END_DECLS
```

テストは次のようになります。`test/test-matrix.rb`の既存のテストを変更します。gobject-introspection gemは`is_empty`という名前を自動でRubyっぽい`empty?`という名前に変換するのでRubyらしいAPIでテストを書けます。

```ruby
class MatrixText < Test::Unit::TestCase
  test(".new") do
    matrix = CV::Matrix.new
    assert do
      matrix.empty?
    end
  end
end
```

テストを実行します。

```console
% ninja -C ../opencv-glib.build test
ninja: Entering directory `../opencv-glib.build'
[3/5] Generating CV-1.0.gir with a custom command.
g-ir-scanner: link: cc -o /tmp/opencv-glib.build/tmp-introspecte9nukgr7/CV-1.0 /tmp/opencv-glib.build/tmp-introspecte9nukgr7/CV-1.0.o -L. -Wl,-rpath,. -Wl,--no-as-needed -lopencv-glib -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_datasets -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_line_descriptor -lopencv_optflow -lopencv_video -lopencv_plot -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_rgbd -lopencv_viz -lopencv_surface_matching -lopencv_text -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core -lgobject-2.0 -lglib-2.0 -L/tmp/opencv-glib.build/opencv-glib -Wl,-rpath,/tmp/opencv-glib.build/opencv-glib -lgio-2.0 -lgobject-2.0 -Wl,--export-dynamic -lgmodule-2.0 -pthread -lglib-2.0
[4/5] Running all tests.
1/1 unit test                               OK       0.12 s

OK:         1
FAIL:       0
SKIP:       0
TIMEOUT:    0

Full log written to /tmp/opencv-glib.build/meson-logs/testlog.txt
```

パスしましたね！Rubyのプログラムを書くときのように実装・テストを繰り返しながら開発を進めていけます。

### サブクラスの追加

簡単な機能については実装できるようになったので、次はサブクラスの実装方法を説明します。

OpenCVでは行列も画像もすべて`cv::Mat`で表現し、画像用の関数を使うときは画像用の`cv::Mat`を渡すというAPIになっています。GObject Introspection対応ライブラリーはオブジェクト指向なAPIにすることが大事です。そのため、そのままOpenCVのAPIをマッピングするのではなく、どのようなAPIがよいか検討しましょう。

オブジェクト指向なAPIではオブジェクトが自分に適した操作を持っているべきです。OpenCVのケースでは画像用の`cv::Mat`には画像用の機能がメソッドとして実装されているとオブジェクト指向なAPIになります。

よって、`GCVMatrix`を継承した`GCVImage`を作成し、`GCVImage`に画像用の機能を紐付けることにします。

次の内容のヘッダーファイル`opencv-glib/image.h`を作成します。詳細はコメントで説明します。

```c
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
GCVImage *gcv_image_new(const gchar *filename);

G_END_DECLS
```

次の内容のソースファイル`opencv-glib/image.cpp`を作成します。詳細はコメントで説明します。

```console
#include <memory>

#include <opencv2/imgcodecs.hpp>

#include <opencv-glib/image.h>

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
/**
 * gcv_image_new:
 * @filename: The filename to be read.
 *
 * It reads an image from file. Image format is determined by the
 * content, not by the extension of the filename.
 *
 * Returns: A newly read #GCVImage.
 *
 * Since: 1.0.0
 */
GCVImage *
gcv_image_new(const gchar *filename)
{
  // OpenCVが提供しているcv::imread()で画像ファイルを読み込む。
  auto cv_matrix_raw = cv::imread(filename, cv::IMREAD_UNCHANGED);
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
```

ファイルを増やしたので、`opencv-glib/meson.build`に追加します。

```meson
sources = files(
  'image.cpp', # 追加
  'matrix.cpp',
)

headers = files(
  'image.h', # 追加
  'matrix.h',
)
```

次の内容のテスト`test/test-image.rb`を作成します。

```ruby
require "test-unit"

require "gi"
CV = GI.load("CV")

class ImageText < Test::Unit::TestCase
  test(".new") do
    image = CV::Image.new(File.join(__dir__, "test.png"))
    assert do
      not image.empty? # 画像ファイルを読み込んだら空じゃない
    end
  end
end
```

テストファイルはOpenCV GLibのリポジトリー内にある[テスト画像ファイル][test-image-file]を使いましょう。

```console
% wget -O test/test.png https://raw.githubusercontent.com/red-data-tools/opencv-glib/master/test/fixture/mail-forward.png
```

テストファイルが複数になったので、複数のファイルを一度にテストできるように次の内容の`test/run-test.rb`を用意します。

```ruby
#!/usr/bin/env ruby

require "test-unit"

test_dir = __dir__

require "gi"
CV = GI.load("CV")

exit(Test::Unit::AutoRunner.run(true, test_dir.to_s))
```

`test/run-test.rb`には実行権をつけましょう。

```console
% chmod +x test/run-test.rb
```

テストファイル中の共通処理を`test/run-test.rb`に持ってきたのでテストファイルはそれぞれ次のようにテストだけになりました。

`test/test-matrix.rb`:

```ruby
class MatrixText < Test::Unit::TestCase
  test(".new") do
    matrix = CV::Matrix.new
    assert do
      matrix.empty?
    end
  end
end
```

`test/test-image.rb`:

```ruby
class ImageText < Test::Unit::TestCase
  test(".new") do
    image = CV::Image.new(File.join(__dir__, "test.png"))
    assert do
      not image.empty? # 画像ファイルを読み込んだら空じゃない
    end
  end
end
```

テスト実行用の便利シェルスクリプト`test/run-test.sh`は`test/run-test.rb`（こっちはRubyスクリプト）を呼ぶようにします。

```shell
# ...
# 途中は同じ。最後だけ変える。
# ...
# ruby ${test_dir}/test-matrix.rb "$@"から↓に変更
${test_dir}/run-test.rb "$@"
```

それではテストを実行します。

```console
% ninja -C ../opencv-glib.build test
...
1/1 unit test                               OK       0.20 s

OK:         1
FAIL:       0
SKIP:       0
TIMEOUT:    0
...
```

パスしましたね！うまく画像を読み込めたようです。

`test/test-image.rb`を変更して、試しに存在しないパスを指定してみましょう。

```ruby
class ImageText < Test::Unit::TestCase
  test(".new") do
    # 絶対存在しないパスを指定。
    image = CV::Image.new(File.join(__dir__, "nonexistent.png"))
    assert do
      not image.empty?
    end
  end
end
```

テストを実行します。

```console
% ninja -C ../opencv-glib.build test
...
1/1 unit test                               FAIL     0.20 s

OK:         0
FAIL:       1
SKIP:       0
TIMEOUT:    0


The output from the failed tests:

1/1 unit test                               FAIL     0.20 s

--- command ---
/tmp/opencv-glib/test/run-test.sh
--- stdout ---
ninja: no work to do.
Loaded suite test
Started
F
===============================================================================
Failure: test: .new(ImageText):
        not image.empty? # 画像ファイルを読み込んだら空じゃない
            |     |
            |     true
            #<CV::Image:0x55d7a6a48890 ptr=0x55d7a5e1f160>
/tmp/opencv-glib/test/test-image.rb:4:in `block in <class:ImageText>'
     1: class ImageText < Test::Unit::TestCase
     2:   test(".new") do
     3:     image = CV::Image.new(File.join(__dir__, "nonexistent.png"))
  => 4:     assert do
     5:       not image.empty? # 画像ファイルを読み込んだら空じゃない
     6:     end
     7:   end
===============================================================================
.
Finished in 0.005666598 seconds.
-------------------------------------------------------------------------------
2 tests, 2 assertions, 1 failures, 0 errors, 0 pendings, 0 omissions, 0 notifications
50% passed
-------------------------------------------------------------------------------
352.95 tests/s, 352.95 assertions/s
-------
...
```

失敗しました。`CV::Image.new`は成功して、その後の`not image.empty?`で失敗しています。ユーザーとしては、存在しないパスを指定したら`CV::Image.new`で例外が発生して欲しいです。

### エラー対応

それでは、`CV::Image.new`に存在しないパスを指定したら例外が発生するようにしましょう。

GLibには[`GError`][gerror]というエラーを扱うための仕組みがあります。これを使うとエラーが発生したらRubyレベルで例外にすることができます。

はじめにエラードメインとそのドメイン用のエラーコードを定義します。エラードメインとはエラーのグループのようなもので、関連したエラーをまとめたIDです。たとえば、ファイル関連のエラードメインを作って、その中にファイルが見つからないときのエラーコード、パーミッションがなかったときのエラーコードといったように使います。

ここでは画像関連のエラーのためのエラードメインを定義します。

次の内容のヘッダーファイル`opencv-glib/image-error.h`を作成します。

```c
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
```

次の内容のソース`opencv-glib/image-error.cpp`を作成します。

```cpp
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
```

ファイルを追加したので`opencv-glib/meson.build`を更新します。

```meson
sources = files(
  'image.cpp',
  'image-error.cpp', # 追加
  'matrix.cpp',
)

headers = files(
  'image.h',
  'image-error.h', # 追加
  'matrix.h',
)
```

これで画像関係のエラーを扱えるようになりました。`gcv_image_new()`を変更して画像の読み込みに失敗したらエラーにしましょう。

まず、`opencv-glib/image.h`を変更して`GError **`を受け取るようにします。エラーが起きたらここにエラー情報を設定します。

```c
// 指定した名前の画像ファイルを読み込んでGCVImageを作る。
// エラーが発生したらerrorに格納する。
GCVImage *gcv_image_new(const gchar *filename, GError **error);
```

それでは`opencv-glib/image.cpp`を変更して読み込みに失敗したらエラーにします。

```cpp
// ...
#include <opencv-glib/image-error.h>
// ...
/**
 * gcv_image_new:
 * @filename: The filename to be read.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 * ↑を追加。「@error: ...」は定形でこれでよい。
 *
 * Returns: (nullable): A newly read #GCVImage.
 * ↑に「 (nullable):」を追加。エラーのときはNULLを返す。
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
  // あとは同じ。
  // ...
}
```

これで画像の読み込みに失敗したときはエラーになるようにできたのでテストしましょう。

```console
% ninja -C ../opencv-glib.build test
...
Loaded suite test
Started
E
===============================================================================
Error: test: .new(ImageText): GLib::Error: Failed to read image: /tmp/opencv-glib/test/nonexistent.png
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:317:in `invoke'
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:317:in `block (2 levels) in load_constructor_infos'
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:328:in `block in load_constructor_infos'
/tmp/opencv-glib/test/test-image.rb:3:in `new'
/tmp/opencv-glib/test/test-image.rb:3:in `block in <class:ImageText>'
===============================================================================
.
Finished in 0.00295928 seconds.
-------------------------------------------------------------------------------
2 tests, 1 assertions, 0 failures, 1 errors, 0 pendings, 0 omissions, 0 notifications
50% passed
-------------------------------------------------------------------------------
675.84 tests/s, 337.92 assertions/s
...
```

画像が存在しないときは`GLib::Error`の例外が発生するようになりましたね！

ただ、`GLib::Error`だと画像関係のエラーだけ`rescue`するのが面倒です。`CV::ImageError`の例外が発生するようにしましょう。

### `enum`のオブジェクト化

`CV::ImageError`の例外が発生するようにするには`CV::ImageError`をオブジェクトにします。言い方を変えると、Cで`enum`で定義されているものをGObjectでのオブジェクトにします。

まだピンとこないかもしれません。これまで、Cで`struct`で定義したものをGObjectでのオブジェクトにしていきました。具体的には`GCVMatrix`と`GCVImage`です。`G_DECLARE_DERIVABLE_TYPE`や`G_DEFINE_TYPE`はそういうことをしていたのです。そうすることでいい感じのバインディングを自動生成してRubyで扱えるようになっていたのです。

同じようにCで`enum`として定義している`GCVImageError`もGObjectでのオブジェクトにします。`GCVMatrix`や`GCVImage`は`G_DECLARE_DERIVABLE_TYPE`や`G_DEFINE_TYPE`を使って手動で実装しましたが、`GCVImageError`は自動で実装できます。ヘッダーファイルを解析して`enum`をオブジェクトにするコードを生成するツールをGLibが提供しているのです。それが[`glib-mkenums`][glib-mkenums]です。

実は、Mesonには`glib-mkenums`を簡単に使うための機能が組み込まれているので簡単にビルドに組み込むことができます。

まず、コードを生成するときに使うテンプレートを用意しします。

ヘッダーファイルを生成するときに使う`opencv-glib/enums.h.template`は次の内容にします。このまま他のライブラリーでも使いまわせるはずです。変更が必要だとしたら`#include`するファイルを変えるくらいです。

```c
/*** BEGIN file-header ***/
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
/*** END file-production ***/

/*** BEGIN value-header ***/
GType @enum_name@_get_type(void) G_GNUC_CONST;
#define @ENUMPREFIX@_TYPE_@ENUMSHORT@ (@enum_name@_get_type())
/*** END value-header ***/

/*** BEGIN file-tail ***/

G_END_DECLS
/*** END file-tail ***/
```

ソースファイルを生成するときに使う`opencv-glib/enums.c.template`は次の内容にします。こちらも最初の`#include`しているところを変更すれば使いまわせるはずです。

```c
/*** BEGIN file-header ***/
#include <opencv-glib/opencv-glib.h>
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
/*** END file-production ***/

/*** BEGIN value-header ***/
GType
@enum_name@_get_type(void)
{
  static GType etype = 0;
  if (G_UNLIKELY(etype == 0)) {
    static const G@Type@Value values[] = {
/*** END value-header ***/

/*** BEGIN value-production ***/
      {@VALUENAME@, "@VALUENAME@", "@valuenick@"},
/*** END value-production ***/

/*** BEGIN value-tail ***/
      {0, NULL, NULL}
    };
    etype = g_@type@_register_static(g_intern_static_string("@EnumName@"), values);
  }
  return etype;
}
/*** END value-tail ***/

/*** BEGIN file-tail ***/
/*** END file-tail ***/
```

ついに`opencv-glib/opencv-glib.h`が必要になったので次の内容で作成します。`opencv-glib/enums.h`以外はすでに作成したファイルです。`opencv-glib/enums.h`はこれから自動生成しようとしているファイルです。

```c
#pragma once

// 自動生成予定のヘッダーファイル。
#include <opencv-glib/enums.h>
#include <opencv-glib/image.h>
#include <opencv-glib/image-error.h>
#include <opencv-glib/matrix.h>
```

これで自動生成する準備ができたので、あとは`opencv-glib/meson.build`に`glib-mkenums`を使う設定を追加するだけです。

```meson
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

# ...

# libopencv-glib.soをビルドする設定。
libopencv_glib = library('opencv-glib',
                         # ライブラリーのソース。
                         # 自動生成したファイルも対象にする。
                         sources: sources + enums,
                         # ...
                         )

# ...

# GObject Introspection用のファイルを生成する。
gnome.generate_gir(libopencv_glib,
                   # ソースファイルとヘッダーファイル。
                   # 自動生成したファイルも対象にする。
                   sources: sources + headers + enums,
                   # ...
                   )
```

これで`enum`もGObjectでのオブジェクトになります。これにより発生する例外が`GLib::Error`ではなく`CV::ImageError`になります。テストを実行してみましょう。

```console
% ninja -C ../opencv-glib.build test
...
Loaded suite test
Started
E
===============================================================================
Error: test: .new(ImageText): CV::ImageError::Read: Failed to read image: /tmp/opencv-glib/test/nonexistent.png
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:317:in `invoke'
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:317:in `block (2 levels) in load_constructor_infos'
/tmp/local/lib/ruby/gems/2.6.0/gems/gobject-introspection-3.2.1/lib/gobject-introspection/loader.rb:328:in `block in load_constructor_infos'
/tmp/opencv-glib/test/test-image.rb:3:in `new'
/tmp/opencv-glib/test/test-image.rb:3:in `block in <class:ImageText>'
===============================================================================
.
Finished in 0.004964249 seconds.
-------------------------------------------------------------------------------
2 tests, 1 assertions, 0 failures, 1 errors, 0 pendings, 0 omissions, 0 notifications
50% passed
-------------------------------------------------------------------------------
402.88 tests/s, 201.44 assertions/s
...
```

発生する例外が`CV::ImageError::Read`に変わりましたね！

それではテストを追加しましょう。正常に画像を読み込めるケースと読み込めないケースです。`test/test-image.rb`は次のようになります。

```ruby
class ImageText < Test::Unit::TestCase
  sub_test_case(".new") do
    test("valid") do
      image = CV::Image.new(File.join(__dir__, "test.png"))
      assert do
        not image.empty? # 画像ファイルを読み込んだら空じゃない
      end
    end

    test("nonexistent") do
      assert_raise(CV::ImageError::Read) do
        CV::Image.new(File.join(__dir__, "nonexistent.png"))
      end
    end
  end
end
```

テストを実行します。

```console
% ninja -C ../opencv-glib.build test
...
[0/1] Running all tests.
1/1 unit test                               OK       0.33 s

OK:         1
FAIL:       0
SKIP:       0
TIMEOUT:    0
...
```

パスしました。

まだ説明していないGObject・GObject Introspectionの機能や仕組みはいろいろありますが、多くの実装はここまで説明した内容で対応できるはずです。より詳しいことを知らなければいけなくなったときは[OpenCV GLibのコード][opencv-glib-repository]を参考にしたり、[Red Data Toolsのチャット][red-data-tools-gitter-ja]で相談してください。

### pkg-config対応

ここからは実装のための作業の説明ではなく、実際にユーザーに使ってもらうための作業の説明をします。

最初はpkg-configへの対応です。ライブラリーがpkg-configに対応していると、ユーザーはとても便利になります。たとえば、ライブラリーの存在チェックをしたり、ビルドフラグを取得したり、といったことが簡単に統一された方法をできるようになります。`opencv-glib/meson.build`内でもOpenCVがpkg-configに対応していたおかげで`dependency('opencv')`と書くだけで済みました。少し後で、GObject Introspectionに対応したライブラリー用のgemの作り方の説明をしますが、そのときにもpkg-configは便利です。

ライブラリーをpkg-configに対応させる方法は簡単です。`.pc`ファイルをインストールするだけです。

たとえば、今回のケースでは次の内容の`/tmp/local/lib/opencv-glib.pc`をインストールできればよいです。

```pc
prefix=/tmp/local
libdir=${prefix}/lib
includedir=${prefix}/include

Name: OpenCV GLib
Description: C API for OpenCV based on GLib
Version: 1.0.0
Requires: gobject-2.0 opencv
Libs: -L${libdir} -lopencv-glib
Cflags: -I${includedir}
```

Mesonにはpkg-config対応を支援する[`pkgconfig`モジュール][meson-pkgconfig]があります。それを使うと、`opencv-glib/meson.build`に以下を追加するだけでpkg-configに対応できます。

```meson
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
```

`ninja install`すると`/tmp/local/lib/opencv-glib.pc`が生成されます。

```console
% ninja -C ../opencv-glib.build install
% cat /tmp/local/lib/pkgconfig/opencv-glib.pc
prefix=/tmp/local
libdir=${prefix}/lib
includedir=${prefix}/include

Name: OpenCV GLib
Description: C API for OpenCV based on GLib
Version: 1.0.0
Requires: gobject-2.0 opencv
Libs: -L${libdir} -lopencv-glib
Cflags: -I${includedir}
```

### リリース方法

最後にライブラリーのリリース方法について説明します。

GNU Autotoolsではリポジトリー内のソースツリーとリリース用のソースツリー（`tar.gz`ファイル）は別物でした。リリース用のソースツリーにはリポジトリー内のソースツリーから自動生成されたファイルも含まれています。これは、ユーザー（リリース用のソースツリーを使う人たち）が準備しなければいけないツールを減らすためです。このアプローチのおかげで、開発者はGNU Autotoolsをインストールしなければいけないが、ユーザーはGNU Autotoolsがなくてもシェルと`make`があればビルドできる状態を実現していました。

一方、Mesonではリポジトリー内のソースツリーとリリース用のソースツリーは同じです。そのため、リリース時はリポジトリー内のソースツリーをアーカイブする（`tar.gz`を作成する）だけでよいです。GitかMercurialを使っている場合は`ninja dist`を実行すればリリース用のファイルを生成できます。なお、`ninja dist`では`tar.xz`を生成するだけでなく、生成した`tar.xz`を使ってユニットテストを実行したりインストールテストをしたり、`tar.xz`のSHA256チェックサムを生成したりもしてくれます。

```console
% ninja -C ../opencv-glib.build dist
```

生成されたファイルは`../oepncv-glib.build/meson-dist/`以下にあります。

```console
% ls ../opencv-glib.build/meson-dist
opencv-glib-1.0.0.tar.xz
opencv-glib-1.0.0.tar.xz.sha256sum
```

あとはこれを適切な場所にアップロードするだけです。

## GObject Introspection対応ライブラリー用のgemの開発方法

GObject Introspectionに対応したライブラリーの場合、実行時にバインディングを自動生成できることはすでに確認済みです。そのため、専用のgemを開発しなくても使えなくはありません。しかし、以下の理由から用意することをオススメします。

  * ユーザーが見つけやすい

    * gemになっていないとRubyから使えることに気付かないユーザーの方が多いです。

  * 細々した使い勝手をよりRubyらしくできる

ここではOpenCV GLibを使ったgemである[Red OpenCV][red-opencv]を例にGObject Introspection対応ライブラリー用のgemの開発方法を説明します。

まず、`red-opencv.gemspec`を作成します。ポイントは`extensions`に`dependency-check/Rakefile`を指定しているところと、gobject-introspection gemに依存させているところです。

```ruby
$LOAD_PATH.unshift(File.join(__dir__, "lib"))
# すぐ後で作成する。
require "cv/version"

Gem::Specification.new do |spec|
  spec.name = "red-opencv"
  spec.version = CV::VERSION
  spec.homepage = "https://github.com/red-data-tools/red-opencv"
  spec.authors = ["Kouhei Sutou"]
  spec.email = ["kou@clear-code.com"]

  spec.summary = "Red OpenCV is a Ruby bindings of OpenCV."
  spec.description = "You can use computer vision features in Ruby."
  spec.license = "BSD-3-Clause"
  spec.files = ["Rakefile", "Gemfile", "#{spec.name}.gemspec"]
  spec.files += Dir.glob("lib/**/*.rb")

  # これでOpenCV GLibがインストールされているかをチェックする。
  spec.extensions = ["dependency-check/Rakefile"]

  # gobject-introspection gemに依存させること。
  spec.add_runtime_dependency("gobject-introspection")

  spec.add_development_dependency("bundler")
  spec.add_development_dependency("rake")
  spec.add_development_dependency("test-unit")
end
```

次の内容の`lib/cv/version.rb`を作成します。単にバージョンを定義しているだけです。

```ruby
module CV
  VERSION = "1.0.0"
end
```

次の内容の`dependency-check/Rakefile`を作成します。`gem install red-opencv`実行時にこのファイルで定義した`default`タスクが実行されます。`default`が実行されると`opencv-glib`というpkg-configのパッケージがないか探します。なければ自動でdebやRPMをインストールしようとします。この文書ではdebやRPMの作成方法は説明していませんが、それらを用意しておくと、`gem install`しただけで自動で必要なライブラリーもインストールされるのでユーザーは非常に簡単にインストールできます。

```ruby
require "pkg-config"
require "native-package-installer"

task :default => "dependency:check"

namespace :dependency do
  desc "Check dependency"
  task :check do
    unless PKGConfig.check_version?("opencv-glib")
      unless NativePackageInstaller.install(:debian => "libopencv-glib-dev",
                                            :redhat => "opencv-glib-devel")
        exit(false)
      end
    end
  end
end
```

Red OpenCVではユーザーは次のようにライブラリーを使います。

```ruby
require "cv"

image = CV::Image.new("test.png")
```

そのため、次の内容の`lib/cv.rb`を用意します。

```ruby
require "gobject-introspection"

require "cv/version"

module CV
  class Loader < GObjectIntrospection::Loader
  end

  Loader.load("CV", self)
end
```

OpenCV GLibでは`GI.load("CV")`だけだった処理が`CV::Loader`を作って`CV::Loader.load`を呼ぶようになっています。実は`GI.load`はこれらの処理をやってくれる便利APIだったのです。今回説明する範囲ではやりませんが、`GObjectIntrospection::Loader`がバインディングを自動生成するオブジェクトなのですが、これの動きをカスタマイズすることでよりRubyらしくできます。そのため、`CV::Loader`と継承して、このgem用にカスタマイズできるようにしておくことをオススメします。

今回は`CV::Image.new`の引数に`String`だけでなく`Pathname`も渡せるようにしましょう。`Pathname`はRubyの標準ライブラリーであり、画像のファイル名を`Pathname`も指定できた方がよりRubyらしいAPIになるからです。

`lib/cv.rb`を以下のように変更します。本当は`lib/cv/image.rb`を作成してそれを`lib/cv.rb`から読み込む方が望ましいのですが、ファイルをわけない方が読者が混乱しにくいだろうという配慮から`lib/cv.rb`を変更しています。

```ruby
require "gobject-introspection"

require "cv/version"

module CV
  class Loader < GObjectIntrospection::Loader
  end

  Loader.load("CV", self)

  # これを追加。
  # Loader.loadの後ではImageは定義済み。
  class Image
    # 自動生成されたinitializeを対比
    alias_method :initialize_raw, :initialize
    def initialize(filename)
      # Rubyにはパスっぽいオブジェクトはto_pathを定義しておくという習慣がある。
      # PathnameやFileがto_pathを持っている。
      if filename.respond_to?(:to_path)
        filename = filename.to_path
      end
      initialize_raw(filename)
    end
  end
end
```

こうすることで次のコードも動くようになります。

```ruby
require "pathname"
require "cv"

image = CV::Image.new(Pathname("test.png"))
```

このように、Rubyに特化した使い勝手の部分はGObject Introspection対応ライブラリー側ではなくgem側で実現しましょう。ただし、一般的なオブジェクト指向なAPIの部分をgem側でがんばってはいけません。そこはGObject Introspection側でがんばるべきです。APIの設計はRubyでの知識がきっと役に立つはずです。

## まとめ

この文書では読者が以下の1つ以上の状態になることを目指しました。

  * GObject Introspectionを使ってバインディングを実装できる

  * 既存のGObject Introspectionを使ったバインディングを改良・修正できる

そのために以下のことについて説明しました。

  * Rubyのバインディングの代表的な開発方法の概略とメリット・デメリット

  * GObject Introspection対応ライブラリーの開発方法の概略

  * GObject Introspection対応ライブラリーを使ったgemの開発方法の概略

一方、以下のことについては説明を省略しました。これらの情報が必要になったら[Red Data Toolsのチャット][red-data-tools-gitter-ja]で相談してください。

  * GObject Introspection対応ライブラリーの開発方法のより立ち入った情報

  * GTK-Doc対応

  * deb/RPM/Homebrew/MSYS2用パッケージの作成方法

  * GObject Introspection対応ライブラリーを使ったgemの開発方法のより立ち入った情報

[gobject-introspection]:https://wiki.gnome.org/Projects/GObjectIntrospection

[opencv]:https://opencv.org/

[swig]:http://www.swig.org/

[fiddle]:https://docs.ruby-lang.org/ja/latest/library/fiddle.html

[libffi]:https://sourceware.org/libffi/

[ffi-gem]:https://github.com/ffi/ffi/wiki

[pkg-config]:https://www.freedesktop.org/wiki/Software/pkg-config/

[gobject]:https://developer.gnome.org/gobject/stable/

[opencv-glib-repository]:https://github.com/red-data-tools/opencv-glib

[glib]:https://developer.gnome.org/glib/stable/

[gtk-doc]:https://www.gtk.org/gtk-doc/

[red-data-tools-gitter-ja]:https://gitter.im/red-data-tools/ja

[meson]:http://mesonbuild.com/

[ninja]:https://ninja-build.org/

[autotools]:https://www.gnu.org/software/automake/

[cmake]:https://cmake.org/

[gnome-generate-gir]:http://mesonbuild.com/Gnome-module.html#gnomegenerate_gir

[gobject-introspection-gem]:https://rubygems.org/gems/gobject-introspection

[pygobject]:https://pygobject.readthedocs.io/

[pygobject-install]:https://pygobject.readthedocs.io/en/latest/getting_started.html

[lgi]:https://github.com/pavouk/lgi

[test-unit-gem]:https://rubygems.org/gems/test-unit

[test-image-file]:https://raw.githubusercontent.com/red-data-tools/opencv-glib/master/test/fixture/mail-forward.png

[gerror]:https://developer.gnome.org/glib/stable/glib-Error-Reporting.html

[glib-mkenums]:https://developer.gnome.org/gobject/stable/glib-mkenums.html

[meson-pkgconfig]:http://mesonbuild.com/Pkgconfig-module.html

[red-opencv]:https://github.com/red-data-tools/red-opencv
