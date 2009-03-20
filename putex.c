#include "ruby.h"
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

struct mutex_info {
  pthread_mutex_t m;
  int fd;
};

static void putex_lock (pthread_mutex_t *m) {
  pthread_mutex_lock(m);
}

static void putex_unlock (pthread_mutex_t *m) {
  pthread_mutex_unlock(m);
}


static VALUE rb_putex_new (VALUE self, VALUE name) {
  struct mutex_info *ms;
  int fd = shm_open (StringValuePtr(name), O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {  
    fd = shm_open (StringValuePtr(name), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (ftruncate(fd, sizeof(struct mutex_info)) == -1) {
      printf("ftruncate error: %s\n", strerror(errno));
      return Qnil;
    }
  }
  
  if (fd < 0) {
    printf("error: %s\n", strerror(errno));
    return Qnil;
  }
  
  ms = (struct mutex_info *)mmap(NULL, sizeof(struct mutex_info), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (ms == MAP_FAILED) {
    printf("mmap error: %s\n", strerror(errno));
    return Qnil;
  }
  ms->fd = fd;
  fsync(fd);
  VALUE obj = Data_Wrap_Struct(self, NULL, NULL, ms);
  return obj;
}

static VALUE rb_putex_destroy (VALUE self) {
  struct mutex_info *m;
  Data_Get_Struct(self, struct mutex_info, m);
  fsync(m->fd);
  close(m->fd);

  return INT2NUM(1);
}

static VALUE rb_putex_lock (VALUE self) {
  struct mutex_info *fs;
  Data_Get_Struct(self, struct mutex_info, fs);
  putex_lock(&(fs->m));

  return INT2NUM(1);
}

static VALUE rb_putex_unlock (VALUE self) {
  struct mutex_info *fs;
  Data_Get_Struct(self, struct mutex_info, fs);
  putex_unlock(&(fs->m));

  return INT2NUM(1);
}

void Init_putex() {
  VALUE putex_class = rb_define_class("Putex", rb_cObject);
  rb_define_singleton_method(putex_class, "new", rb_putex_new, 1);
  rb_define_method(putex_class, "lock", rb_putex_lock, 0);
  rb_define_method(putex_class, "unlock", rb_putex_unlock, 0);
  rb_define_method(putex_class, "destroy", rb_putex_destroy, 0);
}
