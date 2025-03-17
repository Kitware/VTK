// Apache License
// Version 2.0, January 2004
// http://www.apache.org/licenses/
// https://github.com/alexst07/glob-cpp

#pragma once

#include <exception>
#include <iostream>
#include <memory>
#include <stddef.h>
#include <string>
#include <tuple>
#include <vector>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss::glob {

  template <class charT> using String = std::basic_string<charT>;

  template <class charT> class Automata;

  class IOSS_EXPORT Error : public std::exception
  {
  public:
    explicit Error(std::string msg) : msg_{std::move(msg)} {}
    ~Error() override;

    const char *what() const noexcept override { return msg_.c_str(); }

  private:
    std::string msg_;
  };

  enum class StateType {
    MATCH,
    FAIL,
    CHAR,
    QUESTION,
    MULT,
    SET,
    GROUP,
    UNION,
  };

  // From cppreference.com
  template <class T, class U = T> T exchange(T &obj, U &&new_value)
  {
    T old_value = std::move(obj);
    obj         = std::forward<U>(new_value);
    return old_value;
  }

  template <class charT> class State
  {
  public:
    State(StateType type, Automata<charT> &states) : type_(type), states_(states) {}

    virtual ~State() = default;

    virtual bool Check(const String<charT> &str, size_t pos) = 0;

    virtual std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) = 0;

    StateType Type() const { return type_; }

    Automata<charT> &GetAutomata() { return states_; }

    void AddNextState(size_t state_pos) { next_states_.push_back(state_pos); }

    const std::vector<size_t> &GetNextStates() const { return next_states_; }

    const String<charT> &MatchedStr() { return matched_str_; }

    virtual void ResetState() {}

  protected:
    void SetMatchedStr(const String<charT> &str) { matched_str_ = str; }

    void SetMatchedStr(charT c) { matched_str_ = c; }

  private:
    StateType           type_;
    Automata<charT>    &states_;
    std::vector<size_t> next_states_;
    String<charT>       matched_str_;
  };

  template <class charT> class StateFail : public State<charT>
  {
  public:
    explicit StateFail(Automata<charT> &states) : State<charT>(StateType::FAIL, states) {}

    bool Check(const String<charT> &, size_t) override { return false; }

    std::tuple<size_t, size_t> Next(const String<charT> &, size_t pos) override
    {
      return {0, ++pos};
    }
  };

  template <class charT> class StateMatch : public State<charT>
  {
  public:
    explicit StateMatch(Automata<charT> &states) : State<charT>(StateType::MATCH, states) {}

    bool Check(const String<charT> &, size_t) override { return true; }

    std::tuple<size_t, size_t> Next(const String<charT> &, size_t pos) override
    {
      return {0, ++pos};
    }
  };

  template <class charT> class Automata
  {
  public:
    Automata() = default;

    Automata(const Automata<charT> &) = delete;

    Automata<charT> &operator=(const Automata<charT> &automata) = delete;

    Automata(Automata<charT> &&automata)
        : states_{std::move(automata.states_)}, match_state_{automata.match_state_},
          fail_state_{exchange(automata.fail_state_, 0)},
          start_state_{exchange(automata.start_state_, 0)}
    {
    }

    Automata<charT> &operator=(Automata<charT> &&automata)
    {
      states_      = std::move(automata.states_);
      match_state_ = automata.match_state_;
      fail_state_  = automata.fail_state_;
      start_state_ = automata.start_state_;

      return *this;
    }

    const State<charT> &GetState(size_t pos) const { return *states_[pos]; }

    State<charT> &GetState(size_t pos) { return *states_[pos]; }

    size_t FailState() const { return fail_state_; }

    Automata<charT> &SetFailState(size_t state_pos)
    {
      fail_state_ = state_pos;
      return *this;
    }

    Automata<charT> &SetMatchState(size_t state_pos)
    {
      match_state_ = state_pos;
      return *this;
    }

    size_t GetNumStates() const { return states_.size(); }

    std::tuple<bool, size_t> Exec(const String<charT> &str, bool comp_end = true)
    {
      auto r = ExecAux(str, comp_end);
      ResetStates();
      return r;
    }

    std::vector<String<charT>> GetMatchedStrings() const
    {
      std::vector<String<charT>> vec;

      for (auto &state : states_) {
        if (state->Type() == StateType::MULT || state->Type() == StateType::QUESTION ||
            state->Type() == StateType::GROUP || state->Type() == StateType::SET) {
          vec.push_back(state->MatchedStr());
        }
      }

      return vec;
    }

    template <class T, typename... Args> size_t NewState(Args &&...args)
    {
      size_t state_pos = states_.size();
      auto   state     = std::unique_ptr<State<charT>>(new T(*this, std::forward<Args>(args)...));

      states_.push_back(std::move(state));
      return state_pos;
    }

    size_t fail_state_{0};

  private:
    std::tuple<bool, size_t> ExecAux(const String<charT> &str, bool comp_end = true) const
    {
      size_t state_pos = 0;
      size_t str_pos   = 0;

      // run the state vector until state reaches fail or match state, or
      // until the string is all consumed
      while (state_pos != fail_state_ && state_pos != match_state_ && str_pos < str.length()) {
        std::tie(state_pos, str_pos) = states_[state_pos]->Next(str, str_pos);
      }

      // if comp_end is true it matches only if the automata reached the end of
      // the string
      if (comp_end) {
        if ((state_pos == match_state_) && (str_pos == str.length())) {
          return {true, str_pos};
        }

        return {false, str_pos};
      }
      // if comp_end is false, compare only if the states reached the
      // match state
      return {state_pos == match_state_, str_pos};
    }

    void ResetStates()
    {
      for (auto &state : states_) {
        state->ResetState();
      }
    }

    std::vector<std::unique_ptr<State<charT>>> states_;
    size_t                                     match_state_{};

    size_t start_state_{0};
  };

  template <class charT> class StateChar : public State<charT>
  {
    using State<charT>::GetNextStates;
    using State<charT>::GetAutomata;

  public:
    StateChar(Automata<charT> &states, charT c) : State<charT>(StateType::CHAR, states), c_{c} {}

    bool Check(const String<charT> &str, size_t pos) override { return (c_ == str[pos]); }

    std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) override
    {
      if (c_ == str[pos]) {
        this->SetMatchedStr(c_);
        return std::tuple<size_t, size_t>(GetNextStates()[0], pos + 1);
      }

      return std::tuple<size_t, size_t>(GetAutomata().FailState(), pos + 1);
    }

  private:
    charT c_;
  };

  template <class charT> class StateAny : public State<charT>
  {
    using State<charT>::GetNextStates;
    using State<charT>::GetAutomata;

  public:
    explicit StateAny(Automata<charT> &states) : State<charT>(StateType::QUESTION, states) {}

    bool Check(const String<charT> &, size_t) override
    {
      // as it match any char, it is always tried
      return true;
    }

    std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) override
    {
      this->SetMatchedStr(str[pos]);
      // state any always match with any char
      return std::tuple<size_t, size_t>(GetNextStates()[0], pos + 1);
    }
  };

  template <class charT> class StateStar : public State<charT>
  {
    using State<charT>::GetNextStates;
    using State<charT>::GetAutomata;

  public:
    explicit StateStar(Automata<charT> &states) : State<charT>(StateType::MULT, states) {}

    bool Check(const String<charT> &, size_t) override
    {
      // as it match any char, it is always tried
      return true;
    }

    std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) override
    {
      // next state vector from StateStar has two elements, the element 0 points
      // to the same state, and the element points to next state if the
      // conditions is satisfied
      if (GetAutomata().GetState(GetNextStates()[1]).Type() == StateType::MATCH) {
        // this case occurs when star is in the end of the glob, so the pos is
        // the end of the string, because all string is consumed
        this->SetMatchedStr(str.substr(pos));
        return std::tuple<size_t, size_t>(GetNextStates()[1], str.length());
      }

      bool res = GetAutomata().GetState(GetNextStates()[1]).Check(str, pos);
      // if the next state is satisfied goes to next state
      if (res) {
        return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
      }

      // while the next state check is false, the string is consumed by star state
      this->SetMatchedStr(this->MatchedStr() + str[pos]);
      return std::tuple<size_t, size_t>(GetNextStates()[0], pos + 1);
    }
  };

  template <class charT> class SetItem
  {
  public:
    SetItem()          = default;
    virtual ~SetItem() = default;

    virtual bool Check(charT c) const = 0;
  };

  template <class charT> class SetItemChar : public SetItem<charT>
  {
  public:
    explicit SetItemChar(charT c) : c_{c} {}

    bool Check(charT c) const override { return c == c_; }

  private:
    charT c_;
  };

  template <class charT> class SetItemRange : public SetItem<charT>
  {
  public:
    SetItemRange(charT start, charT end)
        : start_{start < end ? start : end}, end_{start < end ? end : start}
    {
    }

    bool Check(charT c) const override { return (c >= start_) && (c <= end_); }

  private:
    charT start_;
    charT end_;
  };

  template <class charT> class StateSet : public State<charT>
  {
    using State<charT>::GetNextStates;
    using State<charT>::GetAutomata;

  public:
    StateSet(Automata<charT> &states, std::vector<std::unique_ptr<SetItem<charT>>> items,
             bool neg = false)
        : State<charT>(StateType::SET, states), items_{std::move(items)}, neg_{neg}
    {
    }

    bool SetCheck(const String<charT> &str, size_t pos) const
    {
      // TODO: Replace with std::any_of()
      for (auto &item : items_) {
        // if any item match, then the set match with char
        if (item.get()->Check(str[pos])) {
          return true;
        }
      }

      return false;
    }

    bool Check(const String<charT> &str, size_t pos) override
    {
      if (neg_) {
        return !SetCheck(str, pos);
      }

      return SetCheck(str, pos);
    }

    std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) override
    {
      if (Check(str, pos)) {
        this->SetMatchedStr(str[pos]);
        return std::tuple<size_t, size_t>(GetNextStates()[0], pos + 1);
      }

      return std::tuple<size_t, size_t>(GetAutomata().FailState(), pos + 1);
    }

  private:
    std::vector<std::unique_ptr<SetItem<charT>>> items_;
    bool                                         neg_;
  };

  template <class charT> class StateGroup : public State<charT>
  {
    using State<charT>::GetNextStates;
    using State<charT>::GetAutomata;

  public:
    enum class Type { BASIC, ANY, STAR, PLUS, NEG, AT };

    StateGroup(Automata<charT> &states, Type type,
               std::vector<std::unique_ptr<Automata<charT>>> &&automatas)
        : State<charT>(StateType::GROUP, states), type_{type}, automatas_{std::move(automatas)}
    {
    }

    void ResetState() override { match_one_ = false; }

    std::tuple<bool, size_t> BasicCheck(const String<charT> &str, size_t pos)
    {
      String<charT> str_part = str.substr(pos);
      bool          r;
      size_t        str_pos = 0;

      // each automata is a part of a union of the group, in basic check,
      // we want find only if any automata is true
      for (auto &automata : automatas_) {
        std::tie(r, str_pos) = automata->Exec(str_part, false);
        if (r) {
          return {r, pos + str_pos};
        }
      }

      return {false, pos + str_pos};
    }

    bool Check(const String<charT> &str, size_t pos) override
    {
      bool r = false;
      switch (type_) {
      case Type::BASIC:
      case Type::AT:
      case Type::ANY:
      case Type::STAR:
      case Type::PLUS:
      case Type::NEG: {
        std::tie(r, std::ignore) = BasicCheck(str, pos);
        break;
      }
      }
      return r;
    }

    std::tuple<size_t, size_t> Next(const String<charT> &str, size_t pos) override
    {
      // STATE 1 -> is the next state
      // STATE 0 -> is the same state
      switch (type_) {
      case Type::BASIC:
      case Type::AT: {
        return NextBasic(str, pos);
      }

      case Type::ANY: {
        return NextAny(str, pos);
      }

      case Type::STAR: {
        return NextStar(str, pos);
      }

      case Type::PLUS: {
        return NextPlus(str, pos);
      }

      case Type::NEG: {
        return NextNeg(str, pos);
      }
      }
      return {0, 0};
    }

    std::tuple<size_t, size_t> NextNeg(const String<charT> &str, size_t pos)
    {
      bool   r;
      size_t new_pos;
      std::tie(r, new_pos) = BasicCheck(str, pos);
      if (r) {
        this->SetMatchedStr(this->MatchedStr() + str.substr(pos, new_pos - pos));
        return std::tuple<size_t, size_t>(GetAutomata().FailState(), new_pos);
      }

      return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
    }

    std::tuple<size_t, size_t> NextBasic(const String<charT> &str, size_t pos)
    {
      bool   r;
      size_t new_pos;
      std::tie(r, new_pos) = BasicCheck(str, pos);
      if (r) {
        this->SetMatchedStr(this->MatchedStr() + str.substr(pos, new_pos - pos));
        return std::tuple<size_t, size_t>(GetNextStates()[1], new_pos);
      }

      return std::tuple<size_t, size_t>(GetAutomata().FailState(), new_pos);
    }

    std::tuple<size_t, size_t> NextAny(const String<charT> &str, size_t pos)
    {
      bool   r;
      size_t new_pos;
      std::tie(r, new_pos) = BasicCheck(str, pos);
      if (r) {
        this->SetMatchedStr(this->MatchedStr() + str.substr(pos, new_pos - pos));
        return std::tuple<size_t, size_t>(GetNextStates()[1], new_pos);
      }

      return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
    }

    std::tuple<size_t, size_t> NextStar(const String<charT> &str, size_t pos)
    {
      bool   r;
      size_t new_pos;
      std::tie(r, new_pos) = BasicCheck(str, pos);
      if (r) {
        this->SetMatchedStr(this->MatchedStr() + str.substr(pos, new_pos - pos));
        if (GetAutomata().GetState(GetNextStates()[1]).Type() == StateType::MATCH &&
            new_pos == str.length()) {
          return std::tuple<size_t, size_t>(GetNextStates()[1], new_pos);
        }
        return std::tuple<size_t, size_t>(GetNextStates()[0], new_pos);
      }

      return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
    }

    std::tuple<size_t, size_t> NextPlus(const String<charT> &str, size_t pos)
    {
      bool   r;
      size_t new_pos;
      std::tie(r, new_pos) = BasicCheck(str, pos);
      if (r) {
        match_one_ = true;
        this->SetMatchedStr(this->MatchedStr() + str.substr(pos, new_pos - pos));

        // if it matches and the string reached at the end, and the next
        // state is the match state, goes to next state to avoid state mistake
        if (GetAutomata().GetState(GetNextStates()[1]).Type() == StateType::MATCH &&
            new_pos == str.length()) {
          return std::tuple<size_t, size_t>(GetNextStates()[1], new_pos);
        }
        return std::tuple<size_t, size_t>(GetNextStates()[0], new_pos);
      }

      // case where the next state matches and the group already matched
      // one time -> goes to next state
      bool res = GetAutomata().GetState(GetNextStates()[1]).Check(str, pos);
      if (res && match_one_) {
        return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
      }

      if (match_one_) {
        return std::tuple<size_t, size_t>(GetNextStates()[1], pos);
      }
      return std::tuple<size_t, size_t>(GetAutomata().FailState(), new_pos);
    }

  private:
    Type                                          type_{};
    std::vector<std::unique_ptr<Automata<charT>>> automatas_;
    bool                                          match_one_{false};
  };

