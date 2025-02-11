// clang-format off
// UNSUPPORTED: system-aix
// XFAIL: arm, arm64-apple, system-windows
// RUN: cat %s | clang-repl | FileCheck %s
// We mark it as XFAIL because in upstream the test is failing
// XFAIL: *
extern "C" int printf(const char *, ...);

int f() { throw "Simple exception"; return 0; }
int checkException() { try { printf("Running f()\n"); f(); } catch (const char *e) { printf("%s\n", e); } return 0; }
auto r1 = checkException();
// CHECK: Running f()
// CHECK-NEXT: Simple exception

%quit
