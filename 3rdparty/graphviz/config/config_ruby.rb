require 'rbconfig'

CONFIG = RbConfig::MAKEFILE_CONFIG

puts RbConfig::expand(CONFIG[ARGV[0]])
