// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2013 Tim Andersen
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ------------------------------------------------------------------------------------------------

// ====================================================================================================================
// TinDefines.h : enums etc... used for parsing, compiling, and the VM
// ====================================================================================================================

#pragma once


// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// PARSING
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// -- tuple defining the token types

#define TokenTypeTuple \
	TokenTypeEntry(NULL)			\
	TokenTypeEntry(COMMENT)			\
	TokenTypeEntry(STRING)			\
	TokenTypeEntry(BINOP)			\
	TokenTypeEntry(ASSOP)			\
	TokenTypeEntry(UNARY)			\
	TokenTypeEntry(IDENTIFIER)		\
	TokenTypeEntry(KEYWORD)			\
	TokenTypeEntry(REGTYPE)			\
	TokenTypeEntry(EXPECTED)		\
	TokenTypeEntry(FLOAT)			\
	TokenTypeEntry(INTEGER)			\
	TokenTypeEntry(BOOL)			\
	TokenTypeEntry(NAMESPACE)		\
	TokenTypeEntry(PAREN_OPEN)		\
	TokenTypeEntry(PAREN_CLOSE)		\
	TokenTypeEntry(COMMA)			\
	TokenTypeEntry(SEMICOLON)		\
	TokenTypeEntry(PERIOD)			\
	TokenTypeEntry(COLON)  	        \
	TokenTypeEntry(TERNARY)         \
	TokenTypeEntry(BRACE_OPEN)		\
	TokenTypeEntry(BRACE_CLOSE)		\
	TokenTypeEntry(SQUARE_OPEN)		\
	TokenTypeEntry(SQUARE_CLOSE)	\
	TokenTypeEntry(EOF)				\
	TokenTypeEntry(ERROR)			\

enum eTokenType {
	#define TokenTypeEntry(a) TOKEN_##a,
	TokenTypeTuple
	#undef TokenTypeEntry
};

// --------------------------------------------------------------------------------------------------------------------
// -- tuple defining the binary operators

// -- note:  the two-char tokens must be listed before the single-chars
// -- in addition, the name of the entry (first column) must be the same
// -- as the OP_name of the operation
// -- precedence - the higher the number, the higher up the tree
// -- which means it will be evaluated later, having a lower actual precedence.
// -- http://msdn.microsoft.com/en-us/library/126fe14k(v=VS.71).aspx

#define BinaryOperatorTuple \
	BinaryOperatorEntry(NULL,					"NULL",     0)			\
	BinaryOperatorEntry(BooleanAnd, 		    "&&",       90)			\
	BinaryOperatorEntry(BooleanOr, 		        "||",       90)			\
	BinaryOperatorEntry(CompareEqual,		    "==",       50)			\
	BinaryOperatorEntry(CompareNotEqual,	    "!=",       50)			\
	BinaryOperatorEntry(CompareLessEqual,		"<=",       40)			\
	BinaryOperatorEntry(CompareGreaterEqual,	">=",       40)			\
	BinaryOperatorEntry(BitLeftShift,			"<<",       30)			\
	BinaryOperatorEntry(BitRightShift,			">>",       30)			\
	BinaryOperatorEntry(CompareLess,			"<",        40)			\
	BinaryOperatorEntry(CompareGreater,			">",        40)			\
	BinaryOperatorEntry(Add,					"+",        20)			\
	BinaryOperatorEntry(Sub,					"-",        20)			\
	BinaryOperatorEntry(Mult,					"*",        10)			\
	BinaryOperatorEntry(Div,					"/",        10)			\
	BinaryOperatorEntry(Mod,					"%",        10)			\
	BinaryOperatorEntry(BitAnd, 		        "&",        60)			\
	BinaryOperatorEntry(BitXor,		            "^",        70)			\
	BinaryOperatorEntry(BitOr, 		            "|",        80)			\

enum eBinaryOpType : int16_t
{
	#define BinaryOperatorEntry(a, b, c) BINOP_##a,
	BinaryOperatorTuple
	#undef BinaryOperatorEntry
	BINOP_COUNT
};

#define AssignOperatorTuple \
	AssignOperatorEntry(NULL,					"NULL")			\
	AssignOperatorEntry(AssignAdd,				"+=")			\
	AssignOperatorEntry(AssignSub,				"-=")			\
	AssignOperatorEntry(AssignMult,				"*=")			\
	AssignOperatorEntry(AssignDiv,				"/=")			\
	AssignOperatorEntry(AssignMod,				"%=")			\
	AssignOperatorEntry(AssignLeftShift,    	"<<=")			\
	AssignOperatorEntry(AssignRightShift,    	">>=")			\
	AssignOperatorEntry(AssignBitAnd,    	    "&=")			\
	AssignOperatorEntry(AssignBitOr,    	    "|=")			\
	AssignOperatorEntry(AssignBitXor,    	    "^=")			\
	AssignOperatorEntry(Assign,					"=")			\

