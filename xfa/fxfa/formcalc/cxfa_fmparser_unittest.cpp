// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fxfa/formcalc/cxfa_fmparser.h"

#include "core/fxcrt/widetext_buffer.h"
#include "testing/fxgc_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "xfa/fxfa/formcalc/cxfa_fmtojavascriptdepth.h"

class CXFAFMParserTest : public FXGCUnitTest {};

TEST_F(CXFAFMParserTest, Empty) {
  CXFA_FMLexer lexer(L"");
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  // TODO(dsinclair): This is a little weird .....
  EXPECT_EQ(L"// comments only", buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, CommentOnlyIsError) {
  CXFA_FMLexer lexer(L"; Just comment");
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  // TODO(dsinclair): This isn't allowed per the spec.
  EXPECT_FALSE(parser.HasError());
  // EXPECT_TRUE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(L"// comments only", buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, CommentThenValue) {
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 12;
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(L"; Just comment\n12");
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, Parse) {
  constexpr wchar_t kInput[] =
      LR"***($ = Avg (-3, 5, -6, 12, -13);
$ = Avg (Table2..Row[*].Cell1);
if ($ ne -1)then
  border.fill.color.value = "255,64,64";
elseif ($ ne -2) then
  border.fill.color.value = "128,128,128";
else
  border.fill.color.value = "20,170,13";
endif
$)***";

  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
if (pfm_rt.is_obj(this))
{
pfm_rt.asgn_val_op(this, pfm_rt.Avg(pfm_rt.neg_op(3), 5, pfm_rt.neg_op(6), 12, pfm_rt.neg_op(13)));
}
if (pfm_rt.is_obj(this))
{
pfm_rt.asgn_val_op(this, pfm_rt.Avg(pfm_rt.dot_acc(pfm_rt.dotdot_acc(Table2, "Table2", "Row", 1), "", "Cell1", 0, 0)));
}
if (pfm_rt.get_val(pfm_rt.neq_op(this, pfm_rt.neg_op(1))))
{
if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0)))
{
pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0), "255,64,64");
}
}
else if (pfm_rt.get_val(pfm_rt.neq_op(this, pfm_rt.neg_op(2))))
{
if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0)))
{
pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0), "128,128,128");
}
}
else {
if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0)))
{
pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "border", "fill", 0, 0), "", "color", 0, 0), "", "value", 0, 0), "20,170,13");
}
}
pfm_ret = this;
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().AsStringView());
}

TEST_F(CXFAFMParserTest, MaxParseDepth) {
  CXFA_FMLexer lexer(L"foo(bar[baz(fizz[0])])");
  CXFA_FMParser parser(heap(), &lexer);
  parser.SetMaxParseDepthForTest(5);
  EXPECT_FALSE(parser.Parse());
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, chromium752201) {
  CXFA_FMLexer lexer(
      LR"***(fTep a
.#
fo@ =[=l)***");

  CXFA_FMParser parser(heap(), &lexer);
  EXPECT_FALSE(parser.Parse());
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, MultipleAssignmentIsNotAllowed) {
  CXFA_FMLexer lexer(L"(a=(b=t))=u");
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(!ast);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseFuncWithParams) {
  constexpr wchar_t kInput[] =
      LR"***(func MyFunction(param1, param2) do
  param1 * param2
endfunc)***";

  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
function MyFunction(param1, param2) {
var pfm_ret = null;
pfm_ret = pfm_rt.mul_op(param1, param2);
return pfm_ret;
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFuncWithoutParams) {
  constexpr wchar_t kInput[] =
      LR"***(func MyFunction() do
  42
endfunc)***";

  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
function MyFunction() {
var pfm_ret = null;
pfm_ret = 42;
return pfm_ret;
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFuncWithBadParamsList) {
  constexpr wchar_t kInput[] =
      LR"***(func MyFunction(param1,) do
  param1 * param2
endfunc)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseBadIfExpression) {
  constexpr wchar_t kInput[] = L"if ( then";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseBadElseIfExpression) {
  constexpr wchar_t kInput[] =
      LR"***(if ($ ne -1) then"
elseif( then)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseDepthWithWideTree) {
  constexpr wchar_t kInput[] =
      L"a <> b <> c <> d <> e <> f <> g <> h <> i <> j";

  {
    CXFA_FMLexer lexer(kInput);
    CXFA_FMParser parser(heap(), &lexer);
    CXFA_FMAST* ast = parser.Parse();
    ASSERT_TRUE(ast);
    EXPECT_TRUE(!parser.HasError());
  }

  {
    CXFA_FMLexer lexer(kInput);
    CXFA_FMParser parser(heap(), &lexer);
    parser.SetMaxParseDepthForTest(5);
    CXFA_FMAST* ast = parser.Parse();
    ASSERT_TRUE(ast == nullptr);
    EXPECT_TRUE(parser.HasError());
  }
}

