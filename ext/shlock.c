#define _GNU_SOURCE
#include "ruby.h"
#include <sys/types.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

struct mutex_info {
  pthread_mutex_t m;
  int fd;
};

struct rwlock_info {
  pthread_rwlock_t rwlock;
  int fd;
};

static int shared_mem_open(const char *name) {
  int fd = shm_open (name, O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    fd = shm_open (name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (ftruncate(fd, sizeof(struct mutex_info)) == -1) {
      printf("ftruncate error: %s\n", strerror(errno));
      return -1;
    }
  }

  if (fd < 0) {
    printf("error: %s\n", strerror(errno));
    return -1;
  }

  return fd;
}

static VALUE rb_putex_new (VALUE self, VALUE name) {
  struct mutex_info *ms;
  int fd = shared_mem_open(StringValuePtr(name));
  if (fd < 0)
    return Qnil;
  
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

  return Qnil;
}

static VALUE rb_putex_lock (VALUE self) {
  struct mutex_info *fs;
  Data_Get_Struct(self, struct mutex_info, fs);
  pthread_mutex_lock(&(fs->m));

  return Qnil;
}

static VALUE rb_putex_unlock (VALUE self) {
  struct mutex_info *fs;
  Data_Get_Struct(self, struct mutex_info, fs);
  pthread_mutex_unlock(&(fs->m));

  return Qnil;
}

static VALUE rb_rwlock_new (VALUE self, VALUE name) {
  struct rwlock_info *rws;
  int fd = shared_mem_open (StringValuePtr(name));
  if (fd < 0)
    return Qnil;

  rws = (struct rwlock_info *)mmap(NULL, sizeof(struct rwlock_info), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (rws == MAP_FAILED) {
    printf("mmap error: %s\n", strerror(errno));
    return Qnil;
  }
  rws->fd = fd;
  fsync(fd);

  VALUE obj = Data_Wrap_Struct(self, NULL, NULL, rws);
  return obj;
}

static VALUE rb_rwlock_destory (VALUE self) {
  struct rwlock_info *rws;
  Data_Get_Struct(self, struct rwlock_info, rws);
  fsync(rws->fd);
  close(rws->fd);

  return Qnil;
}

static VALUE rb_rwlock_read_lock (VALUE self) {
  struct rwlock_info *rws;
  Data_Get_Struct(self, struct rwlock_info, rws);
  pthread_rwlock_rdlock(&(rws->rwlock));

  return Qnil;
}

static VALUE rb_rwlock_write_lock (VALUE self) {
  struct rwlock_info *rws;
  Data_Get_Struct (self, struct rwlock_info, rws);
  pthread_rwlock_wrlock(&(rws->rwlock));

  return Qnil;
}

static VALUE rb_rwlock_unlock (VALUE self) {
  struct rwlock_info *rws;
  Data_Get_Struct (self, struct rwlock_info, rws);
  int e = pthread_rwlock_unlock(&(rws->rwlock));

  if (e != 0) {
    printf("error: %d\n", e);
  }
  return Qnil;
}

void Init_shlock() {
  VALUE shared_lock_module = rb_define_module("Shlock");

  /* Shlock::Putex */

  VALUE putex_class = rb_define_class_under(shared_lock_module, "Putex", rb_cObject);
  rb_define_singleton_method(putex_class, "new", rb_putex_new, 1);
  rb_define_method(putex_class, "lock", rb_putex_lock, 0);
  rb_define_method(putex_class, "unlock", rb_putex_unlock, 0);
  rb_define_method(putex_class, "destroy", rb_putex_destroy, 0);

  /* Shlock::Prwlock */

  VALUE prwlock_class = rb_define_class_under(shared_lock_module, "Prwlock", rb_cObject);
  rb_define_singleton_method(prwlock_class, "new", rb_rwlock_new, 1);
  rb_define_method(prwlock_class, "read_lock", rb_rwlock_read_lock, 0);
  rb_define_method(prwlock_class, "write_lock", rb_rwlock_write_lock, 0);
  rb_define_method(prwlock_class, "unlock", rb_rwlock_unlock, 0);
}
