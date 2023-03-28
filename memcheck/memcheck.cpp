#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <map>

#include <malloc.h>
#include <signal.h>

#include "memcheck.h"

#define MEMCHECK_LOG ("/tmp/memcheck.log")
#define dlog(fmt, ...) fprintf(s_memcheck_log_fp, fmt, ##__VA_ARGS__)
#define dmsg(fmt, ...) fprintf(stderr, "[%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

int memcheck_stats(void);
int memcheck_cul(void);

static FILE *s_memcheck_log_fp = nullptr;
static std::map<void *, std::pair<const void *, size_t>> s_mem_usage_map;
static std::map<const void *, size_t> s_call_usage_map;

extern void *__libc_malloc (size_t bytes);
static int enable_malloc_hook = 1;
extern void __libc_free (void *p);
static int enable_free_hook = 1;


typedef void *(*malloc_hook_t)(size_t size, const void *caller);
typedef void (*free_hook_t)(void *ptr, const void *caller);
malloc_hook_t old_malloc_f = NULL;
free_hook_t old_free_f = NULL;
void mem_untrace();
void mem_trace();

void *malloc_hook_f(size_t size, const void *caller) {
	mem_untrace();
	void *ptr = malloc(size);
	s_mem_usage_map[ptr] = std::pair<const void *, size_t>(caller, size);
	mem_trace();
	return ptr;
}

void free_hook_f(void *ptr, const void *caller) 
{
	mem_untrace();
	auto iter = s_mem_usage_map.find(ptr);
    if (iter != s_mem_usage_map.end())
		s_mem_usage_map.erase(ptr);
	free(ptr);
	mem_trace();
}

int replaced = 0;

void mem_trace(void) 
{
	replaced = 1;
	old_malloc_f = __malloc_hook;
	old_free_f = __free_hook;
	__malloc_hook = malloc_hook_f;
	__free_hook = free_hook_f;
}

void mem_untrace(void) 
{
	__malloc_hook = old_malloc_f;
	__free_hook = old_free_f;
	replaced = 0;
}

static void memcheck_sig_hook(int sig)
{
    switch (sig) {
        case SIGUSR1:
            memcheck_cul();
            break;
        case SIGUSR2:
            memcheck_stats();
            break;
        default:
            break;
    }
}

int memcheck_stats(void)
{
    for (const auto iter: s_call_usage_map) {
        dlog("[%p] %zu\n", iter.first, iter.second);
    }
    return 0;
}

int memcheck_cul(void)
{
    s_call_usage_map.clear();
    for (const auto iter: s_mem_usage_map) {
        auto call = s_call_usage_map.find(iter.second.first);
        if (call != s_call_usage_map.end()) {
            s_call_usage_map[iter.second.first] += iter.second.second;
        } else {
            s_call_usage_map[iter.second.first] = iter.second.second;
        }
    }
    return 0;
}

void memcheck_exit(void)
{
	mem_untrace();

    if (s_memcheck_log_fp) {
        fclose(s_memcheck_log_fp);
        s_memcheck_log_fp = nullptr;
    }
}

int memcheck_init(void)
{
    signal(SIGUSR1, memcheck_sig_hook);
    signal(SIGUSR2, memcheck_sig_hook);
    
    s_memcheck_log_fp = fopen(MEMCHECK_LOG, "w+");
    if (!s_memcheck_log_fp) {
        dmsg("fail to fopen %s", MEMCHECK_LOG);
        return -1;
    }



	mem_trace();

	return 0;
}

#ifdef __XTEST__

void test_for(void)
{
    for (size_t i = 0; i < 100; ++i) {
        malloc(i);
    }
}

void test(void)
{
    malloc(1000);
}

int main(int argc, char *argv[])
{
    memcheck_init();
    char *p = (char *)malloc(100);
    if (!p) {
        dmsg("fail to malloc\n");
        return -1;
    }
    test();
    test_for();

    free(p);
    dmsg("free p done\n");

    memcheck_cul();
    memcheck_stats();

    memcheck_exit();


    return 0;
}

#endif//__XTEST__
