require File.expand_path("../lib/ruxml/version", __FILE__)

source_files = Dir.glob("ext/ruxml/*.cpp") + Dir.glob("ext/ruxml/*.hpp")
ruby_files = Dir.glob("lib/**/*.rb")

Gem::Specification.new do |s|
  s.name = 'ruxml'
  s.version = RUXML::VERSION
  s.date = '2021-02-02'
  s.summary = 'Simple fast XML parser'
  s.authors = ['Divan Burger']
  s.email = ['divan.burger@gmail.com']
  s.licenses = ['MIT']
  s.homepage = 'https://www.github.com/divanburger/ruxml'
  s.extensions = ['ext/ruxml/extconf.rb']
  s.files = source_files + ruby_files
  s.require_paths = ['lib']

  # Required to run tests
  s.add_development_dependency "rspec", "~> 2.13.0"
  s.add_development_dependency "rake", "~> 1.9.1"
  s.add_development_dependency "rake-compiler", "~> 0.8.3"
end