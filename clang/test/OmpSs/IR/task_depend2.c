// RUN: %clang_cc1 -verify -fompss-2 -disable-llvm-passes -ferror-limit 100 %s -S -emit-llvm -o - | FileCheck %s
// expected-no-diagnostics

void foo1() {
    int **a;
    int b[5][6];
    int (*c)[5];
    int d[5][6][7];
    int e[5];
    #pragma oss task depend(in: a, *a, a[1], a[1][2])
    {}
    #pragma oss task depend(in: b, *b, b[1], b[1][2])
    {}
    #pragma oss task depend(in: c, *c, c[1], c[1][2])
    {}
    #pragma oss task depend(in: d, d[1][2][3])
    {}
    #pragma oss task depend(in: e, e[1])
    {}
}

// CHECK: %0 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(i32*** %a), "QUAL.OSS.DEP.IN"(i32*** %a, %struct._depend_unpack_t (i32***)* @compute_dep, i32*** %a), "QUAL.OSS.DEP.IN"(i32*** %a, %struct._depend_unpack_t.0 (i32***)* @compute_dep.1, i32*** %a), "QUAL.OSS.DEP.IN"(i32*** %a, %struct._depend_unpack_t.1 (i32***)* @compute_dep.2, i32*** %a), "QUAL.OSS.DEP.IN"(i32*** %a, %struct._depend_unpack_t.2 (i32***)* @compute_dep.3, i32*** %a) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %0)
// CHECK-NEXT: %1 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([5 x [6 x i32]]* %b), "QUAL.OSS.DEP.IN"([5 x [6 x i32]]* %b, %struct._depend_unpack_t.3 ([5 x [6 x i32]]*)* @compute_dep.4, [5 x [6 x i32]]* %b), "QUAL.OSS.DEP.IN"([5 x [6 x i32]]* %b, %struct._depend_unpack_t.4 ([5 x [6 x i32]]*)* @compute_dep.5, [5 x [6 x i32]]* %b), "QUAL.OSS.DEP.IN"([5 x [6 x i32]]* %b, %struct._depend_unpack_t.5 ([5 x [6 x i32]]*)* @compute_dep.6, [5 x [6 x i32]]* %b), "QUAL.OSS.DEP.IN"([5 x [6 x i32]]* %b, %struct._depend_unpack_t.6 ([5 x [6 x i32]]*)* @compute_dep.7, [5 x [6 x i32]]* %b) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %1)
// CHECK-NEXT: %2 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([5 x i32]** %c), "QUAL.OSS.DEP.IN"([5 x i32]** %c, %struct._depend_unpack_t.7 ([5 x i32]**)* @compute_dep.8, [5 x i32]** %c), "QUAL.OSS.DEP.IN"([5 x i32]** %c, %struct._depend_unpack_t.8 ([5 x i32]**)* @compute_dep.9, [5 x i32]** %c), "QUAL.OSS.DEP.IN"([5 x i32]** %c, %struct._depend_unpack_t.9 ([5 x i32]**)* @compute_dep.10, [5 x i32]** %c), "QUAL.OSS.DEP.IN"([5 x i32]** %c, %struct._depend_unpack_t.10 ([5 x i32]**)* @compute_dep.11, [5 x i32]** %c) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %2)
// CHECK-NEXT: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([5 x [6 x [7 x i32]]]* %d), "QUAL.OSS.DEP.IN"([5 x [6 x [7 x i32]]]* %d, %struct._depend_unpack_t.11 ([5 x [6 x [7 x i32]]]*)* @compute_dep.12, [5 x [6 x [7 x i32]]]* %d), "QUAL.OSS.DEP.IN"([5 x [6 x [7 x i32]]]* %d, %struct._depend_unpack_t.12 ([5 x [6 x [7 x i32]]]*)* @compute_dep.13, [5 x [6 x [7 x i32]]]* %d) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %3)
// CHECK-NEXT: %4 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([5 x i32]* %e), "QUAL.OSS.DEP.IN"([5 x i32]* %e, %struct._depend_unpack_t.13 ([5 x i32]*)* @compute_dep.14, [5 x i32]* %e), "QUAL.OSS.DEP.IN"([5 x i32]* %e, %struct._depend_unpack_t.14 ([5 x i32]*)* @compute_dep.15, [5 x i32]* %e) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %4)

