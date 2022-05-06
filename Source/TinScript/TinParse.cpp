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

#include "integration.h"

// -- includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

// -- ternary expressions are complicated, as the ':' conflicts with the POD member token
static const int32 gMaxTernaryDepth = 32;
static int32 gTernaryDepth = 0;
static int32 gTernaryStack[gMaxTernaryDepth];

// -- stack for managing loops (break and continue statements need to know where to jump)
// -- applies to both while loops and switch statements
static const int32 gMaxBreakStatementDepth = 32;
static int32 gBreakStatementDepth = 0;
static CCompileTreeNode* gBreakStatementStack[gMaxBreakStatementDepth];

// ====================================================================================================================
// -- binary operators
static const char* gBinOperatorString[] =
{
	#define BinaryOperatorEntry(a, b, c) b,
	BinaryOperatorTuple
	#undef BinaryOperatorEntry
};

const char* GetBinOperatorString(eBinaryOpType bin_op)
{
	return gBinOperatorString[bin_op];
}

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

// -- math parsing (constants) ----------------------------------------------------------------------------------------

static const char* gMathConstantKeywords[] =
{
    #define MathKeywordConstantEntry(a, b) #a,
    MathKeywordConstantTuple
    #undef MathKeywordConstantEntry
};

static const char* gMathConstantStringValues[] =
{
    #define MathKeywordConstantEntry(a, b) #b,
    MathKeywordConstantTuple
    #undef MathKeywordConstantEntry
};

static int32 gMathConstantsCount = sizeof(gMathConstantKeywords) / sizeof(const char*);

const char* GetMathConstant(const char* token, size_t token_length)
{
    // sanity check
    if (token == nullptr)
        return nullptr;

    for (int32 i = 0; i < gMathConstantsCount; ++i)
    {
        if (strlen(gMathConstantKeywords[i]) != token_length)
            continue;

        if (!strncmp(token, gMathConstantKeywords[i], token_length))
            return gMathConstantStringValues[i];
    }

    // -- not found
    return nullptr;
}

// -- math parsing (unary) --------------------------------------------------------------------------------------------

static const char* gMathUnaryFunctionKeywords[] =
{
    #define MathKeywordUnaryEntry(a, b) #a,
    MathKeywordUnaryTuple
    #undef MathKeywordUnaryEntry
};

eMathUnaryFunctionType GetMathUnaryFunction(const char* token, size_t token_length)
{
    if (token == nullptr)
        return MATH_UNARY_FUNC_COUNT;

    for (int32 i = 0; i < (int32)MATH_UNARY_FUNC_COUNT; ++i)
    {
        if (!strncmp(token, gMathUnaryFunctionKeywords[i], token_length))
        {
            return (eMathUnaryFunctionType)i;
        }
    }

    // -- not found
    return MATH_UNARY_FUNC_COUNT;
}

static int32 gMathUnaryFunctionCount = sizeof(gMathUnaryFunctionKeywords) / sizeof(const char*);

const char* GetMathUnaryFuncString(eMathUnaryFunctionType math_unary_func_type)
{
    return gMathUnaryFunctionKeywords[math_unary_func_type];
}

// -- math parsing (binary) -------------------------------------------------------------------------------------------

static const char* gMathBinaryFunctionKeywords[] =
{
    #define MathKeywordBinaryEntry(a, b) #a,
    MathKeywordBinaryTuple
    #undef MathKeywordBinaryEntry
};

eMathBinaryFunctionType GetMathBinaryFunction(const char* token, size_t token_length)
{
    if (token == nullptr)
        return MATH_BINARY_FUNC_COUNT;

    for (int32 i = 0; i < (int32)MATH_BINARY_FUNC_COUNT; ++i)
    {
        if (!strncmp(token, gMathBinaryFunctionKeywords[i], token_length))
        {
            return (eMathBinaryFunctionType)i;
        }
    }

    // -- not found
    return MATH_BINARY_FUNC_COUNT;
}

static int32 gMathBinaryFunctionCount = sizeof(gMathBinaryFunctionKeywords) / sizeof(const char*);

const char* GetMathBinaryFuncString(eMathBinaryFunctionType math_binary_func_type)
{
    return gMathBinaryFunctionKeywords[math_binary_func_type];
}

// ====================================================================================================================
// -- reserved keywords
const char* gReservedKeywords[KEYWORD_COUNT] =
{
	#define ReservedKeywordEntry(a) #a,
    #define MathKeywordUnaryEntry(a, b) #a,
    #define MathKeywordBinaryEntry(a, b) #a,

	ReservedKeywordTuple
    MathKeywordUnaryTuple
    MathKeywordBinaryTuple

    #undef MathKeywordBinaryEntry
    #undef MathKeywordUnaryEntry
	#undef ReservedKeywordEntry
};

const char** GetReservedKeywords(int32& count)
{
    count = KEYWORD_COUNT;
    return (gReservedKeywords);
}

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

