#include <iostream>
#include <fcntl.h>

#include <llvm-c/Core.h>
#include <llvm-c/Support.h>
#include <llvm-c/TargetMachine.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/InitializePasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

using namespace llvm;
using namespace llvm::orc;


/* 
 * Функции и классы для вызова изнутри JIT
 */
namespace ns_stub {

    int func_stub(int arg1, short arg2) {
        return arg1*arg2;
    };

    int func_extern_stub() {
        return 4242;
    };

    class class_stub {
    public:
        int field_1;
        static int static_field_2;

        static class_stub * create(int a1, int a2) {
            return new class_stub(a1, a2);
        }

        class_stub() {
            printf("Call constructor class_stub()\n");
            field_1 = 0;
        }

        class_stub(int arg1, int arg2) {
            printf("Call constructor class_stub(%d, %d)\n", arg1, arg2);
            field_1 = arg1;
            static_field_2 = arg2;
        }

        virtual ~class_stub() {
            printf("Call virtual ~class_stub()\n");
        }

        int method_sum() {

            return field_1 + static_field_2;
        }

        int method_field1(int arg) {

            return field_1;
        }

        virtual double method_virt2() {

            return 999999999;
        }

        virtual float method_virt() {

            return 3.14 + field_1;
        }

        static float method_static() {
            return 3.1415;
        }
    };

    int class_stub::static_field_2 = 0;

    class class_full {
    public:

        class_full() {
        }

        int method() {
            return 42;
        }
    };
};

/*
 * Строка прототип для компиляции в JIT
 */
const char * func_text = ""
        "extern \"C\" int printf(const char *, ...);\n"
        "extern \"C\" int nv_add(int a, int b) {"
        "   printf(\"call nv_add(%d, %d)\\n\", a, b);"
        "   return a + b;"
        "};\n"
        ""
        "extern \"C\" int nv_sub(int a, int b) {"
        "   printf(\"call nv_sub(%d, %d)\\n\", a, b);"
        "   return a - b;"
        "};\n"
        "extern \"C\" int run(){"
        "   nv_add(100, 123);"
        "   nv_sub(100, 123);"
        "   return 42;"
        "};\n"
        ""
        "namespace ns_stub {"
        "   class run_internal {"
        "       public:\n"
        "       run_internal(){};"
        "       int method(){"
        "           return 43;"
        "       };"
        "   };"
        "   class class_full {"
        "       public:\n"
        "       class_full();"
        "       int method();"
        "   };"
        ""
        "   class class_stub {"
        "       public:\n"
        "       static class_stub * create(int, int);"
        "       class_stub();"
        "       class_stub(int arg1, int arg2);"
        "       int method_sum();"
        "       int method_field1(int);"
        "       virtual float method_virt();"
        "   };"
        ""
        "};"
        "extern \"C\" int run_internal(){"
        "   ns_stub::run_internal cl_int;"
        "   printf(\"run_internal.method %d\\n\", cl_int.method());"
        "   return 44;"
        "};\n"
        ""
        "extern \"C\" int run_stub(){"
        "   ns_stub::class_stub *cl = ns_stub::class_stub::create(123, 123);"
        "   printf(\"class_stub.method_sum %d\\n\", cl->method_sum());"
        "   delete cl;"
        "   return 42;"
        "};\n"
        ""
        "extern \"C\" int run_extern();"
        "extern \"C\" int run_extern_stub(){"
        "   return run_extern();"
        "};\n"
        "extern \"C\" int run_virt(){"
        "   ns_stub::class_stub *cl = ns_stub::class_stub::create(124, 125);"
        "   printf(\"class_stub.method_virt %f\\n\", cl->method_virt());"
        "   delete cl;"
        "   return 0;"
        "};\n"
        "";



#define DEBUG_MSG(msg) std::cout << "[DEBUG]: "<<msg<< std::endl;

void InitializeLLVM() {

    // We have not initialized any pass managers for any device yet.
    // Run the global LLVM pass initialization functions.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto& Registry = *llvm::PassRegistry::getPassRegistry();

    llvm::initializeCore(Registry);
    llvm::initializeScalarOpts(Registry);
    llvm::initializeVectorization(Registry);
    llvm::initializeIPO(Registry);
    llvm::initializeAnalysis(Registry);
    llvm::initializeTransformUtils(Registry);
    llvm::initializeInstCombine(Registry);
    llvm::initializeTarget(Registry);
}