#define TOKEN(X, Y) X,
  enum class TokenKind {
    UNKNOWN = 0,
    CHAR,
    TOKEN(EOS, "end of source") TOKEN(SUB, "-") TOKEN(STAR, "*") TOKEN(QUESTION, "?")
        TOKEN(LPAREN, "(") TOKEN(QUESTLPAREN, "?(") TOKEN(STARLPAREN, "*(") TOKEN(PLUSLPAREN, "+(")
            TOKEN(NEGLPAREN, "!(") TOKEN(ATLPAREN, "@(") TOKEN(RPAREN, ")") TOKEN(UNION, "|")
                TOKEN(LBRACKET, "[") TOKEN(RBRACKET, "]") TOKEN(NEGLBRACKET, "[^") NUM_TOKENS
  };
#undef TOKEN

#define TOKEN(X, Y) #X,
  static const char *token_name_str[] = {
      "UNKNOWN", // UNKNOWN
      "CHAR",
      TOKEN(EOS, "end of source") TOKEN(SUB, "-") TOKEN(STAR, "*") TOKEN(QUESTION, "?")
          TOKEN(LPAREN, "(") TOKEN(QUESTLPAREN, "?(") TOKEN(STARLPAREN, "*(")
              TOKEN(PLUSLPAREN, "+(") TOKEN(NEGLPAREN, "!(") TOKEN(ATLPAREN, "@(")
                  TOKEN(RPAREN, ")") TOKEN(UNION, "|") TOKEN(LBRACKET, "[") TOKEN(RBRACKET, "]")
                      TOKEN(NEGLBRACKET, "[^") ""};
