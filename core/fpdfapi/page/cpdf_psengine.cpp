// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_psengine.h"

#include <math.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>

#include "core/fpdfapi/parser/cpdf_simple_parser.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_string.h"

namespace {

struct PDF_PSOpName {
  // Inline string data reduces size for small strings.
  const char name[9];
  PDF_PSOP op;
};

constexpr PDF_PSOpName kPsOpNames[] = {
    {"abs", PSOP_ABS},
    {"add", PSOP_ADD},
    {"and", PSOP_AND},
    {"atan", PSOP_ATAN},
    {"bitshift", PSOP_BITSHIFT},
    {"ceiling", PSOP_CEILING},
    {"copy", PSOP_COPY},
    {"cos", PSOP_COS},
    {"cvi", PSOP_CVI},
    {"cvr", PSOP_CVR},
    {"div", PSOP_DIV},
    {"dup", PSOP_DUP},
    {"eq", PSOP_EQ},
    {"exch", PSOP_EXCH},
    {"exp", PSOP_EXP},
    {"false", PSOP_FALSE},
    {"floor", PSOP_FLOOR},
    {"ge", PSOP_GE},
    {"gt", PSOP_GT},
    {"idiv", PSOP_IDIV},
    {"if", PSOP_IF},
    {"ifelse", PSOP_IFELSE},
    {"index", PSOP_INDEX},
    {"le", PSOP_LE},
    {"ln", PSOP_LN},
    {"log", PSOP_LOG},
    {"lt", PSOP_LT},
    {"mod", PSOP_MOD},
    {"mul", PSOP_MUL},
    {"ne", PSOP_NE},
    {"neg", PSOP_NEG},
    {"not", PSOP_NOT},
    {"or", PSOP_OR},
    {"pop", PSOP_POP},
    {"roll", PSOP_ROLL},
    {"round", PSOP_ROUND},
    {"sin", PSOP_SIN},
    {"sqrt", PSOP_SQRT},
    {"sub", PSOP_SUB},
    {"true", PSOP_TRUE},
    {"truncate", PSOP_TRUNCATE},
    {"xor", PSOP_XOR},
};

// Round half up is a nearest integer round with half-way numbers always rounded
// up. Example: -5.5 rounds to -5.
float RoundHalfUp(float f) {
  if (isnan(f)) {
    return 0;
  }
  if (f > std::numeric_limits<float>::max() - 0.5f) {
    return std::numeric_limits<float>::max();
  }
  return floor(f + 0.5f);
}

}  // namespace

CPDF_PSOP::CPDF_PSOP()
    : op_(PSOP_PROC), value_(0), proc_(std::make_unique<CPDF_PSProc>()) {}

CPDF_PSOP::CPDF_PSOP(PDF_PSOP op) : op_(op), value_(0) {
  DCHECK(op_ != PSOP_CONST);
  DCHECK(op_ != PSOP_PROC);
}

CPDF_PSOP::CPDF_PSOP(float value) : op_(PSOP_CONST), value_(value) {}

CPDF_PSOP::~CPDF_PSOP() = default;

bool CPDF_PSOP::Parse(CPDF_SimpleParser* parser, int depth) {
  CHECK_EQ(op_, PSOP_PROC);
  return proc_->Parse(parser, depth);
}

void CPDF_PSOP::Execute(CPDF_PSEngine* pEngine) {
  CHECK_EQ(op_, PSOP_PROC);
  proc_->Execute(pEngine);
}

float CPDF_PSOP::GetFloatValue() const {
  CHECK_EQ(op_, PSOP_CONST);
  return value_;
}

bool CPDF_PSEngine::Execute() {
  return main_proc_.Execute(this);
}

CPDF_PSProc::CPDF_PSProc() = default;

CPDF_PSProc::~CPDF_PSProc() = default;

bool CPDF_PSProc::Parse(CPDF_SimpleParser* parser, int depth) {
  if (depth > kMaxDepth) {
    return false;
  }

  while (true) {
    ByteStringView word = parser->GetWord();
    if (word.IsEmpty()) {
      return false;
    }

    if (word == "}") {
      return true;
    }

    if (word == "{") {
      operators_.push_back(std::make_unique<CPDF_PSOP>());
      if (!operators_.back()->Parse(parser, depth + 1)) {
        return false;
      }
      continue;
    }

    AddOperator(word);
  }
}

