#include "../include/ScopedAllocationDetector.h"

namespace ntlab
{
    ScopedAllocationDetector::ScopedAllocationDetector()
    {
        count++;
        if (count == 1)
            activateDetection();
    }

    ScopedAllocationDetector::~ScopedAllocationDetector()
    {
        count--;
        if (count == 0)
            endDetection();
    }

    std::function<void (size_t, std::string*)> ScopedAllocationDetector::onAllocation = [] (size_t bytesAllocated, std::string* location)
    {
        std::cerr << "Detected allocation of " << bytesAllocated << " bytes" << ((location == nullptr) ? "" : *location) << std::endl;
    };

    std::atomic<int> ScopedAllocationDetector::count = 0;

#ifdef WIN32

    void ScopedAllocationDetector::activateDetection()
    {
        _CrtSetAllocHook (windowsAllocHook);
    }

    void ScopedAllocationDetector::endDetection()
    {
        _CrtSetAllocHook (NULL);
    }

    int ScopedAllocationDetector::windowsAllocHook (int allocType, void*, size_t size, int, long, const unsigned char* filename, int lineNumber)
    {
        if (allocType == _HOOK_ALLOC)
        {
            std::string location = filename + " line " + std::to_string (lineNumber);
            onAllocation (size, &location);
        }

        return TRUE;
    }

#elif defined(__APPLE__) || defined(__MACOSX)

    ScopedAllocationDetector::MacSystemMalloc ScopedAllocationDetector::macSystemMalloc;

    void ScopedAllocationDetector::activateDetection()
    {
        auto* zone = malloc_default_zone();
        macSystemMalloc = zone->malloc;
        zone->malloc = detectingMalloc;
    }

    void ScopedAllocationDetector::endDetection()
    {
        auto* zone = malloc_default_zone();
        zone->malloc = macSystemMalloc;
    }

    void* ScopedAllocationDetector::detectingMalloc (malloc_zone_t *zone, size_t size)
    {
        onAllocation (size, nullptr);
        return macSystemMalloc (zone, size);
    }

#else // Linux

    ScopedAllocationDetector::LinuxMallocHook ScopedAllocationDetector::linuxSystemMalloc;

    void ScopedAllocationDetector::activateDetection()
    {
        linuxSystemMalloc = __malloc_hook;
        __malloc_hook = detectingMalloc;
    }

    void ScopedAllocationDetector::endDetection()
    {
        __malloc_hook = linuxSystemMalloc;
    }

    void* ScopedAllocationDetector::detectingMalloc (size_t size)
    {
        onAllocation (size, nullptr);
        return linuxSystemMalloc (+size);
    }

#endif
}