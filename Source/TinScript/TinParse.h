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
// TinParse.h : Parsing methods for TinScript
// ====================================================================================================================

#ifndef __TINPARSE_H
#define __TINPARSE_H

// -- includes
#include "string.h"

#include "TinTypes.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations

class CCompileTreeNode;
class CFunctionContext;
class CCodeBlock;
struct tExprParenDepthStack;
class CVariableEntry;
class CFunctionEntry;
class CFunctionOverload;

typedef CHashTable<CVariableEntry> tVarTable;
typedef CHashTable<CFunctionEntry> tFuncTable;
typedef CHashTable<CFunctionOverload> tOverloadTable;

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

enum eBinaryOpType
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

enum eAssignOpType
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

// --------------------------------------------------------------------------------------------------------------------
// -- tuple defining the reserved keywords

#define ReservedKeywordTuple \
	ReservedKeywordEntry(NULL)			\
	ReservedKeywordEntry(if)			\
	ReservedKeywordEntry(else)			\
	ReservedKeywordEntry(do)			\
	ReservedKeywordEntry(while)			\
	ReservedKeywordEntry(switch)		\
	ReservedKeywordEntry(case)		    \
	ReservedKeywordEntry(default)		\
	ReservedKeywordEntry(break)			\
	ReservedKeywordEntry(continue)		\
	ReservedKeywordEntry(for)			\
	ReservedKeywordEntry(return)		\
	ReservedKeywordEntry(schedule)		\
    ReservedKeywordEntry(execute)		\
	ReservedKeywordEntry(repeat)		\
    ReservedKeywordEntry(hash)			\
	ReservedKeywordEntry(create)   		\
	ReservedKeywordEntry(createlocal)   \
	ReservedKeywordEntry(destroy) 		\
	ReservedKeywordEntry(self)   		\
	ReservedKeywordEntry(array_count)   \

enum eReservedKeyword
{
	#define ReservedKeywordEntry(a) KEYWORD_##a,
	ReservedKeywordTuple
	#undef ReservedKeywordEntry

	KEYWORD_COUNT
};

const char** GetReservedKeywords(int32& count);

// --------------------------------------------------------------------------------------------------------------------
// struct for reading tokens
struct tReadToken
{
	tReadToken(const tReadToken& _in)
    {
		inbufptr = _in.inbufptr;
		tokenptr = _in.tokenptr;
		length = _in.length;
        type = _in.type;
		linenumber = _in.linenumber;
	}

	tReadToken(const char* _inbufptr, int32 _linenumber)
    {
		inbufptr = _inbufptr;
		tokenptr = NULL;
		length = 0;
		type = TOKEN_NULL;
		linenumber = _linenumber;
	}

	const char* inbufptr;
	const char* tokenptr;
	int32 length;
	eTokenType type;
	int32 linenumber;

	private:
		tReadToken() { }
};

// ====================================================================================================================
// -- token parsing functions

const char* TokenPrint(tReadToken& token);
const char* SkipWhiteSpace(const char* inbuf, int32& linenumber);
bool8 IsIdentifierChar(const char c, bool8 allownumerics);
eReservedKeyword GetReservedKeywordType(const char* token, int32 length);

bool8 GetToken(tReadToken& token, bool8 expectunaryop = false);
const char* GetToken(const char*& inbuf, int32& length, eTokenType& type,	const char* expectedtoken,
                     int32& linenumber, bool8 expectunaryop);

const char* ReadFileAllocBuf(const char* filename);

// ====================================================================================================================
// -- complex expression parsing functions

CCompileTreeNode*& AppendToRoot(CCompileTreeNode& root);
bool8 ParseStatementBlock(CCodeBlock* codeblock, CCompileTreeNode*& root, tReadToken& filebuf,
                         bool8 requiresbraceclose);
CCodeBlock* ParseFile(CScriptContext* script_context, const char* filename);
CCodeBlock* ParseText(CScriptContext* script_context, const char* filename, const char* filebuf);

bool8 SaveBinary(CCodeBlock* codeblock, const char* binfilename);
CCodeBlock* LoadBinary(CScriptContext* script_context, const char* filename, const char* binfilename, bool8 must_exist,
                       bool8& old_version);

const char* ParseFile_CompileToC(CScriptContext* script_context, const char* filename, int32& source_length);
const char* ParseText_CompileToC(CScriptContext* script_context, const char* filename, const char* filebuf,
                                 int32& source_length);
bool8 SaveToSourceC(const char* script_filename, const char* source_C_filename, const char* source_C,
                    int32 source_length);

bool8 TryParseComment(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseVarDeclaration(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseBreakContinue(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseReturn(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
int32 TryParseUnaryPostOp(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode* var_root);
bool8 TryParseStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link,
                        bool8 is_root_statement = false);
bool8 TryParseExpression(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseIfStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseSwitchStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseWhileLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseDoWhileLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseForLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseFuncDefinition(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseFuncCall(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link,
                      bool8 ismethod);
bool8 TryParseArrayHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseArrayCount(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseSchedule(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseCreateObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseDestroyObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);

// ====================================================================================================================
// -- functions for declaring and finding variables and functions
CVariableEntry* AddVariable(CScriptContext* script_context, tVarTable* curglobalvartable,
                            CFunctionEntry* curfuncdefinition, const char* varname,
                            uint32 varhash, eVarType vartype, int array_size);

CVariableEntry* GetVariable(CScriptContext* script_context, tVarTable* globalVarTable, uint32 ns_hash,
                            uint32 func_or_obj, uint32 var_hash, uint32 array_hash);

CVariableEntry* GetObjectMember(CScriptContext* script_context, CObjectEntry*& oe, uint32 ns_hash,
                                uint32 func_or_obj, uint32 var_hash, uint32 array_hash);

CFunctionEntry* FuncDeclaration(CScriptContext* script_context, uint32 namespacehash,
                                const char* funcname, uint32 funchash, uint32 sig_hash, eFunctionType type);
CFunctionEntry* FuncDeclaration(CScriptContext* script_context, CNamespace* nsentry,
                                const char* funcname, uint32 funchash, uint32 sig_hash, eFunctionType type);

// ====================================================================================================================
// -- debugging methods
const char* GetAssOperatorString(eAssignOpType assop);
const char* GetBinOperatorString(eBinaryOpType bin_op);
const char* GetUnaryOperatorString(eUnaryOpType unary_op);

bool8 DumpFile(const char* filename);

void DumpTree(const CCompileTreeNode* root, int32 indent, bool8 isleft, bool8 isright);
void DestroyTree(CCompileTreeNode* root);

int32 CalcVarTableSize(tVarTable* vartable);
void DumpVarTable(CObjectEntry* oe);
void DumpVarTable(CScriptContext* script_context, CObjectEntry* oe, const tVarTable* vartable);
void DumpFuncTable(CObjectEntry* oe);
void DumpFuncTable(CScriptContext* script_context, const tFuncTable* functable);

// ====================================================================================================================

}  // TinScript

#endif // __TINPARSE_H

// ====================================================================================================================
// eof
// ====================================================================================================================
