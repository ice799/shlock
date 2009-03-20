spec = Gem::Specification.new do |s|
  s.name = 'shlock'
  s.version = '0.0.2'
  s.date = '2009-03-20'
  s.summary = 'Inter-process synchronization primitives using shared memory'
  s.email = "ice799@gmail.com"
  s.homepage = "http://github.com/ice799/putex/tree/master"
  s.description = "inter-process synchronization primitives using shared memory"
  s.has_rdoc = false
  s.authors = ["Joe Damato"]
  s.extensions = "ext/extconf.rb"
  s.require_paths << "ext"
  s.files = ["README",
             "shlock.gemspec",
             "ext/shlock.c",
             "ext/extconf.rb"]
end
