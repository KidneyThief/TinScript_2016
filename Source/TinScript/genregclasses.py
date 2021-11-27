# ------------------------------------------------------------------------------------------------
#  The MIT License
#  
#  Copyright (c) 2013 Tim Andersen
#  
#  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
#  and associated documentation files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#  
#  The above copyright notice and this permission notice shall be included in all copies or
#  substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
#  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
#  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# ------------------------------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#  python script to generate the templated registration classes
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# For usage, call: pyton genregclasses.py -help
# -----------------------------------------------------------------------------

import sys
import os
import fileinput
import ctypes
from ctypes import *

# -----------------------------------------------------------------------------
def OutputLicense(outputfile):

    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("//  The MIT License\n");
    outputfile.write("//  \n");
    outputfile.write("//  Copyright (c) 2021 Tim Andersen\n");
    outputfile.write("//  \n");
    outputfile.write("//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software\n");
    outputfile.write("//  and associated documentation files (the \"Software\"), to deal in the Software without\n");
    outputfile.write("//  restriction, including without limitation the rights to use, copy, modify, merge, publish,\n");
    outputfile.write("//  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the\n");
    outputfile.write("//  Software is furnished to do so, subject to the following conditions:\n");
    outputfile.write("//  \n");
    outputfile.write("//  The above copyright notice and this permission notice shall be included in all copies or\n");
    outputfile.write("//  substantial portions of the Software.\n");
    outputfile.write("//  \n");
    outputfile.write("//  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING\n");
    outputfile.write("//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n");
    outputfile.write("//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,\n");
    outputfile.write("//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
    outputfile.write("//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n");
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("\n");

# -----------------------------------------------------------------------------
def GenerateMacros(maxparamcount, outputfilename):
    
    print("GenerateMacros - Output: %s" % outputfilename);

    #open the output file
    outputfile = open(outputfilename, 'w');

    # -- add the MIT license to the top of the output file
    OutputLicense(outputfile);
    
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("// Generated macros for function registration\n");
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");

    paramcount = 0;
    while (paramcount <= maxparamcount):
        outputfile.write("\n\n// -- Parameter count: %d\n" % paramcount);
        
        #  global function
        regfuncstring = "#define REGISTER_FUNCTION_P%d" % paramcount + "(scriptname, funcname, R";
        i = 1;
        while (i <= paramcount):
            regfuncstring = regfuncstring + ", T%d" % i;
            i = i + 1;
        regfuncstring = regfuncstring + ") \\\n";
        outputfile.write(regfuncstring);
        
        regobjstring = "    static TinScript::CRegFunctionP%d" % paramcount + "<R";
        i = 1;
        while (i <= paramcount):
            regobjstring = regobjstring + ", T%d" % i;
            i = i + 1;
        regobjstring = regobjstring + "> _reg_##scriptname(#scriptname, funcname);";
        outputfile.write(regobjstring);

        #  method
        regfuncstring = "\n\n#define REGISTER_METHOD_P%d" % paramcount + "(classname, scriptname, methodname, R";
        i = 1;
        while (i <= paramcount):
            regfuncstring = regfuncstring + ", T%d" % i;
            i = i + 1;
        regfuncstring = regfuncstring + ")    \\\n";
        outputfile.write(regfuncstring);
        
        wrapperstring = "    static R classname##methodname(classname* obj"
        i = 1;
        while (i <= paramcount):
            wrapperstring = wrapperstring + ", T%d t%d" % (i, i);
            i = i + 1;
        wrapperstring = wrapperstring + ") {    \\\n";
        outputfile.write(wrapperstring);
        
        wrapperstring = "        return obj->methodname(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                wrapperstring = wrapperstring + ", ";
            wrapperstring = wrapperstring + "t%d" % i;
            i = i + 1;
        wrapperstring = wrapperstring + ");    \\\n";
        outputfile.write(wrapperstring);
        outputfile.write("    }    \\\n");
        
        regobjstring = "    static TinScript::CRegMethodP%d" % paramcount + "<classname, R";
        i = 1;
        while (i <= paramcount):
            regobjstring = regobjstring + ", T%d" % i;
            i = i + 1;
        regobjstring = regobjstring + "> _reg_##classname##methodname(#scriptname, classname##methodname);";
        outputfile.write(regobjstring);

        # next class definition
        paramcount = paramcount + 1;
        
    outputfile.write("\n");
    outputfile.close();

    
# -----------------------------------------------------------------------------
def GenerateClasses(maxparamcount, outputfilename):
    print("Max param count: %d" % maxparamcount);
    print("GenerateClasses - Output: %s" % outputfilename);
    
    #open the output file
    outputfile = open(outputfilename, 'w');

    # -- add the MIT license to the top of the output file
    OutputLicense(outputfile);

    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("// Generated classes for function registration\n");
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("\n\n");

    outputfile.write('#include "TinRegistration.h"\n');
    outputfile.write('#include "TinVariableEntry.h"\n');
    outputfile.write('#include "TinFunctionEntry.h"\n');

    paramcount = 0;
    while (paramcount <= maxparamcount):
        outputfile.write("\n");
        outputfile.write("// -------------------\n");
        outputfile.write("// Parameter count: %d\n" % paramcount);
        outputfile.write("// -------------------\n");
        outputfile.write("\n");
        
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
            
        outputfile.write(template_string);
        outputfile.write("class CRegFunctionP%d : public CRegFunctionBase {\n" % paramcount);
        outputfile.write("public:\n");
        outputfile.write("\n");

        typedef_string = "    typedef R (*funcsignature)(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                typedef_string = typedef_string + ", ";
            typedef_string = typedef_string + "T%d p%d" % (i, i);
            i = i + 1;
        typedef_string = typedef_string + ");\n";
        outputfile.write(typedef_string);
        outputfile.write("\n");

        outputfile.write("    // -- CRegisterFunctionP%d\n" % paramcount);
        outputfile.write("    CRegFunctionP%d(const char* _funcname, funcsignature _funcptr) :\n" % paramcount);
        outputfile.write("                    CRegFunctionBase(\"\", _funcname) {\n");
        outputfile.write("        funcptr = _funcptr;\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- destructor\n");
        outputfile.write("    virtual ~CRegFunctionP%d() {\n" % paramcount);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void*) {\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
            
        dispatch_string = "        Dispatch(";
        if(paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                if (i > 1):
                    dispatch_string = dispatch_string + ",\n                 ";
                dispatch_string = dispatch_string + "ConvertVariableForDispatch<T%d>(ve%d)" % (i, i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    R Dispatch(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "T%d p%d" % (i, i);
            i = i + 1;
        dispatch_string = dispatch_string + ") {\n";
        outputfile.write(dispatch_string);
        
        functioncall = "        R r = funcptr(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                functioncall = functioncall + ", ";
            functioncall = functioncall + "p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("        assert(GetContext()->GetParameter(0));\n");
        outputfile.write("        CVariableEntry* returnval = GetContext()->GetParameter(0);\n");
        outputfile.write("        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));\n");
        outputfile.write("        return (r);\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register() {\n");
        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    funcsignature funcptr;\n");
        outputfile.write("};\n");
        outputfile.write("\n");
        
        # -----------------------------------------------------------------------------------------
        # repeat for the void specialized template

        template_string = "template<";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                template_string = template_string + ", ";
            template_string = template_string + "typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
            
        outputfile.write(template_string);
        classname_string = "class CRegFunctionP%d<void" % paramcount;
        i = 1;
        while (i <= paramcount):
            classname_string = classname_string + ", T%d" % i;
            i = i + 1;
        classname_string = classname_string + "> : public CRegFunctionBase {\n";
        outputfile.write(classname_string);
        outputfile.write("public:\n");
        outputfile.write("\n");

        typedef_string = "    typedef void (*funcsignature)(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                typedef_string = typedef_string + ", ";
            typedef_string = typedef_string + "T%d p%d" % (i, i);
            i = i + 1;
        typedef_string = typedef_string + ");\n";
        outputfile.write(typedef_string);
        outputfile.write("\n");

        outputfile.write("    // -- CRegisterFunctionP%d\n" % paramcount);
        outputfile.write("    CRegFunctionP%d(const char* _funcname, funcsignature _funcptr) :\n" % paramcount);
        outputfile.write("                    CRegFunctionBase(\"\", _funcname) {\n");
        outputfile.write("        funcptr = _funcptr;\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- destructor\n");
        outputfile.write("    virtual ~CRegFunctionP%d() {\n" % paramcount);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void*) {\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
            
        dispatch_string = "        Dispatch(";
        if(paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                if (i > 1):
                    dispatch_string = dispatch_string + ",\n                 ";
                dispatch_string = dispatch_string + "ConvertVariableForDispatch<T%d>(ve%d)" % (i, i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    void Dispatch(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "T%d p%d" % (i, i);
            i = i + 1;
        dispatch_string = dispatch_string + ") {\n";
        outputfile.write(dispatch_string);
        
        functioncall = "        funcptr(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                functioncall = functioncall + ", ";
            functioncall = functioncall + "p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register() {\n");
        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), TYPE_void, 1, 0);\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    funcsignature funcptr;\n");
        outputfile.write("};\n");
        outputfile.write("\n");

        # -----------------------------------------------------------------------------------------
        # repeat for the methods templates

        template_string = "template<typename C, typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);
        
        outputfile.write("class CRegMethodP%d : public CRegFunctionBase {\n" % paramcount);
        outputfile.write("public:\n");
        outputfile.write("\n");

        typedef_string = "    typedef R (*methodsignature)(C* c";
        i = 1;
        while (i <= paramcount):
            typedef_string = typedef_string + ", T%d p%d" % (i, i);
            i = i + 1;
        typedef_string = typedef_string + ");\n";
        outputfile.write(typedef_string);
        outputfile.write("\n");

        outputfile.write("    // -- CRegisterMethodP%d\n" % paramcount);
        outputfile.write("    CRegMethodP%d(const char* _funcname, methodsignature _funcptr) :\n" % paramcount);
        outputfile.write("                  CRegFunctionBase(__GetClassName<C>(), _funcname) {\n");
        outputfile.write("        funcptr = _funcptr;\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- destructor\n");
        outputfile.write("    virtual ~CRegMethodP%d() {\n" % paramcount);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void* objaddr) {\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
            
        dispatch_string = "        Dispatch(objaddr";
        if(paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                dispatch_string = dispatch_string + ",\n                 ";
                dispatch_string = dispatch_string + "ConvertVariableForDispatch<T%d>(ve%d)" % (i, i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    R Dispatch(void* objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", T%d p%d" % (i, i);
            i = i + 1;
        dispatch_string = dispatch_string + ") {\n";
        outputfile.write(dispatch_string);
        
        outputfile.write("        C* objptr = (C*)objaddr;\n");
        functioncall = "        R r = funcptr(objptr";
        i = 1;
        while (i <= paramcount):
            functioncall = functioncall + ", p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("        assert(GetContext()->GetParameter(0));\n");
        outputfile.write("        CVariableEntry* returnval = GetContext()->GetParameter(0);\n");
        outputfile.write("        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));\n");
        outputfile.write("        return (r);\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register() {\n");
        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    methodsignature funcptr;\n");
        outputfile.write("};\n");
        outputfile.write("\n");
        
        # -----------------------------------------------------------------------------------------
        # repeat for the void specialized methods

        template_string = "template<typename C";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);
        
        classname_string = "class CRegMethodP%d<C, void" % paramcount;
        i = 1;
        while (i <= paramcount):
            classname_string = classname_string + ", T%d" % i;
            i = i + 1;
        classname_string = classname_string + "> : public CRegFunctionBase {\n";
        outputfile.write(classname_string);
        
        outputfile.write("public:\n");
        outputfile.write("\n");

        typedef_string = "    typedef void (*methodsignature)(C* c";
        i = 1;
        while (i <= paramcount):
            typedef_string = typedef_string + ", T%d p%d" % (i, i);
            i = i + 1;
        typedef_string = typedef_string + ");\n";
        outputfile.write(typedef_string);
        outputfile.write("\n");

        outputfile.write("    // -- CRegisterMethodP%d\n" % paramcount);
        outputfile.write("    CRegMethodP%d(const char* _funcname, methodsignature _funcptr) :\n" % paramcount);
        outputfile.write("                  CRegFunctionBase(__GetClassName<C>(), _funcname) {\n");
        outputfile.write("        funcptr = _funcptr;\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- destructor\n");
        outputfile.write("    virtual ~CRegMethodP%d() {\n" % paramcount);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void* objaddr) {\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
            
        dispatch_string = "        Dispatch(objaddr";
        if(paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                dispatch_string = dispatch_string + ",\n                 ";
                dispatch_string = dispatch_string + "ConvertVariableForDispatch<T%d>(ve%d)" % (i, i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    void Dispatch(void* objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", T%d p%d" % (i, i);
            i = i + 1;
        dispatch_string = dispatch_string + ") {\n";
        outputfile.write(dispatch_string);
        
        outputfile.write("        C* objptr = (C*)objaddr;\n");
        functioncall = "        funcptr(objptr";
        i = 1;
        while (i <= paramcount):
            functioncall = functioncall + ", p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register() {\n");
        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), TYPE_void, 1, 0);\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    methodsignature funcptr;\n");
        outputfile.write("};\n");
        outputfile.write("\n");

        # -----------------------------------------------------------------------------------------
        # next class definition
        paramcount = paramcount + 1;
        
    outputfile.close();

# -----------------------------------------------------------------------------
def GenerateVariadicClasses(maxparamcount, outputfilename):
    print("GenerateVariadicClasses - Output: %s" % outputfilename);
    
    #open the output file
    outputfile = open(outputfilename, 'w');

    # -- add the MIT license to the top of the output file
    OutputLicense(outputfile);

    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("// Generated classes using variadic templates, for function registration\n");
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("\n\n");

    outputfile.write('#define REGISTER_FUNCTION(name, funcptr) \\\n');
    outputfile.write('    static const int gArgCount_##name = ::TinScript::SignatureArgCount<decltype(funcptr)>::arg_count; \\\n');
    outputfile.write('    static ::TinScript::CRegisterFunction<gArgCount_##name, decltype(funcptr)> gReg_##name(#name, funcptr);\n');
    outputfile.write("\n");

    outputfile.write("#if !PLATFORM_VS_2019\n");
    outputfile.write('    #define REGISTER_METHOD(classname, name, methodptr) \\\n');
    outputfile.write('        static const int gArgCount_##classname##_##name = ::TinScript::SignatureArgCount<decltype(std::declval<classname>().methodptr)>::arg_count; \\\n');
    outputfile.write('        static ::TinScript::CRegisterMethod<gArgCount_##classname##_##name, classname, decltype(std::declval<classname>().methodptr)> gReg_##classname##_##name(#name, &classname::methodptr);\n');
    outputfile.write("#else\n");
    outputfile.write('    #define REGISTER_METHOD(classname, name, methodptr) \\\n');
    outputfile.write('        static const int gArgCount_##classname##_##name = ::TinScript::MethodArgCount<decltype(&classname::methodptr)>::arg_count; \\\n');
    outputfile.write('        static ::TinScript::CRegisterMethod<gArgCount_##classname##_##name, classname, decltype(&classname::methodptr)> gReg_##classname##_##name(#name, &classname::methodptr);\n');
    outputfile.write("#endif\n");

    outputfile.write('#define REGISTER_CLASS_FUNCTION(classname, name, methodptr) \\\n');
    outputfile.write('    static const int gArgCount_##classname##_##name = ::TinScript::SignatureArgCount<decltype(std::declval<classname>().methodptr)>::arg_count; \\\n');
    outputfile.write('    static ::TinScript::CRegisterFunction<gArgCount_##classname##_##name, decltype(std::declval<classname>().methodptr)> gReg_##classname##_##name(#name, &classname::methodptr); \\\n');
    outputfile.write("\n");

    outputfile.write('template<typename S>\n');
    outputfile.write('class SignatureArgCount;\n\n');

    outputfile.write('template<typename R, typename... Args>\n');
    outputfile.write('class SignatureArgCount<R(Args...)>\n');
    outputfile.write('{\n');
    outputfile.write('    public:\n');
    outputfile.write('        static const int arg_count = sizeof...(Args);\n');
    outputfile.write('};\n\n');

    outputfile.write('template<typename S>\n');
    outputfile.write('class MethodArgCount;\n\n');

    outputfile.write('template<typename C, typename R, typename... Args>\n');
    outputfile.write('class MethodArgCount<R(C::*)(Args...)>\n');
    outputfile.write('{\n');
    outputfile.write('    public:\n');
    outputfile.write('        static const int arg_count = sizeof...(Args);\n');
    outputfile.write('};\n\n');

    outputfile.write('template<int N, typename S>\n');
    outputfile.write('class CRegisterFunction;\n\n');

    outputfile.write('template<int N, typename C, typename S>\n');
    outputfile.write('class CRegisterMethod;\n\n');

    paramcount = 0;
    while (paramcount <= maxparamcount):
        outputfile.write("\n");
        outputfile.write("// -------------------\n");
        outputfile.write("// Parameter count: %d\n" % paramcount);
        outputfile.write("// -------------------\n");
        outputfile.write("\n");

        outputfile.write("// -- class CRegisterFunction<%d, R(Args...)> ----------------------------------------\n\n" % paramcount);
        outputfile.write("template<typename R, typename... Args>\n");
        outputfile.write("class CRegisterFunction<%d, R(Args...)> : public CRegFunctionBase\n{\n" % paramcount);
        outputfile.write("public:\n");
        outputfile.write("\n");

        outputfile.write('    typedef R (*funcsignature)(Args...);\n');
        outputfile.write('    using argument_types = std::tuple<Args...>;\n\n');


        outputfile.write("    // -- constructor\n");


        outputfile.write('    CRegisterFunction(const char* _funcname, funcsignature _funcptr)\n');
        outputfile.write('        : CRegFunctionBase(\"\", _funcname)\n');
        outputfile.write('    {\n');
        outputfile.write('        funcptr = _funcptr;\n');
        outputfile.write('    }\n');
       
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void*)\n    {\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d p%d = ConvertVariableForDispatch<T%d>(ve%d);\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");
            
        dispatch_string = "        Dispatch(";
        if (paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                if (i > 1):
                    dispatch_string = dispatch_string + ", ";
                dispatch_string = dispatch_string + "&p%d" % (i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");

        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    R Dispatch(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "void* _p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ")\n    {\n";
        outputfile.write(dispatch_string);

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d* p%d = (T%d*)_p%d;\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");
        
        functioncall = "        R r = funcptr(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                functioncall = functioncall + ", ";
            functioncall = functioncall + "*p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("\n");

        outputfile.write("        assert(GetContext()->GetParameter(0));\n");
        outputfile.write("        CVariableEntry* returnval = GetContext()->GetParameter(0);\n");
        outputfile.write("        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));\n");
        outputfile.write("        return (r);\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register()\n    {\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("\n");
        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    funcsignature funcptr;\n");
        outputfile.write("};\n\n");
        
        # -----------------------------------------------------------------------------------------
        # repeat for the void specialized template

        outputfile.write("// -- class CRegisterFunction<%d, void(Args...)> ----------------------------------------\n\n" % paramcount);
        outputfile.write("template<typename... Args>\n");
        outputfile.write("class CRegisterFunction<%d, void(Args...)> : public CRegFunctionBase\n{\n" % paramcount);
        outputfile.write("public:\n");
        outputfile.write("\n");

        outputfile.write('    typedef void (*funcsignature)(Args...);\n');
        outputfile.write('    using argument_types = std::tuple<Args...>;\n\n');

        outputfile.write("    // -- constructor\n");
        outputfile.write('    CRegisterFunction(const char* _funcname, funcsignature _funcptr)\n');
        outputfile.write('        : CRegFunctionBase(\"\", _funcname)\n');
        outputfile.write('    {\n');
        outputfile.write('        funcptr = _funcptr;\n');
        outputfile.write('    }\n');
       
        outputfile.write("    // -- virtual DispatchFunction wrapper\n");
        outputfile.write("    virtual void DispatchFunction(void*)\n    {\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d p%d = ConvertVariableForDispatch<T%d>(ve%d);\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");
            
        dispatch_string = "        Dispatch(";
        if(paramcount == 0):
            dispatch_string = dispatch_string + ");\n"
        else:
            i = 1;
            while (i <= paramcount):
                if (i > 1):
                    dispatch_string = dispatch_string + ", ";
                dispatch_string = dispatch_string + "&p%d" % (i);
                i = i + 1;
            dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write("    }\n");
        outputfile.write("\n");

        outputfile.write("    // -- dispatch method\n");
        dispatch_string = "    void Dispatch(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "void* _p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ")\n    {\n";
        outputfile.write(dispatch_string);

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d* p%d = (T%d*)_p%d;\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");
        
        functioncall = "        funcptr(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                functioncall = functioncall + ", ";
            functioncall = functioncall + "*p%d" % i;
            i = i + 1;
        functioncall = functioncall + ");\n";
        outputfile.write(functioncall);
        outputfile.write("\n");

        outputfile.write("    }\n");
        outputfile.write("\n");
        
        outputfile.write("    // -- registration method\n");
        outputfile.write("    virtual void Register()\n    {\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), TYPE_void, 1, 0);\n");
        i = 1;
        while (i <= paramcount):
            outputfile.write("        fc->AddParameter(\"_p%d\", Hash(\"_p%d\"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write("    }\n");
        outputfile.write("\n");
        outputfile.write("private:\n");
        outputfile.write("    funcsignature funcptr;\n");
        outputfile.write("};\n");
        outputfile.write("\n");


        # -----------------------------------------------------------------------------------------
        # repeat for the methods templates

        outputfile.write("// -- class CRegisterMethod<%d, R(Args...)> ----------------------------------------\n\n" % paramcount);
        outputfile.write('template<typename C, typename R, typename... Args>\n');
        outputfile.write('#if !PLATFORM_VS_2019\n');
        outputfile.write('class CRegisterMethod<%d, C, R(Args...)> : public CRegFunctionBase\n{\n' % paramcount);
        outputfile.write('#else\n');
        outputfile.write('class CRegisterMethod<%d, C, R(C::*)(Args...)> : public CRegFunctionBase\n{\n' % paramcount);
        outputfile.write('#endif\n');
        outputfile.write('public:\n');
        outputfile.write('\n');
        outputfile.write('    using argument_types = std::tuple<Args...>;\n');
        outputfile.write('    typedef R (C::*methodsignature)(Args...);\n');
        outputfile.write('\n');
        outputfile.write('    // -- CRegisterMethod\n');
        outputfile.write('    CRegisterMethod(const char* _methodname, methodsignature _methodptr)\n');
        outputfile.write('        : CRegFunctionBase(__GetClassName<C>(), _methodname)\n');
        outputfile.write('    {\n');
        outputfile.write('        methodptr = _methodptr;\n');
        outputfile.write('    }\n');
        outputfile.write('\n');
        outputfile.write('    // -- virtual DispatchFunction wrapper\n');
        outputfile.write('    virtual void DispatchFunction(void* objaddr)\n    {\n');
        outputfile.write('\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d p%d = ConvertVariableForDispatch<T%d>(ve%d);\n" % (i, i, i, i));
            i = i + 1;

        dispatch_string = "        Dispatch(objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", &p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write('    }\n\n');


        outputfile.write('    // -- dispatch method\n');

        dispatch_string = "    R Dispatch(void* objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", void* _p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ")\n";
        outputfile.write(dispatch_string);
        outputfile.write('    {\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d* p%d = (T%d*)_p%d;\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write('        C* object = (C*)(objaddr);\n');

        dispatch_string = "        R r = (object->*methodptr)(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "*p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write('\n');

        outputfile.write('        assert(GetContext()->GetParameter(0));\n');
        outputfile.write('        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);\n');
        outputfile.write('        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));\n');
        outputfile.write('        return (r);\n');
        outputfile.write('    }\n');
        outputfile.write('\n');

        outputfile.write('    // -- registration method\n');
        outputfile.write('    virtual void Register()\n');
        outputfile.write('    {\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write('        fc->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write('        fc->AddParameter("_p%d", Hash("_p%d"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n' % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write('    }\n');
        outputfile.write('\n');
        outputfile.write('private:\n');
        outputfile.write('    methodsignature methodptr;\n');
        outputfile.write('};\n');

        # -----------------------------------------------------------------------------------------
        # repeat for the void methods templates

        outputfile.write("// -- class CRegisterMethod<%d, void(Args...)> ----------------------------------------\n\n" % paramcount);
        outputfile.write('template<typename C, typename... Args>\n');
        outputfile.write('#if !PLATFORM_VS_2019\n');
        outputfile.write('class CRegisterMethod<%d, C, void(Args...)> : public CRegFunctionBase\n{\n' % paramcount);
        outputfile.write('#else\n');
        outputfile.write('class CRegisterMethod<%d, C, void(C::*)(Args...)> : public CRegFunctionBase\n{\n' % paramcount);
        outputfile.write('#endif\n');
        outputfile.write('public:\n');
        outputfile.write('\n');
        outputfile.write('    using argument_types = std::tuple<Args...>;\n');
        outputfile.write('    typedef void (C::*methodsignature)(Args...);\n');
        outputfile.write('\n');
        outputfile.write('    // -- CRegisterMethod\n');
        outputfile.write('    CRegisterMethod(const char* _methodname, methodsignature _methodptr)\n');
        outputfile.write('        : CRegFunctionBase(__GetClassName<C>(), _methodname)\n');
        outputfile.write('    {\n');
        outputfile.write('        methodptr = _methodptr;\n');
        outputfile.write('    }\n');
        outputfile.write('\n');
        outputfile.write('    // -- virtual DispatchFunction wrapper\n');
        outputfile.write('    virtual void DispatchFunction(void* objaddr)\n    {\n');
        outputfile.write('\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        CVariableEntry* ve%d = GetContext()->GetParameter(%d);\n" % (i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d p%d = ConvertVariableForDispatch<T%d>(ve%d);\n" % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        dispatch_string = "        Dispatch(objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", &p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write('    }\n\n');

        outputfile.write('    // -- dispatch method\n');

        dispatch_string = "    void Dispatch(void* objaddr";
        i = 1;
        while (i <= paramcount):
            dispatch_string = dispatch_string + ", void* _p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ")\n";
        outputfile.write(dispatch_string);
        outputfile.write('    {\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("        T%d* p%d = (T%d*)_p%d;\n" % (i, i, i, i));
            i = i + 1;
        outputfile.write("\n");

        outputfile.write('        C* object = (C*)(objaddr);\n');

        dispatch_string = "        (object->*methodptr)(";
        i = 1;
        while (i <= paramcount):
            if (i > 1):
                dispatch_string = dispatch_string + ", ";
            dispatch_string = dispatch_string + "*p%d" % (i);
            i = i + 1;
        dispatch_string = dispatch_string + ");\n";
        outputfile.write(dispatch_string);
        outputfile.write('\n');

        outputfile.write('    }\n');
        outputfile.write('\n');

        outputfile.write('    // -- registration method\n');
        outputfile.write('    virtual void Register()\n');
        outputfile.write('    {\n');

        i = 1;
        while (i <= paramcount):
            outputfile.write("        using T%d = std::tuple_element<%d, argument_types>::type;\n" % (i, i - 1));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write('\n');
        outputfile.write("        CFunctionContext* fc = CreateContext();\n");
        outputfile.write("        fc->AddParameter(\"__return\", Hash(\"__return\"), TYPE_void, 1, 0);\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write('        fc->AddParameter("_p%d", Hash("_p%d"), GetRegisteredType(GetTypeID<T%d>()), 1, GetTypeID<T%d>());\n' % (i, i, i, i));
            i = i + 1;
        if (paramcount > 0):
            outputfile.write("\n");

        outputfile.write('\n');
        outputfile.write('    }\n');
        outputfile.write('\n');
        outputfile.write('private:\n');
        outputfile.write('    methodsignature methodptr;\n');
        outputfile.write('};\n');

        # -----------------------------------------------------------------------------------------
        # next class definition
        paramcount = paramcount + 1;
        
    outputfile.close();

# -----------------------------------------------------------------------------
def GenerateExecs(maxparamcount, outputfilename):
    
    print("GenerateMacros - Output: %s" % outputfilename);

    #open the output file
    outputfile = open(outputfilename, 'w');

    # -- add the MIT license to the top of the output file
    OutputLicense(outputfile);
    
    outputfile.write("// ------------------------------------------------------------------------------------------------\n");
    outputfile.write("// Generated interface for calling scripted functions from code\n");
    outputfile.write("// ------------------------------------------------------------------------------------------------\n\n");

    outputfile.write("#ifndef __REGISTRATIONEXECS_H\n");
    outputfile.write("#define __REGISTRATIONEXECS_H\n\n");

    outputfile.write('#include "TinVariableEntry.h"\n');
    outputfile.write('#include "TinExecute.h"\n\n');

    outputfile.write("namespace TinScript\n");
    outputfile.write("{\n");

    paramcount = 0;
    while (paramcount <= maxparamcount):
        outputfile.write("\n\n// -- Parameter count: %d\n" % paramcount);

        # -- function wrapper, given the unhashed function name
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ExecFunction(R& return_value, const char* func_name"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])\n");
        outputfile.write("        return false;\n\n");

        call_string = "    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name)"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- function wrapper, given the function hash
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ExecFunction(R& return_value, uint32 func_hash"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace())\n");
        outputfile.write("        return false;\n\n");
        call_string = "    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- object method wrapper, given the object address, and the unhashed method name
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])\n");
        outputfile.write("        return false;\n\n");
        outputfile.write("    uint32 object_id = script_context->FindIDByAddress(obj_addr);\n");
        outputfile.write("    if (object_id == 0)\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\\n", kPointerToUInt32(obj_addr));\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");
        call_string = "    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name)"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- object method wrapper, given the object address, and the method hash
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace())\n");
        outputfile.write("        return false;\n\n");
        outputfile.write("    uint32 object_id = script_context->FindIDByAddress(obj_addr);\n");
        outputfile.write("    if (object_id == 0)\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\\n", kPointerToUInt32(obj_addr));\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");
        call_string = "    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- object method wrapper, given the object address, the specific namespace hash, and the method hash
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        call_string = "    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- object method wrapper, given the object ID, and the unhashed method name
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");
        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])\n");
        outputfile.write("        return false;\n\n");
        call_string = "    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name)"
        i = 1;
        while (i <= paramcount):
            call_string = call_string + ", p%d" % i;
            i = i + 1;
        call_string = call_string + "));\n";
        outputfile.write(call_string);
        outputfile.write("}\n\n");

        # -- the actual implmenentation
        template_string = "template<typename R";
        i = 1;
        while (i <= paramcount):
            template_string = template_string + ", typename T%d" % i;
            i = i + 1;
        template_string = template_string + ">\n";
        outputfile.write(template_string);

        function_string = "inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash"
        i = 1;
        while (i <= paramcount):
            function_string = function_string + ", T%d p%d" % (i, i);
            i = i + 1;
        function_string = function_string + ")\n";
        outputfile.write(function_string);
        outputfile.write("{\n");

        outputfile.write("    CScriptContext* script_context = TinScript::GetContext();\n");
        outputfile.write("    if (!script_context->GetGlobalNamespace())\n");
        outputfile.write("        return false;\n\n");

        outputfile.write("    // -- get the object, if one was required\n");
        outputfile.write("    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;\n");
        outputfile.write("    if (!oe && object_id > 0)\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\\n", object_id);\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");

        outputfile.write("    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)\n");
        outputfile.write("                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);\n");
        outputfile.write("    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;\n");
        outputfile.write("    if (!fe || !return_ve)\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\\n", UnHash(func_hash));\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");

        outputfile.write("    // -- see if we can recognize an appropriate type\n");
        outputfile.write("    eVarType returntype = GetRegisteredType(GetTypeID<R>());\n");
        outputfile.write("    if (returntype == TYPE_NULL)\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\\n");\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");


        outputfile.write("    // -- fill in the parameters\n");
        outputfile.write("    if (fe->GetContext()->GetParameterCount() < %d)\n" % paramcount);
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());\n');
        outputfile.write("        return (false);\n");
        outputfile.write("    }\n\n");

        i = 1;
        while (i <= paramcount):
            outputfile.write("    CVariableEntry* ve_p%d = fe->GetContext()->GetParameter(%d);\n" % (i, i));
            outputfile.write("    void* p%d_convert_addr = NULL;\n" % i);
            outputfile.write("    if (GetRegisteredType(GetTypeID<T%d>()) == TYPE_string)\n" % i);
            outputfile.write("    {\n");
            outputfile.write("        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32\n");
            outputfile.write("        void** p%d_ptr_ptr = (void**)(&p%d);\n" % (i, i));
            outputfile.write("        void* p%d_ptr = (void*)(*p%d_ptr_ptr);\n" % (i, i));
            outputfile.write("        p%d_convert_addr = TypeConvert(script_context, TYPE_string, p%d_ptr, ve_p%d->GetType());\n" % (i, i, i));
            outputfile.write("    }\n");
            outputfile.write("    else\n");
            outputfile.write("        p%d_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T%d>()), (void*)&p%d, ve_p%d->GetType());\n" % (i, i, i, i));
            outputfile.write("    if (!p%d_convert_addr)\n" % i);
            outputfile.write("    {\n");
            outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %%s() unable to convert parameter %d\\n", UnHash(func_hash));\n' % i);
            outputfile.write("        return false;\n");
            outputfile.write("    }\n\n");
            outputfile.write("    ve_p%d->SetValueAddr(NULL, p%d_convert_addr);\n\n" % (i, i));
            i = i + 1;

        outputfile.write("    // -- execute the function\n");
        outputfile.write("    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))\n");
        outputfile.write("    {\n");
        outputfile.write('        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\\n", UnHash(func_hash));\n');
        outputfile.write("        return false;\n");
        outputfile.write("    }\n\n");

        outputfile.write("    // -- return true if we're able to convert to the return type requested\n");
        outputfile.write("    return (ReturnExecfResult(script_context, return_value));\n");
        outputfile.write("}\n");

        # -----------------------------------------------------------------------------------------
        # next class definition
        paramcount = paramcount + 1;
        
    # -- close the file
    outputfile.write("} // TinScript\n\n");
    outputfile.write("#endif // __REGISTRATIONEXECS_H\n");
    outputfile.close();
        
# -----------------------------------------------------------------------------
def main():

    print ("\n***********************************");
    print ("*  Generate registration classes  *");
    print ("***********************************\n");
    
    classesfilename = "registrationclasses.h";
    macrosfilename = "registrationmacros.h";
    execsfilename = "registrationexecs.h";

    variadicfilename = "variadicclasses.h";

    maxparamcount = 8;
    
    # parse the command line arguments
    argcount = len(sys.argv);
    printhelp = (argcount <= 1);
    currentarg = 1;
    while (currentarg < argcount):
        if (sys.argv[currentarg] == "-h" or sys.argv[currentarg] == "-help" or sys.argv[currentarg] == "-?"):
            printhelp = 1;
            currentarg = argcount;
        elif (sys.argv[currentarg] == "-maxparam"):
            if(currentarg + 1 >= argcount):
                printhelp = 1;
                currentarg = argcount;
            else:
                maxparamcount = int(sys.argv[currentarg + 1]);
                currentarg = currentarg + 2;
            
        elif (sys.argv[currentarg] == "-oc" or sys.argv[currentarg] == "-outputclasses"):
            if(currentarg + 1 >= argcount):
                printhelp = 1;
                currentarg = argcount;
            else:
                classesfilename = sys.argv[currentarg + 1];
                currentarg = currentarg + 2;
        
        elif (sys.argv[currentarg] == "-om" or sys.argv[currentarg] == "-outputmacros"):
            if(currentarg + 1 >= argcount):
                printhelp = 1;
                currentarg = argcount;
            else:
                macrosfilename = sys.argv[currentarg + 1];
                currentarg = currentarg + 2;
        
        else:
            printhelp = 1;
            break;
        
    if (printhelp == 1):
        help_width = 36;
        print ("Usage:  python genregclasses.py [option]");
        print (str("-h | -help").rjust(help_width) + " :  " + str("This help menu."));
        print (str("-maxparam <count>").rjust(help_width) + " :  " + str("maximum number or parameters"));
        print (str("-oc | -outputclasses <filename>").rjust(help_width) + " :  " + str("templated classes output file."));
        print (str("-om | -outputmacros <filename>").rjust(help_width) + " :  " + str("macros output file."));
        print (str("-oe | -outputexecs <filename>").rjust(help_width) + " :  " + str("execs output file."));
        exit(0);
        
    else:
        GenerateClasses(maxparamcount, classesfilename);
        GenerateMacros(maxparamcount, macrosfilename);
        GenerateExecs(maxparamcount, execsfilename);
        GenerateVariadicClasses(maxparamcount, variadicfilename);
        
    print ("\n**********************************");
    print ("*  Finished generating classes.  *");
    print ("**********************************\n");

# -----------------------------------------------------------------------------
# start of the script execution

main(); 
exit();

# -----------------------------------------------------------------------------
# EOF
# -----------------------------------------------------------------------------