enum eAssignOpType : int16_t
{
	#define AssignOperatorEntry(a, b) ASSOP_##a,
	AssignOperatorTuple
	#undef AssignOperatorEntry
	ASSOP_COUNT
};

#define UnaryOperatorTuple \
    UnaryOperatorEntry(NULL,                    "NULL")        \
    UnaryOperatorEntry(UnaryPreInc,             "++")          \
    UnaryOperatorEntry(UnaryPreDec,             "--")          \
    UnaryOperatorEntry(UnaryBitInvert,          "~")           \
    UnaryOperatorEntry(UnaryNot,                "!")           \
    UnaryOperatorEntry(UnaryNeg,                "-")           \
    UnaryOperatorEntry(UnaryPos,                "+")           \

enum eUnaryOpType
{
	#define UnaryOperatorEntry(a, b) UNARY_##a,
	UnaryOperatorTuple
	#undef UnaryOperatorEntry
	UNARY_COUNT
};

constexpr float _PI = 3.1415926525f;
constexpr float _2_PI = _PI * 2.0f;

#define MathKeywordConstantTuple				\
	MathKeywordConstantEntry(pi, 3.1415926535f)	\

#define MathKeywordUnaryTuple																					\
	MathKeywordUnaryEntry(abs,		[](float inValue) { return (inValue >= 0.0f ? inValue : -inValue); })		\
	MathKeywordUnaryEntry(floor,	[](float inValue) { return (float)floor(inValue); } )						\
	MathKeywordUnaryEntry(ceil,		[](float inValue) { return (float)ceil(inValue); } )						\
	MathKeywordUnaryEntry(round,	[](float inValue) { return (float)round(inValue); } )						\
	MathKeywordUnaryEntry(rad,		[](float inValue) { while (inValue < -180.0f) inValue += 360.0f;  while (inValue > 180.0f) inValue -= 360.0f; return (inValue * 3.1415926535f / 180.0f); } )	\
	MathKeywordUnaryEntry(deg,		[](float inValue) { while (inValue < -_PI) inValue += _2_PI;  while (inValue > _PI) inValue -= _2_PI; return (inValue * 180.0f / _PI); } )						\
	MathKeywordUnaryEntry(sin,		[](float inValue) { return (float)sin(inValue); } )							\
	MathKeywordUnaryEntry(cos,		[](float inValue) { return (float)cos(inValue); } )							\
	MathKeywordUnaryEntry(tan,		[](float inValue) { return (float)tan(inValue); } )							\
	MathKeywordUnaryEntry(asin,		[](float inValue) { return (float)asin(inValue); } )						\
	MathKeywordUnaryEntry(acos,		[](float inValue) { return (float)acos(inValue); } )						\
	MathKeywordUnaryEntry(atan,		[](float inValue) { return(float)atan(inValue); } )							\
	MathKeywordUnaryEntry(sqr,		[](float inValue) { return inValue * inValue; } )							\
	MathKeywordUnaryEntry(sqrt,		[](float inValue) { return (float)sqrt(inValue); } )						\
	MathKeywordUnaryEntry(exp,		[](float inValue) { return (float)exp(inValue); } )							\
	MathKeywordUnaryEntry(loge,		[](float inValue) { return (float)log(inValue); } )							\
	MathKeywordUnaryEntry(log10,	[](float inValue) { return (float)log10(inValue); } )						\

enum eMathUnaryFunctionType
{
	#define MathKeywordUnaryEntry(a, b) eMathUnary_##a,
	MathKeywordUnaryTuple
	#undef MathKeywordUnaryEntry
    MATH_UNARY_FUNC_COUNT
};

#define MathKeywordBinaryTuple																		\
	MathKeywordBinaryEntry(min,		[](float a, float b) { return a < b ? a : b; } )				\
	MathKeywordBinaryEntry(max,		[](float a, float b) { return a > b ? a : b; } )				\
	MathKeywordBinaryEntry(pow,		[](float a, float b) { return (float)pow(a, b); } )				\
	MathKeywordBinaryEntry(atan2,	[](float a, float b) { return (float)atan2(a, b); } )			\

enum eMathBinaryFunctionType
{
	#define MathKeywordBinaryEntry(a, b) eMathBinary_##a,
	MathKeywordBinaryTuple
	#undef MathKeywordBinaryEntry
    MATH_BINARY_FUNC_COUNT
};

// --------------------------------------------------------------------------------------------------------------------
// -- tuple defining the reserved keywords

