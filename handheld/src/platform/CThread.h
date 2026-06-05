/*
 *  CThread.h
 *  Created by aegzorz on 2007-02-09.
 *  Copyright 2007 Oxeye. All rights reserved.
 */

#ifndef _OX_CORE_CTHREAD_H_
#define _OX_CORE_CTHREAD_H_

#ifdef WIN32
	#include <windows.h>
#endif

#ifdef __VITA__
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/clib.h>
#endif

#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX) || defined(__VITA__)
	#include <pthread.h>
	#include <unistd.h>

#endif
#ifdef MACOSX
	#include <CoreServices/CoreServices.h>
	#include <unistd.h>
#endif

class CThread
{
public:
	typedef void* (*pthread_fn)(void*);

	CThread(pthread_fn threadFunc, void* threadParam );
	virtual ~CThread();
#ifndef WIN32
	CThread(const CThread&) = delete;
    CThread& operator=(const CThread&) = delete;

	CThread(CThread&&) = delete;
    CThread& operator=(CThread&&) = delete;
#endif

	static void sleep( const unsigned int millis );

private:
	bool m_started;

#if defined(WIN32)
    void* m_threadHandle;
    unsigned long m_threadID;
    void* (*mp_threadFunc)(void*); 
#elif defined(__VITA__)
    int m_thread;
    void* m_vitaArgs[2];
#elif defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
    unsigned long m_thread;
#endif

};



#endif // _OX_CORE_CTHREAD_H_
