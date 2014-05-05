/*
    TUF - track used files by LD_PRELOADING wrappers to
          standard file access functions (open,exec)

    tuf.c - source for libtuf.so
*/

#ifdef linux
/* RTLD_NEXT needs those under linux */
#define _GNU_SOURCE
#define __USE_GNU       // for RTLD_NEXT in dlfcn.h
#endif
#include <dlfcn.h>  	// for ::dlsym
#include <stdio.h>      // for fprintf, stderr
#include <stdlib.h>     // for abort
#include <unistd.h>     // for getpid
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>    // for va_list, va_start,...
#include <assert.h>

static void* load_sym(const char* symbol_name)
{
    void* result = dlsym(RTLD_NEXT, symbol_name);
    if( !result ) {
        fprintf(stderr, "libtuf[%i]: unable to load %s, aborting!\n", getpid(), symbol_name);
        abort();
    }
    return result;
}

static void tuf_abort(const char* message)
{
    fprintf(stderr, "libtuf[%i]: %s, aborting!\n", getpid(), message);
    abort();
}

static void tuf_abort_errno(const char* message)
{
    fprintf(stderr, "libtuf[%i]: %s: %s(%i), aborting!\n", getpid(), message, strerror(errno), errno);
    abort();
}
static int tuf_lock = 0;
static int tuf_fd = -1;

static void tuf_event(const char* event, const char* path)
{
    if( tuf_lock ) 
        return;
    tuf_lock = 1;
    if( tuf_fd == -1 ) {
        const char* tuf_file_name = getenv("TUF_FILE");
        if( !tuf_file_name ) 
            tuf_abort("$TUF_FILE not defined");
        tuf_fd = open(tuf_file_name, (O_WRONLY | O_CREAT | O_APPEND), (S_IRUSR | S_IWUSR));
        if( tuf_fd < 0)
            tuf_abort("unable to open TUF audit file");
    }
    {
        char buf[1024];
        if (path[0] != '/' ) {
            // relative
#ifdef linux
            const char* cwd = get_current_dir_name();
#else
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            cwd[sizeof(cwd)-1] = 0;
#endif
            snprintf(buf, 1024, "%s %s/%s\n", event, cwd, path);
        } else {
            snprintf(buf, 1024, "%s %s\n", event, path);
        }
        buf[1023] = 0;
        write(tuf_fd, buf, strlen(buf));
    }
    tuf_lock = 0;
}

//
//
//
static int  (*real_open)(const char *pathname, int flags, mode_t mode) = 0;
static int  (*real_creat)(const char *pathname, mode_t mode) = 0;
static DIR* (*real_opendir)(const char* dirname) = 0;
static int  (*real_execve)(const char *filename, char *const argv[], char *const envp[]) = 0;
static int  (*real_execl)(const char *path, const char *arg, ...) = 0;
static int  (*real_execlp)(const char *file, const char *arg, ...) = 0;
static int  (*real_execle)(const char *path, const char *arg, ... /*, char * const envp[]*/) = 0;
static int  (*real_execv)(const char *path, char *const argv[]) = 0;
static int  (*real_execvp)(const char *file, char *const argv[]) = 0;
static int  (*real_execvpe)(const char *file, char *const argv[], char *const envp[]) = 0;

int vopen(const char *pathname, int flags, int mode)
{
    if( !real_open ) {
        real_open = load_sym("open");
    }
    int result = real_open(pathname, flags, mode);
    if( result != -1 ) {
        if ( flags & (O_CREAT) != 0 ||
             flags & (O_WRONLY) != 0 ||
             flags & (O_RDWR) != 0 )
        {
            tuf_event("creat", pathname);
        } else {
            tuf_event("open", pathname);
        }
    }
    return result;
}
int open(const char *pathname, int flags, ...)
{
    va_list ap;
    va_start(ap, flags);
    const int mode = va_arg(ap, int);
    va_end(ap);
    return vopen(pathname, flags, mode);
}

#ifdef linux
int open64(const char *pathname, int flags, ...)
{
    va_list ap;
    va_start(ap, flags);
    const int mode = va_arg(ap, int);
    va_end(ap);
    return vopen(pathname, flags, mode);
}
#endif
int creat(const char *pathname, mode_t mode)
{
    if( !real_creat ) {
        real_creat = load_sym("creat");
    }
    int result = real_creat(pathname, mode);
    if( result != -1 ) {
        tuf_event("creat", pathname);
    }
    return result;
}

