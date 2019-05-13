#pragma once


#ifdef WIN32

#elif defined(__APPLE__) || defined(__MACOSX)
    #include <malloc/malloc.h>
#else // Linux

#endif

#include <iostream>

namespace ntlab
{
    class ScopedAllocationDetector
    {
    public:
        ScopedAllocationDetector();

        ~ScopedAllocationDetector();

        static std::function<void (size_t)> onAllocation;

    private:
        static std::atomic<int> count;

#ifdef WIN32
        
        //???

#elif defined(__APPLE__) || defined(__MACOSX)

        typedef void* (*MacSystemMalloc) (malloc_zone_t*, size_t);

        static MacSystemMalloc macSystemMalloc;

        static void activateDetection();

        static void endDetection();

        static void* detectingMalloc (malloc_zone_t *zone, size_t size);
        
#else // Linux

        //???
        
#endif
    };
}