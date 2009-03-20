spec = Gem::Specification.new do |s|
  s.name = 'putex'
  s.version = '0.0.1'
  s.date = '2008-09-01'
  s.summary = 'Inter-process mutex using shared memory'
  s.email = "ice799@gmail.com"
  s.homepage = "http://github.com/ice799/putex/tree/master"
  s.description = "inter-process mutex using shared memory"
  s.has_rdoc = false
  s.authors = ["Joe Damato"]
  s.extensions = "ext/extconf.rb"
  s.require_paths << "ext"
  s.files = ["README",
             "putex.gemspec",
             "ext/putex.c",
             "ext/extconf.rb"]
end
