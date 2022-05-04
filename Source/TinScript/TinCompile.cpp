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
// TinCompile.cpp
// ====================================================================================================================

// -- lib includes
#include "assert.h"
#include "string.h"
#include "stdio.h"

// -- includes
#include "integration.h"
#include "TinScript.h"
#include "TinParse.h"
#include "TinCompile.h"
#include "TinExecute.h"
#include "TinNamespace.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// GetNodeTypeString():  Declaration and accessor to identify compile tree nodes.
// ====================================================================================================================
const char* gCompileNodeTypes[eNodeTypeCount] =
{
	#define CompileNodeTypeEntry(a) #a,
	CompileNodeTypesTuple
	#undef CompileNodeTypeEntry
};

const char* GetNodeTypeString(ECompileNodeType nodetype)
{
	return gCompileNodeTypes[nodetype];
}

// ====================================================================================================================
// GetOperationString():  Declaration and accessor to identify types of operations.
// ====================================================================================================================
const char* gOperationName[OP_COUNT] =
{
	#define OperationEntry(a) #a,
	OperationTuple
	#undef OperationEntry
};

const char* GetOperationString(eOpCode op)
{
	return gOperationName[op];
}

// ====================================================================================================================
// GetBinOpInstructionType():  Declaration and accessor to identify types of binary operations and their precedence.
// ====================================================================================================================
eOpCode gBinInstructionType[] =
{
	#define BinaryOperatorEntry(a, b, c) OP_##a,
	BinaryOperatorTuple
	#undef BinaryOperatorEntry
};

int32 gBinOpPrecedence[] =
{
	#define BinaryOperatorEntry(a, b, c) c,
	BinaryOperatorTuple
	#undef BinaryOperatorEntry
};

eOpCode GetBinOpInstructionType(eBinaryOpType binoptype)
{
    return gBinInstructionType[binoptype];
}

int32 GetBinOpPrecedence(eBinaryOpType binoptype)
{
    return gBinOpPrecedence[binoptype];
}

// ====================================================================================================================
// GetAssOpInstructionType():  Declaration and accessor to identify types of assignment operations.
// ====================================================================================================================
static eOpCode gAssInstructionType[] =
{
	#define AssignOperatorEntry(a, b) OP_##a,
	AssignOperatorTuple
	#undef AssignOperatorEntry
};

eOpCode GetAssOpInstructionType(eAssignOpType assoptype)
{
    return gAssInstructionType[assoptype];
}

// ====================================================================================================================
// GetUnaryOpInstructionType():  Declaration and accessor to identify types of unary operations.
// ====================================================================================================================
static eOpCode gUnaryInstructionType[] =
{
	#define UnaryOperatorEntry(a, b) OP_##a,
	UnaryOperatorTuple
	#undef UnaryOperatorEntry
};

eOpCode GetUnaryOpInstructionType(eUnaryOpType unarytype)
{
    return gUnaryInstructionType[unarytype];
}

// ====================================================================================================================
// -- debug type, enum, and string to provide labels for byte code traces and dumps
#define DebugByteCodeTuple \
	DebugByteCodeEntry(NULL) \
	DebugByteCodeEntry(instr) \
	DebugByteCodeEntry(vartype) \
	DebugByteCodeEntry(var) \
	DebugByteCodeEntry(value) \
	DebugByteCodeEntry(func) \
	DebugByteCodeEntry(hash) \
	DebugByteCodeEntry(nshash) \
	DebugByteCodeEntry(self) \
	DebugByteCodeEntry(super) \

enum eDebugByteType {
	#define DebugByteCodeEntry(a) DBG_##a,
	DebugByteCodeTuple
	#undef DebugByteCodeEntry
};

static const char* gDebugByteTypeName[] = {
	#define DebugByteCodeEntry(a) #a,
	DebugByteCodeTuple
	#undef DebugByteCodeEntry
};

// ====================================================================================================================
// PushInstructionRaw():  As the parse tree is compiled, instructions are created.
// ====================================================================================================================
int32 PushInstructionRaw(bool8 countonly, uint32*& instrptr, void* content, int32 wordcount, eDebugByteType debugtype,
                         const char* debugmsg = NULL)
{
	if (!countonly)
    {
		memcpy(instrptr, content, wordcount * 4);
		instrptr += wordcount;
	}

#if DEBUG_CODEBLOCK
    if (CScriptContext::gDebugCodeBlock && !countonly) {
	    for(int32 i = 0; i < wordcount; ++i) {
		    if (i == 0) {
			    const char* debugtypeinfo = NULL;
			    switch(debugtype) {
				    case DBG_instr:
					    debugtypeinfo = GetOperationString((eOpCode)(*(uint32*)content));
					    break;
				    case DBG_vartype:
					    debugtypeinfo = GetRegisteredTypeName((eVarType)(*(uint32*)content));
					    break;
                    case DBG_var:
                    case DBG_func:
                    case DBG_nshash:
                        debugtypeinfo = UnHash(*(uint32*)content);
                        break;
				    default:
					    debugtypeinfo = "";
					    break;
			    }
			    TinPrint(TinScript::GetContext(), "0x%08x\t\t:\t// [%s: %s]: %s\n", ((uint32*)content)[i],
												   gDebugByteTypeName[debugtype],
												   debugtypeinfo,
												   debugmsg ? debugmsg : "");
		    }
            else
            {
                TinPrint(TinScript::GetContext(), "0x%x\n", ((uint32*)content)[i]);
            }
	    }
    }
#endif
	return wordcount;
}

// ====================================================================================================================
// PushInstruction():  As the parse tree is compiled, instructions are created.
// ====================================================================================================================
int32 PushInstruction(bool8 countonly, uint32*& instrptr, uint32 content, eDebugByteType debugtype,
                      const char* debugmsg = NULL)
{
	return PushInstructionRaw(countonly, instrptr, (void*)&content, 1, debugtype, debugmsg);
}

// ====================================================================================================================
// DebugEvaluateNode():  Addes debug information to the code block for each node, as the parse tree is compiled.
// ====================================================================================================================
void DebugEvaluateNode(const CCompileTreeNode& node, bool8 countonly, uint32* instrptr)
{
#if DEBUG_CODEBLOCK
    if (CScriptContext::gDebugCodeBlock && !countonly)
    {
        TinPrint(TinScript::GetContext(), "\n--- Eval: %s\n", GetNodeTypeString(node.GetType()));
    }

    // -- if we're debugging, add the line number for the current operation
    if (node.GetCodeBlock())
        node.GetCodeBlock()->AddLineNumber(node.GetLineNumber(), instrptr);
#endif
}

// ====================================================================================================================
// DebugEvaluateBinOpNode():  Adds debug information to the code block for binary op nodes, during compilation.
// ====================================================================================================================
void DebugEvaluateBinOpNode(const CBinaryOpNode& binopnode, bool8 countonly)
{
#if DEBUG_CODEBLOCK
    if (CScriptContext::gDebugCodeBlock && !countonly)
    {
        TinPrint(TinScript::GetContext(), "\n--- Eval: %s [%s]\n", GetNodeTypeString(binopnode.GetType()),
                                          GetOperationString(binopnode.GetOpCode()));
    }
#endif
}

// ====================================================================================================================
// CompileVarTable():  Adds variable declarations for the variables added when compiling the code block.
// ====================================================================================================================
int32 CompileVarTable(tVarTable* vartable, uint32*& instrptr, bool8 countonly)
{
    int32 size = 0;
	if (vartable)
    {
		CVariableEntry* ve = vartable->First();
        while (ve)
        {
		    // -- create instructions to declare each variable
            size += PushInstruction(countonly, instrptr, OP_VarDecl, DBG_instr);
            size += PushInstruction(countonly, instrptr, ve->GetHash(), DBG_var);
            size += PushInstruction(countonly, instrptr, ve->GetType(), DBG_vartype);
            size += PushInstruction(countonly, instrptr, ve->GetArraySize(), DBG_value);

			ve = vartable->Next();
		}
	}
    return size;
}

// ====================================================================================================================
// CompileVarTable():  Adds parameter and local variable declaration operations for functions defined in a code block.
// ====================================================================================================================
int32 CompileFunctionContext(CFunctionEntry* fe, uint32*& instrptr, bool8 countonly)
{
    // -- get the context for the function
    CFunctionContext* funccontext = fe->GetContext();
    int32 size = 0;

    // -- push the parameters
    int32 paramcount = funccontext->GetParameterCount();
    for (int32 i = 0; i < paramcount; ++i)
    {
        CVariableEntry* ve = funccontext->GetParameter(i);
        assert(ve);
        size += PushInstruction(countonly, instrptr, OP_ParamDecl, DBG_instr);
        size += PushInstruction(countonly, instrptr, ve->GetHash(), DBG_var);
        size += PushInstruction(countonly, instrptr, ve->GetType(), DBG_vartype);
        size += PushInstruction(countonly, instrptr, ve->GetArraySize(), DBG_value);
    }

    // -- now declare the rest of the local vars
    tVarTable* vartable = funccontext->GetLocalVarTable();
	if (vartable)
    {
	    CVariableEntry* ve = vartable->First();
        while (ve)
        {
            if (! ve->IsParameter())
            {
                size += PushInstruction(countonly, instrptr, OP_VarDecl, DBG_instr);
                size += PushInstruction(countonly, instrptr, ve->GetHash(), DBG_var);
                size += PushInstruction(countonly, instrptr, ve->GetType(), DBG_vartype);
                size += PushInstruction(countonly, instrptr, ve->GetArraySize(), DBG_value);
            }
		    ve = vartable->Next();
		}
	}

    // -- initialize the stack var offsets
    if (!countonly)
        funccontext->InitStackVarOffsets(fe);

    return size;
}

// == class CCompileTreeNode ==========================================================================================