// ====================================================================================================================
bool8 IsMathConstant(tReadToken& ref_token, const char*& str_value)
{
    // -- math constants are parsed as strings
    tReadToken math_token(ref_token);
    if (ref_token.type != TOKEN_IDENTIFIER)
        return false;

    const char* math_str_value = GetMathConstant(math_token.tokenptr, math_token.length);
    if (math_str_value != nullptr)
    {
        ref_token.tokenptr += ref_token.length;
        str_value = math_str_value;
        return true;
    }

    // -- not a math constant
    return false;
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
    SafeStrcpy(bufferptr, kMaxTokenLength, token.tokenptr, token.length + 1);
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
// GetCommentToken(): Updates the read token if the next token is a comment. 
// ====================================================================================================================
bool8 GetCommentToken(tReadToken& token)
{
    token.type = TOKEN_NULL;

    const char* comment_start = token.inbufptr;
    int32 comment_length = 0;
    const char*& inbuf = token.inbufptr;
    int32 linenumber = token.linenumber;
	if (!inbuf)
		return (false);

    // -- we're going to count comments as whitespace
    bool8 first_whitespace = true;
    bool8 found_comment = false;
    bool8 loop_comment = false;
    do
    {
        // -- reset the flag at the start of the loop - we handle multiple comments
        loop_comment = false;

        // -- first skip actual whitespace
	    while (*inbuf == ' ' || *inbuf == '\t' || *inbuf == '\r' || *inbuf == '\n')
        {
		    if (*inbuf == '\n')
			    ++linenumber;
		    ++inbuf;

            // -- if we haven't yet found a comment, update the comment_start
            if (!found_comment)
                ++comment_start;
	    }

        // -- next comes block comments
	    if (inbuf[0] == '/' && inbuf[1] == '*')
        {
            found_comment = true;
            loop_comment = true;
            inbuf += 2;
		    while (inbuf[0] != '\0' && inbuf[1] != '\0')
            {
                if (inbuf[0] == '*' && inbuf[1] == '/')
                {
                    inbuf += 2;
                    comment_length += 2;
                    break;
                }

                if (inbuf[0] == '\n')
                    ++linenumber;
                ++inbuf;
                ++comment_length;
            }
	    }

	    // -- skip line comments
	    if (inbuf[0] == '/' && inbuf[1] == '/')
        {
            found_comment = true;
            loop_comment = true;
            inbuf += 2;
            comment_length += 2;
		    while (*inbuf != '\0' && *inbuf != '\n')
            {
			    ++inbuf;
                ++comment_length;
            }
	    }
    } while (loop_comment);

    // -- if we found a comment, update the token
    if (found_comment)
    {
        token.tokenptr = comment_start;
        //token.length = kPointerDiffUInt32(inbuf, comment_start);
        token.length = comment_length + 1;
        token.type = TOKEN_COMMENT; 

        // -- because comments are also whitespace, consuming a comment
        // needs to update the line number as well
        token.linenumber = linenumber;
    }

    // -- return if we found a comment
	return (found_comment);
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
const char symbols[] = "(),;.:?{}[]";
static const int32 kNumSymbols = (int32)strlen(symbols);

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
			// -- for parsing, we only allow void, or any type between the first and last valid...
			// -- there are types like TYPE__resolve, which will become legitimate after compilation, or
			// types like TYPE_ue_vector, which is used for conversion and binding to FVector, but not scriptable
			eVarType registeredtype = GetRegisteredType(tokenptr, length);
			if (registeredtype == TYPE_void ||
				registeredtype >= FIRST_VALID_TYPE && registeredtype <= LAST_VALID_TYPE)
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
        		int32 operatorlength_0 = (int32)strlen(gBinOperatorString[BINOP_CompareEqual]);
        		if (!Strncmp_(tokenptr, gBinOperatorString[BINOP_CompareEqual], operatorlength_0))
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

    // -- ensure the file contains *something* besides whitespace
    if (bytesread > 0)
    {
        filebuf[bytesread] = '\0';
        bool found = false;
        const char* test_char = filebuf;
        while (*test_char != '\0')
        {
            if (*test_char++ > 0x20)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            bytesread = 0;
        }
    }

    // -- if we didn't (or couldn't) have anything to read...
    if (bytesread <= 0)
    {
		delete[] filebuf;
		fclose(filehandle);
		return (NULL);
	}

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
			SafeStrcpy(tokenbuf, kMaxTokenLength, token.tokenptr, token.length + 1);
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
    // -- if this is the start of a tree (with an indent of 0), write out a label
    if (indent == 0)
        printf("\n*** DUMP TREE:\n");

	while (root)
    {
		int32 debuglength = 2048;
		char debugbuf[2048];
		char* debugptr = debugbuf;
		for (int32 i = 0; i < indent; ++i)
        {
			SafeStrcpy(debugptr, debuglength, "    ", debuglength);
			debugptr += 4;
			debuglength -= 4;
		}

        const char* branchtype = isleft ? "L-> " : isright ? "R-> " : "N-> ";
        SafeStrcpy(debugptr, debuglength, branchtype, debuglength);
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
void DumpVarTable(CObjectEntry* oe, const char* partial)
{
	// -- sanity check
	if (!oe)
		return;

    CNamespace* curentry = oe->GetNamespace();
    while (curentry)
    {
        TinPrint(oe->GetScriptContext(), "\nNamespace: %s\n", UnHash(curentry->GetHash()));
        DumpVarTable(oe->GetScriptContext(), oe, curentry->GetVarTable(), partial);
        curentry = curentry->GetNext();
    }

    // -- dump the dynamic var table as well
    if (oe->GetDynamicVarTable())
    {
        TinPrint(oe->GetScriptContext(), "\nDYNAMIC VARS:\n");
        DumpVarTable(oe->GetScriptContext(), oe, oe->GetDynamicVarTable(), partial);
    }
}

// ====================================================================================================================
// DebugVarTable():  Debug function to print out the variables in a variable table.
// ====================================================================================================================
void FormatVarEntry(CScriptContext* script_context, CVariableEntry* ve, void* val_addr,
                          char* buffer, int32 size)
{
    // -- sanity check
    if (script_context == nullptr || ve == nullptr || val_addr == nullptr || buffer == nullptr || size <= 0)
        return;

    switch (ve->GetType())
    {
        case TYPE_object:
        {
            CObjectEntry* oe = script_context->FindObjectEntry(*(uint32*)val_addr);
            if (oe != nullptr)
            {
                int32 bytes_written = 0;
                if (oe->GetNameHash() != 0 && oe->GetNamespace() != nullptr)
                {
                    bytes_written = snprintf(buffer, size, "%d: %s [%s]", oe->GetID(), UnHash(oe->GetNameHash()),
                                                                           UnHash(oe->GetNamespace()->GetHash()));
                }
                else if (oe->GetNamespace() != nullptr)
                {
                    bytes_written = snprintf(buffer, size, "%d: [%s]", oe->GetID(), UnHash(oe->GetNamespace()->GetHash()));
                }
                else
                {
                    bytes_written = snprintf(buffer, size, "%d", oe->GetID());
                }

                // -- make sure we terminate
                if (bytes_written >= size)
                    bytes_written = size - 1;
                buffer[bytes_written] = '\0';
            }
        }
        break;

        case TYPE_int:
        {
            int32 string_hash = *(uint32*)val_addr;
            const char* hashed_string = script_context->GetStringTable()->FindString(string_hash);
            if (hashed_string != nullptr && hashed_string[0] != '\0')
            {
                int32 bytes_written = snprintf(buffer, size, "%d  [0x%x `%s`]", string_hash, string_hash, hashed_string);

                // -- make sure we terminate
                if (bytes_written >= size)
                    bytes_written = size - 1;
                buffer[bytes_written] = '\0';
            }
            else
            {
                snprintf(buffer, size, "%d", string_hash);
            }
        }
        break;

        case TYPE_string:
        {
            snprintf(buffer, size, "%s", (const char*)val_addr);
        }
        break;

        default:
        {
            // -- copy the value, as a string (to a max length)
            gRegisteredTypeToString[ve->GetType()](script_context, val_addr, buffer, size);
        }
        break;
    }
}

// ====================================================================================================================
// DebugVarTable():  Debug function to print out the variables in a variable table.
// ====================================================================================================================
void DumpVarTable(CScriptContext* script_context, CObjectEntry* oe, const tVarTable* vartable, const char* partial)
{
	// -- sanity check
	if (!script_context || (!oe && !vartable))
		return;

    void* objaddr = oe ? oe->GetAddr() : NULL;

	CVariableEntry* ve = vartable->First();
    while (ve)
    {
        const char* ve_name = ve->GetName();
        if (!partial || !partial[0] || SafeStrStr(ve_name, partial) != 0)
        {
		    char valbuf[kMaxTokenLength];
            FormatVarEntry(script_context, ve, ve->GetValueAddr(objaddr), valbuf, kMaxTokenLength);
		    TinPrint(script_context, "    [%s] %s: %s\n", gRegisteredTypeNames[ve->GetType()],
                        ve->GetName(), valbuf);
        }
		ve = vartable->Next();
	}
}

// ====================================================================================================================
// DumpFuncEntry():  Debug function to print a function signature
// ====================================================================================================================
void DumpFuncEntry(CScriptContext* script_context, CFunctionEntry* fe)
{
    // -- sanity check
    if (script_context == nullptr || fe == nullptr || fe->GetContext() == nullptr)
    {
        TinPrint(script_context, "Error - Unable to find function\n");
        return;
    }

    // -- get the function context (containing parameters...)
	// -- parameter 0 is always the return value
    CFunctionContext* func_context = fe->GetContext();
    eVarType return_type = func_context->GetParameterCount() > 0 ? func_context->GetParameter(0)->GetType()
                                                                 : TYPE_void;

    // -- for registered functions, we may have a default values registration object
    CRegDefaultArgValues* default_args = fe->GetType() == eFuncTypeRegistered && fe->GetRegObject()
                                         ? fe->GetRegObject()->GetDefaultArgValues()
                                         : nullptr;

    // -- some engines have print functions that auto-append EOL, so we want to print the signature as one line
    int32 length_left = kMaxTokenLength;
    char signature_buf[kMaxTokenLength];
    char* sig_ptr = signature_buf;

    // -- print the signature start
    int len = snprintf(sig_ptr, length_left, "    %s %s(", GetRegisteredTypeName(return_type), UnHash(fe->GetHash()));
    length_left -= len;
    sig_ptr += len;

    // -- print the input parameters (starting at 1)
    for (int32 i = 1; i < func_context->GetParameterCount(); ++i)
    {
        // -- subsequent params need commas...
        CVariableEntry* param = func_context->GetParameter(i);

        bool has_default_value = false;
        const char* default_arg_name = nullptr;
        eVarType default_arg_type = TYPE_COUNT;
        void* default_arg_value = nullptr;
        if (default_args != nullptr && default_args->GetDefaultArgValue(i, default_arg_name, default_arg_type, default_arg_value))
        {
            const char* value_as_string = CRegDefaultArgValues::GetDefaultValueAsString(default_arg_type, default_arg_value, false);
            if (value_as_string != nullptr)
            {
                // -- strings continue to be a pain
                if (default_arg_type == TYPE_string)
                {
                    len = snprintf(sig_ptr, length_left, "%s%s %s = `%s`", (i >= 2 ? ", " : ""), GetRegisteredTypeName(param->GetType()),
                                                           default_arg_name, value_as_string);
                }
                else
                {
                    len = snprintf(sig_ptr, length_left, "%s%s %s = %s", (i >= 2 ? ", " : ""), GetRegisteredTypeName(param->GetType()),
                                                           default_arg_name, value_as_string);
                }
                has_default_value = true;
            }
        }

        // -- if we don't have a default value, or were unable to find/convert, print the standard param name
        if (!has_default_value)
        {
            len = snprintf(sig_ptr, length_left, "%s%s %s", (i >= 2 ? ", " : ""), GetRegisteredTypeName(param->GetType()),
                                                  UnHash(param->GetHash()));
        }
        length_left -= len;
        sig_ptr += len;
        if (length_left <= 0)
            break;
    }

    // -- close the signature string
    len = snprintf(sig_ptr, length_left, ")\n");
    sig_ptr += len;
    length_left -= len;

    // -- append the help string, if it exists
    const char* help_str = default_args != nullptr ? default_args->GetHelpString() : nullptr;
    if (help_str != nullptr && help_str[0] != '\0')
    {
        len = snprintf(sig_ptr, length_left, "        use: %s\n", help_str);
        sig_ptr += len;
        length_left -= len;
    }

    TinPrint(script_context, signature_buf);
}

// ====================================================================================================================
// DumpFuncTable():  Debug function to print the hierarchy of methods for a specific object.
// ====================================================================================================================
void DumpFuncTable(CObjectEntry* oe, const char* partial)
{
	// -- sanity check
	if (!oe)
		return;

    CNamespace* curentry = oe->GetNamespace();
    while (curentry)
    {
        TinPrint(oe->GetScriptContext(), "\nNamespace: %s\n", UnHash(curentry->GetHash()));
        DumpFuncTable(oe->GetScriptContext(), curentry->GetFuncTable(), partial);
        curentry = curentry->GetNext();
    }
}

// ====================================================================================================================
// DumpFuncTable():  Debug function to print all methods registered to a given namespace.
// ====================================================================================================================
void DumpFuncTable(CScriptContext* script_context, const tFuncTable* functable, const char* partial)
{
	// -- sanity check
	if (!functable || !script_context)
		return;

    int32 function_count = 0;
    CFunctionEntry* function_list[1024];

	CFunctionEntry* fe = functable->First();
    while (fe)
    {
        function_list[function_count++] = fe;
		fe = functable->Next();

        if (function_count >= 1024)
            break;
	}

    // -- sort the function list alphabetically
    if (function_count > 1)
    {
        auto function_sort = [](const void* a, const void* b) -> int
        {
            CFunctionEntry* func_a = *(CFunctionEntry**)a;
            CFunctionEntry* func_b = *(CFunctionEntry**)b;

            const char* name_a = func_a != nullptr ? TinScript::UnHash(func_a->GetHash()) : "";
            const char* name_b = func_b != nullptr ? TinScript::UnHash(func_b->GetHash()) : "";
            int result = _stricmp(name_a, name_b);
            return (result);
        };

        qsort(function_list, function_count, sizeof(CFunctionEntry*), function_sort);
    }

    // -- print out the function names
    for (int i = 0; i < function_count; ++i)
    {
        const char* func_name = UnHash(function_list[i]->GetHash());
        if (!partial || !partial[0]|| SafeStrStr(func_name, partial) != nullptr)
        {
            DumpFuncEntry(script_context, function_list[i]);
        }
    }
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
// AppendToRoot():  Parse tree nodes have left/right children, but they also form a linked list at the root level.
// ====================================================================================================================
CCompileTreeNode* FindChildNode(CCompileTreeNode& root, ECompileNodeType node_type)
{
	CCompileTreeNode* curroot = &root;
	while (curroot && curroot->GetType() != node_type)
		curroot = curroot->next;
	return (curroot);
}

// ====================================================================================================================
// -- Functions to parse more complicated expressions

// ====================================================================================================================
// TryParseComment():  Parse a comment (block), used for CompileToC()
// ====================================================================================================================
bool8 TryParseComment(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    tReadToken firsttoken(filebuf);
    if (!GetCommentToken(firsttoken))
        return (false);

    // -- if we actually found a comment, return true
    if (firsttoken.type == TOKEN_COMMENT)
    {
        TinAlloc(ALLOC_TreeNode, CCommentNode, codeblock, link, filebuf.linenumber,
                 firsttoken.tokenptr, firsttoken.length);
        filebuf = firsttoken;
        return (true);
    }

    // -- no comment found
    return (false);
}

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

    // -- if we have an integer, what we actually have is an object_id, we're dynamically adding a member to
    bool8 objvardecl = false;
    if (idtoken.type == TOKEN_INTEGER)
    {
        // -- store the object id
        objvardecl = true;

        // -- we'd better find a TOKEN_PERIOD, followed by an identifier
        tReadToken peektoken(idtoken);
        if (!GetToken(peektoken) || peektoken.type != TOKEN_PERIOD)
            return (false);
    }

    // -- at this point, we should have an identifier
	if (idtoken.type != TOKEN_IDENTIFIER && (idtoken.type != TOKEN_INTEGER || !objvardecl))
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

            // -- the left child of the member node resolves to the object
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
            // -- note, the objvardecl bool determines whether this value node is a literal or not
		    CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, obj_member_decl_node->leftchild,
                                             idtoken.linenumber, idtoken.tokenptr, idtoken.length, !objvardecl, TYPE_object);
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
        // -- see if we're adding a global var
        if (!objvardecl)
        {  
            int32 stacktopdummy = 0;
            CObjectEntry* dummy = NULL;
            CFunctionEntry* curfunction =
                codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);

            AddVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable, curfunction,
                        TokenPrint(idtoken), Hash(TokenPrint(idtoken)), registeredtype, array_size);
        }
/*
        else
        {
            // -- this is only available in immediate execution code
            int32 stacktopdummy = 0;
            CObjectEntry* dummy = NULL;
            CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
            uint32 funchash = curfunction ? curfunction->GetHash() : 0;
            if (funchash != 0)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              finaltoken.linenumber, "Error - dynamically declaring an object member outside a function.\n");
			    return (false);
            }

            uint32 object_id = Atoi(object_id_token.tokenptr, object_id_token.length);
            CScriptContext* script_context = TinScript::GetContext();
            script_context->AddDynamicVariable(object_id,  Hash(TokenPrint(idtoken)), registeredtype, array_size);
        }
*/
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
bool8 TryParseStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link, bool8 is_root_statement)
{
	// -- an statement is one of:
    // -- a semicolon
    // -- a return statement, a create statement, or a destroy statement
	// -- an expression followed by a semicolon
	// -- an expression followed by a binary operator, followed by an expression

	tReadToken firsttoken(filebuf);

    // -- if this is a root statement, see if we can preserve the comment
    if (is_root_statement)
    {
        if (TryParseComment(codeblock, firsttoken, link))
        {
            filebuf = firsttoken;
            return (true);
        }
    }

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

        // -- for statements requiring multiple expressions, 
        // -- e.g. the paremeters in a function call, the true/false expressions in a ternary op
        // -- ignore the delineator, and complete the expression
        else if (nexttoken.type == TOKEN_COMMA ||
                 (nexttoken.type == TOKEN_COLON && gTernaryDepth > 0 &&
                  gTernaryStack[gTernaryDepth - 1] >= gGlobalExprParenDepth))
        {
            // -- don't consume the token - let the statement parsing handle it
            filebuf = readexpr;
            link = statementroot;

            // -- at the end of the statement, we need to sort sequences of binary op nodes
            SortTreeBinaryOps(&link);

            return (true);
        }

        else if (nexttoken.type == TOKEN_SEMICOLON)
        {
            // -- from within a 'For' loop, we have valid ';' within parenthesis
            // -- if so, do not consume the ';'
            if (gGlobalExprParenDepth > 0 || gTernaryDepth > 0)
                filebuf = readexpr;
            // -- otherwise this is a complete statement - consume the ';'
            else
                filebuf = nexttoken;
            link = statementroot;

            // -- at the end of the statement, we need to sort sequences of binary op nodes
            SortTreeBinaryOps(&link);

            return (true);
        }

        else if (nexttoken.type == TOKEN_TERNARY)
        {
            // -- we're committed to a ternary op at this point
            readexpr = nexttoken;

            // -- push the ternary parse depth
            if (gTernaryDepth >= gMaxTernaryDepth)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - Ternary operator max depth exceeded\n");
                return (false);
            }
            gTernaryStack[gTernaryDepth++] = gGlobalExprParenDepth;

            // -- create the ifstatement node, and set it as the statement root
            CCompileTreeNode* null_link = nullptr;
            CIfStatementNode* ifstmtnode = TinAlloc(ALLOC_TreeNode, CIfStatementNode, codeblock, null_link,
                                                    readexpr.linenumber);

            // -- the statement node is now the condition for the ifstatment, and the statement root is now the if
            // -- this speems specific to a single assign...  what if the tenrnary conditional was an assign statement?
            // -- if the statement root is an assignment, then the leftchild of the assignment, then the right child
            // -- is actually the conditional for the ternary op
            if (statementroot->GetType() == eBinaryOp && ((CBinaryOpNode*)statementroot)->IsAssignOpNode())
            {
                ifstmtnode->leftchild = statementroot->rightchild;
                statementroot->rightchild = ifstmtnode;
            }
            else
            {
                ifstmtnode->leftchild = statementroot;
                statementroot = ifstmtnode;
            }

            // -- create the conditional branch node
            CCondBranchNode* condbranchnode = TinAlloc(ALLOC_TreeNode, CCondBranchNode, codeblock,
                                                       ifstmtnode->rightchild, readexpr.linenumber);

            // -- read the left "true" side of the conditional branch
            bool8 result = TryParseStatement(codeblock, readexpr, condbranchnode->leftchild, false);
            if (!result || condbranchnode->leftchild == nullptr)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - Ternary operator without a 'true' expression\n");
                --gTernaryDepth;
                return (false);
            }

            // -- read the ':' 
            if (!GetToken(readexpr) || readexpr.type != TOKEN_COLON)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - Ternary operator, expecting ':'\n");
                --gTernaryDepth;
                return (false);
            }

            // -- read the right "false" side of the conditional branch
            result = TryParseStatement(codeblock, readexpr, condbranchnode->rightchild, false);
            if (!result || condbranchnode->rightchild == nullptr)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              readexpr.linenumber, "Error - Ternary operator without a 'false' expression\n");
                --gTernaryDepth;
                return (false);
            }

            // -- pop the ternary depth
            // -- note:  as long as the ternary depth is "pushed" the "ternary" statement won't consume
            // -- the semi-colon belonging to the actual statement
            --gTernaryDepth;

            // -- read the next token 
            nexttoken = readexpr;
            if (!GetToken(nexttoken))
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                    readexpr.linenumber, "Error - expecting ';'\n");
                return (false);
            }
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
                          readexpr.linenumber, "Error - invalid syntax... possibly missing ';'\n");
		    return (false);
        }
    }

    // -- should be impossible to exit the while loop - fail
    return (false);
}

