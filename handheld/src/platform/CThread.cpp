/*
 *  CThread.cpp
 *  oxeye
 *
 *  Created by aegzorz on 2007-02-09.
 *  Copyright 2007 Mojang AB. All rights reserved.
 *
 */

#include "CThread.h"

#if defined(__VITA__)
static int vita_thread_entry(SceSize argc, void* argv) {
	void** args = (void**)argv;
	((void(*)(void*))args[0])(args[1]);
	return 0;
}
#endif


CThread::CThread( pthread_fn threadFunc, void* threadParam )
	: m_started(false),
#if defined(WIN32)
	  mp_threadFunc(nullptr),
	  m_threadHandle(0)
#elif defined(__VITA__)
	  m_thread(-1),
	  m_vitaArgs(nullptr)
#else
	  m_thread(0)
#endif
{
#ifdef WIN32
	mp_threadFunc = threadFunc;
	m_threadHandle = CreateThread(
		NULL,				// pointer to security attributes
		NULL,               // initial thread stack size
		(LPTHREAD_START_ROUTINE)threadFunc,		// pointer to thread function
		threadParam,        // argument for new thread
		NULL,               // creation flags
		&m_threadID        // pointer to receive thread ID
	);
	m_started = (m_threadHandle != NULL);
#elif defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
	pthread_attr_t attributes;
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
	int result = pthread_create(&m_thread, &attributes, (pthread_fn)threadFunc, threadParam);
	pthread_attr_destroy(&attributes);
	m_started = (result == 0);
#elif defined(__VITA__)
	m_thread = sceKernelCreateThread("CThread", vita_thread_entry, 0x10000100, 2 * 1024 * 1024, 0, 0, NULL);
	void* args[2] = {(void*)threadFunc, threadParam};
	sceKernelStartThread(m_thread, 8, args);
	m_started = true;
#endif
}

void CThread::sleep( const unsigned int millis )
{
	#ifdef WIN32
		Sleep( millis );
	#endif
	#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
		usleep(millis * 1000);
	#endif
	#if defined(__VITA__)
		sceKernelDelayThread(millis * 1000);
	#endif
}

CThread::~CThread()
{
	if (!m_started) return;
#ifdef WIN32
	TerminateThread(m_threadHandle, 0); 
	CloseHandle(m_threadHandle);
#elif defined(__VITA__)
	// nothind to do
#elif defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
	void* retval = nullptr;   
	pthread_join(m_thread, &retval);
#endif
}