// CHECK: define internal %struct._depend_unpack_t @compute_dep(i32*** %a) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32*** %a, i32**** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 8, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.0 @compute_dep.1(i32*** %a) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.0, align 8
// CHECK-NEXT:   %0 = load i32**, i32*** %a, align 8, !dbg !9
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32** %0, i32*** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 8, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %4, align 8
// CHECK-NEXT:   %5 = load %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.0 %5
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.1 @compute_dep.2(i32*** %a) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.1, align 8
// CHECK-NEXT:   %0 = load i32**, i32*** %a, align 8, !dbg !9
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32** %0, i32*** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 8, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 16, i64* %4, align 8
// CHECK-NEXT:   %5 = load %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.1 %5
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.2 @compute_dep.3(i32*** %a) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.2, align 8
// CHECK-NEXT:   %0 = load i32**, i32*** %a, align 8, !dbg !9
// CHECK-NEXT:   %arrayidx = getelementptr inbounds i32*, i32** %0, i64 1, !dbg !9
// CHECK-NEXT:   %1 = load i32*, i32** %arrayidx, align 8, !dbg !9
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %1, i32** %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 8, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 12, i64* %5, align 8
// CHECK-NEXT:   %6 = load %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.2 %6
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.3 @compute_dep.4([5 x [6 x i32]]* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.3, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x [6 x i32]]* %b, [5 x [6 x i32]]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 24, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 24, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 5, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 0, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 5, i64* %6, align 8
// CHECK-NEXT:   %7 = load %struct._depend_unpack_t.3, %struct._depend_unpack_t.3* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.3 %7
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.4 @compute_dep.5([5 x [6 x i32]]* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.4, align 8
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [5 x [6 x i32]], [5 x [6 x i32]]* %b, i64 0, i64 0
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.4, %struct._depend_unpack_t.4* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [6 x i32]* %arraydecay, [6 x i32]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.4, %struct._depend_unpack_t.4* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 24, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.4, %struct._depend_unpack_t.4* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.4, %struct._depend_unpack_t.4* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 24, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.4, %struct._depend_unpack_t.4* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.4 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.5 @compute_dep.6([5 x [6 x i32]]* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.5, align 8
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [5 x [6 x i32]], [5 x [6 x i32]]* %b, i64 0, i64 0, !dbg !11
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [6 x i32]* %arraydecay, [6 x i32]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 24, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 24, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 5, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 2, i64* %6, align 8
// CHECK-NEXT:   %7 = load %struct._depend_unpack_t.5, %struct._depend_unpack_t.5* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.5 %7
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.6 @compute_dep.7([5 x [6 x i32]]* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.6, align 8
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [5 x [6 x i32]], [5 x [6 x i32]]* %b, i64 0, i64 0, !dbg !11
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [6 x i32]* %arraydecay, [6 x i32]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 24, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 8, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 12, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 5, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 2, i64* %6, align 8
// CHECK-NEXT:   %7 = load %struct._depend_unpack_t.6, %struct._depend_unpack_t.6* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.6 %7
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.7 @compute_dep.8([5 x i32]** %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.7, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.7, %struct._depend_unpack_t.7* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x i32]** %c, [5 x i32]*** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.7, %struct._depend_unpack_t.7* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 8, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.7, %struct._depend_unpack_t.7* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.7, %struct._depend_unpack_t.7* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.7, %struct._depend_unpack_t.7* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.7 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.8 @compute_dep.9([5 x i32]** %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.8, align 8
// CHECK-NEXT:   %0 = load [5 x i32]*, [5 x i32]** %c, align 8, !dbg !13
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.8, %struct._depend_unpack_t.8* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x i32]* %0, [5 x i32]** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.8, %struct._depend_unpack_t.8* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 20, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.8, %struct._depend_unpack_t.8* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.8, %struct._depend_unpack_t.8* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 20, i64* %4, align 8
// CHECK-NEXT:   %5 = load %struct._depend_unpack_t.8, %struct._depend_unpack_t.8* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.8 %5
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.9 @compute_dep.10([5 x i32]** %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.9, align 8
// CHECK-NEXT:   %0 = load [5 x i32]*, [5 x i32]** %c, align 8, !dbg !13
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x i32]* %0, [5 x i32]** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 20, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 20, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 1, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 2, i64* %7, align 8
// CHECK-NEXT:   %8 = load %struct._depend_unpack_t.9, %struct._depend_unpack_t.9* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.9 %8
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.10 @compute_dep.11([5 x i32]** %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.10, align 8
// CHECK-NEXT:   %0 = load [5 x i32]*, [5 x i32]** %c, align 8, !dbg !13
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x i32]* %0, [5 x i32]** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 20, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 12, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 1, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 2, i64* %7, align 8
// CHECK-NEXT:   %8 = load %struct._depend_unpack_t.10, %struct._depend_unpack_t.10* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.10 %8
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.11 @compute_dep.12([5 x [6 x [7 x i32]]]* %d) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.11, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x [6 x [7 x i32]]]* %d, [5 x [6 x [7 x i32]]]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 28, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 28, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 6, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 0, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 6, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 7
// CHECK-NEXT:   store i64 5, i64* %7, align 8
// CHECK-NEXT:   %8 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 8
// CHECK-NEXT:   store i64 0, i64* %8, align 8
// CHECK-NEXT:   %9 = getelementptr inbounds %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, i32 0, i32 9
// CHECK-NEXT:   store i64 5, i64* %9, align 8
// CHECK-NEXT:   %10 = load %struct._depend_unpack_t.11, %struct._depend_unpack_t.11* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.11 %10
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.12 @compute_dep.13([5 x [6 x [7 x i32]]]* %d) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.12, align 8
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [5 x [6 x [7 x i32]]], [5 x [6 x [7 x i32]]]* %d, i64 0, i64 0, !dbg !15
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [6 x [7 x i32]]* %arraydecay, [6 x [7 x i32]]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 28, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 12, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 16, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 6, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 2, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 3, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 7
// CHECK-NEXT:   store i64 5, i64* %7, align 8
// CHECK-NEXT:   %8 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 8
// CHECK-NEXT:   store i64 1, i64* %8, align 8
// CHECK-NEXT:   %9 = getelementptr inbounds %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, i32 0, i32 9
// CHECK-NEXT:   store i64 2, i64* %9, align 8
// CHECK-NEXT:   %10 = load %struct._depend_unpack_t.12, %struct._depend_unpack_t.12* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.12 %10
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.13 @compute_dep.14([5 x i32]* %e) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.13, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.13, %struct._depend_unpack_t.13* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [5 x i32]* %e, [5 x i32]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.13, %struct._depend_unpack_t.13* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 20, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.13, %struct._depend_unpack_t.13* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.13, %struct._depend_unpack_t.13* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 20, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.13, %struct._depend_unpack_t.13* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.13 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.14 @compute_dep.15([5 x i32]* %e) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.14, align 8
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [5 x i32], [5 x i32]* %e, i64 0, i64 0, !dbg !17
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.14, %struct._depend_unpack_t.14* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %arraydecay, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.14, %struct._depend_unpack_t.14* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 20, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.14, %struct._depend_unpack_t.14* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 4, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.14, %struct._depend_unpack_t.14* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.14, %struct._depend_unpack_t.14* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.14 %4
// CHECK-NEXT: }


void foo2() {
    struct A {
        int x;
    } a;
    struct B {
        int x[10];
    } b;
    struct C {
        int (*x)[10];
    } c;
    struct D {
        int *x;
    } d;
    #pragma oss task depend(in: a.x)
    {}
    #pragma oss task depend(in: b.x, b.x[0])
    {}
    #pragma oss task depend(in: c.x, c.x[0], c.x[0][1])
    {}
    #pragma oss task depend(in: *(d.x))
    {}
}

// CHECK: %0 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.A* %a), "QUAL.OSS.DEP.IN"(%struct.A* %a, %struct._depend_unpack_t.15 (%struct.A*)* @compute_dep.16, %struct.A* %a) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %0)
// CHECK-NEXT: %1 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.B* %b), "QUAL.OSS.DEP.IN"(%struct.B* %b, %struct._depend_unpack_t.16 (%struct.B*)* @compute_dep.17, %struct.B* %b), "QUAL.OSS.DEP.IN"(%struct.B* %b, %struct._depend_unpack_t.17 (%struct.B*)* @compute_dep.18, %struct.B* %b) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %1)
// CHECK-NEXT: %2 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.C* %c), "QUAL.OSS.DEP.IN"(%struct.C* %c, %struct._depend_unpack_t.18 (%struct.C*)* @compute_dep.19, %struct.C* %c), "QUAL.OSS.DEP.IN"(%struct.C* %c, %struct._depend_unpack_t.19 (%struct.C*)* @compute_dep.20, %struct.C* %c), "QUAL.OSS.DEP.IN"(%struct.C* %c, %struct._depend_unpack_t.20 (%struct.C*)* @compute_dep.21, %struct.C* %c) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %2)
// CHECK-NEXT: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.D* %d), "QUAL.OSS.DEP.IN"(%struct.D* %d, %struct._depend_unpack_t.21 (%struct.D*)* @compute_dep.22, %struct.D* %d) ]
// CHECK-NEXT: call void @llvm.directive.region.exit(token %3)

// CHECK: define internal %struct._depend_unpack_t.15 @compute_dep.16(%struct.A* %a) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.15, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.A, %struct.A* %a, i32 0, i32 0
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.15, %struct._depend_unpack_t.15* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %x, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.15, %struct._depend_unpack_t.15* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.15, %struct._depend_unpack_t.15* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.15, %struct._depend_unpack_t.15* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.15, %struct._depend_unpack_t.15* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.15 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.16 @compute_dep.17(%struct.B* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.16, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.B, %struct.B* %b, i32 0, i32 0
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.16, %struct._depend_unpack_t.16* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [10 x i32]* %x, [10 x i32]** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.16, %struct._depend_unpack_t.16* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 40, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.16, %struct._depend_unpack_t.16* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.16, %struct._depend_unpack_t.16* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 40, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.16, %struct._depend_unpack_t.16* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.16 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.17 @compute_dep.18(%struct.B* %b) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.17, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.B, %struct.B* %b, i32 0, i32 0, !dbg !23
// CHECK-NEXT:   %arraydecay = getelementptr inbounds [10 x i32], [10 x i32]* %x, i64 0, i64 0, !dbg !23
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.17, %struct._depend_unpack_t.17* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %arraydecay, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.17, %struct._depend_unpack_t.17* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 40, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.17, %struct._depend_unpack_t.17* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.17, %struct._depend_unpack_t.17* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.17, %struct._depend_unpack_t.17* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.17 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.18 @compute_dep.19(%struct.C* %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.18, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 0
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.18, %struct._depend_unpack_t.18* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [10 x i32]** %x, [10 x i32]*** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.18, %struct._depend_unpack_t.18* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 8, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.18, %struct._depend_unpack_t.18* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.18, %struct._depend_unpack_t.18* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.18, %struct._depend_unpack_t.18* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.18 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.19 @compute_dep.20(%struct.C* %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.19, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 0, !dbg !25
// CHECK-NEXT:   %0 = load [10 x i32]*, [10 x i32]** %x, align 8, !dbg !25
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [10 x i32]* %0, [10 x i32]** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 40, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 40, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 0, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 1, i64* %7, align 8
// CHECK-NEXT:   %8 = load %struct._depend_unpack_t.19, %struct._depend_unpack_t.19* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.19 %8
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.20 @compute_dep.21(%struct.C* %c) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.20, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 0, !dbg !25
// CHECK-NEXT:   %0 = load [10 x i32]*, [10 x i32]** %x, align 8, !dbg !25
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 0
// CHECK-NEXT:   store [10 x i32]* %0, [10 x i32]** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 40, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 8, i64* %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 4
// CHECK-NEXT:   store i64 1, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 5
// CHECK-NEXT:   store i64 0, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, i32 0, i32 6
// CHECK-NEXT:   store i64 1, i64* %7, align 8
// CHECK-NEXT:   %8 = load %struct._depend_unpack_t.20, %struct._depend_unpack_t.20* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.20 %8
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.21 @compute_dep.22(%struct.D* %d) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.21, align 8
// CHECK-NEXT:   %x = getelementptr inbounds %struct.D, %struct.D* %d, i32 0, i32 0, !dbg !27
// CHECK-NEXT:   %0 = load i32*, i32** %x, align 8, !dbg !27
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.21, %struct._depend_unpack_t.21* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %0, i32** %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.21, %struct._depend_unpack_t.21* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.21, %struct._depend_unpack_t.21* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %3, align 8
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t.21, %struct._depend_unpack_t.21* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %4, align 8
// CHECK-NEXT:   %5 = load %struct._depend_unpack_t.21, %struct._depend_unpack_t.21* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.21 %5
// CHECK-NEXT: }
