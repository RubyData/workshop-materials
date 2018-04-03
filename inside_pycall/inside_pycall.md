# pycall.rb の仕組み

## はじめに

本ガイドは、pycall.rb の仕組みを詳細に解説します。まず全体像を説明し、その後は初期化処理、型変換、オブジェクトの扱い、関数呼び出しの順で説明していきます。

## pycall.rb の全体像

pycall.rb は大まかに分けると次の要素から構成されます。

- 共有ライブラリ libpython のバインディング
- Python オブジェクトラッパー
- Ruby オブジェクトラッパー
- 型変換機構
- Python の基本型 `list`, `dict`, `set`, `tuple` に対するラッパークラス群

## pycall.rb の初期化処理

ここからは pycall.rb の初期化処理について説明します。

### libpython の探索とロード

pycall.rb は共有ライブラリ libpython を実行時に探索し、動的にロードして利用します。この仕組みにより、pycall.rb は Python 2.7 と Python 3 系を同時にサポートできます。

libpython の探索及びロードは次のように行われます。

1. 環境変数 `LIBPYTHON` で指定された共有ライブラリが存在する場合はその動的ロードを試みる。動的ロードに成功したらそのライブラリを使用する。
2. メソッド `PyCall.init` の引数もしくは環境変数 `PYTHON` で指定された `python` コマンド (何も指定されない場合はコマンド検索パスで見つかる `python` コマンドが使用される) を利用し、共有ライブラリ libpython のファイル名と存在するディレクトリの候補をリストアップする。
3. リストアップされたディレクトリを対象に共有ライブラリを探索し、発見した順に動的ロードを試みる。動的ロードに成功したらそこで探索を中断し、そのライブラリを使用する。
4. 使用可能な libpython が見つからない場合は例外 `PyCall::PythonNotFound` を発生させる。

### pycall.rb コアの初期化

共有ライブラリ libpython のロードに成功した後は、pycall.rb のコア部分の初期化が行われます。コア部の初期化は次の処理で構成されます。

- Python オブジェクトのポインタを保持するクラス PyPtr の定義
- Python 型オブジェクトのポインタを保持するクラス PyTypePtr の定義
- Ruby と libpython を繋ぐヘルパ関数群の定義
- 型変換システムの定義
- Python インタープリタの初期化
- PyCall::PyError と PyCall::Tuple の定義
- GC ガード機構の初期化
- Python 側に Ruby オブジェクトを渡す際に利用する `PyCall.ruby_object` 型を Python 側に定義

## 型変換

Python と Ruby の間で自然にデータをやり取りするために、pycall.rb は自動的な型変換の仕組みを持っています。

### Python から Ruby への型変換表の初期状態

pycall.rb が初期状態で持っている Python から Ruby への型変換表は次の通りです。まずは、ハードコードされた変換規則を表で示します。

| 変換前の Python の型 | 変換後の Ruby のクラス | 備考 |
| --- | --- | --- |
| `NoneType` | `NilClass` | |
| `bool` | `TrueClass`, `FalseClass` | |
| `int` | `Integer` | |
| `long` (Python 2.7) | `Integer` | |
| `complex` | `Complex` | |
| `bytes` (Python 3) | `String` | エンコーディングは必ず `ASCII-8BIT` |
| `str` (Python 3) | `String` | エンコーディングは必ず `UTF-8` |
| `str` (Python 2.7) | `String` | エンコーディングは必ず `ASCII-8BIT` |
| `unicode` (Python 2.7) | `String` | エンコーディングは必ず `UTF-8` |
| `type` | `Class` | `extend(PyCall::PyTypeObjectWrapper)` 済み |
| `module` | `Module` | `extend(PyCall::PyTypeObjectWrapper)` 済み |

次に、後述の変換表によって管理されている変換規則の初期状態です。

| 変換前の Python の型 | 変換後の Ruby のクラス | 備考 |
| --- | --- | --- |
| `list` | `PyCall::List` | |
| `tuple` | `PyCall::Tuple` | |
| `dict` | `PyCall::Dict` | |
| `set` | `PyCall::Set` | |

### Python から Ruby への型変換表へのルールの追加

Python から Ruby への型変換表には新しいペアを登録できます。やり方は次の2ステップです。

1. 型変換ルールを追加したい Python の型に対するラッパークラスに名前をつける
2. ラッパークラスのプライベートクラスメソッド `register_python_type_mapping` を呼び出す

例えば、numpy gem では、次のように numpy.ndarray に対するラッパークラスとその型変換を定義しています。

```ruby
Numpy = PyCall.import_module('numpy')
module Numpy
  NDArray = self.ndarray
  class NDArray
    register_python_type_mapping

    # 以下ヘルパメソッドの定義などが続く
    ...
  end
end
```

### Ruby から Python への型変換

Ruby から Python への型変換は、関数呼び出しの際に次の規則で行われます。