// ====================================================================================================================
// TryParseUnaryPostOp():  See if there's a trailing unary post inc/dec operation, return the +1, -1, or 0 if none
// ====================================================================================================================
int32 TryParseUnaryPostOp(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode* var_root)
{
    // -- see if we're post-incrementing/decrementing the last var/member
    tReadToken peek_post_unary(filebuf);
    if (!GetToken(peek_post_unary, true))
        return (false);

    // -- see if we read a unary operator
    if (peek_post_unary.type == TOKEN_UNARY)
    {
        // -- if the unary type is either increment or decrement, create a unary node, and add it to the
        // -- list of nodes to compile upon completion of the statement
        CUnaryOpNode* unarynode = NULL;
        eUnaryOpType unarytype = GetUnaryOpType(peek_post_unary.tokenptr, peek_post_unary.length);
        if (unarytype == UNARY_UnaryPreInc || unarytype == UNARY_UnaryPreDec)
        {
            filebuf = peek_post_unary;

            // -- success
            return (unarytype == UNARY_UnaryPreInc ? 1 : -1);
        }
    }

    // -- not found - no adjustment
    return (0);
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
		unarynode = TinAlloc(ALLOC_TreeNode, CUnaryOpNode, codeblock, link, filebuf.linenumber, unarytype);

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

    // -- all math constants are float, and the method will use the const char* from the
    // constants definition, not the actual token...
    float math_constant = 0.0f;
    const char* math_constant_str = nullptr;
    if (IsMathConstant(firsttoken, math_constant_str))
    {
        // -- committed to value
        filebuf = firsttoken;

		CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, exprlink, filebuf.linenumber,
                                         math_constant_str, strlen(math_constant_str), false, TYPE_float);
        return (true);
    }

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

    // -- an ensure() completes an expression
    if (TryParseEnsure(codeblock, filebuf, exprlink))
        return (true);

    // -- a type() completes an expression
    if (TryParseType(codeblock, filebuf, exprlink))
        return (true);

    // -- a hash() completes an expression
    if (TryParseHash(codeblock, filebuf, exprlink))
        return (true);

    // -- check_interface() ensures that all the methods in the interface namespace
    /// exist in the hierarchy of the given namespace, including signature (except return type)
    if (TryParseEnsureInterface(codeblock, filebuf, exprlink))
        return (true);

    // -- a abs() completes an expression
    if (TryParseMathUnaryFunction(codeblock, filebuf, exprlink))
        return (true);

    // -- a min() completes an expression
    if (TryParseMathBinaryFunction(codeblock, filebuf, exprlink))
        return (true);

    // -- a hashtable_copy() completes an expression
    // -- as does a hashtable_wrap() - we'll use the copy with a flag for wrap
    if (TryParseHashtableCopy(codeblock, filebuf, exprlink))
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

        // -- otherwise if the keyword is "super", we treat it as a ns call, not a method call
        else if (reservedwordtype == KEYWORD_super)
        {
            if (TryParseFuncCall(codeblock, filebuf, *temp_link, EFunctionCallType::Super))
            {
                // -- committed to function call, filebuf will have already been updated
            }
        }
        else
            return (false);
    }

    // -- function call
    else if (TryParseFuncCall(codeblock, filebuf, *temp_link, EFunctionCallType::Global))
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

                // -- see if the array lookup var is being post-inc/decremented
                int32 post_op_adjust = TryParseUnaryPostOp(codeblock, filebuf, valuenode);
                if (post_op_adjust != 0)
                    arrayvarnode->SetPostUnaryOpDelta(post_op_adjust);
            }

            // -- not a hash table - create the value node
            else
            {
		        CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, *temp_link, filebuf.linenumber,
                                                 firsttoken.tokenptr, firsttoken.length, true, var->GetType());

                // -- the valuenode is added to the parse tree as expected, but if there's a following post-inc/dec operator
                // -- we need to add a "deferred" operation
                int32 post_op_adjust = TryParseUnaryPostOp(codeblock, filebuf, valuenode);
                if (post_op_adjust != 0)
                    valuenode->SetPostUnaryOpDelta(post_op_adjust);
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
            if (TryParseFuncCall(codeblock, methodcalltoken, *temp_link, EFunctionCallType::ObjMethod))
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

                    // -- the objmember is added to the parse tree as expected, but if there's a following post-inc/dec operator
                    // -- we need to add a "deferred" operation
                    int32 post_op_adjust = TryParseUnaryPostOp(codeblock, filebuf, objmember);
                    if (post_op_adjust != 0)
                        arrayvarnode->SetPostUnaryOpDelta(post_op_adjust);
                }

                // -- else not an array, just an object member
                else
                {
                    // -- create the member node
		            CObjMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CObjMemberNode, codeblock, *temp_link,
                                                         membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                    // -- the left child is the branch that resolves to an object
                    objmember->leftchild = templeftchild;

                    // -- the objmember is added to the parse tree as expected, but if there's a following post-inc/dec operator
                    // -- we need to add a "deferred" operation
                    int32 post_op_adjust = TryParseUnaryPostOp(codeblock, filebuf, objmember);
                    if (post_op_adjust != 0)
                        objmember->SetPostUnaryOpDelta(post_op_adjust);
                }
            }
        }

        // -- else if we have a colon, we're dereferencing a member of a registered POD type
        else if (nexttoken.type == TOKEN_COLON &&
                 (gTernaryDepth == 0 || gTernaryStack[gTernaryDepth - 1] < gGlobalExprParenDepth))
        {
            // -- we're committed
            filebuf = nexttoken;

            // -- cache the tree that resolves to a variable of a registered POD type
            // -- note:  this could still be a function call - e.g.  "GetPosition():x"
            CCompileTreeNode* templeftchild = *temp_link;

            // -- ensure we've got an identifier for the member/method name next
            tReadToken membertoken(filebuf);
            if (!GetToken(membertoken) || membertoken.type != TOKEN_IDENTIFIER)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              filebuf.linenumber, "Error - Expecting a POD member name\n");
                return (false);
            }

            // -- this must either be a member or a method
            // see if we can parse a "function call", which will actually be a POD method call
            if (TryParseFuncCall(codeblock, filebuf, *temp_link, EFunctionCallType::PODMethod))
            {
                // -- create a POD method node, the left child will resolve to the POD variable
                // -- and the right child will be the tree handling the method call
                CCompileTreeNode* temprightchild = *temp_link;
		        CPODMethodNode* pod_method = TinAlloc(ALLOC_TreeNode, CPODMethodNode, codeblock, *temp_link,
                                                      membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                // -- the left child is the branch that resolves to an object
                pod_method->leftchild = templeftchild;
                pod_method->rightchild = temprightchild;

                // -- and because POD members do not continue to be dereferenced, this is the end of the expression
                exprlink = expression_root;

                return (true);
            }
            else
            {
                // -- we're committed to a POD variable dereference at this point
                filebuf = membertoken;

                // -- create the member node
		        CPODMemberNode* objmember = TinAlloc(ALLOC_TreeNode, CPODMemberNode, codeblock, *temp_link,
                                                     membertoken.linenumber, membertoken.tokenptr, membertoken.length);

                // -- the left child is the branch that resolves to POD variable
                objmember->leftchild = templeftchild;

                // -- and because POD members do not continue to be dereferenced, this is the end of the expression
                exprlink = expression_root;

                // -- the objmember is added to the parse tree as expected, but if there's a following post-inc/dec operator
                // -- we need to add a "deferred" operation
                int32 post_op_adjust = TryParseUnaryPostOp(codeblock, filebuf, objmember);
                if (post_op_adjust != 0)
                    objmember->SetPostUnaryOpDelta(post_op_adjust);
            }

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
	bool8 result = TryParseStatement(codeblock, filebuf, ifstmtnode->leftchild, true);
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
		result = TryParseStatement(codeblock, filebuf, condbranchnode->leftchild, true);
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
		reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
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
			result = TryParseStatement(codeblock, filebuf, condbranchnode->rightchild, true);
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
// TryParseSwitchStatement():  A 'switch' statement is a well defined syntax, but we'll create a chain of if..else
// ====================================================================================================================
bool8 TryParseSwitchStatement(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- the first token can be anything but a reserved word or type
    tReadToken firsttoken(filebuf);
    if (!GetToken(firsttoken) || firsttoken.type != TOKEN_KEYWORD)
        return (false);

    // -- starts with the keyword 'switch'
    int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
    if (reservedwordtype != KEYWORD_switch)
        return (false);

    // -- at this point, we're committed
    filebuf = firsttoken;

    // -- read the opening parenthesis
    if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_OPEN))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      firsttoken.linenumber, "Error - expecting '('\n");
        return (false);
    }

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

    // -- a switch statement 
    // -- and the body as a statement block as its right child
    CSwitchStatementNode* switch_node = TinAlloc(ALLOC_TreeNode, CSwitchStatementNode, codeblock, link,
                                                 filebuf.linenumber);

    // -- push the switch statement onto the stack
    if (gBreakStatementDepth >= gMaxBreakStatementDepth)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - depth of %d exceeded\n", gMaxBreakStatementDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gBreakStatementStack[gBreakStatementDepth++] = switch_node;

    // we need to have a valid expression for the left hand child
    bool8 result = TryParseStatement(codeblock, filebuf, switch_node->leftchild, true);
    if (!result || switch_node->leftchild == nullptr)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - unable to parse 'switch' condition\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- consume the closing parenthesis
    if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ')'\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

    // -- read the opening brace
    if (!GetToken(filebuf) || filebuf.type != TOKEN_BRACE_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting '{'\n");
        --gBreakStatementDepth;
        return (false);
    }
    
    // -- read the case statments, linked together so we can determine jump offsets
    switch_node->rightchild = CCompileTreeNode::CreateTreeRoot(codeblock);
    CCompileTreeNode* case_statements = switch_node->rightchild;

    // -- read the case statements
    while (true)
    {
        // -- read each case
        tReadToken peek_token(filebuf);
        if (!GetToken(peek_token))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                          "Error - expecting '}'\n");
            --gBreakStatementDepth;
            return (false);
        }

        // -- if we have a keyword, it must be either 'case' or 'default'
        if (peek_token.type == TOKEN_KEYWORD)
        {
            // -- starts with the keyword 'switch'
            reservedwordtype = GetReservedKeywordType(peek_token.tokenptr, peek_token.length);
            if (reservedwordtype == KEYWORD_case || reservedwordtype == KEYWORD_default)
            {
                // -- update the filebuf
                filebuf = peek_token;

                // -- create the case statement node
                CCaseStatementNode* case_statement = TinAlloc(ALLOC_TreeNode, CCaseStatementNode, codeblock,
                                                              AppendToRoot(*case_statements), filebuf.linenumber);

                // -- if a case statement, we have a value expression before the colon
                if (reservedwordtype == KEYWORD_case)
                {
                    peek_token = filebuf;
                    if (!GetToken(peek_token) || peek_token.type != TOKEN_PAREN_OPEN)
                    {
                        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                                      "Error - expecting '('.  'case' expression format is: case ( ... ):\n");
                        --gBreakStatementDepth;
                        return (false);
                    }

                    // -- read the value expression
                    // $$$TZA TinScript doesn't yet enforce a constant expression -
                    // -- perhaps validate a single CValue non-var node?
                    bool8 result_0 = TryParseExpression(codeblock, filebuf, case_statement->leftchild);
                    if (!result_0 || case_statement->leftchild == nullptr)
                    {
                        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                                      "Error - expecting 'case' expression\n");
                        --gBreakStatementDepth;
                        return (false);
                    }
                }
                else
                {
                    // -- set default...  make sure it's the only one
                    case_statement->SetDefaultCase();
                    if (!switch_node->SetDefaultNode(case_statement))
                    {
                        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                            "Error - 'default' case already defined\n");
                        --gBreakStatementDepth;
                        return (false);
                    }
                }

                // -- the statements are also a list of statements, as we can have multiple per case
                case_statement->rightchild = CCompileTreeNode::CreateTreeRoot(codeblock);
                CCompileTreeNode* case_content = case_statement->rightchild;

                // -- read the colon
                if (!GetToken(filebuf) || filebuf.type != TOKEN_COLON)
                {
                    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                                  "Error - expecting ':'.  'case' expression format is: case ( ... ): \n");
                    --gBreakStatementDepth;
                    return (false);
                }

                // -- read statement blocks, and individual statments, until we find the next case,
                // -- or the final closing brace
                while (true)
                {
                    // -- we *may* have an opening brace
                    bool8 handled = false;
                    peek_token = filebuf;
                    if (!GetToken(peek_token))
                    {
                        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                                      "Error - expecting ':'\n");
                        --gBreakStatementDepth;
                        return (false);
                    }

                    // -- possible opening brace
                    if (peek_token.type == TOKEN_BRACE_OPEN)
                    {
                        filebuf = peek_token;

                        // -- read the statement block (includes consuming the closing brace)
                        result = ParseStatementBlock(codeblock, AppendToRoot(*case_content), filebuf, true);
                        if (!result)
                        {
                            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                                filebuf.linenumber, "Error - unable to parse the 'case' statmentblock\n");
                            --gBreakStatementDepth;
                            return (false);
                        }

                        // -- set the bool
                        handled = true;
                    }

                    // -- else a new case is about to be defined, concluding our current case
                    if (!handled && peek_token.type == TOKEN_KEYWORD)
                    {
                        reservedwordtype = GetReservedKeywordType(peek_token.tokenptr, peek_token.length);
                        if (reservedwordtype == KEYWORD_case || reservedwordtype == KEYWORD_default)
                        {
                            break;
                        }
                    }

                    // -- the end of the entire switch
                    if (!handled && peek_token.type == TOKEN_BRACE_CLOSE)
                    {
                        break;
                    }

                    // -- else, read the next statement for the case
                    if (!handled)
                    {
                        result = TryParseStatement(codeblock, filebuf, AppendToRoot(*case_content));
                        if (!result)
                        {
                            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                                          filebuf.linenumber, "Error - expecting a '}'\n");
                            --gBreakStatementDepth;
                            return (false);
                        }
                    }
                }
            }

            // -- invalid keyword
            else
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                              "Error - expecting '}'\n");
                --gBreakStatementDepth;
                return (false);
            }
        }

        // -- else it had better be the closing brace
        else if (peek_token.type != TOKEN_BRACE_CLOSE)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                          "Error - expecting '}'\n");
            --gBreakStatementDepth;
            return (false);
        }

        // -- we've reached the end of our cases
        else
        {
            break;
        }
    }

    // -- read the closing brace
    if (!GetToken(filebuf) || filebuf.type != TOKEN_BRACE_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting '{'\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- success
    --gBreakStatementDepth;
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

	// -- starts with the keyword 'while'
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
                                             filebuf.linenumber, false);

    // -- push the while loop onto the stack
    if (gBreakStatementDepth >= gMaxBreakStatementDepth)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' depth of %d exceeded\n", gMaxBreakStatementDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gBreakStatementStack[gBreakStatementDepth++] = whileloopnode;

	// we need to have a valid expression for the left hand child
	bool8 result = TryParseStatement(codeblock, filebuf, whileloopnode->leftchild, true);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' without a conditional expression\n");
        --gBreakStatementDepth;
		return (false);
	}

    // -- consume the closing parenthesis
    if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ')'\n");
        --gBreakStatementDepth;
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
        --gBreakStatementDepth;
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
            --gBreakStatementDepth;
			return (false);
		}

        // -- success - pop the while node off the stack
        --gBreakStatementDepth;
		return (true);
	}

	// -- else it's a single expression
	else
    {
		result = TryParseStatement(codeblock, filebuf, whileloopnode->rightchild, true);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - unable to parse the while loop body\n");
            --gBreakStatementDepth;
			return (false);
		}

        // -- success - pop the while node off the stack
        --gBreakStatementDepth;
		return (true);
	}
}

