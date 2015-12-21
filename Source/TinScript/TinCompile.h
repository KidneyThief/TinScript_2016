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
// TinCompile.h
// ====================================================================================================================

#ifndef __TINCOMPILE_H
#define __TINCOMPILE_H

// -- includes
#include "assert.h"

#include "TinTypes.h"
#include "TinParse.h"
#include "TinRegistration.h"
#include "TinExecute.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================
namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// forward declarations
class CVariableEntry;
class CFunctionContext;
class CFunctionEntry;
class CExecStack;
class CFunctionCallStack;
class CWhileLoopNode;

// --------------------------------------------------------------------------------------------------------------------
// compile tree node types
#define CompileNodeTypesTuple \
	CompileNodeTypeEntry(NOP)					\
	CompileNodeTypeEntry(Value)					\
	CompileNodeTypeEntry(Self)       			\
	CompileNodeTypeEntry(ObjMember) 			\
	CompileNodeTypeEntry(PODMember) 			\
	CompileNodeTypeEntry(Assignment)			\
	CompileNodeTypeEntry(BinaryOp)				\
	CompileNodeTypeEntry(UnaryOp)				\
	CompileNodeTypeEntry(IfStmt)				\
	CompileNodeTypeEntry(CondBranch)			\
	CompileNodeTypeEntry(WhileLoop)				\
	CompileNodeTypeEntry(ForLoop)				\
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
	CompileNodeTypeEntry(SelfVarDecl)			\
	CompileNodeTypeEntry(ObjMemberDecl)			\
	CompileNodeTypeEntry(Schedule)			    \
	CompileNodeTypeEntry(CreateObject)  	    \
	CompileNodeTypeEntry(DestroyObject)  	    \

// enum
enum ECompileNodeType
{
	#define CompileNodeTypeEntry(a) e##a,
	CompileNodeTypesTuple
	#undef CompileNodeTypeEntry

	eNodeTypeCount
};

const char* GetNodeTypeString(ECompileNodeType nodetype);

// --------------------------------------------------------------------------------------------------------------------
// operation types

#define OperationTuple \
	OperationEntry(NULL)				\
	OperationEntry(NOP)					\
	OperationEntry(VarDecl)				\
	OperationEntry(ParamDecl)			\
	OperationEntry(Assign)				\
	OperationEntry(PushParam)			\
	OperationEntry(Push)				\
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
	OperationEntry(UnaryBitInvert)	    \
	OperationEntry(UnaryNot)	        \
	OperationEntry(UnaryNeg)	        \
	OperationEntry(UnaryPos)	        \
	OperationEntry(Branch)				\
	OperationEntry(BranchTrue)			\
	OperationEntry(BranchFalse)			\
	OperationEntry(FuncDecl)    		\
	OperationEntry(FuncDeclEnd)    		\
	OperationEntry(FuncCallArgs)		\
	OperationEntry(FuncCall)			\
	OperationEntry(FuncReturn)			\
	OperationEntry(MethodCallArgs)		\
	OperationEntry(ArrayHash)			\
	OperationEntry(ArrayVarDecl)		\
	OperationEntry(ArrayDecl)		    \
	OperationEntry(SelfVarDecl)		    \
	OperationEntry(ObjMemberDecl)       \
	OperationEntry(ScheduleBegin)       \
	OperationEntry(ScheduleParam)       \
	OperationEntry(ScheduleEnd)         \
	OperationEntry(CreateObject)		\
	OperationEntry(DestroyObject)		\
	OperationEntry(EOF)					\

enum eOpCode {
	#define OperationEntry(a) OP_##a,
	OperationTuple
	#undef OperationEntry
	OP_COUNT
};

const char* GetOperationString(eOpCode op);

// ====================================================================================================================
// class CCompileTreeNode:  Base class for the nodes used comprising the parse tree.
// ====================================================================================================================
class CCompileTreeNode
{
	public:
		CCompileTreeNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link,
                         ECompileNodeType nodetype, int _linenumber);
		virtual ~CCompileTreeNode();

		CCompileTreeNode* next;
		CCompileTreeNode* leftchild;
		CCompileTreeNode* rightchild;

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length) const;

		ECompileNodeType GetType() const { return type; }
        CCodeBlock* GetCodeBlock() const { return codeblock; }
        int GetLineNumber() const { return linenumber; }

		static CCompileTreeNode* CreateTreeRoot(CCodeBlock* codeblock);

	protected:
        CCodeBlock* codeblock;
		ECompileNodeType type;
        int32 linenumber;

	protected:
		CCompileTreeNode(CCodeBlock* _codeblock = NULL)
        {
            type = eNOP;
            codeblock = _codeblock;
            linenumber = -1;
        }
};

