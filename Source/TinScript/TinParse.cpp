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
// TinParse.cpp : Parses text and creates the tree of nodes, to be compiled
// ====================================================================================================================

// -- includes
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"

#include "windows.h"

#include "TinTypes.h"
#include "TinHash.h"
#include "TinParse.h"
#include "TinCompile.h"
#include "TinExecute.h"
#include "TinStringTable.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// -- statics
// -- string delineators
static const int32 kNumQuoteChars = 3;
static const char gQuoteChars[kNumQuoteChars + 1] = "\"'`";

// -- statics to prevent re-entrant parsing
static int32 gGlobalExprParenDepth = 0;
static bool8 gGlobalReturnStatement = false;
static bool8 gGlobalDestroyStatement = false;
static bool8 gGlobalCreateStatement = false;

// -- stack for managing loops (break and continue statments need to know where to jump
static const int32 gMaxWhileLoopDepth = 32;
static int32 gWhileLoopDepth = 0;
static CWhileLoopNode* gWhileLoopStack[gMaxWhileLoopDepth];

// ====================================================================================================================
// -- binary operators
static const char* gBinOperatorString[] =
{
	#define BinaryOperatorEntry(a, b, c) b,
	BinaryOperatorTuple
	#undef BinaryOperatorEntry
};

eBinaryOpType GetBinaryOpType(const char* token, int32 length)
{
	for (eBinaryOpType i = BINOP_NULL; i < BINOP_COUNT; i = (eBinaryOpType)(i + 1))
    {
		int32 comparelength = int32(strlen(gBinOperatorString[i])) > length
                              ? (int32)strlen(gBinOperatorString[i])
                              : length;
		if (!Strncmp_(token, gBinOperatorString[i], comparelength))
        {
			return i;
		}
	}

    // -- not found
	return BINOP_NULL;
}

// ====================================================================================================================
// -- assignment operators
static const char* gAssOperatorString[] =
{
	#define AssignOperatorEntry(a, b) b,
	AssignOperatorTuple
	#undef AssignOperatorEntry
};

const char* GetAssOperatorString(eAssignOpType assop)
{
	return gAssOperatorString[assop];
}

eAssignOpType GetAssignOpType(const char* token, int32 length)
{
	for (eAssignOpType i = ASSOP_NULL; i < ASSOP_COUNT; i = (eAssignOpType)(i + 1))
    {
		int32 comparelength = int32(strlen(gAssOperatorString[i])) > length
                              ? (int32)strlen(gAssOperatorString[i])
                              : length;
		if (!Strncmp_(token, gAssOperatorString[i], comparelength))
        {
			return i;
		}
	}

    // -- not found
	return ASSOP_NULL;
}

// ====================================================================================================================
// -- unary operators
static const char* gUnaryOperatorString[] =
{
	#define UnaryOperatorEntry(a, b) b,
	UnaryOperatorTuple
	#undef UnaryOperatorEntry
};

const char* GetUnaryOperatorString(eUnaryOpType unaryop)
{
	return gUnaryOperatorString[unaryop];
}

eUnaryOpType GetUnaryOpType(const char* token, int32 length)
{
	for (eUnaryOpType i = UNARY_NULL; i < UNARY_COUNT; i = (eUnaryOpType)(i + 1))
    {
		int32 comparelength = int32(strlen(gUnaryOperatorString[i])) > length
                            ? (int32)strlen(gAssOperatorString[i])
                            : length;
		if (!Strncmp_(token, gUnaryOperatorString[i], comparelength))
        {
			return i;
		}
	}

    // -- not found
	return UNARY_NULL;
}

// ====================================================================================================================
// -- reserved keywords
const char* gReservedKeywords[KEYWORD_COUNT] =
{
	#define ReservedKeywordEntry(a) #a,
	ReservedKeywordTuple
	#undef ReservedKeywordEntry
};

eReservedKeyword GetReservedKeywordType(const char* token, int32 length)
{
	for (eReservedKeyword i = KEYWORD_NULL; i < KEYWORD_COUNT; i = (eReservedKeyword)(i + 1))
    {
		int32 comparelength = int32(strlen(gReservedKeywords[i])) > length
                              ? (int32)strlen(gReservedKeywords[i])
                              : length;
		if (!Strncmp_(token, gReservedKeywords[i], comparelength))
        {
			return i;
		}
	}

    // -- not found
	return KEYWORD_NULL;
}

// ====================================================================================================================
bool8 IsFirstClassValue(eTokenType type, eVarType& vartype)
{
    if (type == TOKEN_FLOAT)
    {
        vartype = TYPE_float;
        return (true);
    }

    if (type == TOKEN_INTEGER)
    {
        vartype = TYPE_int;
        return (true);
    }

    if (type == TOKEN_BOOL)
    {
        vartype = TYPE_bool;
        return (true);
    }

    if (type == TOKEN_STRING)
    {
        vartype = TYPE_string;
        return (true);
    }

    return (false);
}

bool8 IsAssignBinOp(eOpCode optype)
{
	return (optype == OP_Assign || optype == OP_AssignAdd || optype == OP_AssignSub ||
			optype == OP_AssignMult || optype == OP_AssignDiv || optype == OP_AssignMod);
}

// ====================================================================================================================
// TokenPrint():  Debug function for printing the contents of a token.
// ====================================================================================================================
const char* TokenPrint(tReadToken& token)
{
    if (!token.tokenptr || token.length <= 0)
        return ("");

    char* bufferptr = TinScript::GetContext()->GetScratchBuffer();
    SafeStrcpy(bufferptr, token.tokenptr, token.length + 1);
    return (bufferptr);
}

// ====================================================================================================================
// SKipWhiteSpace():  Method to advance a token pointer past irrelevant whitespace.
// ====================================================================================================================
bool8 SkipWhiteSpace(tReadToken& token)
{
    const char*& inbuf = token.inbufptr;
    int32& linenumber = token.linenumber;
	if (!inbuf)
		return (false);

    // -- we're going to count comments as whitespace
    bool8 foundcomment = false;
    do
    {
        foundcomment = false;

        // -- first skip actual whitespace
	    while (*inbuf == ' ' || *inbuf == '\t' || *inbuf == '\r' || *inbuf == '\n')
        {
		    if (*inbuf == '\n')
			    ++linenumber;
		    ++inbuf;
	    }

        // -- next comes block comments
	    if (inbuf[0] == '/' && inbuf[1] == '*')
        {
            foundcomment = true;
            inbuf += 2;
		    while (inbuf[0] != '\0' && inbuf[1] != '\0')
            {
                if (inbuf[0] == '*' && inbuf[1] == '/')
                {
                    inbuf += 2;
                    break;
                }

                if (inbuf[0] == '\n')
                    ++linenumber;
                ++inbuf;
            }
	    }

	    // -- skip line comments
	    if (inbuf[0] == '/' && inbuf[1] == '/')
        {
            foundcomment = true;
            inbuf += 2;
		    while (*inbuf != '\0' && *inbuf != '\n')
			    ++inbuf;
	    }

    } while (foundcomment);

    // -- success
	return (true);
}

// ====================================================================================================================
// IsIdentifierChar():  Returns true, if the character can be part of an identifier.
// ====================================================================================================================
bool8 IsIdentifierChar(const char c, bool8 allownumerics)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (allownumerics && c >= '0' && c <= '9'))
    {
		return (true);
	}

    // -- not a valid identifier character
	return (false);
}

// ====================================================================================================================
const char* gTokenTypeStrings[] =
{
	#define TokenTypeEntry(a) #a,
	TokenTypeTuple
	#undef TokenTypeEntry
};

// -- note:  the order must match the defined TokenTypeTuple in TinParse.h, starting at '('
static const int32 kNumSymbols = 10;
const char symbols[kNumSymbols + 1] = "(),;.:{}[]";

// ====================================================================================================================
// GetToken():  Reads the next token, skipping whitespace.
// ====================================================================================================================
bool8 GetToken(tReadToken& token, bool8 unaryop)
{
    if (!SkipWhiteSpace(token))
        return (false);
	token.tokenptr = GetToken(token.inbufptr, token.length, token.type, NULL, token.linenumber, unaryop);
	return (token.tokenptr != NULL);
}

// ====================================================================================================================
// GetToken():  Reads the actual token, including determining the token type.
// ====================================================================================================================
const char* GetToken(const char*& inbuf, int32& length, eTokenType& type, const char* expectedtoken,
					 int32& linenumber, bool8 expectunaryop)
{
	// -- initialize the return results
	length = 0;
	type = TOKEN_NULL;

	if (!inbuf)
		return (NULL);

	// -- check for NULL ptr, or eof ptr
	const char* tokenptr = inbuf;
	if (!tokenptr)
		return (NULL);
	else if (tokenptr == '\0')
		return (tokenptr);

	// -- see if we have the expected token
	if (expectedtoken && expectedtoken[0] != '\0')
    {
		int32 expectedlength = (int32)strlen(expectedtoken);
		if (! Strncmp_(tokenptr, expectedtoken, expectedlength))
        {
			length = expectedlength;
			type = TOKEN_EXPECTED;
			inbuf = tokenptr + length;
			return (tokenptr);
		}
	}

	// -- look for an opening string
    // -- we allow multiple delineators to define a string, but the start and the end must match
	char quotechar = '\0';
	for (int32 i = 0; i < kNumQuoteChars; ++i)
    {
		if (*tokenptr == gQuoteChars[i])
        {
			quotechar = gQuoteChars[i];
			break;
		}
	}

	// -- if we found a string, find the end, and return the stripped string
	if (quotechar != '\0')
    {
		++tokenptr;
		const char* stringend = tokenptr;
		while (*stringend != quotechar && *stringend != '\0')
			++stringend;
		if (*stringend == '\0')
			return (NULL);

		// -- return results
		length = (int32)kPointerDiffUInt32(stringend, tokenptr);
		type = TOKEN_STRING;
		inbuf = stringend + 1;
		return (tokenptr);
	}

	// -- see if we have a bool8
	if (!Strncmp_(tokenptr, "false", 5))
    {
		if (!IsIdentifierChar(tokenptr[5], true))
        {
			length = 5;
			type = TOKEN_BOOL;
			inbuf = tokenptr + length;
			return (tokenptr);
		}
	}
	else if (!Strncmp_(tokenptr, "true", 4))
    {
		if (!IsIdentifierChar(tokenptr[4], true))
        {
			length = 4;
			type = TOKEN_BOOL;
			inbuf = tokenptr + length;
			return (tokenptr);
		}
	}

	// -- see if we have an identifier
	if (IsIdentifierChar(*tokenptr, false))
    {
		const char* tokenendptr = tokenptr + 1;
		while (IsIdentifierChar(*tokenendptr, true))
			++tokenendptr;

		// -- return the result
		length = (int32)kPointerDiffUInt32(tokenendptr, tokenptr);

		// -- see if the identifier is a keyword
		bool8 foundidtype = false;
		if (! foundidtype)
        {
			int32 reservedwordtype = GetReservedKeywordType(tokenptr, length);
			if (reservedwordtype != KEYWORD_NULL)
            {
				type = TOKEN_KEYWORD;
				foundidtype = true;
			}
		}

		if (!foundidtype)
        {
			eVarType registeredtype = GetRegisteredType(tokenptr, length);
			if (registeredtype != TYPE_NULL)
            {
				type = TOKEN_REGTYPE;
				foundidtype = true;
			}
		}

		if (!foundidtype)
        {
			type = TOKEN_IDENTIFIER;
		}

		inbuf = tokenendptr;
		return (tokenptr);
	}

    // -- a unary op takes precedence over a binary/assign op, but is only
    // -- valid at the beginning of an expression.  If we're expecting a unary
    // -- unary op, and we found one, return immediately, otherwise return after
    // -- we've ruled out assignment and binary ops
    bool8 unaryopfound = false;
    int32 unaryoplength = 0;
    for (int32 i = 0; i < UNARY_COUNT; ++i)
    {
		int32 operatorlength = (int32)strlen(gUnaryOperatorString[i]);
		if (!Strncmp_(tokenptr, gUnaryOperatorString[i], operatorlength))
        {
			unaryoplength = operatorlength;
            unaryopfound = true;
            break;
		}
    }

    if (unaryopfound && expectunaryop)
    {
        length = unaryoplength;
        inbuf = tokenptr + length;
        type = TOKEN_UNARY;
	    return (tokenptr);
    }

	// -- see if we have an assignment op
	// -- note:  must search for assignment ops first, or '+=' assign op
	// -- will be mistaken for '+' binary op
    // -- with one exception...  ensure if we find '=' it's not '=='
	for (int32 i = 0; i < ASSOP_COUNT; ++i)
    {
		int32 operatorlength = (int32)strlen(gAssOperatorString[i]);
		if (!Strncmp_(tokenptr, gAssOperatorString[i], operatorlength))
        {
            // -- handle the exception - if we find '=', ensure i's not '=='
            if (i == ASSOP_Assign)
            {
        		int32 operatorlength = (int32)strlen(gBinOperatorString[BINOP_CompareEqual]);
        		if (!Strncmp_(tokenptr, gBinOperatorString[BINOP_CompareEqual], operatorlength))
                {
                    continue;
                }
            }

			length = operatorlength;
			type = TOKEN_ASSOP;
			inbuf = tokenptr + length;
			return (tokenptr);
		}
	}

	// -- see if we have a binary op
	for (int32 i = 0; i < BINOP_COUNT; ++i)
    {
		int32 operatorlength = (int32)strlen(gBinOperatorString[i]);
		if (!Strncmp_(tokenptr, gBinOperatorString[i], operatorlength))
        {
			length = operatorlength;
			type = TOKEN_BINOP;
			inbuf = tokenptr + length;
			return (tokenptr);
		}
	}

    // -- if we weren't expecting a unary op, we still need that we found one,
    // -- after we've ruled out assign/binary ops
    if (unaryopfound)
    {
        length = unaryoplength;
        inbuf = tokenptr + length;
        type = TOKEN_UNARY;
	    return (tokenptr);
    }

	// -- see if we have a namespace '::'
	if (tokenptr[0] == ':' && tokenptr[1] == ':')
    {
		length = 2;
		type = TOKEN_NAMESPACE;
		inbuf = tokenptr + 2;
		return (tokenptr);
	}

    // -- see if we have a hex integer
    const char* hexptr = tokenptr;
    if (*hexptr == '0' && (hexptr[1] == 'x' || hexptr[1] == 'X')) {
        hexptr += 2;
        while ((*hexptr >= '0' && *hexptr <= '9') || (*hexptr >= 'a' && *hexptr <= 'f') ||
               (*hexptr >= 'A' && *hexptr <= 'F'))
        {
            ++hexptr;
        }

        // -- initialize the return values for a float32
        length = (int32)kPointerDiffUInt32(hexptr, tokenptr);
        if (length >= 3 || length <= 10)
        {
            type = TOKEN_INTEGER;
            inbuf = hexptr;
            return (tokenptr);
        }
    }

    // -- see if we have a binary integer
    const char* binaryptr = tokenptr;
    if (*binaryptr == '0' && (binaryptr[1] == 'b' || binaryptr[1] == 'B'))
    {
        binaryptr += 2;
        while (*binaryptr >= '0' && *binaryptr <= '1')
            ++binaryptr;

        // -- initialize the return values for a float32
        length = (int32)kPointerDiffUInt32(binaryptr, tokenptr);
        if (length >= 3)
        {
            type = TOKEN_INTEGER;
            inbuf = binaryptr;
            return (tokenptr);
        }
    }

	// -- see if we have a float32 or an integer
	const char* numericptr = tokenptr;
	while (*numericptr >= '0' && *numericptr <= '9')
		++numericptr;

	if (numericptr > tokenptr)
    {
		// -- see if we have a float32, or an integer
		if (*numericptr == '.' && numericptr[1] >= '0' && numericptr[1] <= '9')
        {
			++numericptr;
			while (*numericptr >= '0' && *numericptr <= '9')
				++numericptr;

			// -- initialize the return values for a float32
			length = (int32)kPointerDiffUInt32(numericptr, tokenptr);
			type = TOKEN_FLOAT;
			inbuf = numericptr;

			// -- see if we need to read the final 'f'
			if (*numericptr == 'f')
				++inbuf;

			return (tokenptr);
		}

		// -- else an integer
		else
        {
            length = (int32)kPointerDiffUInt32(numericptr, tokenptr);
			type = TOKEN_INTEGER;
			inbuf = numericptr;
			return (tokenptr);
		}
	}

	// -- see if we have a symbol
	for (int32 i = 0; i < kNumSymbols; ++i)
    {
		if (*tokenptr == symbols[i])
        {
			length = 1;
			type = eTokenType(TOKEN_PAREN_OPEN + i);
			inbuf = tokenptr + 1;
			return (tokenptr);
		}
	}

	// -- nothing left to parse - ensure we're at eof
	if (*tokenptr == '\0')
    {
		length = 0;
		type = TOKEN_EOF;
		inbuf = NULL;
		return (NULL);
	}

	// -- error
    // $$$TZA Probably should restrict parsing of files to only the MainThread...
    ScriptAssert_(TinScript::GetContext(), 0, "<internal>", linenumber,
                  "Error - unable to parse: %s\n", tokenptr);
	length = 0;
	type = TOKEN_ERROR;
	inbuf = NULL;
	return (NULL);
}

