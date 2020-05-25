// RUN: %clang_cc1 -verify -fompss-2 -disable-llvm-passes -ferror-limit 100 %s -S -emit-llvm -o - | FileCheck %s
// expected-no-diagnostics

#pragma oss task in(*a)
void foo(int size, int (*a)[size]) {
}

int main() {
    int n;
    int mat[n][n];
    foo(n - 1, mat);
}

// CHECK: %call_arg = alloca i32, align 4
// CHECK-NEXT: %call_arg1 = alloca i32*, align 8

// CHECK:  %6 = load i32, i32* %n, align 4
// CHECK-NEXT: %sub = sub nsw i32 %6, 1
// CHECK-NEXT: store i32 %sub, i32* %call_arg, align 4
// CHECK: store i32* %vla, i32** %call_arg1, align 8
// CHECK: %9 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32* %call_arg), "QUAL.OSS.FIRSTPRIVATE"(i32** %call_arg1), "QUAL.OSS.DEP.IN"(i32** %call_arg1, %struct._depend_unpack_t (i32**, i64)* @compute_dep, i32** %call_arg1, i64 %8), "QUAL.OSS.CAPTURED"(i64 %8) ]
// CHECK-NEXT: %10 = load i32, i32* %call_arg, align 4
// CHECK-NEXT: %11 = load i32*, i32** %call_arg1, align 8
// CHECK-NEXT: call void @foo(i32{{( signext)?}} %10, i32* %11)

// CHECK: define internal %struct._depend_unpack_t @compute_dep(i32** %call_arg1, i64 %0) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t, align 8
// CHECK-NEXT:   %1 = load i32*, i32** %call_arg1, align 8, !dbg !16
// CHECK-NEXT:   %2 = mul i64 %0, 4
// CHECK-NEXT:   %3 = mul i64 %0, 4
// CHECK-NEXT:   %4 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %1, i32** %4, align 8
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 %2, i64* %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 %3, i64* %7, align 8
// CHECK-NEXT:   %8 = load %struct._depend_unpack_t, %struct._depend_unpack_t* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t %8
// CHECK-NEXT: }

#pragma oss task out([size/77]p)
void foo1(int size, int *p) {
}

void bar() {
    int n = 10;
    int *v;
    foo1(n/55, v);
    foo1(n/99, v);
}

// checks we do not reuse VLASize between task outline calls

// CHECK: %0 = load i32, i32* %n, align 4
// CHECK-NEXT: %div = sdiv i32 %0, 55
// CHECK-NEXT: store i32 %div, i32* %call_arg, align 4
// CHECK-NEXT: %1 = load i32*, i32** %v, align 8
// CHECK-NEXT: store i32* %1, i32** %call_arg1, align 8
// CHECK-NEXT: %2 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32* %call_arg), "QUAL.OSS.FIRSTPRIVATE"(i32** %call_arg1), "QUAL.OSS.DEP.OUT"(i32** %call_arg1, %struct._depend_unpack_t.0 (i32**, i32*)* @compute_dep.1, i32** %call_arg1, i32* %call_arg) ]

// CHECK: %5 = load i32, i32* %n, align 4
// CHECK-NEXT: %div3 = sdiv i32 %5, 99
// CHECK-NEXT: store i32 %div3, i32* %call_arg2, align 4
// CHECK-NEXT: %6 = load i32*, i32** %v, align 8
// CHECK-NEXT: store i32* %6, i32** %call_arg4, align 8
// CHECK-NEXT: %7 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.FIRSTPRIVATE"(i32* %call_arg2), "QUAL.OSS.FIRSTPRIVATE"(i32** %call_arg4), "QUAL.OSS.DEP.OUT"(i32** %call_arg4, %struct._depend_unpack_t.1 (i32**, i32*)* @compute_dep.2, i32** %call_arg4, i32* %call_arg2) ]

// CHECK: define internal %struct._depend_unpack_t.0 @compute_dep.1(i32** %call_arg1, i32* %call_arg) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.0, align 8
// CHECK-NEXT:   %0 = load i32*, i32** %call_arg1, align 8, !dbg !24
// CHECK-NEXT:   %1 = load i32, i32* %call_arg, align 4, !dbg !24
// CHECK-NEXT:   %div = sdiv i32 %1, 77, !dbg !24
// CHECK-NEXT:   %2 = zext i32 %div to i64
// CHECK-NEXT:   %3 = mul i64 %2, 4
// CHECK-NEXT:   %4 = mul i64 %2, 4
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %0, i32** %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 %3, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %7, align 8
// CHECK-NEXT:   %8 = getelementptr inbounds %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 %4, i64* %8, align 8
// CHECK-NEXT:   %9 = load %struct._depend_unpack_t.0, %struct._depend_unpack_t.0* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.0 %9
// CHECK-NEXT: }

// CHECK: define internal %struct._depend_unpack_t.1 @compute_dep.2(i32** %call_arg4, i32* %call_arg2) {
// CHECK-NEXT: entry:
// CHECK-NEXT:   %return.val = alloca %struct._depend_unpack_t.1, align 8
// CHECK-NEXT:   %0 = load i32*, i32** %call_arg4, align 8, !dbg !24
// CHECK-NEXT:   %1 = load i32, i32* %call_arg2, align 4, !dbg !24
// CHECK-NEXT:   %div = sdiv i32 %1, 77, !dbg !24
// CHECK-NEXT:   %2 = zext i32 %div to i64
// CHECK-NEXT:   %3 = mul i64 %2, 4
// CHECK-NEXT:   %4 = mul i64 %2, 4
// CHECK-NEXT:   %5 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 0
// CHECK-NEXT:   store i32* %0, i32** %5, align 8
// CHECK-NEXT:   %6 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 1
// CHECK-NEXT:   store i64 %3, i64* %6, align 8
// CHECK-NEXT:   %7 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 2
// CHECK-NEXT:   store i64 0, i64* %7, align 8
// CHECK-NEXT:   %8 = getelementptr inbounds %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, i32 0, i32 3
// CHECK-NEXT:   store i64 %4, i64* %8, align 8
// CHECK-NEXT:   %9 = load %struct._depend_unpack_t.1, %struct._depend_unpack_t.1* %return.val, align 8
// CHECK-NEXT:   ret %struct._depend_unpack_t.1 %9
// CHECK-NEXT: }