// ====================================================================================================================
// TryParseDoWhileLoop():  A do..while loop has a well defined syntax.
// ====================================================================================================================
bool8 TryParseDoWhileLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- the first token can be anything but a reserved word or type
    tReadToken firsttoken(filebuf);
    if (!GetToken(firsttoken))
        return (false);

    // -- starts with the keyword 'do'
    if (firsttoken.type != TOKEN_KEYWORD)
        return (false);

    int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
    if (reservedwordtype != KEYWORD_do)
        return (false);

    // -- we're committed to a 'do..while' loop now
    filebuf = firsttoken;

    // -- see if we've got a statement block, or a single statement
    if (!GetToken(filebuf) || filebuf.type != TOKEN_BRACE_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - 'do..while loop' expecting '{'\n");
        return (false);
    }

    // -- a while loop has the expression tree as it's left child,
    // -- and the body as a statement block as its right child
    CWhileLoopNode* whileloopnode = TinAlloc(ALLOC_TreeNode, CWhileLoopNode, codeblock, link,
                                             filebuf.linenumber, true);

    // -- push the while loop onto the stack
    if (gBreakStatementDepth >= gMaxBreakStatementDepth)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - 'while loop' depth of %d exceeded\n", gMaxBreakStatementDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gBreakStatementStack[gBreakStatementDepth++] = whileloopnode;

    // -- read the while loop body
    bool8 result = ParseStatementBlock(codeblock, whileloopnode->rightchild, filebuf, true);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the do..while statmentblock\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- success - pop the while node off the stack
    --gBreakStatementDepth;

    // -- after the statement block, we need to read the while keyword, and the conditional
    if (!GetToken(filebuf) || filebuf.type != TOKEN_KEYWORD)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting keyword 'while'\n");
        return (false);
    }

    // -- ensure the keyword was 'while'
    reservedwordtype = GetReservedKeywordType(filebuf.tokenptr, filebuf.length);
    if (reservedwordtype != KEYWORD_while)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting keyword 'while'\n");
        return (false);
    }

    // -- next token better be an open parenthesis
    if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_OPEN))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting '('\n");
        return (false);
    }

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

    // we need to have a valid expression for the left hand child
    result = TryParseStatement(codeblock, filebuf, whileloopnode->leftchild, true);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - 'while loop' without a conditional expression\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- consume the closing parenthesis
    if (!GetToken(filebuf) || filebuf.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ')'\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

    // -- consume the statement terminator
    if (!GetToken(filebuf) || filebuf.type != TOKEN_SEMICOLON)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - expecting ')'\n");
        --gBreakStatementDepth;
        return (false);
    }

    // -- success
    return (true);
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
                                             AppendToRoot(*forlooproot), filebuf.linenumber, false);

    // -- push the while loop onto the stack
    if (gBreakStatementDepth >= gMaxBreakStatementDepth)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' depth of %d exceeded\n", gMaxBreakStatementDepth);
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gBreakStatementStack[gBreakStatementDepth++] = whileloopnode;

	// -- the for loop condition is the left child of the while loop node
	result = TryParseStatement(codeblock, filebuf, whileloopnode->leftchild);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the conditional expression\n");
        --gBreakStatementDepth;
		return (false);
	}

   	// -- consume the separating semicolon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_SEMICOLON))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ';'\n");
        --gBreakStatementDepth;
		return (false);
	}

	// -- the end of loop expression is next, but we're going to hold on to it for a moment
	CCompileTreeNode* tempendofloop = NULL;
	result = TryParseStatement(codeblock, filebuf, tempendofloop, true);
	if (!result)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the end of loop expression\n");
        --gBreakStatementDepth;
		return (false);
	}

   	// -- consume the closing parenthesis semicolon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_CLOSE))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ')'\n");
        --gBreakStatementDepth;
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
        --gBreakStatementDepth;
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
            --gBreakStatementDepth;
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
            --gBreakStatementDepth;
			return (false);
		}
	}

	// notify the while node of the end of the loop statements
    whileloopnode->SetEndOfLoopNode(tempendofloop);

    // -- success - pop the while node off the stack
    --gBreakStatementDepth;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseForeachLoop():  A 'foreach' loopwill have to operate on hashtables, arrays, and CObjectSets