std::unique_ptr<llvm::Module> CompileCpp(std::string source) {
    clang::CompilerInstance compilerInstance;
    auto& compilerInvocation = compilerInstance.getInvocation();


    // Диагностика работы Clang
    clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts = new clang::DiagnosticOptions;
    clang::TextDiagnosticPrinter *textDiagPrinter =
            new clang::TextDiagnosticPrinter(llvm::outs(), &*DiagOpts);

    clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;

    clang::DiagnosticsEngine *pDiagnosticsEngine =
            new clang::DiagnosticsEngine(pDiagIDs, &*DiagOpts, textDiagPrinter);


    // Целевая платформа
    std::string triple = LLVMGetDefaultTargetTriple();

    std::vector<std::string> itemstrs;
    itemstrs.push_back(triple.insert(0, "-triple="));
    itemstrs.push_back("-xc++");
    itemstrs.push_back("-std=c++20");
    //    itemstrs.push_back("-fno-exceptions");
    //    itemstrs.push_back("-funwind-tables");

    std::vector<const char*> itemcstrs;
    for (unsigned idx = 0; idx < itemstrs.size(); idx++) {
        // note: if itemstrs is modified after this, itemcstrs will be full
        // of invalid pointers! Could make copies, but would have to clean up then...
        itemcstrs.push_back(itemstrs[idx].c_str());
        std::cout << itemcstrs.back() << "\n";
    }

    // Компиляция из памяти
    // Send code through a pipe to stdin
    int codeInPipe[2];
    pipe2(codeInPipe, O_NONBLOCK);
    write(codeInPipe[1], source.c_str(), source.size());
    close(codeInPipe[1]); // We need to close the pipe to send an EOF
    dup2(codeInPipe[0], STDIN_FILENO);

    itemcstrs.push_back("-"); // Read code from stdin

    clang::CompilerInvocation::CreateFromArgs(compilerInvocation,
            llvm::ArrayRef<const char *>(itemcstrs.data(),
            itemcstrs.size()), *pDiagnosticsEngine);

    auto& languageOptions = compilerInvocation.getLangOpts();
    auto& preprocessorOptions = compilerInvocation.getPreprocessorOpts();
    auto& targetOptions = compilerInvocation.getTargetOpts();

    auto& frontEndOptions = compilerInvocation.getFrontendOpts();
    //    frontEndOptions.ShowStats = true;

    auto& headerSearchOptions = compilerInvocation.getHeaderSearchOpts();
    //    headerSearchOptions.Verbose = true;

    auto& codeGenOptions = compilerInvocation.getCodeGenOpts();


    targetOptions.Triple = LLVMGetDefaultTargetTriple();
    compilerInstance.createDiagnostics(textDiagPrinter, false);

    DEBUG_MSG("Using target triple: " << triple);

    LLVMContextRef ctx = LLVMContextCreate();
    std::unique_ptr<clang::CodeGenAction> action = std::make_unique<clang::EmitLLVMOnlyAction>((llvm::LLVMContext *)ctx);

    assert(compilerInstance.ExecuteAction(*action));

    // Runtime LLVM Module
    std::unique_ptr<llvm::Module> module = action->takeModule();

    assert(module);

    // Оптимизация IR
    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cGSCCAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;

    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cGSCCAnalysisManager, moduleAnalysisManager);

    llvm::ModulePassManager modulePassManager = passBuilder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    modulePassManager.run(*module, moduleAnalysisManager);

    return module;
}



ExitOnError ExitOnErr;

ThreadSafeModule createDemoModule() {
    auto Context = std::make_unique<LLVMContext>();
    auto M = std::make_unique<Module>("test", *Context);

    // Create the add1 function entry and insert this entry into module M.  The
    // function will have a return type of "int" and take an argument of "int".
    Function *Add1F = Function::Create(FunctionType::get(Type::getInt32Ty(*Context),{Type::getInt32Ty(*Context)}, false),
    Function::ExternalLinkage, "add1", M.get());

    // Add a basic block to the function. As before, it automatically inserts
    // because of the last argument.
    BasicBlock *BB = BasicBlock::Create(*Context, "EntryBlock", Add1F);

    // Create a basic block builder with default parameters.  The builder will
    // automatically append instructions to the basic block `BB'.
    IRBuilder<> builder(BB);

    // Get pointers to the constant `1'.
    Value *One = builder.getInt32(1);

    // Get pointers to the integer argument of the add1 function...
    assert(Add1F->arg_begin() != Add1F->arg_end()); // Make sure there's an arg
    Argument *ArgX = &*Add1F->arg_begin(); // Get the arg
    ArgX->setName("AnArg"); // Give it a nice symbolic name for fun.

    // Create the add instruction, inserting it into the end of BB.
    Value *Add = builder.CreateAdd(One, ArgX);

    // Create the return instruction and add it to the basic block
    builder.CreateRet(Add);

    return ThreadSafeModule(std::move(M), std::move(Context));
}

