#include "../include/ScopedAllocationDetector.h"
#include <cassert>

namespace ntlab
{
    ScopedAllocationDetector::ScopedAllocationDetector (OperationsToCatch operationsToCatch, AllocationCallback allocationCallback, AllocationCallback freeCallback)
    {
        count++;
        if (count == 1)
            activateDetection();

        // Increase the maxNumDetectors value if you really hit this limit
        assert (count < maxNumDetectors);

        for (auto& d : activeDetectors)
        {
            if (d.detector == nullptr)
            {
                d.detector = this;
                d.threadId = getCurrentThreadID();
                break;
            }
        }

        if (allocationCallback)
            onAllocation = allocationCallback;

        if (freeCallback)
            onFree = freeCallback;

        this->operationsToCatch = operationsToCatch;
    }

    ScopedAllocationDetector::~ScopedAllocationDetector()
    {
        count--;
        if (count == 0)
            endDetection();

        for (auto& d : activeDetectors)
        {
            if (d.detector == this)
            {
                d.detector = nullptr;
                break;
            }
        }
    }

    ScopedAllocationDetector::AllocationCallback ScopedAllocationDetector::defaultAllocationCallback = [] (size_t bytesAllocated, const std::string* optionalFileAndLine)
    {
		std::cerr << "Detected allocation of " << bytesAllocated << " bytes " << ((optionalFileAndLine == nullptr) ? "" : *optionalFileAndLine) << std::endl;
    };

    ScopedAllocationDetector::AllocationCallback ScopedAllocationDetector::defaultFreeCallback = [] (size_t bytesFreed, const std::string* optionalFileAndLine)
    {
        std::cerr << "Detected freeing of " << bytesFreed << " bytes " << ((optionalFileAndLine == nullptr) ? "" : *optionalFileAndLine) << std::endl;
    };

    std::atomic<int> ScopedAllocationDetector::count (0);
    std::array<ScopedAllocationDetector::DetectorProperties, ScopedAllocationDetector::maxNumDetectors> ScopedAllocationDetector::activeDetectors {};

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
			auto currentThreadID = getCurrentThreadID();

            for (auto& d : activeDetectors)
            {
			     if (d.threadId == currentThreadID)
			     {
			         _CrtSetAllocHook (NULL);

			        if (filename != nullptr)
			        {
				        std::string location = "from " + std::string ((const char*)filename) + " line " + std::to_string(lineNumber);
				        d.detector->onAllocation (size, &location);
			        }
			        else
				        d.detector->onAllocation (size, nullptr);

			        _CrtSetAllocHook (windowsAllocHook);
			     }
            }
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
        auto currentThreadID = getCurrentThreadID();

        for (auto& d : activeDetectors)
        {
            if (pthread_equal (d.threadId, currentThreadID))
            {
                auto* defaultZone = malloc_default_zone();

                defaultZone->malloc = macSystemMalloc;
                d.detector->onAllocation (size, nullptr);
                defaultZone->malloc = detectingMalloc;

                break;
            }
        }

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

        auto currentThreadID = getCurrentThreadID();

        for (auto& d : activeDetectors)
        {
            if (pthread_equal (d.threadId, currentThreadID))
            {
                d.detector->onAllocation (size, nullptr);
            }
        }

        void* ptr = malloc (size);

        __malloc_hook = detectingMalloc;

        return ptr;
    }

#endif

#ifdef WIN32
    ScopedAllocationDetector::ThreadID ScopedAllocationDetector::getCurrentThreadID() { return GetCurrentThreadId(); }
#else // Posix
    ScopedAllocationDetector::ThreadID ScopedAllocationDetector::getCurrentThreadID() { return pthread_self(); }
#endif
}