// ====================================================================================================================
// CreateTreeRoot():  Creates a root node for a parse tree.
// ====================================================================================================================
CCompileTreeNode* CCompileTreeNode::CreateTreeRoot(CCodeBlock* codeblock)
{
    CCompileTreeNode* root = TinAlloc(ALLOC_TreeNode, CCompileTreeNode, codeblock);
	root->next = NULL;
	root->leftchild = NULL;
	root->rightchild = NULL;

	return root;
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCompileTreeNode::CCompileTreeNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, ECompileNodeType nodetype,
                                   int32 _linenumber)
{
	type = nodetype;
	next = NULL;
	leftchild = NULL;
	rightchild = NULL;

	// -- hook up the node to the tree
	_link = this;

    // -- store the current code block we're compiline
    codeblock = _codeblock;
    linenumber = _linenumber;

    // -- no post-op increment/decrements by default;
    m_unaryDelta = 0;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CCompileTreeNode::~CCompileTreeNode()
{
	assert(leftchild == NULL && rightchild == NULL);
}

// ====================================================================================================================
// Eval():  Evaluate a parse tree, starting from a tree root, and advancing through the "next" linked list.
// ====================================================================================================================
int32 CCompileTreeNode::Eval(uint32*& instrptr, eVarType, bool8 countonly) const
{

	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- NOP nodes have no children, but loop through and evaluate the chain of siblings
	const CCompileTreeNode* rootptr = next;
	while (rootptr)
    {
        int32 tree_size = rootptr->Eval(instrptr, TYPE_void, countonly);
        if (tree_size < 0)
            return -1;
		size += tree_size;

        // -- we're done if the rootptr is a NOP, as it would have already evaluated
        // -- the rest of the linked list
        if (rootptr->GetType() == eNOP)
            break;

		rootptr = rootptr->next;
	}

	return size;
}

// ====================================================================================================================
// Dump():  Dumps out the debug type for parse tree node.
// ====================================================================================================================
void CCompileTreeNode::Dump(char*& output, int32& length) const
{
	snprintf(output, length, "type: %s", gCompileNodeTypes[type]);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// OutputIndentToBuffer():  Write out spaces to align the output to a given indent level
// ====================================================================================================================
bool8 CCompileTreeNode::OutputIndentToBuffer(int32 indent, char*& out_buffer, int32& max_size) const
{
    // -- write the indent
    const char* indent_buffer = "    ";
    if (indent * (int)strlen(indent_buffer) > max_size)
    {
        ScriptAssert_(TinScript::GetContext(), 0, codeblock->GetFileName(), linenumber,
                      "Error - CompileToC - max buffer size reached.\n");
        return (false);
    }

    // -- prepend the indent
    for (int i = 0; i < indent; ++i)
    {
        snprintf(out_buffer, max_size, "%s", "    ");
        out_buffer += strlen(indent_buffer);
        max_size -= strlen(indent_buffer);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// OutputToBuffer():  Write out the 'source C' for the node instruction.
// ====================================================================================================================
bool8 CCompileTreeNode::OutputToBuffer(int32 indent, char*& out_buffer, int32& max_size, const char* format, ...) const
{
    // -- sanity check
    if (format == nullptr)
        return (true);

    va_list args;
    va_start(args, format);
    char temp[kMaxTokenLength];
    vsprintf_s(temp, kMaxTokenLength, format, args);
    va_end(args);
    
    // -- write the indent
    if (!OutputIndentToBuffer(indent, out_buffer, max_size))
        return (false);

    // -- now append the actual text, indenting after every newline
    const char* out_ptr = temp;
    const char* new_line = strchr(temp, '\n');
    do
    {
        int32 length = new_line != nullptr ? kPointerDiffUInt32(new_line, out_ptr)
                                           : strlen(out_ptr);
        if (length > max_size)
        {
            ScriptAssert_(TinScript::GetContext(), 0, codeblock->GetFileName(), linenumber,
                          "Error - CompileToC - max buffer size reached.\n");
            return (false);
        }

        // -- copy to the out_buffer
        if (length > 0)
        {
            strncpy_s(out_buffer, max_size, out_ptr, length);
            out_buffer += length;
            max_size -= length;
        }
        
        // -- see if there's a new_line
        if (new_line != nullptr)
        {
            if (max_size == 0)
            {
                ScriptAssert_(TinScript::GetContext(), 0, codeblock->GetFileName(), linenumber,
                              "Error - CompileToC - max buffer size reached.\n");
                return (false);
            }

            // -- output the newline
            *out_buffer++ = '\n';
            --max_size;

            // -- update the out_ptr
            out_ptr = new_line + 1;
            new_line = strchr(out_ptr, '\n');

            // -- indent, only if we're not at the end of the string
            if (*out_ptr != '\0' && !OutputIndentToBuffer(indent, out_buffer, max_size))
                return (false);
        }
    } while (new_line != nullptr);

    // -- success
    return (true);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CCompileTreeNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
	// -- NOP nodes have no children, but loop through and evaluate the chain of siblings
	bool is_root_node = root_node;
	const CCompileTreeNode* rootptr = next;
	while (rootptr)
    {
        bool result = rootptr->CompileToC(indent, out_buffer, max_size, is_root_node);
        if (!result)
            return (false);

        // -- we're done if the rootptr is a NOP, as it would have already evaluated
        // -- the rest of the linked list
        if (rootptr->GetType() == eNOP)
        {
            break;
        }

        // -- complete the statement
        if (is_root_node)
        {
            // -- we need a way to determine which root nodes are actually statements
            if (rootptr->GetType() != eComment)
            {
                if (!OutputToBuffer(0, out_buffer, max_size, ";\n"))
                    return (false);
            }
        }

        // -- get the next rootptr in the linked list
		rootptr = rootptr->next;
	}


    return (true);
}

// == class CDebugNode ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugNode::CDebugNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, const char* debug_msg)
    : CCompileTreeNode(_codeblock, _link, eDebugNOP, -1)
{
    m_debugMesssage =
        TinScript::GetContext()->GetStringTable()->AddString(debug_msg, TinScript::Hash(debug_msg), true);
}

// ====================================================================================================================
// Eval():  Has no operations of its own, used to compile left, then right children.
// ====================================================================================================================
int32 CDebugNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- push the op
    size += PushInstruction(countonly, instrptr, OP_DebugMsg, DBG_instr);

    // -- push the hash of the string value
    uint32 hash = TinScript::Hash(m_debugMesssage);
    size += PushInstruction(countonly, instrptr, hash, DBG_value, "debug message");

    return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CDebugNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CDebugNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CCommentNode ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCommentNode::CCommentNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, const char* _comment, int32 _length)
    : CCompileTreeNode(_codeblock, _link, eComment, _linenumber)
{
	SafeStrcpy(m_comment, sizeof(m_comment), _comment, _length + 1);
}

// ====================================================================================================================
// Eval():  Has no operations of its own, used to compile left, then right children.
// ====================================================================================================================
int32 CCommentNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);

    // -- comment nodes preserve the comment for compileToC, and have no functionality
    return (0);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CCommentNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- comments start on a new line
    if (!OutputToBuffer(indent, out_buffer, max_size, "\n"))
        return (false);

    // -- output the comment (preserving the indent)
    if (!OutputToBuffer(indent, out_buffer, max_size, "%s", m_comment))
        return (false);

    return (true);
}

// == class CBinaryTreeNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CBinaryTreeNode::CBinaryTreeNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                 eVarType _left_result_type, eVarType _right_result_type)
    : CCompileTreeNode(_codeblock, _link, eBinaryNOP, _linenumber)
{
    m_leftResultType = _left_result_type;
    m_rightResultType = _right_result_type;
}

// ====================================================================================================================
// Eval():  Has no operations of its own, used to compile left, then right children.
// ====================================================================================================================
int32 CBinaryTreeNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- ensure we have a left child
    if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CBinaryTreeNode with no left child\n");
        return (-1);
    }

    // -- ensure we have a left child
    if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CBinaryTreeNode with no right child\n");
        return (-1);
    }

    // -- evaluate the left child, pushing the result of the type required
    // -- except in the case of an assignment operator - the left child is the variable
    int32 tree_size = leftchild->Eval(instrptr, m_leftResultType, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- evaluate the right child, pushing the result
    tree_size = rightchild->Eval(instrptr, m_rightResultType, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CBinaryTreeNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CBinaryTreeNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CIncludeScriptNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CIncludeScriptNode::CIncludeScriptNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                       uint32 _filename_hash)
    : CCompileTreeNode(_codeblock, _link, eSelf, _linenumber)
{
    mFilenameHash = _filename_hash;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CIncludeScriptNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- if the value is being used, push it on the stack
    size += PushInstruction(countonly, instrptr, OP_Include, DBG_var);
    size += PushInstruction(countonly, instrptr, mFilenameHash, DBG_hash);

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CIncludeScriptNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, filename: %s", gCompileNodeTypes[type], UnHash(mFilenameHash));
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CIncludeScriptNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CIncludeScriptNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CValueNode ================================================================================================

// ====================================================================================================================
// Constructor:  Used for values and variables.
// ====================================================================================================================
CValueNode::CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       const char* _value, int32 _valuelength, bool8 _isvar,
                       eVarType _valtype)
    : CCompileTreeNode(_codeblock, _link, eValue, _linenumber)
{
	SafeStrcpy(value, sizeof(value), _value, _valuelength + 1);
	isvariable = _isvar;
    isparam = false;
    valtype = _valtype;
}

// ====================================================================================================================
// Constructor:  Used when the value is a function parameter.
// ====================================================================================================================
CValueNode::CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 _paramindex,
                       eVarType _valtype)
    : CCompileTreeNode(_codeblock, _link, eValue, _linenumber)
{
    value[0] = '\0';
	isvariable = false;
    isparam = true;
    paramindex = _paramindex;
    valtype = _valtype;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CValueNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- if the value is being used, push it on the stack
	if (pushresult > TYPE_void || m_unaryDelta != 0)
    {
        if (isparam)
        {
			size += PushInstruction(countonly, instrptr, OP_PushParam, DBG_instr);
			size += PushInstruction(countonly, instrptr, paramindex, DBG_hash);
        }
		else if (isvariable)
        {
            int32 stacktopdummy = 0;
            CObjectEntry* dummy = NULL;
            CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);

			// -- ensure we can find the variable
			uint32 varhash = Hash(value);
            uint32 funchash = curfunction ? curfunction->GetHash() : 0;
            uint32 nshash = curfunction ? curfunction->GetNamespaceHash() : CScriptContext::kGlobalNamespaceHash;
            CVariableEntry* var = GetVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable,
                                              nshash, funchash, varhash, 0);
			if (!var)
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                              "Error - undefined variable: %s\n", value);
				return (-1);
			}
			eVarType vartype = var->GetType();

			// -- if we're supposed to be pushing a var (e.g. for an assign...)
            // -- (note:  there is no such thing as the "value" of a hashtable)
            // -- we also pus the variable, if we are planning to perform a post increment/decrement unary op
            bool8 push_value = (m_unaryDelta == 0 && pushresult != TYPE__var && pushresult != TYPE_hashtable &&
                                var->GetType() != TYPE_hashtable && !var->IsArray());

            // -- if this isn't a func var, make sure we push the global namespace
            if (var->GetFunctionEntry() == NULL)
            {
				size += PushInstruction(countonly, instrptr, push_value ? OP_PushGlobalValue : OP_PushGlobalVar,
                                        DBG_instr);
				size += PushInstruction(countonly, instrptr, CScriptContext::kGlobalNamespaceHash, DBG_hash);
				size += PushInstruction(countonly, instrptr, 0, DBG_func);
        		size += PushInstruction(countonly, instrptr, var->GetHash(), DBG_var);
            }
            // -- otherwise this is a stack var
            else
            {
				size += PushInstruction(countonly, instrptr, push_value ? OP_PushLocalValue : OP_PushLocalVar,
                                        DBG_instr);
				size += PushInstruction(countonly, instrptr, var->GetType(), DBG_vartype);

                // -- for local vars, it's the offset on the stack we need to push
                int32 stackoffset = var->GetStackOffset();
                if (!countonly && stackoffset < 0)
                {
                    ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                                    "Error - invalid stack offset for local var: %s\n", UnHash(var->GetHash()));
                    return (-1);
                }
        		size += PushInstruction(countonly, instrptr, stackoffset, DBG_var);

                // -- push the local var index as well
                int32 var_index = 0;
                CVariableEntry* local_ve = var->GetFunctionEntry()->GetLocalVarTable()->First();
                while (local_ve && local_ve != var)
                {
                    local_ve = var->GetFunctionEntry()->GetLocalVarTable()->Next();
                    ++var_index;
                }
        		size += PushInstruction(countonly, instrptr, var_index, DBG_var);
            }

            // -- if we're applying a post increment/decrement, we also need to push the post-op instruction
            if (m_unaryDelta != 0)
            {
                size += PushInstruction(countonly, instrptr, m_unaryDelta > 0 ? OP_UnaryPostInc : OP_UnaryPostDec, DBG_instr);
                size += PushInstruction(countonly, instrptr, 0, DBG_value, "non-array var");

                // -- in addition, if the value isn't actually going to be used, issue an immediate pop
                if (pushresult == TYPE_void)
                    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr, "post unary op");
            }
        }

		// -- else we're pushing an actual value
		else
        {
			size += PushInstruction(countonly, instrptr, OP_Push, DBG_instr);

			// -- the next instruction is the type to be pushed
            eVarType pushtype = (pushresult == TYPE__resolve) ? valtype : pushresult;
			size += PushInstruction(countonly, instrptr, pushtype, DBG_vartype);

			// convert the value string to the appropriate type
			// increment the instrptr by the number of 4-byte instructions
			char valuebuf[kMaxTokenLength];
			if (gRegisteredStringToType[pushtype](TinScript::GetContext(), (void*)valuebuf, (char*)value))
            {
				int32 resultsize = kBytesToWordCount(gRegisteredTypeSize[pushtype]);
    		    size += PushInstructionRaw(countonly, instrptr, (void*)valuebuf, resultsize,
										   DBG_value);

                // -- if the value type is a string literal, we need to ensure it's added to the dictionary
                // $$$TZA This is necessary for unit test "flow_if", I'm not 100% certain this doesn't cause
                // strings to be ref-counted beyond their use, but better to ensure the string still exists,
                // than to remove a string that is still needed...
                if (pushtype == TYPE_string && !countonly)
                    codeblock->GetScriptContext()->GetStringTable()->RefCountIncrement(*(uint32*)valuebuf);
    		}
			else
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                              "Error - unable to convert value %s to type %s\n", value,
                              gRegisteredTypeNames[pushtype]);
                return (-1);
			}
		}

		// -- return the size
		return size;
	}

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CValueNode::Dump(char*& output, int32& length) const
{
    if (isparam)
        snprintf(output, length, "type: %s, param: %d", gCompileNodeTypes[type], paramindex);
    else if (isvariable)
        snprintf(output, length, "type: %s, var: %s", gCompileNodeTypes[type], value);
    else
        snprintf(output, length, "type: %s, %s", gCompileNodeTypes[type], value);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CValueNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- if the value is a variable, print the variable
    if (isvariable)
    {
        if (!OutputToBuffer(indent, out_buffer, max_size, value))
            return (false);

        // -- if there's a post unary op, output it
        if (m_unaryDelta != 0)
        {
            if (!OutputToBuffer(0, out_buffer, max_size, "%s", m_unaryDelta > 0 ? "++" : "--"))
                return (false);
        }
    }

    //-- otherwise, it's a value - conveniently, the value is already stored as a string
    else
    {
        // $$$TZA We need to escape string delineators, and possibly "double escape" internal escaped characters
        if (valtype == TYPE_string)
        {
            if (!OutputToBuffer(indent, out_buffer, max_size, "\"%s\"", value))
                return (false);
        }
        else
        {
            if (!OutputToBuffer(indent, out_buffer, max_size, value))
                return (false);

            // -- if there's a post unary op, output it
            if (m_unaryDelta != 0)
            {
                if (!OutputToBuffer(0, out_buffer, max_size, "%s", m_unaryDelta > 0 ? "++" : "--"))
                    return (false);
            }
        }
    }


    return (true);
}