// ====================================================================================================================
// ReadFileAllocBuf():  Opens a file, allocates a buffer and reads the contents,
// ====================================================================================================================
const char* ReadFileAllocBuf(const char* filename)
{
	// -- open the file
	FILE* filehandle = NULL;
	if (filename)
    {
		 int32 result = fopen_s(&filehandle, filename, "r");
		 if (result != 0)
			 return (NULL);
	}

	if (!filehandle)
		return (NULL);

	// -- get the size of the file
	int32 result = fseek(filehandle, 0, SEEK_END);
	if (result != 0)
    {
		fclose(filehandle);
		return (NULL);
	}

	int32 filesize = ftell(filehandle);
	if (filesize <= 0)
    {
		fclose(filehandle);
		return (NULL);
	}
	fseek(filehandle, 0, SEEK_SET);

	// -- allocate a buffer and read the file into it (will null terminate)
	char* filebuf = (char*)TinAllocArray(ALLOC_FileBuf, char, filesize + 1);
	int32 bytesread = (int32)fread(filebuf, 1, filesize, filehandle);

    // $$$TZA for some reason, my text file is taking more space on disk than what is actually read...
	//if (bytesread != filesize) {
	if (bytesread <= 0)
    {
		delete[] filebuf;
		fclose(filehandle);
		return (NULL);
	}
	filebuf[bytesread] = '\0';

	// -- close the file before we leave
	fclose(filehandle);

	// -- success
	return (filebuf);
}

// ====================================================================================================================
// DumpFile():  Debhug function to open and read a file, then print out contents as it is tokenized.
// ====================================================================================================================
bool8 DumpFile(const char* filename)
{
	// -- see if we can open the file
	const char* filebuf = ReadFileAllocBuf(filename);
	if (! filebuf)
    {
		return (false);
	}

	// now parse the file - print out each token we found
	tReadToken token(filebuf, 0);
	bool8 success = false;
	do {
		success = GetToken(token);
		char tokenbuf[kMaxTokenLength] = { 0 };
		if (token.tokenptr != NULL)
        {
			SafeStrcpy(tokenbuf, token.tokenptr, token.length + 1);
			printf("Found token: [%s] %s\n", gTokenTypeStrings[token.type], tokenbuf);
		}
	} while (success);

	return (true);
}

// ====================================================================================================================
// DumpTree():  Debug function to "draw" the tree created from parsing a file.
// ====================================================================================================================
void DumpTree(const CCompileTreeNode* root, int32 indent, bool8 isleft, bool8 isright)
{

	while (root)
    {
		int32 debuglength = 2048;
		char debugbuf[2048];
		char* debugptr = debugbuf;
		for (int32 i = 0; i < indent; ++i)
        {
			SafeStrcpy(debugptr, "    ", debuglength);
			debugptr += 4;
			debuglength -= 4;
		}

        const char* branchtype = isleft ? "L-> " : isright ? "R-> " : "N-> ";
        SafeStrcpy(debugptr, branchtype, debuglength);
		debugptr += 4;
		debuglength -= 4;
		root->Dump(debugptr, debuglength);
		printf("%s\n", debugbuf);
		if (root->leftchild)
			DumpTree(root->leftchild, indent + 1, true, false);
		if (root->rightchild)
			DumpTree(root->rightchild, indent + 1, false, true);

        // -- special case for while loops - we need to dump the end of loop statmements
        if (root->GetType() == eWhileLoop)
        {
            const CWhileLoopNode* while_loop = static_cast<const CWhileLoopNode*>(root);
            const CCompileTreeNode* end_of_loop = while_loop->GetEndOfLoopNode();
            if (end_of_loop)
                DumpTree(end_of_loop,indent + 1, false, false);
        }

        // -- next root, and clear the left/right flags
		root = root->next;
        isleft = false;
        isright = false;
	}
}

// ====================================================================================================================
// DestroyTree():  After a file is parsed, and the tree is compiled, we delete the tree recursively.
// ====================================================================================================================
void DestroyTree(CCompileTreeNode* root)
{
    while (root)
    {
        CCompileTreeNode* nextroot = root->next;

        if (root->leftchild)
        {
            DestroyTree(root->leftchild);
            root->leftchild = NULL;
        }

        if (root->rightchild)
        {
            DestroyTree(root->rightchild);
            root->rightchild = NULL;
        }

        TinFree(root);
        root = nextroot;
    }
}

// ====================================================================================================================
// DumpVarTable():  Debug function to print all members (both dynamic and registered) belonging to a specific object.
// ====================================================================================================================
void DumpVarTable(CObjectEntry* oe)
{
	// -- sanity check
	if (!oe)
		return;

    CNamespace* curentry = oe->GetNamespace();
    while (curentry)
    {
        TinPrint(oe->GetScriptContext(), "\nNamespace: %s\n", UnHash(curentry->GetHash()));
        DumpVarTable(oe->GetScriptContext(), oe, curentry->GetVarTable());
        curentry = curentry->GetNext();
    }

    // -- dump the dynamic var table as well
    if (oe->GetDynamicVarTable())
    {
        TinPrint(oe->GetScriptContext(), "\nDYNAMIC VARS:\n");
        DumpVarTable(oe->GetScriptContext(), oe, oe->GetDynamicVarTable());
    }
}

// ====================================================================================================================
// DebugVarTable():  Debug function to print out the variables in a variable table.
// ====================================================================================================================
void DumpVarTable(CScriptContext* script_context, CObjectEntry* oe, const tVarTable* vartable) {
	// -- sanity check
	if (!script_context || (!oe && !vartable))
		return;

    void* objaddr = oe ? oe->GetAddr() : NULL;

	CVariableEntry* ve = vartable->First();
    while (ve)
    {
		char valbuf[kMaxTokenLength];
		gRegisteredTypeToString[ve->GetType()](ve->GetValueAddr(objaddr), valbuf, kMaxTokenLength);
		TinPrint(script_context, "    [%s] %s: %s\n", gRegisteredTypeNames[ve->GetType()],
                    ve->GetName(), valbuf);
		ve = vartable->Next();
	}
}

// ====================================================================================================================
// DumpFuncTable():  Debug function to print the hierarchy of methods for a specific object.
// ====================================================================================================================
void DumpFuncTable(CObjectEntry* oe)
{
	// -- sanity check
	if (!oe)
		return;

    CNamespace* curentry = oe->GetNamespace();
    while (curentry)
    {
        TinPrint(oe->GetScriptContext(), "\nNamespace: %s\n", UnHash(curentry->GetHash()));
        DumpFuncTable(oe->GetScriptContext(), curentry->GetFuncTable());
        curentry = curentry->GetNext();
    }
}

// ====================================================================================================================
// DumpFuncTable():  Debug function to print all methods registered to a given namespace.
// ====================================================================================================================
void DumpFuncTable(CScriptContext* script_context, const tFuncTable* functable)
{
	// -- sanity check
	if (!functable || !script_context)
		return;

	CFunctionEntry* fe = functable->First();
    while (fe)
    {
		TinPrint(script_context, "    %s()\n", UnHash(fe->GetHash()));
		fe = functable->Next();
	}
}

// ====================================================================================================================
// -- Functions to parse more complicated expressions

