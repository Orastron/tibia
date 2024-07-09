#ifndef FATICA_H
#define FATICA_H

// API

// unit = 100-nanosecond starting from somewhen
unsigned long long fatica_time_process(void);

// Implementation

#if defined(_WIN32) || defined(__CYGWIN__)

# include <windows.h>

static ULONGLONG filetime_to_ull(const FILETIME* ft) {
	return (((ULONGLONG)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
}

unsigned long long fatica_time_process(void) {
	FILETIME creationTime, exitTime, kernelTime, userTime;
	const DWORD threadId = GetCurrentThreadId();
	const HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadId);
	if (!GetThreadTimes(hThread, &creationTime, &exitTime, &kernelTime, &userTime))
		return 0ull;
	CloseHandle(hThread);
	return (unsigned long long) (filetime_to_ull(&kernelTime) + filetime_to_ull(&userTime));
}

#elif defined(__linux__)

# if __STDC_VERSION__ >= 199901L
#  define _XOPEN_SOURCE 600
# else
#  define _XOPEN_SOURCE 500
# endif
# include <time.h>
# include <unistd.h>

unsigned long long fatica_time_process(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0)
		return 0ull;
	return (unsigned long long) (ts.tv_sec * 1e7 + ts.tv_nsec / 1e2);
}

#elif defined(__APPLE__)

# include <unistd.h>
# include <mach/mach.h>
# include <mach/thread_act.h>

unsigned long long fatica_time_process(void) {
	thread_t thread = mach_thread_self();
	thread_basic_info_data_t info;
	mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
    if (thread_info(thread, THREAD_BASIC_INFO, (thread_info_t) &info, &count) != KERN_SUCCESS)
        return 0ull;
    return (info.user_time.seconds + info.system_time.seconds) * 1000 
    	+ (info.user_time.microseconds + info.system_time.microseconds) / 1000;
}

#else
# error "System not supported"
#endif
#endif // FATICA_H