// ====================================================================================================================
// class CValueNode:  Parse tree node, compiling to a literal value or a variable.
// ====================================================================================================================
class CValueNode : public CCompileTreeNode
{
	public:
		CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber, const char* _value,
                   int _valuelength, bool _isvar, eVarType _valtype);
		CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber, int _paramindex,
                   eVarType _valtype);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length) const;

	protected:
		bool8 isvariable;
        bool8 isparam;
        int32 paramindex;
		char value[kMaxTokenLength];
        eVarType valtype;

	protected:
		CValueNode() { }
};

// ====================================================================================================================
// class CBinaryOpNode:  Parse tree node, compiling to a binary operation.
// ====================================================================================================================
class CBinaryOpNode : public CCompileTreeNode
{
	public:
		CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                      eBinaryOpType _binaryoptype, bool _isassignop, eVarType _resulttype);

		CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                      eAssignOpType _assoptype, bool _isassignop, eVarType _resulttype);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length)const;

        eOpCode GetOpCode() const { return binaryopcode; }
        int GetBinaryOpPrecedence() const { return binaryopprecedence; }
        void OverrideBinaryOpPrecedence(int32 new_precedence) { binaryopprecedence = new_precedence; }

	protected:
        eOpCode binaryopcode;
        int32 binaryopprecedence;
		eVarType binopresult;
		bool8 isassignop;

	protected:
		CBinaryOpNode() { }
};

// ====================================================================================================================
// class CUnaryOpNode:  Parse tree node, compiling to a unary operation.
// ====================================================================================================================
class CUnaryOpNode : public CCompileTreeNode
{
	public:
		CUnaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                     eUnaryOpType _unaryoptype);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CUnaryOpNode() { }
        eOpCode unaryopcode;
};

// ====================================================================================================================
// class CSelfNode:  Parse tree node, compiling to the object ID for which a namespaced method is being called.
// ====================================================================================================================
class CSelfNode : public CCompileTreeNode
{
	public:
		CSelfNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CSelfNode() { }
};

// ====================================================================================================================
// class CObjMemberNode:  Parse tree node, compiling to the member of an object.
// ====================================================================================================================
class CObjMemberNode : public CCompileTreeNode
{
	public:
		CObjMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                       const char* _membername, int _memberlength);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length) const;

	protected:
		char membername[kMaxTokenLength];

	protected:
		CObjMemberNode() { }
};

// ====================================================================================================================
// class CObjMemberNode:  Parse tree node, compiling to the member of a POD registered type.
// ====================================================================================================================
class CPODMemberNode : public CCompileTreeNode
{
	public:
		CPODMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                       const char* _membername, int _memberlength);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length) const;

	protected:
		char podmembername[kMaxTokenLength];

	protected:
		CPODMemberNode() { }
};

// ====================================================================================================================
// class CObjMemberNode:  Parse tree node, compiling to an if statement.
// ====================================================================================================================
class CIfStatementNode : public CCompileTreeNode
{
	public:
		CIfStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CIfStatementNode() { }
};

// ====================================================================================================================
// class CCondBranchNode:  Parse tree node, compiling to an contitional branch.
// ====================================================================================================================
class CCondBranchNode : public CCompileTreeNode
{
	public:
		CCondBranchNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CCondBranchNode() { }
};

// ====================================================================================================================
// class CLoopJumpNode:  Parse tree node, compiling to an jump from within a loop.
// ====================================================================================================================
class CLoopJumpNode : public CCompileTreeNode
{
	public:
		CLoopJumpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber, CWhileLoopNode* loop_node,
                      bool is_break);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        void NotifyLoopInstr(uint32* continue_instr, uint32* break_instr);

	protected:
		CLoopJumpNode() { }
        bool8 mIsBreak;
        mutable uint32* mJumpInstr;
        mutable uint32* mJumpOffset;
};

// ====================================================================================================================
// class CWhileLoopNode:  Parse tree node, compiling to a while loop.
// ====================================================================================================================
class CWhileLoopNode : public CCompileTreeNode
{
	public:
		CWhileLoopNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        bool8 AddLoopJumpNode(CLoopJumpNode* jump_node);

