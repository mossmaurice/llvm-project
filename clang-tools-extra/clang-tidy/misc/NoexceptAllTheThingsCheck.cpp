//===--- NoexceptAllTheThingsCheck.cpp - clang-tidy -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NoexceptAllTheThingsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void NoexceptAllTheThingsCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus) {

    return;
  }
  // Walk over the AST and find function declarations
  Finder->addMatcher(
      functionDecl(
          isExpansionInMainFile(),
          unless(hasTypeLoc(loc(functionProtoType(hasDynamicExceptionSpec())))),
          unless(hasTypeLoc(loc(functionProtoType(isNoThrow())))))
          .bind("FuncDeclNoexceptMissing"),
      this);

  /// @todo Walk over the AST and find function declaration?
  // Finder->addMatcher(parmVarDecl().bind("NoexceptMissing"), this);
}

void NoexceptAllTheThingsCheck::check(const MatchFinder::MatchResult &result) {
  // Add noexcept to all matches
  const auto *MatchedDecl =
      result.Nodes.getNodeAs<FunctionDecl>("FuncDeclNoexceptMissing");
  SourceRange range;
  SourceLocation location;

  auto functionType = MatchedDecl->getType()->getAs<FunctionProtoType>();
  assert(functionType && "FunctionProtoType is null.");

  if (const auto *typeSourceInfo = MatchedDecl->getTypeSourceInfo()) {

    auto closingParenthesis =
        typeSourceInfo->getTypeLoc().castAs<FunctionTypeLoc>().getRParenLoc();
    location =
        Lexer::getLocForEndOfToken(closingParenthesis, 0, *result.SourceManager,
                                   result.Context->getLangOpts());

    range = location;
  }

  if (functionType->getExceptionSpecType() !=
      ExceptionSpecificationType::EST_None) {
    return;
  }
  assert(range.isValid() && "Exception source range is invalid.");

  CharSourceRange charSourceRange = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(range), *result.SourceManager,
      result.Context->getLangOpts());

  std::string insertionString = "noexcept";

  FixItHint FixIt;

  FixIt = FixItHint::CreateInsertion(location, " " + insertionString);

  diag(range.getBegin(), "add 'noexcept'")
      << Lexer::getSourceText(charSourceRange, *result.SourceManager,
                              result.Context->getLangOpts())
      << insertionString << FixIt;
}

} // namespace misc
} // namespace tidy
} // namespace clang