// ====================================================================================================================
// TryParseVarDeclaration():  Parse a variable declaration, global, local, member, array, ...
// ====================================================================================================================
bool8 TryParseVarDeclaration(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- use temporary vars, to ensure we dont' change the actual bufptr, unless successful
	tReadToken nexttoken(filebuf);
	if (!GetToken(nexttoken))
		return (false);

	// -- see if we found a registered type
	if (nexttoken.type != TOKEN_REGTYPE)
		return (false);

	eVarType registeredtype = GetRegisteredType(nexttoken.tokenptr, nexttoken.length);

    // -- now see if we're declaring an array
    // $$$TZA eventually, we can parse an expression to determine the size at runtime
    // -- for now, the size must be fixed
    //CCompileTreeNode* array_size_node = NULL;
    //CCompileTreeNode** array_size_root = &array_size_node;
    bool8 is_array = false;
    int32 array_size = 1;
    tReadToken array_decl_token(nexttoken);
    if (!GetToken(array_decl_token))
        return (false);

    // -- see if we're declaring an array of the given type
    if (array_decl_token.type == TOKEN_SQUARE_OPEN)
    {
        if (registeredtype == TYPE_hashtable)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), array_decl_token.linenumber,
                          "Error - hashtable[] arrays are not supported\n");
            return (false);
        }

        // -- committed to an array, and not just a type
        nexttoken = array_decl_token;

        // $$$TZA TYPE__array
        // -- eventually, we can use int[] as a dynamically sizing array, but for now, we'll require
        // -- a fixed size, e.g. int[37], or int[GetArraySize()]
        //if (TryParseExpression(codeblock, array_size_token, *array_size_root))
        //    nexttoken = array_size_token;
        tReadToken array_size_token(nexttoken);
        if (GetToken(array_size_token) && array_size_token.type == TOKEN_INTEGER)
        {
            nexttoken = array_size_token;
            array_size = Atoi(array_size_token.tokenptr, array_size_token.length);
        }

        // -- ensure we have a valid array
        if (array_size <= 0)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), array_size_token.linenumber,
                          "Error - expecting array size integer value, between 1 and %d\n", kMaxVariableArraySize);
            return (false);
        }

        // -- we'd better be able to read the closing square bracket
        if (!GetToken(nexttoken) || nexttoken.type != TOKEN_SQUARE_CLOSE)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), nexttoken.linenumber,
                          "Error - expecting ']'\n");
            return (false);
        }

        // -- set the flag
        is_array = true;
    }

	// -- see if the next token is an identifier, or a self.identifier
	tReadToken idtoken = nexttoken;
	if (!GetToken(idtoken))
		return (false);

    // -- a variable declaration including the keyword 'self' obviously affects its scope
    bool8 selfvardecl = false;
    bool8 member_decl = false;
    tReadToken selftoken(nexttoken);
    if (idtoken.type == TOKEN_KEYWORD)
	{
	    int32 reservedwordtype = GetReservedKeywordType(idtoken.tokenptr, idtoken.length);
	    if (reservedwordtype == KEYWORD_self)
        {
            // -- we'd better find a TOKEN_PERIOD, followed by an identifier
            selfvardecl = true;
            nexttoken = idtoken;
            if (!GetToken(nexttoken) || nexttoken.type != TOKEN_PERIOD)
                return (false);

            idtoken = nexttoken;
            if (!GetToken(idtoken))
                return (false);
        }
        else
            return (false);
    }

    // -- at this point, we should have an identifier
	if (idtoken.type != TOKEN_IDENTIFIER)
		return (false);

    // -- make sure the next token isn't an open parenthesis
    // -- which would make this a function definition
    tReadToken peektoken(idtoken);
    if (!GetToken(peektoken))
        return (false);

    if (peektoken.type == TOKEN_PAREN_OPEN)
        return (false);

    // -- temporary token marker we'll use later to decide if we're auto-initializing
    tReadToken finaltoken = idtoken;

    // -- if this is a self variable, we don't create it until runtime
    // -- if this is a self array var (self.foo["bar"]), we won't know that
    // -- until after we parse for the '[]', however, an array var decl is the parent node,
    // -- and the left child is the obj mem node, so hold on to the link until we're sure
    // -- which order to parent them
    if (selfvardecl)
    {
        // -- committed to a self.var decl
        filebuf = idtoken;

        // -- we've got the type and the variable name - first check that we're inside a method
        int32 stacktopdummy = 0;
        CObjectEntry* dummy = NULL;
        CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
        uint32 funchash = curfunction ? curfunction->GetHash() : 0;
        uint32 nshash = curfunction ? curfunction->GetNamespaceHash() : CScriptContext::kGlobalNamespaceHash;
        if (funchash == 0 || nshash == 0)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), idtoken.linenumber,
                          "Error - attempting to declare self.%s var outside a method\n", TokenPrint(idtoken));
            return (false);
        }

        // -- reset the nexttoken to be at the start of "self.*", in case we find an assign op
        nexttoken = selftoken;

        // -- set the peek token to be the one following the var id
        peektoken = idtoken;
        if (!GetToken(peektoken))
            return (false);
    }

    // -- if the next token is the beginning of an array variable, we also can't continue,
    // -- as the hash value to dereference the array entry isn't known until runtime
    if (peektoken.type == TOKEN_SQUARE_OPEN)
    {
        // -- for now, we don't support initializing members of an array
        if (is_array)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), peektoken.linenumber,
                          "Error - auto-initialization of arrays is not supported.\n", TokenPrint(idtoken));
            return (false);
        }

        // -- committed to a hashtable dereference
        filebuf = idtoken;

        int32 stacktopdummy = 0;
        CObjectEntry* dummy = NULL;
        CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
		uint32 varhash = Hash(idtoken.tokenptr, idtoken.length);
        uint32 funchash = curfunction ? curfunction->GetHash() : 0;
        uint32 nshash = curfunction ? curfunction->GetNamespaceHash() : CScriptContext::kGlobalNamespaceHash;
        CVariableEntry* var = NULL;

        // -- the hashtable would have already had to have been declared, unless it's a self.hashtable
        if (!selfvardecl)
        {
            var = GetVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable, nshash, funchash,
                              varhash, 0);
            if (!var || var->GetType() != TYPE_hashtable)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - variable %s is not of type hashtable\n", UnHash(varhash));
			    return (false);
            }
        }

        // -- create the ArrayVarDeclNode, leftchild is the hashtable var, right is the hash value
        CArrayVarDeclNode* arrayvarnode = TinAlloc(ALLOC_TreeNode, CArrayVarDeclNode, codeblock,
                                                   link, filebuf.linenumber, registeredtype);

        // -- if we're declaring an array variable belonging to a self.hashtable, then
        // -- the left child is an ObjMemberNode, not a ValueNode
        if (selfvardecl)
        {
		    CObjMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CObjMemberNode, codeblock, arrayvarnode->leftchild,
                                                 idtoken.linenumber, idtoken.tokenptr, idtoken.length);
            Unused_(objmember);

            // -- the left child is the branch that resolves to an object (self, in this case)
            CSelfNode* selfnode = TinAlloc(ALLOC_TreeNode, CSelfNode, codeblock, objmember->leftchild,
                                           idtoken.linenumber);
            Unused_(selfnode);
        }

        // -- otherwise, the left child is the value node, specifying the var name
        else
        {
            // -- left child is the variable (which is obviously a hashtable)
            CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                             arrayvarnode->leftchild, filebuf.linenumber,
                                             idtoken.tokenptr, idtoken.length, true, TYPE_hashtable);
            Unused_(valuenode);
        }

        // -- the right child is the hash value
        if (!TryParseArrayHash(codeblock, filebuf, arrayvarnode->rightchild))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - unable to parse array hash for variable %s\n", UnHash(varhash));
			return (false);
        }

   	    // -- get the final token
	    finaltoken = filebuf;
	    if (!GetToken(finaltoken))
		    return (false);

        // -- see if this is a declaration, or if there's an assignment following
        if (finaltoken.type == TOKEN_SEMICOLON)
        {
		    // -- we've successfully created a var declaration
		    filebuf = finaltoken;
	    }

	    // -- else if the next token is an operator, we're going to
	    else if (finaltoken.type == TOKEN_ASSOP)
        {
		    // -- we're going to update the input buf ptr to just after having read the type
		    // -- and allow the assignment to be ready as an assignment
		    filebuf = nexttoken;
	    }

        // -- we're done
        return (true);
    }

    // -- otherwise, not a hash table entry - if it we were declaring a self variable, we can
    // -- now create the node
    else if (selfvardecl)
    {
        // -- create the node
        CSelfVarDeclNode* self_var_node = TinAlloc(ALLOC_TreeNode, CSelfVarDeclNode, codeblock, link,
                                                   idtoken.linenumber, idtoken.tokenptr, idtoken.length,
                                                   registeredtype, array_size);
        Unused_(self_var_node);
    }

    // -- not a self var, not a hash table entry, it's either global or a local function var
	// -- get the final token
	finaltoken = idtoken;
	if (!GetToken(finaltoken))
		return (false);

	// -- see if the last token was a semicolon, marking the end of a var declaration
    bool is_var_decl = false;
	if (finaltoken.type == TOKEN_SEMICOLON)
    {
		// -- we've successfully created a var declaration
		filebuf = finaltoken;
        is_var_decl = true;
	}

	// -- else if the next token is an operator, we're going to
	else if (finaltoken.type == TOKEN_ASSOP)
    {
        // -- no support for auto-initializing arrays
        if (is_array)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          finaltoken.linenumber, "Error - auto-initializing an array is not supported.\n");
			return (false);
        }

		// -- we're going to update the input buf ptr to just after having read the type
		// -- and allow the assignment to be ready as an assignment
		filebuf = nexttoken;
        is_var_decl = true;
	}

    // -- if the final token is actually a period, then we're dereferencing an object, and the variable is a member
    else if (finaltoken.type == TOKEN_PERIOD)
    {
        // -- the idtoken actually refers to the object (or variable referring to an object)
        tReadToken member_token = finaltoken;
        if (!GetToken(member_token) || member_token.type != TOKEN_IDENTIFIER)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          member_token.linenumber, "Error - expecting a member identifier.\n");
            return (false);
        }

        // -- if we've got a hashtable expression, the left child is the member, the right child is hash value
        CCompileTreeNode* member_root = NULL;
        CCompileTreeNode** member_link = &member_root;

        // -- try to read the hash expression into the temporary root
        CCompileTreeNode* array_root = NULL;
        CCompileTreeNode** temp_root = &array_root;
        tReadToken arrayhashtoken(member_token);
        if (TryParseArrayHash(codeblock, arrayhashtoken, *temp_root))
        {
            // -- we're committed to a method hashtable lookup
            finaltoken = arrayhashtoken;

            // -- create the ArrayVarDeclNode, leftchild is the hashtable var, right is the hash value
            CArrayVarDeclNode* arrayvarnode = TinAlloc(ALLOC_TreeNode, CArrayVarDeclNode, codeblock,
                                                       link, filebuf.linenumber, registeredtype);

            // -- the right child is the hash value
            arrayvarnode->rightchild = array_root;


            // -- the left child is the member node
		    CObjMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CObjMemberNode, codeblock, arrayvarnode->leftchild,
                                                 member_token.linenumber, member_token.tokenptr, member_token.length);

            // -- the left child of the member node resolves tothe object
		    CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, objmember->leftchild,
                                             idtoken.linenumber, idtoken.tokenptr, idtoken.length, true, TYPE_object);
        }

        // -- else we're connecting the member directly to link
        else
        {
            // -- update the token ptr
            finaltoken = member_token;

            // -- create the member node
            CObjMemberDeclNode* obj_member_decl_node = TinAlloc(ALLOC_TreeNode, CObjMemberDeclNode, codeblock, link,
                                                                member_token.linenumber, member_token.tokenptr,
                                                                member_token.length, registeredtype, array_size);

            // -- create the value node that resolves to an object
		    CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, obj_member_decl_node->leftchild,
                                             idtoken.linenumber, idtoken.tokenptr, idtoken.length, true, TYPE_object);
        }

        // -- now we find the final token - is this a declaration, or do we have an assignment
	    if (!GetToken(finaltoken))
		    return (false);

	    // -- see if the last token was a semicolon, marking the end of a var declaration
	    if (finaltoken.type == TOKEN_SEMICOLON)
        {
		    // -- we've successfully created a var declaration
		    filebuf = finaltoken;
	    }

	    // -- else if the next token is an operator, we're going to parse starting
        // -- back at the "next token", so after the var declaration, we'll find an assignment statement
	    else if (finaltoken.type == TOKEN_ASSOP)
        {
            // -- no support for auto-initializing arrays
            if (is_array)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              finaltoken.linenumber, "Error - auto-initializing an array is not supported.\n");
			    return (false);
            }

		    // -- we're going to update the input buf ptr to just after having read the type
		    // -- and allow the assignment to be ready as an assignment
		    filebuf = nexttoken;
	    }

        // -- a variable declaration can only end in one of two ways - a semi colon, completing the statement
        // -- or an assignment, to initialize the var decl
        else
        {
            return (false);
        }

        // -- we're done
        return (true);
    }

    // -- if we found a variable declaration, add the variable
    if (is_var_decl && !selfvardecl)
    {
        int32 stacktopdummy = 0;
        CObjectEntry* dummy = NULL;
        CFunctionEntry* curfunction =
            codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);

        AddVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable, curfunction,
                    TokenPrint(idtoken), Hash(TokenPrint(idtoken)), registeredtype, array_size);
    }

    // -- return the result
	return (is_var_decl);
}

// ====================================================================================================================
// SortBinOpPrecedence():  Operators have precedence, e.g.  multiplication before addition.
// ====================================================================================================================
CCompileTreeNode** SortBinOpPrecedence(CCompileTreeNode** toplink, bool8& found_swap)
{
    // -- initialize the return value
    found_swap = false;

    // -- sanity check
    if (toplink == NULL)
        return (NULL);

    // -- we need to sort binary non-assign ops by precedence, for any sequential binop nodes
    // -- along the right children
    CCompileTreeNode* head = *toplink;
    CCompileTreeNode** parent = toplink;
    while (head && (head->GetType() != eBinaryOp ||
           static_cast<CBinaryOpNode*>(head)->GetBinaryOpPrecedence() == 0))
    {
        parent = &head->rightchild;
        head = head->rightchild;
    }

    // -- if we didn't find a head, or a head->rightchild, nothing to sort
    if (!head || !head->rightchild)
        return (NULL);

    // -- now look for the highest priority child to sort above "head"
    // -- include a depth increment, since right-to-left applies to equal precedence ops
    int32 headprecedence = static_cast<CBinaryOpNode*>(head)->GetBinaryOpPrecedence() * 1000;
    int32 depth = 1;
    CCompileTreeNode** swapparent = &head->rightchild;
    CCompileTreeNode* swap = head->rightchild;

    while (swap && swap->GetType() == eBinaryOp)
    {
        int32 swapprecedence = static_cast<CBinaryOpNode*>(swap)->GetBinaryOpPrecedence() * 1000 + depth;
        if (swapprecedence <= headprecedence)
        {
            ++depth;
            swapparent = &swap->rightchild;
            swap = swap->rightchild;
        }
        else
            break;
    }

    // -- if we didn't find a node to swap, we're done
    if (!swap || swap == head)
        return (NULL);

    // -- if swap isn't a binary op, we want to continue testing the next rightchild from head
    if (swap->GetType() != eBinaryOp ||
        static_cast<CBinaryOpNode*>(swap)->GetBinaryOpPrecedence() == 0)
    {
        return &head->rightchild;
    }

    // -- swap the two nodes:
    // -- swap's leftchild take the place of swap (e.g. the right child of swap's parent)
    // -- swap's new leftchild is head
    // -- whatever was pointing at head, now points to swap
    CCompileTreeNode* temp = swap->leftchild;
    swap->leftchild = head;
    *swapparent = temp;
    *parent = swap;

    // -- set the return value
    found_swap = true;

    // -- the new toplink we need to sort from is swap->leftchild
    return (&swap->leftchild);
}

// ====================================================================================================================
// SortTreeBinaryOps():  Sort all binary op nodes in a branch.
// ====================================================================================================================
void SortTreeBinaryOps(CCompileTreeNode** toplink)
{
    static bool8 enablesort = true;
    if (!enablesort)
        return;

    // -- we need to do passes through the list, until we make it through with no swaps
    while (true)
    {
        // -- we're looking to see if any of the passes in this loop performed a swap
        bool8 loop_swap = false;
        bool8 pass_swap = false;

        CCompileTreeNode** sorthead = SortBinOpPrecedence(toplink, pass_swap);

        // -- set the loop bool after every pass
        loop_swap = loop_swap || pass_swap;

        while (sorthead != NULL)
        {
            sorthead = SortBinOpPrecedence(sorthead, pass_swap);

            // -- set the loop bool after every pass
            loop_swap = loop_swap || pass_swap;
        }

        // -- if we made it all the way through with no loops, we're done
        if (!loop_swap)
            break;
    }
}

// ====================================================================================================================
// TryParseStatement():  Parse a complete statement, as described in the comments below.
// ====================================================================================================================
bool8 TryParseStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- an statement is one of:
    // -- a semicolon
    // -- a return statement, a create statement, or a destroy statement
	// -- an expression followed by a semicolon
	// -- an expression followed by a binary operator, followed by an expression

	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken))
		return (false);

    // -- if the first token is a semi-colon, consume the empty expression
    // -- unless we're in the middle of a parenthetical expression
    if (firsttoken.type == TOKEN_SEMICOLON)
    {
        if (gGlobalExprParenDepth > 0)
            return (false);
        filebuf = firsttoken;
        return (true);
    }

    // -- check for a break or continue statement
    if (TryParseBreakContinue(codeblock, filebuf, link))
    {
        return (true);
    }

    // -- check for a return statement
    if (TryParseReturn(codeblock, filebuf, link))
    {
        return (true);
    }

    // -- check for a destroy statement
    if (TryParseDestroyObject(codeblock, filebuf, link))
    {
        return (true);
    }

    // -- check for a create statement
    if (TryParseCreateObject(codeblock, filebuf, link))
    {
        return (true);
    }

    // -- use a temporary root to construct the statement, before hooking it into the tree
    CCompileTreeNode* statementroot = NULL;
    CCompileTreeNode** templink = &statementroot;

    // -- use a temporary link to see if we have an expression
    tReadToken readexpr(filebuf);
    if (!TryParseExpression(codeblock, readexpr, *templink))
    {
        return (false);
    }

    // -- see if we've got a semicolon, a binop, an assop or an object dereference
    tReadToken nexttoken(readexpr);
    if (!GetToken(nexttoken))
        return (false);

    // -- reached the end of the statement
    while (true)
    {
        // -- see if we've reached the end of the statement
        // -- if we find a closing parenthesis that we're expecting, we're done
        if (nexttoken.type == TOKEN_PAREN_CLOSE || nexttoken.type == TOKEN_SQUARE_CLOSE)
        {
            // -- make sure we were expecting it
            if (gGlobalExprParenDepth == 0)
            {
                return (false);
            }

            // -- otherwise we're done successfully
            else
            {
                // -- don't consume the ')' - let the expression handle it
                filebuf = readexpr;
                link = statementroot;

                // -- at the end of the statement, we need to sort sequences of binary op nodes
                SortTreeBinaryOps(&link);

                return (true);
            }
        }

        else if (nexttoken.type == TOKEN_COMMA)
        {
            // -- don't consume the ',' - let the expression handle it
            filebuf = readexpr;
            link = statementroot;

            // -- at the end of the statement, we need to sort sequences of binary op nodes
            SortTreeBinaryOps(&link);

            return (true);
        }

        else if (nexttoken.type == TOKEN_SEMICOLON)
        {
            // $$$TZA From within a 'For' loop, we have valid ';' within parenthesis
            // -- if so, do not consume the ';'
            if (gGlobalExprParenDepth > 0)
                filebuf = readexpr;
            // -- otherwise this is a complete statement - consume the ';'
            else
                filebuf = nexttoken;
            link = statementroot;

            // -- at the end of the statement, we need to sort sequences of binary op nodes
            SortTreeBinaryOps(&link);

            return (true);
        }

        // -- see if we've got a binary operation
        else if (nexttoken.type == TOKEN_BINOP)
        {
            // -- we're committed to a statement at this point
            readexpr = nexttoken;

            CCompileTreeNode* templeftchild = *templink;
		    eBinaryOpType binoptype = GetBinaryOpType(nexttoken.tokenptr, nexttoken.length);
            CBinaryOpNode* binopnode = TinAlloc(ALLOC_TreeNode, CBinaryOpNode, codeblock,
                                                *templink, readexpr.linenumber, binoptype,
                                                false, TYPE__resolve);
            binopnode->leftchild = templeftchild;

		    // -- ensure we have an expression to fill the right child
		    bool8 result = TryParseExpression(codeblock, readexpr, binopnode->rightchild);
		    if (!result || !binopnode->rightchild)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber,
                              "Error - Binary operator without a rhs expression\n");
			    return (false);
		    }

            // -- update our temporary root
            templink = &binopnode->rightchild;

            // -- successfully read the rhs, get the next token
            nexttoken = readexpr;
            if (!GetToken(nexttoken))
            {
			    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - expecting ';'\n");
			    return (false);
            }
        }

        // -- see if we've got an assignment op
        else if (nexttoken.type == TOKEN_ASSOP)
        {
            // -- we're committed to a statement at this point
            readexpr = nexttoken;

            CCompileTreeNode* templeftchild = *templink;
            eAssignOpType assoptype = GetAssignOpType(nexttoken.tokenptr, nexttoken.length);
		    CBinaryOpNode* binopnode = TinAlloc(ALLOC_TreeNode, CBinaryOpNode, codeblock,
                                                *templink, readexpr.linenumber, assoptype,
                                                true, TYPE__resolve);
            binopnode->leftchild = templeftchild;

		    // -- ensure we have an expression to fill the right child
		    bool8 result = TryParseExpression(codeblock, readexpr, binopnode->rightchild);
		    if (!result || !binopnode->rightchild)
            {
			    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber,
                              "Error - Assignment operator without a rhs expression\n");
			    return (false);
		    }

            // -- update our temporary root
            templink = &binopnode->rightchild;

            // -- successfully read the rhs, get the next token
            nexttoken = readexpr;
            if (!GetToken(nexttoken))
            {
			    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - expecting ';'\n");
			    return (false);
            }
        }

        else
        {
		    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          readexpr.linenumber, "Error - expecting ';'\n");
		    return (false);
        }
    }

    // -- should be impossible to exit the while loop - fail
    return (false);
}

