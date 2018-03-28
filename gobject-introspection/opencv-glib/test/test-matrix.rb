class MatrixText < Test::Unit::TestCase
  test(".new") do
    matrix = CV::Matrix.new
    assert do
      matrix.empty?
    end
  end
end