int main(int argc, char *argv[]) {
    // Initialize LLVM.
    InitLLVM X(argc, argv);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    cl::ParseCommandLineOptions(argc, argv, "HowToUseLLJIT");
    ExitOnErr.setBanner(std::string(argv[0]) + ": ");

    // Create an LLJIT instance.
    auto J = ExitOnErr(LLJITBuilder().create());
    //    auto M = createDemoModule();
    auto M = ThreadSafeModule(std::move(CompileCpp(func_text)), std::make_unique<LLVMContext>());


    std::string dump;
    llvm::raw_string_ostream err(dump);


    ExecutionSession &ES = J->getExecutionSession();

    //    JITDylib *plat = ES.getJITDylibByName("<Platform>");
    //    assert(plat);
    //    dump.clear();
    //    plat->dump(err);
    //    std::cout << "<Platform>:\n" << dump << "\n";
    //
    //    JITDylib *proc = ES.getJITDylibByName("<Process Symbols>");
    //    assert(proc);
    //    dump.clear();
    //    proc->dump(err);
    //    std::cout << "<Process Symbols>:\n" << dump << "\n";


    ExitOnErr(J->addIRModule(std::move(M)));



    // Функция с именем run_extern отсуствует (JIT session error: Symbols not found: [ run_extern ])
    // Подставим вместо нее указатель на другу функцию, но с таким же прототипом (func_extern_stub)
    const SymbolStringPtr Foo = ES.intern("run_extern");
    const ExecutorSymbolDef FooSym(ExecutorAddr::fromPtr(&ns_stub::func_extern_stub), llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Absolute);
    auto as = absoluteSymbols({
        {Foo, FooSym}
    });

    if (auto Err = J->getMainJITDylib().define(as)) {
        std::cout << "JD.define error: !\n";
        return 0;
    }


    Expected<ExecutorAddr> test = J->lookup("nv_add");
    if (!test) {
        std::cout << "lookup error:\n" << toString(test.takeError());
        return 0;
    }

    DEBUG_MSG("Retrieving nv_add/nv_sub functions...");

    auto addAddr = ExitOnErr(J->lookup("nv_add"));
    int (*add)(int, int) = addAddr.toPtr<int(int, int) >();
    assert(add);

    int res = add(40, 2);
    assert(42 == res);

    auto subAddr = ExitOnErr(J->lookup("nv_sub"));
    int (*sub)(int, int) = subAddr.toPtr<int(int, int) >();
    assert(sub);

    res = sub(50, 7);
    assert(43 == res);



    printf("Call: run_internal\n");
    auto run_internalAddr = ExitOnErr(J->lookup("run_internal"));
    int (*run_internal)() = run_internalAddr.toPtr<int() >();
    assert(run_internal);

    res = run_internal();
    assert(44 == res);


    // Линкер удаяет не используемый код, 
    // и если нет обращения к методу то его будет нельзя вызвать в JIT
    // JIT session error: Symbols not found: [ _ZN7ns_stub10class_stub6createEii, _ZN7ns_stub10class_stub10method_sumEv ]
    ns_stub::class_stub *cl = ns_stub::class_stub::create(0, 0);
    printf("Check run_stub.method %d\n", cl->method_sum());
    printf("Check run_stub.method_virt %f\n", cl->method_virt());
    delete cl;


    printf("Call: run_stub\n");
    auto run_stubAddr = ExitOnErr(J->lookup("run_stub"));
    int (*run_stub)() = run_stubAddr.toPtr<int() >();
    assert(run_stub);

    res = run_stub();
    assert(42 == res);


    printf("Call: run_extern_stub\n");
    auto run_extern_stubAddr = ExitOnErr(J->lookup("run_extern_stub"));
    int (*run_extern_stub)() = run_extern_stubAddr.toPtr<int() >();
    assert(run_extern_stub);

    res = run_extern_stub();
    assert(4242 == res);


    /*
     * 
     * Так нельзя !!!!! 
     * Виртуальные методы изнутри JIT вызываются неправильно при некорректном заголовочном файле!
     * 
     * ERROR !!!!
     * Virtual methods from within JIT are called incorrectly when the header file is incorrect!
     *      
     */

    printf("Call: run_virt\n");
    auto run_virtAddr = ExitOnErr(J->lookup("run_virt"));
    int (*run_virt)() = run_virtAddr.toPtr<int() >();
    assert(run_virt);

    res = run_virt();
    assert(0 == res);

    return 0;
}