// ====================================================================================================================
// TryParseExpression():  Parse an expression, as defined in the comments below.
// ====================================================================================================================
bool8 TryParseExpression(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- an expression is:
    // -- a schedule call
    // -- a new object
    // -- a function call
    // -- possibly a leading unary operator, followed by
    // -- possibly a 'self' keyword,  or a value (integer, refering to an object)
    // -- a chain of identifiers and method calls separated by dereference operators '.'
    // -- a variable of a POD type, followed by a podmember ':' operator, followed by a POD member
	// -- a var/value/hash table entry
    // -- basically anything that results in pushing a value onto the stack

	// -- see if we've got a unary operator
	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken, true))
		return (false);

    CUnaryOpNode* unarynode = NULL;
    if (firsttoken.type == TOKEN_UNARY)
    {
        eUnaryOpType unarytype = GetUnaryOpType(firsttoken.tokenptr, firsttoken.length);
		unarynode = TinAlloc(ALLOC_TreeNode, CUnaryOpNode, codeblock, link, filebuf.linenumber,
                             unarytype);

        // -- committed
        filebuf = firsttoken;

        // -- read the next token  (an expression can't end after just a unary operator...)
        if (!GetToken(firsttoken))
            return (false);
    }

    // -- the new link to connect to is either the given, or the left child of the unary op
    CCompileTreeNode*& exprlink = (unarynode != NULL) ? unarynode->leftchild : link;

    // -- use a temporary root to construct expression, before linking it to the rest of the tree
    CCompileTreeNode* expression_root = NULL;
    CCompileTreeNode** temp_link = &expression_root;

    // -- if the first token is an opening parenthesis, add the node to the tree and
    // -- parse the contained expression
    if (firsttoken.type == TOKEN_PAREN_OPEN)
    {
        filebuf = firsttoken;
        CParenOpenNode* parenopennode = TinAlloc(ALLOC_TreeNode, CParenOpenNode, codeblock, *temp_link,
                                                 filebuf.linenumber);

        // -- increment the parenthesis stack
        ++gGlobalExprParenDepth;

        // -- read the statement that should exist between the parenthesis
        int32 result = TryParseStatement(codeblock, filebuf, parenopennode->leftchild);
        if (!result)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), firsttoken.linenumber,
                          "Error - Unable to parse expression following '('\n");
            return (false);
        }

        // -- read the closing parenthesis
        if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        // -- decriment the parenthesis stack
        --gGlobalExprParenDepth;

        // -- the leftchild of the parenopennode is our value, so we use it
        // -- hook up the link to the correct subtree, and delete the unneeded paren node
        *temp_link = parenopennode->leftchild;
        parenopennode->leftchild = NULL;
        TinFree(parenopennode);

        // -- override the binary op precedence, as we don't sort past a parenthesized sub-tree
        if ((*temp_link)->GetType() == eBinaryOp)
        {
            static_cast<CBinaryOpNode*>(*temp_link)->OverrideBinaryOpPrecedence(0);
        }

        // -- hook the parenthetical expression up to the actual tree (possibly as the child of the unary op)
        exprlink = expression_root;

        // -- success
        return (true);
    }

    // -- a schedule is completes an expression
    if (TryParseSchedule(codeblock, filebuf, exprlink))
        return (true);

    // -- a create object completes an expression
    if (TryParseCreateObject(codeblock, filebuf, exprlink))
        return (true);

    // -- a first class value that is *not* an integer completes an expression
    // -- (an integer can be followed by a dereference operator, and then it becomes an object ID)
    eVarType firstclassvartype;
    if (IsFirstClassValue(firsttoken.type, firstclassvartype) && firstclassvartype != TYPE_int)
    {
        // -- committed to value
        filebuf = firsttoken;

		CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, exprlink, filebuf.linenumber,
                                         firsttoken.tokenptr, firsttoken.length, false,
                                         firstclassvartype);
        return (true);
    }

    // -- a hash() completes an expression
    if (TryParseHash(codeblock, filebuf, exprlink))
        return (true);

    // -- after the potential unary op, an expression may start with:
    // -- a 'self'
    // -- a function call (not a method)
    // -- an identifier
    // -- an integer
    if (firsttoken.type == TOKEN_KEYWORD)
    {
	    int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
	    if (reservedwordtype == KEYWORD_self)
        {
            // -- committed to self
            filebuf = firsttoken;
		    CSelfNode* selfnode = TinAlloc(ALLOC_TreeNode, CSelfNode, codeblock, *temp_link, filebuf.linenumber);
        }
        else
            return (false);
    }

    // -- function call
    else if (TryParseFuncCall(codeblock, filebuf, *temp_link, false))
    {
        // -- committed to function call, filebuf will have already been updated
    }

    // -- if we've got a first class value... (we've already read the firsttoken in)
    else if (IsFirstClassValue(firsttoken.type, firstclassvartype) && firstclassvartype == TYPE_int)
    {
        // -- committed to value
        filebuf = firsttoken;

		TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, *temp_link, filebuf.linenumber, firsttoken.tokenptr,
                 firsttoken.length, false, firstclassvartype);
    }

    // -- if we've got an identifier, see if it's a variable
	else if (firsttoken.type == TOKEN_IDENTIFIER)
    {
        int32 stacktopdummy = 0;
        CObjectEntry* dummy = NULL;
        CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
		uint32 varhash = Hash(firsttoken.tokenptr, firsttoken.length);
        uint32 funchash = curfunction ? curfunction->GetHash() : 0;
        uint32 nshash = curfunction ? curfunction->GetNamespaceHash() : CScriptContext::kGlobalNamespaceHash;
        CVariableEntry* var = GetVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable, nshash,
                                          funchash, varhash, 0);
		if (var)
        {
            // -- we're committed to the variable
            filebuf = firsttoken;

            // -- if the type is a hash table, try to parse a hash table lookup
            CCompileTreeNode* array_root = NULL;
            CCompileTreeNode** temp_root = &array_root;
            tReadToken arrayhashtoken(filebuf);
            if (TryParseArrayHash(codeblock, arrayhashtoken, *temp_root))
            {
                // -- we're committed to a method hashtable lookup
                filebuf = arrayhashtoken;

                // -- create the ArrayVarNode, leftchild is the hashtable var, right is the hash value
                CArrayVarNode* arrayvarnode = TinAlloc(ALLOC_TreeNode, CArrayVarNode, codeblock, *temp_link,
                                                       filebuf.linenumber);

                // -- create the variable node
		        CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, arrayvarnode->leftchild,
                                                 filebuf.linenumber, firsttoken.tokenptr, firsttoken.length, true,
                                                 TYPE_hashtable);

                // the right child of the array is the array hash
                arrayvarnode->rightchild = *temp_root;

                // -- we're committed to a method hashtable lookup
                filebuf = arrayhashtoken;
            }

            // -- not a hash table - create the value node
            else
            {
		        CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, *temp_link, filebuf.linenumber,
                                                 firsttoken.tokenptr, firsttoken.length, true, var->GetType());
            }
		}
        else
        {
            // -- identifier, but at the start of an expression,
            // -- this can only be a variable (not a member, type, keyword, etc...)
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), firsttoken.linenumber,
                          "Error - unknown identifier: %s\n", TokenPrint(firsttoken));
            return (false);
        }
    }

    // -- at this point, we have a valid expression, that is either complete, or can be a sequence of dereferences
    while (true)
    {
	    // -- read the next token - an expression is not the end of a statement - if we have no next token,
        // -- something is amiss
	    tReadToken nexttoken(filebuf);
	    if (!GetToken(nexttoken))
		    return (false);

        // -- see if we're dereferencing an object, then our expression is not complete - we need a method or member
        if (nexttoken.type == TOKEN_PERIOD)
        {
            // -- we're committed to a dereference operator
            filebuf = nexttoken;

            // -- either we have a member, or a method after the period
            // -- cache the tree that resolves to an object ID
            CCompileTreeNode* templeftchild = *temp_link;

            // -- ensure we've got an identifier for the member name next
            tReadToken membertoken(filebuf);
            if (!GetToken(membertoken) || membertoken.type != TOKEN_IDENTIFIER)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - Expecting a member name\n");
                return (false);
            }

            // -- determine if we actually have a method call, and not just a member
            tReadToken methodcalltoken(filebuf);
            if (TryParseFuncCall(codeblock, methodcalltoken, *temp_link, true))
            {
                // -- we're committed to a method call
                filebuf = methodcalltoken;

                // -- create an object method node, the left child will resolve to the objectID
                // -- and the right child will be the tree handling the method call
                CCompileTreeNode* temprightchild = *temp_link;
		        CObjMethodNode* objmethod = TinAlloc(ALLOC_TreeNode, CObjMethodNode, codeblock, *temp_link,
                                                     membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                // -- the left child is the branch that resolves to an object
                objmethod->leftchild = templeftchild;
                objmethod->rightchild = temprightchild;
            }

            // -- not a method - we've already read the member name
            else
            {
                // -- we're committed to an object dereference at this point
                filebuf = membertoken;

                // -- see if we've got a hashtable expression - the right child will resolve to a hash value
                CCompileTreeNode* array_root = NULL;
                CCompileTreeNode** temp_root = &array_root;
                tReadToken arrayhashtoken(filebuf);
                if (TryParseArrayHash(codeblock, arrayhashtoken, *temp_root))
                {
                    // -- we're committed to a method hashtable lookup
                    filebuf = arrayhashtoken;

                    // -- create the ArrayVarNode, leftchild is the hashtable var, right is the hash value
                    CArrayVarNode* arrayvarnode = TinAlloc(ALLOC_TreeNode, CArrayVarNode, codeblock,
                                                           *temp_link, filebuf.linenumber);

                    // -- create the member node
		            CObjMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CObjMemberNode, codeblock, arrayvarnode->leftchild,
                                                         membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                    // -- the left child is the branch that resolves to an object
                    objmember->leftchild = templeftchild;

                    // the right child of the array is the array hash
                    arrayvarnode->rightchild = *temp_root;
                }

                // -- else not an array, just an object member
                else
                {
                    // -- create the member node
		            CObjMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CObjMemberNode, codeblock, *temp_link,
                                                         membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                    // -- the left child is the branch that resolves to an object
                    objmember->leftchild = templeftchild;
                }
            }
        }

        // -- else if we have a colon, we're dereferrencing a member of a registered POD type
        else if (nexttoken.type == TOKEN_COLON)
        {
            // -- we're committed
            filebuf = nexttoken;

            // -- cache the tree that resolves to a variable of a registered POD type
            // -- note:  this could still be a function call - e.g.  "GetPosition():x"
            CCompileTreeNode* templeftchild = *temp_link;

            // -- ensure we've got an identifier for the member name next
            tReadToken membertoken(filebuf);
            if (!GetToken(membertoken) || membertoken.type != TOKEN_IDENTIFIER)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - Expecting a POD member name\n");
                return (false);
            }

            // -- we're committed to a POD variable dereference at this point
            filebuf = membertoken;

            // -- create the member node
		    CPODMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CPODMemberNode, codeblock, *temp_link,
                                                 membertoken.linenumber, membertoken.tokenptr, membertoken.length);

            // -- the left child is the branch that resolves to an object
            objmember->leftchild = templeftchild;

            // -- and because POD members do not continue to be dereferenced, this is the end of the expression
            exprlink = expression_root;

            // -- and we're done
            return (true);
        }

        // -- otherwise, we've hit the end of our expression
        else
        {
            // -- hook up our expression sub-tree to the rest of the tree
            exprlink = expression_root;

            // -- and we're done
            return (true);
        }
    }

	// -- not an expression
	return (false);
}

// ====================================================================================================================
// TryParseIfStatement():  An 'if' statement is a well defined syntax.
// ====================================================================================================================
bool8 TryParseIfStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- the first token can be anything but a reserved word or type
	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken))
		return (false);

	// -- starts with the keyword 'if'
	if (firsttoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
	if (reservedwordtype != KEYWORD_if)
		return (false);

    // -- we're committed to an 'if' statement now
    filebuf = firsttoken;

	// -- next token better be an open parenthesis
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_OPEN))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting '('\n");
		return (false);
	}

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

	// -- an 'if' statement has the expression tree as it's left child,
	// -- and a branch node as it's right child, based on the true/false
	CIfStatementNode* ifstmtnode = TinAlloc(ALLOC_TreeNode, CIfStatementNode, codeblock, link,
                                            filebuf.linenumber);

	// we need to have a valid expression for the left hand child
	bool8 result = TryParseStatement(codeblock, filebuf, ifstmtnode->leftchild);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'if statement' without a conditional expression\n");
		return (false);
	}

    // -- consume the closing parenthesis
    if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ')'\n");
        return (false);
    }

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

	// -- we've got our conditional expression - the right child is a branch node
	CCondBranchNode* condbranchnode = TinAlloc(ALLOC_TreeNode, CCondBranchNode, codeblock,
                                               ifstmtnode->rightchild, filebuf.linenumber);

	// -- the left side of the condbranchnode is the 'true' branch
	// -- see if we have a statement, or a statement block
	tReadToken peektoken(filebuf);
	if (!GetToken(peektoken))
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'if statement' without a following statement block\n");
		return (false);
	}

	if (peektoken.type == TOKEN_BRACE_OPEN)
    {
		// -- consume the brace, and parse an entire statement block
		filebuf = peektoken;
		result = ParseStatementBlock(codeblock, condbranchnode->leftchild, filebuf, true);
		if (!result) {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - failed to read statement block\n");
			return (false);
		}
	}

	// else try a single expression
	else
	{
		result = TryParseStatement(codeblock, filebuf, condbranchnode->leftchild);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                          "Error - 'if statement' without a statement block\n");
			return (false);
		}
	}

	// -- now handle the "false" branch
	peektoken = filebuf;
	if (!GetToken(peektoken))
    {
		// -- no token - technically we're done successfully
		return (true);
	}

	// -- we're done, unless we find an an 'else', or an 'else if'
	if (peektoken.type == TOKEN_KEYWORD)
    {
		int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
		if (reservedwordtype != KEYWORD_else)
			return (true);

		// -- we have an 'else': three options to follow
		filebuf = peektoken;

		// -- first, see if it's an else 'if'
		if (TryParseIfStatement(codeblock, filebuf, condbranchnode->rightchild))
			return (true);

		// -- next, see if we have a statement block
		if (!GetToken(peektoken))
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - 'else' without a statement block\n");
			return (false);
		}

		if (peektoken.type == TOKEN_BRACE_OPEN)
        {
			filebuf = peektoken;
			result = ParseStatementBlock(codeblock, condbranchnode->rightchild, filebuf, true);
			if (!result) {
				ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - unable to parse statmentblock following 'else'\n");
				return (false);
			}

			return (true);
		}

		// -- finally, it must be a simple expression
		else
        {
			result = TryParseStatement(codeblock, filebuf, condbranchnode->rightchild);
			if (!result)
            {
				ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - unable to parse expression following 'else'\n");
				return (false);
			}
			return (true);
		}
	}

	return (true);
}

