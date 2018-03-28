require "gobject-introspection"

require "cv/version"

module CV
  class Loader < GObjectIntrospection::Loader
  end

  Loader.load("CV", self)

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