#define ReservedKeywordTuple \
	ReservedKeywordEntry(NULL)			        \
	ReservedKeywordEntry(if)			        \
	ReservedKeywordEntry(else)			        \
	ReservedKeywordEntry(do)			        \
	ReservedKeywordEntry(while)			        \
	ReservedKeywordEntry(switch)		        \
	ReservedKeywordEntry(case)		            \
	ReservedKeywordEntry(default)		        \
	ReservedKeywordEntry(break)			        \
	ReservedKeywordEntry(continue)		        \
	ReservedKeywordEntry(for)			        \
	ReservedKeywordEntry(foreach)			    \
	ReservedKeywordEntry(return)		        \
    ReservedKeywordEntry(schedule)		        \
    ReservedKeywordEntry(execute)		        \
    ReservedKeywordEntry(repeat)		        \
    ReservedKeywordEntry(hash)			        \
    ReservedKeywordEntry(include)		        \
	ReservedKeywordEntry(create)   		        \
	ReservedKeywordEntry(create_local)          \
	ReservedKeywordEntry(destroy) 		        \
	ReservedKeywordEntry(self)   		        \
	ReservedKeywordEntry(hashtable_copy)        \
	ReservedKeywordEntry(hashtable_wrap)        \
	ReservedKeywordEntry(type)                  \
	ReservedKeywordEntry(ensure)                \
	ReservedKeywordEntry(super)                 \
	ReservedKeywordEntry(interface)             \
	ReservedKeywordEntry(ensure_interface)      \

enum eReservedKeyword
{
	#define ReservedKeywordEntry(a) KEYWORD_##a,
	#define MathKeywordUnaryEntry(a, b) KEYWORD_##a,
	#define MathKeywordBinaryEntry(a, b) KEYWORD_##a,

	ReservedKeywordTuple
	MathKeywordUnaryTuple
    MathKeywordBinaryTuple

	#undef MathKeywordBinaryEntry
	#undef MathKeywordUnaryEntry
	#undef ReservedKeywordEntry

	KEYWORD_COUNT
};

// ====================================================================================================================
// COMPILING
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// compile tree node types
#define CompileNodeTypesTuple					\
	CompileNodeTypeEntry(NOP)					\
	CompileNodeTypeEntry(Comment)				\
	CompileNodeTypeEntry(BinaryNOP)			    \
	CompileNodeTypeEntry(DebugNOP)			    \
	CompileNodeTypeEntry(Ensure)  	            \
	CompileNodeTypeEntry(EnsureInterface)  	    \
	CompileNodeTypeEntry(IncludeScript)         \
	CompileNodeTypeEntry(Type)  	            \
	CompileNodeTypeEntry(Value)					\
	CompileNodeTypeEntry(Self)       			\
	CompileNodeTypeEntry(ObjMember) 			\
	CompileNodeTypeEntry(PODMember) 			\
	CompileNodeTypeEntry(PODMethod) 			\
	CompileNodeTypeEntry(Assignment)			\
	CompileNodeTypeEntry(BinaryOp)				\
	CompileNodeTypeEntry(UnaryOp)				\
	CompileNodeTypeEntry(SwitchStmt)    		\
	CompileNodeTypeEntry(CaseStmt)    		    \
	CompileNodeTypeEntry(IfStmt)				\
	CompileNodeTypeEntry(CondBranch)			\
	CompileNodeTypeEntry(WhileLoop)				\
	CompileNodeTypeEntry(ForLoop)				\
	CompileNodeTypeEntry(ForeachLoop)			\
	CompileNodeTypeEntry(ForeachIterNext)		\
	CompileNodeTypeEntry(LoopJump)        		\
	CompileNodeTypeEntry(FuncDecl)				\
	CompileNodeTypeEntry(FuncCall)				\
	CompileNodeTypeEntry(FuncReturn)			\
	CompileNodeTypeEntry(ObjMethod) 			\
    CompileNodeTypeEntry(Sched) 				\
    CompileNodeTypeEntry(SchedParam)    		\
    CompileNodeTypeEntry(SchedFunc)     		\
	CompileNodeTypeEntry(ArrayHash) 			\
	CompileNodeTypeEntry(ArrayVar)			    \
	CompileNodeTypeEntry(ArrayVarDecl)			\
	CompileNodeTypeEntry(ArrayDecl)			    \
	CompileNodeTypeEntry(MathUnaryFunc)			\
	CompileNodeTypeEntry(MathBinaryFunc)		\
	CompileNodeTypeEntry(HashtableCopy)	    	\
	CompileNodeTypeEntry(SelfVarDecl)			\
	CompileNodeTypeEntry(ObjMemberDecl)			\
	CompileNodeTypeEntry(Schedule)			    \
	CompileNodeTypeEntry(CreateObject)  	    \
	CompileNodeTypeEntry(DestroyObject)  	    \

