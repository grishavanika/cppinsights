/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "FunctionDeclHandler.h"
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

FunctionDeclHandler::FunctionDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(functionDecl(unless(anyOf(cxxMethodDecl(),
                                                 isExpansionInSystemHeader(),
                                                 isTemplate,
                                                 hasParent(linkageSpecDecl()),
                                                 hasAncestor(friendDecl()),    // friendDecl has functionDecl as child
                                                 hasAncestor(functionDecl()),  // prevent forward declarations
                                                 isMacroOrInvalidLocation())))
                           .bind("funcDecl"),
                       this);

    static const auto hasTemplateDescendant = anyOf(hasDescendant(classTemplateDecl()),
                                                    hasDescendant(functionTemplateDecl()),
                                                    hasDescendant(classTemplateSpecializationDecl()));

    matcher.addMatcher(friendDecl(unless(anyOf(cxxMethodDecl(),
                                               hasAncestor(cxxRecordDecl()),
                                               isExpansionInSystemHeader(),
                                               isTemplate,
                                               hasTemplateDescendant,
                                               hasAncestor(functionDecl()),  // prevent forward declarations
                                               isMacroOrInvalidLocation())))
                           .bind("friendDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void FunctionDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
        const auto         columnNr = GetSM(result).getSpellingColumnNumber(GetBeginLoc(funcDecl)) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(funcDecl);

        // Find the correct ending of the source range. In case of a declaration we need to find the ending semi,
        // otherwise the provided source range is correct.
        const auto sr = [&]() {
            if(!funcDecl->doesThisDeclarationHaveABody()) {
                return GetSourceRangeAfterSemi(funcDecl->getSourceRange(), result);
            }

            return funcDecl->getSourceRange();
        }();

        // DPrint("fd rw:  %d %s\n", (sr.getBegin() == sr.getEnd()), outputFormatHelper.GetString());

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
