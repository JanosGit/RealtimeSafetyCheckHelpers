#pragma once


#ifdef WIN32
    #include <crtdbg.h>
#elif defined(__APPLE__) || defined(__MACOSX)
    #include <malloc/malloc.h>
#else // Linux
    #include <malloc.h>
#endif

#include <iostream>
#include <functional>
#include <atomic>
#include <string>

namespace ntlab
{
    /**
     * A class to catch any allocation while there is an instance of this class on the stack. Note that allocations on
     * other threads will be detected too while this is active. Also note that if multiple objects are existing at the
     * same time, detection will take place as long as the last object has gone out of scope.
     *
     * Use it like this
     *
     * \code
       struct SomeObj
       {
           int a, b, c;
       }

       void myFunc()
       {
           someUncriticalCalls();

           // the start of the section you want to examine
           {
               ntlab::ScopedAllocationDetection allocationDetection;

               // this will trigger the detection
               std::uniqe_ptr<SomeObj> someObj (new SomeObj);

               // perform some library call to see if it allocates under the hood
               callToSomeClosedSourceLibAPI();
           }

           // this won't trigger the detection
           std::uniqe_ptr<SomeObj> someOtherObj (new SomeObj);
       }
     */
    class ScopedAllocationDetector
    {
    public:
        ScopedAllocationDetector();

        ~ScopedAllocationDetector();

        /**
         * A callback invoked if an allocation took place. Per default, this prints an information on the number of
         * bytes allocated to stderr. On Windows an additional string containing the file and line number of the
         * function that called malloc is passed to the callback. On non-windows systems this will always be a nullptr.
         */
        static std::function<void (size_t numBytesAllocated, const std::string* location)> onAllocation;

    private:
        static std::atomic<int> count;

#ifdef WIN32

        static int windowsAllocHook (int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber);

#elif defined(__APPLE__) || defined(__MACOSX)

        typedef void* (*MacSystemMalloc) (malloc_zone_t*, size_t);

        static MacSystemMalloc macSystemMalloc;

        static void* detectingMalloc (malloc_zone_t *zone, size_t size);

#else // Linux
        typedef void* (*LinuxMallocHook) (size_t, const void*);

        static LinuxMallocHook originalMallocHook;

        static void* detectingMalloc (size_t size, const void* caller);

#endif
        static void activateDetection();

        static void endDetection();
    };
}