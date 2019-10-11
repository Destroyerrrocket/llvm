// RUN: %clang_cc1 -verify -fompss-2 -ast-dump -ferror-limit 100 %s | FileCheck %s
// expected-no-diagnostics

int main() {
    int a;
    int b[5];
    int *c;
    #pragma oss task depend(in : a, b[0], *c)
    { a = b[0] = *c; }
}

// CHECK: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'a' 'int'
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'b' 'int [5]'
// CHECK-NEXT: IntegerLiteral {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' 0
// CHECK-NEXT: UnaryOperator {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue prefix '*' cannot overflow
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' lvalue Var {{[a-z0-9]+}} 'c' 'int *'
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'a' 'int'
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'b' 'int [5]'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' lvalue Var {{[a-z0-9]+}} 'c' 'int *'

int *p;
int foo() {
  #pragma oss task depend(inout: *p)
  {
    #pragma oss task
    {
      *p = 0;
    }
  }
  return 0;
}

// CHECK: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: UnaryOperator {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue prefix '*' cannot overflow
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' lvalue Var {{[a-z0-9]+}} 'p' 'int *'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' lvalue Var {{[a-z0-9]+}} 'p' 'int *'
// CHECK-NEXT: CompoundStmt {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, line:{{[a-z0-9]+}}:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}> 'int *' lvalue Var {{[a-z0-9]+}} 'p' 'int *'

int array[5];
int foo1() {
  #pragma oss task depend(inout: array[2])
  {
    #pragma oss task
    {
      array[2] = 0;
    }
  }
  return 0;
}

// CHECK: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'
// CHECK-NEXT: IntegerLiteral {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' 2
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'
// CHECK-NEXT: CompoundStmt {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, line:{{[a-z0-9]+}}:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'

int foo2() {
  int array[5];
  #pragma oss task depend(inout: array[2])
  {
    #pragma oss task
    {
      array[2] = 0;
    }
  }
  return 0;
}

// CHECK: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'
// CHECK-NEXT: IntegerLiteral {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' 2
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'
// CHECK-NEXT: CompoundStmt {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, line:{{[a-z0-9]+}}:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}> 'int [5]' lvalue Var {{[a-z0-9]+}} 'array' 'int [5]'

void foo3() {
    int array[10];
    int i;
    struct A {
        int x;
    } a;
    int index[10];
    #pragma oss task depend(in: array[i])
    #pragma oss task depend(in: array[i + a.x])
    #pragma oss task depend(in: array[a.x])
    #pragma oss task depend(in: array[index[i]])
    {}
}

// CHECK: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: BinaryOperator {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' '+'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: MemberExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue .x {{[a-z0-9]+}}
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'struct A':'struct A' lvalue Var {{[a-z0-9]+}} 'a' 'struct A':'struct A'
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'struct A':'struct A' lvalue Var {{[a-z0-9]+}} 'a' 'struct A':'struct A'
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: MemberExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue .x {{[a-z0-9]+}}
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'struct A':'struct A' lvalue Var {{[a-z0-9]+}} 'a' 'struct A':'struct A'
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'struct A':'struct A' lvalue Var {{[a-z0-9]+}} 'a' 'struct A':'struct A'
// CHECK-NEXT: OSSTaskDirective {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: OSSDependClause {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: ArraySubscriptExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}> 'int' lvalue
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int *' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'index' 'int [10]'
// CHECK-NEXT: ImplicitCastExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: OSSSharedClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'array' 'int [10]'
// CHECK-NEXT: OSSFirstprivateClause {{[a-z0-9]+}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int [10]' lvalue Var {{[a-z0-9]+}} 'index' 'int [10]'
// CHECK-NEXT: DeclRefExpr {{[a-z0-9]+}} <col:{{[a-z0-9]+}}> 'int' lvalue Var {{[a-z0-9]+}} 'i' 'int'
// CHECK-NEXT: CompoundStmt {{[a-z0-9]+}} <line:{{[a-z0-9]+}}:{{[a-z0-9]+}}, col:{{[a-z0-9]+}}>
void foo4() {
    int array[10][10];
    int a;
    #pragma oss task depend(out : array[0 : 5])
    {}
    #pragma oss task depend(out : array[ : a])
    {}
    #pragma oss task depend(out : array[ : ])
    {}
    #pragma oss task depend(out : array[ : ][ : ])
    {}
}