// == class CSelfNode =================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSelfNode::CSelfNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eSelf, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CSelfNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- if the value is being used, push it on the stack
	if (pushresult > TYPE_void) {
	    size += PushInstruction(countonly, instrptr, OP_PushSelf, DBG_var);
	}

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CSelfNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CValueNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CObjMemberNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjMemberNode::CObjMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                               const char* _membername, int32 _memberlength)
    : CCompileTreeNode(_codeblock, _link, eObjMember, _linenumber)
{
	SafeStrcpy(membername, sizeof(membername), _membername, _memberlength + 1);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CObjMemberNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const {
	
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                        "Error - CObjMemberNode with no left child\n");
		return (-1);
	}

  	// -- evaluate the left child, pushing the a result of TYPE_object
    int32 tree_size = leftchild->Eval(instrptr, TYPE_object, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	// -- if the value is being used, push it on the stack
    if (pushresult > TYPE_void || m_unaryDelta != 0)
    {
        // -- get the hash of the member
		uint32 memberhash = Hash(membername);

		// -- if we're supposed to be pushing a var (for an assign...), we actually push
        // -- a member (still a variable, but the lookup is different)
        if (pushresult == TYPE__var || pushresult == TYPE_hashtable || m_unaryDelta != 0)
        {
			size += PushInstruction(countonly, instrptr, OP_PushMember, DBG_instr);
			size += PushInstruction(countonly, instrptr, memberhash, DBG_var);
		}

		// -- otherwise we push the hash, but the instruction is to get the value
		else
        {
			size += PushInstruction(countonly, instrptr, OP_PushMemberVal, DBG_instr);
			size += PushInstruction(countonly, instrptr, memberhash, DBG_var);
		}

        // -- if we're applying a post increment/decrement, we also need to push the post-op instruction
        if (m_unaryDelta != 0)
        {
            size += PushInstruction(countonly, instrptr, m_unaryDelta > 0 ? OP_UnaryPostInc : OP_UnaryPostDec, DBG_instr);
            size += PushInstruction(countonly, instrptr, 0, DBG_value, "non-array var");
        }
    }

    // -- if we're referencing a member without actually doing anything - pop the stack
    if (pushresult == TYPE_void)
    {
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    }

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CObjMemberNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, %s", gCompileNodeTypes[type], membername);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CObjMemberNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CObjMemberNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CPODMemberNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CPODMemberNode::CPODMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                               const char* _membername, int32 _memberlength)
    : CCompileTreeNode(_codeblock, _link, ePODMember, _linenumber)
{
	SafeStrcpy(podmembername, sizeof(podmembername), _membername, _memberlength + 1);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CPODMemberNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CPODMemberNode with no left child\n");
		return (-1);
	}

  	// -- evaluate the left child - the pushresult for the leftchild should be the same
    // -- either we're referencing the POD member of a value, or a variable
    // -- note:  if we're applying a post unary op, then we need to left child to resolve to a variable, not a value
    eVarType var_result_type = (pushresult == TYPE_void && m_unaryDelta != 0) ? TYPE__var : pushresult;
    int32 tree_size = leftchild->Eval(instrptr, var_result_type, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	// -- if the value is being used, push it on the stack
    if (pushresult > TYPE_void || m_unaryDelta != 0)
    {
        // -- get the hash of the member
		uint32 memberhash = Hash(podmembername);

		// -- if we're supposed to be pushing a var (for an assign...), we actually push
        // -- a member (still a variable, but the lookup is different)
        if (pushresult == TYPE__var || m_unaryDelta != 0)
        {
			size += PushInstruction(countonly, instrptr, OP_PushPODMember, DBG_instr);
			size += PushInstruction(countonly, instrptr, memberhash, DBG_var);
		}

		// -- otherwise we push the hash, but the instruction is to get the value
		else
        {
			size += PushInstruction(countonly, instrptr, OP_PushPODMemberVal, DBG_instr);
			size += PushInstruction(countonly, instrptr, memberhash, DBG_var);
		}

        // -- if we're applying a post increment/decrement, we also need to push the post-op instruction
        if (m_unaryDelta != 0)
        {
            size += PushInstruction(countonly, instrptr, m_unaryDelta > 0 ? OP_UnaryPostInc : OP_UnaryPostDec, DBG_instr);
            size += PushInstruction(countonly, instrptr, 0, DBG_value, "POD var");
        }
    }

    // -- otherwise, we're referencing a member without actually doing anything - pop the stack
    if (pushresult == TYPE_void)
    {
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    }

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CPODMemberNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, %s", gCompileNodeTypes[type], podmembername);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CPODMemberNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CPODMemberNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CPODMethodNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CPODMethodNode::CPODMethodNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                               const char* _method_name, int32 _method_length)
    : CCompileTreeNode(_codeblock, _link, ePODMethod, _linenumber)
{
	SafeStrcpy(mPODMethodName, sizeof(mPODMethodName), _method_name, _method_length + 1);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CPODMethodNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CPODMethodNode with no left child\n");
		return (-1);
	}
    // -- ensure we have a right child
    if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CPODMethodNode with no left child\n");
        return (-1);
    }

  	// -- evaluate the left child, pushing a result that is a POD variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- evaluate the right child, which contains the function call node
    tree_size = rightchild->Eval(instrptr, pushresult, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- after the function call, we want to notify the POD method is complete, so any changes to the
    // original POD value can copied back to the variable, as per the function's reassign flag
    size += PushInstruction(countonly, instrptr, OP_PODCallComplete, DBG_instr);

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CPODMethodNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, %s", gCompileNodeTypes[type], mPODMethodName);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CPODMethodNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CPODMethodNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CBinaryOpNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CBinaryOpNode::CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                             eBinaryOpType _binaryoptype, bool8 _isassignop, eVarType _resulttype)
    : CCompileTreeNode(_codeblock, _link, eBinaryOp, _linenumber)
{
	binaryopcode = GetBinOpInstructionType(_binaryoptype);
    binaryopprecedence = GetBinOpPrecedence(_binaryoptype);
	binopresult = _resulttype;
	assign_op = ASSOP_NULL;
    bin_op = _binaryoptype;
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CBinaryOpNode::CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                             eAssignOpType _assoptype, bool8 _isassignop, eVarType _resulttype)
    : CCompileTreeNode(_codeblock, _link, eBinaryOp, _linenumber)
{
	binaryopcode = GetAssOpInstructionType(_assoptype);
    binaryopprecedence = 0;
	binopresult = _resulttype;
	assign_op = _isassignop ? _assoptype : ASSOP_NULL;
    bin_op = BINOP_NULL;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CBinaryOpNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateBinOpNode(*this, countonly);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        ScriptAssert_(TinScript::GetContext(), 0, codeblock->GetFileName(), linenumber,
                      "Error - BinOp [%s] failed - No left child.\n", GetBinOperatorString(bin_op));
		return (-1);
	}

	// -- ensure we have a left child
	if (!rightchild)
    {
        ScriptAssert_(TinScript::GetContext(), 0, codeblock->GetFileName(), linenumber,
                      "Error - BinOp [%s] failed - No right child.\n", GetBinOperatorString(bin_op));
		return (-1);
	}

	// -- note:  if the binopresult is TYPE_NULL, simply inherit the result from the parent node
	eVarType childresulttype = binopresult != TYPE_NULL ? binopresult : pushresult;

	// -- evaluate the left child, pushing the result of the type required
	// -- except in the case of an assignment operator - the left child is the variable
    int32 tree_size = leftchild->Eval(instrptr, IsAssignOpNode() ? TYPE__var : childresulttype, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- if our left child is an assignment, it'll consume the stack contents - we need to re-push
    // -- the value of the assignment back onto the stack
    if (leftchild->IsAssignOpNode())
    {
        // -- add the instruction to push the last assignment result before performing ours
        size += PushInstruction(countonly, instrptr, OP_PushAssignValue, DBG_instr, "consec assigns");
    }

    // -- if the binary op is boolean, we can insert a branch to pre-empt the result:
	// -- e.g.  if the lhs of an "or" is true, we don't need to evaluate the rhs
    uint32* branchwordcount = instrptr;
    uint32 empty = 0;
    bool useShortCircuit = (binaryopcode == OP_BooleanAnd || binaryopcode == OP_BooleanOr);
	if (useShortCircuit)
	{
		size += PushInstruction(countonly, instrptr, OP_BranchCond, DBG_instr);

        // -- push the condition value (branch false, or branch true)
        size += PushInstruction(countonly, instrptr, binaryopcode == OP_BooleanAnd ? 0 : 1,
                                DBG_value, "condition type for branch");

        // -- this is a "short circuit" conditional branch, so we don't pop the result
        size += PushInstruction(countonly, instrptr, 1, DBG_value, "short circuit conditional branch");


		// -- cache the current intrptr, because we'll need to how far to
		// -- jump, after we've evaluated the left child
		// -- push a placeholder in the meantime
        branchwordcount = instrptr;
		size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");
	}

    // -- cache the current size, in case we need to branch
    int32 cursize = size;

	// -- evaluate the right child, pushing the result
    tree_size = rightchild->Eval(instrptr, childresulttype, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- if our right child is an assignment, it'll consume the stack contents - we need to re-push
    // -- the value of the assignment back onto the stack
    if (rightchild->IsAssignOpNode())
    {
        // -- add the instruction to push the last assignment result before performing ours
        size += PushInstruction(countonly, instrptr, OP_PushAssignValue, DBG_instr, "consec assigns");
    }

    // -- push the specific operation to be performed
    size += PushInstruction(countonly, instrptr, binaryopcode, DBG_instr);

    // -- the branch destination is after the evaluation of the binary op code
    // -- if booleanAnd, and the left child is false, then:
    // -- 1.  we leave the "false" on the stack by using the "short circuit" branch
    // -- 2.  we skip evaulating the right child, so there is still only one arg on the stack
    // -- 3.  we skip evaluating the binary op, since it would pops two, and pushes the same result
    if (useShortCircuit)
    {
        // fill in the jumpcount
        if (!countonly)
        {
            int32 jumpcount = size - cursize;
            *branchwordcount = jumpcount;
        }
    }

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CBinaryOpNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, op: %s", gCompileNodeTypes[type],
             gOperationName[binaryopcode]);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CBinaryOpNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CBinaryOpNode with no left child\n");
		return (false);
	}

	// -- ensure we have a left child
	if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CBinaryOpNode with no right child\n");
		return (false);
	}

    // -- if this binary op is a parameter assignment, we only need to output the values
    bool is_param_assignment = IsAssignOpNode() && leftchild->GetType() == ECompileNodeType::eValue &&
                               ((CValueNode*)(leftchild))->IsParameter();
    if (!is_param_assignment)
    {
        // -- Compile the left child to C
        if (!leftchild->CompileToC(root_node ? indent : 0, out_buffer, max_size))
            return (false);

        // -- output the operation
        if (IsAssignOpNode())
        {
            if (!OutputToBuffer(0, out_buffer, max_size, " %s ", GetAssOperatorString(assign_op)))
                return (false);
        }
        else
        {
            if (!OutputToBuffer(0, out_buffer, max_size, " %s ", GetBinOperatorString(bin_op)))
                return (false);
        }
    }

    // -- Compile the right child to C
    if (!rightchild->CompileToC(0, out_buffer, max_size))
    {
        return (false);
    }

    // -- if this is a parameter assignment, and we have another parameter, output the comma separator
    if (is_param_assignment && next != nullptr)
    {
        if (!OutputToBuffer(0, out_buffer, max_size, ", "))
            return (false);
    }

    return (true);
}

// == class CUnaryOpNode ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CUnaryOpNode::CUnaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                           eUnaryOpType _unaryoptype)
    : CCompileTreeNode(_codeblock, _link, eUnaryOp, _linenumber)
{
	unaryopcode = GetUnaryOpInstructionType(_unaryoptype);
    m_unaryOpType = _unaryoptype;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CUnaryOpNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CUnaryOpNode with no left child\n");
		return (-1);
	}

    // -- pre inc/dec operations are assignments - we need to ensure the left branch resolves to a variable
    eVarType resultType = pushresult;
    if (unaryopcode == OP_UnaryPreInc || unaryopcode == OP_UnaryPreDec)
        resultType = TYPE__var;

	// -- evaluate the left child, pushing the result of the type required
	// -- except in the case of an assignment operator - the left child is the variable
    int32 tree_size = leftchild->Eval(instrptr, resultType, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	// -- push the specific operation to be performed
	size += PushInstruction(countonly, instrptr, unaryopcode, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CUnaryOpNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- output the unary operator string
    const char* unary_op_name = GetUnaryOperatorString(m_unaryOpType);
    if (!OutputToBuffer(root_node ? indent : 0, out_buffer, max_size, unary_op_name))
        return (false);

    // -- compile the left child
    if (!leftchild->CompileToC(0, out_buffer, max_size, false))
        return (false);

    return (true);
}

// == class CLoopJumpNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CLoopJumpNode::CLoopJumpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
    CCompileTreeNode* loop_node, bool8 is_break)
    : CCompileTreeNode(_codeblock, _link, eLoopJump, _linenumber)
{
    mIsBreak = is_break;
    mJumpInstr = NULL;
    mJumpOffset = NULL;

    // -- notify the loop node that this node is jumping within
    if (loop_node->GetType() == eWhileLoop)
    {
        ((CWhileLoopNode*)loop_node)->AddLoopJumpNode(this);
    }
    else if (loop_node->GetType() == eSwitchStmt)
    {
        ((CSwitchStatementNode*)loop_node)->AddLoopJumpNode(this);
    }
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CLoopJumpNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    Unused_(pushresult);

    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- we'll need to calculate the offset, based on where we are now
    // -- push the branch instruction
    if (!countonly)
        mJumpInstr = instrptr;
    size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr);

    // -- cache the location of the offset - we'll fill it in after the while node has finished compiling
    // -- push a placeholder in the meantime
    if (!countonly)
        mJumpOffset = instrptr;
    uint32 empty = 0;
    size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");
    return size;
}

// ====================================================================================================================
// NotifyLoopInstr():  Fill in the jump offset to the start/end of a loop when a break/continue is executed
// ====================================================================================================================
void CLoopJumpNode::NotifyLoopInstr(uint32* continue_instr, uint32* break_instr)
{
    // -- ensure we have valid loop start and end instructions
    if ((!mIsBreak && !continue_instr) || (mIsBreak && !break_instr) || !mJumpInstr || !mJumpOffset)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
            "Error - NotifyLoopInstr(): invalid offsets\n");
        return;
    }

    // -- pick which instruction we're jumping to
    uint32* next_instr = mIsBreak ? break_instr : continue_instr;

    // -- if the instruction we're jumping to is *before* our current instruction, we'll have a negative jump,
    // -- and we'll add 2, to jump before the OP_BRANCH itself.
    if (kPointerToUInt32(next_instr) <= kPointerToUInt32(mJumpInstr))
    {
        int32 jump_offset = (int32)(kPointerDiffUInt32(mJumpInstr, next_instr)) >> 2;
        jump_offset += 2;

        // -- fill in the offset
        *mJumpOffset = -jump_offset;
    }
    else
    {
        int32 jump_offset = (int32)(kPointerDiffUInt32(next_instr, mJumpInstr)) >> 2;
        jump_offset -= 2;
        if (jump_offset < 0)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                "Error - NotifyLoopInstr(): invalid offsets\n");
            return;
        }

        // -- fill in the offset
        *mJumpOffset = jump_offset;
    }
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CLoopJumpNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CLoopJumpNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CCaseStatementNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCaseStatementNode::CCaseStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eCaseStmt, _linenumber)
{
    // -- initialize the members
    m_isDefaultCase = false;
    m_branchOffset = nullptr;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CCaseStatementNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
    return (0);
}

