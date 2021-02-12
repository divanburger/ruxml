# frozen_string_literal: true

require "bundler/gem_tasks"
require "rspec/core/rake_task"
require 'rake/extensiontask'

desc "ruxml test suite"
RSpec::Core::RakeTask.new(:spec) do |t|
  t.pattern = "spec/*_spec.rb"
  t.verbose = true
end

gemspec = Gem::Specification.load('ruxml.gemspec')
Rake::ExtensionTask.new do |ext|
  ext.name = 'ruxml'
  ext.source_pattern = "*.{cpp,hpp}"
  ext.ext_dir = 'ext/ruxml'
  ext.lib_dir = 'lib/ruxml'
  ext.gem_spec = gemspec
end

task :default => [:compile, :spec]