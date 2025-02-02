/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#pragma once

#include "hphp/util/thread-local.h"

namespace HPHP::jit {

struct Vout;
struct Vreg;

///////////////////////////////////////////////////////////////////////////////

/*
 * Thread-local variable abstraction.
 *
 * This serves as a typed wrapper for the thread-local variables accepted by
 * the emit routines below.
 *
 * It exists in order to provide some encapsulation for the tricky macrology
 * required on x64 OSX to obtain the memory locations of thread locals.
 */
template<typename T>
struct TLSDatum {
  explicit TLSDatum(const T& var) : tls{&var} {}
  explicit TLSDatum(const long* addr) : raw{addr} {}

  union {
    const T* tls;
    const long* raw;
  };
};

/*
 * See code-gen-tls-x64.h for an explanation of this assembly.
 */
#define tls_datum(var) ([] {                \
  long* ret;                                \
  __asm__("lea %1, %%rax\nmov %%rdi, %0" :  \
          "=r"(ret) : "m"(var));            \
  return TLSDatum<decltype(var)>(ret);      \
}())

///////////////////////////////////////////////////////////////////////////////

/*
 * Return the Vptr for the location of a __thread variable `datum'.
 */
template<typename T>
Vptr emitTLSAddr(Vout& v, TLSDatum<T> datum);

/*
 * Like emitTLSAddr, but turns the address into a Vreg. You cannot necessarily
 * just use a lea on the output of emitTLSAddr because of potential segment
 * register issues.
 */
template<typename T>
Vreg emitTLSLea(Vout& v, TLSDatum<T> datum, int offset = 0);

/*
 * Load the value of the ThreadLocalNoCheck `datum' into `d'.
 */
template<typename T>
void emitTLSLoad(Vout& v, TLSDatum<ThreadLocalNoCheck<T>> datum, Vreg d);

///////////////////////////////////////////////////////////////////////////////

}

#include "hphp/runtime/vm/jit/code-gen-tls-x64.h"
#include "hphp/runtime/vm/jit/code-gen-tls-arm.h"

// This has to follow all the arch-specific includes.
#include "hphp/runtime/vm/jit/code-gen-tls-inl.h"