// ====================================================================================================================
// TryParseWhileLoop():  A while loop has a well defined syntax.
// ====================================================================================================================
bool8 TryParseWhileLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- the first token can be anything but a reserved word or type
	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken))
		return (false);

	// -- starts with the keyword 'for'
	if (firsttoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
	if (reservedwordtype != KEYWORD_while)
		return (false);

    // -- we're committed to a 'while' loop now
    filebuf = firsttoken;

	// -- next token better be an open parenthesis
	tReadToken peektoken(firsttoken);
	if (!GetToken(peektoken) || (peektoken.type != TOKEN_PAREN_OPEN))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      firsttoken.linenumber, "Error - expecting '('\n");
		return (false);
	}

	// -- committed to a while loop
	filebuf = peektoken;

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

	// -- a while loop has the expression tree as it's left child,
	// -- and the body as a statement block as its right child
	CWhileLoopNode* whileloopnode = TinAlloc(ALLOC_TreeNode, CWhileLoopNode, codeblock, link,
                                             filebuf.linenumber);

    // -- push the while loop onto the stack
    if (gWhileLoopDepth >= gMaxWhileLoopDepth)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' depth of %d exceeded\n", gMaxWhileLoopDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gWhileLoopStack[gWhileLoopDepth++] = whileloopnode;

	// we need to have a valid expression for the left hand child
	bool8 result = TryParseStatement(codeblock, filebuf, whileloopnode->leftchild);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' without a conditional expression\n");
        --gWhileLoopDepth;
		return (false);
	}

    // -- consume the closing parenthesis
    if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ')'\n");
        --gWhileLoopDepth;
        return (false);
    }

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

	// -- see if we've got a statement block, or a single statement
	peektoken = filebuf;
	if (!GetToken(peektoken))
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - 'while loop' without a body\n");
        --gWhileLoopDepth;
		return (false);
	}

	if (peektoken.type == TOKEN_BRACE_OPEN)
    {
		filebuf = peektoken;
		result = ParseStatementBlock(codeblock, whileloopnode->rightchild, filebuf, true);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - unable to parse the while loop statmentblock\n");
            --gWhileLoopDepth;
			return (false);
		}

        // -- success - pop the while node off the stack
        --gWhileLoopDepth;
		return (true);
	}

	// -- else it's a single expression
	else
    {
		result = TryParseStatement(codeblock, filebuf, whileloopnode->rightchild);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - unable to parse the while loop body\n");
            --gWhileLoopDepth;
			return (false);
		}

        // -- success - pop the while node off the stack
        --gWhileLoopDepth;
		return (true);
	}
}

// ====================================================================================================================
// TryParseForLoop():  A 'for' loop has a well defined syntax.
// ====================================================================================================================
bool8 TryParseForLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- the first token can be anything but a reserved word or type
	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken))
		return (false);

	// -- starts with the keyword 'for'
	if (firsttoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
	if (reservedwordtype != KEYWORD_for)
		return (false);

    // -- we're committed to a 'for' loop now
    filebuf = firsttoken;

	// -- next token better be an open parenthesis
	tReadToken peektoken(firsttoken);
	if (!GetToken(peektoken) || (peektoken.type != TOKEN_PAREN_OPEN))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      firsttoken.linenumber, "Error - expecting '('\n");
		return (false);
	}

	// -- valid so far
	filebuf = peektoken;

    // -- increment the parenthesis stack
    ++gGlobalExprParenDepth;

	// -- we can use a while loop:
	// -- the left child is the condition
	// -- the right child is a tree, containing the body, appended with the end-of-loop expr.

	link = CCompileTreeNode::CreateTreeRoot(codeblock);
	CCompileTreeNode* forlooproot = link;

	// -- initial expression
	bool8 result = TryParseStatement(codeblock, filebuf, AppendToRoot(*forlooproot));
	if (! result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the initial expression\n");
		return (false);
	}

  	// -- consume the separating semicolon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_SEMICOLON))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ';'\n");
		return (false);
	}

	// add the while loop node
	CWhileLoopNode* whileloopnode = TinAlloc(ALLOC_TreeNode, CWhileLoopNode, codeblock,
                                             AppendToRoot(*forlooproot), filebuf.linenumber);

    // -- push the while loop onto the stack
    if (gWhileLoopDepth >= gMaxWhileLoopDepth)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' depth of %d exceeded\n", gMaxWhileLoopDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gWhileLoopStack[gWhileLoopDepth++] = whileloopnode;

	// -- the for loop condition is the left child of the while loop node
	result = TryParseStatement(codeblock, filebuf, whileloopnode->leftchild);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the conditional expression\n");
        --gWhileLoopDepth;
		return (false);
	}

   	// -- consume the separating semicolon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_SEMICOLON))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ';'\n");
        --gWhileLoopDepth;
		return (false);
	}

	// -- the end of loop expression is next, but we're going to hold on to it for a moment
	CCompileTreeNode* tempendofloop = NULL;
	result = TryParseStatement(codeblock, filebuf, tempendofloop);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the end of loop expression\n");
        --gWhileLoopDepth;
		return (false);
	}

   	// -- consume the closing parenthesis semicolon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_CLOSE))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ')'\n");
        --gWhileLoopDepth;
		return (false);
	}

    // -- decrement the parenthesis stack
    --gGlobalExprParenDepth;

	// -- the body of the for loop needs to become a tree, as it will have consecutive nodes
	whileloopnode->rightchild = CCompileTreeNode::CreateTreeRoot(codeblock);

	// -- see if it's a single statement, or a statement block
	peektoken = filebuf;
	if (!GetToken(peektoken))
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the for loop body\n");
        --gWhileLoopDepth;
		return (false);
	}

	if (peektoken.type == TOKEN_BRACE_OPEN)
    {
		// -- consume the brace, and parse an entire statement block
		filebuf = peektoken;
		result = ParseStatementBlock(codeblock, AppendToRoot(*whileloopnode->rightchild), filebuf,
                                     true);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - failed to read statement block\n");
            --gWhileLoopDepth;
			return (false);
		}
	}

	// else try a single expression
	else
	{
		result = TryParseStatement(codeblock, filebuf, AppendToRoot(*whileloopnode->rightchild));
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - failed to read statement block\n");
            --gWhileLoopDepth;
			return (false);
		}
	}

	// notify the while node of the end of the loop statements
    whileloopnode->SetEndOfLoopNode(tempendofloop);

    // -- success - pop the while node off the stack
    --gWhileLoopDepth;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseFuncDefinition():  A function has a well defined syntax.
// ====================================================================================================================
bool8 TryParseFuncDefinition(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- use temporary vars, to ensure we dont' change the actual bufptr, unless successful
	tReadToken returntype(filebuf);
	if (!GetToken(returntype))
		return (false);

	// -- see if we found a registered type
	if (returntype.type != TOKEN_REGTYPE)
		return (false);

	eVarType regreturntype = GetRegisteredType(returntype.tokenptr, returntype.length);

	// -- see if the next token is an identifier
	tReadToken idtoken = returntype;
	if (!GetToken(idtoken))
		return (false);

	if (idtoken.type != TOKEN_IDENTIFIER)
		return (false);

    // -- see if this is a namespaced function declaration
    bool8 usenamespace = false;
    tReadToken nsnametoken(idtoken);
    tReadToken nstoken(idtoken);
    if (GetToken(nstoken) && nstoken.type == TOKEN_NAMESPACE)
    {
        usenamespace = true;
        // -- we'd better find another identifier
        idtoken = nstoken;
        if (!GetToken(idtoken) || idtoken.type != TOKEN_IDENTIFIER)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          idtoken.linenumber,
                          "Error - Expecting an identifier after namespace %s::\n",
                          TokenPrint(nsnametoken));
            return (false);
        }
    }

    // -- ensure the next token is an open parenthesis, making this a function definition
    tReadToken peektoken(idtoken);
    if (!GetToken(peektoken))
        return (false);

    if (peektoken.type != TOKEN_PAREN_OPEN)
        return (false);

    // -- we're committed to a function definition
    filebuf = peektoken;

    // -- find the namespace to which this function belongs
    tFuncTable* functable = NULL;
    if (usenamespace)
    {
        // -- see if we need to create a new namespace
        CNamespace* nsentry = codeblock->GetScriptContext()->
                                         FindOrCreateNamespace(TokenPrint(nsnametoken), true);
        if (!nsentry)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken.linenumber,
                          "Error - Failed to find/create Namespace: %s\n",
                          TokenPrint(nsnametoken));
            return (false);
        }

        functable = nsentry->GetFuncTable();
    }

    // -- no namespace - must be a global function
    else
    {
        functable = codeblock->GetScriptContext()->GetGlobalNamespace()->GetFuncTable();
    }

    if (!functable)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      peektoken.linenumber,
                      "Error - How do we not have a function table???\n");
        return (false);
    }

    // -- see if this function already existed
    uint32 funchash = Hash(idtoken.tokenptr, idtoken.length);
    uint32 nshash = usenamespace ? Hash(nsnametoken.tokenptr, nsnametoken.length) : 0;
	CFunctionEntry* exists = functable->FindItem(funchash);
    CFunctionEntry* curfunction = exists;

    // -- if thus function doesn't exist, we're defining it now
    if (! exists)
    {
	    curfunction = FuncDeclaration(codeblock->GetScriptContext(), nshash, TokenPrint(idtoken),
                                      Hash(TokenPrint(idtoken)), eFuncTypeScript);
        codeblock->smFuncDefinitionStack->Push(curfunction, NULL, 0);
    }

    else
    {
        codeblock->smFuncDefinitionStack->Push(exists, NULL, 0);
    }

    // get the function context
    CFunctionContext* funccontext = curfunction->GetContext();

    // $$$TZA TYPE__array - do we need to add additional support?
    // -- first parameter is always the return type
    int32 paramcount = 0;
    if (!exists)
        funccontext->AddParameter("__return", Hash("__return"), regreturntype, 1, 0);

    else if (exists->GetReturnType() != regreturntype)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - return type doesn't match - Removing %s()\n"
                      "re-Exec() script to redefine\n", exists->GetName());

        // -- delete the function entirely - re-executing the script will redefine
        // -- it with the (presumably) updated signature
        functable->RemoveItem(funchash);
        TinFree(exists);

        return (false);
    }

    ++paramcount;

    // -- now we build the parameter list
    while (true)
    {
        // -- read either a parameter, or the closing parenthesis
        tReadToken paramtypetoken(filebuf);
        if (!GetToken(paramtypetoken))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        // -- see if the paramtype is actually the closing parenthesis
        if (paramtypetoken.type == TOKEN_PAREN_CLOSE)
        {
            // -- we're done with the parameter list
            filebuf = paramtypetoken;
            break;
        }

        // -- ensure we read a valid type (also, no void parameters)
        bool8 param_is_array = false;
        eVarType paramtype = GetRegisteredType(paramtypetoken.tokenptr, paramtypetoken.length);
        if (paramtype < FIRST_VALID_TYPE)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - invalid parameter type\n");
            return (false);
        }

        // -- get the parameter name (or possibly an '[]' denoting the param is an array)
        tReadToken paramname(paramtypetoken);
        if (!GetToken(paramname))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - invalid parameter identifier\n");
            return (false);
        }

        // -- see if we've got an array
        if (paramname.type == TOKEN_SQUARE_OPEN)
        {
            if (!GetToken(paramname) || paramname.type != TOKEN_SQUARE_CLOSE)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - expecting ']'\n");
                return (false);
            }

            // -- no support for arrays of hashtables (yet?)
            if (paramtype == TYPE_hashtable)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - arrays of hashtables is not supported.\n");
            }

            // -- set the bool
            param_is_array = true;

            // -- read the param name now
            if (!GetToken(paramname))
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - expecting parameter identifier\n");
                return (false);
            }
        }

        if (paramname.type != TOKEN_IDENTIFIER)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - invalid parameter identifier\n");
            return (false);
        }

        // -- so far so good
        filebuf = paramname;

        // -- if the function doesn't exists, add the parameter
        if (!exists)
        {
            // -- add the parameter to the context
            if (!funccontext->AddParameter(TokenPrint(paramname), Hash(TokenPrint(paramname)), paramtype,
                                           param_is_array ? -1 : 1, 0))
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - unable to add parameter %s to function declaration %s\n",
                              TokenPrint(paramname), TokenPrint(idtoken));
                return (false);
            }
        }

        // -- else ensure the parameters match
        else
        {
            int cur_param_count = exists->GetContext()->GetParameterCount();
            CVariableEntry* paramexists = paramcount < cur_param_count
                                          ? exists->GetContext()->GetParameter(paramcount)
                                          : NULL;
            if (!paramexists || paramexists->GetType() != paramtype ||
                (paramtype != TYPE_hashtable && paramexists->IsArray() != param_is_array))
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - function signature does not match: %s\n"
                              "Removing %s() - re-Exec() to redefine\n",
                              TokenPrint(idtoken), exists->GetName());

                // -- delete the function entirely - re-executing the script will redefine
                // -- it with the (presumably) updated signature
                functable->RemoveItem(funchash);
                TinFree(exists);

                return (false);
            }
        }

        // -- increment the parameter count
        ++paramcount;

        // -- see if we've got a comma
        tReadToken peektoken(filebuf);
        if (!GetToken(peektoken) || (peektoken.type != TOKEN_COMMA && peektoken.type != TOKEN_PAREN_CLOSE))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken.type == TOKEN_COMMA)
        {
            // -- if we do have a comma, ensure the token after it is the next param type
            tReadToken peektoken2(peektoken);
            if (!GetToken(peektoken2) || peektoken2.type != TOKEN_REGTYPE)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              peektoken.linenumber, "Error - expecting ')'\n");
                return (false);
            }

            // -- consume the comma
            filebuf = peektoken;
        }
    }

    // see if we're simply declaring the function
    peektoken = filebuf;
    if (!GetToken(peektoken))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting '{'\n");
        return (false);
    }

    // -- see if this is an OnCreate() function, and if we're "deriving" the namespace
    // -- syntax is:  void ChildNamespace::OnCreate() : Parentnamespace { ... }
    uint32 derived_hash = 0;
    static uint32 oncreate_hash = Hash("OnCreate");
    if (funchash == oncreate_hash)
    {
        // -- as a "constructor", we want to enforce no parameters, and potentially specifying a derivation
        if (paramcount != 1)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - OnCreate() methods are constructors\nNot eligible for parameters.\n");
            return (false);
        }

        // -- see if we're specifying a derivation
        if (peektoken.type == TOKEN_COLON)
        {
            // -- we need a derivation identifier
            tReadToken parenttoken(peektoken);
            if (!GetToken(parenttoken) || parenttoken.type != TOKEN_IDENTIFIER)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - OnCreate():  expecting derived namespace identifier.\n");
                return (false);
            }

            // -- set the derived namespace, which will become part of the function declaration node
            derived_hash = Hash(parenttoken.tokenptr, parenttoken.length);

            // -- committed
            peektoken = parenttoken;
            if (!GetToken(peektoken))
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - OnCreate() declaration:  expecting '{' or ';'.\n");
                return (false);
            }
        }
    }

    if (peektoken.type == TOKEN_SEMICOLON)
    {
        // -- just a declaration
        filebuf = peektoken;

        // -- clear the active function definition
        CObjectEntry* dummy = NULL;
        int32 dumm_offset;
        codeblock->smFuncDefinitionStack->Pop(dummy, dumm_offset);

        // -- and we're done
        return (true);
    }

    // -- after the function prototype, we should have the statement body, beginning with a brace
    if (peektoken.type != TOKEN_BRACE_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting '{'\n");
        return (false);
    }

    // -- committed to a function definition
    filebuf = peektoken;

    // -- add a funcdecl node, and set its left child to be the statement block
    CFuncDeclNode* funcdeclnode = TinAlloc(ALLOC_TreeNode, CFuncDeclNode, codeblock, link,
                                           filebuf.linenumber, idtoken.tokenptr, idtoken.length,
                                           usenamespace ? nsnametoken.tokenptr : "",
                                           usenamespace ? nsnametoken.length : 0, derived_hash);

    // -- read the function body
    int32 result = ParseStatementBlock(codeblock, funcdeclnode->leftchild, filebuf, true);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unabled to parse statement block\n");
        return (false);
    }

    // -- we're going to force every script function to have a return value, to ensure
    // -- we can consistently pop the stack after every function call regardless of return type
    // -- this node will never be hit, if a "real" return statement was found
    CFuncReturnNode* funcreturnnode = TinAlloc(ALLOC_TreeNode, CFuncReturnNode, codeblock,
                                               AppendToRoot(*funcdeclnode->leftchild),
                                               filebuf.linenumber);

    CValueNode* nullreturn = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                      funcreturnnode->leftchild, filebuf.linenumber, "", 0, false,
                                      TYPE_int);
    Unused_(nullreturn);

    // -- clear the active function definition
    CObjectEntry* dummy = NULL;
    int32 dummy_offset = 0;
    codeblock->smFuncDefinitionStack->Pop(dummy, dummy_offset);

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseFuncCall():  A function call has a well defined syntax.
// ====================================================================================================================
bool8 TryParseFuncCall(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link,
                      bool8 ismethod)
{
	// -- see if the next token is an identifier
	tReadToken idtoken(filebuf);
	if (!GetToken(idtoken))
		return (false);

	if (idtoken.type != TOKEN_IDENTIFIER)
		return (false);

    // -- see if this is a namespaced function declaration
    bool8 usenamespace = false;
    tReadToken nsnametoken(idtoken);
    tReadToken nstoken(idtoken);
    if (GetToken(nstoken) && nstoken.type == TOKEN_NAMESPACE)
    {
        usenamespace = true;
        // -- we'd better find another identifier
        idtoken = nstoken;
        if (!GetToken(idtoken) || idtoken.type != TOKEN_IDENTIFIER)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          idtoken.linenumber,
                          "Error - Expecting an identifier after namespace %s::\n",
                          TokenPrint(nsnametoken));
            return (false);
        }
    }

    // -- ensure the next token is an open parenthesis, making this a function call
    tReadToken peektoken(idtoken);
    if (!GetToken(peektoken))
        return (false);

    if (peektoken.type != TOKEN_PAREN_OPEN)
        return (false);

    // -- we're committed to a function call
    filebuf = peektoken;

    // -- increment the paren stack
    ++gGlobalExprParenDepth;

    // -- if we're not explicitly a method, and we're not forcing a namespace (which by definition
    // -- is also a method), it's still possible this is a method.  However, without having the
    // -- object available, there's no way to know, so methods currently require the 'self' keyword

    // -- add a funccall node, and set its left child to be the tree of parameter assignments
    CFuncCallNode* funccallnode = TinAlloc(ALLOC_TreeNode, CFuncCallNode, codeblock, link,
                                           filebuf.linenumber, idtoken.tokenptr, idtoken.length,
                                           usenamespace ? nsnametoken.tokenptr : "",
                                           usenamespace ? nsnametoken.length : 0,
                                           ismethod);

    // -- $$$TZA add default args

    // -- create a tree root to contain all the parameter assignments
  	funccallnode->leftchild = CCompileTreeNode::CreateTreeRoot(codeblock);
	CCompileTreeNode* assignments = funccallnode->leftchild;

    // -- keep reading and assigning params, until we reach the closing parenthesis
    int32 paramindex = 0;
    while (true)
    {
        // -- see if we have a closing parenthesis
        tReadToken peektoken(filebuf);
        if (!GetToken(peektoken))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken.type == TOKEN_PAREN_CLOSE)
        {
            // -- we've found all the parameters we're going to find
            filebuf = peektoken;
            break;
        }

        // -- if we didn't find a closing parenthesis, and this isn't the first parameter, then
        // -- we'd better find the separating comma
        if (paramindex >= 1)
        {
            if (!GetToken(filebuf) || filebuf.type != TOKEN_COMMA)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber,
                              "Error - Expecting ',' after parameter %d in call to %s()\n",
                              paramindex, TokenPrint(idtoken));
                return (false);
            }
        }

        // -- increment the paramindex we add nodes starting with index 1, since 0 is the return
        ++paramindex;

        // -- create an assignment binary op
		CBinaryOpNode* binopnode = TinAlloc(ALLOC_TreeNode, CBinaryOpNode, codeblock,
                                            AppendToRoot(*assignments), filebuf.linenumber,
                                            ASSOP_Assign, true, TYPE__resolve);

   		// -- create the (parameter) value node, add it to the assignment node
		CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                         binopnode->leftchild, filebuf.linenumber, paramindex,
                                         TYPE__var);
        Unused_(valuenode);

        bool8 result = TryParseStatement(codeblock, filebuf, binopnode->rightchild);
        if (!result)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - Unable to evaluate parameter %d in call to %s()\n", paramindex,
                          TokenPrint(idtoken));
            return (false);
        }
    }

    // -- decrement the paren stack
    --gGlobalExprParenDepth;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseBreakContinue():  A "break" or "continue" statement is valid if within the definition of a loop.