// ====================================================================================================================
// EvalCondition():  Case Statements build a table of comparisons and jumps, so they evaluate out of sequence.
// ====================================================================================================================
int32 CCaseStatementNode::EvalCondition(uint32*& instrptr, bool countonly)
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- ensure we have a left child
    if (!leftchild && !m_isDefaultCase)
    {
        TinPrint(TinScript::GetContext(), "Error - CSwitchStatementNode with no left child\n");
        return (-1);
    }

    // -- if this is the default case, the "condition" is already true - we don't need to do anything
    // -- otherwise, we'll push a duplicate of the top of the stack, which at this point is the switch value
    if (!m_isDefaultCase)
    {
        size += PushInstruction(countonly, instrptr, OP_PushCopy, DBG_instr);

        // -- then evaluate the left child, resolves to this case's value (must be of type int)
        size += leftchild->Eval(instrptr, TYPE_int, countonly);

        // -- perform a comparison - pops the value, and the copy of the switch value, pushes the bool result
        size += PushInstruction(countonly, instrptr, OP_CompareEqual, DBG_instr);

        // -- if the comparison is equal, we want to pop the original switch value, and then jump
        // -- therefore, if not equal, we want to skip over those instructions
        size += PushInstruction(countonly, instrptr, OP_BranchCond, DBG_instr);
        size += PushInstruction(countonly, instrptr, 0, DBG_value, "branch false");
        size += PushInstruction(countonly, instrptr, 0, DBG_value, "not a short_circuit branch");

        // -- we're jumping over a pop, and another jump
        // -- a pop instruction is one, branch take 2x instructions, so 3 total
        size += PushInstruction(countonly, instrptr, 3, DBG_value, "branch over case value");

        // -- pop the switch value
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr, "switch value");
        size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr, "branch to case statement");

        // -- if this isn't just calculating the count, cache the location for the branch offset
        if (!countonly)
            m_branchOffset = instrptr;

        size += PushInstruction(countonly, instrptr, 0, DBG_NULL, "placeholder for branch");
    }

    // -- return the size
    return (size);
}

// ====================================================================================================================
// EvalStatements():  Evaluate the statement block for the case.
// ====================================================================================================================
int32 CCaseStatementNode::EvalStatements(uint32*& instrptr, bool countonly)
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- all case statement nodes have a branch offset - now is when we fill it in
    if (!countonly)
    {
        // -- note:  when reading the branch offset, we increment, so the actual offset is diff - 1
        int32 branch_offset = (int32)(kPointerDiffUInt32(instrptr, m_branchOffset)) >> 2;
        *m_branchOffset = branch_offset - 1;
    }

    // -- ensure we have a right child
    if (rightchild)
    {
        size += rightchild->Eval(instrptr, TYPE_void, countonly);
    }

    // -- return the size
    return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CCaseStatementNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CCaseStatementNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CSwitchStatementNode ======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSwitchStatementNode::CSwitchStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eSwitchStmt, _linenumber)
{
    // -- we need to cache the default node, so it is compiled last (the final 'else')
    m_defaultNode = nullptr;
    mLoopJumpNodeCount = 0;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CSwitchStatementNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- ensure we have a left child
    if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CSwitchStatementNode with no left child\n");
        return (-1);
    }

    // -- ensure we have a right child
    if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - Switch Statement with no cases\n");
        return (-1);
    }

    // -- evaluate the left child, pushing the comparison value onto the stack
    int32 tree_size = leftchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- this is unusual, in that we evaluate all of the left children of each of the case nodes
    // -- to create the list of comparison-jump instructions
    CCompileTreeNode* next_node = rightchild;
    while (next_node != nullptr)
    {
        // -- if the case_node is a cast statement node, evaluate its left child
        if (next_node->GetType() == eCaseStmt)
        {
            CCaseStatementNode* case_node = (CCaseStatementNode*)next_node;
            size += case_node->EvalCondition(instrptr, countonly);
        }

        // -- get the next node
        next_node = next_node->next;
    }

    // -- at this point, we've compiled the "jump table" instructions
    // -- however, if none of the cases matched, then we'll need to pop the switch value back off
    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr, "pop unmatched switch value");

    // -- if we have a default case, then we jump to wherever the default instructions start
    uint32* no_default_branch = nullptr;
    if (m_defaultNode != nullptr)
    {
        size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr, "default case branch");

        // -- store default the branch offset instruction, since this is what we'll need to fill in
        if (!countonly)
            m_defaultNode->SetDefaultOffsetInstr(instrptr);

        size += PushInstruction(countonly, instrptr, 0, DBG_NULL, "placeholder for default branch");
    }

    //-- otherwise, no default instruction - we need to jump to the end of the switch statement instructions
    // -- the same place the break nodes jump to
    else
    {
        size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr, "no default branch");
        if (!countonly)
            no_default_branch = instrptr;
        size += PushInstruction(countonly, instrptr, 0, DBG_NULL, "placeholder for branch");
    }

    // -- now we loop through the case nodes, and compile their instructions...
    // -- as we do so, we fill in the jump offsets
    // -- note:  by compiling all comparisons with jumps, then compiling all instructions into one
    // -- big block statement, we support fallthrough
    next_node = rightchild;
    while (next_node != nullptr)
    {
        // -- if the case_node is a cast statement node, evaluate its left child
        if (next_node->GetType() == eCaseStmt)
        {
            CCaseStatementNode* case_node = (CCaseStatementNode*)next_node;
            size += case_node->EvalStatements(instrptr, countonly);
        }

        // -- get the next node
        next_node = next_node->next;
    }

    // -- this is the end of body of the swtich statement - mark the instruction pointer
    // -- so continue and break nodes can jump correctly
    if (!countonly)
    {
        // -- now that we've completed compiling the while loop, go through all break
        // -- nodes that jump out of their case
        for (int32 i = 0; i < mLoopJumpNodeCount; ++i)
        {
            mLoopJumpNodeList[i]->NotifyLoopInstr(nullptr, instrptr);
        }

        // -- if we have no default case, we also need to set the no_default branch
        if (no_default_branch != nullptr)
        {
            int32 jump_offset = (int32)(kPointerDiffUInt32(instrptr, no_default_branch)) >> 2;
            jump_offset -= 1;
            *no_default_branch = jump_offset;
        }
    }

    // -- return the size
    return (size);
}

// ====================================================================================================================
// SetDefaultNode():  Set the node for the default case - returns false if we already have one.
// ====================================================================================================================
bool8 CSwitchStatementNode::SetDefaultNode(CCaseStatementNode* default_node)
{
    // -- if we already have a default node, we're done
    if (m_defaultNode != nullptr)
        return (false);

    m_defaultNode = default_node;
    return (true);
}

// ====================================================================================================================
// AddLoopJumpNode():  Adds a jump node to the list belonging to a loop, so the beginning/end offset can be set.
// ====================================================================================================================
bool8 CSwitchStatementNode::AddLoopJumpNode(CLoopJumpNode* jump_node)
{
    if (mLoopJumpNodeCount >= kMaxLoopJumpCount || !jump_node)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, "<internal>", -1,
                      "Error - AddLoopJumpNode() in file: %s\n", codeblock->GetFileName());
        return (false);
    }

    mLoopJumpNodeList[mLoopJumpNodeCount++] = jump_node;
    return (true);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CSwitchStatementNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CSwitchStatementNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CIfStatementNode ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CIfStatementNode::CIfStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eIfStmt, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CIfStatementNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CIfStatementNode with no left child\n");
		return (-1);
	}

	// -- ensure we have a right child
	if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CIfStatementNode with no right child\n");
		return (-1);
	}

	// -- evaluate the left child, which is the condition
    int32 tree_size = leftchild->Eval(instrptr, TYPE_bool, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	// -- evaluate the right child, which is the branch node
    // note:  if used as an actual 'if', the pushresult will be void
    // -- otherwise, if it's a ternary op, it *might* require a non-void result
    tree_size = rightchild->Eval(instrptr, pushresult, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CIfStatementNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CIfStatementNode with no left child\n");
		return (false);
	}

	// -- ensure we have a right child
	if (!rightchild || rightchild->GetType() != eCondBranch || rightchild->leftchild == nullptr)
    {
		TinPrint(TinScript::GetContext(), "Error - CIfStatementNode with invald right child\n");
		return (false);
	}

    // -- output the 'if' statement
    if (!OutputToBuffer(indent, out_buffer, max_size, "if ("))
        return (false);

    // -- output the condition
    if (!leftchild->CompileToC(0, out_buffer, max_size, false))
        return (false);

    // -- close the condition
    if (!OutputToBuffer(0, out_buffer, max_size, ")\n"))
        return (false);

    // -- open the statement block
    if (!OutputToBuffer(indent, out_buffer, max_size, "{\n"))
        return (false);

    // -- Compile to C, the statement block
    if (!rightchild->leftchild->CompileToC(indent + 1, out_buffer, max_size, true))
        return (false);

    // -- close the statement block
    if (!OutputToBuffer(indent, out_buffer, max_size, "\n}\n"))
        return (false);

    // -- see if there's an else statment block
    if (rightchild->rightchild != nullptr)
    {
        // -- output the else and the statment block open
        if (!OutputToBuffer(indent, out_buffer, max_size, "else\n"))
            return (false);

        if (!OutputToBuffer(indent, out_buffer, max_size, "{\n"))
            return (false);

        // -- Compile the else statement block
        if (!rightchild->rightchild->CompileToC(indent + 1, out_buffer, max_size, true))
            return (false);

        // -- close the else statement block
        if (!OutputToBuffer(indent, out_buffer, max_size, "\n}\n"))
            return (false);
    }

    // -- add a newline after an if statement
    if (!OutputToBuffer(indent, out_buffer, max_size, "\n"))
        return (false);

    // success
    return (true);
}

// == class CCondBranchNode ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCondBranchNode::CCondBranchNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eCondBranch, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CCondBranchNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- left child is if the stacktop contains the result of a conditional
    // -- so we branchm,if the condition is 'false'
	size += PushInstruction(countonly, instrptr, OP_BranchCond, DBG_instr);
    size += PushInstruction(countonly, instrptr, 0, DBG_value, "branch false");
    size += PushInstruction(countonly, instrptr, 0, DBG_value, "not a short_circuit branch");

	// -- cache the current intrptr, because we'll need to how far to
	// -- jump, after we've evaluated the left child
	// -- push a placeholder in the meantime
	uint32* branchwordcount = instrptr;
	uint32 empty = 0;
	size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");

	// -- if we have a left child, this is the 'true' tree
	if (leftchild)
    {
		int32 cursize = size;

        int32 tree_size = leftchild->Eval(instrptr, pushresult, countonly);
        if (tree_size < 0)
            return (-1);
        size += tree_size;

		// -- the size of the leftchild is how many instructions to jump, should the
		// -- branch condition be false - but add two, since the end of the 'true'
		// -- tree will have to jump the 'false' section
        if (!countonly)
        {
		    int32 jumpcount = size - cursize;
		    if (rightchild)
			    jumpcount += 2;
		    *branchwordcount = jumpcount;
        }
	}

	// -- the right tree is the 'false' tree
	if (rightchild)
    {
		// -- start with adding a branch at the end of the 'true' section
		size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr);
		branchwordcount = instrptr;
		size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");

		// now evaluate the right child, tracking it's size
		int32 cursize = size;

        int32 tree_size = rightchild->Eval(instrptr, pushresult, countonly);
        if (tree_size < 0)
            return (-1);
        size += tree_size;

		// fill in the jumpcount
        if (!countonly)
        {
		    int32 jumpcount = size - cursize;
		    *branchwordcount = jumpcount;
        }
	}

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CCondBranchNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CCondBranchNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CWhileLoopNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CWhileLoopNode::CWhileLoopNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool8 is_do_while)
    : CCompileTreeNode(_codeblock, _link, eWhileLoop, _linenumber)
{
    mEndOfLoopNode = NULL;
    mContinueHereInstr = NULL;
    mBreakHereInstr = NULL;
    mLoopJumpNodeCount = 0;
    m_isDoWhile = is_do_while;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CWhileLoopNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CWhileLoopNode with no left child\n");
		return (-1);
	}

	// -- ensure we have a right child
	if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CWhileLoopNode with no right child\n");
		return (-1);
	}

    // -- if this is a do..while loop, then the first instruction we push, is to skip the conditional,
    // -- so the body is run at least once
    uint32 empty = 0;
    int32 do_while_jump_count = 0;
    uint32* do_while_branch = nullptr;
    if (m_isDoWhile)
    {
        size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr);
        do_while_branch = instrptr;
        size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for do-while branch");
    }

    // -- this is the start of the condition for the loop - mark the instruction pointer
    // -- so continue and break nodes can jump correctly
    if (!countonly)
    {
        // -- if we don't have an end of loop node, then if we hit a "continue" statement, we jump here
        if (!mEndOfLoopNode)
            mContinueHereInstr = instrptr;
    }

	// the instruction at the start of the leftchild is where we begin each loop
	// -- evaluate the left child, which is the condition
    int32 tree_size = leftchild->Eval(instrptr, TYPE_bool, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	// -- add a BranchFalse here, to skip the body of the while loop
	size += PushInstruction(countonly, instrptr, OP_BranchCond, DBG_instr);
    size += PushInstruction(countonly, instrptr, 0, DBG_value, "branch false");
    size += PushInstruction(countonly, instrptr, 0, DBG_value, "not a short_circuit branch");

	uint32* branchwordcount = instrptr;
	size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");

    // -- we don't want to branch all the way to skipping the conditional
    int32 cursize = size;

    // -- if this is a do-while loop, this is where we want to initially jump to
    if (!countonly && m_isDoWhile)
    {
        // -- the count is the current size, minus the branch instruction itself
        *do_while_branch = (size - 2);
    }

	// -- evaluate the right child, which is the body of the while loop
    tree_size = rightchild->Eval(instrptr, TYPE_void, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- continue statements need to jump to the end of the loop body, but still evaluate the end of loop statement
    // -- e.g.  continuing within a 'for' loop, skips the body, but executes the end of loop statement(s)
    if (!countonly)
    {
        // -- a "continue" statement jumps to the end of loop node
        if (mEndOfLoopNode)
            mContinueHereInstr = instrptr;
    }

    // -- there may be an end of loop node (for loops use this, for example)
    if (mEndOfLoopNode)
    {
        tree_size = mEndOfLoopNode->Eval(instrptr, TYPE_void, countonly);
        if (tree_size < 0)
            return (-1);
        size += tree_size;
    }

	// -- after the body of the while loop has been executed, we want to jump back
	// -- to the top and evaluate the condition again
    // note:  in a do-while, we want to jump to the conditional, not to the initial branch
    // note: + 2 is to account for the actual jump itself
	int32 jumpcount = m_isDoWhile ? -size : -(size + 2);
	size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr);
	size += PushInstruction(countonly, instrptr, (uint32)jumpcount, DBG_NULL);

	// fill in the top jumpcount, which is to skip the while loop body if the condition is false
    if (!countonly)
    {
	    jumpcount = size - cursize;
	    *branchwordcount = jumpcount;
    }

    // -- this is the end of body of the loop - mark the instruction pointer
    // -- so continue and break nodes can jump correctly
    if (!countonly)
    {
        // -- break instructions jump to the very end, past the end of loop 
        mBreakHereInstr = instrptr;

        // -- now that we've completed compiling the while loop, go through all break/continue
        // -- nodes that jump out of this loop
        for (int32 i = 0; i < mLoopJumpNodeCount; ++i)
        {
            mLoopJumpNodeList[i]->NotifyLoopInstr(mContinueHereInstr, mBreakHereInstr);
        }
    }

	return size;
}

