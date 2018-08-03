// bslmt_testutil.t.cpp                                               -*-C++-*-
#include <bslmt_testutil.h>

#include <bslmt_barrier.h>
#include <bslmt_lockguard.h>
#include <bslmt_threadgroup.h>

#include <bslma_defaultallocatorguard.h>
#include <bslma_testallocator.h>

#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_platform.h>

#include <bsl_algorithm.h>
#include <bsl_cstddef.h>     // 'size_t'
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_string.h>
#include <bsl_sstream.h>

#include <fcntl.h>

#include <sys/types.h>  // for 'struct stat'; required on Sun and Windows only
#include <sys/stat.h>   // (ditto)

#if defined(BSLS_PLATFORM_OS_WINDOWS)
# include <io.h>        // '_dup2'
# define snprintf _snprintf
#else
# include <unistd.h>
#endif

// Ensure that the 'BSLMT_TESTUTIL_*' macros do not require:
//  using namespace bsl;

using namespace BloombergLP;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//                              Overview
//                              --------
// The component under test implements a set of macros that write to the
// standard output ('bsl::cout').
//
// The macros provided mirror the standard test macros normally used in test
// drivers.  The intention is that the standard test macros should be
// implemented as aliases of the 'BSLMT_TESTUTIL_*' macros, as illustrated in
// the first usage example.  As a result, the identifiers normally used in a
// test driver conflict with the identifiers used in the usage example.
// Therefore, this test driver avoids the standard test macros (and support
// functions), and uses the following instead:
//..
//      STANDARD        bslmt_testutil.t.cpp
//  ----------------    --------------------
//  LOOP1_ASSERT        REALLOOP1_ASSERT
//  LOOP2_ASSERT        REALLOOP2_ASSERT
//  LOOP3_ASSERT        REALLOOP3_ASSERT
//  LOOP4_ASSERT        REALLOOP4_ASSERT
//  LOOP5_ASSERT        not used
//  LOOP6_ASSERT        not used
//  Q                   not used
//  P                   REALP
//  P_                  REALT_
//  T_                  REALT_
//  L_                  not used
//  'void aSsErT()'     'void realaSsErT()'
//  'int testStatus'    'int realTestStatus'
//..
// All of the methods write output to 'stdout' and do nothing more.  Therefore,
// there are no Primary Manipulators or Basic Accessors to test.  All of the
// tests fall under the category of Print Operations.
//
// The main difficulty with writing this test driver is capturing the output of
// the methods under test so that it can be checked for accuracy.  In addition,
// error messages and other output produced by the test driver itself must
// still appear on 'stdout' for compatibility with the standard building and
// testing scripts.  For this purpose, a support class named 'OutputRedirector'
// is provided.  'OutputRedirector' redirects 'stdout' to a temporary file and
// verifies that the contents of the temporary file match the character buffers
// provided by the user.
//-----------------------------------------------------------------------------
// CLASS METHODS
// [ 7] void *callFunc(void *arg);
// [ 7] void setFunc(Func func);
//
// MACROS
// [ 6] BSLMT_TESTUTIL_ASSERT(X)
// [ 6] BSLMT_TESTUTIL_LOOP0_ASSERT(X)
// [ 6] BSLMT_TESTUTIL_LOOP1_ASSERT(I,X)
// [ 6] BSLMT_TESTUTIL_LOOP2_ASSERT(I,J,X)
// [ 6] BSLMT_TESTUTIL_LOOP3_ASSERT(I,J,K,X)
// [ 6] BSLMT_TESTUTIL_LOOP4_ASSERT(I,J,K,L,X)
// [ 6] BSLMT_TESTUTIL_LOOP5_ASSERT(I,J,K,L,M,X)
// [ 6] BSLMT_TESTUTIL_LOOP6_ASSERT(I,J,K,L,M,N,X)
// [ 6] BSLMT_TESTUTIL_ASSERTV(...)
// [ 5] BSLMT_TESTUTIL_Q(X)
// [ 4] BSLMT_TESTUTIL_P(X)
// [ 4] BSLMT_TESTUTIL_P_(X)
// [ 3] BSLMT_TESTUTIL_L_
// [ 3] BSLMT_TESTUTIL_T_
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 2] TEST APPARATUS
// [ 8] MULTITHREADED TEST
// [ 9] USAGE EXAMPLE
//-----------------------------------------------------------------------------

// ============================================================================
//                VARIATIONS ON STANDARD BDE ASSERT TEST MACROS
// ----------------------------------------------------------------------------
//
// In order to accommodate the use of the identifiers 'testStatus' and 'aSsErT'
// in the usage example, the rest of the test driver uses the identifiers
// 'realTestStatus' and 'realaSsErT' instead.
//
// Additionally, in order to allow capturing the output of the
// 'BSLMT_TESTUTIL_*' macros, the standard macros output to 'stderr' instead of
// 'stdout'.

static int realTestStatus = 0;

static void realaSsErT(bool b, const char *s, int i)
    // Implement 'aSsErT' for the 'REAL*' macros.  If the specified 'b' is
    // false, emit a message showing the specified code expression 's' and the
    // specified line number 'i'.
{
    if (b) {
        bsl::cerr << "Error " << __FILE__ << "(" << i << "): " << s
                  << "    (failed)" << bsl::endl;
        if (realTestStatus >= 0 && realTestStatus <= 100) ++realTestStatus;
    }
}
// The standard 'ASSERT' macro definition is deferred until after the usage
// example code:
//..
// #define ASSERT(X) { realaSsErT(!(X), #X, __LINE__); }
//..

