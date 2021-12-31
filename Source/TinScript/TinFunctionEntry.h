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
// TinFunctionEntry.h:  Defines the classes for a registered function, type (script or code), context (parameters...)
// ====================================================================================================================

#pragma once

// -- includes
#include "TinScript.h"

namespace TinScript
{

// -- forward declarations --------------------------------------------------------------------------------------------
class CVariableEntry;
class CRegFunctionBase;

// ====================================================================================================================
// class CFunctionContext:  Used to store the variable table for local variables and parameters.
// ====================================================================================================================
class CFunctionContext
{
public:
    CFunctionContext();
    virtual ~CFunctionContext();

    bool8 AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                       uint32 convert_type_from_object, bool is_thread_exec = false);
    bool8 AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size, int32 paramindex,
                       uint32 convert_type_from_object, bool8 is_thread_exec = false);
    CVariableEntry* AddLocalVar(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                bool8 is_param, bool8 is_thread_exec = false);
    int32 GetParameterCount();
    CVariableEntry* GetParameter(int32 index);
    CVariableEntry* GetLocalVar(uint32 varhash);
    tVarTable* GetLocalVarTable();
    int32 CalculateLocalVarStackSize();
    bool8 IsParameter(CVariableEntry* ve);
    void ClearParameters();
    bool InitDefaultArgs(CFunctionEntry* fe);
    void InitStackVarOffsets(CFunctionEntry* fe);

    enum { eMaxParameterCount = 16,eMaxLocalVarCount = 37 };

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

    template <typename H,typename... T>
    struct SignatureHash <H,T...>
    {
        static uint32 calculate(uint32 cur_hash)
        {
            // -- the strategy here is, each paremeter causes the current hash to be multiplied
            // -- by 3x the number of valid types...  then we add the current type, multiplied by 2 if
            // -- it's an array...  there should be no numerical collisions based on this
            const uint32 next_hash_multiplier = 3 * (uint32)TYPE_COUNT;

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
    tVarTable* localvartable;

    // -- note:  the first parameter in the list is the return value
    // -- we're using an array to ensure the list stays ordered
    int32 paramcount;
    CVariableEntry* parameterlist[eMaxParameterCount];
};

// ====================================================================================================================
// class CFunctionEntry:  Stores the details for a registered function, including the vartable and context.
// ====================================================================================================================
class CFunctionEntry
{
public:
    CFunctionEntry(uint32 _nshash, const char* _name, uint32 _hash,EFunctionType _type, void* _addr);
    CFunctionEntry(uint32 _nshash, const char* _name, uint32 _hash,EFunctionType _type, CRegFunctionBase* _func);
    virtual ~CFunctionEntry();

    const char* GetName() const { return (mName); }
    EFunctionType GetType() const { return (mType); }

    uint32 GetNamespaceHash() const
    {
        if(mNamespaceHash == 0)
            return (CScriptContext::kGlobalNamespaceHash);
        else
            return (mNamespaceHash);
    }

    uint32 GetHash() const { return (mHash); }
    void* GetAddr() const;

    void SetCodeBlockOffset(CCodeBlock* _codeblock,uint32 _offset);
    uint32 GetCodeBlockOffset(CCodeBlock*& _codeblock) const;
    CFunctionContext* GetContext();

    CCodeBlock* GetCodeBlock() const { return (mCodeblock); }

    eVarType GetReturnType();
    tVarTable* GetLocalVarTable();
    CRegFunctionBase* GetRegObject();

private:
    char mName[kMaxNameLength];
    uint32 mHash;
    EFunctionType mType;
    uint32 mNamespaceHash;

    void* mAddr;
    uint32 mInstrOffset;
    CCodeBlock* mCodeblock;

    CFunctionContext mContext;

    CRegFunctionBase* mRegObject;
};

} // namespace TinScript

// -- eof -------------------------------------------------------------------------------------------------------------
