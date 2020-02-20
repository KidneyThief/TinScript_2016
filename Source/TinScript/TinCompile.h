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
	CompileNodeTypeEntry(Comment)				\
	CompileNodeTypeEntry(BinaryNOP)			    \
	CompileNodeTypeEntry(DebugNOP)			    \
	CompileNodeTypeEntry(Value)					\
	CompileNodeTypeEntry(Self)       			\
	CompileNodeTypeEntry(ObjMember) 			\
	CompileNodeTypeEntry(PODMember) 			\
	CompileNodeTypeEntry(Assignment)			\
	CompileNodeTypeEntry(BinaryOp)				\
	CompileNodeTypeEntry(UnaryOp)				\
	CompileNodeTypeEntry(SwitchStmt)    		\
	CompileNodeTypeEntry(CaseStmt)    		    \
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
	CompileNodeTypeEntry(ArrayCount)		    \
	CompileNodeTypeEntry(ArrayCopy)		        \
	CompileNodeTypeEntry(HashtableHasKey)		\
	CompileNodeTypeEntry(HashtableCopy)	    	\
	CompileNodeTypeEntry(HashtableIter)		    \
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
	OperationEntry(DebugMsg)			\
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
	OperationEntry(ArrayHash)			\
	OperationEntry(ArrayVarDecl)		\
	OperationEntry(ArrayDecl)		    \
	OperationEntry(ArrayCount)		    \
	OperationEntry(ArrayCopy)		    \
	OperationEntry(HashtableHasKey)		\
	OperationEntry(HashtableCopy)		\
	OperationEntry(HashtableIter)		\
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
                         ECompileNodeType nodetype, int32 _linenumber);
		virtual ~CCompileTreeNode();

		CCompileTreeNode* next;
		CCompileTreeNode* leftchild;
		CCompileTreeNode* rightchild;

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length) const;

        bool8 OutputIndentToBuffer(int32 indent, char*& out_buffer, int32& max_size) const;
        bool8 OutputToBuffer(int32 indent, char*& out_buffer, int32& max_size, const char* format, ...) const;
        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool8 root_node = false) const;

		ECompileNodeType GetType() const { return type; }
        CCodeBlock* GetCodeBlock() const { return codeblock; }
        int32 GetLineNumber() const { return linenumber; }

        virtual bool8 IsAssignOpNode() const { return (false); }

		static CCompileTreeNode* CreateTreeRoot(CCodeBlock* codeblock);

        void SetPostUnaryOpDelta(int32 unary_delta) { m_unaryDelta = unary_delta; }

	protected:
        CCodeBlock* codeblock;
		ECompileNodeType type;
        int32 linenumber;
        int32 m_unaryDelta;

	protected:
        friend class CMemoryTracker;
		CCompileTreeNode(CCodeBlock* _codeblock = NULL)
        {
            type = eNOP;
            codeblock = _codeblock;
            linenumber = -1;
            m_unaryDelta = 0;
        }
};

// ====================================================================================================================
// class CDebugNode:  An inoccuous node that will issue a debug op command, to print a message.
// ====================================================================================================================
class CDebugNode : public CCompileTreeNode
{
    public:
        CDebugNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, const char* debug_msg);

        virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

    protected:
        const char* m_debugMesssage;

    protected:
        CDebugNode() { }
};

// ====================================================================================================================
// class CCommentNode:  An inoccuous node that will store a comment string for compiling to C
// ====================================================================================================================
class CCommentNode : public CCompileTreeNode
{
    public:
        CCommentNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                     const char* _comment, int32 _length);

        virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

    protected:
        CCommentNode() { }
        char m_comment[kMaxTokenLength];
};

// ====================================================================================================================
// class CBinaryTreeNode:  Parse tree node, that holds a left and right child, and specifies their eval types.
// ====================================================================================================================
class CBinaryTreeNode : public CCompileTreeNode
{
    public:
        CBinaryTreeNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, eVarType _left_result_type,
                    eVarType _right_result_type);

        virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

