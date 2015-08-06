// bdlqq_muteximpl_win32.h                                            -*-C++-*-
#ifndef INCLUDED_BDLQQ_MUTEXIMPL_WIN32
#define INCLUDED_BDLQQ_MUTEXIMPL_WIN32

#ifndef INCLUDED_BSLS_IDENT
#include <bsls_ident.h>
#endif
BSLS_IDENT("$Id: $")

//@PURPOSE: Provide a win32 implementation of 'bdlqq::Mutex'.
//
//@CLASSES:
//  bdlqq::MutexImpl<Platform::Win32Threads>: win32 specialization
//
//@SEE_ALSO: bdlqq_mutex
//
//@AUTHOR: Ilougino Rocha (irocha)
//
//@DESCRIPTION: This component provides an implementation of 'bdlqq::Mutex' for
// Windows (win32) via the template specialization:
//..
//  bdlqq::MutexImpl<Platform::Win32Threads>
//..
// This template class should not be used (directly) by client code.  Clients
// should instead use 'bdlqq::Mutex'.
//
///Usage
///-----
// This component is an implementation detail of 'bcemt' and is *not* intended
// for direct client use.  It is subject to change without notice.  As such, a
// usage example is not provided.

#ifndef INCLUDED_BDLSCM_VERSION
#include <bdlscm_version.h>
#endif

#ifndef INCLUDED_BDLQQ_PLATFORM
#include <bdlqq_platform.h>
#endif

#ifndef INCLUDED_BSLS_PLATFORM
#include <bsls_platform.h>
#endif

#ifdef BDLQQ_PLATFORM_WIN32_THREADS

// Platform-specific implementation starts here.

// Rather than setting 'WINVER' or 'NTDDI_VERSION', just forward declare the
// Windows 2000 functions that are used.

struct _RTL_CRITICAL_SECTION;

typedef struct _RTL_CRITICAL_SECTION CRITICAL_SECTION, *LPCRITICAL_SECTION;
typedef int BOOL;
typedef unsigned long DWORD;

extern "C" {
    __declspec(dllimport) BOOL __stdcall InitializeCriticalSectionAndSpinCount(
                                    LPCRITICAL_SECTION lpCriticalSection,
                                    DWORD dwSpinCount);

    __declspec(dllimport) void __stdcall DeleteCriticalSection(
                                LPCRITICAL_SECTION lpCriticalSection);

    __declspec(dllimport) BOOL __stdcall TryEnterCriticalSection(
                                 LPCRITICAL_SECTION lpCriticalSection);

    __declspec(dllimport) void __stdcall EnterCriticalSection(
                                LPCRITICAL_SECTION lpCriticalSection);

    __declspec(dllimport) void __stdcall LeaveCriticalSection(
                                LPCRITICAL_SECTION lpCriticalSection);

}  // extern "C"

namespace BloombergLP {
namespace bdlqq {

template <class THREAD_POLICY>
class MutexImpl;

               // =======================================
               // class MutexImpl<Platform::Win32Threads>
               // =======================================

template <>
class MutexImpl<Platform::Win32Threads> {
    // This class provides a full specialization of 'MutexImpl' for
    // Windows.  It provides an efficient proxy for Windows critical sections,
    // and related operations.  Note that the mutex implemented in this class
    // is *not* error checking, and is non-recursive.
    // TYPES

  public:
    enum {
        // Size of the buffer allocated for the critical section, in
        // pointer-sized elements.  We have to make it public so we could
        // access it in a .cpp file to verify the size.

#ifdef BSLS_PLATFORM_CPU_64_BIT
        // 5*8 = 40 bytes
        CRITICAL_SECTION_BUFFER_SIZE = 5
#else
        // 6*4 = 24 bytes
        CRITICAL_SECTION_BUFFER_SIZE = 6
#endif
    };

  private:
    enum {
        // A Windows critical section has a configurable spin count.  A lock
        // operation spins this many iterations (on, presumably, some atomic
        // integer) before sleeping on the underlying primitive.

        BCEMT_SPIN_COUNT = 30
    };

    // DATA
    void *d_lock[CRITICAL_SECTION_BUFFER_SIZE];

    // NOT IMPLEMENTED
    MutexImpl(const MutexImpl&);
    MutexImpl& operator=(const MutexImpl&);

  public:
    // PUBLIC TYPES
    typedef _RTL_CRITICAL_SECTION NativeType;
       // The underlying OS-level type.  Exposed so that other bcemt components
       // can operate directly on this mutex.

    // CREATORS
    MutexImpl();
        // Create a mutex initialized to an unlocked state.

    ~MutexImpl();
        // Destroy this mutex object.

    // MANIPULATORS
    void lock();
        // Acquire a lock on this mutex object.  If this object is currently
        // locked, then suspend execution of the current thread until a
        // lock can be acquired.  Note that the behavior is undefined if the
        // calling thread already owns the lock on this mutex, and will likely
        // result in a deadlock.

    NativeType& nativeMutex();
        // Return a reference to the modifiable OS-level mutex underlying this
        // object.  This method is intended only to support other bcemt
        // components that must operate directly on this mutex.

    int tryLock();
        // Attempt to acquire a lock on this mutex object.  Return 0 on
        // success, and a non-zero value of this object is already locked, or
        // if an error occurs.

    void unlock();
        // Release a lock on this mutex that was previously acquired through a
        // successful call to 'lock', or 'tryLock'.  The behavior is
        // undefined, unless the calling thread currently owns the lock on this
        // mutex.
};

}  // close package namespace

// ===========================================================================
//                            INLINE DEFINITIONS
// ===========================================================================

                // ---------------------------------------
                // class MutexImpl<Platform::Win32Threads>
                // ---------------------------------------

// CREATORS
inline
bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::MutexImpl()
{
    InitializeCriticalSectionAndSpinCount(
          reinterpret_cast<_RTL_CRITICAL_SECTION *>(d_lock), BCEMT_SPIN_COUNT);
}

inline
bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::~MutexImpl()
{
    DeleteCriticalSection(
        reinterpret_cast<_RTL_CRITICAL_SECTION*>(d_lock));
}

// MANIPULATORS
inline
void bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::lock()
{
    EnterCriticalSection(
        reinterpret_cast<_RTL_CRITICAL_SECTION*>(d_lock));
}

inline
bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::NativeType&
bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::nativeMutex()
{
    return *reinterpret_cast<_RTL_CRITICAL_SECTION*>(d_lock);
}

inline
int bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::tryLock()
{
    return !TryEnterCriticalSection(
        reinterpret_cast<_RTL_CRITICAL_SECTION*>(d_lock));
}

inline
void bdlqq::MutexImpl<bdlqq::Platform::Win32Threads>::unlock()
{
    LeaveCriticalSection(
        reinterpret_cast<_RTL_CRITICAL_SECTION*>(d_lock));
}

}  // close namespace BloombergLP

#endif // BDLQQ_PLATFORM_WIN32_THREADS

#endif

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
