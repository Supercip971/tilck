# SPDX-License-Identifier: BSD-2-Clause
cmake_minimum_required(VERSION 3.2)

function (show_wrong_arch_error)

   set(msg "")
   string(CONCAT msg "Currently, Tilck can be built ONLY on x86_64 host "
                     "machines no matter which target architecture has been "
                     "chosen. Reason: the build system uses a toolchain "
                     "pre-compiled for x86_64 hosts.")

   message(FATAL_ERROR "${msg}")

endfunction()

function (show_wconv_warning)

   set(msg "")
   string(APPEND msg "WCONV (-Wconversion) is supported ONLY when the "
                     "kernel is built with Clang: "
                     "(CC=clang and CMAKE_ARGS='-DKERNEL_SYSCC=1')")

   message(WARNING "${msg}")

endfunction()

function (show_clang_and_syscc_error)

   set(msg "")
   string(APPEND msg "USE_SYSCC=1 is not supported with Clang. "
                     "Please use GCC. Note[1]: it is possible to build just "
                     "the kernel with Clang by setting CC=clang the CMake "
                     "variable KERNEL_SYSCC=1. However, the i686 gcc "
                     "toolchain will still be used for assembly files and "
                     "other targets. Note[2]: if you're using "
                     "./scripts/cmake_run, you can set CMake variables by "
                     "setting the CMAKE_ARGS environment variable this way: "
                     "\nCMAKE_ARGS='-DCMAKE_VAR1=0 -DVAR2=blabla' "
                     "./scripts/cmake_run")

   message(FATAL_ERROR "${msg}")

endfunction()

macro (show_missing_lcov_error)
   message(FATAL_ERROR "TEST_GCOV/KERNEL_GCOV set but no lcov in toolchain. "
                       "Run ${BTC_SCRIPT} -s build_lcov first.")
endmacro()