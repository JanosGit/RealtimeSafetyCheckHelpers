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

    std::function<void (size_t, const std::string*)> ScopedAllocationDetector::onAllocation = [] (size_t bytesAllocated, const std::string* location)
    {
		std::cerr << "Detected allocation of " << bytesAllocated << " bytes " << ((location == nullptr) ? "" : *location) << std::endl;
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
			_CrtSetAllocHook (NULL);
			
			if (filename != nullptr)
			{
				std::string location = "from " + std::string ((const char*)filename) + " line " + std::to_string(lineNumber);
				onAllocation (size, &location);
			}
			else
				onAllocation (size, nullptr);

			_CrtSetAllocHook (windowsAllocHook);
        }

        return 1;
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
        auto* defaultZone = malloc_default_zone();

        defaultZone->malloc = macSystemMalloc;
        onAllocation (size, nullptr);
        defaultZone->malloc = detectingMalloc;

        return macSystemMalloc (zone, size);
    }

#else // Linux

    ScopedAllocationDetector::LinuxMallocHook ScopedAllocationDetector::originalMallocHook;

    void ScopedAllocationDetector::activateDetection()
    {
        originalMallocHook = __malloc_hook;
        __malloc_hook = detectingMalloc;
    }

    void ScopedAllocationDetector::endDetection()
    {
        __malloc_hook = originalMallocHook;
    }

    void* ScopedAllocationDetector::detectingMalloc (size_t size, const void* caller)
    {
        __malloc_hook = originalMallocHook;

        onAllocation (size, nullptr);

        void* ptr = malloc (size);

        __malloc_hook = detectingMalloc;

        return ptr;
    }

#endif
}