// ====================================================================================================================
// AddLoopJumpNode():  Adds a jump node to the list belonging to a loop, so the beginning/end offset can be set.
// ====================================================================================================================
bool8 CWhileLoopNode::AddLoopJumpNode(CLoopJumpNode* jump_node)
{
    if (mLoopJumpNodeCount >= kMaxLoopJumpCount || !jump_node)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, "<internal>", -1,
                      "Error - AddLoopJumpNode() in file: %s\n", codeblock->GetFileName());
        return (false);
    }

    mLoopJumpNodeList[mLoopJumpNodeCount++] = jump_node;
    return (true);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CWhileLoopNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CWhileLoopNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CForeachLoopNode ============================================================================================

// ====================================================================================================================
// Constructor:  Used to evaluate the container expression, then set up an iterator of the correct type
// ====================================================================================================================
CForeachLoopNode::CForeachLoopNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       const char* _iter_name, int32 _iter_length)
    : CCompileTreeNode(_codeblock, _link, eForeachLoop, _linenumber)
{
	SafeStrcpy(mIteratorVar, sizeof(mIteratorVar), _iter_name, _iter_length + 1);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CForeachLoopNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- ensure we have a left child
    if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CWhileLoopNode with no left child\n");
        return (-1);
    }

    // -- ensure we have a right child
    if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CWhileLoopNode with no right child\n");
        return (-1);
    }

    // the left child is the branch that resolves (for now) to an array variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- next we push the iterator variable (member, global, stack var, etc...)
    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    CFunctionEntry* curfunction = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);

    // -- ensure we can find the variable
    uint32 varhash = Hash(mIteratorVar);
    uint32 funchash = curfunction ? curfunction->GetHash() : 0;
    uint32 nshash = curfunction ? curfunction->GetNamespaceHash() : CScriptContext::kGlobalNamespaceHash;
    CVariableEntry* var = GetVariable(codeblock->GetScriptContext(), codeblock->smCurrentGlobalVarTable,
        nshash, funchash, varhash, 0);
    if (!var || var->IsArray() || var->GetType() == TYPE_hashtable)
    {
        ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                      "Error - undefined or invalid iterator variable: %s\n", mIteratorVar);
        return (-1);
    }
    eVarType vartype = var->GetType();

    // -- if this isn't a func var, make sure we push the global namespace
    if (var->GetFunctionEntry() == NULL)
    {
        size += PushInstruction(countonly, instrptr, OP_PushGlobalVar, DBG_instr);
        size += PushInstruction(countonly, instrptr, CScriptContext::kGlobalNamespaceHash, DBG_hash);
        size += PushInstruction(countonly, instrptr, 0, DBG_func);
        size += PushInstruction(countonly, instrptr, var->GetHash(), DBG_var);
    }
    // -- otherwise this is a stack var
    else
    {
        size += PushInstruction(countonly, instrptr, OP_PushLocalVar, DBG_instr);
        size += PushInstruction(countonly, instrptr, var->GetType(), DBG_vartype);

        // -- for local vars, it's the offset on the stack we need to push
        int32 stackoffset = var->GetStackOffset();
        if (!countonly && stackoffset < 0)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                "Error - invalid stack offset for local var: %s\n", UnHash(var->GetHash()));
            return (-1);
        }
        size += PushInstruction(countonly, instrptr, stackoffset, DBG_var);

        // -- push the local var index as well
        int32 var_index = 0;
        CVariableEntry* local_ve = var->GetFunctionEntry()->GetLocalVarTable()->First();
        while (local_ve && local_ve != var)
        {
            local_ve = var->GetFunctionEntry()->GetLocalVarTable()->Next();
            ++var_index;
        }
        size += PushInstruction(countonly, instrptr, var_index, DBG_var);
    }

    // -- we need to push the initial iterator var assignment
    size += PushInstruction(countonly, instrptr, OP_ForeachIterInit, DBG_instr);

    // -- finally, we can simply use our while loop
    tree_size = rightchild->Eval(instrptr, TYPE_void, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- after the while loop exits, we need to pop the container, iterator variables and index off the stack
    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);

    return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CForeachLoopNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CForeachLoopNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CForeachIterNext =========================================================================================

// ====================================================================================================================
// Constructor:  Used to evaluate the container expression, then set up an iterator of the correct type
// ====================================================================================================================
CForeachIterNext::CForeachIterNext(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eForeachIterNext, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CForeachIterNext::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- we don't need to do much - simply issue the op instruction
    size += PushInstruction(countonly, instrptr, OP_ForeachIterNext, DBG_instr);

    return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CForeachIterNext::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CForeachIterNext::CompileToC() not implemented.\n");
    return (true);
}

// == class CParenOpenNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CParenOpenNode::CParenOpenNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eWhileLoop, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CParenOpenNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const {
    Unused_(pushresult);

	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CParenOpenNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CParenOpenNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CFuncDeclNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFuncDeclNode::CFuncDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                             const char* _funcname, int32 _length, const char* _funcns,
                             int32 _funcnslength, uint32 derived_ns)
    : CCompileTreeNode(_codeblock, _link, eFuncDecl, _linenumber)
{
    SafeStrcpy(funcname, sizeof(funcname), _funcname, _length + 1);
    SafeStrcpy(funcnamespace, sizeof(funcnamespace), _funcns, _funcnslength + 1);

    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    functionentry = codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
    mDerivedNamespace = derived_ns;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CFuncDeclNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    // -- get the function entry
	uint32 funchash = Hash(funcname);

    // -- if we're using a namespace, find the function entry from there
    uint32 funcnshash = funcnamespace[0] != '\0' ? Hash(funcnamespace) : 0;
    tFuncTable* functable = NULL;
    if (funcnshash != 0)
    {
        CNamespace* nsentry = codeblock->GetScriptContext()->FindOrCreateNamespace(funcnamespace);
        if (!nsentry)
        {
            ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                          "Error - Failed to find/create Namespace: %s\n", funcnamespace);
            return (-1);
        }

        functable = nsentry->GetFuncTable();
    }
    else
	    functable = codeblock->GetScriptContext()->GetGlobalNamespace()->GetFuncTable();

	CFunctionEntry* fe = functable->FindItem(funchash);
	if (!fe)
    {
		ScriptAssert_(codeblock->GetScriptContext(), 0, codeblock->GetFileName(), linenumber,
                      "Error - undefined function: %s\n", funcname);
		return (-1);
	}

    // -- set the current function definition
    codeblock->smFuncDefinitionStack->Push(fe, NULL, 0);

    eVarType returntype = fe->GetReturnType();

    // -- recreate the function entry - first the instruction 
    size += PushInstruction(countonly, instrptr, OP_FuncDecl, DBG_instr);

    // -- function hash
    size += PushInstruction(countonly, instrptr, fe->GetHash(), DBG_func);

    // -- push the function namespace hash
    size += PushInstruction(countonly, instrptr, funcnshash, DBG_hash);

    // -- after we declare the function namespace, specify the derived namespace (only ever valid for OnCreate())
    size += PushInstruction(countonly, instrptr, mDerivedNamespace, DBG_hash); 

    // -- push the function offset placeholder
	uint32* funcoffset = instrptr;
	uint32 empty = 0;
	size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for func offset");

    // -- function context - parameters + local vartable
    size += CompileFunctionContext(fe, instrptr, countonly);

    // -- need to complete the function declaration
    size += PushInstruction(countonly, instrptr, OP_FuncDeclEnd, DBG_instr);

    // -- we want to skip over the entire body, as it's not for immediate execution
    size += PushInstruction(countonly, instrptr, OP_Branch, DBG_instr);
	uint32* branchwordcount = instrptr;
	size += PushInstructionRaw(countonly, instrptr, (void*)&empty, 1, DBG_NULL, "placeholder for branch");
    int32 cursize = size;

    // -- we're now at the start of the function body
    if (!countonly)
    {
        // -- fill in the missing offset
        uint32 offset = codeblock->CalcOffset(instrptr);

        // -- note, there's a possibility we're stomping a registered code function here
        if (fe->GetType() != eFuncTypeScript)
        {
            ScriptAssert_(codeblock->GetScriptContext(), false, codeblock->GetFileName(), linenumber,
                          "Error - there is already a C++ registered function %s()\n"
                          "Removing %s() - re-Exec() to redefine\n", fe->GetName(), fe->GetName());

            // -- delete the function entirely - re-executing the script will redefine
            // -- it with the (presumably) updated signature
            functable->RemoveItem(funchash);
            TinFree(fe);

            return (-1);
        }

        fe->SetCodeBlockOffset(codeblock, offset);
        *funcoffset = offset;
    }

    // -- before the function body, we need to dump out the dictionary of local vars
    size += CompileVarTable(fe->GetLocalVarTable(), instrptr, countonly);

    // -- compile the function body (unless this is just a prototype)
    if (leftchild != nullptr)
    {
        int32 tree_size = leftchild->Eval(instrptr, returntype, countonly);
        if (tree_size < 0)
            return (-1);
        size += tree_size;
    }

    // -- fill in the jumpcount
    if (!countonly)
    {
        int32 jumpcount = size - cursize;
	    *branchwordcount = jumpcount;
    }

    // -- clear the current function definition
    CObjectEntry* dummy = NULL;
    int32 var_offset = 0;
    codeblock->smFuncDefinitionStack->Pop(dummy, var_offset);

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CFuncDeclNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, funcname: %s", gCompileNodeTypes[type], funcname);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CFuncDeclNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- output the function signature
    const char* return_type_name =
        TinScript::GetRegisteredTypeName(functionentry->GetContext()->GetParameter(0)->GetType());
    if (!OutputToBuffer(indent, out_buffer, max_size, "%s %s(", return_type_name, funcname))
        return (false);

    // -- output the signature
    for (int i = 1; i < functionentry->GetContext()->GetParameterCount(); ++i)
    {
        // -- add the comma separator
        if (i > 1 && !OutputToBuffer(0, out_buffer, max_size, ", "))
            return (false);

        const char* arg_type_name =
            TinScript::GetRegisteredTypeName(functionentry->GetContext()->GetParameter(i)->GetType());
        const char* arg_name = functionentry->GetContext()->GetParameter(i)->GetName();
        if (!OutputToBuffer(0, out_buffer, max_size, "%s %s", arg_type_name, arg_name))
            return (false);
    }

    // -- close the function declaration
    if (!OutputToBuffer(0, out_buffer, max_size, ")"))
        return (false);

    // -- if we don't have a left child, this is a forward declaration
    if (leftchild == nullptr)
    {
        if (!OutputToBuffer(0, out_buffer, max_size, ";\n"))
            return (false);
    }

    // -- output the function body
    else
    {
        // -- open statement block 
        if (!OutputToBuffer(indent, out_buffer, max_size, "\n{\n"))
            return (false);

        // -- declare all local variables
        bool first_local_var = true;
        CVariableEntry* local_var = functionentry->GetLocalVarTable()->First();
        while (local_var != nullptr)
        {
            const char* arg_type_name = TinScript::GetRegisteredTypeName(local_var->GetType());
            const char* arg_name = local_var->GetName();
            if (!local_var->IsParameter())
            {
                // -- output the comment
                if (first_local_var)
                {
                    // -- set the flag and output the comment
                    first_local_var = false;
                    if (!OutputToBuffer(1, out_buffer, max_size, "// -- local vars -- //\n"))
                        return (false);
                }

                // -- output the local var declaration
                if (!OutputToBuffer(1, out_buffer, max_size, "%s %s;\n", arg_type_name, arg_name))
                    return (false);
            }

            // -- next local var
            local_var = functionentry->GetLocalVarTable()->Next();
        }

        // -- if we output any local vars, add a space
        if (!first_local_var)
        {
            if (!OutputToBuffer(0, out_buffer, max_size, "\n"))
                return (false);
        }

        // -- output the comment
        if (!OutputToBuffer(1, out_buffer, max_size, "// -- function implementation -- //\n"))
            return (false);

        // -- output the function body instructions
        if (!leftchild->CompileToC(indent + 1, out_buffer, max_size, true))
            return (false);

        // -- close statement block 
        if (!OutputToBuffer(indent, out_buffer, max_size, "}\n"))
            return (false);
    }

    // -- success
    return (true);
}

