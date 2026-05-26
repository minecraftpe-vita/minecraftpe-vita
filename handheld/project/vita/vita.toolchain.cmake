if(DEFINED ENV{VITASDK})
    include("$ENV{VITASDK}/share/vita.toolchain.cmake")
else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
endif()

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_ASM_COMPILER "clang")

string(REPLACE "-Wl,-q" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wl,-q" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# awful
execute_process(
    COMMAND ${VITASDK}/bin/arm-vita-eabi-gcc -dumpversion
    OUTPUT_VARIABLE VITA_GCC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_compile_options(
    --sysroot=${VITASDK}/arm-vita-eabi
    --target=armv7-vita-eabi
    -mcpu=cortex-a9
    -march=armv7-a
    -mfpu=neon
    -mfloat-abi=hard
    -stdlib=libstdc++
    -isystem ${VITASDK}/arm-vita-eabi/include/c++/${VITA_GCC_VERSION}/arm-vita-eabi
    -D__vita__
)

add_link_options(
    --sysroot=${VITASDK}/arm-vita-eabi
    --target=armv7-vita-eabi
    -mcpu=cortex-a9
    -march=armv7-a
    -mfpu=neon
    -mfloat-abi=hard
    
    -nodefaultlibs
    -lstdc++
    -lm
    -lSceRtc_stub
    -lSceSysmem_stub
    -lSceLibKernel_stub
    -lSceIofilemgr_stub
    -lSceSysmodule_stub
    -lSceProcessmgr_stub
    -lSceKernelThreadMgr_stub
    ${VITASDK}/arm-vita-eabi/lib/crt0.o
    ${VITASDK}/lib/gcc/arm-vita-eabi/${VITA_GCC_VERSION}/crti.o
    ${VITASDK}/lib/gcc/arm-vita-eabi/${VITA_GCC_VERSION}/crtn.o
    ${VITASDK}/lib/gcc/arm-vita-eabi/${VITA_GCC_VERSION}/libgcc.a
    -lc

    -Wl,-q
    -Wl,--image-base=0x81000000
    -Wl,--no-rosegment
    -Wl,-z,norelro
)



