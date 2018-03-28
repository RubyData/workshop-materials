require "pathname"

$LOAD_PATH.unshift(File.join(__dir__, "lib"))
require "cv"

image = CV::Image.new(Pathname("test.png"))
