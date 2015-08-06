// bdlqq_muteximpl_pthread.t.cpp                                      -*-C++-*-
#include <bdlqq_muteximpl_pthread.h>

#include <bsls_atomic.h>

#include <bsl_cstdlib.h>
#include <bsl_iostream.h>

using namespace BloombergLP;
using namespace bsl;

#ifdef BDLQQ_PLATFORM_POSIX_THREADS

//-----------------------------------------------------------------------------
//                  STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
static int testStatus = 0;

static void aSsErT(int c, const char *s, int i) {
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}
#define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }
//-----------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) {                                                    \
    if (!(X)) { cout << #I << ": " << I << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP2_ASSERT(I,J,X) {                                                 \
    if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\n";   \
                aSsErT(1, #X, __LINE__); } }
#define LOOP3_ASSERT(I,J,K,X) {                                               \
    if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J <<         \
                        "\t" << #K << ": " << K << "\n";                      \
                aSsErT(1, #X, __LINE__); } }
//-----------------------------------------------------------------------------
#define P(X) cout << #X " = " << (X) << endl; // Print identifier and value.
#define Q(X) cout << "<| " #X " |>" << endl;  // Quote identifier literally.
#define P_(X) cout << #X " = " << (X) << ", " << flush; // P(X) without '\n'
#define L_ __LINE__                           // current Line number
#define T_() cout << '\t' << flush;           // Print tab w/o line feed.

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

int verbose;
int veryVerbose;

typedef bdlqq::MutexImpl<bdlqq::Platform::PosixThreads> Obj;

void My_DoLock(Obj::NativeType* lock) {
    pthread_mutex_lock(lock);
}

void My_DoUnlock(Obj::NativeType* lock) {
    pthread_mutex_unlock(lock);
}

extern "C" {
   typedef void *(*ThreadFunction)(void *);
}

void My_Sleep(int ms)
{
    timespec naptime;

    naptime.tv_sec = 0;
    naptime.tv_nsec = ms * 1000000;
    nanosleep(&naptime, 0);
}

void My_CreateDetachedThread(ThreadFunction function,
                             void* userData) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,
                                PTHREAD_CREATE_DETACHED);
    pthread_t handle;
    int rc = pthread_create(&handle, &attr, function, userData);
    BSLS_ASSERT(0 == rc); // test invariant
}

struct ThreadInfo {
    Obj *d_lock;
    bsls::AtomicInt d_retval;
    bsls::AtomicInt d_retvalSet;
};

extern "C" void* MyThread(void* arg_p) {
    ThreadInfo* arg = (ThreadInfo*)arg_p;

    arg->d_retval = arg->d_lock->tryLock();
    if (0 == arg->d_retval) {
        arg->d_lock->unlock();
    }
    arg->d_retvalSet = 1;
    return arg_p;
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    verbose = argc > 2;
    veryVerbose = argc > 3;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    switch (test) { case 0:  // Zero is always the leading case.
    case 2: {
        // ------------------------------------------------------------------
        // Native mutex test
        //
        // Test that the native mutex is exposed properly by performing native
        // operations on it and checking the state of the Mutex object.
        // ------------------------------------------------------------------
        enum {
            MAX_SLEEP_CYCLES = 1000,
            SLEEP_MS = 100
        };

        if (verbose) {
            cout << "Native mutex test" << endl;
            cout << "=================" << endl;
        }
        Obj lock;

        Obj::NativeType* native = &lock.nativeMutex();

        My_DoLock(native);

        ThreadInfo args;
        args.d_lock = &lock;

        args.d_retval = 0;
        args.d_retvalSet = 0;
        My_CreateDetachedThread(MyThread, &args);
        for (int i = 0; 0 == args.d_retvalSet && i < MAX_SLEEP_CYCLES;
             ++i) {
            My_Sleep(SLEEP_MS);
        }
        ASSERT(args.d_retvalSet);
        ASSERT(0 != args.d_retval); // should fail
        if (verbose) {
            P(args.d_retval);
        }

        My_DoUnlock(native);

    } break;

    case 1: {
        // ------------------------------------------------------------------
        // Breathing test
        //
        // Create and destroy a mutex.  Lock and verify that tryLock fails;
        // unlock and verify that tryLock succeeds.
        // ------------------------------------------------------------------
        if (verbose) {
            cout << "BREATHING TEST" << endl;
            cout << "==============" << endl;
        }
        enum {
            MAX_SLEEP_CYCLES = 1000,
            SLEEP_MS = 100
        };

        {
            Obj lock;
            lock.lock();

            ThreadInfo args;
            args.d_lock = &lock;

            args.d_retval = 0;
            args.d_retvalSet = 0;
            My_CreateDetachedThread(MyThread, &args);
            for (int i = 0; 0 == args.d_retvalSet && i < MAX_SLEEP_CYCLES;
                 ++i) {
                My_Sleep(SLEEP_MS);
            }
            ASSERT(args.d_retvalSet);
            ASSERT(0 != args.d_retval); // should fail
            if (verbose) {
                P(args.d_retval);
            }

            lock.unlock();

            args.d_retval = 0;
            args.d_retvalSet = 0;
            My_CreateDetachedThread(MyThread, &args);
            for (int i = 0; 0 == args.d_retvalSet && i < MAX_SLEEP_CYCLES;
                 ++i) {
                My_Sleep(SLEEP_MS);
            }
            ASSERT(args.d_retvalSet);
            ASSERT(0 == args.d_retval); // should succeed
            if (verbose) {
                P(args.d_retval);
            }

        }
      } break;

      default: {
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = "
             << testStatus << "." << endl;
    }
    return testStatus;
}

#else

int main()
{
    return -1;
}

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