// == class CFuncCallNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFuncCallNode::CFuncCallNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
    const char* _funcname, int32 _length, const char* _nsname,
    int32 _nslength, EFunctionCallType call_type)
    : CCompileTreeNode(_codeblock, _link, eFuncCall, _linenumber)
{
    SafeStrcpy(funcname, sizeof(funcname), _funcname, _length + 1);
    SafeStrcpy(nsname, sizeof(nsname), _nsname, _nslength + 1);
    mCallType = call_type;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CFuncCallNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- get the function/method hash
    uint32 funchash = Hash(funcname);
    uint32 nshash = Hash(nsname);

    // -- we need a valid call type by this point
    if (mCallType <= EFunctionCallType::None || mCallType >= EFunctionCallType::Count)
    {
        assert(false && "CFuncCallNode with no valid call type");
        return (false);
    }

    // -- first we push the function to the call stack
    // -- for methods, we want to find the method searching from the top of the objects hierarchy
    if (mCallType == EFunctionCallType::ObjMethod)
    {
        size += PushInstruction(countonly, instrptr, OP_MethodCallArgs, DBG_instr);
        size += PushInstruction(countonly, instrptr, 0, DBG_nshash);
        size += PushInstruction(countonly, instrptr, 0, DBG_super); // unused
    }
    // note:  a namespaced function call has a similar syntax as global function call
    // e.g.  it's not obj.method(), but XXX::Method()...  no "object."  (including super)
    else if (mCallType == EFunctionCallType::Global || mCallType == EFunctionCallType::Super)
    {
        // -- if this isn't a method, but we specified a namespace, then it's a
        // method from a specific namespace in an object's hierarchy.
        // -- PushSelf, since this will have been called via NS::Func() instead of obj.Func();
        // -- if this is a 'super::method()' call, then the nshash is actually the current
        // namespace, and we're looking for the function defined for an ancestor
        if (nshash != 0)
        {
            size += PushInstruction(countonly, instrptr, OP_PushSelf, DBG_self);
            size += PushInstruction(countonly, instrptr, OP_MethodCallArgs, DBG_instr);
            size += PushInstruction(countonly, instrptr, nshash, DBG_nshash);
            size += PushInstruction(countonly, instrptr, mCallType == EFunctionCallType::Super ? 1 : 0, DBG_super);
        }
        else
        {
            size += PushInstruction(countonly, instrptr, OP_FuncCallArgs, DBG_instr);
            size += PushInstruction(countonly, instrptr, nshash, DBG_nshash);
        }
    }

    // -- POD method call
    else if (mCallType == EFunctionCallType::PODMethod)
    {
        // -- we need to push the POD value onto the stack
        size += PushInstruction(countonly, instrptr, OP_PODCallArgs, DBG_instr);
    }

    size += PushInstruction(countonly, instrptr, funchash, DBG_func);

    // -- then evaluate all the argument assignments
    int32 tree_size = leftchild->Eval(instrptr, TYPE_void, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- then call the function
    size += PushInstruction(countonly, instrptr, OP_FuncCall, DBG_instr);

    // -- if we're not looking for a return value
    if (mCallType != EFunctionCallType::PODMethod && pushresult <= TYPE_void)
    {
        // -- all functions will return a value - by default, a "" for void functions
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    }

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CFuncCallNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, funcname: %s", gCompileNodeTypes[type], funcname);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CFuncCallNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- output the function name, and the opening parenthesis
    // -- note:  if this is a method, preceed it by a '.'
    // $$TZA fixme - this is probably broken, since super::x(), and namespace::x() are both parsed as a global function
    if (!OutputToBuffer(indent, out_buffer, max_size, "%s%s(", mCallType != EFunctionCallType::Global ? "." : "", funcname))
        return (false);

    // -- the left child contains all the parameter assignments
    if (leftchild != nullptr)
    {
        if (!leftchild->CompileToC(0, out_buffer, max_size))
            return (false);
    }

    // -- output the closing parenthesis
    if (!OutputToBuffer(0, out_buffer, max_size, ")"))
        return (false);

    // -- output the parameter values
    return (true);
}

// == class CFuncReturnNode ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFuncReturnNode::CFuncReturnNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eFuncReturn, _linenumber)
{
    int32 stacktopdummy = 0;
    CObjectEntry* dummy = NULL;
    functionentry = _codeblock->smFuncDefinitionStack->GetTop(dummy, stacktopdummy);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CFuncReturnNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    // -- get the context, which will contain the return parameter (type)..
    assert(functionentry != NULL);
    CFunctionContext* context = functionentry->GetContext();
    assert(context != NULL && context->GetParameterCount() > 0);
    CVariableEntry* returntype = context->GetParameter(0);

    // -- all functions are required to return a value, to keep the virtual machine consistent
    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CFuncReturnNode::Eval() - invalid return from function %s()\n",
                      functionentry->GetName());
        return (-1);
    }

    int32 tree_size = 0;
    if (returntype->GetType() <= TYPE_void)
        tree_size = leftchild->Eval(instrptr, TYPE_int, countonly);
    else
        tree_size = leftchild->Eval(instrptr, returntype->GetType(), countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- finally, issue the function return instruction
	size += PushInstruction(countonly, instrptr, OP_FuncReturn, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CFuncReturnNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    // -- get the context, which will contain the return parameter (type)..
    assert(functionentry != NULL);
    CFunctionContext* context = functionentry->GetContext();
    assert(context != NULL && context->GetParameterCount() > 0);
    CVariableEntry* returntype = context->GetParameter(0);

    // -- if the return type is void, we're done
    if (returntype->GetType() <= TYPE_void)
    {
        if (!OutputToBuffer(indent, out_buffer, max_size, "return;"))
            return (false);
        
        return (true);
    }

    // -- output the return keyword, with the contents enclosed on parenthesis
    if (!OutputToBuffer(indent, out_buffer, max_size, "return ("))
        return (false);

    // -- compile the return expression
    if (!leftchild->CompileToC(0, out_buffer, max_size, false))
        return (false);

    // -- output the closing parenthesis
    if (!OutputToBuffer(0, out_buffer, max_size, ")"))
        return (false);

    return (true);
}

// == class CObjMethodNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjMethodNode::CObjMethodNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                               const char* _methodname, int32 _methodlength)
    : CCompileTreeNode(_codeblock, _link, eObjMethod, _linenumber)
{
	SafeStrcpy(methodname, sizeof(methodname), _methodname, _methodlength + 1);
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CObjMethodNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CObjMemberNode with no left child\n");
		return (-1);
	}

  	// -- evaluate the left child, pushing the a result of TYPE_object
    int32 tree_size = leftchild->Eval(instrptr, TYPE_object, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- evaluate the right child, which contains the function call node
    tree_size = rightchild->Eval(instrptr, pushresult, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CObjMethodNode::Dump(char*& output, int32& length) const
{
    snprintf(output, length, "type: %s, %s", gCompileNodeTypes[type], methodname);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CObjMethodNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CObjMethodNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayHashNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayHashNode::CArrayHashNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eArrayHash, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayHashNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayHashNode::Eval() - missing leftchild\n");
        return (-1);
    }

    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayHashNode::Eval() - missing rightchild\n");
        return (-1);
    }

   	// -- evaluate the left child, which pushes the "current hash", TYPE_int
    int32 tree_size = leftchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

   	// -- evaluate the right child, which pushes the next string to be hashed and appended
    tree_size = rightchild->Eval(instrptr, TYPE_string, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- if the right child happened to be an assignment, then we need to push the assign value back onto the stack
    if (rightchild->IsAssignOpNode())
    {
        // -- add the instruction to push the last assignment result before performing ours
        size += PushInstruction(countonly, instrptr, OP_PushAssignValue, DBG_instr, "consec assign");
    }

    // -- push an OP_ArrayHash, pops the top two stack items, the first is a "hash in progress",
    // -- and the second is a string to continue to add to the hash value
    // -- pushes the int32 hash result back onto the stack
	size += PushInstruction(countonly, instrptr, OP_ArrayHash, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayHashNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayHashNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayVarNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayVarNode::CArrayVarNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eArrayVar, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayVarNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayVarNode::Eval() - missing leftchild\n");
        return (-1);
    }

    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayVarNode::Eval() - missing rightchild\n");
        return (-1);
    }

   	// -- left child will have pushed the hashtable variable
    // $$$TZA Array - this is "unusual", but basically, the OpExecArrayHash(), will use the var
	// pushed on the stack, and will figure out then, whether it's a hashtable (using a key),
	// or it'll convert the key to an index... arguably this could use a small clarity refactor
    int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

   	// -- right child will contain the hash value or array index for the entry we're declaring
    tree_size = rightchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- if we're applying a post increment/decrement, we also need to push the post-op instruction
    if (m_unaryDelta != 0)
    {
        size += PushInstruction(countonly, instrptr, m_unaryDelta > 0 ? OP_UnaryPostInc : OP_UnaryPostDec, DBG_instr);
        size += PushInstruction(countonly, instrptr, 1, DBG_value, "array var");

        // -- in addition, if the value isn't actually going to be used, issue an immediate pop
        //if (pushresult == TYPE_void)
        //    size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr, "post unary op");
    }

    // -- see if we're supposed to be pushing a var (e.g. for an assign...)
    bool8 push_value = (pushresult != TYPE__var && pushresult != TYPE_hashtable && pushresult != TYPE_void);
    size += PushInstruction(countonly, instrptr, push_value ? OP_PushArrayValue : OP_PushArrayVar, DBG_instr);

    // -- if the return type is void, and we're performing a unary post op, then pop the array var back off
    if (pushresult == TYPE_void && m_unaryDelta != 0)
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr, "post unary op");

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayVarNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayVarNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayVarDeclNode =========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayVarDeclNode::CArrayVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                     eVarType _type)
    : CCompileTreeNode(_codeblock, _link, eArrayVarDecl, _linenumber)
{
    type = _type;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayVarDeclNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CArrayVarDeclNode::Eval() - missing leftchild\n");
        return (-1);
    }

    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CArrayVarDeclNode::Eval() - missing rightchild\n");
        return (-1);
    }

   	// -- left child will have pushed the hashtable variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

   	// -- right child will contain the hash value for the entry we're declaring
    tree_size = rightchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	size += PushInstruction(countonly, instrptr, OP_ArrayVarDecl, DBG_instr);
	size += PushInstruction(countonly, instrptr, type, DBG_vartype);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayVarDeclNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayVarDeclNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayDeclNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayDeclNode::CArrayDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 _size)
    : CCompileTreeNode(_codeblock, _link, eArrayDecl, _linenumber)
{
    mSize = _size;
    if (mSize < 1)
        mSize = 1;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayDeclNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayDeclNode::Eval() - missing leftchild\n");
        return (-1);
    }

    // $$$TZA Eventually, we may want dynamically sized arrays, in which case, the size is the right child
    /*
    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CArrayDeclNode::Eval() - missing rightchild\n");
        return (-1);
    }
    */

   	// -- left child will have pushed the variable that is to become an array
    int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

   	// -- right child will contain the size of the array
    /*
    tree_size = rightchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;
    */

    // -- push the size
	size += PushInstruction(countonly, instrptr, OP_Push, DBG_instr);
    size += PushInstruction(countonly, instrptr, TYPE_int, DBG_vartype);
	size += PushInstruction(countonly, instrptr, mSize, DBG_value);

    // -- push the instruction to convert the given variable to an array
    // -- note:  the variable will have been created, with a NULL mAddr, to be when this instruction is executed
	size += PushInstruction(countonly, instrptr, OP_ArrayDecl, DBG_instr);

    // -- success
    return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayDeclNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayDeclNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayCountNode ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayCountNode::CArrayCountNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eArrayCount, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayCountNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			linenumber,
			"Error - CArrayCountNode::Eval() - missing leftchild\n");
		return (-1);
	}


	// -- left child will have pushed the array variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to read the and push the size of the array
	size += PushInstruction(countonly, instrptr, OP_ArrayCount, DBG_instr);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayCountNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayCountNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CArrayContainsNode ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CArrayContainsNode::CArrayContainsNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eArrayContains, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CArrayContainsNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			linenumber,
			"Error - CArrayContainsNode::Eval() - missing leftchild\n");
		return (-1);
	}

	// -- left child will have pushed the array variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

    // -- right child will have pushed a value to compare
	tree_size = rightchild->Eval(instrptr, TYPE__resolve, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to read the and push true if the array contains the value
	size += PushInstruction(countonly, instrptr, OP_ArrayContains, DBG_instr);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CArrayContainsNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CArrayContainsNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CMathUnaryFuncNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CMathUnaryFuncNode::CMathUnaryFuncNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                       eMathUnaryFunctionType math_func_type)
	: CCompileTreeNode(_codeblock, _link, eMathUnaryFunc, _linenumber)
{
    mFuncType = math_func_type;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CMathUnaryFuncNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			linenumber,
			"Error - CMathUnaryFuncNode::Eval() - missing leftchild\n");
		return (-1);
	}

	// -- left child will have pushed the array variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE_float, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to read the and push the float result
	size += PushInstruction(countonly, instrptr, OP_MathUnaryFunc, DBG_instr);
    size += PushInstruction(countonly, instrptr, mFuncType, DBG_instr);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CMathUnaryFuncNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CMathUnaryFuncNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CMathBinaryFuncNode =======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CMathBinaryFuncNode::CMathBinaryFuncNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                         eMathBinaryFunctionType math_func_type)
	: CCompileTreeNode(_codeblock, _link, eMathBinaryFunc, _linenumber)
{
    mFuncType = math_func_type;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CMathBinaryFuncNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			          linenumber,
			          "Error - CMathBinaryFuncNode::Eval() - missing leftchild\n");
		return (-1);
	}

    if (!rightchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
			          linenumber,
			          "Error - CMathBinaryFuncNode::Eval() - missing leftchild\n");
		return (-1);
	}

	// -- left child will have pushed the array variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE_float, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

    // -- left child will have pushed the array variable
	tree_size = rightchild->Eval(instrptr, TYPE_float, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to read the and push the float result
	size += PushInstruction(countonly, instrptr, OP_MathBinaryFunc, DBG_instr);
    size += PushInstruction(countonly, instrptr, mFuncType, DBG_instr);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CMathBinaryFuncNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CMathBInaryFuncNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CHashtableHasKey ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CHashtableHasKey::CHashtableHasKey(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eHashtableHasKey, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CHashtableHasKey::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CHashtableHasKey::Eval() - missing leftchild\n");
        return (-1);
    }

    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CHashtableHasKey::Eval() - missing rightchild\n");
        return (-1);
    }

   	// -- left child will have pushed the hashtable variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

   	// -- right child will contain the hash value that we're seeing if it exists
    tree_size = rightchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the "has key", which will pop the hashtable, and potential key, and push a bool if the key exists
    size += PushInstruction(countonly, instrptr, OP_HashtableHasKey, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CHashtableHasKey::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CHashtableHasKey::CompileToC() not implemented.\n");
    return (true);
}

