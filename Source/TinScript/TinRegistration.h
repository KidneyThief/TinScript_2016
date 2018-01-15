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
// TinRegistration.h:  This is the second header file that must be included for any source requiring access.
// ====================================================================================================================

#ifndef __TINREGISTRATION_H
#define __TINREGISTRATION_H

// -- includes

#include <tuple>
#include <type_traits>

#include "TinNamespace.h"
#include "TinStringTable.h"
#include "TinInterface.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations

class CVariableEntry;
class CFunctionEntry;
class CFunctionOverload;
class CCodeBlock;

typedef CHashTable<CVariableEntry> tVarTable;
typedef CHashTable<CFunctionEntry> tFuncTable;
typedef CHashTable<CFunctionOverload> tOverloadTable;

// ====================================================================================================================
// registration macros
// ====================================================================================================================
// -- these macros are generated from the python script genregclasses.py
// -- default usage is:  python genregclasses.py -maxparams 8
#include "registrationmacros.h"

// ====================================================================================================================
// class CFunctionContext:  Used to store the variable table for local varables and parameters.
// ====================================================================================================================
class CFunctionContext
{
    public:
        CFunctionContext(CScriptContext* script_context = nullptr);
        virtual ~CFunctionContext();

        CScriptContext* GetScriptContext() { return mContextOwner; }

        bool8 AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                           uint32 convert_type_from_object);
        bool8 AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size, int32 paramindex,
                           uint32 convert_type_from_object);
        CVariableEntry* AddLocalVar(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                    bool is_param);
        int32 GetParameterCount();
        CVariableEntry* GetParameter(int32 index);
        CVariableEntry* GetLocalVar(uint32 varhash);
        tVarTable* GetLocalVarTable();
        int32 CalculateLocalVarStackSize();
        bool8 IsParameter(CVariableEntry* ve);
        void ClearParameters();
        void InitStackVarOffsets(CFunctionEntry* fe);

        enum { eMaxParameterCount = 16, eMaxLocalVarCount = 37 };

        // -- for overloading, we want to hash the signature
        uint32 CalcHash();

        // -- use a variadic template to calculate the hash of a variable signature
        template <typename... T>
        struct SignatureHash;

        template <>
        struct SignatureHash <>
        {
            static uint32 calculate(uint32 cur_hash)
            {
                return cur_hash;
            }
        };

        template <typename H, typename... T>
        struct SignatureHash <H, T...>
        {
            static uint32 calculate(uint32 cur_hash)
            {
                // -- the strategy here is, each paremeter causes the current hash to be multiplied
                // -- by 3x the number of valid types...  then we add the current type, multiplied by 2 if
                // -- it's an array...  there should be no numerical collisions based on this
                const uint32 next_hash_multiplier = 3 * (uint32)LAST_VALID_TYPE;

                // -- first bump the current hash to the next multiplied value
                cur_hash *= next_hash_multiplier;

                // -- now add to the hash, the value of the "head", plus
                // -- the calculated value of the tail
                eVarType type = GetRegisteredType(GetTypeID<H>());
                uint32 hash = (uint32)type;

                return SignatureHash<T...>::calculate(cur_hash + hash);
            }
        };

        template <typename... T>
        static uint32 CalcSignatureHash()
        {
            return SignatureHash<T...>::calculate(0);
        }

    private:
        CScriptContext* mContextOwner;
        tVarTable* localvartable;

        // -- note:  the first parameter in the list is the return value
        // -- we're using an array to ensure the list stays ordered
        int32 paramcount;
        CVariableEntry* parameterlist[eMaxParameterCount];
};

// ====================================================================================================================
// class CRegFunctionBase:  Base class for registering functions.
// Templated versions for each parameter variation are derived from this class, implemented in registrationclasses.h.
// ====================================================================================================================
class CRegFunctionBase
{
    public:
        CRegFunctionBase(const char* _funcname = "")
        {
            funcname = _funcname;
            next = gRegistrationList;
            gRegistrationList = this;
        }

        virtual ~CRegFunctionBase() { }

        const char* GetName() { return funcname; }

        void SetScriptContext(CScriptContext* script_context) { mScriptContext = script_context; }
        CScriptContext* GetScriptContext() { return (mScriptContext); }

        void SetContext(CFunctionContext* fe) { funccontext = fe; }
        CFunctionContext* GetContext() { return (funccontext); }

        virtual void DispatchFunction(void*) { }

        virtual void Register(CScriptContext* script_context) = 0;
        CRegFunctionBase* GetNext() { return (next); }

        static CRegFunctionBase* gRegistrationList;

    private:
        const char* funcname;
        CFunctionContext* funccontext;
        CScriptContext* mScriptContext;

        CRegFunctionBase* next;
};