protected:
    eVarType m_leftResultType;
    eVarType m_rightResultType;

protected:
    CBinaryTreeNode() { }
};

// ====================================================================================================================
// class CValueNode:  Parse tree node, compiling to a literal value or a variable.
// ====================================================================================================================
class CValueNode : public CCompileTreeNode
{
	public:
        CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, const char* _value,
                   int32 _valuelength, bool _isvar, eVarType _valtype);
        CValueNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 _paramindex,
                   eVarType _valtype);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length) const;

        bool IsParameter() { return (isparam); }

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                      eBinaryOpType _binaryoptype, bool _isassignop, eVarType _resulttype);

		CBinaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                      eAssignOpType _assoptype, bool _isassignop, eVarType _resulttype);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length)const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

        virtual bool8 IsAssignOpNode() const { return (assign_op != eAssignOpType::ASSOP_NULL); }
        eOpCode GetOpCode() const { return binaryopcode; }
        int32 GetBinaryOpPrecedence() const { return binaryopprecedence; }
        void OverrideBinaryOpPrecedence(int32 new_precedence) { binaryopprecedence = new_precedence; }

	protected:
        eOpCode binaryopcode;
        int32 binaryopprecedence;
		eVarType binopresult;
		eAssignOpType assign_op;
        eBinaryOpType bin_op;

	protected:
		CBinaryOpNode() { }
};

// ====================================================================================================================
// class CUnaryOpNode:  Parse tree node, compiling to a unary operation.
// ====================================================================================================================
class CUnaryOpNode : public CCompileTreeNode
{
	public:
		CUnaryOpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                     eUnaryOpType _unaryoptype);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        virtual bool8 IsAssignOpNode() const { return (unaryopcode == OP_UnaryPreInc ||
                                                       unaryopcode == OP_UnaryPreDec); }

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CUnaryOpNode() { }
        eOpCode unaryopcode;
        eUnaryOpType m_unaryOpType;
};

// ====================================================================================================================
// class CSelfNode:  Parse tree node, compiling to the object ID for which a namespaced method is being called.
// ====================================================================================================================
class CSelfNode : public CCompileTreeNode
{
	public:
		CSelfNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CSelfNode() { }
};

// ====================================================================================================================
// class CObjMemberNode:  Parse tree node, compiling to the member of an object.
// ====================================================================================================================
class CObjMemberNode : public CCompileTreeNode
{
	public:
		CObjMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       const char* _membername, int32 _memberlength);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CPODMemberNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       const char* _membername, int32 _memberlength);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

    protected:
		char podmembername[kMaxTokenLength];

	protected:
		CPODMemberNode() { }
};

// ====================================================================================================================
// class CLoopJumpNode:  Parse tree node, compiling to an jump from within a loop.
// ====================================================================================================================
class CLoopJumpNode : public CCompileTreeNode
{
	public:
		CLoopJumpNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, CCompileTreeNode* loop_node,
                      bool is_break);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        void NotifyLoopInstr(uint32* continue_instr, uint32* break_instr);

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CLoopJumpNode() { }
        bool8 mIsBreak;
        mutable uint32* mJumpInstr;
        mutable uint32* mJumpOffset;
};

// ====================================================================================================================
// class CCaseStatementNode:  Parse tree node, compiling to a switch statement.
// ====================================================================================================================
class CCaseStatementNode : public CCompileTreeNode
{
	public:
        CCaseStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        int32 EvalCondition(uint32*& instrptr, bool countonly);
        int32 EvalStatements(uint32*& instrptr, bool countonly);

        void SetDefaultCase() { m_isDefaultCase = true; }
        void SetDefaultOffsetInstr(uint32* instrptr) { m_branchOffset = instrptr; }

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
        CCaseStatementNode() { }
        bool8 m_isDefaultCase;
        uint32* m_branchOffset;
};

