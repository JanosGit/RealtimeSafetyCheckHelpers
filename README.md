# RealtimeSafetyCheckHelpers
Helper tools to find out if your DSP code or third party libraries you might use don't perform any realtime-critical system calls

## ScopedAllocationDetector
A convenient RAII-style class you can use to spot heap alloctaions going on when calling specific functions. Note that it is based on static functions and is not thread safe. This means, if an allocation detector is active in a certain scope, any allocation on any thread happening at the same time will be detected. Also note that the detection is deactivated for the time the callback is performed, this makes it safe to do prints and other system interaction for logging purposes that could allocate. However this means you shouldn't rely on the ability to also detect allocations on other threads as they might occur just at the moment a callback is active. So again, this is not thread safe. You should also note that if there are multiple instances active at the same time, detection will be active until the last instance goes out of scope.

This is how you use it:

```
void foo()
{
    someFunctionThatWontBeDetected();
    
    {
        ntlab::ScopedAllocationDetector allocationDetector;
        
        someFunctionThatWillBeDetectedIfItAllocates();
        
        // will obviously be detected as allocation
        std::vector<int> largeVec (100000);
    }
    
    // will not be detected
    std::vector<int> anotherLargeVec (100000);
}
```

As default, this will print a message like `Detected allocation of 112 bytes` to stderr. On Windows it will also print the sourcefile and line the allocation came from if available and then continues execution. If you want to do something other in case of allocation like custom logging or abortion of the program, you can override the callback like this:

```
ntlab::ScopedAllocationDetector::onAllocation = [](size_t size, const std::string* fileAndLine)
{
    logAnAllocation (size);
}
```

If there is no file and line available, a `nullptr` will be passed as second argument.