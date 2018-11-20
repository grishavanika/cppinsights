/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CodeGenerator.h"
#include "DPrint.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

void CodeGenerator::InsertArg(const CoroutineBodyStmt* stmt)
{
    mOutputFormatHelper.AppendNewLine("// CoroutineBodyStmt");

    mOutputFormatHelper.OpenScope();

    InsertArg(stmt->getPromiseDecl());

    DPrint("ptr: %d\n", stmt->getPromiseDecl()->getType()->isAnyPointerType());

    mOutputFormatHelper.AppendNewLine("// new __promise;");
    mOutputFormatHelper.Append("__promise = ");
    InsertArg(stmt->getAllocate());
    mOutputFormatHelper.AppendNewLine(';');

    if(stmt->getReturnStmtOnAllocFailure()) {
        mOutputFormatHelper.AppendNewLine();
        mOutputFormatHelper.AppendNewLine("if(not __promise)");
        mOutputFormatHelper.OpenScope();

        InsertArg(stmt->getReturnStmtOnAllocFailure());

        mOutputFormatHelper.CloseScope();
        mOutputFormatHelper.AppendNewLine();
        mOutputFormatHelper.AppendNewLine();
    }

#if 0
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.Append("auto&& gro = ");
    InsertArg(stmt->getReturnValueInit());
    mOutputFormatHelper.AppendNewLine(';');
#endif

    // P0057R8: [dcl.fct.def.coroutine] p5: before initial_supsends and at tops 1
    InsertArg(stmt->getResultDecl());
    mOutputFormatHelper.AppendNewLine();

    InsertArg(stmt->getInitSuspendStmt());
    mOutputFormatHelper.AppendNewLine(';');
    mOutputFormatHelper.AppendNewLine();

    const auto* exceptionHandler = stmt->getExceptionHandler();
    const bool  createTryCatchBlock{nullptr != exceptionHandler};

    if(createTryCatchBlock) {
        mOutputFormatHelper.AppendNewLine("try");
        mOutputFormatHelper.OpenScope();
    }

    InsertArg(stmt->getBody());
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine();

    InsertArg(stmt->getReturnStmt());
    mOutputFormatHelper.AppendNewLine(';');

    mOutputFormatHelper.AppendNewLine("goto final_suspend;");

    if(createTryCatchBlock) {
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::No);
        mOutputFormatHelper.AppendNewLine();
        mOutputFormatHelper.Append("catch(...) { ");
        InsertArg(exceptionHandler);
        mOutputFormatHelper.AppendNewLine("; }");
    }

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.AppendNewLine("final_suspend:");
    InsertArg(stmt->getFinalSuspendStmt());
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine();

    InsertArg(stmt->getFallthroughHandler());
    mOutputFormatHelper.AppendNewLine("// delete __promise;");

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);

    // P0057R8: [dcl.fct.def.coroutine] p8: get_return_object_on_allocation_failure indicates that new may return a
    // nullptr. In this case return get_return_object_on_allocation_failure.

    /*
    if(stmt->getAllocate()) {
        InsertArg(stmt->getAllocate());
    }*/

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine("// getParmMoves");

    for(auto* param : stmt->getParamMoves()) {
        InsertArg(param);
    }

#if 0
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine("----------------");

    stmt->getAllocate()->dump();

    InsertArg(stmt->getAllocate());
    mOutputFormatHelper.AppendNewLine();
     InsertArg(stmt->getDeallocate());
    mOutputFormatHelper.AppendNewLine();

    InsertArg(stmt->getReturnStmtOnAllocFailure());
    mOutputFormatHelper.AppendNewLine();

    if(stmt->hasDependentPromiseType()) {
        InsertArg(stmt->getPromiseDeclStmt());
    }

    mOutputFormatHelper.AppendNewLine();
#endif
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CoroutineSuspendExpr* stmt)
{
    mOutputFormatHelper.AppendNewLine("// CoroutineSuspendExpr");

    /// Represents an expression that might suspend coroutine execution;
    /// either a co_await or co_yield expression.
    ///
    /// Evaluation of this expression first evaluates its 'ready' expression. If
    /// that returns 'false':
    ///  -- execution of the coroutine is suspended
    ///  -- the 'suspend' expression is evaluated
    ///     -- if the 'suspend' expression returns 'false', the coroutine is
    ///        resumed
    ///     -- otherwise, control passes back to the resumer.
    /// If the coroutine is not suspended, or when it is resumed, the 'resume'
    /// expression is evaluated, and its result is the result of the overall
    /// expression.

    // InsertArg(stmt->getOperand());

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.Append("if(");

    InsertArg(stmt->getReadyExpr());
    mOutputFormatHelper.AppendNewLine(")");
    mOutputFormatHelper.OpenScope();

    InsertArg(stmt->getResumeExpr());
    mOutputFormatHelper.Append(';');

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::No);
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine("else");
    mOutputFormatHelper.OpenScope();

    InsertArg(stmt->getSuspendExpr());
    mOutputFormatHelper.Append(';');

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::No);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CoreturnStmt* stmt)
{
    mOutputFormatHelper.AppendNewLine("// CoreturnStmt");

    if(stmt->getOperand()) {
        InsertArg(stmt->getOperand());
        mOutputFormatHelper.AppendNewLine();
    }

    if(stmt->getPromiseCall()) {
        mOutputFormatHelper.Append("return ");
        InsertArg(stmt->getPromiseCall());
        mOutputFormatHelper.AppendNewLine(';');
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
