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
#ifdef __SWITCH__
#include <switch.h>
#include <inttypes.h>
#include <pthread.h>
#endif
#ifdef __3DS__
#include <3ds.h>
#include <inttypes.h>
#include <pthread.h>
#elif __NDS__
#include <nds.h>
#include <inttypes.h>
#include <pthread.h>
#endif
#ifdef __VITA__
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/clib.h>
#endif

typedef void *( * pthread_fn )( void * );

#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX) || defined(__VITA__) || defined(__SWITCH__) || defined(__3DS__) || defined(__NDS__)
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
		CThread( pthread_fn threadFunc, void* threadParam );

		virtual ~CThread();
		
		static void sleep( const unsigned int millis );
	
	private:
	#ifdef WIN32
		LPTHREAD_START_ROUTINE		mp_threadFunc;
		DWORD						m_threadID;
		HANDLE						m_threadHandle;
	#endif
	#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX) || defined(__SWITCH__) || defined(__3DS__) || defined(__NDS__)
		pthread_fn					mp_threadFunc;
		pthread_t					m_thread;
		pthread_attr_t				m_attributes;
	#endif
	#ifdef MACOSX
		TaskProc					mp_threadFunc;
		MPTaskID					m_threadID;
	#endif
	#ifdef __VITA__
		SceUID m_thread;
	#endif
	
	};



#endif // _OX_CORE_CTHREAD_H_
