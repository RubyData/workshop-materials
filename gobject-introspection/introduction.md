# GObject Introspection入門

## 背景

RubyにはC・C++で実装されたライブラリーの機能を使えるようにするための仕組みがあります。この仕組みを使うと既存の高速・高機能なライブラリーをRubyから利用できます。これによりRubyの書きやすさとC・C++の速度を組み合わせて開発することができます。

C・C++で実装されたライブラリーの機能をRubyから使えるようにしたRubyのライブラリーのことを「バインディング」と呼びます。

データサイエンスの分野ではC・C++で実装されたライブラリーが多くあります。大量のデータを現実的な時間で扱うために性能が必要になるからです。

まだRubyのライブラリーとして利用できない機能を、新しくRubyでも利用できるようにするアプローチは、主に以下の2つです。

  * 既存のC・C++のライブラリーのバインディングを実装する

  * 1からRubyで実装する

多くの場合、前者の方が実装コスト・性能面で有利です。

よって、C・C++で実装された既存の有用なライブラリーのバインディングを実装することは、データサイエンスの分野でRubyをより活用するための現実的な方法の1つです。

この文書では[GObject Introspection][gobject-introspeciton]を使ってバインディングを実装する方法を順を追って説明します。この文書の読者が以下の1つ以上の状態になることを目指します。

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

// OpenCVのMatrixクラスの型情報を取得するための便利マクロ。
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

少し長いのですが、ほとんどがよく使うパターンのものなので、何度か作ると慣れるはずです。ドキュメントはコード中のコメントから[GTK-Doc][gtk-doc]というドキュメントツールで生成します。GTK-Docついては後で説明します。ここでは軽く説明するだけにします。

```cpp
// 自分で定義したヘッダーファイルを読み込む。
// #include "..."とせずにユーザーが使うのと同じ使い方にしておいた方が
// 問題が早く見つかってよい。
#include <opencv-glib/matrix.h>

// ヘッダーファイルでも使っていたマクロ。
// CのAPIとして公開するものはソースの中でもこのマクロで囲む。
G_BEGIN_DECLS

// GCVMatrixクラスの説明。
// GTK-Docのフォーマットを使う。
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
  // C++の場合はC++ 11以降で使えるstd::shared_ptrを使うのが便利。
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
  // C++ 11以降はautoと書くと明示的に型を書かなくてもよいので便利。
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
                              "The raw std::shared<cv::Matrix> *",
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
        license: 'BSD-3-Clause')

# APIのバージョン。プロジェクトのメジャーバージョンと合わせるとよい。
# GObject Introspectionで公開するAPIで使う。
api_version = '1.0'
# 共有ライブラリーのバージョン。
# libopoencv-glib.so.1.0.0の最後の「1.0.0」の部分が共有ライブラリーのバージョン。
so_version = '1'
library_version = '@0@.0.0'.format(so_version)

# prefix引数で指定されたインストール先。
prefix = get_option('prefix')
# ヘッダーファイルをインストールする場所。
include_dir = join_paths(prefix, get_option('includedir'))

# Mesonが提供するGLib関連の便利機能を使う。
# GLibは主にGNOMEプロジェクトで活用していて、この便利機能にはGLib関連だけ
# でなく、GNOME関連の便利機能も含まれているので'glib'ではなく'gnome'に
# なっている。
gnome = import('gnome')
# Mesonが提供するpkg-config関連の便利機能を使う。
# pkg-configはライブラリーを見つけるための便利ツール。
pkgconfig = import('pkgconfig')

# トップディレクトリーをヘッダーファイルの検索パスに設定する準備。
# ここではまだ設定していない。opencv-glib/以下で実際に設定する。
root_inc = include_directories('.')

subdir('opencv-glib')
```


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

[meson]:http://mesonbuild.com/

[ninja]:https://ninja-build.org/

[autotools]:https://www.gnu.org/software/automake/

[cmake]:https://cmake.org/
