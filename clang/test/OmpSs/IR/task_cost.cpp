// RUN: %clang_cc1 -verify -fompss-2 -disable-llvm-passes -ferror-limit 100 %s -S -emit-llvm -o - | FileCheck %s
// expected-no-diagnostics
template<typename T> T foo() { return T(); }

#pragma oss task cost(foo<int>())
void foo1() {}
#pragma oss task cost(n)
void foo2(int n) {}

void bar(int n) {
    int vla[n];
    #pragma oss task cost(foo<int>())
    {}
    #pragma oss task cost(n)
    {}
    #pragma oss task cost(vla[1])
    {}
    foo1();
    foo2(n);
}

// CHECK: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.COST"(i32 %call), "QUAL.OSS.CAPTURED"(i32 %call) ]
// CHECK: %5 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.COST"(i32 %4), "QUAL.OSS.CAPTURED"(i32 %4) ]
// CHECK: %7 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.COST"(i32 %6), "QUAL.OSS.CAPTURED"(i32 %6) ]

// CHECK: %8 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.COST"(i32 %call1), "QUAL.OSS.CAPTURED"(i32 %call1) ]
// CHECK: %11 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32* %call_arg), "QUAL.OSS.COST"(i32 %10), "QUAL.OSS.CAPTURED"(i32 %10) ]