// ====================================================================================================================
bool8 TryParseBreakContinue(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- disallow break/continue statments while in the middle of parenthetical expressions
    // -- (at least until I can think of a valid example)
    if (gGlobalExprParenDepth > 0)
        return (false);

    // -- ensure the next token is the 'return' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_break && reservedwordtype != KEYWORD_continue)
        return (false);

    // -- ensure we're in the middle of compiling a loop
    if (gWhileLoopDepth < 1)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - trying parse continue / break, outside of a loop\n");
        return (false);
    }

    // -- committed
    filebuf = peektoken;

    // -- add a return node to the tree, and parse the return expression
    CLoopJumpNode* loopJumpNode = TinAlloc(ALLOC_TreeNode, CLoopJumpNode, codeblock, link, filebuf.linenumber,
                                           gWhileLoopStack[gWhileLoopDepth - 1], reservedwordtype == KEYWORD_break);

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseReturn():  A "return" statement is valid within a function definition.
// ====================================================================================================================
bool8 TryParseReturn(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- if we're already parsing a return statement, ensure this is non-reentrant
    if (gGlobalReturnStatement)
        return (false);

    // -- disallow return statments while in the middle of parenthetical expressions
    // -- (at least until I can think of a valid example)
    if (gGlobalExprParenDepth > 0)
        return (false);

    // -- can't return from a function, if there's no active function being defined
    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    if (codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy) == NULL)
        return (false);

    // -- ensure the next token is the 'return' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_return)
        return (false);

    // -- committed
    filebuf = peektoken;
    gGlobalReturnStatement = true;

    // -- add a return node to the tree, and parse the return expression
    CFuncReturnNode* returnnode = TinAlloc(ALLOC_TreeNode, CFuncReturnNode, codeblock, link,
                                           filebuf.linenumber);
    bool8 result = TryParseStatement(codeblock, filebuf, returnnode->leftchild);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - failed to parse 'return' statement\n");
		gGlobalReturnStatement = false;
		return (false);
	}

    // -- ensure we have a non-empty return - all functions return a value
    if (!returnnode->leftchild)
    {
        CValueNode* nullreturn = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                          returnnode->leftchild, filebuf.linenumber, "", 0, false,
                                          TYPE_int);
        Unused_(nullreturn);
    }

    // -- reset the global
    gGlobalReturnStatement = false;

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseArrayHash():  Used to dereference for both arrays and hashtables, parse an expression within []'s.
// ====================================================================================================================
bool8 TryParseArrayHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    tReadToken nexttoken(filebuf);
    if (!GetToken(nexttoken) || nexttoken.type != TOKEN_SQUARE_OPEN)
        return (false);

    // -- committed to an array hash, comma delineated sequence of statements
    filebuf = nexttoken;

	CCompileTreeNode** arrayhashlink = &link;

    // -- first we push a "0" hash - this will get bumped down every time we create a new
    // -- CArrayHash node
    CValueNode* valnode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, link,
                                   filebuf.linenumber, "", 0, false, TYPE_int);
    Unused_(valnode);

    // -- create a temp link, to look for the next array hash statement
    int32 hashexprcount = 0;
    while (true)
    {
        tReadToken hashexpr(filebuf);
        if (!GetToken(hashexpr))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ']'\n");
            return (false);
        }

        // -- see if the paramtype is actually the closing parenthesis
        if (hashexpr.type == TOKEN_SQUARE_CLOSE)
        {
            // -- we're done with the hash value list
            filebuf = hashexpr;

            // -- ensure we found at least one hash value
            if (hashexprcount == 0)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - empty array hash []\n");
                return (false);
            }
            else
                return (true);
            break;
        }

        // -- if this isn't our first hash expr, then we'd better find a comma
        if (hashexprcount > 0)
        {
            if (hashexpr.type != TOKEN_COMMA)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - expecting ']'\n");
                return (false);
            }

            // -- consume the comma
            filebuf = hashexpr;
        }

        // -- we're not done - create an ArrayHashNode
        ++hashexprcount;
        ++gGlobalExprParenDepth;
        CCompileTreeNode* templink = NULL;
        CArrayHashNode* ahn = TinAlloc(ALLOC_TreeNode, CArrayHashNode, codeblock, templink,
                                       filebuf.linenumber);

        if (!TryParseStatement(codeblock, filebuf, ahn->rightchild))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ']'\n");
            return (false);
        }
        --gGlobalExprParenDepth;

        // -- hook up the nodes - the original arrayhashlink is now the left child of the AHN
        ahn->leftchild = *arrayhashlink;

        // -- which is now replaced by the AHN
        *arrayhashlink = ahn;

        // -- the chain of AHNs continues down the left children...  the left child of an AHN
        // -- is always the current hash, the right child is the string to be hashed and appended
        arrayhashlink = &ahn->leftchild;

        // -- ensure we didn't exit the TryParseStatement with a ';' or a ')'
        tReadToken peektoken(filebuf);
        if (!GetToken(peektoken) || peektoken.type == TOKEN_SEMICOLON ||
            peektoken.type == TOKEN_PAREN_CLOSE)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ']'\n");
            return (false);
        }
    }

    return (true);

}
// ====================================================================================================================
// TryParseHash():  The keyword "hash" has a well defined syntax.
// ====================================================================================================================
bool8 TryParseHash(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- ensure the next token is the 'hash' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_hash)
        return (false);

    // -- we're committed to a hash expression
    filebuf = peektoken;

    // -- the complete format is: hash("string")
    // -- read an open parenthesis
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hash() expression, expecting '('\n");
        return (false);
    }

    // -- next, we read a non-empty string
    tReadToken string_token(peektoken);
    if (!GetToken(string_token) || string_token.type != TOKEN_STRING || string_token.length == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hash() expression, expecting a non-empty string literal\n");
        return (false);
    }

    // -- read the closing parenthesis
    peektoken = string_token;
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hash() expression, expecting ')'\n");
        return (false);
    }

    // -- update the file buf
    filebuf = peektoken;

    // -- hash expressions resolve at *compile* time, directly into values.
    // -- because these are literals, add the string to the dictionary, as it may help debugging
    uint32 hash_value = Hash(string_token.tokenptr, string_token.length, true);
    char hash_value_buf[32];
    sprintf_s(hash_value_buf, 32, "%d", hash_value);
    CValueNode* hash_node = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, link, filebuf.linenumber, hash_value_buf,
                                     (int32)strlen(hash_value_buf), false, TYPE_int);

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseSchedule():  The keyword "schedule" has a well defined syntax, similar to a function call.
// ====================================================================================================================
bool8 TryParseSchedule(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- ensure the next token is the 'new' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_schedule && reservedwordtype != KEYWORD_execute &&
        reservedwordtype != KEYWORD_repeat)
        return (false);

    // -- see if we're parsing an execute statement - same as a schedule, but executes immediately
    // -- (right there in place, not the same as a schedule with a '0' duration on the next frame)
    bool8 immediate_execution = (reservedwordtype == KEYWORD_execute);
    bool8 repeat_execution = (reservedwordtype == KEYWORD_repeat);

    // -- format is schedule(objid, time, funchash, arg1, ... argn);
    // -- formate is execute(objid, funchash, arg1, ... argn);
    // -- ensure the next token is an open parenthesis, making this a function call
    if (!GetToken(peektoken))
        return (false);

    if (peektoken.type != TOKEN_PAREN_OPEN)
        return (false);

    // -- we're committed to a schedule call
    filebuf = peektoken;

    // -- increment the paren stack
    ++gGlobalExprParenDepth;

    // -- the left child is the tree resolving to an objectid
    CCompileTreeNode* templink = NULL;
    bool8 result = TryParseStatement(codeblock, filebuf, templink);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - Unable to resolve object ID in schedule/execute() call\n");
        return (false);
    }

    // -- read a comma next
    peektoken = filebuf;
    if (!GetToken(peektoken))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ',' in schedule/execute() call\n");
        return (false);
    }

    // -- read the delay (msec)
    // $$$TZA read a statement?  tree resolving to the scheduled delay time?
    int32 delaytime = 0;
    if (!immediate_execution)
    {
        tReadToken delaytoken(peektoken);
        if (!GetToken(delaytoken) || delaytoken.type != TOKEN_INTEGER)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          delaytoken.linenumber,
                          "Error - expecting delay (msec) in schedule/execute() call\n");
            return (false);
        }

        char delaybuf[kMaxTokenLength];
        SafeStrcpy(delaybuf, delaytoken.tokenptr, delaytoken.length + 1);
        delaytime = Atoi(delaybuf);

        // -- read a comma next
        peektoken = delaytoken;
        if (!GetToken(peektoken))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken.linenumber,
                          "Error - expecting ',' in schedule/execute() call\n");
            return (false);
        }
    }

    // -- at this point we're obviously committed, the rightchild of a CScheduleNode is a
    // -- CSchedFuncNode, who's left child resolves to the hashvalue identifying a function,
    // -- and the right child is the root of the parameter assignments
    filebuf = peektoken;

    // -- add a CScheduleNode node
    CScheduleNode* schedulenode = TinAlloc(ALLOC_TreeNode, CScheduleNode, codeblock, link,
                                           filebuf.linenumber, delaytime, repeat_execution);

    // -- set its left child to be the tree resolving to an object ID
    schedulenode->leftchild = templink;

    // -- add a CSchedFuncNode node
    CSchedFuncNode* schedulefunc = TinAlloc(ALLOC_TreeNode, CSchedFuncNode, codeblock,
                                            schedulenode->rightchild, filebuf.linenumber,
                                            immediate_execution);

    // -- the left child is the tree resolving to a function hash
    result = TryParseStatement(codeblock, filebuf, schedulefunc->leftchild);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - Unable to resolve function hash ID in schedule() call\n");
        return (false);
    }

    // -- create a tree root to contain all the parameter assignments
  	schedulefunc->rightchild = CCompileTreeNode::CreateTreeRoot(codeblock);
	CCompileTreeNode* assignments = schedulefunc->rightchild;

    // -- keep reading and assigning params, until we reach the closing parenthesis
    int32 paramindex = 0;
    while (true)
    {
        // -- see if we have a closing parenthesis
        tReadToken peektoken(filebuf);
        if (!GetToken(peektoken))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken.linenumber,
                          "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken.type == TOKEN_PAREN_CLOSE)
        {
            // -- we've found all the parameters we're going to find
            filebuf = peektoken;
            break;
        }

        // -- if we didn't find a closing parenthesis, we'd better find the separating comma
        if (!GetToken(filebuf) || filebuf.type != TOKEN_COMMA)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - Expecting ',' after parameter %d in schedule() call\n",
                          paramindex);
            return (false);
        }

        // -- increment the paramindex we add nodes starting with index 1, since 0 is the return
        ++paramindex;

        // -- create a schedule param node
		CSchedParamNode* schedparamnode = TinAlloc(ALLOC_TreeNode, CSchedParamNode, codeblock,
                                                   AppendToRoot(*assignments), filebuf.linenumber,
                                                   paramindex);

        bool8 result = TryParseStatement(codeblock, filebuf, schedparamnode->leftchild);
        if (!result)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - Unable to evaluate parameter %d in schedule() statement\n",
                          paramindex);
            return (false);
        }
    }

    // -- decrement the paren stack
    --gGlobalExprParenDepth;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseCreateObject():  Creating an object has a well defined syntax.