// ====================================================================================================================
// class CSwitchStatementNode:  Parse tree node, compiling to a switch statement.
// ====================================================================================================================
class CSwitchStatementNode : public CCompileTreeNode
{
    public:
        CSwitchStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

        virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        bool8 SetDefaultNode(CCaseStatementNode* default_node);
        bool8 AddLoopJumpNode(CLoopJumpNode* jump_node);

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

    protected:
        CSwitchStatementNode() { }
        CCaseStatementNode* m_defaultNode;

        // -- we're also going to keep a list of break nodes so we can set the offsets to 
        enum { kMaxLoopJumpCount = 128 };
        int32 mLoopJumpNodeCount;
        CLoopJumpNode* mLoopJumpNodeList[kMaxLoopJumpCount];
};

// ====================================================================================================================
// class CIfStatementNode:  Parse tree node, compiling to an if statement.
// ====================================================================================================================
class CIfStatementNode : public CCompileTreeNode
{
	public:
		CIfStatementNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
         
        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CIfStatementNode() { }
};

// ====================================================================================================================
// class CCondBranchNode:  Parse tree node, compiling to an contitional branch.
// ====================================================================================================================
class CCondBranchNode : public CCompileTreeNode
{
	public:
		CCondBranchNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CCondBranchNode() { }
};

// ====================================================================================================================
// class CWhileLoopNode:  Parse tree node, compiling to a while loop.
// ====================================================================================================================
class CWhileLoopNode : public CCompileTreeNode
{
	public:
		CWhileLoopNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool is_do_while);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        bool8 AddLoopJumpNode(CLoopJumpNode* jump_node);

        void SetEndOfLoopNode(CCompileTreeNode* node)
        {
            mEndOfLoopNode = node;
        }

        const CCompileTreeNode* GetEndOfLoopNode() const
        {
            return (mEndOfLoopNode);
        }

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CWhileLoopNode() { }

        CCompileTreeNode* mEndOfLoopNode;

        // -- we're also going to keep a list of break and continue nodes that need to jump
        // -- to the beginning / end of this loop
        enum { kMaxLoopJumpCount = 128 };

        mutable uint32* mContinueHereInstr;
        mutable uint32* mBreakHereInstr;

        bool8 m_isDoWhile;
        int32 mLoopJumpNodeCount;
        CLoopJumpNode* mLoopJumpNodeList[kMaxLoopJumpCount];
};

// ====================================================================================================================
// class CParenOpenNode:  Parse tree node, placeholder node to enforce an operation priority over precedence.
// ====================================================================================================================
class CParenOpenNode : public CCompileTreeNode
{
	public:
		CParenOpenNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CParenOpenNode() { }
};

// ====================================================================================================================
// class CFuncDeclNode:  Parse tree node, compiles to a function declaration.
// ====================================================================================================================
class CFuncDeclNode : public CCompileTreeNode
{
	public:
		CFuncDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, const char* _funcname,
                      int32 _length, const char* _funcns, int32 _funcnslength, uint32 derived_ns);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length)const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CFuncCallNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                      const char* _funcname, int32 _length, const char* _nsname, int32 _nslength,
                      bool _ismethod);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length)const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CFuncReturnNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CObjMethodNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       const char* _methodname, int32 _methodlength);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
		virtual void Dump(char*& output, int32& length) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CArrayHashNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CArrayHashNode() { }
};

// ====================================================================================================================
// class CArrayHashNode:  Parse tree node, concatenates a value into an array hash for a hashtable dereference.
// ====================================================================================================================
class CArrayVarNode : public CCompileTreeNode
{
	public:
		CArrayVarNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);
		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CArrayVarNode() { }
};

// ====================================================================================================================
// class CArrayHashNode:  Parse tree node, compiles to finding a variable entry from a hashtable.
// ====================================================================================================================
class CArrayVarDeclNode : public CCompileTreeNode
{
	public:
		CArrayVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                          eVarType _type);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CArrayDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 _size);
		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CArrayDeclNode() { }
        int32 mSize;
};