        void SetEndOfLoopNode(CCompileTreeNode* node)
        {
            mEndOfLoopNode = node;
        }

        const CCompileTreeNode* GetEndOfLoopNode() const
        {
            return (mEndOfLoopNode);
        }

	protected:
		CWhileLoopNode() { }

        CCompileTreeNode* mEndOfLoopNode;

        // -- we're also going to keep a list of break and continue nodes that need to jump
        // -- to the beginning / end of this loop
        enum { kMaxLoopJumpCount = 128 };

        mutable uint32* mContinueHereInstr;
        mutable uint32* mBreakHereInstr;

        int32 mLoopJumpNodeCount;
        CLoopJumpNode* mLoopJumpNodeList[kMaxLoopJumpCount];
};

// ====================================================================================================================
// class CParenOpenNode:  Parse tree node, placeholder node to enforce an operation priority over precedence.
// ====================================================================================================================
class CParenOpenNode : public CCompileTreeNode
{
	public:
		CParenOpenNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CParenOpenNode() { }
};

// ====================================================================================================================
// class CFuncDeclNode:  Parse tree node, compiles to a function declaration.
// ====================================================================================================================
class CFuncDeclNode : public CCompileTreeNode
{
	public:
		CFuncDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber, const char* _funcname,
                      int _length, const char* _funcns, int _funcnslength, uint32 derived_ns);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length)const;

	protected:
		CFuncDeclNode() { }
        char funcname[kMaxNameLength];
        char funcnamespace[kMaxNameLength];
        CFunctionEntry* functionentry;
        uint32 mDerivedNamespace;
};

// ====================================================================================================================
// class CFuncCallNode:  Parse tree node, compiles to a function call.
// ====================================================================================================================
class CFuncCallNode : public CCompileTreeNode
{
	public:
		CFuncCallNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                      const char* _funcname, int _length, const char* _nsname, int _nslength,
                      bool _ismethod);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length)const;

	protected:
		char funcname[kMaxNameLength];
		char nsname[kMaxNameLength];
        bool8 ismethod;

	protected:
		CFuncCallNode() { }
};

// ====================================================================================================================
// class CFuncCallNode:  Parse tree node, compiles to a return stataement from within a function.
// ====================================================================================================================
class CFuncReturnNode : public CCompileTreeNode
{
	public:
		CFuncReturnNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CFuncReturnNode() { }
        CFunctionEntry* functionentry;
};

// ====================================================================================================================
// class CObjMethodNode:  Parse tree node, compiles to a object method call.
// ====================================================================================================================
class CObjMethodNode : public CCompileTreeNode
{
	public:
		CObjMethodNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                       const char* _methodname, int _methodlength);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int& length) const;

	protected:
		char methodname[kMaxTokenLength];

	protected:
		CObjMethodNode() { }
};

// ====================================================================================================================
// class CArrayHashNode:  Parse tree node, concatenates a value into an array hash for a hashtable dereference.
// ====================================================================================================================
class CArrayHashNode : public CCompileTreeNode
{
	public:
		CArrayHashNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CArrayHashNode() { }
};

// ====================================================================================================================
// class CArrayHashNode:  Parse tree node, concatenates a value into an array hash for a hashtable dereference.
// ====================================================================================================================
class CArrayVarNode : public CCompileTreeNode
{
	public:
		CArrayVarNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);
		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CArrayVarNode() { }
};

// ====================================================================================================================
// class CArrayHashNode:  Parse tree node, compiles to finding a variable entry from a hashtable.
// ====================================================================================================================
class CArrayVarDeclNode : public CCompileTreeNode
{
	public:
		CArrayVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                          eVarType _type);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CArrayVarDeclNode() { }
        eVarType type;
};

// ====================================================================================================================
// class CArrayDeclNode:  Parse tree node, converts a variable into an array.
// ====================================================================================================================
class CArrayDeclNode : public CCompileTreeNode
{
	public:
		CArrayDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber, int32 _size);
		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CArrayDeclNode() { }
        int32 mSize;
};

