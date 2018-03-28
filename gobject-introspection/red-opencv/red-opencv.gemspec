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