// ====================================================================================================================
bool8 TryParseForeachLoop(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- the first token can be anything but a reserved word or type
	tReadToken firsttoken(filebuf);
	if (!GetToken(firsttoken))
		return (false);

	// -- starts with the keyword 'foreach'
	if (firsttoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(firsttoken.tokenptr, firsttoken.length);
	if (reservedwordtype != KEYWORD_foreach)
		return (false);

    // -- we're committed to a 'foreach' loop now
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

    // a foreach is a while loop structured as:
    // -- the left child resolves the container to iterate
    // -- the right node is the while loop
    // -- whileloop left child is normally the condition but in this case,
    // the condition is pushed by the endOfLoop which is a single OP foreachIterNext instruction
    // -- whileloop right child is the loop body
    // -- whileloop endOfLoopNode increments the iterator
    // note:  (eventually) a container will be one of:  hashtable, array, CObjectSet

    // -- the next token is the variable identifier
    tReadToken iter_var_name(filebuf);
    if (!GetToken(iter_var_name))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - invalid foreach loop iterator var identifier\n");
        --gBreakStatementDepth;
        return (false);
    }

    filebuf = iter_var_name;

  	// -- consume the separating colon
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_COLON))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ':'\n");
        --gBreakStatementDepth;
        return (false);
	}

    // -- we start with a Foreach loop node, the left branch resolves the expression to push a container
    // -- the right branch initializes and pushes the iterator variable
    CForeachLoopNode* foreach_node = TinAlloc(ALLOC_TreeNode, CForeachLoopNode, codeblock, link, filebuf.linenumber,
                                              iter_var_name.tokenptr, iter_var_name.length);

    // -- the second parameter in the foreach is a statement resolving to a container
    bool result = TryParseStatement(codeblock, filebuf, foreach_node->leftchild, false);
    if (!result)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the foreach 'container' expression\n");
                      --gBreakStatementDepth;
        return (false);
    }

	// add the foreach loop node (implemented as a while loop), as the right child of our foreach loop node
    int32 foreach_linenumber = filebuf.linenumber;
	CWhileLoopNode* foreach_while_loop = TinAlloc(ALLOC_TreeNode, CWhileLoopNode, codeblock,
                                                  foreach_node->rightchild, foreach_linenumber, false);

    // -- push the while loop onto the stack
    if (gBreakStatementDepth >= gMaxBreakStatementDepth)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - 'while loop' depth of %d exceeded\n", gMaxBreakStatementDepth);
                      --gBreakStatementDepth;
        return (false);
    }

    // -- push the while node onto the stack (used so break and continue know which loop they're affecting)
    gBreakStatementStack[gBreakStatementDepth++] = foreach_while_loop;

   	// -- consume the closing parenthesis
	if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_CLOSE))
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ')'\n");
        --gBreakStatementDepth;
		return (false);
	}

    // -- decrement the parenthesis stack
    --gGlobalExprParenDepth;

	// -- the left child of a while loop is normally a condition
    // -- the foreachIterNext operation will push the "true/false", so here we need an empty node
    foreach_while_loop->leftchild = CCompileTreeNode::CreateTreeRoot(codeblock);

	// -- see if it's a single statement, or a statement block
	peektoken = filebuf;
	if (!GetToken(peektoken))
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber,
                      "Error - unable to parse the foreach loop body\n");
        --gBreakStatementDepth;
		return (false);
	}

	if (peektoken.type == TOKEN_BRACE_OPEN)
    {
		// -- consume the brace, and parse an entire statement block
		filebuf = peektoken;
        result = ParseStatementBlock(codeblock, foreach_while_loop->rightchild, filebuf, true);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - failed to read statement block\n");
            --gBreakStatementDepth;
			return (false);
		}
	}

	// else try a single expression
	else
	{
		result = TryParseStatement(codeblock, filebuf, foreach_while_loop->rightchild);
		if (!result)
        {
			ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - failed to read statement block\n");
            --gBreakStatementDepth;
			return (false);
		}
	}

	// notify the while node of the end of the loop statements
    // -- set up the while node end of loop to be a CForeachIterNext node
    CCompileTreeNode* tempendofloop = NULL;
    CForeachIterNext* foreach_iter_next = TinAlloc(ALLOC_TreeNode, CForeachIterNext, codeblock, tempendofloop,
                                                   foreach_linenumber);
    foreach_while_loop->SetEndOfLoopNode(foreach_iter_next);

    // -- success - pop the while node off the stack
    --gBreakStatementDepth;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseFuncDefinition():  A function has a well defined syntax.