// ====================================================================================================================
bool8 TryParseCreateObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- if we're already parsing a destroy statement, ensure this is non-reentrant
    if (gGlobalCreateStatement)
        return (false);

    // -- ensure the next token is the 'new' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
	if (reservedwordtype != KEYWORD_create && reservedwordtype != KEYWORD_createlocal)
        return (false);

	// -- see if we're creating a local object - one that is destructed as soon as the function context is popped
	bool local_object = (reservedwordtype == KEYWORD_createlocal);

    // -- committed
    filebuf = peektoken;
    gGlobalCreateStatement = true;

    tReadToken classtoken(filebuf);
    if (!GetToken(classtoken) || classtoken.type != TOKEN_IDENTIFIER)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting class name\n");
        gGlobalCreateStatement = false;
        return (false);
    }

    // -- read an open parenthesis
    tReadToken nexttoken(classtoken);
    if (!GetToken(nexttoken) || nexttoken.type != TOKEN_PAREN_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), nexttoken.linenumber,
                      "Error - expecting '('\n");
        gGlobalCreateStatement = false;
        return (false);
    }

    // -- see if we have an expression which will resolve to the object name
    CCompileTreeNode* obj_name_expr_root = NULL;
    CCompileTreeNode** templink = &obj_name_expr_root;

    // -- if we read a valid expression, update the token
    tReadToken objnameexpr(nexttoken);
    if (TryParseExpression(codeblock, objnameexpr, *templink))
    {
        nexttoken = objnameexpr;
    }

    // -- read the closing parenthesis
    if (!GetToken(nexttoken) || nexttoken.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), nexttoken.linenumber,
                      "Error - expecting ')'\n");
        gGlobalCreateStatement = false;
        return (false);
    }

    // -- success
    filebuf = nexttoken;

    // -- create the node
    if (obj_name_expr_root != NULL)
    {
        CCreateObjectNode* newobjnode = TinAlloc(ALLOC_TreeNode, CCreateObjectNode, codeblock,
                                                 link, filebuf.linenumber, classtoken.tokenptr,
												 classtoken.length, local_object);
        newobjnode->leftchild = obj_name_expr_root;
    }
    else
    {
        CCreateObjectNode* newobjnode = TinAlloc(ALLOC_TreeNode, CCreateObjectNode, codeblock,
                                                 link, filebuf.linenumber, classtoken.tokenptr,
												 classtoken.length, local_object);
        CValueNode* emptyname = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                         newobjnode->leftchild, filebuf.linenumber, "", 0, false,
                                         TYPE_string);

        Unused_(emptyname);
    }

    // -- reset the bool
    gGlobalCreateStatement = false;

    return (true);
}

// ====================================================================================================================
// TryParseDestroyObject():  Deleting an object has a well defined syntax.
// ====================================================================================================================
bool8 TryParseDestroyObject(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- if we're already parsing a destroy statement, ensure this is non-reentrant
    if (gGlobalDestroyStatement)
        return (false);

    // -- disallow return statments while in the middle of parenthetical expressions
    // -- (at least until I can think of a valid example)
    if (gGlobalExprParenDepth > 0)
        return (false);

    // -- ensure the next token is the 'delete' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_destroy)
        return (false);

    // -- committed
    filebuf = peektoken;
    gGlobalDestroyStatement = true;

    // -- create a destroy object node
    CDestroyObjectNode* destroyobjnode = TinAlloc(ALLOC_TreeNode, CDestroyObjectNode, codeblock,
                                                  link, filebuf.linenumber);

    // -- ensure we have a valid statement
    if (!TryParseStatement(codeblock, filebuf, destroyobjnode->leftchild))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'destroy' found, expecting an object statement\n");
        gGlobalDestroyStatement = false;
        return (false);
    }

    // -- reset the bool
    gGlobalDestroyStatement = false;

    return (true);
}

// ====================================================================================================================
// AppendToRoot():  Parse tree nodes have left/right children, but they also form a linked list at the root level.
// ====================================================================================================================
CCompileTreeNode*& AppendToRoot(CCompileTreeNode& root)
{
	CCompileTreeNode* curroot = &root;
	while (curroot && curroot->next)
		curroot = curroot->next;
	return curroot->next;
}

// ====================================================================================================================
// ParseStatementBlocK():  Parse a sequence of (any type of) statements, delineated by {}'s.
// ====================================================================================================================
bool8 ParseStatementBlock(CCodeBlock* codeblock, CCompileTreeNode*& link, tReadToken& filebuf,
                         bool8 requiresbraceclose)
{
    // -- within a statement block, since we have no scoping to variable, we only
    // -- care that the brace depth balances out.  If we require a brace, it means
    // -- we're already one level deep (completing the body of an 'if' statement)
    int32 bracedepth = requiresbraceclose ? 1 : 0;

	// -- attach the statement block root to the new link
	link = CCompileTreeNode::CreateTreeRoot(codeblock);
	CCompileTreeNode* curroot = link;

	// parse beginning at the current filebuf, and returning once the closing brace has been found
	tReadToken filetokenbuf(filebuf);

	// build the compile tree
	bool8 foundtoken = true;
	do {
        // -- a small optimization, skip whitespace and comments at the start of the loop
        if (!SkipWhiteSpace(filetokenbuf))
        {
		    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filetokenbuf.linenumber, "Error - unexpected EOF\n");
            return (false);
        }

		// -- see if we're done with this statement block
		tReadToken peekbuf(filetokenbuf);
		if (! GetToken(peekbuf))
        {
            if (bracedepth > 0)
            {
				ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filetokenbuf.linenumber, "Error - expecting '}'\n");
				return (false);
            }
            else
            {
                filebuf = filetokenbuf;
                return (true);
            }
		}

        // -- see if we've increased our brace depth
        if (peekbuf.type == TOKEN_BRACE_OPEN)
        {
            filetokenbuf = peekbuf;
            ++bracedepth;
            continue;
        }

		// -- see if we're done
		if (peekbuf.type == TOKEN_BRACE_CLOSE)
        {
			filetokenbuf = peekbuf;
            --bracedepth;

            // -- see if we've balanced out
            if (bracedepth == 0) {
                filebuf = filetokenbuf;
				return (true);
            }
            else
                continue;
		}

		// -- parsing node priority
		bool8 found = false;
		found = found || TryParseVarDeclaration(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseFuncDefinition(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseStatement(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseIfStatement(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseWhileLoop(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseForLoop(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseDestroyObject(codeblock, filetokenbuf, curroot->next);

		if (found)
        {
			// -- always add to the end of the current root linked list
            while (curroot && curroot->next)
                curroot = curroot->next;
		}
		else
        {
			// -- not found - dump out the token
			foundtoken = GetToken(filetokenbuf);
			if (foundtoken)
            {
				ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filetokenbuf.linenumber, "Unhandled token: [%s] %s, line %d\n",
                              gTokenTypeStrings[filetokenbuf.type], TokenPrint(filetokenbuf),
                              filetokenbuf.linenumber);
			}
            return (false);
		}
	} while (foundtoken);

	return (true);
}

// ====================================================================================================================
// -- Implementation of functions to parse files, text blocks...
static bool8 gDebugParseTree = false;

// ====================================================================================================================
// ParseFile():  Parse and compile a given file.
// ====================================================================================================================
CCodeBlock* ParseFile(CScriptContext* script_context, const char* filename)
{
	// -- see if we can open the file
	const char* filebuf = ReadFileAllocBuf(filename);
    return ParseText(script_context, filename, filebuf);
}

// ====================================================================================================================
// ParseText();  Parse and compile a text block (loaded from the given file)
// ====================================================================================================================
CCodeBlock* ParseText(CScriptContext* script_context, const char* filename, const char* filebuf)
{

#if DEBUG_CODEBLOCK
    if (GetDebugCodeBlock())
    {
        printf("\n*** COMPILING: %s\n\n", filename && filename[0] ? filename : "<stdin>");
    }
#endif

    // -- ensure at the start of parsing any text, we reset the paren depth
    gGlobalExprParenDepth = 0;

    // -- sanity check
    if (!filebuf)
        return (NULL);

    CCodeBlock* codeblock = TinAlloc(ALLOC_CodeBlock, CCodeBlock, script_context, filename);

	// create the starting root, initial token, and parse the existing statements
	CCompileTreeNode* root = CCompileTreeNode::CreateTreeRoot(codeblock);
	tReadToken parsetoken(filebuf, 0);
	if (!ParseStatementBlock(codeblock, root->next, parsetoken, false))
    {
		ScriptAssert_(script_context, 0, codeblock->GetFileName(), parsetoken.linenumber,
                      "Error - failed to ParseStatementBlock()\n");
        codeblock->SetFinishedParsing();
        return (NULL);
	}

	// dump the tree
    if (gDebugParseTree)
    {
	    DumpTree(root, 0, false, false);
    }

    // we successfully created the tree, now calculate the size needed by running through the tree
    int32 size = codeblock->CalcInstrCount(*root);
    if (size < 0)
    {
		ScriptAssert_(script_context, 0, codeblock->GetFileName(), -1,
                      "Error - failed to compile file: %s", codeblock->GetFileName());

        // -- failed
        codeblock->SetFinishedParsing();
        DestroyTree(root);
        return (NULL);
    }

    codeblock->AllocateInstructionBlock(size, codeblock->GetLineNumberCount());

    // -- run through the tree again, this time actually compiling it
    if (!codeblock->CompileTree(*root))
    {
		ScriptAssert_(script_context, 0, codeblock->GetFileName(), -1,
                      "Error - failed to compile tree for file: %s", codeblock->GetFileName());
        // -- failed
        codeblock->SetFinishedParsing();
        DestroyTree(root);
        return (NULL);
    }

    // -- destroy the tree
    DestroyTree(root);

    // -- return the result
	return (codeblock);
}

// ====================================================================================================================
// SaveBinary():  Write the compiled byte code to a binary file.
// ====================================================================================================================
bool8 SaveBinary(CCodeBlock* codeblock, const char* binfilename)
{
    if (!codeblock || !binfilename)
        return (false);

  	// -- open the file
	FILE* filehandle = NULL;
	if (binfilename)
    {
		 int32 result = fopen_s(&filehandle, binfilename, "wb");
		 if (result != 0)
         {
             ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                           "Error - unable to write file %s\n", binfilename);
			 return (false);
         }
	}

	if (!filehandle)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                      "Error - unable to write file %s\n", binfilename);
		return (false);
    }
    setvbuf(filehandle, NULL, _IOFBF, BUFSIZ);

    // -- write the version
    int32 version = kCompilerVersion;
    int32 instrwritten = (int32)fwrite((void*)&version, sizeof(int32), 1, filehandle);
    if (instrwritten != 1)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                      "Error - unable to write file %s\n", binfilename);
        return (false);
    }

    // -- write the instrcount
    int32 instrcount = codeblock->GetInstructionCount();
    instrwritten = (int32)fwrite((void*)&instrcount, sizeof(int32), 1, filehandle);
    if (instrwritten != 1)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                      "Error - unable to write file %s\n", binfilename);
        return (false);
    }

    // -- write the linenumber count
#if DEBUG_COMPILE_SYMBOLS
    int32 linenumbercount = codeblock->GetLineNumberCount();
#else
    int32 linenumbercount = 0;
#endif

    instrwritten = (int32)fwrite((void*)&linenumbercount, sizeof(int32), 1, filehandle);
    if (instrwritten != 1)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                      "Error - unable to write file %s\n", binfilename);
        return (false);
    }

    // -- write the instruction block
    int32 remaining = codeblock->GetInstructionCount();
    uint32* instrptr = codeblock->GetInstructionPtr();
    while (remaining > 0)
    {
        int32 writecount = remaining > (BUFSIZ >> 2) ? (BUFSIZ >> 2) : remaining;
        remaining -= writecount;
        int32 instrwritten = (int32)fwrite((void*)instrptr, (int32)sizeof(uint32), writecount, filehandle);
        fflush(filehandle);
        if (instrwritten != writecount)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                          "Error - unable to write file %s\n", binfilename);
            return (false);
        }
        instrptr += writecount;
    }