// == class CHashtableContainsNode ====================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CHashtableContainsNode::CHashtableContainsNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eHashtableContains, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CHashtableContainsNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			linenumber,
			"Error - CHashtableContainsNode::Eval() - missing leftchild\n");
		return (-1);
	}

	if (!rightchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
			linenumber, "Error - CHashtableContainsNode::Eval() - missing rightchild\n");
		return (-1);
	}

	// -- left child will have pushed the hashtable variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

    // -- right child will have pushed a value to compare
	tree_size = rightchild->Eval(instrptr, TYPE__resolve, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to read the and push true if the value is found within the hashtable
	size += PushInstruction(countonly, instrptr, OP_HashtableContains, DBG_instr);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CHashtableContainsNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CHashtableContainsNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CHashtableCopyNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CHashtableCopyNode::CHashtableCopyNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool is_wrap)
	 : CCompileTreeNode(_codeblock, _link, eHashtableCopy, _linenumber)
    , mIsWrap(is_wrap)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CHashtableCopyNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	if (!leftchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
			          linenumber, "Error - CHashtableCopyNode::Eval() - missing leftchild\n");
		return (-1);
	}

	if (!rightchild)
	{
		ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(),
			          linenumber, "Error - CHashtableCopyNode::Eval() - missing rightchild\n");
		return (-1);
	}

	// -- left child will have pushed the hashtable variable
	int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
	if (tree_size < 0)
		return (-1);
	size += tree_size;

    // -- right child will have pushed either an internal hashtable, or a CHashtable object,
    // which we can use to pass to C++
    tree_size = rightchild->Eval(instrptr, TYPE_hashtable, countonly);
    if (tree_size < 0)
		return (-1);
	size += tree_size;

	// -- push the instruction to copy the source hashtable to the dest (either as a copy, or as a wrap)
	size += PushInstruction(countonly, instrptr, OP_HashtableCopy, DBG_instr);

    int32 copy_as_wrap = mIsWrap ? 1 : 0;
    size += PushInstruction(countonly, instrptr, copy_as_wrap, DBG_value);

	// -- success
	return (size);
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CHashtableCopyNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CHashtableCopyNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CHashtableIter ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CHashtableIter::CHashtableIter(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 iter_Type)
	: CCompileTreeNode(_codeblock, _link, eHashtableIter, _linenumber)
    , m_iterType(iter_Type)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CHashtableIter::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CHashtableIter::Eval() - missing leftchild\n");
        return (-1);
    }

   	// -- left child will have pushed the hashtable variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE_hashtable, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the "hashtable iter", which will pop the hashtable, and push the value (or bool if hashtable_end)
    size += PushInstruction(countonly, instrptr, OP_HashtableIter, DBG_instr);

    // -- push 0 == first(), 1 == next(), -1 == end()
	size += PushInstruction(countonly, instrptr, m_iterType, DBG_value);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CHashtableIter::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CHashtableIter::CompileToC() not implemented.\n");
    return (true);
}