// ====================================================================================================================
// class CFunctionEntry:  Stores the details for a registered function, including the vartable and context.
// ====================================================================================================================
class CFunctionEntry
{
	public:
		CFunctionEntry(CScriptContext* script_context, uint32 _nshash, const char* _name,
                       uint32 _hash, eFunctionType _type, void* _addr, uint32 sig_hash = kFunctionSignatureUndefined);
		CFunctionEntry(CScriptContext* script_context, uint32 _nshash, const char* _name,
                       uint32 _hash, eFunctionType _type, CRegFunctionBase* _func,
                       uint32 sig_hash = kFunctionSignatureUndefined);
		virtual ~CFunctionEntry();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

		const char* GetName() const { return (mName); }

		uint32 GetNamespaceHash() const
        {
            if (mNamespaceHash == 0)
                return (CScriptContext::kGlobalNamespaceHash);
            else
			    return (mNamespaceHash);
		}

		uint32 GetHash() const { return (mHash); }

        void SetActiveOverload(uint32 sig_hash) { mDefineOverload = sig_hash; }
        uint32 GetActiveOverload() const { return (mDefineOverload); }

        // -- when preparing to call a function, because we *might* not know which
        // -- overload we're calling, we use the "active context", which might either
        // -- be the context for the known overload, or the one owned by the
        // -- function calls stack entry, which is used to match and find the overload
        void SetPrepareContext(CFunctionContext* context) { mPrepareContext = context; }
        CFunctionContext* GetPrepareContext() const {return (mPrepareContext); }
        CFunctionContext* GetActiveContext() const;

        bool8 HasOverloads() const { return (mOverloadTable.Used() > 1); }

        CFunctionOverload* FindOverload(uint32 signature_hash) const;

        // -- if there's an existing overload with the same signature, this replaces it
        void AddOverload(CFunctionOverload* _overload);

        // -- additional bool to remove the owning function entry, if there are no other overloads
        void RemoveOverload(CFunctionOverload* _overload, bool remove_function);

        // -- the overload signature might not match *exactly*, so given a CFunctionContext*, see
        // -- if we can find a signel overload that matches without ambiguity
        uint32 FindMatchingSignature(CFunctionContext* caller) const;

        // accessors that return the details of a specific overload
		eFunctionType GetType(uint32 signature_hash) const;
		void* GetAddr(uint32 signature_hash) const;

        void SetCodeBlockOffset(uint32 signature_hash, CCodeBlock* _codeblock, uint32 _offset);
        int32 GetCodeBlockOffset(uint32 signature_hash, CCodeBlock*& _codeblock) const;
        CFunctionContext* GetContext(uint32 signature_hash);

        eVarType GetReturnType(uint32 signature_hash);
        tVarTable* GetLocalVarTable(uint32 signature_hash);
        CRegFunctionBase* GetRegObject(uint32 signature_hash);

	private:
        CScriptContext* mContextOwner;

		char mName[kMaxNameLength];
		uint32 mHash;
        uint32 mNamespaceHash;

        // -- store the hash dictionary of overloads
        CHashTable<CFunctionOverload> mOverloadTable;

        // -- track which overload is currently being defined
        uint32 mDefineOverload;

        // -- tr4ack which context we're filling in with values before calling
        CFunctionContext* mPrepareContext;
};

// ====================================================================================================================
// class CFunctionOverload:  Stores the implementations for a registered function with different signatures
// ====================================================================================================================
class CFunctionOverload
{
    public:
        CFunctionOverload(CFunctionEntry* fe = nullptr, uint32 _signature_hash = kFunctionSignatureUndefined,
                            eFunctionType _type = eFunctionType::None, void* _addr = nullptr,
                            CRegFunctionBase* reg_object = nullptr);
        ~CFunctionOverload();

        // -- an overload belongs to the same script context as its owner
        CScriptContext* GetScriptContext() { return (mFunctionEntry->GetScriptContext()); }

        // -- the overload is declared befor the signature has been parsed
        bool8 FinalizeSignature();

        // -- accessors
        CFunctionContext* GetContext() { return (&mContext); }
        uint32 GetHash() const { return (mSignatureHash); }
        eFunctionType GetType() const { return (mType); }
        CFunctionEntry* GetFunctionEntry() { return (mFunctionEntry); }
        void* GetAddr() const { return (mAddr); }
        CCodeBlock* GetCodeBlock() const { return mCodeBlock; }
        int32 GetCodeBlockOffset(CCodeBlock*& _codeblock) const
        {
            _codeblock = mCodeBlock;
            return (mInstrOffset); 
        }
        CRegFunctionBase* GetRegObject() const { return (mRegObject); }

        void SetCodeBlockOffset(CCodeBlock* _codeblock, uint32 _offset);

    private:
        uint32 mSignatureHash;
        CFunctionEntry* mFunctionEntry;

		eFunctionType mType;
        void* mAddr;
		uint32 mInstrOffset;
        CCodeBlock* mCodeBlock;
        CFunctionContext mContext;
        CRegFunctionBase* mRegObject;
};

// ====================================================================================================================
// registration classes
// ====================================================================================================================
// -- these classes are generated from the python script genregclasses.py
// -- default usage is:  python genregclasses.py -maxparams 8
#include "registrationclasses.h"
#include "variadicclasses.h"

}  // TinScript

#endif // __TINREGISTRATION_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
