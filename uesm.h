// Copyright (c) [2024] [Jovan J. E. Odassius]
//
// License: MIT (See the LICENSE file in the root directory)
// Github: https://github.com/untyper/universal-events-and-shared-memory-functions

#ifndef UNIVERSAL_EVENTS_AND_SHARED_MEMORY_FUNCTIONS
#define UNIVERSAL_EVENTS_AND_SHARED_MEMORY_FUNCTIONS

#include <iostream>

#ifdef _WIN32
// Windows includes
#include <windows.h>
#else
// Linux includes
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#endif

// Define INFINITE for Linux
#ifndef _WIN32
  #ifndef INFINITE
  #define INFINITE ((unsigned int)-1)
  #endif
#endif

// Universal Event Functions
void* create_event(const char* name, bool initial_state = false)
{
  if (name == nullptr)
  {
    return nullptr;
  }

#ifdef _WIN32
  return CreateEventA(nullptr, FALSE, initial_state, name);
#else
  sem_t* sem = sem_open(name, O_CREAT, 0666, initial_state ? 1 : 0);

  if (sem == SEM_FAILED)
  {
    perror("sem_open");
    return nullptr;
  }

  return sem;
#endif
}

bool set_event(void* event)
{
  if (event == nullptr)
  {
    return false;
  }

#ifdef _WIN32
  return SetEvent(static_cast<HANDLE>(event));
#else
  sem_t* sem = static_cast<sem_t*>(event);
  return sem_post(sem) == 0;
#endif
}

bool wait_for_event(void* event, unsigned int timeout_ms)
{
  if (event == nullptr)
  {
    return false;
  }

#ifdef _WIN32
  DWORD result = WaitForSingleObject(static_cast<HANDLE>(event), timeout_ms);
  return result == WAIT_OBJECT_0;
#else
  sem_t* sem = static_cast<sem_t*>(event);

  if (timeout_ms == INFINITE)
  {
    return sem_wait(sem) == 0;
  }
  else
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000)
    {
      ts.tv_sec += 1;
      ts.tv_nsec -= 1000000000;
    }

    return sem_timedwait(sem, &ts) == 0;
  }
#endif
}

void close_event(void* event, const char* name = nullptr)
{
  if (event == nullptr)
  {
    return;
  }

#ifdef _WIN32
  CloseHandle(static_cast<HANDLE>(event));
#else
  sem_t* sem = static_cast<sem_t*>(event);
  sem_close(sem);

  if (name == nullptr)
  {
    // Name is nullptr, skipping sem_unlink
    return;
  }

  sem_unlink(name);
#endif
}

// Universal Shared Memory Functions
void* create_shared_memory(const char* name, size_t size)
{
  if (name == nullptr)
  {
    return nullptr;
  }

  if (size == 0)
  {
    return nullptr;
  }

#ifdef _WIN32
  return CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name);
#else
  int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

  if (shm_fd == -1)
  {
    perror("shm_open");
    return nullptr;
  }

  if (ftruncate(shm_fd, size) == -1)
  {
    perror("ftruncate");
    close(shm_fd);
    shm_unlink(name);
    return nullptr;
  }

  return reinterpret_cast<void*>(shm_fd);
#endif
}

void* map_shared_memory(void* shm, size_t size)
{
  if (shm == nullptr)
  {
    return nullptr;
  }

  if (size == 0)
  {
    return nullptr;
  }

#ifdef _WIN32
  void* ptr = MapViewOfFile(static_cast<HANDLE>(shm), FILE_MAP_ALL_ACCESS, 0, 0, size);

  if (ptr == nullptr)
  {
    CloseHandle(static_cast<HANDLE>(shm)); // Cleanup handle
  }

  return ptr;
#else
  int shm_fd = reinterpret_cast<int>(shm);
  void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

  if (ptr == MAP_FAILED)
  {
    perror("mmap");
    close(shm_fd); // Cleanup file descriptor
    return nullptr;
  }

  return ptr;
#endif
}

void close_shared_memory(void* shm, const char* name, void* mapped = nullptr, size_t size = 0)
{
  if (shm == nullptr)
  {
    return;
  }

#ifdef _WIN32
  if (mapped != nullptr)
  {
    UnmapViewOfFile(mapped);
  }

  CloseHandle(static_cast<HANDLE>(shm));
#else
  if (mapped != nullptr)
  {
    munmap(mapped, size);
  }

  close(reinterpret_cast<int>(shm));

  if (name == nullptr)
  {
    // Name is nullptr, skipping shm_unlink
    return;
  }

  shm_unlink(name);
#endif
}

#endif // UNIVERSAL_EVENTS_AND_SHARED_MEMORY_FUNCTIONS