// ====================================================================================================================
// class CArrayCountNode:  Parse tree node, pushes the array count of the left child array var.
// ====================================================================================================================
class CArrayCountNode : public CCompileTreeNode
{
	public:
		CArrayCountNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);
		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CArrayCountNode() { }
};

// ====================================================================================================================
// class CHashtableHasKey:  Parse tree node, pushes a bool for the left node having the given key.
// ====================================================================================================================
class CHashtableHasKey : public CCompileTreeNode
{
	public:
		CHashtableHasKey(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);
		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CHashtableHasKey() { }
};

// ====================================================================================================================
// class CHashtableIter:  Parse tree node, pushes the string of the first/next hashtable key.
// ====================================================================================================================
class CHashtableIter : public CCompileTreeNode
{
	public:
		CHashtableIter(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, int32 iter_type);
		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CHashtableIter() { }
        int32 m_iterType;
};

// ====================================================================================================================
// class CArrayDeclNode:  Parse tree node, compiles to declaring a member of the object calling a method.
// ====================================================================================================================
class CSelfVarDeclNode : public CCompileTreeNode
{
	public:
		CSelfVarDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                         const char* _varname, int32 _varnamelength, eVarType _type, int32 _array_size);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        virtual void Dump(char*& output, int32& length) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CObjMemberDeclNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                           const char* _varname, int32 _varnamelength, eVarType _type, int32 _array_size);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;
        virtual void Dump(char*& output, int32& length) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CScheduleNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber, bool8 repeat);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		char funcname[kMaxNameLength];
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
        CSchedFuncNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                       bool8 _immediate);

        virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CSchedParamNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                        int32 _paramindex);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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
		CCreateObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber,
                          const char* _classname, uint32 _classlength, bool local_object);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

	protected:
		CCreateObjectNode() { }
		char classname[kMaxTokenLength];
		bool mLocalObject;
};

// ====================================================================================================================
// class CDestroyObjectNode:  Parse tree node, compiles to a node to destroy an object.
// ====================================================================================================================
class CDestroyObjectNode : public CCompileTreeNode
{
	public:
		CDestroyObjectNode(CCodeBlock* _codeblock, CCompileTreeNode*& _link, int32 _linenumber);

		virtual int32 Eval(uint32*& instrptr, eVarType pushresult, bool countonly) const;

        virtual bool8 CompileToC(int32 indent, char*& out_buffer, int32& max_size, bool root_node) const;

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

        void AllocateInstructionBlock(int32 _size, int32 _linecount)
        {
			mInstrBlock = NULL;
			mInstrCount = _size;
            if (_size > 0)
                mInstrBlock = TinAllocArray(ALLOC_CodeBlock, uint32, _size);
            if(_linecount > 0)
                mLineNumbers = TinAllocArray(ALLOC_CodeBlock, uint32, _linecount);
        }

        const char* GetFileName() const { return (mFileName); }

        uint32 GetFilenameHash() const { return (mFileNameHash); }

        void AddLineNumber(int32 linenumber, uint32* instrptr)
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
                            int32 testlineindex = lineindex;
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

        int32 CalcInstrCount(const CCompileTreeNode& root);
        bool CompileTree(const CCompileTreeNode& root);
        bool Execute(uint32 offset, CExecStack& execstack, CFunctionCallStack& funccallstack);

        //-- CompileToC members
        bool CompileTreeToSourceC(const CCompileTreeNode& root, char*& out_buffer, int32& max_size);

        void AddFunction(CFunctionEntry* _func)
        {
            if (!mFunctionList->FindItem(_func->GetHash()))
                mFunctionList->AddItem(*_func, _func->GetHash());

            // $$$TZA Overload
            //printf("### DEBUG: 0x%x\n", _func->GetContext()->CalcHash());
        }

        void RemoveFunction(CFunctionEntry* _func)
        {
            mFunctionList->RemoveItem(_func->GetHash());
        }

        int32 IsInUse()
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