// ====================================================================================================================
bool8 TryParseFuncDefinition(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- use temporary vars, to ensure we don't change the actual filebuf, unless successful
	tReadToken returntype(filebuf);
	if (!GetToken(returntype))
		return (false);

    bool is_interface = false;
    if (returntype.type == TOKEN_KEYWORD &&
        GetReservedKeywordType(returntype.tokenptr, returntype.length) == KEYWORD_interface)
    {
        is_interface = true;
    }

	// -- see if we found a registered type
	if (!is_interface && returntype.type != TOKEN_REGTYPE)
		return (false);

	eVarType regreturntype = !is_interface ? GetRegisteredType(returntype.tokenptr, returntype.length)
                                           : TYPE__resolve;

	// -- see if the next token is an identifier
	tReadToken idtoken = returntype;
	if (!GetToken(idtoken))
		return (false);

	if (idtoken.type != TOKEN_IDENTIFIER)
		return (false);

    // -- see if this is a namespace'd function declaration
    bool8 usenamespace = false;
    tReadToken nsnametoken(idtoken);
    tReadToken nstoken(idtoken);
    CNamespace* func_namespace = nullptr;
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
        func_namespace = codeblock->GetScriptContext()->FindOrCreateNamespace(TokenPrint(nsnametoken));
        if (!func_namespace)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken.linenumber,
                          "Error - Failed to find/create Namespace: %s\n",
                          TokenPrint(nsnametoken));
            return (false);
        }

        // -- if this is an interface function definition, ensure we're permitted to
        // set the namespace as an interface namespace
        if (is_interface && !func_namespace->IsInterface())
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                            peektoken.linenumber,
                            "Error - Unable to add an interface method to a non-interface namespace %s::\nAll methods in a namespace must either be interface declarations, or implemented methods\n",
                            TokenPrint(nsnametoken));
            return (false);
        }

        functable = func_namespace->GetFuncTable();
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
	CFunctionEntry* curfunction = functable->FindItem(funchash);

    // -- if we're replacing the function definition, delete the old
    if (curfunction != nullptr)
    {
        // -- if it was just defined, we don't want duplicate implementations - or it's impossible
        // to tell which is the latest/correct implementation
        if (codeblock->GetScriptContext()->IsDefiningFunction(funchash, nshash))
        {
            if (nshash != 0)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                    filebuf.linenumber, "Error - trying to define multiple implementations of %s::%s()\n",
                    TokenPrint(nsnametoken), TokenPrint(idtoken));
            }
            else
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                    filebuf.linenumber, "Error - trying to define multiple implementations of %s()\n",
                    TokenPrint(idtoken));
            }

            return false;
        }

        // -- otherwise, we're free to replace the existing function definition
        functable->RemoveItem(funchash);
        TinFree(curfunction)
        curfunction = nullptr;
    }

    // -- if this is not an interface - ensure we don't try to add any non-interface methods to an interface
    if (!is_interface && usenamespace && func_namespace != nullptr && functable != nullptr)
    {
        if (functable->Used() > 0 && func_namespace->IsInterface())
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - non-interface method ::%s() being added to an interface %s::\n",
                          TokenPrint(idtoken), TokenPrint(nsnametoken));
            return false;
        }
    }

    // -- begin the definition for the new definition
    // note:  we are no longer (but could?) warn if the signature has changed
    curfunction = FuncDeclaration(codeblock->GetScriptContext(), nshash, TokenPrint(idtoken),
                                                  Hash(TokenPrint(idtoken)), eFuncTypeScript);

    // -- notify the context of the new function definition
    codeblock->GetScriptContext()->NotifyFunctionDefinition(curfunction);

    // -- push the function onto the definition stack
    codeblock->smFuncDefinitionStack->Push(curfunction, NULL, 0);

    // get the function context
    CFunctionContext* funccontext = curfunction->GetContext();

    // -- first parameter is always the return type
    int32 paramcount = 0;
    funccontext->AddParameter("__return", Hash("__return"), regreturntype, 1, 0);
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

        // -- increment the parameter count
        ++paramcount;

        // -- see if we've got a comma
        tReadToken peektoken_0(filebuf);
        if (!GetToken(peektoken_0) || (peektoken_0.type != TOKEN_COMMA && peektoken_0.type != TOKEN_PAREN_CLOSE))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken_0.type == TOKEN_COMMA)
        {
            // -- if we do have a comma, ensure the token after it is the next param type
            tReadToken peektoken2(peektoken_0);
            if (!GetToken(peektoken2) || peektoken2.type != TOKEN_REGTYPE)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              peektoken_0.linenumber, "Error - expecting ')'\n");
                return (false);
            }

            // -- consume the comma
            filebuf = peektoken_0;
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
    // -- syntax is:  void ChildNamespace::OnCreate() : ParentNamespace { ... }
    uint32 derived_hash = 0;
    static uint32 oncreate_hash = Hash("OnCreate");
    if (funchash == oncreate_hash && !usenamespace)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                     filebuf.linenumber, "Error - OnCreate() must be defined for a namespace, not as a global function\n");
        return (false);
    }

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

            // -- committed
            peektoken = parenttoken;
            if (!GetToken(peektoken))
            {
                if (is_interface)
                {
                    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                                  filebuf.linenumber, "Error - interface OnCreate() declaration:  expecting ';'\n");
                }
                else
                {
                    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                                  filebuf.linenumber, "Error - method OnCreate() definition:  expecting '{'\n");
                }
                return (false);
            }

            // -- set the derived namespace, which will become part of the function declaration node
            derived_hash = Hash(parenttoken.tokenptr, parenttoken.length);

            // -- if this is an interface, it can only be derived from another interface...
            // -- normally we link namespaces when we actually create an object, but we never create an
            // instance of an interface, so we need to link them now
            // -- this means of course, base interfaces must be declared before derived interfaces, a restriction
            // not imposed on non-interfaces
            if (is_interface)
            {
                const char* parent_interface_name = UnHash(derived_hash);
                CNamespace* parent_interface =
                    codeblock->GetScriptContext()->FindOrCreateNamespace(parent_interface_name);
                if (!parent_interface || !parent_interface->IsInterface())
                {
                    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                                  filebuf.linenumber, "Error - invalid parent interface ::%s for \n", parent_interface_name,
                                  UnHash(nshash));
                    return false;
                }

                // -- we link the namespaces here...
                // -- when ensure_interface() from a non-interface to an interface is called,
                // the entire interface hierarchy will be validated
                codeblock->GetScriptContext()->LinkNamespaces(codeblock->GetScriptContext()->FindNamespace(nshash),
                                                              parent_interface);
            }
        }
    }

    // -- for interfaces, ensure this is just a signature
    if (is_interface)
    {
        if (peektoken.type != TOKEN_SEMICOLON)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                           filebuf.linenumber, "Error - interface method(), only a signature is permitted, expecting ';'.\n");
            return (false);
        }

        // -- update the file buf
        filebuf = peektoken;

        // -- add a funcdecl node, and set its left child to be the statement block
        CFuncDeclNode* funcdeclnode = TinAlloc(ALLOC_TreeNode, CFuncDeclNode, codeblock, link,
                                               filebuf.linenumber, idtoken.tokenptr, idtoken.length,
                                               nsnametoken.tokenptr, nsnametoken.length, derived_hash);

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
                      filebuf.linenumber, "Error - non-interface function requires a statement block, expecting '{'\n");
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

    // $$$TZA ideally, we'd like to validate every path to see if we already have a return
    // if one is missing, we'll fall through to the nullreturn, and catch the invalid return at runtime
    if (!FindChildNode(*funcdeclnode->leftchild, ECompileNodeType::eFuncReturn))
    {
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
    }

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
                       EFunctionCallType call_type)
{
	// -- see if the next token is an identifier
	tReadToken idtoken(filebuf);
	if (!GetToken(idtoken))
		return (false);

    // -- "super" is a special kind of namespace identifier in this case
    if (idtoken.type == TOKEN_KEYWORD &&
        GetReservedKeywordType(idtoken.tokenptr, idtoken.length) == KEYWORD_super)
    {
        if (call_type != EFunctionCallType::Super && call_type != EFunctionCallType::None)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          idtoken.linenumber,
                          "Error - trying to call super::x() as a method\n");
        }
        call_type = EFunctionCallType::Super;
    }

	if (call_type != EFunctionCallType::Super && idtoken.type != TOKEN_IDENTIFIER)
		return (false);

    // -- see if this is a namespace'd function declaration
    bool8 usenamespace = false;
    tReadToken nsnametoken(idtoken);
    tReadToken nstoken(idtoken);
    if (GetToken(nstoken) && nstoken.type == TOKEN_NAMESPACE)
    {
        usenamespace = true;

        // -- if this is a "super::" method, verify that the nsnametoken is "super"
        if (call_type == EFunctionCallType::Super)
        {
            if (strncmp(nsnametoken.tokenptr, "super", 5) != 0)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              nsnametoken.linenumber,
                              "Error - namespace should be 'super' %s::\n",
                              TokenPrint(nsnametoken));
                return (false);
            }

            // -- so now that we're using a super::method(), we want the actual nsnametoken to be
            // the function we're currently defining
            int32 cur_method_def_obj_stack_top = 0;
            CObjectEntry* cur_method_def_obj = NULL;
            CFunctionEntry* cur_method_def =
                codeblock->smFuncDefinitionStack->GetTop(cur_method_def_obj, cur_method_def_obj_stack_top);
            uint32 cur_method_ns_hash = cur_method_def != nullptr ? cur_method_def->GetNamespaceHash() : 0;
            if (cur_method_def == nullptr || cur_method_ns_hash == 0)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                              nsnametoken.linenumber,
                              "Error - cannot call super::method() outside outside of a <namespace>::method() definition\n");
                return (false);
            }

            // $$$TZA critical we don't try to write to the nsnametoken
            const char* cur_method_ns = UnHash(cur_method_ns_hash);
            int32 cur_method_ns_length = (int32)strlen(cur_method_ns);
            nsnametoken.tokenptr = cur_method_ns;
            nsnametoken.length = cur_method_ns_length;
        }

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
                                           call_type);

    // -- create a tree root to contain all the parameter assignments
  	funccallnode->leftchild = CCompileTreeNode::CreateTreeRoot(codeblock);
	CCompileTreeNode* assignments = funccallnode->leftchild;

    // -- keep reading and assigning params, until we reach the closing parenthesis
    // note:  for function and method calls, we assign parameters starting at 1... (0 is the return param)
    // but for POD method calls, we force the first parameter to be the POD value itself, so
    // args passed to the method begin at parameter 2
    int32 first_param_index = call_type == EFunctionCallType::PODMethod ? 2 : 1;
    int32 paramindex = first_param_index;
    while (true)
    {
        // -- see if we have a closing parenthesis
        tReadToken peektoken_0(filebuf);
        if (!GetToken(peektoken_0))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken_0.linenumber, "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken_0.type == TOKEN_PAREN_CLOSE)
        {
            // -- we've found all the parameters we're going to find
            filebuf = peektoken_0;
            break;
        }

        // -- if we didn't find a closing parenthesis, and this isn't the first parameter, then
        // -- we'd better find the separating comma
        if (paramindex > first_param_index)
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

        // -- create an assignment binary op
		CBinaryOpNode* binopnode = TinAlloc(ALLOC_TreeNode, CBinaryOpNode, codeblock,
                                            AppendToRoot(*assignments), filebuf.linenumber,
                                            ASSOP_Assign, true, TYPE__resolve);

   		// -- create the (parameter) value node, add it to the assignment node
		CValueNode* valuenode = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                         binopnode->leftchild, filebuf.linenumber, paramindex,
                                         TYPE__var);
        Unused_(valuenode);

        bool8 result = TryParseStatement(codeblock, filebuf, binopnode->rightchild, true);
        if (!result)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          filebuf.linenumber,
                          "Error - Unable to evaluate parameter %d in call to %s()\n", paramindex,
                          TokenPrint(idtoken));
            --gGlobalExprParenDepth;
            return (false);
        }

        // -- increment the paramindex
        ++paramindex;
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
    if (gBreakStatementDepth < 1)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - trying parse continue / break, outside of a loop\n");
        return (false);
    }

    // -- ensure we don't have a 'continue' within a 'switch' statement
    if (reservedwordtype == KEYWORD_continue && gBreakStatementStack[gBreakStatementDepth - 1]->GetType() == eSwitchStmt)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - 'continue' is not valid within a 'switch' statement\n");
        return (false);
    }

    // -- committed
    filebuf = peektoken;

    // -- add a return node to the tree, and parse the return expression
    CLoopJumpNode* loopJumpNode = TinAlloc(ALLOC_TreeNode, CLoopJumpNode, codeblock, link, filebuf.linenumber,
                                           gBreakStatementStack[gBreakStatementDepth - 1], reservedwordtype == KEYWORD_break);

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

    // -- disallow return statements while in the middle of parenthetical expressions
    // -- (at least until I can think of a valid example)
    if (gGlobalExprParenDepth > 0)
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

        // -- can't return from a function, if there's no active function being defined
    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
    if (curfunction == nullptr)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                     filebuf.linenumber,
                     "Error - 'return' statement not within a function definition\n");
        gGlobalReturnStatement = false;
        return (false);
    }

    // -- see if the return type is void
    CFunctionContext* fe_context = curfunction->GetContext();
    CVariableEntry* return_ve = fe_context != nullptr ? fe_context->GetParameter(0) : nullptr;
    eVarType return_type = return_ve != nullptr ? return_ve->GetType() : TYPE_void;

    // -- add a return node to the tree, and parse the return expression
    CFuncReturnNode* returnnode = TinAlloc(ALLOC_TreeNode, CFuncReturnNode, codeblock, link,
                                           filebuf.linenumber);

    // -- if the return type is void, then this must be a semi-colon completed statement as is
    if (return_type == TYPE_void)
    {
        bool valid_return = false;
        if (!GetToken(peektoken) || peektoken.type != TOKEN_SEMICOLON)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                filebuf.linenumber,
                "Error - void function return type, expecting a ';'\n");
        }
        else
        {
            filebuf = peektoken;
            valid_return = true;

            // -- we still need to push a return value on the stack... even for void
            CValueNode* nullreturn = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                              returnnode->leftchild, filebuf.linenumber, "", 0, false,
                                              TYPE_int);
            Unused_(nullreturn);
        }

        // -- reset the re-entrant guard, and return the result
        gGlobalReturnStatement = false;
        return (valid_return);

    }
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
    snprintf(hash_value_buf, sizeof(hash_value_buf), "%d", hash_value);
    CValueNode* hash_node = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock, link, filebuf.linenumber, hash_value_buf,
                                     (int32)strlen(hash_value_buf), false, TYPE_int);

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseInclude():  The keyword "include" will force execution of the included script immediately
// ====================================================================================================================
bool8 TryParseInclude(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- ensure the next token is the 'hash' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

    int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_include)
        return (false);

    // -- we're committed to an include statement
    filebuf = peektoken;

    // -- the complete format is: hash("string")
    // -- read an open parenthesis
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - include() statement, expecting '('\n");
        return (false);
    }

    // -- next, we read a non-empty string
    tReadToken string_token(peektoken);
    if (!GetToken(string_token) || string_token.type != TOKEN_STRING || string_token.length == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - include() statement, expecting a non-empty string literal filename\n");
        return (false);
    }

    // -- read the closing parenthesis
    peektoken = string_token;
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - include() statement, expecting ')'\n");
        return (false);
    }

    // -- read the statement semicolon
    if (!GetToken(peektoken) || peektoken.type != TOKEN_SEMICOLON)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - include() statement, expecting ';'\n");
        return (false);
    }

    // -- update the file buf
    filebuf = peektoken;

    // -- ensure the include statement is executed at the global scope only
    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
    if (curfunction != nullptr)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - include() can only be executed at the global scope, not within a function.')'\n");
        return (false);
    }

    // -- execute the script immediately
    char filename[kMaxNameLength];
    SafeStrcpy(filename, kMaxNameLength, string_token.tokenptr, string_token.length + 1);
    codeblock->GetScriptContext()->ExecScript(filename, true, false);

    // -- we also generate an include node, as when the script doesn't need re-compiling (e.g. parsing)
    // we still need it to executing the included script immediately
    uint32 filename_hash = Hash(string_token.tokenptr, string_token.length, true);
    TinAlloc(ALLOC_TreeNode, CIncludeScriptNode, codeblock, link, filebuf.linenumber, filename_hash);

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseEnsureInterface():  The keyword usage is "ensure_interface(ns_hash, interface_hash)":
// and does not allow expressions
// ====================================================================================================================
bool8 TryParseEnsureInterface(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- ensure the next token is the 'hash' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_ensure_interface)
        return (false);

    // -- we're committed to a hash expression
    filebuf = peektoken;

    // -- the complete format is: hash("string")
    // -- read an open parenthesis
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - ensure_interface(): expecting '('\n");
        return (false);
    }

    // -- next, we read a non-empty namespace string
    tReadToken namespace_token(peektoken);
    if (!GetToken(namespace_token) || namespace_token.type != TOKEN_STRING || namespace_token.length == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - ensure_interface(): expecting a non-empty namespace string literal\n");
        return (false);
    }

    // -- update the file buf
    filebuf = namespace_token;

    // -- consume the comma
    if (!GetToken(filebuf) || filebuf.type != TOKEN_COMMA)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
            filebuf.linenumber, "Error - ensure_interface(): expecting ','\n");
        return (false);
    }

    // -- next, we read a non-empty namespace string
    tReadToken interface_token(filebuf);
    if (!GetToken(interface_token) || interface_token.type != TOKEN_STRING || interface_token.length == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
            "Error - ensure_interface(): expecting a non-empty iterface string literal\n");
        return (false);
    }

    // -- update the file buf
    filebuf = interface_token;

    // -- read the closing parenthesis
    peektoken = interface_token;
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hash() expression, expecting ')'\n");
        return (false);
    }

    // -- update the file buf
    filebuf = peektoken;

    // -- ensure_interface expressions resolve at *compile* time, directly into values.
    // -- because these are literals, add the string to the dictionary, as it may help debugging
    uint32 ns_hash_value = Hash(namespace_token.tokenptr, namespace_token.length, true);
    uint32 interface_hash_value = Hash(interface_token.tokenptr, interface_token.length, true);
    CEnsureInterfaceNode* interface_node = TinAlloc(ALLOC_TreeNode, CEnsureInterfaceNode, codeblock, link, filebuf.linenumber,
                                                    ns_hash_value, interface_hash_value);

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseHashtableCopy():  copies the contents of a hashtable to another (including a C++ friendly CHashtable)
// ====================================================================================================================
bool8 TryParseHashtableCopy(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- ensure the next token is the 'hash' keyword
	tReadToken peektoken(filebuf);
	if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
	if (reservedwordtype != KEYWORD_hashtable_copy && reservedwordtype != KEYWORD_hashtable_wrap)
		return (false);

    bool is_wrap = reservedwordtype == KEYWORD_hashtable_wrap;

	// -- read the opening parenthesis
	if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
		return (false);

	// -- we're committed to an array count expression
	filebuf = peektoken;

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

	// -- create the ArrayVarNode, leftchild is the array var
	CHashtableCopyNode* ht_copy_node = TinAlloc(ALLOC_TreeNode, CHashtableCopyNode, codeblock, link,
											    filebuf.linenumber, is_wrap);

	// -- ensure we have an expression to fill the left child
	bool8 result = TryParseExpression(codeblock, filebuf, ht_copy_node->leftchild);
	if (!result || !ht_copy_node->leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - hashtable_copy() requires a hashtable variable expression\n");
		return (false);
	}

    // -- the next token must be a comma
    peektoken = filebuf;
    if (!GetToken(peektoken) || peektoken.type != TOKEN_COMMA)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hashtable_copy() requires 2nd parameter hashtable or CHashtable object\n");
        return (false);
    }

    // -- update the file buf
    filebuf = peektoken;

    // -- ensure we have an expression to fill the right child
    result = TryParseExpression(codeblock, filebuf, ht_copy_node->rightchild);
    if (!result || !ht_copy_node->rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - hashtable_copy() requires a hashtable or CHashtable object expression\n");
        return (false);
    }

	// -- read the closing parenthesis
	peektoken = filebuf;
	if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - hashtable_copy() expression, expecting ')'\n");
		return (false);
	}

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

    // -- update the file buf
	filebuf = peektoken;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseType():  Pushes the string name for the type of the given variable/value