#undef TOKEN

  template <class charT> class Token
  {
  public:
    explicit Token(TokenKind kind) : kind_{kind} {}
    Token(TokenKind kind, charT value) : kind_{kind}, value_{value} {}
    TokenKind Kind() const { return kind_; }

    charT Value() const { return value_; }

    bool operator==(TokenKind kind) { return kind_ == kind; }

    bool operator==(TokenKind kind) const { return kind_ == kind; }

    bool operator!=(TokenKind kind) { return kind_ != kind; }

    bool operator!=(TokenKind kind) const { return kind_ != kind; }

  private:
    template <class charU>
    friend std::ostream &operator<<(std::ostream &stream, const Token<charU> &token);

    TokenKind kind_;
    charT     value_{};
  };

  template <class charT>
  inline std::ostream &operator<<(std::ostream &stream, const Token<charT> &token)
  {
    stream << '[' << token_name_str[static_cast<int>(token.kind_)] << ']';
    return stream;
  }

  template <class charT> class Lexer
  {
  public:
    static const char kEndOfInput = -1;

    explicit Lexer(const String<charT> &str) : str_(str), c_{str[0]} {}

    std::vector<Token<charT>> Scanner()
    {
      std::vector<Token<charT>> tokens;
      while (true) {
        switch (c_) {
        case '?': {
          Advance();
          if (c_ == '(') {
            tokens.push_back(Select(TokenKind::QUESTLPAREN));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::QUESTION));
          }
          break;
        }

        case '*': {
          Advance();
          if (c_ == '(') {
            tokens.push_back(Select(TokenKind::STARLPAREN));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::STAR));
          }
          break;
        }

        case '+': {
          Advance();
          if (c_ == '(') {
            tokens.push_back(Select(TokenKind::PLUSLPAREN));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::CHAR, '+'));
          }
          break;
        }

        case '-': {
          tokens.push_back(Select(TokenKind::SUB));
          Advance();
          break;
        }

        case '|': {
          tokens.push_back(Select(TokenKind::UNION));
          Advance();
          break;
        }

        case '@': {
          Advance();
          if (c_ == '(') {
            tokens.push_back(Select(TokenKind::ATLPAREN));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::CHAR, '@'));
          }
          break;
        }

        case '!': {
          Advance();
          if (c_ == '(') {
            tokens.push_back(Select(TokenKind::NEGLPAREN));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::CHAR, '!'));
          }
          break;
        }

        case '(': {
          tokens.push_back(Select(TokenKind::LPAREN));
          Advance();
          break;
        }

        case ')': {
          tokens.push_back(Select(TokenKind::RPAREN));
          Advance();
          break;
        }

        case '[': {
          Advance();
          if (c_ == '!') {
            tokens.push_back(Select(TokenKind::NEGLBRACKET));
            Advance();
          }
          else {
            tokens.push_back(Select(TokenKind::LBRACKET));
          }
          break;
        }

        case ']': {
          tokens.push_back(Select(TokenKind::RBRACKET));
          Advance();
          break;
        }

        case '\\': {
          Advance();
          if (c_ == kEndOfInput) {
            throw Error("No valid char after '\\'");
          }
          tokens.push_back(Select(TokenKind::CHAR, c_));
          Advance();
          break;
        }

        default: {
          if (c_ == kEndOfInput) {
            tokens.push_back(Select(TokenKind::EOS));
            return tokens;
          }
          tokens.push_back(Select(TokenKind::CHAR, c_));
          Advance();
        }
        }
      }
    }

  private:
    inline Token<charT> Select(TokenKind k) { return Token<charT>(k); }

    inline Token<charT> Select(TokenKind k, charT value) { return Token<charT>(k, value); }

    void Advance()
    {
      if (pos_ == (str_.length() - 1)) {
        c_ = kEndOfInput;
        return;
      }

      c_ = str_[++pos_];
    }

    String<charT> str_;
    size_t        pos_{0};
    charT         c_;
  };