// enum to mark the nodes for debug output
enum ECompileNodeType
{
#define CompileNodeTypeEntry(a) e##a,
	CompileNodeTypesTuple
#undef CompileNodeTypeEntry

	eNodeTypeCount
};

// --------------------------------------------------------------------------------------------------------------------
// -- used when we're parsing, to determine the context of the function call
enum class EFunctionCallType
{
	None,
	Global,
	ObjMethod,
	PODMethod,
	Super,
	Count
};

// --------------------------------------------------------------------------------------------------------------------
// operation types

#define OperationTuple					\
	OperationEntry(NULL)				\
	OperationEntry(NOP)					\
	OperationEntry(DebugMsg)			\
	OperationEntry(Include)				\
	OperationEntry(Ensure)				\
	OperationEntry(EnsureInterface)		\
	OperationEntry(Type)				\
	OperationEntry(VarDecl)				\
	OperationEntry(ParamDecl)			\
	OperationEntry(Assign)				\
	OperationEntry(PushAssignValue)     \
	OperationEntry(PushParam)			\
	OperationEntry(Push)				\
	OperationEntry(PushCopy)			\
	OperationEntry(PushLocalVar)		\
	OperationEntry(PushLocalValue)		\
	OperationEntry(PushGlobalVar)		\
	OperationEntry(PushGlobalValue)		\
	OperationEntry(PushArrayVar)     	\
	OperationEntry(PushArrayValue)     	\
	OperationEntry(PushMember)			\
	OperationEntry(PushMemberVal)		\
	OperationEntry(PushPODMember)       \
	OperationEntry(PushPODMemberVal)    \
	OperationEntry(PushSelf)    		\
	OperationEntry(Pop)					\
	OperationEntry(ForeachIterInit)		\
	OperationEntry(ForeachIterNext)		\
	OperationEntry(Add)					\
	OperationEntry(Sub)					\
	OperationEntry(Mult)				\
	OperationEntry(Div)					\
	OperationEntry(Mod)					\
	OperationEntry(AssignAdd)			\
	OperationEntry(AssignSub)			\
	OperationEntry(AssignMult)			\
	OperationEntry(AssignDiv)			\
	OperationEntry(AssignMod)			\
	OperationEntry(AssignLeftShift)		\
	OperationEntry(AssignRightShift)	\
	OperationEntry(AssignBitAnd)        \
	OperationEntry(AssignBitOr)         \
	OperationEntry(AssignBitXor)        \
	OperationEntry(BooleanAnd)      	\
	OperationEntry(BooleanOr)         	\
	OperationEntry(CompareEqual)		\
	OperationEntry(CompareNotEqual)		\
	OperationEntry(CompareLess)			\
	OperationEntry(CompareLessEqual)	\
	OperationEntry(CompareGreater)		\
	OperationEntry(CompareGreaterEqual)	\
	OperationEntry(BitLeftShift)        \
	OperationEntry(BitRightShift)       \
	OperationEntry(BitAnd)	            \
	OperationEntry(BitOr)	            \
	OperationEntry(BitXor)	            \
	OperationEntry(UnaryPreInc)	        \
	OperationEntry(UnaryPreDec)	        \
	OperationEntry(UnaryPostInc)        \
	OperationEntry(UnaryPostDec)        \
	OperationEntry(UnaryBitInvert)	    \
	OperationEntry(UnaryNot)	        \
	OperationEntry(UnaryNeg)	        \
	OperationEntry(UnaryPos)	        \
	OperationEntry(Branch)				\
	OperationEntry(BranchCond)			\
	OperationEntry(FuncDecl)    		\
	OperationEntry(FuncDeclEnd)    		\
	OperationEntry(FuncCallArgs)		\
	OperationEntry(FuncCall)			\
	OperationEntry(FuncReturn)			\
	OperationEntry(MethodCallArgs)		\
	OperationEntry(PODCallArgs)			\
	OperationEntry(PODCallComplete)		\
	OperationEntry(ArrayHash)			\
	OperationEntry(ArrayVarDecl)		\
	OperationEntry(ArrayDecl)		    \
	OperationEntry(MathUnaryFunc)		\
	OperationEntry(MathBinaryFunc)		\
	OperationEntry(HashtableCopy)		\
	OperationEntry(SelfVarDecl)		    \
	OperationEntry(ObjMemberDecl)       \
	OperationEntry(ScheduleBegin)       \
	OperationEntry(ScheduleParam)       \
	OperationEntry(ScheduleEnd)         \
	OperationEntry(CreateObject)		\
	OperationEntry(DestroyObject)		\
	OperationEntry(EOF)					\

enum eOpCode : int16
{
#define OperationEntry(a) OP_##a,
	OperationTuple
#undef OperationEntry
	OP_COUNT
};

}  // TinScript

// -- eof -------------------------------------------------------------------------------------------------------------
