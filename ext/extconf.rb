require 'mkmf'
if ( have_library('c', 'shm_open') or
   have_library('rt', 'shm_open') )
   create_makefile ('putex')
end
