#define _GNU_SOURCE
#include "ruby.h"
#include <sys/types.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <semaphore.h>

struct mutex_info {
  pthread_mutex_t m;
};

struct rwlock_info {
  pthread_rwlock_t rwlock;
};

struct sem_info {
  sem_t sem;
};

/**
 * @return ret 0 if the shared memory already existed, 1 if it had to be created.
 */

static int shared_mem_open(const char *name, int *fd) {
  int ret = 0;
  int mfd = shm_open (name, O_RDWR, S_IRUSR | S_IWUSR);
  if (mfd < 0) {
    mfd = shm_open (name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (mfd < 0) {
      fprintf(stderr, "shm_open error: %s\n", strerror(errno));
      return -1;
    }
    ret = 1;
    if (ftruncate(mfd, sizeof(struct mutex_info)) == -1) {
      fprintf(stderr,"ftruncate error: %s\n", strerror(errno));
      return -1;
    }
  }

  *fd = mfd;
  return ret;
}

static VALUE rb_putex_new (VALUE self, VALUE name) {
  struct mutex_info *ms;
  int mfd = 0;
  int ret = shared_mem_open(StringValuePtr(name), &mfd);
  if (mfd < 0)
    return Qnil;
  
  ms = (struct mutex_info *)mmap(NULL, sizeof(struct mutex_info), PROT_READ|PROT_WRITE, MAP_SHARED, mfd, 0);
  close(mfd);

  if (ms == MAP_FAILED) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
    return Qnil;
  }

  if (ret) {
    /* mutex was just created so we need to set some attributes and initalize the mutex */
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(ms->m), &mattr);
  }

  VALUE obj = Data_Wrap_Struct(self, NULL, NULL, ms);
  return obj;
}

static VALUE rb_putex_destroy (VALUE self) {
  struct mutex_info *m;
  Data_Get_Struct(self, struct mutex_info, m);
  if (munmap(m, sizeof(struct mutex_info)) == -1) {
    fprintf(stderr, "munmap error: %s\n", strerror(errno));
  }

  return Qnil;
}

static VALUE rb_putex_lock (VALUE self) {
  struct mutex_info *fs;
  int ret = 0;
  Data_Get_Struct(self, struct mutex_info, fs);
  if ((ret = pthread_mutex_lock(&(fs->m))) == -1) {
    fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_putex_unlock (VALUE self) {
  struct mutex_info *fs;
  int ret = 0;
  Data_Get_Struct(self, struct mutex_info, fs);
  if ((ret = pthread_mutex_unlock(&(fs->m))) == -1) {
    fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_rwlock_new (VALUE self, VALUE name) {
  struct rwlock_info *rws;
  int mfd = 0;

  int ret = shared_mem_open (StringValuePtr(name), &mfd);
  if (ret == -1)
    return Qnil;

  rws = (struct rwlock_info *)mmap(NULL, sizeof(struct rwlock_info), PROT_READ|PROT_WRITE, MAP_SHARED, mfd, 0);
  close(mfd);

  if (rws == MAP_FAILED) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
    return Qnil;
  }

  if (ret) {
    /* rwlock was just created so we need to set some attributes and initalize */
    pthread_rwlockattr_t rwattr;
    pthread_rwlockattr_init(&rwattr);
    pthread_rwlockattr_setpshared(&rwattr, PTHREAD_PROCESS_SHARED);
    pthread_rwlock_init(&(rws->rwlock), &rwattr);
  }

  VALUE obj = Data_Wrap_Struct(self, NULL, NULL, rws);
  return obj;
}

static VALUE rb_rwlock_destory (VALUE self) {
  struct rwlock_info *rws;
  Data_Get_Struct(self, struct rwlock_info, rws);
  munmap(rws, sizeof(struct rwlock_info));
  return Qnil;
}

static VALUE rb_rwlock_read_lock (VALUE self) {
  struct rwlock_info *rws;
  int ret = 0;
  Data_Get_Struct(self, struct rwlock_info, rws);
  if ((ret = pthread_rwlock_rdlock(&(rws->rwlock))) == -1) {
    fprintf(stderr, "pthread_rwlock_rdlock: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_rwlock_write_lock (VALUE self) {
  struct rwlock_info *rws;
  int ret = 0;
  Data_Get_Struct (self, struct rwlock_info, rws);
  if ((ret = pthread_rwlock_wrlock(&(rws->rwlock))) == -1) {
    fprintf(stderr, "pthread_rwlock_wrlock: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_rwlock_unlock (VALUE self) {
  struct rwlock_info *rws;
  int ret = 0;
  Data_Get_Struct (self, struct rwlock_info, rws);
  if ((ret = pthread_rwlock_unlock(&(rws->rwlock))) == -1) {
    fprintf(stderr, "pthread_rwlock_unlock: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_rwlock_destroy (VALUE self) {
  struct rwlock_info *rws;
  int ret = 0;
  Data_Get_Struct (self, struct rwlock_info, rws);
  if ((ret = pthread_rwlock_destroy(&(rws->rwlock))) == -1) {
    fprintf(stderr, "pthread_rwlock_destroy: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_psem_new (VALUE self, VALUE name, VALUE val) {
  struct sem_info *si;
  int mfd = 0;

  int ret = shared_mem_open (StringValuePtr(name), &mfd);
  if (ret == -1)
    return Qnil;

  si = (struct sem_info *)mmap(NULL, sizeof(struct sem_info), PROT_READ|PROT_WRITE, MAP_SHARED, mfd, 0);
  close(mfd);

  if (si == MAP_FAILED) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
    return Qnil;
  }

  if (ret) {
    /* sem was just created so we need to set some attributes and initalize */
    sem_init(&(si->sem), 1, INT2NUM(val));
  }

  VALUE obj = Data_Wrap_Struct(self, NULL, NULL, si);
  return obj;
}

static VALUE rb_psem_lock (VALUE self) {
  struct sem_info *si;
  int ret = 0;
  Data_Get_Struct (self, struct sem_info, si);
  if ((ret = sem_wait(&(si->sem))) == -1) {
    fprintf(stderr, "sem_wait: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_psem_unlock (VALUE self) {
  struct sem_info *si;
  int ret = 0;
  Data_Get_Struct (self, struct sem_info, si);
  if ((ret = sem_post(&(si->sem))) == -1) {
    fprintf(stderr, "sem_post: %s\n", strerror(ret));
  }

  return Qnil;
}

static VALUE rb_psem_destroy (VALUE self) {
  struct sem_info *si;
  int ret = 0;
  Data_Get_Struct (self, struct sem_info, si);
  if ((ret = sem_destroy(&(si->sem))) == -1) {
    fprintf(stderr, "sem_destroy: %s\n", strerror(ret));
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
  rb_define_method(prwlock_class, "destroy", rb_rwlock_destroy, 0);

  /* Shlock::Psem */
  VALUE psem_class = rb_define_class_under(shared_lock_module, "Psem", rb_cObject);
  rb_define_singleton(psem_class, "new", rb_psem_new, 2);
  rb_define_method(psem_class, "lock", rb_psem_lock, 0);
  rb_define_method(psem_class, "unlock", rb_psem_unlock, 0);
  rb_define_method(psem_class, "destroy", rb_psem_destroy, 0);
}
