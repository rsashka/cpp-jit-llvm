#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=clang-18
CCC=clang++-18
CXX=clang++-18
FC=gfortran
AS=lld-18

# Macros
CND_PLATFORM=CLang-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=`llvm-config-18 --cxxflags` -fexceptions -fcxx-exceptions -std=c++20  -Wfloat-equal -Wundef  -Wwrite-strings -Wmissing-declarations  -Wno-trigraphs -Wno-invalid-source-encoding -stdlib=libstdc++ -Wno-error=unused-variable -Wno-error=unused-parameter -Wno-error=switch -fsanitize=undefined -fsanitize-trap=undefined   -gdwarf-4               -Wno-undefined-var-template -Wno-switch  -fvisibility=default  -ggdb -O0 -DGTEST_HAS_CXXABI_H_=0 
CXXFLAGS=`llvm-config-18 --cxxflags` -fexceptions -fcxx-exceptions -std=c++20  -Wfloat-equal -Wundef  -Wwrite-strings -Wmissing-declarations  -Wno-trigraphs -Wno-invalid-source-encoding -stdlib=libstdc++ -Wno-error=unused-variable -Wno-error=unused-parameter -Wno-error=switch -fsanitize=undefined -fsanitize-trap=undefined   -gdwarf-4               -Wno-undefined-var-template -Wno-switch  -fvisibility=default  -ggdb -O0 -DGTEST_HAS_CXXABI_H_=0 

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-ldl -lpthread

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk cpp-jit-llvm

cpp-jit-llvm: ${OBJECTFILES}
	${LINK.cc} -o cpp-jit-llvm ${OBJECTFILES} ${LDLIBSOPTIONS} `llvm-config-18 --link-static --system-libs --libs all` -fuse-ld=lld -g -fvisibility=default -Wl,--export-dynamic -Wl,--exclude-libs,ALL -lclang-cpp -rdynamic

${OBJECTDIR}/main.o: main.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBUILD_DEBUG -DENABLE_LLVM_SHARED=1 -I/usr/local/include -I/usr/include/x86_64-linux-gnu/c++/10 -I../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