// ====================================================================================================================
// class CArrayDeclNode:  Parse tree node, compiles to declaring a member of the object calling a method.
// ====================================================================================================================
class CSelfVarDeclNode : public CCompileTreeNode
{
	public:
		CSelfVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                         const char* _varname, int _varnamelength, eVarType _type, int32 _array_size);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        virtual void Dump(char*& output, int32& length) const;

	protected:
		CSelfVarDeclNode() { }
        eVarType type;
        int32 mArraySize;
        char varname[kMaxNameLength];
};

// ====================================================================================================================
// class CObjMemberDeclNode:  Parse tree node, compiles to declaring a member of an object.
// ====================================================================================================================
class CObjMemberDeclNode : public CCompileTreeNode
{
	public:
		CObjMemberDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                           const char* _varname, int _varnamelength, eVarType _type, int32 _array_size);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        virtual void Dump(char*& output, int32& length) const;

	protected:
		CObjMemberDeclNode() { }
        eVarType type;
        int32 mArraySize;
        char varname[kMaxNameLength];
};

// ====================================================================================================================
// class CScheduleNode:  Parse tree node, compiles to scheduling function/method call.
// ====================================================================================================================
class CScheduleNode : public CCompileTreeNode
{
	public:
		CScheduleNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                      int32 _delaytime, bool8 repeat);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		char funcname[kMaxNameLength];
        int32 delaytime;
        bool8 mRepeat;

	protected:
		CScheduleNode() { }
};

// ====================================================================================================================
// class CScheduleNode:  Parse tree node, compiles to a node calling a scheduled function/method.
// ====================================================================================================================
class CSchedFuncNode : public CCompileTreeNode
{
    public:
        CSchedFuncNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                       bool8 _immediate);

        virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

    protected:
        bool8 mImmediate;

    protected:
        CSchedFuncNode() { }
};

// ====================================================================================================================
// class CScheduleNode:  Parse tree node, compiles to a node defining a parameter value for a scheduled function.
// ====================================================================================================================
class CSchedParamNode : public CCompileTreeNode
{
	public:
		CSchedParamNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                        int _paramindex);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

    protected :
        int32 paramindex;

	protected:
		CSchedParamNode() { }
};

// ====================================================================================================================
// class CCreateObjectNode:  Parse tree node, compiles to a node to create an object.
// ====================================================================================================================
class CCreateObjectNode : public CCompileTreeNode
{
	public:
		CCreateObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber,
                          const char* _classname, uint32 _classlength);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CCreateObjectNode() { }
		char classname[kMaxTokenLength];
};

// ====================================================================================================================
// class CDestroyObjectNode:  Parse tree node, compiles to a node to destroy an object.
// ====================================================================================================================
class CDestroyObjectNode : public CCompileTreeNode
{
	public:
		CDestroyObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int _linenumber);

		virtual int Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

	protected:
		CDestroyObjectNode() { }
};

// ====================================================================================================================
// class CCodeBlock:  Stores the table of local variables, functions, and the byte code for a compiled script.
// ====================================================================================================================
class CCodeBlock
{
	public:

		CCodeBlock(CScriptContext* script_context, const char* _filename = NULL);
		virtual ~CCodeBlock();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        void AllocateInstructionBlock(int _size, int _linecount)
        {
			mInstrBlock = NULL;
			mInstrCount = _size;
			if (_size > 0)
				mInstrBlock = TinAllocInstrBlock(_size);
            if(_linecount > 0)
                mLineNumbers = TinAllocInstrBlock(_linecount);
        }

        const char* GetFileName() const { return (mFileName); }

        uint32 GetFilenameHash() const { return (mFileNameHash); }

        void AddLineNumber(int linenumber, uint32* instrptr)
        {
            if (mLineNumbers)
            {
                uint32 offset = CalcOffset(instrptr);
                mLineNumbers[mLineNumberIndex++] = (offset << 16) + (linenumber & (0xffff));
            }
            else
                ++mLineNumberCount;
        }

		const uint32 GetInstructionCount() const { return (mInstrCount); }
		const uint32* GetInstructionPtr() const { return (mInstrBlock); }
		uint32* GetInstructionPtr() { return (mInstrBlock); }

		const uint32 GetLineNumberCount() const { return (mLineNumberCount); }
        void SetLineNumberCount(uint32 line_count) { mLineNumberCount = line_count; }
		uint32* GetLineNumberPtr() { return (mLineNumbers); }