// ====================================================================================================================
bool8 TryParseType(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- ensure the next token is the 'type' keyword
	tReadToken peektoken(filebuf);
	if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
		return (false);

	int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
	if (reservedwordtype != KEYWORD_type)
		return (false);

	// -- read the opening parenthesis
	if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
		return (false);

	// -- we're committed to a hashtable_keys expression
	filebuf = peektoken;

	// -- create the CTypeNode, leftchild is the string[] to copy the keys to,
	CTypeNode* type_node = TinAlloc(ALLOC_TreeNode, CTypeNode, codeblock, link, filebuf.linenumber);

	// -- ensure we have an expression to fill the left child
	bool8 result = TryParseExpression(codeblock, filebuf, type_node->leftchild);
	if (!result || !type_node->leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - type() requires variable expression.\n");
		return (false);
	}

	// -- read the closing parenthesis
	peektoken = filebuf;
	if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - type() expression, expecting ')' following array variable\n");
		return (false);
	}

	// -- update the file buf
	filebuf = peektoken;

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseEnsure():  Evaluates the condition and pushes the result... if false, triggers an assert with the msg
// ====================================================================================================================
bool8 TryParseEnsure(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
    // -- ensure the next token is the 'ensure' keyword
    tReadToken peektoken(filebuf);
    if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
        return (false);

    int32 reservedwordtype = GetReservedKeywordType(peektoken.tokenptr, peektoken.length);
    if (reservedwordtype != KEYWORD_ensure)
        return (false);

    // -- read the opening parenthesis
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_OPEN)
        return (false);

    // -- we're committed to a hashtable_keys expression
    filebuf = peektoken;

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

    // -- create the CTypeNode, leftchild is the string[] to copy the keys to,
    CEnsureNode* ensure_node = TinAlloc(ALLOC_TreeNode, CEnsureNode, codeblock, link, filebuf.linenumber);

    // -- ensure we have an expression to fill the left child
    bool8 result = TryParseStatement(codeblock, filebuf, ensure_node->leftchild);
    if (!result || !ensure_node->leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - ensure() requires boolean expression.\n");
        return (false);
    }

    // -- consume the comma
    if (!GetToken(filebuf) || filebuf.type != TOKEN_COMMA)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ','\n");
        return (false);
    }

    // -- ensure we have a string message to fill the right child
    result = TryParseExpression(codeblock, filebuf, ensure_node->rightchild);
    if (!result || !ensure_node->rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - ensure() requires an error message.\n");
        return (false);
    }

    // -- read the closing parenthesis
    peektoken = filebuf;
    if (!GetToken(peektoken) || peektoken.type != TOKEN_PAREN_CLOSE)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                      "Error - ensure() expression, expecting ')'\n");
        return (false);
    }

    // -- decrement the paren depth
    --gGlobalExprParenDepth;

    // -- update the file buf
    filebuf = peektoken;

    // -- success
    return (true);
}

// ====================================================================================================================
// TryParseMathUnaryFUnction():  The keyword "abs" is a unary function.
// ====================================================================================================================
bool8 TryParseMathUnaryFunction(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- ensure the next token is the 'hash' keyword
	tReadToken peektoken(filebuf);
	if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
		return (false);

    eMathUnaryFunctionType math_unary_type = GetMathUnaryFunction(peektoken.tokenptr, peektoken.length);
    if (math_unary_type == MATH_UNARY_FUNC_COUNT)
        return (false);

    filebuf = peektoken;

    // -- next token better be an open parenthesis
    if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_OPEN))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
            "Error - expecting '('\n");
        return (false);
    }

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

	// -- create the Math function node, leftchild is statement resolving to a float arg for the math function
	CMathUnaryFuncNode* math_func_node = TinAlloc(ALLOC_TreeNode, CMathUnaryFuncNode, codeblock, link,
											      filebuf.linenumber, math_unary_type);

	// -- ensure we have a statement to fill the left child
	bool8 result = TryParseStatement(codeblock, filebuf, math_func_node->leftchild);
	if (!result || !math_func_node->leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - %s() requires a numerical expression\n", GetMathUnaryFuncString(math_unary_type));
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

	// -- success
	return (true);
}