| 変換前の Ruby のクラス | 変換後の Python の型 | 備考 |
| --- | --- | --- |
| `NilClass` | `NoneType` | |
| `TrueClass` | `bool` | |
| `FalseClass` | `bool` | |
| `Integer` | `int` | |
| `String` (encode が `ASCII-8BIT`) | `bytes` | |
| `String` (encode が `ASCII-8BIT` 以外) | `str` | `UTF-8` に変換される |
| `Symbol` | `str` | `UTF-8` に変換される |
| `Array` | `list` | |
| `Hash` | `dict` | |
| `PyCall::PyObjectWrapper` | `object` | `@__pyptr__` が参照する Python オブジェクトがそのまま使用される |

現時点 (バージョン 1.0.3) の pycall.rb は、Ruby から Python への型変換のカスタマイズをサポートしていません (将来の機能改善でサポートされる可能性はあります)。

## Python オブジェクトの Ruby 側での扱い

pycall.rb は Python オブジェクトを次の2段階に分けて管理します。

1. オブジェクトポインタの寿命管理
2. オブジェクトの機能へのアクセス方法の提供するラッパーオブジェクト

後者については、型オブジェクト、モジュールオブジェクト、一般のオブジェクトのそれぞれについて異なる方法で Ruby 側にラッパーオブジェクトを作ります。

### Python オブジェクトポインタの寿命管理

pycall.rb は Ruby 側で Python オブジェクトポインタを保持するためにクラス `PyCall::PyPtr` のインスタンスを用います。このクラスのインスタンスは、ポインタを保持するとその参照カウントをインクリメントし、GC によって解放される際に参照カウントをデクリメントします。そうすることで、Ruby 側で `PyCall::PyPtr` オブジェクトが生きている限り、それが保持する Python オブジェクトが解放されないようにします。

クラス `PyCall::PyPtr` は参照カウントの管理のみに責任を持つ小さなクラスであり、pycall.rb の内部でのみ意味を持ちます。Python の機能を利用するために pycall.rb を使う場合は、このクラスのインスタンスを直接触ることはまずあり得ません。

pycall.rb のユーザが直接触れることになるオブジェクトは、`PyCall::PyPtr` オブジェクトを保持するラッッパーオブジェクトと呼ばれるオブジェクトです。

### Python オブジェクトのラッパーオブジェクト

pycall.rb では、インスタンス変数 `@__pyptr__` に `PyCall::PyPtr` オブジェクトを保持し、`PyCall::PyObjectWrapper` モジュールで extend されたオブジェクトが Python オブジェクトのラッパーオブジェクトとして機能します。

型オブジェクト、モジュールオブジェクト、型変換が登録された型のインスタンスについては、特別なルールでラッパーオブジェクトが作成されます。それ以外のオブジェクトについては、Object クラスのインスタンスがラッパーオブジェクトになります。

#### 型オブジェクトの場合

型オブジェクトは Class クラスのインスタンスがラッパーオブジェクトになります。そして、ラッパーオブジェクトは `PyCall::PyObjectWrapper` ではなく `PyCall::PyTypeObjectWrapper` で extend されます。

#### モジュールオブジェクトの場合

モジュールオブジェクトは Module クラスのインスタンスがラッパーオブジェクトになります。

#### 型変換が登録されている型のオブジェクト

型変換が登録されている型のインスタンスは、変換後のクラスとして登録されたクラスのインスタンスでラップされます。

## Ruby オブジェクトの Python 側での扱い

pycall.rb は Ruby オブジェクトを Python 側で扱うための仕組みも持っています。
ただし、現時点 (バージョン 1.0.x) では、Python 側から Ruby をコールバックするために最低限必要なものだけを実装しています。

メソッド `PyCall.wrap_ruby_object` に Python 側へ渡したい Ruby オブジェクトを渡すと、Python 側のラッパーオブジェクトを作成して、そのポインタを保持する `PyCall::PyRubyPtr` オブジェクトを Ruby 側に返します (`PyCall::PyRubyPtr` は `PyCall::PyPtr` のサブクラスです)。この `PyCall::PyRubyPtr` オブジェクトを Python の関数に渡すと、それがそのまま Python 側で Ruby オブジェクトのラッパーオブジェクトとして振舞います。

Python 側のラッパーオブジェクトは `PyCall.ruby_object` 型のインスタンスです。この型のインスタンスは、以下の表に示すように Python 側のメソッドコールを Ruby のメソッドコールへ移譲します。

| Python 側のメソッド | Ruby 側のメソッド |
| --- | --- |
| `__repr__` | `inspect` |
| `__hash__` | `hash` |
| `__call__` | `call` |
| `__name__` | `name` もしくは `to_s` |
| その他 | 同名のメソッドがあれば移譲する (`respond_to?` で検査する) |

## まとめ

本ガイドでは、pycall.rb の仕組みについて解説しました。