        uint32 CalcLineNumber(const uint32* instrptr, bool* isNewLine = NULL) const
        {

#if !TIN_DEBUGGER
    return (0);
#endif
            // -- initialize the result
            if (isNewLine)
                *isNewLine = false;

            if (!instrptr || mLineNumberCount == 0)
                return (0);

            // get the offset
            uint32 curoffset = CalcOffset(instrptr);
            uint32 lineindex = 0;
            uint32 linenumber = 0;
            for (lineindex = 0; lineindex < mLineNumberCount; ++lineindex)
            {
                uint32 offset = mLineNumbers[lineindex] >> 16;
                uint32 line = mLineNumbers[lineindex] & 0xffff;
                if (line != 0xffff)
                {
                    // -- see if the current offset falls within range of this line
                    if (offset <= curoffset)
                        linenumber = line;
                    if (offset >= curoffset)
                    {
                        // -- set the result
                        if (isNewLine)
                        {
                            // -- check the previous valid instruction - see if it's on the same line or not
                            *isNewLine = true;
                            int testlineindex = lineindex;
                            while (--testlineindex >= 0)
                            {
                                uint32 testline = mLineNumbers[testlineindex] & 0xffff;
                                if (testline != 0xffff)
                                {
                                    *isNewLine = (testline != linenumber);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            return (linenumber);
        }

        uint32 CalcOffset(const uint32* instrptr) const
        {
            return kBytesToWordCount(kPointerDiffUInt32(instrptr, mInstrBlock));
        }

        int CalcInstrCount(const CCompileTreeNode& root);
        bool CompileTree(const CCompileTreeNode& root);
        bool Execute(uint32 offset, CExecStack& execstack, CFunctionCallStack& funccallstack);

        void AddFunction(CFunctionEntry* _func)
        {
            if (!mFunctionList->FindItem(_func->GetHash()))
                mFunctionList->AddItem(*_func, _func->GetHash());
        }

        void RemoveFunction(CFunctionEntry* _func)
        {
            mFunctionList->RemoveItem(_func->GetHash());
        }

        int IsInUse()
        {
            return (mIsParsing || !mFunctionList->IsEmpty());
        }

        void SetFinishedParsing() { mIsParsing = false; }

        CFunctionCallStack* smFuncDefinitionStack;
        tVarTable* smCurrentGlobalVarTable;

        // -- debugger interface
        bool8 HasBreakpoints();
        int32 AdjustLineNumber(int32 line_number);
        int32 AddBreakpoint(int32 line_number, bool8 break_enabled, const char* conditional, const char* trace,
                            bool8 trace_on_condition);
        int32 RemoveBreakpoint(int32 line_number);
        void RemoveAllBreakpoints();

        static void DestroyCodeBlock(CCodeBlock* codeblock)
        {
            if (!codeblock)
                return;
            if (codeblock->IsInUse())
            {
                ScriptAssert_(codeblock->GetScriptContext(), 0, "<internal>", -1,
                              "Error - Attempting to destroy active codeblock: %s\n",
                              codeblock->GetFileName());
                return;
            }
            codeblock->GetScriptContext()->GetCodeBlockList()->RemoveItem(codeblock, codeblock->mFileNameHash);
            TinFree(codeblock);
        }

        static void DestroyUnusedCodeBlocks(CHashTable<CCodeBlock>* code_block_list)
        {
            CCodeBlock* codeblock = code_block_list->First();
            while (codeblock)
            {
                if (!codeblock->IsInUse())
                {
                    code_block_list->RemoveItem(codeblock, codeblock->mFileNameHash);
                    TinFree(codeblock);
                }
                codeblock = code_block_list->Next();
            }
        }

	private:
        CScriptContext* mContextOwner;

        bool8 mIsParsing;

        char mFileName[kMaxNameLength];
        uint32 mFileNameHash;
		uint32* mInstrBlock;
		uint32 mInstrCount;

        // -- keep track of the linenumber offsets
        uint32 mLineNumberIndex;
        uint32 mLineNumberCount;
        uint32* mLineNumbers;

        // -- need to keep a list of all functions that are tied to this codeblock
        tFuncTable* mFunctionList;

        // -- keep a list of all lines to be broken on, for this code block
        CHashTable<CDebuggerWatchExpression>* mBreakpoints;
};

// ====================================================================================================================
// -- debugging support
void SetDebugCodeBlock(bool torf);
bool GetDebugCodeBlock();

}  // TinScript

#endif // __TINCOMPILE_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
