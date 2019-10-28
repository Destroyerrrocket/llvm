// RUN: %clang_cc1 -verify -fompss-2 -disable-llvm-passes -ferror-limit 100 %s -S -emit-llvm -o - | FileCheck %s
// expected-no-diagnostics

typedef int* Pointer;

void foo(int x) {
    Pointer aFix[10][10];
    int aVLA[x][7];
    #pragma oss task depend(in : aFix[:], aFix[1:], aFix[: 2], aFix[3 : 4])
    #pragma oss task depend(in : aFix[5][:], aFix[5][1:], aFix[5][: 2], aFix[5][3 : 4])
    #pragma oss task depend(in : aVLA[:], aVLA[1:], aVLA[: 2], aVLA[3 : 4])
    #pragma oss task depend(in : aVLA[5][:], aVLA[5][1:], aVLA[5][: 2], aVLA[5][3 : 4])
    {}
}

// CHECK: %x.addr = alloca i32, align 4
// CHECK-NEXT: %aFix = alloca [10 x [10 x i32*]], align 16
// CHECK-NEXT: %saved_stack = alloca i8*, align 8
// CHECK-NEXT: %__vla_expr0 = alloca i64, align 8
// CHECK-NEXT: store i32 %x, i32* %x.addr, align 4
// CHECK-NEXT: %0 = load i32, i32* %x.addr, align 4 
// CHECK-NEXT: %1 = zext i32 %0 to i64 
// CHECK-NEXT: %2 = call i8* @llvm.stacksave() 
// CHECK-NEXT: store i8* %2, i8** %saved_stack, align 8 
// CHECK-NEXT: %vla = alloca [7 x i32], i64 %1, align 16 
// CHECK-NEXT: store i64 %1, i64* %__vla_expr0, align 8 
// CHECK: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([10 x [10 x i32*]]* %aFix), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay, i64 80, i64 0, i64 80, i64 10, i64 0, i64 10), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay1, i64 80, i64 0, i64 80, i64 10, i64 1, i64 10), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay2, i64 80, i64 0, i64 80, i64 10, i64 0, i64 2), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay3, i64 80, i64 0, i64 80, i64 10, i64 3, i64 7) ] 

// CHECK: %4 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([10 x [10 x i32*]]* %aFix), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay4, i64 80, i64 0, i64 80, i64 10, i64 5, i64 6), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay5, i64 80, i64 8, i64 80, i64 10, i64 5, i64 6), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay6, i64 80, i64 0, i64 16, i64 10, i64 5, i64 6), "QUAL.OSS.DEP.IN"([10 x i32*]* %arraydecay7, i64 80, i64 24, i64 56, i64 10, i64 5, i64 6) ] 
// CHECK-NEXT: %5 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([7 x i32]* %vla), "QUAL.OSS.VLA.DIMS"([7 x i32]* %vla, i64 %1, i64 7), "QUAL.OSS.CAPTURED"(i64 %1, i64 7), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 28, i64 %1, i64 0, i64 %1), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 28, i64 %1, i64 1, i64 %1), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 28, i64 %1, i64 0, i64 2), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 28, i64 %1, i64 3, i64 7) ]
// CHECK-NEXT: %6 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([7 x i32]* %vla), "QUAL.OSS.VLA.DIMS"([7 x i32]* %vla, i64 %1, i64 7), "QUAL.OSS.CAPTURED"(i64 %1, i64 7), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 28, i64 %1, i64 5, i64 6), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 4, i64 28, i64 %1, i64 5, i64 6), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 0, i64 8, i64 %1, i64 5, i64 6), "QUAL.OSS.DEP.IN"([7 x i32]* %vla, i64 28, i64 12, i64 28, i64 %1, i64 5, i64 6) ]

void bar() {
    int **p;
    int (*kk)[10];
    int array[10][20];
    #pragma oss task depend(in: kk[0 : 11])
    {}
    #pragma oss task depend(in: p[0 : 11])
    {}
    #pragma oss task depend(in: array[0: 11][7 : 11])
    {}
    struct C {
        int (*x)[10];
    } c;

    #pragma oss task depend(in: c.x[0 : 11])
    {}

    #pragma oss task in(kk[0 ; 11])
    {}
    #pragma oss task in(p[0 ; 11])
    {}
    #pragma oss task in(array[0; 11][7 ; 11])
    {}

    #pragma oss task in(kk[0 : 11 - 1])
    {}
    #pragma oss task in(p[0 : 11 - 1])
    {}
    #pragma oss task in(array[0: 11 - 1][7 : 7 + 11 - 1])
    {}
}


// CHECK: %1 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"([10 x i32]** %kk), "QUAL.OSS.DEP.IN"([10 x i32]* %0, i64 40, i64 0, i64 40, i64 11, i64 0, i64 11) ]
// CHECK: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32*** %p), "QUAL.OSS.DEP.IN"(i32** %2, i64 88, i64 0, i64 88) ]
// CHECK: %4 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([10 x [20 x i32]]* %array), "QUAL.OSS.DEP.IN"([20 x i32]* %arraydecay, i64 80, i64 28, i64 72, i64 10, i64 0, i64 11) ]
// CHECK: %6 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.C* %c), "QUAL.OSS.DEP.IN"([10 x i32]* %5, i64 40, i64 0, i64 40, i64 11, i64 0, i64 11) ]

// CHECK: %8 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"([10 x i32]** %kk), "QUAL.OSS.DEP.IN"([10 x i32]* %7, i64 40, i64 0, i64 40, i64 11, i64 0, i64 11) ]
// CHECK: %10 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32*** %p), "QUAL.OSS.DEP.IN"(i32** %9, i64 88, i64 0, i64 88) ]
// CHECK: %11 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([10 x [20 x i32]]* %array), "QUAL.OSS.DEP.IN"([20 x i32]* %arraydecay1, i64 80, i64 28, i64 72, i64 10, i64 0, i64 11) ]

// CHECK: %13 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"([10 x i32]** %kk), "QUAL.OSS.DEP.IN"([10 x i32]* %12, i64 40, i64 0, i64 40, i64 10, i64 0, i64 11) ]
// CHECK: %15 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32*** %p), "QUAL.OSS.DEP.IN"(i32** %14, i64 80, i64 0, i64 88) ]
// CHECK: %16 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([10 x [20 x i32]]* %array), "QUAL.OSS.DEP.IN"([20 x i32]* %arraydecay2, i64 80, i64 28, i64 72, i64 10, i64 0, i64 11) ]