#define GLOB_AST_NODE_LIST(V)                                                                      \
  V(CharNode)                                                                                      \
  V(RangeNode)                                                                                     \
  V(SetItemsNode)                                                                                  \
  V(PositiveSetNode)                                                                               \
  V(NegativeSetNode)                                                                               \
  V(StarNode)                                                                                      \
  V(AnyNode)                                                                                       \
  V(GroupNode)                                                                                     \
  V(ConcatNode)                                                                                    \
  V(UnionNode)                                                                                     \
  V(GlobNode)

  template <class charT> class AstVisitor;

// declare all classes used for nodes
#define DECLARE_TYPE_CLASS(type) template <class charT> class type;
  GLOB_AST_NODE_LIST(DECLARE_TYPE_CLASS)

#undef DECLARE_TYPE_CLASS

  template <class charT> class AstNode
  {
  public:
    enum class Type {
      CHAR,
      RANGE,
      SET_ITEM,
      SET_ITEMS,
      POS_SET,
      NEG_SET,
      SET,
      STAR,
      ANY,
      GROUP,
      CONCAT_GLOB,
      UNION,
      GLOB
    };

    virtual ~AstNode() = default;

    Type GetType() const { return type_; }

    virtual void Accept(AstVisitor<charT> *visitor) = 0;

  protected:
    explicit AstNode(Type type) : type_{type} {}

  private:
    Type type_;
  };

  template <class charT> using AstNodePtr = std::unique_ptr<AstNode<charT>>;

  template <class charT> class AstVisitor
  {
  public:
// define all visitor methods for the nodes
#define DECLARE_VIRTUAL_FUNC(type)                                                                 \
  virtual void Visit##type(type<charT> * /*node*/) {}
    GLOB_AST_NODE_LIST(DECLARE_VIRTUAL_FUNC)
#undef DECLARE_VIRTUAL_FUNC
  };

  template <class charT> class CharNode : public AstNode<charT>
  {
  public:
    explicit CharNode(charT c) : AstNode<charT>(AstNode<charT>::Type::CHAR), c_{c} {}

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitCharNode(this); }

    char GetValue() const { return c_; }

  private:
    charT c_;
  };

  template <class charT> class RangeNode : public AstNode<charT>
  {
  public:
    RangeNode(AstNodePtr<charT> &&start, AstNodePtr<charT> &&end)
        : AstNode<charT>(AstNode<charT>::Type::RANGE), start_{std::move(start)},
          end_{std::move(end)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitRangeNode(this); }

    AstNode<charT> *GetStart() const { return start_.get(); }

    AstNode<charT> *GetEnd() const { return end_.get(); }

  private:
    AstNodePtr<charT> start_;
    AstNodePtr<charT> end_;
  };

  template <class charT> class SetItemsNode : public AstNode<charT>
  {
  public:
    explicit SetItemsNode(std::vector<AstNodePtr<charT>> &&items)
        : AstNode<charT>(AstNode<charT>::Type::SET_ITEMS), items_{std::move(items)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitSetItemsNode(this); }

    std::vector<AstNodePtr<charT>> &GetItems() { return items_; }

  private:
    std::vector<AstNodePtr<charT>> items_;
  };

  template <class charT> class PositiveSetNode : public AstNode<charT>
  {
  public:
    explicit PositiveSetNode(AstNodePtr<charT> &&set)
        : AstNode<charT>(AstNode<charT>::Type::POS_SET), set_{std::move(set)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitPositiveSetNode(this); }

    AstNode<charT> *GetSet() { return set_.get(); }

  private:
    AstNodePtr<charT> set_;
  };

  template <class charT> class NegativeSetNode : public AstNode<charT>
  {
  public:
    explicit NegativeSetNode(AstNodePtr<charT> &&set)
        : AstNode<charT>(AstNode<charT>::Type::NEG_SET), set_{std::move(set)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitNegativeSetNode(this); }

    AstNode<charT> *GetSet() { return set_.get(); }

  private:
    AstNodePtr<charT> set_;
  };

  template <class charT> class StarNode : public AstNode<charT>
  {
  public:
    StarNode() : AstNode<charT>(AstNode<charT>::Type::STAR) {}

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitStarNode(this); }
  };

  template <class charT> class AnyNode : public AstNode<charT>
  {
  public:
    AnyNode() : AstNode<charT>(AstNode<charT>::Type::ANY) {}

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitAnyNode(this); }
  };

  template <class charT> class GroupNode : public AstNode<charT>
  {
  public:
    enum class GroupType { BASIC, ANY, STAR, PLUS, NEG, AT };

    GroupNode(GroupType group_type, AstNodePtr<charT> &&glob)
        : AstNode<charT>(AstNode<charT>::Type::GROUP), glob_{std::move(glob)},
          group_type_{group_type}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitGroupNode(this); }

    AstNode<charT> *GetGlob() { return glob_.get(); }

    GroupType GetGroupType() const { return group_type_; }

  private:
    AstNodePtr<charT> glob_;
    GroupType         group_type_;
  };

  template <class charT> class ConcatNode : public AstNode<charT>
  {
  public:
    explicit ConcatNode(std::vector<AstNodePtr<charT>> &&basic_glob)
        : AstNode<charT>(AstNode<charT>::Type::CONCAT_GLOB), basic_glob_{std::move(basic_glob)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitConcatNode(this); }

    std::vector<AstNodePtr<charT>> &GetBasicGlobs() { return basic_glob_; }

  private:
    std::vector<AstNodePtr<charT>> basic_glob_;
  };

  template <class charT> class UnionNode : public AstNode<charT>
  {
  public:
    explicit UnionNode(std::vector<AstNodePtr<charT>> &&items)
        : AstNode<charT>(AstNode<charT>::Type::UNION), items_{std::move(items)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitUnionNode(this); }

    std::vector<AstNodePtr<charT>> &GetItems() { return items_; }

  private:
    std::vector<AstNodePtr<charT>> items_;
  };

  template <class charT> class GlobNode : public AstNode<charT>
  {
  public:
    explicit GlobNode(AstNodePtr<charT> &&glob)
        : AstNode<charT>(AstNode<charT>::Type::GLOB), glob_{std::move(glob)}
    {
    }

    void Accept(AstVisitor<charT> *visitor) override { visitor->VisitGlobNode(this); }

    AstNode<charT> *GetConcat() { return glob_.get(); }

  private:
    AstNodePtr<charT> glob_;
  };

  template <class charT> class Parser
  {
  public:
    Parser() = delete;

    explicit Parser(std::vector<Token<charT>> &&tok_vec) : tok_vec_{std::move(tok_vec)}, pos_{0} {}

    AstNodePtr<charT> GenAst() { return ParserGlob(); }

  private:
    AstNodePtr<charT> ParserChar()
    {
      Token<charT> &tk = NextToken();
      if (tk != TokenKind::CHAR) {
        throw Error("char expected");
      }

      charT c = tk.Value();
      return AstNodePtr<charT>(new CharNode<charT>(c));
    }

    AstNodePtr<charT> ParserRange()
    {
      AstNodePtr<charT> char_start = ParserChar();

      Token<charT> &tk = NextToken();
      if (tk != TokenKind::SUB) {
        throw Error("range expected");
      }

      AstNodePtr<charT> char_end = ParserChar();
      return AstNodePtr<charT>(new RangeNode<charT>(std::move(char_start), std::move(char_end)));
    }

    AstNodePtr<charT> ParserSetItem()
    {
      if (PeekAhead() == TokenKind::SUB) {
        return ParserRange();
      }

      return ParserChar();
    }

    AstNodePtr<charT> ParserSetItems()
    {
      std::vector<AstNodePtr<charT>> items;

      do {
        items.push_back(ParserSetItem());
      } while (GetToken() != TokenKind::RBRACKET);

      Advance();

      return AstNodePtr<charT>(new SetItemsNode<charT>(std::move(items)));
    }

    AstNodePtr<charT> ParserSet()
    {
      Token<charT> &tk = NextToken();

      if (tk == TokenKind::LBRACKET) {
        return AstNodePtr<charT>(new PositiveSetNode<charT>(ParserSetItems()));
      }
      if (tk == TokenKind::NEGLBRACKET) {
        return AstNodePtr<charT>(new NegativeSetNode<charT>(ParserSetItems()));
      }
      throw Error("set expected");
    }

    AstNodePtr<charT> ParserBasicGlob()
    {
      Token<charT> &tk = GetToken();

      switch (tk.Kind()) {
      case TokenKind::QUESTION: Advance(); return AstNodePtr<charT>(new AnyNode<charT>());

      case TokenKind::STAR: Advance(); return AstNodePtr<charT>(new StarNode<charT>());

      case TokenKind::SUB: Advance(); return AstNodePtr<charT>(new CharNode<charT>('-'));

      case TokenKind::CHAR: return ParserChar();

      case TokenKind::LBRACKET:
      case TokenKind::NEGLBRACKET: return ParserSet();

      case TokenKind::LPAREN:
      case TokenKind::QUESTLPAREN:
      case TokenKind::STARLPAREN:
      case TokenKind::PLUSLPAREN:
      case TokenKind::NEGLPAREN:
      case TokenKind::ATLPAREN: return ParserGroup();

      default: throw Error("basic glob expected");
      }
    }

    AstNodePtr<charT> ParserGroup()
    {
      typename GroupNode<charT>::GroupType type;
      Token<charT>                        &tk = NextToken();

      switch (tk.Kind()) {
      case TokenKind::LPAREN: type = GroupNode<charT>::GroupType::BASIC; break;

      case TokenKind::QUESTLPAREN: type = GroupNode<charT>::GroupType::ANY; break;

      case TokenKind::STARLPAREN: type = GroupNode<charT>::GroupType::STAR; break;

      case TokenKind::PLUSLPAREN: type = GroupNode<charT>::GroupType::PLUS; break;

      case TokenKind::NEGLPAREN: type = GroupNode<charT>::GroupType::NEG; break;

      case TokenKind::ATLPAREN: type = GroupNode<charT>::GroupType::AT; break;

      default: throw Error("Not valid group"); break;
      }

      AstNodePtr<charT> group_glob = ParserUnion();
      tk                           = NextToken();
      if (tk != TokenKind::RPAREN) {
        throw Error("Expected ')' at and of group");
      }

      return AstNodePtr<charT>(new GroupNode<charT>(type, std::move(group_glob)));
    }

    AstNodePtr<charT> ParserConcat()
    {
      auto check_end = [&]() -> bool {
        Token<charT> &tk = GetToken();

        switch (tk.Kind()) {
        case TokenKind::EOS:
        case TokenKind::RPAREN:
        case TokenKind::UNION: return true;

        default: return false;
        }
      };

      std::vector<AstNodePtr<charT>> parts;

      while (!check_end()) {
        parts.push_back(ParserBasicGlob());
      }

      return AstNodePtr<charT>(new ConcatNode<charT>(std::move(parts)));
    }

    AstNodePtr<charT> ParserUnion()
    {
      std::vector<AstNodePtr<charT>> items;
      items.push_back(ParserConcat());

      while (GetToken() == TokenKind::UNION) {
        Advance();
        items.push_back(ParserConcat());
      }

      return AstNodePtr<charT>(new UnionNode<charT>(std::move(items)));
    }

    AstNodePtr<charT> ParserGlob()
    {
      AstNodePtr<charT> glob = ParserConcat();

      if (GetToken() != TokenKind::EOS) {
        throw Error("Expected the end of glob");
      }

      return AstNodePtr<charT>(new GlobNode<charT>(std::move(glob)));
    }

    inline const Token<charT> &GetToken() const { return tok_vec_.at(pos_); }

    inline Token<charT> &GetToken() { return tok_vec_.at(pos_); }

    inline const Token<charT> &PeekAhead() const
    {
      if (pos_ >= (tok_vec_.size() - 1)) {
        return tok_vec_.back();
      }

      return tok_vec_.at(pos_ + 1);
    }

    inline Token<charT> &NextToken()
    {
      if (pos_ >= (tok_vec_.size() - 1)) {
        return tok_vec_.back();
      }

      Token<charT> &tk = tok_vec_.at(pos_);
      pos_++;
      return tk;
    }

    inline bool Advance()
    {
      if (pos_ == tok_vec_.size() - 1) {
        return false;
      }

      ++pos_;
      return true;
    }

    inline size_t Size() const noexcept { return tok_vec_.size(); }

    std::vector<Token<charT>> tok_vec_;
    size_t                    pos_;
  };

  template <class charT> class AstConsumer
  {
  public:
    AstConsumer() = default;

    void GenAutomata(AstNode<charT> *root_node, Automata<charT> &automata)
    {
      AstNode<charT> *concat_node = static_cast<GlobNode<charT> *>(root_node)->GetConcat();
      ExecConcat(concat_node, automata);

      size_t match_state = automata.template NewState<StateMatch<charT>>();
      automata.GetState(preview_state_).AddNextState(match_state);
      automata.SetMatchState(match_state);

      size_t fail_state = automata.template NewState<StateFail<charT>>();
      automata.SetFailState(fail_state);
    }

  private:
    void ExecConcat(AstNode<charT> *node, Automata<charT> &automata)
    {
      auto *concat_node = static_cast<ConcatNode<charT> *>(node);
      auto &basic_globs = concat_node->GetBasicGlobs();

      for (auto &basic_glob : basic_globs) {
        ExecBasicGlob(basic_glob.get(), automata);
      }
    }

    void ExecBasicGlob(AstNode<charT> *node, Automata<charT> &automata)
    {
      switch (node->GetType()) {
      case AstNode<charT>::Type::CHAR: ExecChar(node, automata); break;

      case AstNode<charT>::Type::ANY: ExecAny(node, automata); break;

      case AstNode<charT>::Type::STAR: ExecStar(node, automata); break;

      case AstNode<charT>::Type::POS_SET: ExecPositiveSet(node, automata); break;

      case AstNode<charT>::Type::NEG_SET: ExecNegativeSet(node, automata); break;

      case AstNode<charT>::Type::GROUP: ExecGroup(node, automata); break;

      default: break;
      }
    }

    void ExecChar(AstNode<charT> *node, Automata<charT> &automata)
    {
      auto *char_node = static_cast<CharNode<charT> *>(node);
      char  c         = char_node->GetValue();
      NewState<StateChar<charT>>(automata, c);
    }

    void ExecAny(AstNode<charT> *, Automata<charT> &automata)
    {
      NewState<StateAny<charT>>(automata);
    }

    void ExecStar(AstNode<charT> *, Automata<charT> &automata)
    {
      NewState<StateStar<charT>>(automata);
      automata.GetState(current_state_).AddNextState(current_state_);
    }

    void ExecPositiveSet(AstNode<charT> *node, Automata<charT> &automata)
    {
      auto *pos_set_node = static_cast<PositiveSetNode<charT> *>(node);

      auto items = ProcessSetItems(pos_set_node->GetSet());
      NewState<StateSet<charT>>(automata, std::move(items));
    }

    void ExecNegativeSet(AstNode<charT> *node, Automata<charT> &automata)
    {
      auto *pos_set_node = static_cast<NegativeSetNode<charT> *>(node);

      auto items = ProcessSetItems(pos_set_node->GetSet());
      NewState<StateSet<charT>>(automata, std::move(items), /*neg*/ true);
    }

    std::vector<std::unique_ptr<SetItem<charT>>> ProcessSetItems(AstNode<charT> *node)
    {
      auto *set_node = static_cast<SetItemsNode<charT> *>(node);
      std::vector<std::unique_ptr<SetItem<charT>>> vec;
      auto                                        &items = set_node->GetItems();
      for (auto &item : items) {
        vec.push_back(std::move(ProcessSetItem(item.get())));
      }

      return vec;
    }

    std::unique_ptr<SetItem<charT>> ProcessSetItem(AstNode<charT> *node)
    {
      if (node->GetType() == AstNode<charT>::Type::CHAR) {
        auto *char_node = static_cast<CharNode<charT> *>(node);
        char  c         = char_node->GetValue();
        return std::unique_ptr<SetItem<charT>>(new SetItemChar<charT>(c));
      }
      if (node->GetType() == AstNode<charT>::Type::RANGE) {
        auto *range_node = static_cast<RangeNode<charT> *>(node);
        auto *start_node = static_cast<CharNode<charT> *>(range_node->GetStart());
        auto *end_node   = static_cast<CharNode<charT> *>(range_node->GetEnd());

        char start_char = start_node->GetValue();
        char end_char   = end_node->GetValue();
        return std::unique_ptr<SetItem<charT>>(new SetItemRange<charT>(start_char, end_char));
      }
      throw Error("Not valid set item");
    }

    void ExecGroup(AstNode<charT> *node, Automata<charT> &automata)
    {
      auto *group_node = static_cast<GroupNode<charT> *>(node);
      auto *union_node = group_node->GetGlob();
      auto  automatas  = ExecUnion(union_node);

      typename StateGroup<charT>::Type state_group_type{};
      switch (group_node->GetGroupType()) {
      case GroupNode<charT>::GroupType::BASIC:
        state_group_type = StateGroup<charT>::Type::BASIC;
        break;

      case GroupNode<charT>::GroupType::ANY: state_group_type = StateGroup<charT>::Type::ANY; break;

      case GroupNode<charT>::GroupType::STAR:
        state_group_type = StateGroup<charT>::Type::STAR;
        break;

      case GroupNode<charT>::GroupType::PLUS:
        state_group_type = StateGroup<charT>::Type::PLUS;
        break;

      case GroupNode<charT>::GroupType::AT: state_group_type = StateGroup<charT>::Type::AT; break;

      case GroupNode<charT>::GroupType::NEG: state_group_type = StateGroup<charT>::Type::NEG; break;
      }

      NewState<StateGroup<charT>>(automata, state_group_type, std::move(automatas));
      automata.GetState(current_state_).AddNextState(current_state_);
    }

    std::vector<std::unique_ptr<Automata<charT>>> ExecUnion(AstNode<charT> *node)
    {
      auto *union_node = static_cast<UnionNode<charT> *>(node);
      auto &items      = union_node->GetItems();
      std::vector<std::unique_ptr<Automata<charT>>> vec_automatas;
      for (auto &item : items) {
        std::unique_ptr<Automata<charT>> automata_ptr(new Automata<charT>);
        AstConsumer                      ast_consumer;
        ast_consumer.ExecConcat(item.get(), *automata_ptr);

        size_t match_state = automata_ptr->template NewState<StateMatch<charT>>();
        automata_ptr->GetState(ast_consumer.preview_state_).AddNextState(match_state);
        automata_ptr->SetMatchState(match_state);

        size_t fail_state = automata_ptr->template NewState<StateFail<charT>>();
        automata_ptr->SetFailState(fail_state);

        vec_automatas.push_back(std::move(automata_ptr));
      }

      return vec_automatas;
    }

    template <class T, typename... Args> void NewState(Automata<charT> &automata, Args &&...args)
    {
      current_state_ = automata.template NewState<T>(std::forward<Args>(args)...);
      if (preview_state_ >= 0) {
        automata.GetState(preview_state_).AddNextState(current_state_);
      }
      preview_state_ = current_state_;
    }

    int    preview_state_ = -1;
    size_t current_state_ = 0;
  };

  template <class charT> class ExtendedGlob
  {
  public:
    explicit ExtendedGlob(const String<charT> &pattern)
    {
      Lexer<charT>              l(pattern);
      std::vector<Token<charT>> tokens = l.Scanner();
      Parser<charT>             p(std::move(tokens));
      AstNodePtr<charT>         ast_ptr = p.GenAst();

      AstConsumer<charT> ast_consumer;
      ast_consumer.GenAutomata(ast_ptr.get(), automata_);
    }

    ExtendedGlob(const ExtendedGlob &)      = delete;
    ExtendedGlob &operator=(ExtendedGlob &) = delete;

    ExtendedGlob(ExtendedGlob &&glob) : automata_{std::move(glob.automata_)} {}

    ExtendedGlob &operator=(ExtendedGlob &&glob)
    {
      automata_ = std::move(glob.automata_);
      return *this;
    }

    bool Exec(const String<charT> &str)
    {
      bool r;
      std::tie(r, std::ignore) = automata_.Exec(str);
      return r;
    }

    const Automata<charT> &GetAutomata() const { return automata_; }

  private:
    Automata<charT> automata_;
  };

  template <class charT> class SimpleGlob
  {
  public:
    explicit SimpleGlob(const String<charT> &pattern) { Parser(pattern); }

    SimpleGlob(const SimpleGlob &)      = delete;
    SimpleGlob &operator=(SimpleGlob &) = delete;

    SimpleGlob(SimpleGlob &&glob) : automata_{std::move(glob.automata_)} {}

    SimpleGlob &operator=(SimpleGlob &&glob)
    {
      automata_ = std::move(glob.automata_);
      return *this;
    }

    void Parser(const String<charT> &pattern)
    {
      size_t pos           = 0;
      int    preview_state = -1;

      while (pos < pattern.length()) {
        size_t current_state = 0;
        char   c             = pattern[pos];
        switch (c) {
        case '?': {
          current_state = automata_.template NewState<StateAny<charT>>();
          ++pos;
          break;
        }

        case '*': {
          current_state = automata_.template NewState<StateStar<charT>>();
          automata_.GetState(current_state).AddNextState(current_state);
          ++pos;
          break;
        }

        default: {
          current_state = automata_.template NewState<StateChar<charT>>(c);
          ++pos;
          break;
        }
        }

        if (preview_state >= 0) {
          automata_.GetState(preview_state).AddNextState(current_state);
        }
        preview_state = current_state;
      }

      size_t match_state = automata_.template NewState<StateMatch<charT>>();
      automata_.GetState(preview_state).AddNextState(match_state);
      automata_.SetMatchState(match_state);

      size_t fail_state = automata_.template NewState<StateFail<charT>>();
      automata_.SetFailState(fail_state);
    }

    bool Exec(const String<charT> &str) const
    {
      bool r;
      std::tie(r, std::ignore) = automata_.Exec(str);
      return r;
    }

    const Automata<charT> &GetAutomata() const { return automata_; }

  private:
    Automata<charT> automata_;
  };

  template <class charT> using extended_glob = ExtendedGlob<charT>;

  template <class charT> using no_extended_glob = SimpleGlob<charT>;

  template <class charT> class MatchResults;

  template <class charT, class globT = extended_glob<charT>> class BasicGlob
  {
  public:
    explicit BasicGlob(const String<charT> &pattern) : glob_{pattern} {}

    BasicGlob(const BasicGlob &)      = delete;
    BasicGlob &operator=(BasicGlob &) = delete;

    BasicGlob(BasicGlob &&glob) : glob_{std::move(glob.glob_)} {}

    BasicGlob &operator=(BasicGlob &&glob)
    {
      glob_ = std::move(glob.glob_);
      return *this;
    }

    const Automata<charT> &GetAutomata() const { return glob_.GetAutomata(); }

  private:
    bool Exec(const String<charT> &str) { return glob_.Exec(str); }

    template <class charU, class globU>
    friend bool glob_match(const String<charU> &str, BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const charU *str, BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const String<charU> &str, MatchResults<charU> &res,
                           BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const charU *str, MatchResults<charU> &res,
                           BasicGlob<charU, globU> &glob);

    globT glob_;
  };

  template <class charT> class MatchResults
  {
  public:
    using const_iterator = typename std::vector<String<charT>>::const_iterator;

    MatchResults() = default;

    MatchResults(const MatchResults &m) : results_{m.results_} {}

    MatchResults(MatchResults &&m) : results_{std::move(m.results_)} {}

    MatchResults &operator=(const MatchResults &m)
    {
      results_ = m.results_;

      return *this;
    }

    MatchResults &operator=(MatchResults &&m)
    {
      results_ = std::move(m.results_);

      return *this;
    }

    bool empty() const { return results_.empty(); }

    size_t size() const { return results_.size(); }

    const_iterator begin() const noexcept { return results_.begin(); }

    const_iterator end() const noexcept { return results_.end(); }

    const_iterator cbegin() const noexcept { return results_.cbegin(); }

    const_iterator cend() const noexcept { return results_.cend(); }

    String<charT> &operator[](size_t n) const { return results_[n]; }

  private:
    void SetResults(std::vector<String<charT>> &&results) { results_ = std::move(results); }

    template <class charU, class globU>
    friend bool glob_match(const String<charU> &str, BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const charU *str, BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const String<charU> &str, MatchResults<charU> &res,
                           BasicGlob<charU, globU> &glob);

    template <class charU, class globU>
    friend bool glob_match(const charU *str, MatchResults<charU> &res,
                           BasicGlob<charU, globU> &glob);

    std::vector<String<charT>> results_;
  };

  template <class charT, class globT = extended_glob<charT>>
  bool glob_match(const String<charT> &str, BasicGlob<charT, globT> &glob)
  {
    return glob.Exec(str);
  }

  template <class charT, class globT = extended_glob<charT>>
  bool glob_match(const charT *str, BasicGlob<charT, globT> &glob)
  {
    return glob.Exec(str);
  }

  template <class charT, class globT = extended_glob<charT>>
  bool glob_match(const String<charT> &str, MatchResults<charT> &res, BasicGlob<charT, globT> &glob)
  {
    bool r = glob.Exec(str);
    res.SetResults(glob.GetAutomata().GetMatchedStrings());
    return r;
  }

  template <class charT, class globT = extended_glob<charT>>
  bool glob_match(const charT *str, MatchResults<charT> &res, BasicGlob<charT, globT> &glob)
  {
    bool r = glob.Exec(str);
    res.SetResults(glob.GetAutomata().GetMatchedStrings());
    return r;
  }

  template <class charT, class globT = extended_glob<charT>>
  using basic_glob = BasicGlob<charT, globT>;

  using glob = basic_glob<char, extended_glob<char>>;

  using wglob = basic_glob<wchar_t, extended_glob<wchar_t>>;

  using cmatch = MatchResults<char>;

  using wmatch = MatchResults<wchar_t>;

} // namespace Ioss::glob