TEST_F(CXFAFMParserTest, ParseCallSmall) {
  constexpr wchar_t kInput[] = L"i.f(O)";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = pfm_rt.get_val((function() {
  return pfm_method_runner(i, function(obj) {
    return obj.f(pfm_rt.get_val(O));
  });
}).call(this));
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseCallBig) {
  constexpr wchar_t kInput[] = L"i.f(O.e(O.e(O)))";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = pfm_rt.get_val((function() {
  return pfm_method_runner(i, function(obj) {
    return obj.f(pfm_rt.get_val((function() {
  return pfm_method_runner(O, function(obj) {
    return obj.e(pfm_rt.get_val((function() {
  return pfm_method_runner(O, function(obj) {
    return obj.e(pfm_rt.get_val(O));
  });
}).call(this)));
  });
}).call(this)));
  });
}).call(this));
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseVar) {
  constexpr wchar_t kInput[] = LR"(var s = "")";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
var s = "";
s = pfm_rt.var_filter(s);
pfm_ret = s;
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallNoArguments) {
  constexpr wchar_t kInput[] = L"P.x()";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = pfm_rt.get_val((function() {
  return pfm_method_runner(P, function(obj) {
    return obj.x();
  });
}).call(this));
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallSingleArgument) {
  constexpr wchar_t kInput[] = L"P.x(foo)";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = pfm_rt.get_val((function() {
  return pfm_method_runner(P, function(obj) {
    return obj.x(pfm_rt.get_jsobj(foo));
  });
}).call(this));
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallMultipleArguments) {
  constexpr wchar_t kInput[] = L"P.x(foo, bar, baz)";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = pfm_rt.get_val((function() {
  return pfm_method_runner(P, function(obj) {
    return obj.x(pfm_rt.get_jsobj(foo), pfm_rt.get_val(bar), pfm_rt.get_val(baz));
  });
}).call(this));
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";

  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallMissingCommas) {
  constexpr wchar_t kInput[] = L"P.x(!foo!bar!baz)";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallTrailingComma) {
  constexpr wchar_t kInput[] = L"P.x(foo,bar,baz,)";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseFunctionCallExtraComma) {
  constexpr wchar_t kInput[] = L"P.x(foo,bar,,baz)";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser.HasError());
}