bool CPDF_PSProc::Execute(CPDF_PSEngine* pEngine) {
  for (size_t i = 0; i < operators_.size(); ++i) {
    const PDF_PSOP op = operators_[i]->GetOp();
    if (op == PSOP_PROC) {
      continue;
    }

    if (op == PSOP_CONST) {
      pEngine->Push(operators_[i]->GetFloatValue());
      continue;
    }

    if (op == PSOP_IF) {
      if (i == 0 || operators_[i - 1]->GetOp() != PSOP_PROC) {
        return false;
      }

      if (pEngine->PopInt()) {
        operators_[i - 1]->Execute(pEngine);
      }
    } else if (op == PSOP_IFELSE) {
      if (i < 2 || operators_[i - 1]->GetOp() != PSOP_PROC ||
          operators_[i - 2]->GetOp() != PSOP_PROC) {
        return false;
      }
      size_t offset = pEngine->PopInt() ? 2 : 1;
      operators_[i - offset]->Execute(pEngine);
    } else {
      pEngine->DoOperator(op);
    }
  }
  return true;
}

void CPDF_PSProc::AddOperatorForTesting(ByteStringView word) {
  AddOperator(word);
}

void CPDF_PSProc::AddOperator(ByteStringView word) {
  const auto* pFound = std::ranges::lower_bound(kPsOpNames, word, std::less<>{},
                                                &PDF_PSOpName::name);
  if (pFound != std::end(kPsOpNames) && pFound->name == word) {
    operators_.push_back(std::make_unique<CPDF_PSOP>(pFound->op));
  } else {
    operators_.push_back(std::make_unique<CPDF_PSOP>(StringToFloat(word)));
  }
}

CPDF_PSEngine::CPDF_PSEngine() = default;

CPDF_PSEngine::~CPDF_PSEngine() = default;

void CPDF_PSEngine::Push(float v) {
  if (stack_count_ < kPSEngineStackSize) {
    stack_[stack_count_++] = v;
  }
}

float CPDF_PSEngine::Pop() {
  return stack_count_ > 0 ? stack_[--stack_count_] : 0;
}

int CPDF_PSEngine::PopInt() {
  return static_cast<int>(Pop());
}

bool CPDF_PSEngine::Parse(pdfium::span<const uint8_t> input) {
  CPDF_SimpleParser parser(input);
  return parser.GetWord() == "{" && main_proc_.Parse(&parser, 0);
}

