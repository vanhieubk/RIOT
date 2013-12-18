/*
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */
 
/**
 * @addtogroup  core_util
 * @{
 *
 * @file        debug.h
 * @brief       Debug-header
 *
 * #define ENABLE_DEBUG, include this and then use DEBUG as printf you can toggle.
 *
 * @author      Freie Universität Berlin, Computer Systems & Telematics
 * @author      Kaspar Schleiser <kaspar.schleiser@fu-berlin.de>
 */
 
#ifndef __DEBUG_H
#define __DEBUG_H
 
#include <stdio.h>

#if ENABLE_DEBUG

#ifdef HAVE_VALGRIND_H  // TODO: && NATIVE
#include <valgrind.h>
#define VG_DEBUG 1
#elif defined(HAVE_VALGRIND_VALGRIND_H)
#include <valgrind/valgrind.h>
#define VG_DEBUG 1
#endif


#if VG_DEBUG
// use VALGRIND_PRINTF_BACKTRACE if running in valgrind
#define DEBUG(...) if (RUNNING_ON_VALGRIND) VALGRIND_PRINTF_BACKTRACE(__VA_ARGS__); else printf(__VA_ARGS__);
#else // ENABLE_DEBUG and not valgrind
#define DEBUG(...) printf(__VA_ARGS__)
#endif

#undef ENABLE_DEBUG

#else // not ENABLE_DEBUG
#define DEBUG(...)
#endif


/** @} */
#endif /* __DEBUG_H */
