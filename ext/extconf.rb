require 'mkmf'
if ( (have_library('c', 'shm_open') or
      have_library('rt', 'shm_open')) and
     (have_library('pthread', 'pthread_mutex_lock')) and
     (have_header('pthread.h')))
   create_makefile ('shlock')
end
