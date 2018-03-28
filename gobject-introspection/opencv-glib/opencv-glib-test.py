import gi
gi.require_version("CV", "1.0")
from gi.repository import CV

matrix = CV.Matrix.new()
print(matrix)
