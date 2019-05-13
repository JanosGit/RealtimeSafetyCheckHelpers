#pragma once


#ifdef WIN32
    #include <crtdbg.h>
#elif defined(__APPLE__) || defined(__MACOSX)
    #include <malloc/malloc.h>
#else // Linux
    #include <malloc.h>
#endif

#include <iostream>

namespace ntlab
{
    class ScopedAllocationDetector
    {
    public:
        ScopedAllocationDetector();

        ~ScopedAllocationDetector();

        static std::function<void (size_t, std::string*)> onAllocation;

    private:
        static std::atomic<int> count;

#ifdef WIN32

        static int windowsAllocHook (int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber);

#elif defined(__APPLE__) || defined(__MACOSX)

        typedef void* (*MacSystemMalloc) (malloc_zone_t*, size_t);

        static MacSystemMalloc macSystemMalloc;

        static void* detectingMalloc (malloc_zone_t *zone, size_t size);

#else // Linux
        typedef void* (*LinuxMallocHook) (size_t);

        static LinuxMallocHook linuxSystemMalloc;

        static void detectingMalloc (size_t size);

#endif
        static void activateDetection();

        static void endDetection();
    };
}