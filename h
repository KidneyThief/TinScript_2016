[33mcommit a3aef37667b38792a0d03d879b33c47f5d28084b[m[33m ([m[1;36mHEAD -> [m[1;32mchanges[m[33m)[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Oct 9 12:03:46 2021 -0700

    ObjectInspector is now (properly) populated with the selected ID from the browser

[33mcommit 20dfb4f1290db6f522eb39dfed1c4f1680233075[m[33m ([m[1;31morigin/master[m[33m, [m[1;31morigin/HEAD[m[33m)[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Oct 9 11:36:46 2021 -0700

    Watch Expressions are re-sent instead of duplicated

[33mcommit 8049e6a79371bb35bdbe0e310c024b9b68493440[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Fri Oct 8 20:27:50 2021 -0700

    Fixed Watch Expression to work live, and for both objects and expressions

[33mcommit 78f0e29b806da197a4c234538baf569fdc50db15[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Fri Oct 8 11:34:51 2021 -0700

    Compiled line numbers re-worked, breakpoints and tracepoints reliably break on the first instruction for the line

[33mcommit 200f15a20431a12eb781dfbf85ad6d49a65cda1a[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Thu Oct 7 12:56:54 2021 -0700

    TinQtConsole now uses '/' to issue commands Locally, also LocalTabComplete()

[33mcommit 51d65b359e0bcae9e173706fc38b643bcd8a1a7f[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Wed Oct 6 19:10:50 2021 -0700

    Debugger will now load the string table from the .exe target directory

[33mcommit 80584c71b87eb9c176e777f652576fbe3b1b3a9a[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Wed Oct 6 17:39:05 2021 -0700

    All codeblock hashes are now of the full path, coordinated with setting breakpoints, etc...

[33mcommit 04bbd88f3ac6a00b91cd225bc4cd05f5faa50ce6[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Wed Oct 6 11:16:50 2021 -0700

    Fixed tSocketHeader size for 32-bit, added SetDirectory()
    
    Named the objects in UnitTest

[33mcommit cedff692eb9d3b391ccaddd42bf18c2bac7eb3ef[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Mon Oct 4 15:01:51 2021 -0700

    Minor fix to QtConsole...

[33mcommit c5070823a90105f730fa54d9b4cd6ce5cbcc052a[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Mon Oct 4 11:25:23 2021 -0700

    Slight file re-org, RegFunction objects now use thread CScriptContext, multithreading now works
    
    Fixed the SocketExec() creation of VariableEntry parameters from the debugger thread

[33mcommit b24ecd01879a1dd041f5d9321a68a96f91587474[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Thu Sep 30 12:14:29 2021 -0700

    ListFunctions() now dumps the full signature

[33mcommit ed9d6d2ce71e243b3febe04675647910878dfa1f[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Wed Sep 29 20:00:06 2021 -0700

    FunctionAssist - creation callstack now displayed in the parameters tab

[33mcommit 2efd87a946db6b6539add52c7b5aa509508d5fbb[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Wed Sep 29 15:38:59 2021 -0700

    FunctionAssist - added Refresh button

[33mcommit c5d039b37432c860e0d0016319300cda1490b216[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Wed Sep 29 15:13:55 2021 -0700

    DebuggerNotifyCreateObject() now sends the entire origin stack, and uses a new socket packet

[33mcommit 8d3f2ec29b5ea148019b858c782a5a0c597ffbb7[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Tue Sep 28 11:27:39 2021 -0700

    Function Assist - added method source

[33mcommit 8f6ca20bd1d26028320592f7269eda0e2d8f913c[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Tue Sep 28 09:59:07 2021 -0700

    Assist panel improved - added DebuggerListObjectsComplete()

[33mcommit c01d414f799447cbce4543770c18766fac0e7722[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Mon Sep 27 17:53:14 2021 -0700

    Small fixes to the FunctionAssist - now a more vertical layout

[33mcommit 8cc3149508d093d4ebe2c1612e69135777e6165e[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Mon Sep 27 16:25:13 2021 -0700

    DebuggerSendWatchVariable() had a bad strcpy length

[33mcommit 107c69d53bff6dd2b2b7154a0323d2db096a1824[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Mon Sep 27 15:20:36 2021 -0700

    Object Inspect window - fixed formatting

[33mcommit 3a07f74a3f324c3264137e979dab89dfffa0e354[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Sun Sep 26 16:55:50 2021 -0700

    Fixed ToolPalette formatting, CSchedule::InsertCommand() now maintains order when dispatch time is the same

[33mcommit eaca66560b6924b59467f65c7951ad5a797b1e7a[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Sun Sep 26 12:59:31 2021 -0700

    Fixed formatting on Scheduler and Console Input

[33mcommit 8b3fc3fd0dccbfb511b70cd84afa229dc107a8db[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 24 17:43:39 2021 -0700

    Fixed demotools (ToolPalette) to use SocketExec()

[33mcommit 7512011b1e002a18818577be34ecff45e76124d1[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 24 17:35:09 2021 -0700

    First param __return should be initialized to null, fixed SocketExec()

[33mcommit d43c3a5cbe94a634d914a707588d5b9a0563c1c1[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 24 16:45:55 2021 -0700

    Updating REGISTER_METHOD

[33mcommit 0cd97ceca786d701ed95394771f667b44e2daa48[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 24 16:24:28 2021 -0700

    Converted all registered functions to variadic

[33mcommit 796c3af25f7516f3fd69fceed2f42e2dd698b361[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 24 11:51:36 2021 -0700

    Updated genregclasses.py, including minor formatting

[33mcommit dda2cafe8c268369084866970b5c795973c9c408[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Thu Sep 23 18:37:45 2021 -0700

    Added REGISTER_CLASS_FUNCTION(), converted mathutil.cpp to variadic registration

[33mcommit c3ac4cb3a3383c141344c9ee4b8936d373a5fd89[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Thu Sep 23 11:31:32 2021 -0700

    Added SetDebugForceCompile() to compile, even if the .tso is up to date

[33mcommit 4c5b3a5ebf3a0b02f0732150c80469548a8e968c[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Wed Sep 22 19:27:58 2021 -0700

    Fixed array indexing at 0, UnitTests succeeded

[33mcommit 3859a6e5e3f245154ee2a1bd84178a844503192b[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Tue Sep 21 16:51:42 2021 -0700

    Removed unnecessary assert that triggered in multi-thread test

[33mcommit a959b1893a5b24f1dfef55caf642f9f372525b87[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Tue Sep 21 16:32:12 2021 -0700

    Debug prints removed

[33mcommit 9a59e98d8d201716d8367c85b455ca2ec060f833[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Tue Sep 21 16:25:47 2021 -0700

    IsStackVariable() fixed for arrays and hashtables, UnitTests execute successfully

[33mcommit 9ce85716fa705775d79e50bc1bc4467027b0e666[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Fri Sep 17 18:17:13 2021 -0700

    Fixed SafeStrcpy, fixed TryParseComment to properly update linenumber, removed incomplete check for return node

[33mcommit d0128084c872bfc65bae6ad9d632b1f985b24a05[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Thu Sep 16 17:43:43 2021 -0700

    A few more UI alignment polish fixes, removed kFontHeight

[33mcommit 872504b64682e863d6f6957877a143de468b3fcd[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Sun Sep 12 15:40:06 2021 -0700

    TinQTDemo:  updated font heights, create title widgets

[33mcommit eea0a2a0e0c7b41867ab3513bdb694fdfd57ce62[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Sat Sep 11 14:09:17 2021 -0700

    Updated use of SafeStrcpy to include sizeof(dest)

[33mcommit 2a86605de5811306e705cbe7e6d1c204cc93e3b8[m
Author: kidneythief <kidneythief9@gmail.com>
Date:   Sat Sep 11 14:06:12 2021 -0700

    Updated .gitignore for *.exe

[33mcommit a42c50830a0237e0408d2290a57c12fc01756a8c[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Wed Feb 19 17:39:04 2020 -0800

    added type(), returns the string of the type of variable

[33mcommit c2381c90f621a111694f0e5e7269f2807870a70c[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Wed Feb 19 16:12:26 2020 -0800

    Added hashtable_first() and hashtable_next()
    
    added hashtable_end(), can now use a while loop to iterate through the values of a hashtable

[33mcommit ea1680dbad68a772192681e917a69e97769cd357[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Feb 15 17:12:32 2020 -0800

    Added hashtable_haskey()

[33mcommit 4d6c8d8d2ceb255c1974787d8a39d6a69d78a258[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Feb 9 15:55:21 2020 -0800

    Added registered HasMember/Method/Namespace() methods

[33mcommit e03a3df931410a67c2259e66b9e9f4f87f52a29e[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Feb 9 15:13:54 2020 -0800

    string cmp through ==, SafeStrCpy() now checks dest buffer size, ListFunctions/Methods/Members() now accepts partial strings

[33mcommit f02e7ed1afa184b4611a2da1a7d815bd4a019fe7[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Jan 26 17:57:32 2020 -0800

    Added ListNamespaces(), fixed bug with SchedParam using TYPE_hashtable

[33mcommit 4a6eca780e77a3fcd4a75bd4cd612e5ea21d4660[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Mon Jan 20 17:40:32 2020 -0800

    Can now dynamically add members via object_id e.g. int 1.foo = 9;

[33mcommit 81ce8c40bcb6abf29d73cb75301e119b319309b2[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Dec 24 15:18:19 2017 -0800

    CFunctionContext::CalculateHash() added

[33mcommit dd7ca6d31d67931c00766024b761aa8647c1af92[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Dec 3 16:02:45 2017 -0800

    CompileToC():  Added support for 'if' statements

[33mcommit f54d503a2c535a524d91ea83626187132529ca0a[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Oct 29 18:38:15 2016 -0700

    * CompileToC():  UnaryOpNode now supported

[33mcommit ec6a2ed4d061fa98fd255828e407218698018119[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Oct 29 17:28:20 2016 -0700

    *  CompileToC():  FuncDeclNode now outputs the local var table

[33mcommit a63d22b57ec05997a23251ac1ecf06d1fef58d2f[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Oct 23 15:50:55 2016 -0700

    *  COmment nodes added to the parse tree, comments written out during CompileToC()

[33mcommit 0ef1f59df346af3e69abcc6886d79614b53f0f0e[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sat Oct 22 18:20:24 2016 -0700

    WIP CompileToC(), preliminary pass on CFuncReturnNode()

[33mcommit c22b2051ea6704ba68c30e7f50728e4e315f8123[m
Author: Tim Andersen <kidneythief9@gmail.com>
Date:   Sun Oct 16 12:35:13 2016 -0700

    Beginning of CompileToC() pipeline, need to implement for all nodes

[33mcommit f38d31ebe8bebf260a4a04eac8f089176acfa86a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Oct 9 00:33:35 2016 -0700

    *  Apparently there's a DrawText() already defined somewhere? Registering  DrawScreenText() works fine.

[33mcommit 10473ca0f9ed31f0d75671b8761473369af83276[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Oct 8 17:44:36 2016 -0700

    *  TinQtDemo now uses the new registration macros - apparently const char* isn't yet supported.

[33mcommit 4d846378d35f94e0d266900ccee24d494c1d6a39[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Oct 8 16:44:40 2016 -0700

    *  All method and function registrations are using the new REGISTRATION_FUNCTION/METHOD() macros from the variadicclasses.h

[33mcommit 06c567db55f02cee2ac4dd9e05835031b17a3fa9[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Sep 25 17:09:52 2016 -0700

    *  Resolved a namespace conflict, allowing the REGISTER_FUNCTION() macro to be used outside namespace TinScript....
    *  Still a problem if I try to use REGISTER_METHOD() for CVector3::Length()

[33mcommit 3e3dea95cdbf133a731b34d549c977b7809b2bd7[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Sep 25 16:48:28 2016 -0700

    *  New registration macros complete!

[33mcommit 70145049105cc23a44f609dacac77b54e79bc4c0[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Sep 25 15:09:13 2016 -0700

    *  First pass at variadic templates for the registration macros (that don't require explicitly identifying each argument type)

[33mcommit 4e2a007ffef403afc919a4856335f9bc599e4c2f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jul 17 16:58:53 2016 -0700

    *  Added a Hash/Unhash line edit to the TinQt console

[33mcommit 3457a91b662ad6066321e19365deb5c2774a131e[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri Jul 1 17:06:15 2016 -0700

    *  IsArray() reworked to be more precise (differentiating between hashtables, array parameters, etc...)
    *  TinExecute.h/cpp has been updated for a better coding standard
    *  Return value of GetStackValue() now allows either a non-null value address, or a non-null variable

[33mcommit 33471b61e250973d2d4c68caaeadeb1190c7b5ca[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri Jul 1 15:40:39 2016 -0700

    *  Using SocketExec() to remotely execute functions, the string arg is only refcounted if the parameter type of the function is actually a string.

[33mcommit 250c32ee4c6dbff7b088e1ca5f9098e87cf98ec4[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri Jul 1 01:20:23 2016 -0700

    *  Added a "Function Signature" Cache, so when a SendExec() remote function exec is received by the target, and the arguments are not optimally typed (e.g. all strings probably), we send back the actual function signature, so the sender can pack them more appropriately.
    *  SendExec() creates scheduled commands - but does so from the socket thread.  As such, they're not added by the CMemoryTracker when created - but they are freed from the main thread, and therefore, not found in the memory tracker either - causing an invalid assert.
    *  Minor fix to pulling stack values - if the value couldn't be found (from a bad SendExec(), we now return false, allowing an assert, instead of a nullptr deref.

[33mcommit 8b694792615480d76f131e4ebcd0dc568e540936[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jun 26 18:05:45 2016 -0700

    *  Double-clicking in the ObjectBrowser now sets the objectID in the Function Assist Win - also supports the new "Created" button, to find out where an object originated.

[33mcommit 03d9ebd79a01f1d99da02b6ca56ae81bce3b21b3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jun 26 02:01:23 2016 -0700

    *  TinQt Console is now hooked up to the memory tracker, finding script file/line create instruction for an object.  (WIP)

[33mcommit 343c525ad5ae6a9441384fd41faec96ef609c638[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Jun 25 14:45:51 2016 -0700

    *  Memory tracker update, calculating the object create file/line more accurately

[33mcommit 2e49e3a8f959cacb0f9baf02ccb380b5216aee0f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Thu Jun 23 00:00:57 2016 -0700

    *  Minor fix - with the Type Conversion methods (StringToX, XToString) becoming threadsafe, the TinQtConsole had one call needing to be updated.

[33mcommit 812657cf1ae9072531f13cfd3ebe5315d6f905c9[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Wed Jun 22 23:56:33 2016 -0700

    *  SocketExec(), an optimized form of SocketCommand() added - directly queues a scheduled command, without having to parse, compile, and execute through the virtual machine.
    *  worms.ts has been updated to use SocketExec()
    *  There was a crash bug in executing a scheduled command, if the value was not able to be converted - fixed!  (last known bug!)

[33mcommit cba525a8cf99808a577bc9e5cd3761a82350403f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jun 5 17:24:08 2016 -0700

    *  CMemoryTracker now tracks array allocations, and object created file/line locations

[33mcommit 34b438d10291138d83948a4e280eb8d3cea846f8[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jun 5 01:54:23 2016 -0700

    *  First pass at the CMemoryTracker, *and* it uncovered the memory leak in TinCompile.cpp

[33mcommit 7332ae88aa872958ed4007c202664689cc292de9[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Jun 4 15:28:43 2016 -0700

    *  Fixed the assign = ternary parsing
    *  Fixed switch statements to not "fall through", if no case matches, and there is no default
    *  Ugh ancient bug with compare !=, affecting types bool and object...  fixed.

[33mcommit ee25be81c07ef2451fcfca43e077334491da0d47[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 31 23:09:16 2016 -0700

    *  worms.ts:  ties handled, as well as disconnects...

[33mcommit 565cc90f8e6e3e89a5baef0472a71de30a44c67d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 31 22:03:00 2016 -0700

    *  worms.ts:  Added score keeping (test)

[33mcommit d9226b3c8a9ef9de8c492173db93aa0f744328eb[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 31 21:37:33 2016 -0700

    *  worms.ts:  A smoother ending to the game, press <return> for rematch

[33mcommit 26f47270447858af7e4ba10fe0f16d7f327e2c1a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 31 20:09:26 2016 -0700

    *  worms.ts  removed server bias from player collision

[33mcommit 6990d70ac456fe725b9c69c60d1680330fdc4784[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 30 23:11:47 2016 -0700

    *  Worms.ts

[33mcommit ce32b6642c5e3d6dd4c507e9d5d17cd710462ef9[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 30 22:45:16 2016 -0700

    *  worms.ts

[33mcommit b30268673ed756a1d5c816dcf48848eb2ca7b2cb[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 30 22:36:31 2016 -0700

    *  worms.ts:  Now with a variable number of apples

[33mcommit 92bf21b62205004b8b137eb252408a21d340ea48[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 30 20:54:13 2016 -0700

    *  Worms:  Adding minor network confirmation...

[33mcommit d8287456702babf67c2b1397ac2457d1f0300336[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 30 20:13:17 2016 -0700

    *  worms.ts:  adding update packet confirmation

[33mcommit a15d8cc2e178f69deb225adb457cdbad93543c51[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 29 22:53:20 2016 -0700

    *  Working version of worms.ts
    *  Added SocketIsConnected() registered function.

[33mcommit a1a97755faa4e44dfd6a8aa913f84c68e5178e5d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 29 22:41:45 2016 -0700

    *  Worms.ts

[33mcommit a61791e700b41097e403684c1fda6fc8f20e6a81[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 29 22:36:31 2016 -0700

    *  Worms, take 2

[33mcommit be94429497394d02b5d7b1f5a1bbe586f4f516f8[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 29 21:48:26 2016 -0700

    *  Worms.ts   testing network

[33mcommit 8afb3a10e1d8d1464dc023576790b9eefe1a39de[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 29 17:36:55 2016 -0700

    *  First implementation of Worms

[33mcommit 2395fec6e75987caf88f618310ca526594f8e2e4[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Thu May 26 21:44:07 2016 -0700

    *  Switch statement "default" wasn't hooked up properly - fixed.

[33mcommit 7867833c36fd39c4aa96fd7aa1f98a6e3eb3fd42[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Thu May 26 20:03:34 2016 -0700

    *  Switch statements now supported!

[33mcommit 2971194aa7034273b7ab5433568d3c6a8489e28d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 23 21:39:06 2016 -0700

    *  Ternary op now implemented...  requires the usage of parenthesis if POD members are involved.

[33mcommit 79032f0415aeb1f4370e2e4e9ebb050d8d20198a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 23:52:29 2016 -0700

    *  Minor update, do..while loops now require both brace delineated statement blocks, and an end-of-statement semicolon.
    *  Did I mention while() loops now work with pre/post increment ops?

[33mcommit b6cb94263f9438ce5254ff662e7c149accde2ec3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 23:06:37 2016 -0700

    *  BinaryOpNode now checks both sides for an assign op, and re-pushes the assigned value as needed.  Allows conditionals and unary-pre/post ops to cooperate.

[33mcommit 4fd612fef7ab370cb65c3c287e7885bda1b94b1c[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 22:14:06 2016 -0700

    *  Working on do..while() loops....  while loops with unary increment is currently broken

[33mcommit e69fcf397515ac7b551f4b54dbb57a56421e8bb2[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 15:08:59 2016 -0700

    *  TooPalette entries now use a named dictionary for SetValue(), SetDescription() - allows the use of SocketCommand() from the client.

[33mcommit 502f5e587600f8b474c69322b5ade4e3f6dfff8d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 01:27:45 2016 -0700

    *  After registering all the KeyCodes, bumped the global var and global function table sizes

[33mcommit dde609576840d0dbdc88dbf131932cd473ac171b[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 22 01:10:56 2016 -0700

    *  Updated the TinQt Demo key inputs to be either event or query based.  Now uses keyPressEvent() to handle all keys, rather than shortcuts for just a handful.

[33mcommit 1892af05da4c75262f7ec80fdd87914a6b6ca8c5[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat May 21 21:52:30 2016 -0700

    * Minor update:  !xxx Will now tab-complete through the history of commands (if the first char is a '!')

[33mcommit aa45b94805b19bf371393928d8b2aeb4e72778a5[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri May 20 22:54:33 2016 -0700

    *  Updating demotools.ts, to use SocketCommand() where possible.

[33mcommit 54c938c93a1343ee3f5407ae15f0dbc21aec67b3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Thu May 19 21:01:43 2016 -0700

    *  Minor updates to TabComplete.

[33mcommit 1d30bc25c5efa21838e29ec28354a715458d9038[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 17 20:20:06 2016 -0700

    *  Added "DrawRect" to the Qt Demo

[33mcommit 60d5394cd060864f675cbefb780e5646383360f3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat May 14 22:51:34 2016 -0700

    *  Post-Inc  Unary Operator appears to work in all circumstances... unit tests added.

[33mcommit 54980a8bd3ff81a1b4bd6b9c8b085784b60fbd0c[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat May 14 21:15:48 2016 -0700

    *  Post-inc POD Members not yet working...

[33mcommit c8525ff38bce7a86f450a6166c91be8515ff0d91[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri May 13 23:18:29 2016 -0700

    *  2nd pass of post unary op implementation

[33mcommit f92c5bcd99205b2a07f7a8786d18b934a0f29748[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 8 23:38:02 2016 -0700

    *  PostOp nodes are now hooked up through BinaryTreeNodes, to the appropriate parse tree nodes instead of always at the root

[33mcommit 0f511d9e03a10b7d319f856b11027293d94298c0[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 8 11:16:05 2016 -0700

    *  WIP post-increment...  need to organize the post-op nodes after the specific statement has executed... conditional for an 'if', for example.

[33mcommit 0530f114ff35c219a2f69f491d7bd288806e180c[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 8 01:21:54 2016 -0700

    *  Post-increment/decrement is now supported.

[33mcommit d38ee8026e2bf1b71090bedd2f22fd781f1fee7c[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat May 7 01:02:29 2016 -0700

    *  Consecutive assignments (int x = y = ++z) , and assignments as array indices (array[++index]) are now supported.

[33mcommit b9fcdeae8b5b3c622854458d44e94f3694cd7318[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Wed May 4 20:04:07 2016 -0700

    *  Added SocketCommand() registered function.

[33mcommit 1e3038a398deed868111c6573cb2e6709d932dbe[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue May 3 22:35:01 2016 -0700

    *  schedule() now allows an expression for the "delay time"

[33mcommit b636b2e0f612fa0857961366d489fa89d4a59917[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon May 2 23:13:43 2016 -0700

    *  Reserved keywords added to tab completion
    *  CmdShellPrintf() added as a default print handler

[33mcommit b47dfff6825c1ec26b32ddb130ecdd040baf711d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun May 1 18:08:18 2016 -0700

    *  ListFunctions() now sorts the list alphabetically

[33mcommit df75b3c31d7f5da8d091d02f35ac9c7c52c39cef[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 30 17:30:27 2016 -0700

    *  CmdShell needs to include windows.h  (may need to #ifdef around some functionality....

[33mcommit dbdfe3e82f495e834a8975073f48c0629e2bfc9a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 30 17:18:10 2016 -0700

    *  Reworked the cmd shell input to allow a cursor position not at the end of the input, as well as home/end/left arrow/right arrow/del

[33mcommit 141750a48ea2cc3b14fefede96d0f3dca65d179d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Fri Apr 29 23:24:40 2016 -0700

    *  TabCompletion now supported by the TinQtConsole

[33mcommit d2a3567433005c4f4b2c5d8e460527d274ce5c9f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Wed Apr 27 00:23:12 2016 -0700

    *  Tab completion will now parse the chain of identifiers, and resolve each, to tab complete the member of the last object in the chain.
    *  It will also tab complete the right-most complete-able  partial string, without "backing up" beyond an argument, into a function call

[33mcommit e53b5a02877f1e43eab99762751ad26fe23e055f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Apr 25 22:31:46 2016 -0700

    *  Tab completion now looks for global object vars, and populates members and methods

[33mcommit 0ac8f3b4c3cc3712b957ec1c79a5273bdc6e7c00[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Apr 25 00:14:01 2016 -0700

    *  Tab completion for global functions has been added.

[33mcommit 941e92b717b2eeb4a2ad83799cc4d6ddc394d62e[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 24 22:21:31 2016 -0700

    *  Minor assert fix.

[33mcommit 615edad1469596211bcab3475ae794e0a0c89bc3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 24 22:18:57 2016 -0700

    *  CCmdShell:  Fixed the standard console output, to deal with word wrap...

[33mcommit f2f96a2553e6a8d7918ed325f8899ccdfd7f7294[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 24 16:55:21 2016 -0700

    Working on the TinConsole shell - irritating new lines, conflicts between Print()s and the current input string....

[33mcommit 310072bdcacd4a7d82d6462e13c6d71aa15677b2[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 23 16:43:46 2016 -0700

    Consolidated the OP_Branch* conditionals into a single OP_BranchCond instruction.

[33mcommit 10c39d796a5044d0a0d1920fbf20bc30fa650c9d[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 23 16:11:48 2016 -0700

    *  Boolean And/Or short-circuit logic has been added.

[33mcommit 94467215c853b637247a2d1f3e32274aa5717c04[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue Apr 19 20:22:31 2016 -0700

    *  TinHash, added Prev(), Current() methods
    *  CObjectSet:  added Prev(), IsFirst(), IsLast(), Current()

[33mcommit 2f028e5332c09e4f14a6cb3e99dcf8a8a1b02022[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Tue Apr 19 15:54:12 2016 -0700

    *  TinHash:  iterators are no longer reset, when an object is hashed, as this doesn't actually conflict with the current iterator state.

[33mcommit d5c0b8b898e42292222a37b1d32ab4f21d289835[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 17 23:01:54 2016 -0700

    ** CGroupIterator class implemented, using the new CHashTable iterators.

[33mcommit eac1b87b6735b1ad820a8d6a1d2704f0a8b2fc6d[m
Merge: bc3f1b4 3d1d966
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 17 16:45:59 2016 -0700

    Merge branch 'master' of https://github.com/KidneyThief/TinScript_2016

[33mcommit bc3f1b4ab647e0f9cb0c23c1f39e9d4babb14d31[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 17 16:45:24 2016 -0700

    *  CHashTableIterator, first pass, replacing the built in iterator with the ability to track a list of iterators

[33mcommit 3d1d966e9bf167ecfd6d4fe5aa5a6b6086394366[m
Merge: e06741f bfe61a0
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 16 23:37:21 2016 -0700

    Merge branch 'master' of https://github.com/KidneyThief/TinScript_2016

[33mcommit e06741f928c266a308d5d6e7b2dd0d94da0a9fc1[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 16 23:36:58 2016 -0700

    wiki Images

[33mcommit bfe61a0bead1198660189e0ed1315f1ec010c99e[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 16 23:14:49 2016 -0700

    *  Changed the keyword to array_count(), and used a better parsing implementation to support object member arrays

[33mcommit 1206e3a4a932f1bb4a7940ce8dd6fb09be3886c3[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Apr 16 16:27:51 2016 -0700

    *  Added the keyword count(<array>)

[33mcommit 56f2b48a2bd10ed839fb348d5ff4d00a778c4063[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 3 23:14:01 2016 -0700

    *  createlocal cannot be called outside a function definition

[33mcommit efbf7bfbe7d8c159d8114ebcd765e7b45ad18b19[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Apr 3 20:56:24 2016 -0700

    *   createlocal keyword added

[33mcommit 9c33e481ebc912607087a2661ea4fea8ca1d5464[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 27 15:12:59 2016 -0700

    *  Resource reference is now relative

[33mcommit 2473eb35a3ed8c68e51857db26d8d379ec70925b[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 27 13:05:43 2016 -0700

    *  Added icons to TinQtDemo and TinQtConsole

[33mcommit 381853d93158c3a452536387ca736d4474502184[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Mar 26 17:30:00 2016 -0700

    *  Crash on shutdown, destructing CDebuggerWatchExpression instances
    *  TinScript::GetContext() can now be used in any destructor to see if the entire system is shutting down.

[33mcommit a0bcd56b4edefbf05b235242e11de45ec8e5134c[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 20 14:38:23 2016 -0700

    Added the "Method" button for the Function Assist Win, to look up the actual implementation for an object method.

[33mcommit 74117ebfb006d8a69d3831d9c347dae41cffde9e[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Mar 19 18:12:28 2016 -0700

    *  Compile errors, in projects with stricter rules have been improved...  e.g. explicitly using int32 instead of just int

[33mcommit 94ff06d3b7bc0a6779a491873f418c3cf7761ffe[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sat Mar 19 13:26:37 2016 -0700

    Final solution for registering an Enum!

[33mcommit cbfc871f09f7a5074cede4ab6443896e0bbded13[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 13 22:34:00 2016 -0700

    Still unable to find a simpler syntax...

[33mcommit d0492517f177963642df9af43745530df7f78a39[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 13 22:00:33 2016 -0700

    Final(?) solution for registered enums...

[33mcommit fbf9e1adc147fcbef35ddf32dc389ce1e96dd8ea[m
Merge: 2a954e1 20c6432
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 13 18:04:22 2016 -0700

    Merge branch 'master' of https://github.com/KidneyThief/TinScript_2016
    
    Conflicts:
            TinQtDemo/TinQtDemo_r.exe

[33mcommit 2a954e16f272610156870a33efb33ed530f043b2[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Mar 13 18:03:29 2016 -0700

    *  Quick test for a registered enum

[33mcommit 20c643292e51c8919f112b9c7c7868f50e6db0e7[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Wed Feb 24 00:18:45 2016 -0800

    * Mission framework updated to work with the Qt demo...

[33mcommit d71137811af1541137dfbb72d4a831315a92bcee[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Jan 31 14:14:54 2016 -0800

    *  Debugging information enabled
    *  Updated the usage of font sizes, so some labels and titles are 'less clipped'

[33mcommit 15d94dec6f677d8dcdd1475d8e13c25e5242960f[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Dec 28 22:42:50 2015 -0800

    *  All projects updated with release configurations

[33mcommit 358a6a30fd62d2972f058e33fadd203a8f04dd36[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Dec 28 22:16:57 2015 -0800

    *  Updating the TinConsole project

[33mcommit 7365b48848f10aac8d89d3fd68650b82071dd881[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Dec 28 21:06:04 2015 -0800

    *  Benign sprintf_s() error fixed.

[33mcommit f2dade9fda39e99097782150560d89bdf3c7163e[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Dec 28 21:02:17 2015 -0800

    *  TinScript unit tests added

[33mcommit 9cc27fc8aa7bdcb428b43f6bc765c0767e74dc4a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Mon Dec 28 20:48:54 2015 -0800

    *  Updating the Ignore

[33mcommit d0f4edbfc293eaceb886a90887dcb1e57c69fc0a[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Dec 27 20:29:10 2015 -0800

    *  Ignoring build targets

[33mcommit 416bba2cc16814f18ef7f37f162e96dae6e750f7[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Dec 27 20:27:11 2015 -0800

    *  Output executable is now in the solution directory

[33mcommit 8e372bb3cc6325e8f0c6b37e65dfdef45a7527de[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Dec 27 15:58:29 2015 -0800

    *  First commit of the TinQtConsole

[33mcommit 8b27cf62e30dc5a5e9720f0125cd508967235b01[m
Author: Tim Andersen <kidneythief@shaw.ca>
Date:   Sun Dec 20 21:20:09 2015 -0800

    Original depot