#ifdef linux
int creat64(const char *pathname, mode_t mode)
{
    return creat(pathname, mode);
}
#endif

DIR * opendir(const char* dirname)
{
    if( !real_opendir ) {
        real_opendir = load_sym("opendir");
    }
    DIR* result = real_opendir(dirname);
    if( result != 0 ) {
        tuf_event("open", dirname);
    }
    return result;
}


typedef struct ptr_array {
    char const** begin;
    char const** end;
    char const** end_allocated;
} ptr_array;

void ptr_array_init(ptr_array* arr)
{
    arr->begin = 0;
    arr->end = 0;
    arr->end_allocated = 0;
}

int ptr_array_left(ptr_array* arr)
{
    return arr->end < arr->end_allocated;
}
void ptr_array_add(ptr_array* arr, const char* arg)
{
    if( arr->end >= arr->end_allocated ) {
        const int ITEM_SIZE = sizeof(const char*);
        const size_t current_capacity = (arr->end_allocated - arr->begin);
        const size_t new_capacity = current_capacity ? current_capacity*2 : 10;
        const char** new_begin = malloc(new_capacity*ITEM_SIZE);
        const size_t current_allocated = (arr->end - arr->begin);

        assert(new_capacity > current_capacity);

        memcpy(new_begin, arr->begin, current_allocated*ITEM_SIZE);
        arr->begin = new_begin;
        arr->end   = new_begin + current_allocated;
        arr->end_allocated = new_begin + new_capacity;
    }
    assert(arr->end < arr->end_allocated);

    *(arr->end++) = arg;
}

int execve(const char *filename, char *const argv[],
                  char *const envp[])
{
    if( !real_execve ) {
        real_execve = load_sym("execve");
    }

    if( access(filename, X_OK) == 0) {
        tuf_event("exec", filename);
    }
    return real_execve(filename, argv, envp);
}

int execl(const char *path, const char *arg, ...)
{
    va_list ap;
    ptr_array arr;
    ptr_array_init(&arr);
    va_start(ap, arg);
    const char* cur_arg = arg;
    while( cur_arg ) {
        ptr_array_add(&arr, cur_arg);
        cur_arg = va_arg(ap, const char*);
    }
    va_end(ap);
    ptr_array_add(&arr, 0);

    // execl is same as execv 
    return execv(path, arr.begin);
}
int execlp(const char *file, const char *arg, ...)
{
    // PATH is searched
    va_list ap;
    struct ptr_array arr;
    ptr_array_init(&arr);
    va_start(ap, arg);
    const char* cur_arg = arg;
    while( cur_arg ) {
        ptr_array_add(&arr, cur_arg);
        cur_arg = va_arg(ap, const char*);
    }
    va_end(ap);
    ptr_array_add(&arr, 0);

    // execlp is same as execvp
    return execvp(file, arr.begin);
}
int execle(const char *path, const char *arg, ... /*, char * const envp[]*/)
{
    tuf_abort("execle not implemented");
    return -1;
}

int execv(const char *path, char *const argv[])
{
    if( !real_execv ) {
        real_execv = load_sym("execv");
    }

    if( access(path, X_OK) == 0) {
        tuf_event("exec", path);
    }
    return real_execv(path, argv);
}
int execvp(const char *file, char *const argv[])
{
    // PATH is searched
    if( !real_execvp ) {
        real_execvp = load_sym("execvp");
    }

    if( access(file, X_OK) == 0) {
        tuf_event("exec", file);
    }
    return real_execvp(file, argv);
}

int execvpe(const char *file, char *const argv[],
           char *const envp[])
{
    // PATH is searched
    if( !real_execvpe ) {
        real_execvpe = load_sym("execvpe");
    }
    if( access(file, X_OK) == 0) {
        tuf_event("exec", file);
    }
    return real_execvpe(file, argv, envp);
}

/*
 * ANSI C API
 * stdio 
 */
 
static FILE*  (*real_fopen)(const char *filename, const char* modes) = 0;
FILE* fopen(const char* filename,
            const char* modes)
{
    if( !real_fopen ) {
        real_fopen = load_sym("fopen");
    }
    if( modes != 0 && 
        (strchr(modes, 'w') != 0 ) ||
        (strchr(modes, 'a') != 0 )) 
    {
        tuf_event("creat", filename);
    } else {
        tuf_event("open", filename);
    }
    return real_fopen(filename, modes);
}

#ifdef linux
FILE* fopen64(const char * filename,
             const char *modes)
{
    return fopen(filename, modes);
}
#endif

