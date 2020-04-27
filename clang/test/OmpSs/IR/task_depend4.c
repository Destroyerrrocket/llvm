// RUN: %clang_cc1 -verify -fompss-2 -disable-llvm-passes -ferror-limit 100 %s -S -emit-llvm -o - | FileCheck %s
// expected-no-diagnostics

void foo1(int a) {
    // concurrent
    #pragma oss task depend(mutexinoutset: a)
    {}
    #pragma oss task concurrent(a)
    {}
    // commutative
    #pragma oss task depend(inoutset: a)
    {}
    #pragma oss task commutative(a)
    {}
}

// CHECK: %0 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(i32* %a.addr), "QUAL.OSS.DEP.CONCURRENT"(i32* %a.addr, %struct._depend_unpack_t (i32*)* @compute_dep, i32* %a.addr) ]
// CHECK: %1 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(i32* %a.addr), "QUAL.OSS.DEP.CONCURRENT"(i32* %a.addr, %struct._depend_unpack_t.0 (i32*)* @compute_dep.1, i32* %a.addr) ]
// CHECK: %2 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(i32* %a.addr), "QUAL.OSS.DEP.COMMUTATIVE"(i32* %a.addr, %struct._depend_unpack_t.1 (i32*)* @compute_dep.2, i32* %a.addr) ]
// CHECK: %3 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(i32* %a.addr), "QUAL.OSS.DEP.COMMUTATIVE"(i32* %a.addr, %struct._depend_unpack_t.2 (i32*)* @compute_dep.3, i32* %a.addr) ]

// CHECK: define internal %struct._depend_unpack_t @compute_dep(i32* %a.addr) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %a.addr, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.0 @compute_dep.1(i32* %a.addr) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.0, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %a.addr, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.0 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.1 @compute_dep.2(i32* %a.addr) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.1, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %a.addr, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.1 %4
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.2 @compute_dep.3(i32* %a.addr) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.2, align 8
// CHECK-NEXT:   %0 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %a.addr, i32** %0, align 8
// CHECK-NEXT:   %1 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 4, i64* %1, align 8
// CHECK-NEXT:   %2 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %2, align 8
// CHECK-NEXT:   %3 = getelementptr inbounds %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 4, i64* %3, align 8
// CHECK-NEXT:   %4 = load %struct._depend_unpack_t.2, %struct._depend_unpack_t.2* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.2 %4
// CHECK-NEXT: }