#if DEBUG_COMPILE_SYMBOLS

    // -- write the debugger line numbers / offsets block
    remaining = codeblock->GetLineNumberCount();
    instrptr = codeblock->GetLineNumberPtr();

    while (remaining > 0) {
        int32 writecount = remaining > (BUFSIZ >> 2) ? (BUFSIZ >> 2) : remaining;
        remaining -= writecount;
        int32 instrwritten = (int32)fwrite((void*)instrptr, (int32)sizeof(uint32), writecount, filehandle);
        fflush(filehandle);
        if (instrwritten != writecount) {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                          "Error - unable to write file %s\n", binfilename);
            return (false);
        }
        instrptr += writecount;
    }

#endif

    // -- close the file before we leave
	fclose(filehandle);

#if DEBUG

    // -- the total byte size, is:
    // --     version (uint32)
    // --     the # instructions (uint32)
    // --     instructions array  (instrcount * uint32)
    // --     the # line/offset entries (uint32)
    // --     line/offset array  (count * uint32)
    int32 totalsize = (int32)sizeof(uint32) * 3;
    totalsize += (int32)codeblock->GetInstructionCount() * (int32)sizeof(uint32);
    totalsize += linenumbercount * (int32)sizeof(uint32);
    TinPrint(codeblock->GetScriptContext(), "LoadBinary file: %s, size: %d\n",
             binfilename, totalsize);

#endif

    return (true);
}

// ====================================================================================================================
// LoadBinary():  Load the compiled byte code for a given file.
// ====================================================================================================================
CCodeBlock* LoadBinary(CScriptContext* script_context, const char* filename, const char* binfilename, bool8 must_exist,
                       bool8& old_version)
{
    // -- initialize the return value
    old_version = false;

    // -- sanity check
    if (!binfilename)
        return (NULL);

	// -- open the file
	FILE* filehandle = NULL;
	if (binfilename)
    {
        int32 result = fopen_s(&filehandle, binfilename, "rb");
        if (result != 0)
        {
            if (must_exist)
            {
                ScriptAssert_(script_context, 0, "<internal>", -1, "Error - failed to load file: %s\n", binfilename);
            }
            else
            {
                TinPrint(script_context, "Unable to open file: %s\n", binfilename);
            }
            return (NULL);
        }
	}

	if (!filehandle)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - failed to load file: %s\n", binfilename);
		return (NULL);
    }

    // -- read the version
    int32 version = -1;
    int32 instrread = (int32)fread((int32*)&version, sizeof(int32), 1, filehandle);
    if (ferror(filehandle) || instrread != 1)
    {
        fclose(filehandle);
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to read file: %s\n", binfilename);
        return (NULL);
    }

    // -- if the version is not current, close and recomile
    if (version != kCompilerVersion)
    {
        fclose(filehandle);
        old_version = true;
        return (NULL);
    }

    // -- read the instrcount
    int32 instrcount = -1;
    instrread = (int32)fread((int32*)&instrcount, sizeof(int32), 1, filehandle);
    if (ferror(filehandle) || instrread != 1)
    {
        fclose(filehandle);
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to read file: %s\n", binfilename);
        return (NULL);
    }

    if (instrcount <= 0)
    {
	    fclose(filehandle);
		return (NULL);
    }

    // -- read the linenumber count
    int32 linenumbercount = -1;
    instrread = (int32)fread((int32*)&linenumbercount, sizeof(int32), 1, filehandle);
    if (ferror(filehandle) || instrread != 1)
    {
        fclose(filehandle);
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to read file: %s\n", binfilename);
        return (NULL);
    }

    // -- create the codeblock
    CCodeBlock* codeblock = TinAlloc(ALLOC_CodeBlock, CCodeBlock, script_context, filename);
    codeblock->AllocateInstructionBlock(instrcount, linenumbercount);

    // -- read the file into the codeblock
    uint32* readptr = codeblock->GetInstructionPtr();
    instrread = (int32)fread(readptr, sizeof(uint32), (int32)instrcount, filehandle);
    if (ferror(filehandle))
    {
        fclose(filehandle);
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to read file: %s\n", binfilename);
        codeblock->SetFinishedParsing();
        return (NULL);
    }

    if (instrread != instrcount)
    {
	    fclose(filehandle);
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to read file: %s\n", binfilename);
        codeblock->SetFinishedParsing();
        return (NULL);
    }

    // -- read the debug symbols into the codeblock
    // -- note:  the compile flag is only to prevent writing excess debug info
    // -- if the debug line offsets are already in the binary, might as well read them
    if (linenumbercount > 0)
    {
        uint32* readptr = codeblock->GetLineNumberPtr();
        instrread = (int32)fread(readptr, sizeof(uint32), (int32)linenumbercount, filehandle);
        if (ferror(filehandle))
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to read file: %s\n", binfilename);
            codeblock->SetFinishedParsing();
            return (NULL);
        }

        if (instrread != linenumbercount)
        {
	        fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to read file: %s\n", binfilename);
            codeblock->SetFinishedParsing();
            return (NULL);
        }

        // -- make sure we also set the array count, after reading in the line number array
        else
        {
            codeblock->SetLineNumberCount(linenumbercount);
        }
    }

    // -- close the file
    fclose(filehandle);

    // -- return the result
    codeblock->SetFinishedParsing();
    return (codeblock);
}

// ====================================================================================================================
// AddVariable():  Adds an entry to a variable table (global, or local to a function)
// ====================================================================================================================
CVariableEntry* AddVariable(CScriptContext* script_context, tVarTable* curglobalvartable,
                            CFunctionEntry* curfuncdefinition, const char* varname, uint32 varhash,
                            eVarType vartype, int array_size)
{
    // get the function we're currently defining
    CVariableEntry* ve = NULL;
    if (curfuncdefinition)
    {
        // -- search the local var table for the executing function
        ve = curfuncdefinition->GetContext()->GetLocalVar(varhash);
        if (!ve)
            ve = curfuncdefinition->GetContext()->AddLocalVar(varname, varhash, vartype, array_size, false);
    }

    // -- not defining a function - see if we're compiling
    else if (curglobalvartable)
    {
        // -- if the variable already exists, we're done
        ve = curglobalvartable->FindItem(varhash);
        if (!ve)
        {
	        ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, script_context, varname, varhash,
                                                          vartype, array_size, false, 0, false);
	        uint32 hash = ve->GetHash();
	        curglobalvartable->AddItem(*ve, hash);
        }
    }
    else
    {
        // -- if the variable already exists, we're done
	    tVarTable* globalvartable = script_context->GetGlobalNamespace()->GetVarTable();
        ve = globalvartable->FindItem(varhash);
        if (!ve)
        {
	        ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, script_context, varname, varhash, vartype, array_size,
                          false, 0, false);
	        uint32 hash = ve->GetHash();
	        globalvartable->AddItem(*ve, hash);
        }
    }
    return ve;
}

// ====================================================================================================================
// GetObjectMember():  Given a NS hash, function or object ID, Var Hash, and an array hash, find the variable entry
// ====================================================================================================================
CVariableEntry* GetObjectMember(CScriptContext* script_context, CObjectEntry*& oe, uint32 ns_hash,
                              uint32 func_or_obj, uint32 var_hash, uint32 array_hash)
{
    // -- note: these are the same 4x parameters as used to find a variable
    // -- if they don't resolve to a member, we return (NULL)

    // -- objects don't belong to a namespace
    if (ns_hash != 0)
        return (NULL);

    // -- with no ns_hash, the next parameter is an object ID - see if we can find the objecct
    oe = script_context->FindObjectEntry(func_or_obj);
    if (!oe)
        return (NULL);

    // -- see if the object has the requested member
    CVariableEntry* ve = oe->GetVariableEntry(var_hash);

    // -- if we found a variable, but it's a hashtable, and we've been given an array hash,
    // -- we need to find the variable within the hashtable
    if (ve && ve->GetType() == TYPE_hashtable && array_hash != 0)
    {
        // -- get the var table
        tVarTable* vartable = (tVarTable*)ve->GetAddr(oe ? oe->GetAddr() : NULL);

        // -- look for the entry in the vartable
        CVariableEntry* vte = vartable->FindItem(array_hash);
        if (!vte)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                            "Error - HashTable Variable %s: unable to find entry: %d\n",
                            UnHash(ve->GetHash()), UnHash(array_hash));
            return (NULL);
        }

        // -- return the vte
        ve = vte;
    }

    // -- return the result
    return (ve);
}

// ====================================================================================================================
// GetVariable():  Given a NS hash, function or object ID, Var Hash, and an array hash, find the variable entry
// ====================================================================================================================
CVariableEntry* GetVariable(CScriptContext* script_context, tVarTable* globalVarTable, uint32 ns_hash,
                            uint32 func_or_obj, uint32 var_hash, uint32 array_hash_index)
{

    // -- to retrieve the variable:
    // -- if the ns hash is zero, then the next word is the object ID
    // -- if the ns hash is non-zero, then
    // --    the next word is non-zero means the var is a local var in a function
    // --    (note:  the ns hash could be "_global" for global functions)
    // --    else if the next word is zero, it's a global variable
    // -- the last two words are, the table variable name, and the hash value, or index
    // -- if the variable is either an array or a hashtable

    CFunctionEntry* fe = NULL;
    CObjectEntry* oe = NULL;
    CNamespace* ns_entry = NULL;
    if (ns_hash != 0)
    {
        ns_entry = script_context->FindNamespace(ns_hash);
        if (!ns_entry)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Unable to find resolve variable with namespace: %s\n", UnHash(ns_hash));
            return (NULL);
        }

        // -- if the func is non-zero, then the variable is the local variable of a function
        if (func_or_obj != 0)
        {
            fe = ns_entry->GetFuncTable()->FindItem(func_or_obj);

            if (!fe)
            {
                ScriptAssert_(script_context, 0, "<internal>", -1,
                              "Error - Unable to find function: %s:() in namespace: %s\n",
                              UnHash(func_or_obj), UnHash(ns_hash));
                return (NULL);
            }
        }
    }

    // -- otherwise, if we have a '0' namespace, the next word is an object ID
    else if (func_or_obj != 0)
    {
        oe = script_context->FindObjectEntry(func_or_obj);
        if (!oe)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - Unable to find object: %d\n");
            return (NULL);
        }
    }

    // -- now we find the variable entry
    CVariableEntry* ve = NULL;

    // -- if we found an object, we need to find the member
    if (oe)
        ve = oe->GetVariableEntry(var_hash);

    // -- else if were given a function, find the local variable
    else if (fe)
    {
        ve = fe->GetContext()->GetLocalVar(var_hash);

        // -- mark the variable entry with the owning function
        if (ve)
            ve->SetFunctionEntry(fe);
    }

    // -- if we haven't found the variable yet, and if we were given a specific variable table,
    // -- find the variable there (
    if (!ve && globalVarTable)
        ve = globalVarTable->FindItem(var_hash);

    // -- if still not found - look in the context global variable table
    if (!ve)
    {
        ve = script_context->GetGlobalNamespace()->GetVarTable()->FindItem(var_hash);
    }

    // -- if we did not find the variable entry, fail
    if (!ve)
    {
        //ScriptAssert_(script_context, 0, "<internal>", -1, "Error - Unable to find variable: %s\n",
        //              UnHash(var_hash));
        return (NULL);
    }

    // -- if we did find the variable, but were given an array hash, we need to go one step deeper
    if (array_hash_index != 0)
    {
        // -- if we've got a hashtable variable, the array_hash is a hash value, not an index
        if (ve->GetType() == TYPE_hashtable)
        {
            // -- get the var table
            tVarTable* vartable = (tVarTable*)ve->GetAddr(oe ? oe->GetAddr() : NULL);

            // -- look for the entry in the vartable
            CVariableEntry* vte = vartable->FindItem(array_hash_index);
            if (!vte) {
                ScriptAssert_(script_context, 0, "<internal>", -1,
                              "Error - HashTable Variable %s: unable to find entry: %d\n",
                              UnHash(ve->GetHash()), UnHash(array_hash_index));
                return (NULL);
            }

            // -- return the vte
            ve = vte;
        }

        else if (!ve->IsArray())
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - expecting variable %s to be a hashtable or an array\n", UnHash(ve->GetHash()));
            return (NULL);
        }
    }

    return (ve);
}

// ====================================================================================================================
// FuncDeclaration():  Add a function entry to a given namespace.
// ====================================================================================================================
CFunctionEntry* FuncDeclaration(CScriptContext* script_context, uint32 namespacehash,
                                const char* funcname, uint32 funchash, EFunctionType type)
{
    const char* ns_string = NULL;
    CNamespace* nsentry = script_context->FindNamespace(namespacehash);
    if (!nsentry)
    {
        // -- during a function declaration, if the namespace doesn't exist, it's probably
        // -- because we're loading a compiled binary, where the namespace is usually created during parsing
        ns_string = TinScript::GetContext()->GetStringTable()->FindString(namespacehash);
        if (ns_string && ns_string[0])
        {
            nsentry = script_context->FindOrCreateNamespace(ns_string, true);
        }
    }

    if (!nsentry)
    {
        if (!ns_string)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to find Namespace: %s\n"
                          "This happens when the string table is deleted.\nRecompile or delete .tso files\n",
                          UnHash(namespacehash));
        }
        else
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to find Namespace: %s\n", UnHash(namespacehash));
        }
        return (NULL);
    }

    return (FuncDeclaration(script_context, nsentry, funcname, funchash, type));
}

// ====================================================================================================================
// FuncDeclaration():  Add a function entry to a given namespace.
// ====================================================================================================================
CFunctionEntry* FuncDeclaration(CScriptContext* script_context, CNamespace* nsentry,
                                const char* funcname, uint32 funchash, EFunctionType type)
{
    // -- no namespace means by definition this is a global function
    if (!nsentry)
    {
        nsentry = script_context->GetGlobalNamespace();
    }

    // -- remove any existing function decl
	CFunctionEntry* fe = nsentry->GetFuncTable()->FindItem(funchash);
	if (fe)
    {
        nsentry->GetFuncTable()->RemoveItem(fe->GetHash());
        TinFree(fe);
    }

	// -- create the function entry, and add it to the global table
	fe = TinAlloc(ALLOC_FuncEntry, CFunctionEntry, script_context, nsentry->GetHash(), funcname,
                                                   funchash, type, (void*)NULL);
	uint32 hash = fe->GetHash();
	nsentry->GetFuncTable()->AddItem(*fe, hash);
    return fe;
}

} // Tinscript

// ====================================================================================================================
// -- debug helper functions

// ====================================================================================================================
// SetDebugParseTree():  Enables the bool to display the tree every time a file/buffer is parsed.
// ====================================================================================================================
void SetDebugParseTree(bool8 torf)
{
    TinScript::gDebugParseTree = torf;
}

// ====================================================================================================================
// GetDebugParseTree():  Returns true if we're currently debugging parse trees.
// ====================================================================================================================
bool8 GetDebugParseTree() {
    return TinScript::gDebugParseTree;
}

REGISTER_FUNCTION_P1(SetDebugParseTree, SetDebugParseTree, void, bool8);

// ====================================================================================================================
// eof
// ====================================================================================================================