bool CPDF_PSEngine::DoOperator(PDF_PSOP op) {
  int i1;
  int i2;
  float d1;
  float d2;
  FX_SAFE_INT32 result;
  switch (op) {
    case PSOP_ADD:
      d1 = Pop();
      d2 = Pop();
      Push(d1 + d2);
      break;
    case PSOP_SUB:
      d2 = Pop();
      d1 = Pop();
      Push(d1 - d2);
      break;
    case PSOP_MUL:
      d1 = Pop();
      d2 = Pop();
      Push(d1 * d2);
      break;
    case PSOP_DIV:
      d2 = Pop();
      d1 = Pop();
      Push(d2 ? d1 / d2 : 0);
      break;
    case PSOP_IDIV:
      i2 = PopInt();
      i1 = PopInt();
      if (i2) {
        result = i1;
        result /= i2;
        Push(result.ValueOrDefault(0));
      } else {
        Push(0);
      }
      break;
    case PSOP_MOD:
      i2 = PopInt();
      i1 = PopInt();
      if (i2) {
        result = i1;
        result %= i2;
        Push(result.ValueOrDefault(0));
      } else {
        Push(0);
      }
      break;
    case PSOP_NEG:
      d1 = Pop();
      Push(-d1);
      break;
    case PSOP_ABS:
      d1 = Pop();
      Push(fabs(d1));
      break;
    case PSOP_CEILING:
      d1 = Pop();
      Push(ceil(d1));
      break;
    case PSOP_FLOOR:
      d1 = Pop();
      Push(floor(d1));
      break;
    case PSOP_ROUND:
      d1 = Pop();
      Push(RoundHalfUp(d1));
      break;
    case PSOP_TRUNCATE:
      i1 = PopInt();
      Push(i1);
      break;
    case PSOP_SQRT:
      d1 = Pop();
      Push(sqrt(d1));
      break;
    case PSOP_SIN:
      d1 = Pop();
      Push(sin(d1 * FXSYS_PI / 180.0f));
      break;
    case PSOP_COS:
      d1 = Pop();
      Push(cos(d1 * FXSYS_PI / 180.0f));
      break;
    case PSOP_ATAN:
      d2 = Pop();
      d1 = Pop();
      d1 = atan2(d1, d2) * 180.0 / FXSYS_PI;
      if (d1 < 0) {
        d1 += 360;
      }
      Push(d1);
      break;
    case PSOP_EXP:
      d2 = Pop();
      d1 = Pop();
      Push(powf(d1, d2));
      break;
    case PSOP_LN:
      d1 = Pop();
      Push(log(d1));
      break;
    case PSOP_LOG:
      d1 = Pop();
      Push(log10(d1));
      break;
    case PSOP_CVI:
      i1 = PopInt();
      Push(i1);
      break;
    case PSOP_CVR:
      break;
    case PSOP_EQ:
      d2 = Pop();
      d1 = Pop();
      Push(d1 == d2);
      break;
    case PSOP_NE:
      d2 = Pop();
      d1 = Pop();
      Push(d1 != d2);
      break;
    case PSOP_GT:
      d2 = Pop();
      d1 = Pop();
      Push(d1 > d2);
      break;
    case PSOP_GE:
      d2 = Pop();
      d1 = Pop();
      Push(d1 >= d2);
      break;
    case PSOP_LT:
      d2 = Pop();
      d1 = Pop();
      Push(d1 < d2);
      break;
    case PSOP_LE:
      d2 = Pop();
      d1 = Pop();
      Push(d1 <= d2);
      break;
    case PSOP_AND:
      i1 = PopInt();
      i2 = PopInt();
      Push(i1 & i2);
      break;
    case PSOP_OR:
      i1 = PopInt();
      i2 = PopInt();
      Push(i1 | i2);
      break;
    case PSOP_XOR:
      i1 = PopInt();
      i2 = PopInt();
      Push(i1 ^ i2);
      break;
    case PSOP_NOT:
      i1 = PopInt();
      Push(!i1);
      break;
    case PSOP_BITSHIFT: {
      int shift = PopInt();
      result = PopInt();
      if (shift > 0) {
        result <<= shift;
      } else {
        // Avoids unsafe negation of INT_MIN.
        FX_SAFE_INT32 safe_shift = shift;
        result >>= (-safe_shift).ValueOrDefault(0);
      }
      Push(result.ValueOrDefault(0));
      break;
    }
    case PSOP_TRUE:
      Push(1);
      break;
    case PSOP_FALSE:
      Push(0);
      break;
    case PSOP_POP:
      Pop();
      break;
    case PSOP_EXCH:
      d2 = Pop();
      d1 = Pop();
      Push(d2);
      Push(d1);
      break;
    case PSOP_DUP:
      d1 = Pop();
      Push(d1);
      Push(d1);
      break;
    case PSOP_COPY: {
      int n = PopInt();
      if (n < 0 || stack_count_ + n > kPSEngineStackSize ||
          n > static_cast<int>(stack_count_)) {
        break;
      }
      for (int i = 0; i < n; i++) {
        stack_[stack_count_ + i] = stack_[stack_count_ + i - n];
      }
      stack_count_ += n;
      break;
    }
    case PSOP_INDEX: {
      int n = PopInt();
      if (n < 0 || n >= static_cast<int>(stack_count_)) {
        break;
      }
      Push(stack_[stack_count_ - n - 1]);
      break;
    }
    case PSOP_ROLL: {
      int j = PopInt();
      int n = PopInt();
      if (j == 0 || n == 0 || stack_count_ == 0) {
        break;
      }
      if (n < 0 || n > static_cast<int>(stack_count_)) {
        break;
      }

      j %= n;
      if (j > 0) {
        j -= n;
      }
      auto begin_it = std::begin(stack_) + stack_count_ - n;
      auto middle_it = begin_it - j;
      auto end_it = std::begin(stack_) + stack_count_;
      std::rotate(begin_it, middle_it, end_it);
      break;
    }
    default:
      break;
  }
  return true;
}