// CHECK: OSSTaskDirective 0x{{[^ ]*}} <line:175:13, col:48>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:47>
// CHECK-NEXT: OSSArraySectionExpr 0x{{[^ ]*}} <col:35, col:46> '<OmpSs-2 array section type>' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:35> 'int (*)[10]' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:41> 'int' 0
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:45> 'int' 5
// CHECK-NEXT: OSSSharedClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: CompoundStmt 0x{{[^ ]*}} <line:176:5, col:6>
// CHECK-NEXT: OSSTaskDirective 0x{{[^ ]*}} <line:177:13, col:47>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:46>
// CHECK-NEXT: OSSArraySectionExpr 0x{{[^ ]*}} <col:35, col:45> '<OmpSs-2 array section type>' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:35> 'int (*)[10]' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:44> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:44> 'int' lvalue Var 0x{{[^ ]*}} 'a' 'int'
// CHECK-NEXT: OSSSharedClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: OSSFirstprivateClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:44> 'int' lvalue Var 0x{{[^ ]*}} 'a' 'int'
// CHECK-NEXT: CompoundStmt 0x{{[^ ]*}} <line:178:5, col:6>
// CHECK-NEXT: OSSTaskDirective 0x{{[^ ]*}} <line:179:13, col:46>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:45>
// CHECK-NEXT: OSSArraySectionExpr 0x{{[^ ]*}} <col:35, col:44> '<OmpSs-2 array section type>' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:35> 'int (*)[10]' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: OSSSharedClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: CompoundStmt 0x{{[^ ]*}} <line:180:5, col:6>
// CHECK-NEXT: OSSTaskDirective 0x{{[^ ]*}} <line:181:13, col:51>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:50>
// CHECK-NEXT: OSSArraySectionExpr 0x{{[^ ]*}} <col:35, col:49> '<OmpSs-2 array section type>' lvalue
// CHECK-NEXT: OSSArraySectionExpr 0x{{[^ ]*}} <col:35, col:44> '<OmpSs-2 array section type>' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:35> 'int (*)[10]' <ArrayToPointerDecay>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: <<<NULL>>>
// CHECK-NEXT: OSSSharedClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int [10][10]' lvalue Var 0x{{[^ ]*}} 'array' 'int [10][10]'
// CHECK-NEXT: CompoundStmt 0x{{[^ ]*}} <line:182:5, col:6>


void foo5(int *p, int n) {
    #pragma oss task depend(in : [n + 1][n + 2]p)
    {}
    #pragma oss task depend(in : ([n + 1][n + 2]p)[43])
    {}
}

// CHECK: OSSTaskDirective 0x{{[^ ]*}} <line:234:13, col:50>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:49>
// CHECK-NEXT: OSSArrayShapingExpr 0x{{[^ ]*}} <col:34, col:48> 'int [n + 1][n + 2]' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:48> 'int *' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:48> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: BinaryOperator 0x{{[^ ]*}} <col:35, col:39> 'int' '+'
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:35> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:39> 'int' 1
// CHECK-NEXT: BinaryOperator 0x{{[^ ]*}} <col:42, col:46> 'int' '+'
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:42> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:42> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:46> 'int' 2
// CHECK-NEXT: OSSFirstprivateClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:48> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:35> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'

// CHECK: OSSTaskDirective 0x{{[^ ]*}} <line:236:13, col:56>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:55>
// CHECK-NEXT: ArraySubscriptExpr 0x{{[^ ]*}} <col:34, col:54> 'int [n + 2]' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:34, col:50> 'int (*)[n + 2]' <ArrayToPointerDecay>
// CHECK-NEXT: ParenExpr 0x{{[^ ]*}} <col:34, col:50> 'int [n + 1][n + 2]' lvalue
// CHECK-NEXT: OSSArrayShapingExpr 0x{{[^ ]*}} <col:35, col:49> 'int [n + 1][n + 2]' lvalue
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:49> 'int *' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:49> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: BinaryOperator 0x{{[^ ]*}} <col:36, col:40> 'int' '+'
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:36> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:36> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:40> 'int' 1
// CHECK-NEXT: BinaryOperator 0x{{[^ ]*}} <col:43, col:47> 'int' '+'
// CHECK-NEXT: ImplicitCastExpr 0x{{[^ ]*}} <col:43> 'int' <LValueToRValue>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:43> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:47> 'int' 2
// CHECK-NEXT: IntegerLiteral 0x{{[^ ]*}} <col:52> 'int' 43
// CHECK-NEXT: OSSFirstprivateClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:49> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:36> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'

void foo6(int *p, int n) {
    #pragma oss task in(p) out(n)
    {}
}

// CHECK: OSSTaskDirective 0x{{[^ ]*}} <line:279:13, col:34>
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:22, col:26> <oss syntax>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:25> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: OSSDependClause 0x{{[^ ]*}} <col:28, col:33> <oss syntax>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:32> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'
// CHECK-NEXT: OSSFirstprivateClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:25> 'int *' lvalue ParmVar 0x{{[^ ]*}} 'p' 'int *'
// CHECK-NEXT: OSSSharedClause 0x{{[^ ]*}} <<invalid sloc>> <implicit>
// CHECK-NEXT: DeclRefExpr 0x{{[^ ]*}} <col:32> 'int' lvalue ParmVar 0x{{[^ ]*}} 'n' 'int'

