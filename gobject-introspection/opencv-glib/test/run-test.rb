#!/usr/bin/env ruby

require "test-unit"

test_dir = __dir__

require "gi"
CV = GI.load("CV")

exit(Test::Unit::AutoRunner.run(true, test_dir.to_s))
