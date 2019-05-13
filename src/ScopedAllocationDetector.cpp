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

    std::function<void (size_t)> ScopedAllocationDetector::onAllocation = [] (size_t bytesAllocated)
    {
        std::cerr << "Detected allocation of " << bytesAllocated << " bytes" << std::endl;
    };

    std::atomic<int> ScopedAllocationDetector::count = 0;

#ifdef _WIN32

    //???

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

    void* ScopedAllocationDetector::detectingMalloc(malloc_zone_t *zone, size_t size)
    {
        onAllocation (size);
        return macSystemMalloc (zone, size);
    }

#else // Linux

    //???

#endif
}