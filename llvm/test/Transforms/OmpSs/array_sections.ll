; RUN: opt %s -ompss-2 -S | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@mat = common dso_local global [7 x [3 x i32]] zeroinitializer, align 16

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() !dbg !6 {
entry:
  %0 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"([7 x [3 x i32]]* @mat), "QUAL.OSS.DEP.IN"([3 x i32]* getelementptr inbounds ([7 x [3 x i32]], [7 x [3 x i32]]* @mat, i64 0, i64 0), i64 12, i64 0, i64 12, i64 7, i64 0, i64 7), "QUAL.OSS.DEP.IN"([3 x i32]* getelementptr inbounds ([7 x [3 x i32]], [7 x [3 x i32]]* @mat, i64 0, i64 0), i64 12, i64 4, i64 8, i64 7, i64 0, i64 2) ], !dbg !8
  call void @llvm.directive.region.exit(token %0), !dbg !8
  ret i32 0, !dbg !8
}

; CHECK: define internal void @nanos6_unpacked_deps_main0([7 x [3 x i32]]* %mat, i8* %loop_bounds, i8* %handler) {
; CHECK-NEXT:   entry:
; CHECK-NEXT:   %0 = getelementptr inbounds [7 x [3 x i32]], [7 x [3 x i32]]* %mat, i64 0, i64 0
; CHECK-NEXT:   %1 = bitcast [3 x i32]* %0 to i8*
; CHECK-NEXT:   call void @nanos6_register_region_read_depinfo2(i8* %handler, i32 0, i8* null, i8* %1, i64 12, i64 0, i64 12, i64 7, i64 0, i64 7)
; CHECK-NEXT:   %2 = bitcast [3 x i32]* %0 to i8*
; CHECK-NEXT:   call void @nanos6_register_region_read_depinfo2(i8* %handler, i32 0, i8* null, i8* %2, i64 12, i64 4, i64 8, i64 7, i64 0, i64 2)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK: define internal void @nanos6_ol_deps_main0(%nanos6_task_args_main0* %task_args, i8* %loop_bounds, i8* %handler) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %gep_mat = getelementptr %nanos6_task_args_main0, %nanos6_task_args_main0* %task_args, i32 0, i32 0
; CHECK-NEXT:   %load_gep_mat = load [7 x [3 x i32]]*, [7 x [3 x i32]]** %gep_mat
; CHECK-NEXT:   call void @nanos6_unpacked_deps_main0([7 x [3 x i32]]* %load_gep_mat, i8* %loop_bounds, i8* %handler)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

%struct.S = type { i32, i32 }

@s = common dso_local global %struct.S zeroinitializer, align 4

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @foo1() !dbg !11 {
entry:
  %0 = call token @llvm.directive.region.entry() [ "DIR.OSS"([5 x i8] c"TASK\00"), "QUAL.OSS.SHARED"(%struct.S* @s), "QUAL.OSS.DEP.IN"(i32* getelementptr inbounds (%struct.S, %struct.S* @s, i32 0, i32 0), i64 4, i64 0, i64 4), "QUAL.OSS.DEP.IN"(i32* getelementptr inbounds (%struct.S, %struct.S* @s, i32 0, i32 1), i64 4, i64 0, i64 4) ], !dbg !12
  call void @llvm.directive.region.exit(token %0), !dbg !12
  ret i32 0, !dbg !12
}

; CHECK: define internal void @nanos6_unpacked_deps_foo10(%struct.S* %s, i8* %loop_bounds, i8* %handler) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %0 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
; CHECK-NEXT:   %1 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
; CHECK-NEXT:   %2 = bitcast i32* %0 to i8*
; CHECK-NEXT:   call void @nanos6_register_region_read_depinfo1(i8* %handler, i32 0, i8* null, i8* %2, i64 4, i64 0, i64 4)
; CHECK-NEXT:   %3 = bitcast i32* %1 to i8*
; CHECK-NEXT:   call void @nanos6_register_region_read_depinfo1(i8* %handler, i32 0, i8* null, i8* %3, i64 4, i64 0, i64 4)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK: define internal void @nanos6_ol_deps_foo10(%nanos6_task_args_foo10* %task_args, i8* %loop_bounds, i8* %handler) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %gep_s = getelementptr %nanos6_task_args_foo10, %nanos6_task_args_foo10* %task_args, i32 0, i32 0
; CHECK-NEXT:   %load_gep_s = load %struct.S*, %struct.S** %gep_s
; CHECK-NEXT:   call void @nanos6_unpacked_deps_foo10(%struct.S* %load_gep_s, i8* %loop_bounds, i8* %handler)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

declare token @llvm.directive.region.entry()
declare void @llvm.directive.region.exit(token)

!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "", isOptimized: false, runtimeVersion: 0, emissionKind: NoDebug, enums: !2, nameTableKind: None)
!1 = !DIFile(filename: "array_sections.ll", directory: "")
!2 = !{}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!6 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !7, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!7 = !DISubroutineType(types: !2)
!8 = !DILocation(line: 4, column: 13, scope: !6)

!11 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 3, type: !7, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!12 = !DILocation(line: 6, column: 13, scope: !11)
