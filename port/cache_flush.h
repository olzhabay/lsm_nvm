#ifndef CACHE_FLUSH_H
#define CACHE_FLUSH_H

#include <stdio.h>
#include <stdlib.h>

//#define _ENABLE_PMEMIO

#ifdef _ENABLE_PMEMIO
#include <libpmem.h>
#endif

static uint64_t WRITE_LATENCY_IN_NS = 500;

//Cacheline size
//TODO: Make it configurable
#define CPU_FREQ_MHZ (2400) // cat /proc/cpuinfo
#define CAS(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
#define CACHE_LINE_SIZE 64
#define ASMFLUSH(dest) __asm__ __volatile__ ("clflush %0" : : "m"(*(volatile char *)dest))

static inline void clflush(volatile char* __p)
{
    asm volatile("clflush %0" : "+m" (*__p));
}

static inline void mfence()
{
    asm volatile("mfence":::"memory");
    return;
}

static inline void cpu_pause() {
  __asm__ volatile ("pause" ::: "memory");
}

static inline unsigned long read_tsc() {
  unsigned long var;
  unsigned int hi, lo;
  asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
  var = ((unsigned long long int) hi << 32) | lo;
  return var;
}

static inline void flush_cache(void *data, size_t size){

#ifdef _ENABLE_PMEMIO
  pmem_persist((const void*)ptr, size);
#else
  if (data == nullptr) return;
  volatile char *ptr = (char *)((unsigned long)data &~(CACHE_LINE_SIZE-1));
  mfence();
  for (; ptr< const_cast<volatile void*>(data+size); ptr+=CACHE_LINE_SIZE) {
    unsigned long etsc = read_tsc() + (unsigned long)(WRITE_LATENCY_IN_NS*CPU_FREQ_MHZ/1000);
    asm volatile("clflush %0" : "+m" (*(volatile char *)ptr));
    while (read_tsc() < etsc)
      cpu_pause();
  }
  mfence();
#endif
}

static inline void memcpy_persist
                    (void *dest, const void *src, size_t size){

#ifdef _ENABLE_PMEMIO
  pmem_memcpy_persist(dest, (const void *)src, size);
#else
  memcpy(dest, src, size);

  flush_cache(dest, size);
#endif

}
#endif