TEST_F(CXFAFMParserTest, ParseWhileLoop) {
  constexpr wchar_t kInput[] = L"while (i < 10) do\n  i = i + 1\nendwhile";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 0;
while (pfm_rt.lt_op(i, 10))
{
if (pfm_rt.is_obj(i))
{
pfm_ret = pfm_rt.asgn_val_op(i, pfm_rt.plus_op(i, 1));
}
else
{
pfm_ret = i = pfm_rt.asgn_val_op(i, pfm_rt.plus_op(i, 1));
}
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseWhileWithBreakAndContinue) {
  constexpr wchar_t kInput[] =
      L"while (i < 10) do\n  if (i == 5) then\n    break\n  endif\n  "
      L"continue\nendwhile";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 0;
while (pfm_rt.lt_op(i, 10))
{
if (pfm_rt.get_val(pfm_rt.eq_op(i, 5)))
{
pfm_ret = 0;
break;
}
pfm_ret = 0;
continue;
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseForLoopUptoWithStep) {
  constexpr wchar_t kInput[] =
      L"for i = 1 upto 10 step 2 do\n  sum = sum + i\nendfor";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 0;
{
var i = null;
for (i = pfm_rt.get_val(1); i <= pfm_rt.get_val(10); i += pfm_rt.get_val(2))
{
if (pfm_rt.is_obj(sum))
{
pfm_ret = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, i));
}
else
{
pfm_ret = sum = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, i));
}
}
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseForLoopDownto) {
  constexpr wchar_t kInput[] =
      L"for i = 10 downto 1 do\n  sum = sum + i\nendfor";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 0;
{
var i = null;
for (i = pfm_rt.get_val(10); i >= pfm_rt.get_val(1); i -= 1)
{
if (pfm_rt.is_obj(sum))
{
pfm_ret = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, i));
}
else
{
pfm_ret = sum = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, i));
}
}
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseForeachLoop) {
  constexpr wchar_t kInput[] =
      L"foreach item in (1, 2, 3)\n  sum = sum + item\nendfor";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
pfm_ret = 0;
{
var item = null;
var pfm_ary = pfm_rt.concat_obj(1, 2, 3);
var pfm_ary_idx = 0;
while(pfm_ary_idx < pfm_ary.length)
{
item = pfm_ary[pfm_ary_idx++];
{
if (pfm_rt.is_obj(sum))
{
pfm_ret = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, item));
}
else
{
pfm_ret = sum = pfm_rt.asgn_val_op(sum, pfm_rt.plus_op(sum, item));
}
}
}
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseDoBlock) {
  constexpr wchar_t kInput[] = L"do\n  x = 1\nend";
  constexpr wchar_t kExpected[] =
      LR"***((function() {
let pfm_method_runner = function(obj, cb) {
  if (pfm_rt.is_ary(obj)) {
    let pfm_method_return = null;
    for (var idx = obj.length -1; idx > 1; idx--) {
      pfm_method_return = cb(obj[idx]);
    }
    return pfm_method_return;
  }
  return cb(obj);
};
var pfm_ret = null;
{
if (pfm_rt.is_obj(x))
{
pfm_ret = pfm_rt.asgn_val_op(x, 1);
}
else
{
pfm_ret = x = pfm_rt.asgn_val_op(x, 1);
}
}
return pfm_rt.get_val(pfm_ret);
}).call(this);)***";
  CXFA_FMLexer lexer(kInput);
  CXFA_FMParser parser(heap(), &lexer);
  CXFA_FMAST* ast = parser.Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser.HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  std::optional<WideTextBuffer> buf = ast->ToJavaScript();
  ASSERT_TRUE(buf.has_value());
  EXPECT_EQ(kExpected, buf.value().MakeString());
}

TEST_F(CXFAFMParserTest, ParseWhileErrors) {
  {
    CXFA_FMLexer lexer(L"while do endwhile");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
  {
    CXFA_FMLexer lexer(L"while (i < 10) do i = 1");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
}

TEST_F(CXFAFMParserTest, ParseForErrors) {
  {
    CXFA_FMLexer lexer(L"for i = 1 to 10 do endfor");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
  {
    CXFA_FMLexer lexer(L"for i = 1 upto 10 do i = 1");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
}

TEST_F(CXFAFMParserTest, ParseForeachErrors) {
  {
    CXFA_FMLexer lexer(L"foreach item (1, 2) endfor");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
  {
    CXFA_FMLexer lexer(L"foreach item in () endfor");
    CXFA_FMParser parser(heap(), &lexer);
    EXPECT_FALSE(parser.Parse());
    EXPECT_TRUE(parser.HasError());
  }
}

TEST_F(CXFAFMParserTest, ParseDoErrors) {
  CXFA_FMLexer lexer(L"do x = 1");
  CXFA_FMParser parser(heap(), &lexer);
  EXPECT_FALSE(parser.Parse());
  EXPECT_TRUE(parser.HasError());
}
