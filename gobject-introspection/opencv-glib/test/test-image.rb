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