#define REAL_ASSERT(X)                                                        \
    if (X) ; else do { realaSsErT(1, #X, __LINE__); } while (false)

#define REALLOOP1_ASSERT(I,X)                                                 \
    if (X) ; else do { bsl::cerr << #I << ": " << (I) << "\n";                \
                       realaSsErT(1, #X, __LINE__); } while (false)

#define REALLOOP2_ASSERT(I,J,X)                                               \
    if (X) ; else do { bsl::cerr << #I << ": " << (I) << "\t"                 \
                                 << #J << ": " << (J) << "\n";                \
                       realaSsErT(1, #X, __LINE__); } while (false)

#define REALLOOP3_ASSERT(I,J,K,X)                                             \
    if (X) ; else do { bsl::cerr << #I << ": " << (I) << "\t"                 \
                                 << #J << ": " << (J) << "\t"                 \
                                 << #K << ": " << (K) << "\n";                \
                       realaSsErT(1, #X, __LINE__); } while (false)

#define REALLOOP4_ASSERT(I,J,K,L,X)                                           \
    if (X) ; else do { bsl::cerr << #I << ": " << (I) << "\t"                 \
                                 << #J << ": " << (J) << "\t"                 \
                                 << #K << ": " << (K) << "\t"                 \
                                 << #L << ": " << (L) << "\n";                \
                       realaSsErT(1, #X, __LINE__); } while (false)

#define REALP(X)                                                              \
    (bsl::cerr << #X " = " << (X) << bsl::endl)

#define REALP_(X)                                                             \
    (bsl::cerr << #X " = " << (X) << ", " << bsl::flush)

#define REALT_                                                                \
    (bsl::cerr << "\t" << bsl::flush)

#define REALL_                                                                \
    __LINE__

//=============================================================================
//                             USAGE EXAMPLE CODE
//-----------------------------------------------------------------------------

namespace Usage {

///Example 1: Writing a Test Driver
/// - - - - - - - - - - - - - - - -
// First, we write an elided component to test, which provides a utility class:
//..
    namespace bslabc {

    struct SumUtil {
        // This utility class provides sample functionality to demonstrate how
        // a multi threaded test driver might be written.

        // CLASS METHODS
        static double sumOfSquares(double a,
                                   double b = 0,
                                   double c = 0,
                                   double d = 0);
            // Return the sum of the squares of one to 4 arguments, the
            // specified 'a' and the optionally specified 'b', 'c', and 'd'.
    };

    // CLASS METHODS
    inline
    double SumUtil::sumOfSquares(double a,
                                 double b,
                                 double c,
                                 double d)
    {
        // Note that there is a bug here in that we have added the cube, rather
        // than the square, of 'd'.

        double ret = a*a;
        ret += b*b;
        ret += c*c;
        ret += d*d*d;
        return ret;
    }

    }  // close namespace bslabc
//..
// Then, we can write an elided test driver for this component.  We start by
// providing the standard BDE assert test macro:
//..
    //=========================================================================
    //                       STANDARD BDE ASSERT TEST MACRO
    //-------------------------------------------------------------------------
    int testStatus = 0;

    void aSsErT(int c, const char *s, int i)
    {
        if (c) {
            bsl::cout << "Error " << __FILE__ << "(" << i << "): " << s
                      << "    (failed)" << bsl::endl;
            if (testStatus >= 0 && testStatus <= 100) ++testStatus;
        }
    }
//..
// Next, we define the standard print and 'ASSERT*' macros, as aliases to the
// macros defined by this component:
//..
    //=========================================================================
    //                       STANDARD BDE TEST DRIVER MACROS
    //-------------------------------------------------------------------------
    #define ASSERT   BSLMT_TESTUTIL_ASSERT
    #define ASSERTV  BSLMT_TESTUTIL_ASSERTV

    #define Q        BSLMT_TESTUTIL_Q   // Quote identifier literally.
    #define P        BSLMT_TESTUTIL_P   // Print identifier and value.
    #define P_       BSLMT_TESTUTIL_P_  // P(X) without '\n'.
    #define T_       BSLMT_TESTUTIL_T_  // Print a tab (w/o newline).
    #define L_       BSLMT_TESTUTIL_L_  // current Line number
//..
// Then, we define global verbosity flags to be used for controlling debug
// traces.  The flags will be set by elided code at the beginning of 'main' to
// determine the level of output verbosity the client wants.  We also define
// six global double variables that we will use for testing, and a barrier to
// coordinate threads
//..
    //=========================================================================
    //                     GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
    //-------------------------------------------------------------------------

    bool  verbose;
    bool  veryVerbose;
    bool  veryVeryVerbose;
    bool  veryVeryVeryVerbose;
//..
// Next we define some global typedefs, variables, and constants used by this
// test case:
//..
                                // ---------------
                                // Usage Test Case
                                // ---------------

    typedef  bslabc::SumUtil SU;

    bsls::AtomicInt threadIdx(0);
    bsls::AtomicInt lastRand(0);

    enum { k_NUM_ITERATIONS     = 5,
           k_NUM_THREADS        = 10,
           k_NUM_RAND_VARIABLES = k_NUM_ITERATIONS * k_NUM_THREADS * 4 };

    double *randNumbers;
//..
// Then, using out test macros, we write five test functors that can be run
// concurrently to test the five static functions.
//..
    struct TestSums {
        void operator()()
            // Test 'TestUtil::sumOfSquares' with a variety of randomly
            // generated arguments to be found in the array 'randNumbers'
        {
            const int idx     = threadIdx++;
            int       randIdx = idx * k_NUM_ITERATIONS * 4;

            if (veryVerbose) {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                // Instantiating the output guard above locks the output
                // mutex.  The following two macros would lock that mutex
                // themselves, except that they detect the output guard and
                // refrain.  Having the two macros guarded by one output guard
                // ensures that the output from both will appear on the same
                // line, uninterrupted by output from the 'BSLMT_TESTUTIL_*'
                // macros being called from any other thread.

                P_(idx);    P(randIdx);
            }

            for (int ii = 0; ii < k_NUM_ITERATIONS; ++ii) {
                double x[4];
                for (int jj = 0; jj < 4; ++jj, ++randIdx) {
                    x[jj] = randNumbers[randIdx];
                }
                if (randIdx >= k_NUM_RAND_VARIABLES) {
                    BSLMT_TESTUTIL_OUTPUT_GUARD;

                    // We expect with the following two asserts, if either one
                    // fails, both will fail.  We create the output guard above
                    // to ensure that if they both output, their output will
                    // appear adjacent to each other, uninterrupted by any
                    // output from 'bslmt_TESTUTIL_*' macros being called by
                    // any other thread.

                    ASSERTV(randIdx, k_NUM_RAND_VARIABLES, !lastRand);
                    ASSERTV(randIdx, k_NUM_RAND_VARIABLES,
                                              randIdx == k_NUM_RAND_VARIABLES);
                    lastRand = true;

                    if (veryVerbose) {
                        // This output calling 'cout' could become illegible if
                        // 'BSLMT_TESTUTIL_*' macros in the other threads
                        // output at the same time.  We are safe here because
                        // we are within scope of an output guard.

                        bsl::cout << "Thread number " << idx <<
                             " reached the end of the random number buffer." <<
                                                                     bsl::endl;
                    }
                }

                // If any of the 'ASSERTV's following here fail, they will
                // detect that no output guard object is in scope and lock the
                // output mutex before doing any output, so the entire trace
                // from any one failing 'ASSERTV' will be in one contiguous
                // block.

                double exp = x[0] * x[0];
                ASSERTV(x[0], exp, SU::sumOfSquares(x[0]),
                                                exp == SU::sumOfSquares(x[0]));

                exp += x[1] * x[1];
                ASSERTV(x[0], x[1], exp, SU::sumOfSquares(x[0], x[1]),
                                          exp == SU::sumOfSquares(x[0], x[1]));

                exp += x[2] * x[2];
                ASSERTV(x[0], x[1], x[2], exp,
                                            SU::sumOfSquares(x[0], x[1], x[2]),
                                    exp == SU::sumOfSquares(x[0], x[1], x[2]));

                exp += x[3] * x[3];
                ASSERTV(x[0], x[1], x[2], x[3], exp,
                                      SU::sumOfSquares(x[0], x[1], x[2], x[3]),
                              exp == SU::sumOfSquares(x[0], x[1], x[2], x[3]));
            }

            if (veryVerbose) {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                bsl::cout << "Thread number " << idx << " finishing." <<
                                                                     bsl::endl;
            }
        }
    };
//..
//..
int testMain()    // do not copy to .h file
{
//..
// Next, in 'main', we allocate and populate our array of random numbers with
// 'bsl::rand', which is single-threaded:
//..
    bslma::TestAllocator testAllocator("usage");
    randNumbers = static_cast<double *>(testAllocator.allocate(
                                       sizeof(double) * k_NUM_RAND_VARIABLES));

    for (int ii = 0; ii < k_NUM_RAND_VARIABLES; ++ii) {
        randNumbers[ii] = static_cast<double>(bsl::rand()) / RAND_MAX *
                                                                   bsl::rand();
    }
//..
// Then, we spawn our threads and let them run:
//..
    bslmt::ThreadGroup tg;
    tg.addThreads(TestSums(), k_NUM_THREADS);
//..
// Now, we join the threads, clean up, and at the end of 'main' examine
// 'testStatus'.  If it's greater than 0, report that the test failed.H:
//..
    tg.joinAll();

    testAllocator.deallocate(randNumbers);

    if (testStatus > 0) {
        // Note that since there is a bug in 'SU::sumOfSquares' with 4 args, we
        // expect the last assert in 'TestSums::operator()' to fail 5
        // iterations times 10 threads == 50 times, so the following message
        // will report 'test status = 50'.

        bsl::cerr << "Error, non-zero test status = " << testStatus << "."
                  << bsl::endl;
    }

    return testStatus;
//..
// Finally, after the program has run, we see 50 assertion failures in the
// output with differring values of the 'x[*]' variables, but each assert's
// output is intact (rather than being interleaved with output from asserts in
// other threads).
}

}  // close namespace Usage

//=============================================================================
//                    UNDEFINE STANDARD TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#undef ASSERT
#undef ASSERTV

#undef Q
#undef P
#undef P_
#undef T_
#undef L_

//=============================================================================
//                    DEFINE SHORTHAND MACROS UNDER TEST
//-----------------------------------------------------------------------------

#define MT_ASSERT      BSLMT_TESTUTIL_LOOP0_ASSERT
#define MT_ASSERTV     BSLMT_TESTUTIL_ASSERTV

#define MT_Q           BSLMT_TESTUTIL_Q   // Quote identifier literally.
#define MT_P           BSLMT_TESTUTIL_P   // Print identifier and value.
#define MT_P_          BSLMT_TESTUTIL_P_  // P(X) without '\n'.
#define MT_T_          BSLMT_TESTUTIL_T_  // Print a tab (w/o newline).
#define MT_L_          BSLMT_TESTUTIL_L_  // current Line number

int&      testStatus          = Usage::testStatus;

bool&     verbose             = Usage::verbose;
bool&     veryVerbose         = Usage::veryVerbose;
bool&     veryVeryVerbose     = Usage::veryVeryVerbose;
bool&     veryVeryVeryVerbose = Usage::veryVeryVeryVerbose;

typedef bslmt::TestUtil Obj;

int                  test;
bslma::TestAllocator ta("test");

inline
void aSsErT(int c, const char *s, int i)
{
    Usage::aSsErT(c, s, i);
}

//=============================================================================
//                       GLOBAL HELPER CLASSES FOR TESTING
//-----------------------------------------------------------------------------

namespace BSLMT_TESTUTIL_TEST_FUNCTION_CALL {

int callCount = 0;

void *testFunctionAdd(void *arg)
    // Increment call counter and return the specified 'arg'.
{
    ++callCount;
    return arg;
}

void *testFunctionSub(void *arg)
    // Decrement call counter and return the specified 'arg'.
{
    --callCount;
    return arg;
}
}  // close namespace BSLMT_TESTUTIL_TEST_FUNCTION_CALL

namespace {
namespace u {

class OutputRedirector {
    // This class provides a facility for redirecting 'stdout' to a temporary
    // file, retrieving output from the temporary file, and comparing the
    // output to user-supplied character buffers.  An 'OutputRedirector' object
    // can exist in one of two states, un-redirected or redirected.  In the
    // un-redirected state, the process' 'stdout' and 'stderr' are connected to
    // their normal targets.  In the redirected state, the process' 'stdout' is
    // connected to a temporary file, and the process' 'stderr' is connected to
    // the original target of 'stdout'.  The redirected state of an
    // 'OutputRedirector' object can be tested by calling 'isRedirected'.  An
    // 'OutputRedirector' object has the concept of a scratch buffer, where
    // output captured from the process' 'stdout' stream is stored when the
    // 'OutputRedirector' object is in the redirected state.  Throughout this
    // class, the term "captured output" refers to data that has been written
    // to the 'stdout' stream and is waiting to be loaded into the scratch
    // buffer.  Each time the 'load' method is called, the scratch buffer is
    // truncated, and the captured output is moved into the scratch buffer.
    // When this is done, there is no longer any captured output.

    // DATA
    const bsl::string d_fileName;

    char *d_outputBuffer_p;                   // scratch buffer for holding
                                              // captured output

    bool d_isRedirectedFlag;                  // has 'stdout' been redirected

    bool d_isFileCreatedFlag;                 // has a temp file been created

    bool d_isOutputReadyFlag;                 // has output been read from the
                                              // temp file

    long d_outputSize;                        // size of output loaded into
                                              // 'd_outputBuffer_p'

    struct stat d_originalStdoutStat;         // status information for
                                              // 'stdout' just before
                                              // redirection

    bslma::Allocator *d_allocator_p;

  private:
    // PRIVATE CLASS METHODS
    static int redirectStderrToStdout();
        // Redirect stderr to stdout.  Return 0 on success, and a negative
        // value otherwise.

    // PRIVATE MANIPULATORS
    void cleanup();
        // Close 'stdout', if redirected, and delete the temporary output
        // capture file.

  private:
    // NOT IMPLEMENTED
    OutputRedirector(const OutputRedirector&);
    OutputRedirector& operator=(const OutputRedirector&);

  public:
    // CREATORS
    OutputRedirector(const bsl::string& fileName, bslma::Allocator *alloc);
        // Create an 'OutputRedirector' in an un-redirected state having empty
        // scratch buffer, ready to open the specified 'fileName'.  Use the
        // specified 'alloc' to supply memory.

    ~OutputRedirector();
        // Destroy this 'OutputRedirector' object.  If the object is in a
        // redirected state, 'stdout' will be closed and the temporary file to
        // which 'stdout' was redirected will be removed.

    // MANIPULATORS
    char *buffer();
        // Return the address providing modifiable access to the scratch
        // buffer.  Note that this method is used only for error reporting and
        // to test the correctness of 'OutputRedirector'.

    bool load();
        // Read captured output into the scratch buffer.  Return 'true' if all
        // captured output was successfully loaded, and 'false' otherwise.  The
        // behavior is undefined unless 'redirect' has been previously called
        // successfully.  Note that captured output is allowed to have 0
        // length.

    int numInstances(const bsl::string& expected);
        // Scan the buffer for instances of the specified 'expected', and
        // return the number of of instances of 'expected' that were found.
        // Overwrite the matched memory in the buffer with '*'s.

    void redirect();
        // Redirect 'stdout' to a temp file, and 'stderr' to the original
        // 'stdout', putting this 'OutputRedirector' object into the redirected
        // state.  The temp file to which 'stdout' is redirected will be
        // created the first time 'redirect' is called, and will be removed
        // when this object is destroyed.  Subsequent calls to this method no
        // effect on 'stdout' and 'stderr'.  If this method fails to redirect
        // either 'stdout' or 'stderr', 'bsl::abort' is called.

    void reset();
        // Reset the scratch buffer to empty.  The behavior is undefined unless
        // 'redirect' has been previously called successfully.

    // ACCESSORS
    int compare(const char *expected) const;
        // Compare the specified 'expected' character buffer with any output
        // that has been loaded into the scratch buffer.  'expected' is assumed
        // to be an NTBS, and its length is taken to be the string length of
        // the NTBS.  Return 0 if 'expected' has the same length and contents
        // as the scratch buffer, and non-zero otherwise.  The behavior is
        // undefined unless 'redirect' has been previously called successfully.

    int compare(const char *expected, bsl::size_t expectedLength) const;
        // Compare the specified 'expected' character buffer of the specified
        // 'expectedLength' with any output that has been loaded into the
        // scratch buffer.  Return 0 if 'expected' has the same length and
        // contents as the scratch buffer, and non-zero otherwise.  The
        // behavior is undefined unless 'redirect' has been previously called
        // successfully.  Note that 'expected' may contain embedded null
        // characters.

    bool isOutputReady() const;
        // Return 'true' if captured output been loaded into the scratch
        // buffer, and 'false' otherwise.

    bool isRedirected() const;
        // Return 'true' if 'stdout' and 'stderr' have been successfully
        // redirected, and 'false' otherwise.

    const struct stat& originalStdoutStat() const;
        // Return a reference providing non-modifiable access to the status
        // information for 'stdout' collected just before redirection.  Note
        // that this method is used only to test the correctness of
        // 'OutputRedirector'.

    bsl::size_t outputSize() const;
        // Return the number of bytes currently loaded into the scratch buffer.
};

// PRIVATE CLASS METHODS
int OutputRedirector::redirectStderrToStdout()
{
    // The canonical way to redirect 'stderr' to 'stdout' is:
    //..
    //  freopen("/dev/stdout", "w", stderr);
    //..
    // but we use 'dup2' instead of 'freopen', because 'freopen' fails on AIX
    // with 'errno == 13' ("Permission denied") when redirecting 'stderr'.

#if defined(BSLS_PLATFORM_OS_AIX)
    FILE *from = stderr;
    FILE *to   = stdout;

    int redirected = dup2(fileno(from), fileno(to));
    return redirected == fileno(to) ? 0 : -1;
#elif defined(BSLS_PLATFORM_OS_WINDOWS)
    FILE *from = stderr;
    FILE *to   = stdout;

    return _dup2(_fileno(from), _fileno(to));
#else
    return (stderr == freopen("/dev/stdout", "w", stderr)) ? 0 : -1;
#endif
}

// PRIVATE MANIPULATORS
void OutputRedirector::cleanup()
{
    if (d_isRedirectedFlag) {
        fclose(stdout);
    }

    if (d_isFileCreatedFlag) {
        unlink(d_fileName.c_str());
    }
}

// CREATORS
OutputRedirector::OutputRedirector(const bsl::string&  fileName,
                                   bslma::Allocator   *alloc)
: d_fileName(fileName, alloc)
, d_outputBuffer_p(0)
, d_isRedirectedFlag(false)
, d_isFileCreatedFlag(false)
, d_isOutputReadyFlag(false)
, d_outputSize(0L)
, d_allocator_p(alloc)
{
    unlink(d_fileName.c_str());

    bsl::memset(&d_originalStdoutStat, 0, sizeof(struct stat));
}

OutputRedirector::~OutputRedirector()
{
    cleanup();

    if (d_outputBuffer_p) {
        d_allocator_p->deallocate(d_outputBuffer_p);
    }
}

// MANIPULATORS
char *OutputRedirector::buffer()
{
    return d_outputBuffer_p;
}

bool OutputRedirector::load()
{
    REAL_ASSERT(d_isRedirectedFlag);
    REAL_ASSERT(!ferror(stdout));

    if (d_outputBuffer_p) {
        d_allocator_p->deallocate(d_outputBuffer_p);
    }
    d_outputSize = ftell(stdout);
    d_outputBuffer_p = static_cast<char *>(
                                    d_allocator_p->allocate(d_outputSize + 1));

    rewind(stdout);

    long charsRead =
                   fread(d_outputBuffer_p, sizeof(char), d_outputSize, stdout);

    if (d_outputSize != charsRead) {

        // We failed to read all output from the capture file.

        if (veryVerbose) {
            REALP_(d_outputSize); REALP(charsRead);
            if (ferror(stdout)) {

                // We encountered a file error, not 'EOF'.

                perror("\tError message: ");
                clearerr(stdout);
            }
            bsl::fprintf(stderr,
                         "Error "
                         __FILE__
                         "(%d): Could not read all captured output\n",
                         __LINE__);
        }

        d_outputBuffer_p[charsRead] = '\0';

            // ... to ensure that direct inspection of buffer does not overflow

        return false;                                                 // RETURN
    } else {

        // We have read all output from the capture file.

        d_outputBuffer_p[d_outputSize] = '\0';

        // ... to ensure that direct inspection of buffer does not overflow
    }

    d_isOutputReadyFlag = true;

    return true;
}

int OutputRedirector::numInstances(const bsl::string& expected)
{
    BSLS_ASSERT(d_isOutputReadyFlag);
    BSLS_ASSERT(!expected.empty());
    BSLS_ASSERT(bsl::string::npos == expected.find('#'));

    const char        *e   = expected.c_str();
    const bsl::size_t  len = expected.length();

    int ret = 0;
    for (char *pc = d_outputBuffer_p; (pc = bsl::strstr(pc, e)); ) {
        ++ret;

        bsl::fill(pc, pc + len, '#');
    }

    return ret;
}

void OutputRedirector::redirect()
{
    if (d_isRedirectedFlag) {

        // Do not redirect anything if we have already redirected.

        if (veryVerbose) {
            bsl::fprintf(stdout,
                         "Warning " __FILE__
                         "(%d): Output already redirected\n",
                         __LINE__);
        }

        return;                                                       // RETURN
    }

    // Retain information about the original 'stdout' file descriptor for use
    // in later tests.

    int originalStdoutFD = fileno(stdout);
    REAL_ASSERT(-1 != originalStdoutFD);
    REAL_ASSERT(0 == fstat(originalStdoutFD, &d_originalStdoutStat));

    // Redirect 'stderr' to 'stdout'.

    if (0 != redirectStderrToStdout()) {
        // We want 'stderr' to point to 'stdout', so we have to redirect it
        // before we change the meaning of 'stdout'.

        if (veryVerbose) {

            // Note that we print this error message on 'stdout' instead of
            // 'stderr', because 'stdout' has not been redirected.

            bsl::fprintf(stdout,
                         "Error " __FILE__ "(%d): Failed to redirect stderr\n",
                         __LINE__);
        }
        bsl::abort();
    }

    // Redirect 'stdout' to our test file.

    if (!freopen(d_fileName.c_str(), "w+", stdout)) {
        if (veryVerbose) {

            // Note that we print this error message on 'stderr', because we
            // have just redirected 'stdout' to the capture file.

            REALP(d_fileName.c_str());
            bsl::fprintf(stderr,
                         "Error " __FILE__ "(%d): Failed to redirect stdout\n",
                         __LINE__);
        }
        cleanup();
        bsl::abort();
    }

    // 'stderr' and 'stdout' have been successfully redirected.

#if defined(BSLS_PLATFORM_OS_WINDOWS)
    if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
        REAL_ASSERT(0 == "Failed to set stdout to binary mode.");
        cleanup();
        bsl::abort();
    }
#endif

    d_isFileCreatedFlag = true;
    d_isRedirectedFlag  = true;

    if (EOF == fflush(stdout)) {
        // Not flushing 'stdout' is not a fatal condition, so we print out a
        // warning, but do not abort.

        if (veryVerbose) {

            // Note that we print this error message on 'stderr', because we
            // have just redirected 'stdout' to the capture file.

            perror("Error message: ");
            bsl::fprintf(stderr,
                         "Warning " __FILE__ "(%d): Error flushing stdout\n",
                         __LINE__);
        }
    }
}

void OutputRedirector::reset()
{
    REAL_ASSERT(d_isRedirectedFlag);

    if (d_outputBuffer_p) {
        d_allocator_p->deallocate(d_outputBuffer_p);
        d_outputBuffer_p = 0;
    }
    d_outputSize        = 0L;
    d_isOutputReadyFlag = false;
    rewind(stdout);
}

// ACCESSORS
int OutputRedirector::compare(const char *expected) const
{
    REAL_ASSERT(expected);

    return compare(expected, bsl::strlen(expected));
}

int OutputRedirector::compare(const char  *expected,
                              bsl::size_t  expectedLength) const
{
    REAL_ASSERT(d_isRedirectedFlag);
    REAL_ASSERT(expected || !expectedLength);

    if (!d_isOutputReadyFlag) {
        if (veryVerbose) {
            REALP(expected);
            bsl::fprintf(stderr,
                         "Error " __FILE__
                         "(%d): No captured output available\n",
                         __LINE__);
        }
        return -1;                                                    // RETURN
    }

    // Use 'memcmp' instead of 'strncmp' to compare 'd_outputBuffer_p' with
    // 'expected' because 'expected' may contain embedded null characters.

    return d_outputSize != static_cast<long>(expectedLength)
        || bsl::memcmp(d_outputBuffer_p, expected, expectedLength);
}

bool OutputRedirector::isOutputReady() const
{
    return d_isOutputReadyFlag;
}

bool OutputRedirector::isRedirected() const
{
    return d_isRedirectedFlag;
}

const struct stat& OutputRedirector::originalStdoutStat() const
{
    return d_originalStdoutStat;
}

bsl::size_t OutputRedirector::outputSize() const
{
    return static_cast<bsl::size_t>(d_outputSize);
}

}  // close namespace u
}  // close unnamed namespace

namespace MultiThreadedTest {

enum { k_NUM_THREADS = 40,
       k_NUM_ITERATIONS  = 100,
       k_NUM_REPEATED_RECORDS = 4 };

bslmt::Barrier       barrier(k_NUM_THREADS + 1);
bsls::AtomicInt      atomicBarrier(0);
bsls::AtomicInt      threadIdx(0);
bsls::AtomicInt      numChecked(0);

u::OutputRedirector *outputRedirector_p = 0;
bslmt::Mutex         checkMutex;

struct ThreadData {
    // Data for one thread.  All the variables will be initialized to have
    // distinct values for each thread, so that output take takes variables
    // will all be unique to each thread, and should be able to be found in
    // the output buffer.

    // CONSTANTS
    enum { k_NUM_ASSERTS = 8 };

    // DATA
    const unsigned d_idx;        // Index of this thread, range '[ 0 .. 40 )'

    const int      d_arf;        // All these variables will be given unique
    const int      d_woof;       // values for each thread, so that output
    const int      d_meow;       // done by a thread that outputs variables
    const double   d_fracA;      // will be unique to that thread.
    const double   d_fracB;
    const double   d_fracC;
    const double   d_fracD;
    const double   d_fracE;
    const double   d_fracF;

    int            d_assertLines[k_NUM_ASSERTS];    // line numbers where the
                                                    // 'MT_ASSERT*'s occur.

    // CREATOR
    explicit
    ThreadData(unsigned idx)
    : d_idx(idx)
    , d_arf(idx * 43)
    , d_woof(idx * 47)
    , d_meow(idx * 379 + 271)
    , d_fracA(idx * 59.0 / 64)
    , d_fracB(idx * 61.0 / 128)
    , d_fracC(idx * 101.0 / 128)
    , d_fracD(idx * 383.0 / 512)
    , d_fracE(idx * 397.0 / 512)
    , d_fracF(idx * 2237.0 / (4 * 1024))
        // Initialize all the variables other than 'd_assertLine' and
        // 'd_assertVLine' to have values unique to this thread, given a
        // specified 'idx' that is a thread index unique to the thread.
    {}

    // MANIPULATOR
    void doOutput()
        // Iterate 'K_NUM_ITERATIONS' times, doing various kinds of output
        // simultaneous with all the other threads.  All output containing
        // variables, which all have unique values for each thread, should
        // occur exactly 'k_NUM_ITERATIONS' times, and output not containing
        // variables should occur exactly 'k_NUM_THREADS * k_NUM_ITERATIONS'
        // times.
    {
        for (int ii = 0; ii < k_NUM_ITERATIONS; ++ii) {
            // Test 'MT_Q', 'MT_P', and 'MT_P_', all of which have built-in
            // mutex control so their respective outputs will occur distinct
            // but intact.

            MT_Q(meow bow wow wow wow);
            MT_P(d_fracA);
            MT_P_(d_fracB);

            // Output a block out output.  The 'BSLMT_TESTUTIL_OUTPUT_GUARD'
            // creates a guard locking the output mutex, and the 'MT_P_',
            // 'MT_P', 'MT_Q', and 'MT_T_' macros will all detect the presence
            // of that guard and not try to lock the mutex themselves.

            {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                MT_P_(d_idx);    MT_T_    MT_P_(d_arf);    MT_P(d_fracC);
                MT_P(d_idx * d_arf * d_fracD);
                MT_Q(To be or not to be -- that is the question.);
            }

            int assertIdx = 0;

            // An assert with no variable output.

            d_assertLines[assertIdx++] = MT_L_ + 1;
            MT_ASSERT(d_woof == d_meow);

            // An assert with several variables output.

            d_assertLines[assertIdx++] = MT_L_ + 1;
            MT_ASSERTV(d_woof, d_meow, d_fracE, d_fracF, d_meow < d_fracF);

            // Output block containing regular stream output.

            {
                BSLMT_TESTUTIL_OUTPUT_GUARD;
                using namespace bsl;

                cout << "There are more things in heaven and earth, Horatio,"<<
                                                                          endl;
                cout << "Than are dreamt of in your philosophy." << endl;
                cout << d_fracD * d_fracE << endl;
            }

            // Output blocks containing all the asserts we haven't done yet.

            {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                d_assertLines[assertIdx++] = MT_L_ + 1;
                MT_ASSERTV(d_woof > 2 * d_meow);
                d_assertLines[assertIdx++] = MT_L_ + 1;
                MT_ASSERTV(d_woof, d_woof > 3 * d_meow);
            }

            // In Aix, the value of '__LINE__' within a macro is at the first
            // line, on other platforms, it's at the last line.

#if !defined(BSLS_PLATFORM_OS_AIX)
            const int twoLineMacroRel = 2;
#else
            const int twoLineMacroRel = 1;
#endif

            {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                d_assertLines[assertIdx++] = MT_L_ + 1;
                MT_ASSERTV(d_woof, d_meow, d_woof >= 3 * d_meow);
                d_assertLines[assertIdx++] = MT_L_ + twoLineMacroRel;
                MT_ASSERTV(d_arf, d_meow, d_fracA,
                                                d_arf + d_meow == 3 * d_fracA);
            }

            {
                BSLMT_TESTUTIL_OUTPUT_GUARD;

                d_assertLines[assertIdx++] = MT_L_ + twoLineMacroRel;
                MT_ASSERTV(d_arf, d_meow, d_fracA, d_fracB, d_fracC,
                            d_arf + d_meow == 3 * d_fracA * d_fracB * d_fracC);
                d_assertLines[assertIdx++] = MT_L_ + twoLineMacroRel;
                MT_ASSERTV(d_arf, d_meow, d_fracA, d_fracB, d_fracC, d_fracD,
                  d_arf + d_meow == 3 * d_fracA * d_fracB * d_fracC * d_fracD);
            }

            BSLS_ASSERT(k_NUM_ASSERTS == assertIdx);
        }
    }

    // ACCESSOR
    void checkPatterns() const
        // The threads are all done running 'doOutput' and doing output and
        // the output redirector has been loaded.  Now we look through the
        // buffer of output for all the expected output.  As we find expected
        // patterns, we count the number of times they occur and fill the
        // buffer footprint of the patterns with '#' (both of which are done
        // by 'u::OutputRedirector::numInstances').
    {
        bslmt::LockGuard<bslmt::Mutex> lockGuard(&checkMutex);
            // We are going to be using the single-threaded 'REAL*' asserts and
            // using the 'str()' accessor of 'bsl::ostringstream', which uses
            // the default allocator, and we want to install a default
            // allocator guard to guide that to use our test allocator, which
            // doesn't work well in a multithreaded context, so we control this
            // method with a mutex to ensure that happens on only one thread at
            // a time.  The method 'u::OutputRedirector::numInstances' is also
            // not thread-safe.

        bslma::DefaultAllocatorGuard  allocGuard(&ta);
        bsl::ostringstream            oss(&ta);
        bsl::string                   pattern(&ta);
        int                           expMatch;
        int                           assertIdx = 0;

#define PV(X)    #X ": " << X << '\n'
#define PV_(X)   #X ": " << X << '\t'
#define PE      "Error " << __FILE__ << '(' << d_assertLines[assertIdx++]

        if (0 == d_idx) {
            // First, check output that did not contain variable values, which
            // will have been output every iteration by every thread.  Only do
            // this in the first thread.

            ++numChecked;
            expMatch = k_NUM_THREADS * k_NUM_ITERATIONS;

            pattern = "<| meow bow wow wow wow |>\n";
            const int numQ = outputRedirector_p->numInstances(pattern);
            REALLOOP4_ASSERT(d_idx, pattern, expMatch, numQ,
                                                           expMatch == numQ);

            oss.str("");
            oss << PE <<"): d_woof == d_meow    (failed)\n";
            pattern = oss.str();
            const int numA = outputRedirector_p->numInstances(pattern);
            REALLOOP4_ASSERT(d_idx, pattern, expMatch, numA, expMatch == numA);
        }
        else {
            ++assertIdx;
        }

        // Now check all the output that had variable values in it.  Since the
        // variables were all unique to each thread, each pattern will be
        // unique to this thread and will have occurred exactly
        // 'k_NUM_ITERATINS' times.

        ++numChecked;
        expMatch = k_NUM_ITERATIONS;

        oss.str("");
        oss << "d_fracA = " << d_fracA << "\n";
        pattern = oss.str();
        const int numP = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numP, expMatch == numP);

        oss.str("");
        oss << "d_fracB = " << d_fracB << ", ";
        pattern = oss.str();
        const int numP_ = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numP_, expMatch == numP_);

        oss.str("");
        oss << "d_idx = " << d_idx << ", \t" << "d_arf = " << d_arf <<
                                           ", d_fracC = " << d_fracC << "\n" <<
               "d_idx * d_arf * d_fracD = " << d_idx*d_arf*d_fracD << "\n" <<
               "<| To be or not to be -- that is the question. |>\n";
        pattern = oss.str();
        const int numBlock = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numBlock,
                                                         expMatch == numBlock);

        oss.str("");
        oss << PV_(d_woof) << PV_(d_meow) << PV_(d_fracE) << PV(d_fracF) <<
               PE << "): d_meow < d_fracF    (failed)\n";
        pattern = oss.str();
        const int numAV = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numAV, expMatch == numAV);

        oss.str("");
        oss << "There are more things in heaven and earth, Horatio,\n"
               "Than are dreamt of in your philosophy.\n" <<
               d_fracD * d_fracE << bsl::endl;
        pattern = oss.str();
        const int numCout = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numCout,
                                                          expMatch == numCout);

        oss.str("");
        oss << PE << "): d_woof > 2 * d_meow    (failed)\n";
        oss << PV(d_woof) <<
               PE << "): d_woof > 3 * d_meow    (failed)\n";
        pattern = oss.str();
        const int numBA1 = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numBA1, expMatch == numBA1);

        oss.str("");
        oss << PV_(d_woof) << PV(d_meow) <<
               PE << "): d_woof >= 3 * d_meow    (failed)\n";
        oss << PV_(d_arf) << PV_(d_meow) << PV(d_fracA) <<
               PE << "): d_arf + d_meow == 3 * d_fracA    (failed)\n";
        pattern = oss.str();
        const int numBA2 = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numBA2, expMatch == numBA2);

        oss.str("");
        oss << PV_(d_arf) << PV_(d_meow) << PV_(d_fracA) << PV_(d_fracB) <<
               PV(d_fracC) <<
               PE << "): d_arf + d_meow == 3 * d_fracA * d_fracB * d_fracC"
                     "    (failed)\n";
        oss << PV_(d_arf) << PV_(d_meow) << PV_(d_fracA) << PV_(d_fracB) <<
               PV_(d_fracC) << PV(d_fracD) <<
               PE << "): d_arf + d_meow == 3 * d_fracA * d_fracB * d_fracC *"
                     " d_fracD    (failed)\n";
        pattern = oss.str();
        const int numBA3 = outputRedirector_p->numInstances(pattern);
        REALLOOP4_ASSERT(d_idx, pattern, expMatch, numBA3, expMatch == numBA3);

        BSLS_ASSERT(k_NUM_ASSERTS == assertIdx);

        // At this point, this method has counted up all the output created by
        // the 'doOutput' method and replaced what it found with '#'s.  After
        // all the threads have been joined, the main thread will check that
        // the output buffer is 100% '#'s.
#undef PV
#undef PV_
#undef PE
    }
};

struct Func {
    void operator()()
        // Execute one thread of the multi threaded test.
    {
        ThreadData threadData(threadIdx++);
        REAL_ASSERT(0 != outputRedirector_p);

        barrier.wait();
        ++atomicBarrier;
        while (atomicBarrier < k_NUM_THREADS) ;

        threadData.doOutput();

        barrier.wait();

        // main thread is loading the output redirector

        barrier.wait();

        threadData.checkPatterns();
    }
};

}  // close namespace MultiThreadedTest

//=============================================================================
//                                 MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    test                = argc > 1 ? bsl::atoi(argv[1]) : 0;
    verbose             = argc > 2;
    veryVerbose         = argc > 3;
    veryVeryVerbose     = argc > 4;
    veryVeryVeryVerbose = argc > 5;

    bsl::string scratchFileName(&ta);
    {
#if   defined(BSLS_PLATFORM_OS_AIX)
        const char *platform = "aix";
#elif defined(BSLS_PLATFORM_OS_CYGWIN)
        const char *platform = "cygwin";
#elif defined(BSLS_PLATFORM_OS_DARWIN)
        const char *platform = "darwin";
#elif defined(BSLS_PLATFORM_OS_FREEBSD)
        const char *platform = "freebsd";
#elif defined(BSLS_PLATFORM_OS_HPUX)
        const char *platform = "hpux";
#elif defined(BSLS_PLATFORM_OS_LINUX)
        const char *platform = "linux";
#elif defined(BSLS_PLATFORM_OS_SOLARIS)
        const char *platform = "solaris";
#elif defined(BSLS_PLATFORM_OS_SUNOS)
        const char *platform = "sunos";
#elif defined(BSLS_PLATFORM_OS_WINDOWS)
        const char *platform = "windows";
#else
        const char *platform = "other";
#endif

#if defined(BSLS_PLATFORM_CPU_32_BIT)
        const int numBits = 32;
#else
        const int numBits = 64;
#endif

        char buf[1024];
        snprintf(buf, sizeof(buf),
                "tmp.bslmt_testutil.output.case:%d.%s%d.txt",
                test, platform, numBits);
        scratchFileName = buf;
    }
    unlink(scratchFileName.c_str());
    if (veryVerbose) REALP(scratchFileName);

    static const int usageTest = 9;    // test number of usage test case

    // Capture 'stdout' to a file, and send 'stderr' to 'stdout', unless we are
    // running the usage example.

    u::OutputRedirector output(scratchFileName, &ta);
    if (test != usageTest && test != 0) {
        output.redirect();
    }

    bsl::cerr << "TEST " << __FILE__ << " CASE " << test << bsl::endl;

    switch (test) { case 0:  // Zero is always the leading case.
      case 9: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //
        // Concerns:
        //: 1 The usage example provided in the component header file compiles,
        //:   links, and runs as shown.
        //
        // Plan:
        //: 1 Incorporate usage example from header into test driver, remove
        //:   leading comment characters, and replace 'assert' with 'ASSERT'.
        //:   (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "USAGE EXAMPLE\n"
                                  "=============\n";

        REAL_ASSERT(9 == usageTest);

        namespace TC = Usage;

        int rc = TC::testMain();
        REALLOOP2_ASSERT(rc, testStatus, rc == testStatus);

        REALLOOP3_ASSERT(TC::k_NUM_THREADS, TC::k_NUM_ITERATIONS, testStatus,
                       TC::k_NUM_THREADS * TC::k_NUM_ITERATIONS == testStatus);
      } break;
      case 8: {
        // --------------------------------------------------------------------
        // MULTITHREADED TEST
        //
        // Concern:
        //: 1 That the macros all operate properly in a multithreaded context.
        //:
        //: 2 That output from macros not contained in a guarded block are
        //:   always preserved intact in their entirety.
        //:
        //: 3 That output done within a guarded block is preserved intact in
        //:   its entirety.
        //
        // Plan:
        //: 1 Launch 40 threads that will simultaneously use the macros to do
        //:   output, and output directly within guarded blocks, which will be
        //:   captured by 'output'.
        //:
        //: 2 Load the captured output into a buffer.
        //:
        //: 3 Have the threads simultaneously construct all the unique strings
        //:   expected to be found in the output.  Have them call
        //:   'u::OutputRedirector::numInstances' to count the number of
        //:   occurrences of each string and fill their footprints with '#'
        //:   characters.  Verify that the number of occurrences was as
        //:   expected.
        //
        // Testing:
        //   MULTITHREADED TEST
        // --------------------------------------------------------------------


        if (verbose) bsl::cerr << "MULTITHREADED TEST\n"
                                  "==================\n";

        namespace TC = MultiThreadedTest;

        TC::outputRedirector_p = &output;

        bslmt::ThreadGroup tg(&ta);
        tg.addThreads(TC::Func(), TC::k_NUM_THREADS);

        // Threads are initializing their 'ThreadData' objects.

        TC::barrier.wait();

        REALLOOP2_ASSERT(TC::k_NUM_THREADS, TC::threadIdx,
                                           TC::k_NUM_THREADS == TC::threadIdx);

        // Subthreads are all running 'Data::doOutput' concurrently.

        TC::barrier.wait();

        output.load();
        const bsl::size_t  len = output.outputSize();
        char              *buf = output.buffer();

        REALLOOP1_ASSERT(len,
                          TC::k_NUM_THREADS * TC::k_NUM_ITERATIONS * 80 < len);

        bsl::size_t numHashes = bsl::count(buf, buf + len, '#');
        REALLOOP1_ASSERT(numHashes, 0 == numHashes);

        int saveRealTestStatus = realTestStatus;
        if (veryVeryVerbose) REALP(saveRealTestStatus);

        TC::barrier.wait();

        // Subthreads are all running 'Data::checkPatterns', checking the
        // output for matching strings and replacing matched strings with '*'s
        // in the output buffer.  'Data::checkPatterns' is controlled by a
        // mutex, so they only run one at a time.

        tg.joinAll();

        if (veryVeryVerbose && saveRealTestStatus != realTestStatus) {
            bsl::cerr << "Unmatched strings:\n"
                         "=================\n";

            int stringIdx = 0;
            bsl::string s(&ta);
            for (const char *a = output.buffer(), *b, *c; *a; a = c) {
                for (b = a; '#' == *b; ++b) ;
                if (!*b) {
                    break;
                }
                for (c = b + 1; *c && '#' != *c; ++c) ;

                bsl::cerr << "String " << stringIdx++ << bsl::endl;
                s.clear();
                s.insert(s.end(), b, c);
                bsl::cerr << s << bsl::endl;
            }
        }

        REALLOOP2_ASSERT(TC::k_NUM_THREADS, TC::numChecked,
                                      TC::k_NUM_THREADS + 1 == TC::numChecked);

        // The output buffer should be undisturbed, other than being
        // overwritten with '#'s.

        REAL_ASSERT(output.outputSize() == len);
        REAL_ASSERT(output.buffer()     == buf);

        // Check that 100% of the buffer has been overwritten with '#'s.

        numHashes = bsl::count(buf, buf + len, '#');
        REALLOOP2_ASSERT(numHashes, len, numHashes == len);

        if (verbose) {
            REALP_(len);    REALP(numHashes);
        }
      } break;
      case 7: {
        // --------------------------------------------------------------------
        // TESTING 'callFunc' AND 'setFunc' METHODS
        //
        // Concerns:
        //: 1 A custom function with external linkage can be installed and
        //:   later called via 'bslmt' test utility component.
        //
        // Plan:
        //: 1 Install custom function using 'setFunc' method.
        //:
        //: 2 Invoke previously installed function and verify the result of
        //:   its invocation.
        //
        // Testing:
        //   void *callFunc(void *arg);
        //   void setFunc(Func func);
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "TESTING 'callFunc' AND 'setFunc' METHODS\n"
                                  "========================================\n";

        using namespace BSLMT_TESTUTIL_TEST_FUNCTION_CALL;

        REAL_ASSERT(0 == callCount);

        Obj::setFunc(testFunctionAdd);

        REAL_ASSERT(0 == callCount);

        int   refValue;
        void *INPUT = &refValue;
        void *result = Obj::callFunc(INPUT);

        REAL_ASSERT(1 == callCount);
        REAL_ASSERT(&refValue == result);

        result = Obj::callFunc(INPUT);

        REAL_ASSERT(2 == callCount);
        REAL_ASSERT(&refValue == result);

        Obj::setFunc(testFunctionSub);

        REAL_ASSERT(2 == callCount);

        result = Obj::callFunc(INPUT);

        REAL_ASSERT(1 == callCount);
        REAL_ASSERT(&refValue == result);

        result = Obj::callFunc(INPUT);

        REAL_ASSERT(0 == callCount);
        REAL_ASSERT(&refValue == result);
      } break;
      case 6: {
        // --------------------------------------------------------------------
        // BSLMT_TESTUTIL_LOOP*_ASSERT AND BSLMT_ASSERTV MACROS
        //
        // Concerns:
        //: 1 Macros do not call 'aSsErT' and emit no output when the assertion
        //:   is 'true'.
        //:
        //: 2 Macros call 'aSsErT' and emit output each time the assertion is
        //:   'false'.
        //:
        //: 3 Macros emit properly formatted output for each loop variable
        //:   supplied.
        //
        // Plan:
        //: 1 Loop through an arbitrary number of iterations, calling one of
        //:   the loop assert macros with distinct values for each loop
        //:   variable and an assertion that evaluates to 'true'.  Confirm that
        //:   the value of 'testStatus' does not change, and that no output is
        //:   captured by the output redirection apparatus.  (C-1)
        //:
        //: 2 Loop through an arbitrary number of iterations, calling one of
        //:   the loop assert macros with distinct values for each loop
        //:   variable and an assertion that evaluates to 'false'.  Confirm
        //:   that 'testStatus' increments each time the loop assert macro is
        //:   called, and that the expected error output is captured by the
        //:   output redirection apparatus.  Note that using distinct values
        //:   for each loop variable allows us to detect omissions,
        //:   repetitions, or mis-ordering of the loop assert macro's
        //:   arguments.  Also note that we test the loop assert macro with
        //:   only one set of variable types, since we test separately in test
        //:   case 3 the ability of the underlying apparatus to identify and
        //:   correctly format each primitive type.  (C-2..3)
        //
        // Testing:
        //   BSLMT_TESTUTIL_ASSERT(X)
        //   BSLMT_TESTUTIL_LOOP0_ASSERT(X)
        //   BSLMT_TESTUTIL_LOOP1_ASSERT(I,X)
        //   BSLMT_TESTUTIL_LOOP2_ASSERT(I,J,X)
        //   BSLMT_TESTUTIL_LOOP3_ASSERT(I,J,K,X)
        //   BSLMT_TESTUTIL_LOOP4_ASSERT(I,J,K,L,X)
        //   BSLMT_TESTUTIL_LOOP5_ASSERT(I,J,K,L,M,X)
        //   BSLMT_TESTUTIL_LOOP6_ASSERT(I,J,K,L,M,N,X)
        //   BSLMT_TESTUTIL_ASSERTV(...)
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr <<
                      "BSLMT_TESTUTIL_LOOP*_ASSERT AND BSLMT_ASSERTV MACROS\n"
                      "====================================================\n";

        enum {
            BUFFER_SIZE    = 1024,  // size of the buffer used to store
                                    // captured output

            NUM_ITERATIONS =   10   // number of iterations to use when testing
                                    // loop assert macros
        };

        static char s_expectedOutput[BUFFER_SIZE];  // scratch area for
                                                    // assembling model output
                                                    // that will be compared to
                                                    // real output captured
                                                    // from 'stdout'

        // 'BSLMT_TESTUTIL_ASSERT(X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_ASSERT(idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(idx < NUM_ITERATIONS);

                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERT(idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "Error %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP0_ASSERT(X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP0_ASSERT(idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(idx < NUM_ITERATIONS);

                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_LOOP0_ASSERT(idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "Error %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP1_ASSERT(I,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP1_ASSERT(I, idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, idx < NUM_ITERATIONS);

                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_LOOP1_ASSERT(I, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP2_ASSERT(I,J,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP2_ASSERT(I, J, idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, J,idx < NUM_ITERATIONS);
                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_LOOP2_ASSERT(I, J, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, J, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP3_ASSERT(I,J,K,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP3_ASSERT(I, J, K,
                                            idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, J, K, idx < NUM_ITERATIONS);
                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_LOOP3_ASSERT(I, J, K, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, J, K, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP4_ASSERT(I,J,K,L,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP4_ASSERT(I, J, K, L,
                                            idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, idx < NUM_ITERATIONS);
                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_LOOP4_ASSERT(I, J, K, L, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\nError %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP5_ASSERT(I,J,K,L,M,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP5_ASSERT(I, J, K, L, M,
                                            idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, M, idx < NUM_ITERATIONS);
                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 2;
                BSLMT_TESTUTIL_LOOP5_ASSERT(I, J, K, L, M,
                                            idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\tM: %d\nError"
                         " %s(%d): idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L, M,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;

                if (veryVerbose) { REALP(idx); }

                output.reset();

                // On AIX, the line number printed from the
                // 'BSLMT_TESTUTIL_ASSERTV' macro is the line number of the
                // first line of the call statement, even if the statement is
                // split over multiple lines.  This behavior is different from
                // the equivalent loop-assert alternative, which prints the
                // last line of the statement.  The behavior of 'ASSERTV' and
                // regular loop-assert is consistent on other platforms.  So
                // here we make sure that the call to 'BSLMT_TESTUTIL_ASSERTV'
                // fits on a single line to ensure that the output is the same
                // on all platforms.

                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, M, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\tM: %d\nError"
                         " %s(%d): idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L, M,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }

        // 'BSLMT_TESTUTIL_LOOP6_ASSERT(I,J,K,L,M,N,X)'
        {
            REAL_ASSERT(0 == testStatus);
            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;
                const int N = idx + 5;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                BSLMT_TESTUTIL_LOOP6_ASSERT(I, J, K, L, M, N,
                                            idx < NUM_ITERATIONS);
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, M, N, idx < NUM_ITERATIONS);
                REALLOOP1_ASSERT(testStatus, 0 == testStatus);
                REAL_ASSERT(output.load());
                REALLOOP1_ASSERT(output.buffer(), 0 == output.compare(""));
            }
            REAL_ASSERT(0 == testStatus);

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;
                const int N = idx + 5;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 2;
                BSLMT_TESTUTIL_LOOP6_ASSERT(I, J, K, L, M, N,
                                            idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\tM: %d\tN: %d\n"
                         "Error %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L, M, N,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;

            // Repeat for 'ASSERTV'.

            for (int idx = 0; idx < NUM_ITERATIONS; ++idx) {
                const int I = idx;
                const int J = idx + 1;
                const int K = idx + 2;
                const int L = idx + 3;
                const int M = idx + 4;
                const int N = idx + 5;

                if (veryVerbose) { REALP(idx); }

                output.reset();
                const int LINE = __LINE__ + 1;
                BSLMT_TESTUTIL_ASSERTV(I, J, K, L, M, N, idx > NUM_ITERATIONS);
                REALLOOP2_ASSERT(testStatus, idx, testStatus == idx + 1);
                REAL_ASSERT(output.load());
                snprintf(s_expectedOutput,
                         BUFFER_SIZE,
                         "I: %d\tJ: %d\tK: %d\tL: %d\tM: %d\tN: %d\n"
                         "Error %s(%d):"
                         " idx > NUM_ITERATIONS    (failed)\n",
                         I, J, K, L, M, N,
                         __FILE__,
                         LINE);
                REALLOOP2_ASSERT(s_expectedOutput,
                                 output.buffer(),
                                 0 == output.compare(s_expectedOutput));
            }
            REAL_ASSERT(NUM_ITERATIONS == testStatus);
            testStatus = 0;
        }
      } break;
      case 5: {
        // --------------------------------------------------------------------
        // 'Q' (IDENTIFIER OUTPUT MACRO)
        //
        // Concerns:
        //: 1 The identifier output macro emits the correct output,
        //:   '<| [macro argument] |>', where '[macro argument]' is the
        //:   tokenization of the text supplied as an argument to the macro.
        //
        // Plan:
        //: 1 Call 'BSLMT_TESTUTIL_Q' with a series of arbitrary
        //:   identifiers containing single and multiple tokens, with and
        //:   without initial, final, and repeated whitespace and compare each
        //:   captured output to the expected output.  (C-1)
        //
        // Testing:
        //   BSLMT_TESTUTIL_Q(X)
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "'Q' (IDENTIFIER OUTPUT MACRO)\n"
                                  "=============================\n";
        {
            output.reset();
            BSLMT_TESTUTIL_Q(sample);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare("<| sample |>\n"));
        }

        {
            output.reset();
            BSLMT_TESTUTIL_Q(embedded white   space);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare("<| embedded white space |>\n"));
        }

        {
            output.reset();
            BSLMT_TESTUTIL_Q(   initial whitespace);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare("<| initial whitespace |>\n"));
        }

        {
            output.reset();
            BSLMT_TESTUTIL_Q(final whitespace   );
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare("<| final whitespace |>\n"));
        }
      } break;
      case 4: {
        // --------------------------------------------------------------------
        // 'P' AND 'P_' (VALUE OUTPUT MACROS)
        //
        // Concerns:
        //: 1 The value output macros emit output.
        //:
        //: 2 Output emitted is in the correct format for the standard 'P' and
        //:   'P_' macros, i.e., 'identifier = value' (with following newline
        //:   in the case of 'BSLMT_TESTUTIL_P') where
        //:   'identifier' is the name of the argument supplied to the macro,
        //:   and 'value' is the value of that argument.
        //
        // Plan:
        //: 1 Call the value output macros on a variable of known value, and
        //:   confirm that the captured output is in the correct format.  Note
        //:   that it is only necessary to conduct this test once with a single
        //:   variable type because the underlying type-differentiation and
        //:   formatting mechanisms are handled by the input operators.
        //:   (C-1..2)
        //
        // Testing:
        //   BSLMT_TESTUTIL_P(X)
        //   BSLMT_TESTUTIL_P_(X)
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "'P' AND 'P_' (VALUE OUTPUT MACROS)\n"
                                  "==================================\n";

        // 'BSLMT_TESTUTIL_P(X)'
        {
            const int INPUT = 42;

            output.reset();
            BSLMT_TESTUTIL_P(INPUT);
            REAL_ASSERT(output.load());
            REALLOOP1_ASSERT(output.buffer(),
                             0 == output.compare("INPUT = 42\n"));
        }

        // 'BSLMT_TESTUTIL_P_(X)'
        {
            const int INPUT = 42;

            output.reset();
            BSLMT_TESTUTIL_P_(INPUT);
            REAL_ASSERT(output.load());
            REALLOOP1_ASSERT(output.buffer(),
                             0 == output.compare("INPUT = 42, "));
        }
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // 'L_' AND 'T_' (STATIC MACROS)
        //
        // Concerns:
        //: 1 The line number macro has the correct value.
        //:
        //: 2 The tab output macro emits output.
        //:
        //: 3 The output of the tab output macro is emitted in the correct
        //:   format.
        //
        // Plan:
        //: 1 Compare the value of the line number macro to '__LINE__'.  (C-1)
        //:
        //: 2 Call the tab output macro and confirm that the captured output
        //:   is in the correct format.  (C-2..3)
        //
        // Testing:
        //   BSLMT_TESTUTIL_L_
        //   BSLMT_TESTUTIL_T_
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "'L_' AND 'T_' (STATIC MACROS)\n"
                                  "=============================\n";

        // 'BSLMT_TESTUTIL_L_'
        {
            // Line spacing is significant as it assures
            // 'BSLMT_TESTUTIL_L_' is not a simple sequence.

            REAL_ASSERT(__LINE__ == BSLMT_TESTUTIL_L_);


            REAL_ASSERT(__LINE__ == BSLMT_TESTUTIL_L_);
            REAL_ASSERT(__LINE__ == BSLMT_TESTUTIL_L_);



            REAL_ASSERT(__LINE__ == BSLMT_TESTUTIL_L_);

            REAL_ASSERT(__LINE__ == BSLMT_TESTUTIL_L_);
        }

        // 'BSLMT_TESTUTIL_T_'
        {
            output.reset();
            BSLMT_TESTUTIL_T_;
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare("\t"));
        }
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TEST APPARATUS
        //
        // Concerns:
        //: 1 Output is redirected.
        //:
        //: 2 Captured output is readable.
        //:
        //: 3 'load' works.
        //:
        //: 4 'reset' works.
        //:
        //: 5 'compare' works.
        //:
        //: 6 Incorrect output is correctly diagnosed.
        //:
        //: 7 Embedded newlines work.
        //:
        //: 8 Empty output works.
        //:
        //: 9 Embedded null characters work.
        //:
        //:10 Filesystem-dependent control sequences work.
        //:
        //:11 'stderr' points to the original target of 'stdout'.
        //
        // Plan:
        //: 1 Confirm that 'ftell(stdout)' succeeds.  This demonstrates that
        //:   'stdout' is a seekable file.  (C-1)
        //:
        //: 2 Write a string to 'stdout' and confirm that 'stdout's seek
        //:   position has changed; read back the contents of 'stdout' and
        //:   compare them to the original string.  (C-2)
        //:
        //: 3 Write a string to 'stdout'.  Confirm that
        //:   'OutputRedirector::load' changes the contents of the output
        //:   buffer and that it changes the result of
        //:   'OutputRedirector::isOutputReady' from 'false' to 'true'.
        //:   Confirm that the contents of the output buffer match the
        //:   original string.  (C-3)
        //:
        //: 4 Write a string to 'stdout' and load it with
        //:   'OutputRedirector::load'.  Confirm that
        //:   'OutputRedirector::reset' rewinds 'stdout', changes the output
        //:   of 'OutputRedirector::isOutputReady' from 'true' to 'false', and
        //:   sets the length of the output buffer to 0.  (C-4)
        //:
        //: 5 Write a string to 'stdout' and load it with
        //:   'OutputRedirector::load'.  Confirm that
        //:   'OutputRedirector::compare' gives the correct results when the
        //:   captured output is compared with the following data:  (C-5)
        //:
        //:       Data                           Comparison Result
        //:   ------------                   -------------------------
        //:   input string                           true
        //:   input string with appended data        false
        //:   input string truncated                 false
        //:   string different from input:
        //:     at beginning                         false
        //:     at end                               false
        //:     elsewhere                            false
        //:
        //: 6 Confirm that 'load' fails when there is more data in 'stdout'
        //:   than can fit in the capture buffer.  Confirm that 'compare' fails
        //:   if 'load' has not been first called to read data into the capture
        //:   buffer.  (C-6)
        //:
        //: 7 Confirm that strings containing embedded newlines are correctly
        //:   captured and correctly identified by 'compare'.  (C-7)
        //:
        //: 8 Write an empty string to 'stdout'.  Confirm that it can be
        //:   correctly loaded and compared with the original.  (C-8)
        //:
        //: 9 Write a series of strings to 'stdout' containing '\0' at the
        //:   beginning, end, or interior of the string.  Confirm that the
        //:   captured output can be correctly loaded and compared with the
        //:   original input.  (C-9)
        //:
        //:10 Write a series of strings to 'stdout' containing '^D' and
        //:   '<CRLF>' and confirm that these strings are correctly captured
        //:   and loaded.  (C-10)
        //:
        //:11 Use 'fstat' to find out the device and inode of the current
        //:   (post-redirection) 'stderr'.  Compare these values to the device
        //:   and inode of 'stdout' before redirection.  (C-11)
        //
        // Testing:
        //   TEST APPARATUS
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "TEST APPARATUS\n"
                                  "==============\n";

        {
            // 1) Output is redirected.

            REAL_ASSERT(-1 != ftell(stdout));
        }

        enum { OUTPUT_BUFFER_SIZE = 1024 * 1024 };

        {
            // 2) Captured output is readable.

            enum { TEST_STRING_SIZE = 15 };
            const char *testString = "This is output";
            char        buffer[TEST_STRING_SIZE];

            REAL_ASSERT(TEST_STRING_SIZE == bsl::strlen(testString) + 1);

            rewind(stdout);
            long initialStdoutPosition = ftell(stdout);
            REAL_ASSERT(0 == initialStdoutPosition);
            bsl::printf("%s", testString);
            long finalStdoutPosition = ftell(stdout);
            REAL_ASSERT(-1 != finalStdoutPosition);
            REAL_ASSERT(finalStdoutPosition > initialStdoutPosition);
            long outputSize = finalStdoutPosition - initialStdoutPosition;
            REAL_ASSERT(outputSize + 1 == TEST_STRING_SIZE);
            rewind(stdout);
            bsl::size_t bytesWritten =
                               fread(buffer, sizeof(char), outputSize, stdout);
            REAL_ASSERT(static_cast<long>(bytesWritten) == outputSize);
            buffer[TEST_STRING_SIZE - 1] = '\0';
            REAL_ASSERT(0 == bsl::strcmp(testString, buffer));
        }

        {
            // 3) 'load' works.

            const char  *testString       = "This is output";
            bsl::size_t  testStringLength = bsl::strlen(testString);

            rewind(stdout);
            bsl::printf("%s", testString);
            REAL_ASSERT(static_cast<long>(testStringLength) == ftell(stdout));
            REAL_ASSERT(false == output.isOutputReady());
            REAL_ASSERT(0 == output.outputSize());
            REAL_ASSERT(output.load());
            REAL_ASSERT(static_cast<long>(testStringLength) == ftell(stdout));
            REAL_ASSERT(true == output.isOutputReady());
            REAL_ASSERT(testStringLength == output.outputSize());
            REAL_ASSERT(0 == bsl::memcmp(testString,
                                         output.buffer(),
                                         output.outputSize()));
        }

        {
            // 4) 'reset' works.

            const char  *testString       = "This is output";
            bsl::size_t  testStringLength = bsl::strlen(testString);

            rewind(stdout);
            bsl::printf("%s", testString);
            output.load();
            REAL_ASSERT(static_cast<long>(testStringLength) == ftell(stdout));
            REAL_ASSERT(true == output.isOutputReady());
            REAL_ASSERT(testStringLength == output.outputSize());
            output.reset();
            REAL_ASSERT(0 == ftell(stdout));
            REAL_ASSERT(true != output.isOutputReady());
            REAL_ASSERT(0 == output.outputSize());
        }

        {
            // 5) 'compare' works.

            const char *testString            = "This is output";
            const char *longString            = "This is outputA";
            const char *shortString           = "This is outpu";
            const char *differentStartString  = "Xhis is output";
            const char *differentEndString    = "This is outpuy";
            const char *differentMiddleString = "This iz output";

            output.reset();
            bsl::printf("%s", testString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(!!bsl::strcmp(testString, testString) ==
                                                 !!output.compare(testString));

            REAL_ASSERT(!!bsl::strcmp(testString, shortString) ==
                                                !!output.compare(shortString));
            REAL_ASSERT(!!bsl::strcmp(testString, longString) ==
                                                 !!output.compare(longString));
            REAL_ASSERT(!!bsl::strcmp(testString, differentStartString) ==
                                       !!output.compare(differentStartString));
            REAL_ASSERT(!!bsl::strcmp(testString, differentEndString) ==
                                         !!output.compare(differentEndString));
            REAL_ASSERT(!!bsl::strcmp(testString, differentMiddleString) ==
                                      !!output.compare(differentMiddleString));
        }

        {
            // 6) Incorrect output is correctly diagnosed.

            // Reset verbosity levels to suppress expected error output.
            const bool tempVeryVerbose     = veryVerbose;
            const bool tempVeryVeryVerbose = veryVeryVerbose;
            veryVerbose     = false;
            veryVeryVerbose = false;

            const char *testString = "This is good output";

            output.reset();
            bsl::printf("%s", testString);
            REAL_ASSERT(0 != output.compare(testString));

            output.reset();
            REAL_ASSERT(0 != output.compare("", 0));

            veryVerbose     = tempVeryVerbose;
            veryVeryVerbose = tempVeryVeryVerbose;
        }

        {
            // 7) Embedded newlines work.

            const char *testString = "This has an\nembedded newline";

            output.reset();
            bsl::printf("%s", testString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(testString));

            const char *twoNewlineTestString =
                "This has two\nembedded newlines\n";

            output.reset();
            bsl::printf("%s", twoNewlineTestString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(twoNewlineTestString));
        }

        {
            // 8) Empty output works.

            const char *testString = "";

            output.reset();
            bsl::printf("%s", testString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(testString));
        }

        {
            // 9) Embedded null characters work.

            const char *testString1 = "abc\0def";
            const char *testString2 = "\0def";
            const char *testString3 = "abc\0";

            output.reset();
            fwrite(testString1, sizeof(char), 7, stdout);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(testString1, 7));

            output.reset();
            fwrite(testString2, sizeof(char), 4, stdout);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(testString2, 4));

            output.reset();
            fwrite(testString3, sizeof(char), 4, stdout);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(testString3, 4));
        }

        {
            // 10) Filesystem-dependent control sequences work.

            const char *crnlTestString = "ab\r\ncd";
            const char *ctrlDTestString = "ab" "\x04" "cd";

            output.reset();
            bsl::printf("%s", crnlTestString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(crnlTestString));

            output.reset();
            bsl::printf("%s", ctrlDTestString);
            REAL_ASSERT(output.load());
            REAL_ASSERT(0 == output.compare(ctrlDTestString));
        }

        {
            // 11) 'stderr' points to the original target of 'stdout'.

            int newStderrFD = fileno(stderr);
            REAL_ASSERT(-1 != newStderrFD);

#if !defined(BSLS_PLATFORM_OS_WINDOWS) && !defined(BSLS_PLATFORM_OS_AIX)
            struct stat stderrStat;
            REAL_ASSERT(-1 != fstat(newStderrFD, &stderrStat));
            // The 'stat' fields only seem to be well-preserved when output was
            // re-directed via 'freopen' and not necessary when it's redirected
            // through forms of 'dup2'.

            REALLOOP2_ASSERT(stderrStat.st_dev,
                                           output.originalStdoutStat().st_dev,
                      stderrStat.st_dev == output.originalStdoutStat().st_dev);
            REALLOOP2_ASSERT(stderrStat.st_rdev,
                                          output.originalStdoutStat().st_rdev,
                    stderrStat.st_rdev == output.originalStdoutStat().st_rdev);
            REALLOOP2_ASSERT(stderrStat.st_ino,
                                           output.originalStdoutStat().st_ino,
                      stderrStat.st_ino == output.originalStdoutStat().st_ino);
#endif
        }
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //   This case exercises (but does not fully test) basic functionality.
        //
        // Concerns:
        //: 1 The macros are sufficiently functional to enable comprehensive
        //:   testing in subsequent test cases.
        //
        // Plan:
        //: 1 Perform an ad-hoc test of the macros.  (C-1)
        //
        // Testing:
        //   BREATHING TEST
        // --------------------------------------------------------------------

        if (verbose) bsl::cerr << "BREATHING TEST\n"
                                  "==============\n";

        const bool    b = true;
        const char    c = 'c';
        const int     i = 123;
        const double  d = 123.56;
        const float   f = 789.01f;
        const char   *s = "hello";

        if (verbose) {
            BSLMT_TESTUTIL_Q(BSLMT_TESTUTIL_L_);
            BSLMT_TESTUTIL_P_(b);
            BSLMT_TESTUTIL_T_;
            BSLMT_TESTUTIL_P_(c);
            BSLMT_TESTUTIL_T_;
            BSLMT_TESTUTIL_P(i);

            testStatus = 0;
            BSLMT_TESTUTIL_ASSERT(!b);
            REAL_ASSERT(testStatus == 1);

            testStatus = 0;
            BSLMT_TESTUTIL_LOOP0_ASSERT(!b);
            BSLMT_TESTUTIL_LOOP1_ASSERT(b, !b);
            BSLMT_TESTUTIL_LOOP2_ASSERT(b, c, !b);
            BSLMT_TESTUTIL_LOOP3_ASSERT(b, c, i, !b);
            BSLMT_TESTUTIL_LOOP4_ASSERT(b, c, i, d, !b);
            BSLMT_TESTUTIL_LOOP5_ASSERT(b, c, i, d, f, !b);
            BSLMT_TESTUTIL_LOOP6_ASSERT(b, c, i, d, f, s, !b);
            REAL_ASSERT(testStatus == 7);

            testStatus = 0;
            BSLMT_TESTUTIL_ASSERTV(!b);
            BSLMT_TESTUTIL_ASSERTV(b, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, f, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, f, s, !b);
            REAL_ASSERT(testStatus == 7);
        }

        // Ensure that all the above macros still work with namespace
        // 'BloombergLP' in scope.

        if (verbose) {
            using namespace BloombergLP;

            BSLMT_TESTUTIL_Q(BSLMT_TESTUTIL_L_);
            BSLMT_TESTUTIL_P_(b);
            BSLMT_TESTUTIL_T_;
            BSLMT_TESTUTIL_P_(c);
            BSLMT_TESTUTIL_T_;
            BSLMT_TESTUTIL_P(i);

            testStatus = 0;
            BSLMT_TESTUTIL_LOOP0_ASSERT(!b);
            BSLMT_TESTUTIL_LOOP1_ASSERT(b, !b);
            REAL_ASSERT(testStatus == 2);

            testStatus = 0;
            BSLMT_TESTUTIL_LOOP0_ASSERT(!b);
            BSLMT_TESTUTIL_LOOP1_ASSERT(b, !b);
            BSLMT_TESTUTIL_LOOP2_ASSERT(b, c, !b);
            BSLMT_TESTUTIL_LOOP3_ASSERT(b, c, i, !b);
            BSLMT_TESTUTIL_LOOP4_ASSERT(b, c, i, d, !b);
            BSLMT_TESTUTIL_LOOP5_ASSERT(b, c, i, d, f, !b);
            BSLMT_TESTUTIL_LOOP6_ASSERT(b, c, i, d, f, s, !b);
            REAL_ASSERT(testStatus == 7);

            testStatus = 0;
            BSLMT_TESTUTIL_ASSERTV(!b);
            BSLMT_TESTUTIL_ASSERTV(b, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, f, !b);
            BSLMT_TESTUTIL_ASSERTV(b, c, i, d, f, s, !b);
            REAL_ASSERT(testStatus == 7);
        }
      } break;
      default: {
        bsl::fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        realTestStatus = -1;
      }
    }

    if (realTestStatus > 0) {
        bsl::fprintf(stderr,
                     "Error, non-zero test status = %d.\n",
                     realTestStatus);
    }
    return realTestStatus;
}

// ----------------------------------------------------------------------------
// Copyright 2018 Bloomberg Finance L.P.
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