// ====================================================================================================================
// TryParseMathBinaryFunction():  The keyword "min" is a binary function.
// ====================================================================================================================
bool8 TryParseMathBinaryFunction(CCodeBlock* codeblock, tReadToken& filebuf, CCompileTreeNode*& link)
{
	// -- ensure the next token is the 'hash' keyword
	tReadToken peektoken(filebuf);
	if (!GetToken(peektoken) || peektoken.type != TOKEN_KEYWORD)
		return (false);

    eMathBinaryFunctionType math_binary_type = GetMathBinaryFunction(peektoken.tokenptr, peektoken.length);
    if (math_binary_type == MATH_BINARY_FUNC_COUNT)
        return (false);

    filebuf = peektoken;

    // -- next token better be an open parenthesis
    if (!GetToken(filebuf) || (filebuf.type != TOKEN_PAREN_OPEN))
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
            "Error - expecting '('\n");
        return (false);
    }

    // -- increment the paren depth
    ++gGlobalExprParenDepth;

	// -- create the Math function node, leftchild is statement resolving to a float arg for the math function
	CMathBinaryFuncNode* math_func_node = TinAlloc(ALLOC_TreeNode, CMathBinaryFuncNode, codeblock, link,
											       filebuf.linenumber, math_binary_type);

	// -- ensure we have a statement to fill the left child
	bool8 result = TryParseStatement(codeblock, filebuf, math_func_node->leftchild);
	if (!result || !math_func_node->leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - %s() requires a numerical expression\n", GetMathBinaryFuncString(math_binary_type));
		return (false);
	}

    // -- consume the comma
    if (!GetToken(filebuf) || filebuf.type != TOKEN_COMMA)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                      filebuf.linenumber, "Error - expecting ','\n");
        return (false);
    }

	// -- ensure we have a statement to fill the left child
	result = TryParseStatement(codeblock, filebuf, math_func_node->rightchild);
	if (!result || !math_func_node->rightchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
			          "Error - %s() requires a numerical expression\n", GetMathBinaryFuncString(math_binary_type));
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

    // -- at this point we're obviously committed, the rightchild of a CScheduleNode is a
    // -- CSchedFuncNode, who's left child resolves to the hashvalue identifying a function,
    // -- and the right child is the root of the parameter assignments
    filebuf = peektoken;

    // -- add a CScheduleNode node
    CScheduleNode* schedulenode = TinAlloc(ALLOC_TreeNode, CScheduleNode, codeblock, link,
                                           filebuf.linenumber, repeat_execution);

    // -- the left child is a generic binary tree node
    CBinaryTreeNode* binary_tree_node = TinAlloc(ALLOC_TreeNode, CBinaryTreeNode, codeblock, schedulenode->leftchild,
                                                 filebuf.linenumber, TYPE_object, TYPE_int);

    // -- the binary tree node's left child resolving to an object ID,
    // -- and the right child resolves to a delay time
    binary_tree_node->leftchild = templink;

    // -- if this is immediate execution, the right child is a value (0) node, else an expression
    if (immediate_execution)
    {
        CValueNode* delay_0 = TinAlloc(ALLOC_TreeNode, CValueNode, codeblock,
                                       binary_tree_node->rightchild, filebuf.linenumber, "", 0, false,
                                       TYPE_int);
    }
    else
    {
        bool8 result_0 = TryParseStatement(codeblock, filebuf, binary_tree_node->rightchild);
        if (!result_0)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), filebuf.linenumber,
                          "Error - Unable to resolve a 'delay time' expression in a schedule/execute() call\n");
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

        // -- update the file buf
        filebuf = peektoken;
    }

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
        tReadToken peektoken_0(filebuf);
        if (!GetToken(peektoken_0))
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                          peektoken_0.linenumber,
                          "Error - expecting ')'\n");
            return (false);
        }

        if (peektoken_0.type == TOKEN_PAREN_CLOSE)
        {
            // -- we've found all the parameters we're going to find
            filebuf = peektoken_0;
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

        bool8 result_0 = TryParseStatement(codeblock, filebuf, schedparamnode->leftchild);
        if (!result_0)
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
	if (reservedwordtype != KEYWORD_create && reservedwordtype != KEYWORD_create_local)
        return (false);

	// -- see if we're creating a local object - one that is destructed as soon as the function context is popped
	bool local_object = (reservedwordtype == KEYWORD_create_local);

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

    // -- disallow return statements while in the middle of parenthetical expressions
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
        // -- preserve comments at the start of the statement block
        tReadToken comment_token(filetokenbuf);
        if (TryParseComment(codeblock, comment_token, curroot->next))
        {
            filetokenbuf = comment_token;
            curroot = curroot->next;
            continue;
        }

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
        const char* cur_file_tokenptr = filetokenbuf.tokenptr;
		found = found || TryParseComment(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseInclude(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseVarDeclaration(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseFuncDefinition(codeblock, filetokenbuf, curroot->next);
		found = found || TryParseStatement(codeblock, filetokenbuf, curroot->next, true);
		found = found || TryParseIfStatement(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseSwitchStatement(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseWhileLoop(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseDoWhileLoop(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseForLoop(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseForeachLoop(codeblock, filetokenbuf, curroot->next);
        found = found || TryParseDestroyObject(codeblock, filetokenbuf, curroot->next);

        // -- ensure we're not parsing in an infinite loop - this can (only?) happen
        // if there was an error at a lower recursive level, that somehow didn't get surfaced
        if (found && filetokenbuf.tokenptr == cur_file_tokenptr)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(),
                filetokenbuf.linenumber, "Parsing failed at token: [%s] %s, line %d\n",
                gTokenTypeStrings[filetokenbuf.type], TokenPrint(filetokenbuf),
                filetokenbuf.linenumber);
            return (false);
        }

		else if (found)
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
            // -- at this point, we want to ensure the paren depth is reset...
            // an un-parsable watch expression for example, can leave this non-zero, which will
            // prevent any subsequent watch expressions from being parsed
            gGlobalExprParenDepth = 0;
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
CCodeBlock* ParseFile(CScriptContext* script_context, const char* filename, bool& is_empty)
{
	// -- open the file - if it fails, it's an empty (or unreadable) file, and we're done
    is_empty = false;
	const char* filebuf = ReadFileAllocBuf(filename);
    if (filebuf == nullptr)
    {
        is_empty = true;
        return nullptr;
    }

    // -- return the codeblock created from parsing the file
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
        int32 instrwritten_0 = (int32)fwrite((void*)instrptr, (int32)sizeof(uint32), writecount, filehandle);
        fflush(filehandle);
        if (instrwritten_0 != writecount)
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

    while (instrptr != nullptr && remaining > 0) {
        int32 writecount = remaining > (BUFSIZ >> 2) ? (BUFSIZ >> 2) : remaining;
        remaining -= writecount;
        int32 instrwritten_0 = (int32)fwrite((void*)instrptr, (int32)sizeof(uint32), writecount, filehandle);
        fflush(filehandle);
        if (instrwritten_0 != writecount) {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), -1,
                          "Error - unable to write file %s\n", binfilename);
            return (false);
        }
        instrptr += writecount;
    }

#endif

    // -- close the file before we leave
	fclose(filehandle);

#if MEMORY_TRACKER_ENABLE

    // -- the total byte size, is:
    // --     version (uint32)
    // --     the # instructions (uint32)
    // --     instructions array  (instrcount * uint32)
    // --     the # line/offset entries (uint32)
    // --     line/offset array  (count * uint32)
    int32 totalsize = (int32)sizeof(uint32) * 3;
    totalsize += (int32)codeblock->GetInstructionCount() * (int32)sizeof(uint32);
    totalsize += linenumbercount * (int32)sizeof(uint32);
    TinPrint(codeblock->GetScriptContext(), "Compiled file: %s, size: %d\n",
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

    // -- if the version is not current, close and recompile
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
    if (linenumbercount > 0 && codeblock->GetLineNumberPtr() != nullptr)
    {
        uint32* readptr_0 = codeblock->GetLineNumberPtr();
        instrread = (int32)fread(readptr_0, sizeof(uint32), (int32)linenumbercount, filehandle);
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
// ParseFile_CompileToC():  Parse and compile a given file into a compileable 'source C' version
// ====================================================================================================================
const char* ParseFile_CompileToC(CScriptContext* script_context, const char* filename, int32& source_length)
{
	// -- see if we can open the file
	const char* filebuf = ReadFileAllocBuf(filename);
    return (ParseText_CompileToC(script_context, filename, filebuf, source_length));
}

// ====================================================================================================================
// ParseText_CompileToC();  Parse a text block, and return a text block with the 'source C' equivalent.
// ====================================================================================================================
const char* ParseText_CompileToC(CScriptContext* script_context, const char* filename, const char* filebuf,
                                 int32& source_length)
{
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

    // -- allocate a buffer large enough to contain the source
    const int k_maxFileLength = 1024 * 245;
    int32 max_size = k_maxFileLength;
    char* compile_to_source_c = new char[k_maxFileLength];
    char* compile_ptr = compile_to_source_c;
    
    // -- run through the tree again, this time actually compiling it
    if (!codeblock->CompileTreeToSourceC(*root, compile_ptr, max_size))
    {
		ScriptAssert_(script_context, 0, codeblock->GetFileName(), -1,
                      "Error - failed to compile tree for file: %s\n", codeblock->GetFileName());
        // -- failed
        codeblock->SetFinishedParsing();
        DestroyTree(root);
        return (NULL);
    }

    	// dump the tree
    if (gDebugParseTree)
    {
	    DumpTree(root, 0, false, false);
    }

    // -- finish parsing and destroy the tree
    codeblock->SetFinishedParsing();
    DestroyTree(root);

    // -- return the buffer containing the compiled source C, and the length
    source_length = k_maxFileLength - max_size;
	return (compile_to_source_c);
}

// ====================================================================================================================
// SaveToSourceC():  Writes out the compiled 'source C' to a file.
// ====================================================================================================================
bool8 SaveToSourceC(const char* script_filename, const char* source_C_filename, const char* source_c,
                    int32 source_length)
{
    if (!source_c || !source_C_filename || source_length == 0)
        return (false);

  	// -- open the file
	FILE* filehandle = NULL;
	if (source_C_filename)
    {
		 int32 result = fopen_s(&filehandle, source_C_filename, "w");
		 if (result != 0)
         {
             ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                           "Error - unable to write file %s\n", source_C_filename);
			 return (false);
         }
	}

	if (!filehandle)
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
		return (false);
    }
    setvbuf(filehandle, NULL, _IOFBF, BUFSIZ);

    // -- write the header
    const char* comment = "// ====================================================================================================================\n";
    int32 instrwritten = (int32)fwrite((void*)comment, sizeof(char), strlen(comment), filehandle);
    if (instrwritten != strlen(comment))
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
        return (false);
    }

    // -- write the filename
    char fn_buffer[kMaxArgLength];
    snprintf(fn_buffer, sizeof(fn_buffer), "// Comile To C: %s\n", source_C_filename);
    instrwritten = (int32)fwrite((void*)fn_buffer, sizeof(char), strlen(fn_buffer), filehandle);
    if (instrwritten != strlen(fn_buffer))
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
        return (false);
    }

    // -- write the version
    char version_buffer[kMaxArgLength];
    snprintf(version_buffer, sizeof(version_buffer), "// version: %d\n", kCompilerVersion);
    instrwritten = (int32)fwrite((void*)&version_buffer, sizeof(char), strlen(version_buffer), filehandle);
    if (instrwritten != strlen(version_buffer))
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
        return (false);
    }

    // -- close the header
    instrwritten = (int32)fwrite((void*)comment, sizeof(char), strlen(comment), filehandle);
    if (instrwritten != strlen(comment))
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
        return (false);
    }

    // -- write the source C
    instrwritten = (int32)fwrite((void*)source_c, sizeof(char), source_length, filehandle);
    if (instrwritten != source_length)
    {
        ScriptAssert_(TinScript::GetContext(), 0, script_filename, -1,
                      "Error - unable to write file %s\n", source_C_filename);
        return (false);
    }

    // -- close the file
    fclose(filehandle);
    // -- success
    return (true);
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
        // note:  hashtable isn't a natural C++ type, therefore, it'll never be found as the addr + offset of an object
        tVarTable* vartable = (tVarTable*)ve->GetAddr(NULL);

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
            // note:  hashtable isn't a natural C++ type, therefore, it'll never be found as the addr + offset of an object
            tVarTable* vartable = (tVarTable*)ve->GetAddr(NULL);

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
            nsentry = script_context->FindOrCreateNamespace(ns_string);
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
	fe = TinAlloc(ALLOC_FuncEntry, CFunctionEntry, nsentry->GetHash(), funcname,
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

REGISTER_FUNCTION(SetDebugParseTree, SetDebugParseTree);

// ====================================================================================================================
// eof
// ====================================================================================================================