// == class CTypeNode =================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CTypeNode::CTypeNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eType, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CTypeNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CTypeNode::Eval() - missing leftchild\n");
        return (-1);
    }

   	// -- left child will have pushed the variable
    int32 tree_size = leftchild->Eval(instrptr, TYPE__var, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the 'type' instruction
    size += PushInstruction(countonly, instrptr, OP_Type, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CTypeNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CHashtableIter::CompileToC() not implemented.\n");
    return (true);
}

// == class CEnsureNode ===============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CEnsureNode::CEnsureNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
	: CCompileTreeNode(_codeblock, _link, eEnsure, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CEnsureNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CEnsureNode::Eval() - missing leftchild\n");
        return (-1);
    }

    if (!rightchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), rightchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CEnsureNode::Eval() - missing rightchild\n");
        return (-1);
    }

   	// -- left child will have pushed the boolean result
    int32 tree_size = leftchild->Eval(instrptr, TYPE_bool, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- right child will have pushed the error string
    tree_size = rightchild->Eval(instrptr, TYPE_string, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the ensure instruction
    size += PushInstruction(countonly, instrptr, OP_Ensure, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CEnsureNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CEnsureNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CEnsureInterfaceNode ======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CEnsureInterfaceNode::CEnsureInterfaceNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                           uint32 ns_hash, uint32 interface_hash)
	: CCompileTreeNode(_codeblock, _link, eEnsureInterface, _linenumber)
    , m_nsHash(ns_hash)
    , m_interfaceHash(interface_hash)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CEnsureInterfaceNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    if (m_nsHash == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CEnsureCEnsureInterfaceNodeNode::Eval() - invalid namespace hash\n");
        return (-1);
    }

    if (m_interfaceHash == 0)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(), linenumber,
                      "Error - CEnsureCEnsureInterfaceNodeNode::Eval() - invalid interface hash\n");
        return (-1);
    }

    // -- push the ensure instruction
    size += PushInstruction(countonly, instrptr, OP_EnsureInterface, DBG_instr);
    size += PushInstruction(countonly, instrptr, m_nsHash, DBG_nshash);
    size += PushInstruction(countonly, instrptr, m_interfaceHash, DBG_nshash);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CEnsureInterfaceNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CEnsureInterfaceNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CSelfVarDeclNode ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSelfVarDeclNode::CSelfVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link,
                                   int32 _linenumber, const char* _varname, int32 _varnamelength,
                                   eVarType _type, int32 _array_size)
    : CCompileTreeNode(_codeblock, _link, eSelfVarDecl, _linenumber)
{
	SafeStrcpy(varname, sizeof(varname), _varname, _varnamelength + 1);
    type = _type;
    mArraySize = _array_size;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CSelfVarDeclNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	uint32 varhash = Hash(varname);
	size += PushInstruction(countonly, instrptr, OP_SelfVarDecl, DBG_instr);
	size += PushInstruction(countonly, instrptr, varhash, DBG_var);
	size += PushInstruction(countonly, instrptr, type, DBG_vartype);
	size += PushInstruction(countonly, instrptr, mArraySize, DBG_value);

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CSelfVarDeclNode::Dump(char*& output, int32& length) const
{
    if (mArraySize > 1)
        snprintf(output, length, "type: %s, var[%d]: %s", gCompileNodeTypes[GetType()], mArraySize, varname);
    else
        snprintf(output, length, "type: %s, var: %s", gCompileNodeTypes[GetType()], varname);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CSelfVarDeclNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CSelfVarDeclNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CObjMemberDeclNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjMemberDeclNode::CObjMemberDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link,
                                       int32 _linenumber, const char* _varname, int32 _varnamelength,
                                       eVarType _type, int32 _array_size)
    : CCompileTreeNode(_codeblock, _link, eObjMemberDecl, _linenumber)
{
	SafeStrcpy(varname, sizeof(varname), _varname, _varnamelength + 1);
    type = _type;
    mArraySize = _array_size;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CObjMemberDeclNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const {

	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    // -- left child resolves to an object
    if (!leftchild)
    {
        ScriptAssert_(codeblock->GetScriptContext(), leftchild != NULL, codeblock->GetFileName(),
                      linenumber,
                      "Error - CObjMemberDeclNode::Eval() - missing leftchild\n");
        return (-1);
    }

   	// -- left child will have pushed the variable that is to become an array
    int32 tree_size = leftchild->Eval(instrptr, TYPE_object, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	uint32 varhash = Hash(varname);
	size += PushInstruction(countonly, instrptr, OP_ObjMemberDecl, DBG_instr);
	size += PushInstruction(countonly, instrptr, varhash, DBG_var);
	size += PushInstruction(countonly, instrptr, type, DBG_vartype);
	size += PushInstruction(countonly, instrptr, mArraySize, DBG_value);

	return size;
}

// ====================================================================================================================
// Dump():  Outputs the text version of the instructions compiled from this node.
// ====================================================================================================================
void CObjMemberDeclNode::Dump(char*& output, int32& length) const
{
    if (mArraySize > 1)
        snprintf(output, length, "type: %s, var[%d]: %s", gCompileNodeTypes[GetType()], mArraySize, varname);
    else
        snprintf(output, length, "type: %s, var: %s", gCompileNodeTypes[GetType()], varname);
	int32 debuglength = (int32)strlen(output);
	output += debuglength;
	length -= debuglength;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CObjMemberDeclNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CObjMemberDeclNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CScheduleNode =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CScheduleNode::CScheduleNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool8 _repeat)
    : CCompileTreeNode(_codeblock, _link, eSched, _linenumber)
{
    mRepeat = _repeat;
};

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CScheduleNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CScheduleNode with no left child\n");
		return (-1);
	}

	// -- ensure we have a right child
	if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CScheduleNode with no right child\n");
		return (-1);
	}

    // -- push the "repeat" flag
    size += PushInstruction(countonly, instrptr, OP_Push, DBG_instr);
    size += PushInstruction(countonly, instrptr, TYPE_bool, DBG_vartype);
    size += PushInstruction(countonly, instrptr, mRepeat ? 1 : 0, DBG_value);

    // -- evaluate the left child, to push the object ID, and then the delay time
    int32 tree_size = leftchild->Eval(instrptr, TYPE_void, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- evaluate the right child, which first pushes the function hash,
    // -- then evaluates all the parameter assignments
    tree_size = rightchild->Eval(instrptr, pushresult, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CScheduleNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CScheduleNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CSchedFuncNode ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSchedFuncNode::CSchedFuncNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool8 _immediate)
    : CCompileTreeNode(_codeblock, _link, eSched, _linenumber)
{
    mImmediate = _immediate;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CSchedFuncNode::Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const
{
    DebugEvaluateNode(*this, countonly, instrptr);
    int32 size = 0;

    // -- ensure we have a left child
    if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CScheduleNode with no left child\n");
        return (-1);
    }

    // -- ensure we have a right child
    if (!rightchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CScheduleNode with no right child\n");
        return (-1);
    }

    // -- evaluate the leftchild, which will push the function hash
    int32 tree_size = leftchild->Eval(instrptr, TYPE_int, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the instruction to begin the schedule call
    size += PushInstruction(countonly, instrptr, OP_ScheduleBegin, DBG_instr);
    size += PushInstruction(countonly, instrptr, mImmediate, DBG_value);

    // -- evaluate the right child, tree of all parameters for the scheduled function call
    tree_size = rightchild->Eval(instrptr, TYPE_void, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- finalize the schedule call, which will push the schedule ID on the stack
    size += PushInstruction(countonly, instrptr, OP_ScheduleEnd, DBG_instr);

    // -- if we're not looking for a return value (e.g. not assigning this schedule call)
    if (pushresult <= TYPE_void)
    {
        // -- all functions will return a value - by default, a "" for void functions
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);
    }

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CSchedFuncNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CSchedFuncNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CSchedParamNode ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSchedParamNode::CSchedParamNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                 int32 _paramindex)
    : CCompileTreeNode(_codeblock, _link, eSchedParam, _linenumber)
{
    paramindex = _paramindex;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CSchedParamNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

	// -- ensure we have a left child
	if (!leftchild)
    {
        TinPrint(TinScript::GetContext(), "Error - CScheduleNode with no left child\n");
		return (-1);
	}

    // -- evaluate the left child, resolving to the value of the parameter
    int32 tree_size = leftchild->Eval(instrptr, TYPE__resolve, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- push the instruction to assign the parameter
    size += PushInstruction(countonly, instrptr, OP_ScheduleParam, DBG_instr);

    // -- push the index of the param to assign
    size += PushInstruction(countonly, instrptr, paramindex, DBG_value);

    return size;
};

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CSchedParamNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CSchedParamNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CCreateObjectNode =========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCreateObjectNode::CCreateObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                                     const char* _classname, uint32 _classlength, bool create_local)
    : CCompileTreeNode(_codeblock, _link, eCreateObject, _linenumber)
{
	SafeStrcpy(classname, sizeof(classname), _classname, _classlength + 1);
	mLocalObject = create_local;
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CCreateObjectNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

    // -- evaluate the left child, which resolves to the string name of the object
    int32 tree_size = leftchild->Eval(instrptr, TYPE_string, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- create the object by classname, objectname
	uint32 classhash = Hash(classname);
	size += PushInstruction(countonly, instrptr, OP_CreateObject, DBG_instr);
	size += PushInstruction(countonly, instrptr, classhash, DBG_hash);
	size += PushInstruction(countonly, instrptr, mLocalObject ? 1 : 0, DBG_value);

    // -- if we're not looking to assign the new object ID to anything, pop the stack
    if (pushresult <= TYPE_void)
        size += PushInstruction(countonly, instrptr, OP_Pop, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CCreateObjectNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CCreateObjectNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CDestroyObjectNode ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDestroyObjectNode::CDestroyObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber)
    : CCompileTreeNode(_codeblock, _link, eDestroyObject, _linenumber)
{
}

// ====================================================================================================================
// Eval():  Generates the byte code instruction compiled from this node.
// ====================================================================================================================
int32 CDestroyObjectNode::Eval(uint32*& instrptr, eVarType pushresult, bool8 countonly) const
{
	DebugEvaluateNode(*this, countonly, instrptr);
	int32 size = 0;

  	// -- evaluate the left child, pushing the a result of TYPE_object
    int32 tree_size = leftchild->Eval(instrptr, TYPE_object, countonly);
    if (tree_size < 0)
        return (-1);
    size += tree_size;

    // -- create the object by classname, objectname
	size += PushInstruction(countonly, instrptr, OP_DestroyObject, DBG_instr);

	return size;
}

// ====================================================================================================================
// CompileToC(): Convert the parse tree to valid C, to compile directly to the executable. 
// ====================================================================================================================
bool8 CDestroyObjectNode::CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const
{
    TinPrint(TinScript::GetContext(), "CDestroyObjectNode::CompileToC() not implemented.\n");
    return (true);
}

// == class CCodeBlock ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCodeBlock::CCodeBlock(CScriptContext* script_context, const char* _filename)
{
    mContextOwner = script_context;

    mIsParsing = true;
    mSourceHasChanged = false;

    mInstrBlock = NULL;
    mInstrCount = 0;

    smFuncDefinitionStack = TinAlloc(ALLOC_FuncCallStack, CFunctionCallStack);
    smCurrentGlobalVarTable = TinAlloc(ALLOC_VarTable, tVarTable, kLocalVarTableSize);
    mFunctionList = TinAlloc(ALLOC_FuncTable, tFuncTable, kLocalFuncTableSize);
    mBreakpoints = TinAlloc(ALLOC_Debugger, CHashTable<CDebuggerWatchExpression>, kBreakpointTableSize);

    // -- add to the resident list of codeblocks, if a name was given
    mFileName[0] = '\0';
    mFileNameHash = 0;
    if (_filename && _filename[0])
    {
        SafeStrcpy(mFileName, sizeof(mFileName), _filename, kMaxNameLength);
        mFileNameHash = Hash(mFileName);
        script_context->GetCodeBlockList()->AddItem(*this, mFileNameHash);
    }

    // -- keep track of the linenumber offsets
    mLineNumberIndex = 0;
    mLineNumberCount = 0;
    mLineNumberCurrent = -1;
    mLineNumbers = nullptr;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CCodeBlock::~CCodeBlock()
{
	if (mInstrBlock)
		TinFreeArray(mInstrBlock);

	// -- clear out the breakpoints list
	// -- do this before clearing functions, since conditionals/trace points contain funtion definitions
	mBreakpoints->DestroyAll();
	TinFree(mBreakpoints);

	smCurrentGlobalVarTable->DestroyAll();
    TinFree(smCurrentGlobalVarTable);

    mFunctionList->DestroyAll();
    TinFree(mFunctionList);

    // -- free the function definition stack, and the global var table
    TinFree(smFuncDefinitionStack);

    if (mLineNumbers)
        TinFreeArray(mLineNumbers);
}

// ====================================================================================================================
// CalcInstrCount():  Calculate the entire size of code block, including the instructions and the var table.
// ====================================================================================================================
int32 CCodeBlock::CalcInstrCount(const CCompileTreeNode& root)
{
	// -- the root is always a NOP, which will loop through and eval its siblings
	uint32* instrptr = NULL;
    int32 instr_count = 0;

    // -- add the size needed to store this block's global variables
    int32 var_table_instr_count = CompileVarTable(smCurrentGlobalVarTable, instrptr, true);
    if (var_table_instr_count < 0)
    {
        ScriptAssert_(GetScriptContext(), 0, GetFileName(), -1,
                      "Error - Unable to calculate the var table size for file: %s\n", GetFileName());
        return (-1);
    }
    instr_count += var_table_instr_count;

    // -- run through the tree, calculating the size needed to contain the compiled code
    int32 instruction_count = root.Eval(instrptr, TYPE_void, true);
    if (instruction_count < 0)
    {
        ScriptAssert_(GetScriptContext(), 0, GetFileName(), -1,
                      "Error - Unable to compile file: %s\n", GetFileName());
        return (-1);
    }

    instr_count += instruction_count;

    // -- add one to account for the OP_EOF added to the end of every code block
    ++instr_count;

    return instr_count;
}

// ====================================================================================================================
// CompileTree():  Recursively compile the nodes of the parse tree.
// ====================================================================================================================
bool8 CCodeBlock::CompileTree(const CCompileTreeNode& root)
{
	// -- the root is always a NOP, which will loop through and eval its siblings
	uint32* instrptr = mInstrBlock;

    // -- write out the instructions to populate the global variables needed
    CompileVarTable(smCurrentGlobalVarTable, instrptr, false);

    // -- compile the tree
	root.Eval(instrptr, TYPE_void, false);

	// -- push the specific operation to be performed
	PushInstruction(false, instrptr, OP_EOF, DBG_instr);

    uint32 verifysize = kPointerDiffUInt32(instrptr, mInstrBlock);
    if (mInstrCount != verifysize >> 2)
    {
        ScriptAssert_(GetScriptContext(), mInstrCount == verifysize >> 2, GetFileName(), -1,
                      "Error - Unable to compile: %s\n", GetFileName());
        return (false);
    }

	return true;
}

// ====================================================================================================================
// CompileTreeToSourceC():  Recursively compile the nodes of the parse tree to 'source C'
// ====================================================================================================================
bool8 CCodeBlock::CompileTreeToSourceC(const CCompileTreeNode& root, char*& out_buffer, int32& max_size)
{
    // -- write out the instructions to populate the global variables needed
    //CompileVarTable(smCurrentGlobalVarTable, instrptr, false);

    // -- compile the tree through the 
	if (!root.CompileToC(0, out_buffer, max_size, true))
        return (false);

    // -- success
	return true;
}

// ====================================================================================================================
// HasBreakpoints():  Method used by the debugger, returns true if there are debug breakpoints set in this codeblock.
// ====================================================================================================================
bool8 CCodeBlock::HasBreakpoints()
{
    return (mBreakpoints->Used() > 0);
}

// ====================================================================================================================
// AdjustLineNumber():  Given a line number, return the line number for an actual breakable line.
// ====================================================================================================================
int32 CCodeBlock::AdjustLineNumber(int32 line_number)
{
    // -- sanity check
    if (mLineNumberCount == 0)
        return (0);

    // -- ensure the line number we're attempting to set, is one that will actually execute
    for (uint32 i = 0; i < mLineNumberCount; ++i)
    {
        int32 instr_line_number = mLineNumbers[i] & 0xffff;
        if (instr_line_number != 0xffff && instr_line_number >= line_number)
        {
            return (mLineNumbers[i] & 0xffff);
        }
    }

    // -- return the last line
    return (mLineNumbers[mLineNumberCount - 1] & 0xffff);
}

// ====================================================================================================================
// GetPCForFunctionLineNumber():  given a line number, return where the instruction ptr would execute
// ====================================================================================================================
const uint32* CCodeBlock::GetPCForFunctionLineNumber(int32 line_number, int32& adjusted_line) const
{
    // -- sanity check
    if (line_number < 0)
        return nullptr;

    // -- ensure the line number we're attempting to set, is one that will actually execute
    adjusted_line = -1;
    const uint32* instrptr = nullptr;
    for (uint32 i = 0; i < mLineNumberCount; ++i)
    {
        int32 instr_line_number = mLineNumbers[i] & 0xffff;
        if (instr_line_number != 0xffff && instr_line_number >= line_number)
        {
            // -- at this point, we have the first breakable line number beyond the one we were given
            adjusted_line = mLineNumbers[i] & 0xffff;

            // -- the pc is the codeblock address + the offset for the adjusted line
            uint32 offset = mLineNumbers[i] >> 16;

            instrptr = GetInstructionPtr();
            instrptr += offset;

            // $$$TZA we need to validate that the instruction ptr is actually within a specific function definition
            break;
        }
    }

    // -- not found
    return instrptr;
}

// ====================================================================================================================
// AddBreakpoint():  Add notification that the debugger wants to break on the given line.
// ====================================================================================================================
int32 CCodeBlock::AddBreakpoint(int32 line_number, bool8 break_enabled, const char* conditional, const char* trace,
                                bool8 trace_on_condition)
{
    int32 adjusted_line_number = AdjustLineNumber(line_number);
    CDebuggerWatchExpression* watch = mBreakpoints->FindItem(adjusted_line_number);
    if (!watch)
    {
        CDebuggerWatchExpression *new_break = TinAlloc(ALLOC_Debugger, CDebuggerWatchExpression, adjusted_line_number,
                                                       true, break_enabled, conditional, trace, trace_on_condition);
        mBreakpoints->AddItem(*new_break, adjusted_line_number);
    }

    // -- otherwise, just set the expression
    else
        watch->SetAttributes(break_enabled, conditional, trace, trace_on_condition);

    return (adjusted_line_number);
}

// ====================================================================================================================
// RemoveBreakpoint():  Remove the breakpoint for a given line.
// ====================================================================================================================
int32 CCodeBlock::RemoveBreakpoint(int32 line_number)
{
    int32 adjusted_line_number = AdjustLineNumber(line_number);
    CDebuggerWatchExpression* watch = mBreakpoints->FindItem(adjusted_line_number);
    if (watch)
    {
        mBreakpoints->RemoveItem(adjusted_line_number);
        TinFree(watch);
    }

    return (adjusted_line_number);
}

// ====================================================================================================================
// RemoveAllBreakpoints():  Remove all breakpoints from the code block.
// ====================================================================================================================
void CCodeBlock::RemoveAllBreakpoints()
{
    mBreakpoints->DestroyAll();
}

// ====================================================================================================================
// -- debugging suppport

// ====================================================================================================================
// SetDebugCodeBlock():  Registered function to enable the debug output when compiling a code block.
// ====================================================================================================================
void SetDebugCodeBlock(bool8 torf)
{
    CScriptContext::gDebugCodeBlock = torf;
}

// ====================================================================================================================
// GetDebugCodeBlock():  Returns true if we're dumping the debug output during compiliation.
// ====================================================================================================================
bool8 GetDebugCodeBlock()
{
    return (CScriptContext::gDebugCodeBlock);
}

// ====================================================================================================================
// SetDebugForceCompile():  Registered function to force a compile on executed scripts, ignoring the .tso.
// ====================================================================================================================
void SetDebugForceCompile(bool8 torf)
{
    CScriptContext::gDebugForceCompile = torf;
    if (torf)
    {
        // -- capture the current time...  we'll use this time when comparing the compiled bin file for
        // any script we execute - if we're forcing compilation, then we only do it if the bin file
        // last change time is less than this time..
        // -- this allows us to force all scripts to be compiled before execution, but only once
        TinScript::CScriptContext::gDebugForceCompileTime = std::time(nullptr);
    }
}

// ====================================================================================================================
// GetDebugForceCompile():  Returns true if we're forcing compilation of executed scripts.
// ====================================================================================================================
bool8 GetDebugForceCompile(std::time_t& force_compile_time)
{
    force_compile_time = TinScript::CScriptContext::gDebugForceCompileTime;
    return (CScriptContext::gDebugForceCompile);
}

// ====================================================================================================================
// -- function registration

REGISTER_FUNCTION(SetDebugCodeBlock, SetDebugCodeBlock);
REGISTER_FUNCTION(SetDebugForceCompile, SetDebugForceCompile);

} // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
