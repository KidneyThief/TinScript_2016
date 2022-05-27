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

#pragma once

// -- includes
#include <string.h>

#include "TinHash.h"
//#include "TinTypes.h"
//#include "TinScript.h"
#include "TinVariableEntry.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations

class CCompileTreeNode;
class CFunctionEntry;
class CFunctionContext;
class CCodeBlock;
struct tExprParenDepthStack;
class CFunctionEntry;
enum class EFunctionCallType;

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
CCodeBlock* ParseFile(CScriptContext* script_context, const char* filename, bool& is_empty);
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
bool8 TryParseForeachLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseFuncDefinition(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseFuncCall(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link,
					   EFunctionCallType call_type);
bool8 TryParseArrayHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseInclude(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseArrayCopy(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseSchedule(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseMathUnaryFunction(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseMathBinaryFunction(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseCreateObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseDestroyObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseHashtableCopy(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseType(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseEnsure(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);
bool8 TryParseEnsureInterface(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link);

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
                                const char* funcname, uint32 funchash, EFunctionType type);
CFunctionEntry* FuncDeclaration(CScriptContext* script_context, CNamespace* nsentry,
                                const char* funcname, uint32 funchash, EFunctionType type);

// ====================================================================================================================
// -- debugging methods
const char* GetAssOperatorString(eAssignOpType assop);
const char* GetBinOperatorString(eBinaryOpType bin_op);
const char* GetUnaryOperatorString(eUnaryOpType unary_op);
const char* GetMathUnaryFuncString(eMathUnaryFunctionType math_unary_func_type);
const char* GetMathBinaryFuncString(eMathBinaryFunctionType math_unary_func_type);

bool8 DumpFile(const char* filename);

void DumpTree(const CCompileTreeNode* root, int32 indent, bool8 isleft, bool8 isright);
void DestroyTree(CCompileTreeNode* root);

int32 CalcVarTableSize(tVarTable* vartable);
void DumpVarTable(CObjectEntry* oe, const char* partial = nullptr);
void DumpVarTable(CScriptContext* script_context, CObjectEntry* oe, const tVarTable* vartable,
                  const char* partial = nullptr);
void DumpFuncEntry(CScriptContext* script_context, CFunctionEntry* fe);
void DumpFuncTable(CObjectEntry* oe, const char* partial = nullptr);
void DumpFuncTable(CScriptContext* script_context, const tFuncTable* functable, const char* partial = nullptr);

// ====================================================================================================================

}  // TinScript

// -- eof -------------------------------------------------------------------------------------------------------------
