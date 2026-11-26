# Phase 06 — SQL Lexer and Parser (`DAY-153` – `DAY-178`)

**Phase Budget:** 20.0 hours (24 × 45 min + 2 × 60 min = 26 sessions)
**Goal:** Build TinyDB's SQL subset formal grammar, line/column-aware `Lexer`, recursive-descent `Parser`, strongly-typed `AST`, strict resource/depth limits, clear syntax error messages, and parser boundary/fuzz tests.

---

## DAY-153 — Define TinyDB v1 SQL Formal Grammar (EBNF) and `SourceLocation` Types

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-153
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Define TinyDB v1 SQL Formal Grammar (EBNF) and `SourceLocation` Types
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 05 complete (`DAY-152`).

### Objective
Write the complete unambiguous EBNF grammar for TinyDB's v1 SQL subset in `docs/sql-spec.md` and implement `SourceLocation` (`line`, `column`, `offset`) and `Diagnostic` formatting helpers in `include/tinydb/parser/source_location.hpp`.

### Concepts & Daily Learning
- **Concept:** Context-free grammars (EBNF), operator precedence stratification (`and_expr -> comparison_expr -> primary_expr`), and 1-based line/column diagnostic spans.
- **Why Database Engines Need It:** Writing an ad-hoc parser without an explicit grammar creates shift/reduce ambiguities and inconsistent parentheses/precedence bugs; writing down the EBNF first makes recursive-descent parsing mechanical and bug-free!
- **How It Works:** Stratify `where_expr := and_expr`, `and_expr := comparison ('AND' comparison)*`, `comparison := primary (comp_op primary)?`, `primary := literal | identifier | '(' where_expr ')'`. Define `SourceLocation { uint32_t line = 1; uint32_t column = 1; uint32_t offset = 0; }`.
- **Design Trade-offs:** Keeping the grammar explicitly LL(1)/LL(2) recursive-descent avoids needing Flex/Bison build dependencies while giving crisp human-friendly error messages.
- **Common Mistakes:** Confusing `0`-based string offsets with `1`-based user-facing line/column numbers.
- **Understanding Checkpoint:** How does encoding `AND` at a lower precedence level than `=`, `<`, `>` in EBNF naturally prevent `a = 1 AND b = 2` from parsing as `a = (1 AND b) = 2`?

### Component Context
- **Why It Exists:** Formal blueprint and error-location primitives for the SQL frontend.
- **Module Relationships:** Used by `Token`, `Lexer`, `Parser`, and CLI error reporting.

### Files Expected to Change
- `docs/sql-spec.md` (add full EBNF grammar + precedence table)
- `include/tinydb/parser/source_location.hpp` (new)
- `tests/unit/parser/source_location_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Finalize the v1 EBNF grammar in `docs/sql-spec.md`.
2. Create `source_location.hpp` with `SourceLocation` and `FormatSyntaxError(location, msg)`.
3. Add unit tests verifying formatting and equality.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Grammar covers all required v1 statements without ambiguity; `SourceLocation` compiles and tests pass.

### Definition of Done
EBNF grammar and `SourceLocation` complete, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-153: specify SQL EBNF grammar and SourceLocation diagnostics`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-154` (`TokenType` enum & `Token` struct).
````

---

## DAY-154 — Define `TokenType` Enum and `Token` Representation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-154
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Define `TokenType` Enum and `Token` Representation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-153 complete.

### Objective
Define `TokenType` enum (keywords `CREATE`, `DROP`, `TABLE`, `INDEX`, `ON`, `INSERT`, `INTO`, `VALUES`, `SELECT`, `FROM`, `WHERE`, `UPDATE`, `SET`, `DELETE`, `AND`, `INT`, `BIGINT`, `BOOL`, `VARCHAR`, `TRUE`, `FALSE`; identifiers; integer & string literals; operators `=`, `!=`, `<>`, `<`, `<=`, `>`, `>=`, `*`; punctuation `(`, `)`, `,`, `;`; `EOF`, `INVALID`) and `Token` struct in `include/tinydb/parser/token.hpp` and `src/parser/token.cpp`.

### Concepts & Daily Learning
- **Concept:** Lexical token categories, separating keywords from identifiers, and carrying parsed literal values/lexemes alongside `SourceLocation`.
- **Why Database Engines Need It:** Decoupling lexical scanning (`Lexer`) from grammar analysis (`Parser`) simplifies whitespace/comment handling and lets the parser operate uniformly on structured `Token`s instead of raw characters.
- **How It Works:** `Token` stores `TokenType type`, `std::string lexeme` (or unescaped string value for string literals, signed `int64_t` optional value or parsed value in `Lexer`), and `SourceLocation location`. Include `TokenTypeName(TokenType)` for readable parser error diagnostics (`"Expected ';' near 'FROM' at line 2, column 1"`).
- **Design Trade-offs:** Storing `std::string lexeme` (copying tiny tokens) avoids lifetime dangling if callers drop the input query string after lexing!
- **Common Mistakes:** Missing `TokenType::kInvalid` or `TokenType::kEof` (forcing parser to check array bounds inconsistently instead of matching `kEof` cleanly!).
- **Understanding Checkpoint:** Why is an explicit `TokenType::kEof` sentinel token at the end of the token stream cleaner for a recursive-descent parser than returning `std::optional<Token>`?

### Component Context
- **Why It Exists:** Currency passed between `Lexer` and `Parser`.
- **Module Relationships:** Output of `Lexer` (`DAY-155`–`DAY-159`), input to `Parser` (`DAY-165`+).

### Files Expected to Change
- `include/tinydb/parser/token.hpp` (new)
- `src/parser/token.cpp` (new)
- `tests/unit/parser/token_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TokenType`, `Token`, `TokenTypeName`, and `IsKeyword`.
2. Add unit tests verifying `TokenTypeName` coverage and `Token` helpers.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every `TokenType` has a clean human-readable name; `Token` compiles with `-Werror`.

### Definition of Done
Tokens defined and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-154: define SQL TokenType enum and Token representation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-155` (`Lexer` skeleton & whitespace/cursor tracking).
````

---

## DAY-155 — Implement `Lexer` Skeleton, Cursor Navigation, and Line/Column Tracking

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-155
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `Lexer` Skeleton, Cursor Navigation, and Line/Column Tracking
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-154 complete.

### Objective
Create `tinydb::Lexer` (`include/tinydb/parser/lexer.hpp`, `src/parser/lexer.cpp`) implementing bounded input scanning (`<= config::kMaxSqlQueryBytes`), UTF-8/ASCII cursor helpers (`Peek`, `PeekNext`, `Advance`, `IsAtEnd`), whitespace (`' '`, `'\t'`, `'\r'`, `'\n'`) skipping, and `--` single-line SQL comment skipping with exact line/column updates!

### Concepts & Daily Learning
- **Concept:** Single-pass scanner state machine, CRLF (`\r\n`) vs LF (`\n`) line counting, and bounded query limits.
- **Why Database Engines Need It:** Multi-line SQL scripts in `.sql` files and CLI input contain newlines, tabs, and `-- comments`; accurate `line`/`column` tracking lets users pinpoint syntax errors instantly without counting characters!
- **How It Works:** `Advance()` increments `offset_`, resets `column_ = 1` and increments `line_++` on `'\n'`, otherwise increments `column_++`. `SkipWhitespaceAndComments()` loops over spaces/tabs/newlines and `-- ... \n`. Reject query inputs `> kMaxSqlQueryBytes` (`64 KiB`) up front!
- **Design Trade-offs:** Enforcing input size limit at lex time stops megabyte-sized adversarial SQL strings before allocating token vectors!
- **Common Mistakes:** Calling `input_[offset_ + 1]` without checking `offset_ + 1 < input_.size()`; incrementing `column_` wrongly on `'\n'`.
- **Understanding Checkpoint:** What happens to `line` and `column` across `"SELECT\n  -- comment\n  *"`? Where must the `*` token report its `SourceLocation`?

### Component Context
- **Why It Exists:** Character-stream cursor and comment/whitespace stripper for SQL lexing.
- **Module Relationships:** Completed incrementally across `DAY-156`–`DAY-159`.

### Files Expected to Change
- `include/tinydb/parser/lexer.hpp` (new)
- `src/parser/lexer.cpp` (new)
- `tests/unit/parser/lexer_cursor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Lexer` skeleton, input limit guard (`kMaxSqlQueryBytes`), cursor helpers, and `SkipWhitespaceAndComments`.
2. Add tests verifying whitespace/comment skipping, line/col tracking across `\n` and `\r\n`, empty input (`kEof` at 1:1), and oversized query rejection (`kResourceExhausted` / `kInvalidArgument`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Empty/comment-only input returns `[kEof]` cleanly; multiline line/col coordinates match exact 1-based positions.

### Definition of Done
`Lexer` cursor skeleton tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-155: implement Lexer skeleton, comment skipping, and line/col tracking`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-156` (Keywords & identifiers).
````

---

## DAY-156 — Implement SQL Keyword Recognition and Identifier Scanning

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-156
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement SQL Keyword Recognition and Identifier Scanning
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-155 complete.

### Objective
Implement `ScanIdentifierOrKeyword()` in `src/parser/lexer.cpp` recognizing `[A-Za-z_][A-Za-z0-9_]*`, performing case-insensitive SQL keyword lookup (`SELECT`, `select`, `SeLeCt` -> `TokenType::kSelect`), enforcing identifier length limits (`<= 64`), and preserving/normalizing identifier lexemes cleanly.

### Concepts & Daily Learning
- **Concept:** Maximal-munch identifier scanning, ASCII case-folding (`std::tolower(static_cast<unsigned char>(c))`), and keyword lookup table.
- **Why Database Engines Need It:** SQL keywords are case-insensitive (`select * from users`), and `SELECT_ID` must lex as a single identifier `SELECT_ID` rather than keyword `SELECT` + `_ID` (maximal munch!).
- **How It Works:** Consume all `[A-Za-z0-9_]` starting with `[A-Za-z_]`. Uppercase/lowercase an ASCII temporary copy to look up in a `static const std::unordered_map<std::string_view, TokenType>` (or sorted `constexpr` array!). If not a keyword, validate length `<= 64` and emit `TokenType::kIdentifier`.
- **Design Trade-offs:** Passing `unsigned char` to `std::tolower` / `std::isalpha` avoids classic C++ Undefined Behavior on negative `char` values (`> 127`)!
- **Common Mistakes:** Calling `std::isalpha(ch)` directly on signed `char` (UB for non-ASCII UTF-8 bytes!); matching prefix `"SELE"` or `"FROMAGE"` as keyword `"FROM"` + `"AGE"`.
- **Understanding Checkpoint:** Why is calling `std::isalpha(c)` directly on a `char` without `static_cast<unsigned char>(c)` Undefined Behavior in C++?

### Component Context
- **Why It Exists:** Distinguishes SQL grammar keywords from table/column/index identifiers.
- **Module Relationships:** Core scanner branch inside `Lexer::NextToken`.

### Files Expected to Change
- `src/parser/lexer.cpp`
- `tests/unit/parser/lexer_keywords_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement ASCII-safe `IsIdentStart` / `IsIdentChar`, keyword lookup table (all v1 keywords including `INT`, `BIGINT`, `BOOL`, `VARCHAR`, `TRUE`, `FALSE`), and `ScanIdentifierOrKeyword`.
2. Add tests verifying case-insensitive keywords, maximal munch (`select_all`, `from2`, `create_table`), underscores, and >64-char identifier error reporting.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All v1 keywords recognized case-insensitively; `select_1` stays `kIdentifier`; >64-char identifiers report `kInvalid`/`kSyntaxError`.

### Definition of Done
Keyword/identifier lexer tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-156: implement case-insensitive SQL keywords and identifier scanning`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-157` (Integer literal scanning & overflow checks).
````

---

## DAY-157 — Implement Integer Literal Scanning and Overflow Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-157
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Integer Literal Scanning and Overflow Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-156 complete.

### Objective
Implement `ScanNumber()` in `src/parser/lexer.cpp` scanning unsigned digit sequences `[0-9]+` (or unary `-` handled cleanly in Parser/Expression so `-9223372036854775808` (`INT64_MIN`) works without false overflow, AND rejecting invalid trailing suffixes like `123abc` or `12.34` with explicit unsupported/invalid diagnostics!).

### Concepts & Daily Learning
- **Concept:** Lexical number scanning, `INT64_MIN` two's-complement asymmetry (`|+9223372036854775808| > INT64_MAX`), and rejecting malformed numeric literals (`123abc`, `3.14`).
- **Why Database Engines Need It:** Notice `-9223372036854775808` (`INT64_MIN`) cannot have its magnitude stored in a signed `int64_t` (`INT64_MAX = 9223372036854775807`) if `-` is parsed separately without `uint64_t` magnitude handling! And silently parsing `123abc` as `123` followed by identifier `abc` would mask syntax errors!
- **How It Works:** Scan digits `[0-9]+`; if immediately followed by `.` (unsupported `FLOAT`/`DECIMAL` in v1!) or `[A-Za-z_]`, emit clear error (`"Floating-point literals are not supported in v1"` or `"Invalid numeric literal"`). Parse digits via `std::from_chars` into `uint64_t` (checking `<= 9223372036854775808ULL`—if `9223372036854775808ULL`, flag/allow only when unary `-` precedes it in `Parser`, otherwise reject `> INT64_MAX`!).
- **Design Trade-offs:** Parsing unsigned magnitude in `Lexer` + applying unary `-`/`+` in `Parser` handles both `SELECT ... WHERE x = -9223372036854775808` AND binary subtraction cleanly (wait: since v1 doesn't have binary `-` arithmetic, unary `-` only prefixes literals!).
- **Common Mistakes:** Using `std::stoll` (locale-dependent, throws exceptions) or overflowing signed `int64_t` during accumulation!
- **Understanding Checkpoint:** Why does `INT64_MIN` (`-9223372036854775808`) require care if the lexer scans `-` and `9223372036854775808` as two tokens?

### Component Context
- **Why It Exists:** Safe numeric literal scanning for `VALUES`, `VARCHAR(n)`, and `WHERE` predicates.
- **Module Relationships:** Paired with `Parser::ParseLiteral` (`DAY-167`).

### Files Expected to Change
- `include/tinydb/parser/token.hpp`
- `src/parser/lexer.cpp`
- `tests/unit/parser/lexer_numbers_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ScanNumber()` using overflow-safe digit parsing (no exceptions, no signed overflow UB!).
2. Add tests: `0`, `42`, `9223372036854775807`, `9223372036854775808` (boundary metadata), `99999999999999999999999` (overflow rejected!), `3.14` (rejected cleanly!), `123foo` (rejected cleanly!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero signed-overflow UB under UBSan; invalid numeric suffixes and float literals produce crisp line/col error diagnostics.

### Definition of Done
Numeric lexer implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-157: implement overflow-safe SQL integer literal scanning`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-158` (String literals & escape sequences).
````

---

## DAY-158 — Implement String Literal Scanning, Escape Sequences, and Unterminated Errors

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-158
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement String Literal Scanning, Escape Sequences, and Unterminated Errors
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-157 complete.

### Objective
Implement `ScanString()` in `src/parser/lexer.cpp` supporting single-quoted SQL strings `'...'`, standard SQL doubled-quote escapes (`''` -> `'`), backslash escapes (`\'`, `\\`, `\n`, `\t`, `\r`), string-length bounds (`<= config::kMaxVarcharLength`), and reporting the exact *opening* quote `SourceLocation` when a string literal is unterminated!

### Concepts & Daily Learning
- **Concept:** String escape state machines, doubled single quotes (`'It''s ok'`), and accurate error origin reporting.
- **Why Database Engines Need It:** Strings frequently contain apostrophes (`'O''Reilly'`), newlines, or unclosed quotes (`'hello`); reporting the opening line/column of an unterminated string helps users locate missing closing quotes immediately instead of pointing at EOF!
- **How It Works:** Record `start_loc` at opening `'`. Loop while `!IsAtEnd()`:
  - If `Peek() == '\''`: consume; if `Peek() == '\''` (doubled `''`), consume and append `'\''`; else string closes—emit `kStringLiteral`!
  - If `Peek() == '\\'`: consume; handle next char (`'`, `\`, `n`, `t`, `r`, or invalid escape error!).
  - Check `value.size() <= config::kMaxVarcharLength` (bounded memory!).
  - If `IsAtEnd()` before closing quote: return `SyntaxError` anchored at `start_loc` ("Unterminated string literal").
- **Design Trade-offs:** Supporting both SQL-standard `''` AND `\'` makes TinyDB friendly to users familiar with SQLite/PostgreSQL or MySQL/C-escapes (documented clearly in `docs/sql-spec.md`).
- **Common Mistakes:** Failing to advance past BOTH quotes on `''` or past BOTH chars on `\\'` (causing `\'` at the end of `'abc\\'` to look like an escaped closing quote!! Think carefully: `'abc\\'` is backslash escaping backslash, then closing `'`!).
- **Understanding Checkpoint:** Trace lexing `'abc\\'` vs `'abc\''`—why must `\\` consume BOTH backslashes immediately so the trailing `'` closes the string rather than being treated as escaped?

### Component Context
- **Why It Exists:** Safe SQL string literal parsing.
- **Module Relationships:** Feeds string `LiteralExpr`s in `Parser`.

### Files Expected to Change
- `src/parser/lexer.cpp`
- `docs/sql-spec.md` (document string escape rules)
- `tests/unit/parser/lexer_strings_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ScanString()` in `src/parser/lexer.cpp`.
2. Test: empty `''`, `'hello'`, `'it''s'`, `'a\'b'`, `'backslash\\'`, `'\n\t'`, embedded newlines inside string, oversized string (> `kMaxVarcharLength`), unknown escape `\q`, and unterminated `'abc` reporting opening `SourceLocation`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `'backslash\\'` terminates cleanly with value `"backslash\\"`; unterminated strings report exact opening location.

### Definition of Done
String scanning implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-158: implement SQL string literal scanning and escape handling`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-159` (Operators, punctuation & invalid tokens).
````

---

## DAY-159 — Implement Operators, Punctuation, Invalid-Token Diagnostics, and `TokenizeAll`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-159
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Operators, Punctuation, Invalid-Token Diagnostics, and `TokenizeAll`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-158 complete.

### Objective
Complete `Lexer::NextToken()` and `Lexer::TokenizeAll()` for all single- and two-character operators (`=`, `!=`, `<>`, `<`, `<=`, `>`, `>=`, `-`, `*`), punctuation (`(`, `)`, `,`, `;`), max token limit (`kMaxTokensPerQuery`), and clear error diagnostics for invalid symbols (`@`, `$`, lone `!`, double quotes `"` if unquoted identifiers only).

### Concepts & Daily Learning
- **Concept:** Two-character lookahead operators (`<=` vs `<`, `<>` vs `<`, `!=` vs lone `!`) and token-count DoS protection.
- **Why Database Engines Need It:** If `!` isn't followed by `=`, emitting a crisp `"Unexpected character '!' (did you mean '!=')" at line X, col Y` beats silent skipping or generic failure; capping total token count prevents memory spikes on pathological inputs like 30,000 commas.
- **How It Works:** Switch on `Peek()`: for `<`, check next for `=` (`kLessEqual`) or `>` (`kNotEqual`) else `kLess`; for `>`, check `=` (`kGreaterEqual`) else `kGreater`; for `!`, require `=` (`kNotEqual`) else emit diagnostic error!
- **Design Trade-offs:** Supporting both `!=` and `<>` as `TokenType::kNotEqual` matches standard SQL and user expectations.
- **Common Mistakes:** Consuming `<` and failing to distinguish `<=` / `<>` before advancing.
- **Understanding Checkpoint:** How does 1-character lookahead (`Peek()`) inside `ScanOperator` prevent `a<=5` from lexing as `a`, `<`, `=`, `5`?

### Component Context
- **Why It Exists:** Completes the SQL `Lexer`.
- **Module Relationships:** Full token stream consumed by `Parser` (`DAY-165`+).

### Files Expected to Change
- `src/parser/lexer.cpp`
- `include/tinydb/core/config.hpp` (`kMaxTokensPerQuery`)
- `tests/unit/parser/lexer_ops_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Complete `Lexer::NextToken()` and `TokenizeAll()`.
2. Add tests covering compact unspaced expressions (`a<=10`, `b<>5`, `x!=1`, `x=-5`), punctuation, lone `!`, unknown symbol `@`, and token count cap.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Unspaced multi-token SQL lexes cleanly; invalid symbols report exact line/col errors.

### Definition of Done
`Lexer` complete and unit-tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-159: complete SQL Lexer operators, punctuation, and TokenizeAll`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-160` (Comprehensive `Lexer` edge-case suite).
````

---

## DAY-160 — Write Comprehensive `Lexer` Unit Test Suite

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-160
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Write Comprehensive `Lexer` Unit Test Suite
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-159 complete.

### Objective
Write comprehensive end-to-end `Lexer` tests (`tests/unit/parser/lexer_comprehensive_test.cpp`) verifying multi-statement SQL scripts, exact token types + lexemes + line/col coordinates for every v1 SQL statement (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`), binary-byte/null-byte input rejection, and zero ASan/UBSan findings.

### Concepts & Daily Learning
- **Concept:** Adversarial lexer fuzzing / binary-byte rejection (`\0` inside query text).
- **Why Database Engines Need It:** Control characters or embedded NUL bytes inside SQL strings/queries shouldn't truncate lexing silently or trigger `std::isalpha` UB!
- **How It Works:** Exercise complete multi-line queries checking every emitted token's `(type, lexeme, line, column)` tuple, plus embedded `\0` and high-bit (`0x80..0xFF`) invalid bytes.
- **Design Trade-offs:** Testing full `(type, lexeme, line, col)` vectors locks down lexer positions before `Parser` relies on them for error reporting.
- **Common Mistakes:** Ignoring embedded `\0` inside `std::string_view` SQL inputs (C-string APIs stop at `\0`, whereas `std::string_view` sees `\0` as an invalid control byte!).
- **Understanding Checkpoint:** Why should an embedded `\0` byte in unquoted SQL query text produce a syntax error instead of terminating scanning early?

### Component Context
- **Why It Exists:** Finalizes and locks down the SQL `Lexer` layer.
- **Module Relationships:** Gate before AST & `Parser` (`DAY-161`+).

### Files Expected to Change
- `tests/unit/parser/lexer_comprehensive_test.cpp` (new)
- `src/parser/lexer.cpp` (any corner fix uncovered)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `lexer_comprehensive_test.cpp` testing full multi-line SQL scripts, NUL bytes, high bytes (`0xFF`), and position accuracy.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 100% of lexer tests pass under ASan/UBSan with zero warnings or memory errors.

### Definition of Done
Lexer suite verified and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-160: add comprehensive SQL Lexer unit and adversarial tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-161` (AST base classes & ownership).
````

---

## DAY-161 — Design AST Base Hierarchy (`AstNode`, `Expression`, `Statement`) and Ownership Rules

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-161
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Design AST Base Hierarchy (`AstNode`, `Expression`, `Statement`) and Ownership Rules
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-160 complete.

### Objective
Design the Abstract Syntax Tree (AST) base types (`Expression`, `ExprKind`, `Statement`, `StmtKind`) in `include/tinydb/parser/ast.hpp` using strict `std::unique_ptr` ownership, `SourceLocation` tracking on every node, non-copyable/virtual destructor safety, and readable `ToString()` debug printers.

### Concepts & Daily Learning
- **Concept:** Polymorphic AST trees (`std::unique_ptr<Expression>` / `std::unique_ptr<Statement>`), tagged kinds (`ExprKind` / `StmtKind`) for safe downcasting/visiting without RTTI overhead, and clear tree ownership.
- **Why Database Engines Need It:** Raw `Expression*` AST nodes leak memory whenever syntax validation aborts mid-parse; `std::unique_ptr` guarantees automatic zero-leak cleanup on any parse error!
- **How It Works:** `Expression` stores `ExprKind kind` + `SourceLocation loc`; `Statement` stores `StmtKind kind` + `SourceLocation loc`. Provide `virtual ~Expression() = default` and virtual `ToString()`.
- **Design Trade-offs:** Using `enum class StmtKind` + explicit `static_cast` helper (`As<SelectStmt>()`) is faster and simpler than heavy Visitor boilerplate while staying type-safe (`assert(stmt.kind() == StmtKind::kSelect)`).
- **Common Mistakes:** Missing `virtual ~Expression() = default` on polymorphic base classes (classic ASan/UBSan leak & UB!); copying AST nodes accidentally.
- **Understanding Checkpoint:** Why is `std::unique_ptr<Expression>` mandatory for recursive AST expression trees when a parse error occurs halfway through parsing `WHERE a = 1 AND (b = 2 AND ...`?

### Component Context
- **Why It Exists:** Structural backbone connecting `Parser` to `Planner` and `ExecutionEngine`.
- **Module Relationships:** Extended in `DAY-162`–`DAY-164`; produced by `Parser` (`DAY-165`+).

### Files Expected to Change
- `include/tinydb/parser/ast.hpp` (new)
- `tests/unit/parser/ast_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `ExprKind`, `StmtKind`, `Expression`, and `Statement` base types in `include/tinydb/parser/ast.hpp`.
2. Add initial lifecycle/ownership smoke tests in `ast_test.cpp`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Base AST compiles cleanly with `-Werror` and virtual destructors clean up derived objects under ASan.

### Definition of Done
AST base hierarchy created and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-161: design AST Expression and Statement base hierarchy`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-162` (Expression AST nodes).
````

---

## DAY-162 — Implement Expression AST Nodes (`LiteralExpr`, `ColumnRefExpr`, `BinaryOpExpr`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-162
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Expression AST Nodes (`LiteralExpr`, `ColumnRefExpr`, `BinaryOpExpr`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-161 complete.

### Objective
Implement `LiteralExpr` (integer `int64_t`, string, bool `Value`), `ColumnRefExpr` (column identifier), and `BinaryOpExpr` (`BinaryOp`: `kEq`, `kNeq`, `kLt`, `kLte`, `kGt`, `kGte`, `kAnd` with `lhs` and `rhs` `unique_ptr<Expression>`) in `include/tinydb/parser/ast_expr.hpp` and `src/parser/ast.cpp`.

### Concepts & Daily Learning
- **Concept:** Recursive expression trees representing `WHERE` clauses (`(a = 1 AND b != 'x')`).
- **Why Database Engines Need It:** A tree structure cleanly captures arbitrary nesting and precedence so `Planner` can inspect top-level `AND` conjuncts (e.g., pulling out `id = 5` for an `IndexScan`!) and `FilterExecutor` can evaluate any predicate recursively!
- **How It Works:** `BinaryOpExpr` owns `std::unique_ptr<Expression> lhs` and `rhs`. `ToString()` renders parenthesized S-expressions like `(AND (= id 5) (> age 18))` so parser precedence tests are unambiguous and readable!
- **Design Trade-offs:** Printing AST trees as canonical parenthesized strings makes parser unit tests clean (`EXPECT_EQ(expr->ToString(), "(AND (= a 1) (< b 2))")`).
- **Common Mistakes:** Moving `lhs`/`rhs` carelessly leaving `nullptr` children inside `BinaryOpExpr` (check non-null invariants!).
- **Understanding Checkpoint:** Why is canonical parenthesized output like `(AND (= a 1) (> b 2))` ideal for testing operator precedence in unit tests?

### Component Context
- **Why It Exists:** Represents SQL literals, column references, and boolean predicates.
- **Module Relationships:** Used by `SelectStmt`, `UpdateStmt`, `DeleteStmt`, `Planner`, and `FilterExecutor`.

### Files Expected to Change
- `include/tinydb/parser/ast_expr.hpp` (new)
- `src/parser/ast.cpp` (new)
- `tests/unit/parser/ast_test.cpp`
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `LiteralExpr`, `ColumnRefExpr`, `BinaryOpExpr`, and `BinaryOpToString`.
2. Write unit tests building nested expression trees and checking `ToString()` and `Kind()` helpers.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Nested expressions construct, print canonically, and destruct cleanly with zero ASan leaks.

### Definition of Done
Expression AST nodes tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-162: implement LiteralExpr, ColumnRefExpr, and BinaryOpExpr AST nodes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-163` (DDL Statement AST nodes).
````

---

## DAY-163 — Implement DDL Statement AST Nodes (`CreateTableStmt`, `DropTableStmt`, `CreateIndexStmt`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-163
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement DDL Statement AST Nodes (`CreateTableStmt`, `DropTableStmt`, `CreateIndexStmt`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-162 complete.

### Objective
Implement `ColumnDef`, `CreateTableStmt`, `DropTableStmt`, and `CreateIndexStmt` in `include/tinydb/parser/ast_stmt.hpp` and `src/parser/ast.cpp`, capturing table/index names, column types (`INT`, `BIGINT`, `BOOL`, `VARCHAR(n)`), and source locations.

### Concepts & Daily Learning
- **Concept:** AST data structures for DDL statements separating syntax representation from catalog execution.
- **Why Database Engines Need It:** `Parser` shouldn't know whether a table already exists or execute `CatalogManager::CreateTable` during parsing—producing pure DDL AST nodes keeps parsing stateless, unit-testable without disk files, and cleanly separated from `ExecutionEngine`!
- **How It Works:** `ColumnDef { std::string name; DataType type; std::uint32_t max_length; SourceLocation loc; }`. `CreateTableStmt` holds `table_name` and `std::vector<ColumnDef>`. `DropTableStmt` holds `table_name`. `CreateIndexStmt` holds `index_name`, `table_name`, `column_name`.
- **Design Trade-offs:** Storing `ColumnDef` rather than already-validated `Column` allows `Parser` to report syntax errors while `ExecutionEngine`/`Schema::Create` checks semantic column duplication cleanly (or `Parser` can also guard syntax limits!).
- **Common Mistakes:** Binding `CatalogManager&` inside the `Parser`!
- **Understanding Checkpoint:** Why should `Parser` remain completely independent of `CatalogManager` and `DiskManager`?

### Component Context
- **Why It Exists:** Represents parsed DDL requests cleanly.
- **Module Relationships:** Built by `Parser` (`DAY-168`–`DAY-169`), consumed by `Planner` (`Phase 07`).

### Files Expected to Change
- `include/tinydb/parser/ast_stmt.hpp` (new)
- `src/parser/ast.cpp`
- `tests/unit/parser/ast_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ColumnDef`, `CreateTableStmt`, `DropTableStmt`, and `CreateIndexStmt` with `ToString()`.
2. Add AST construction and printer unit tests.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- DDL AST nodes build, print cleanly, and compile with `-Werror`.

### Definition of Done
DDL AST nodes tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-163: implement CreateTable, DropTable, and CreateIndex AST nodes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-164` (DML Statement AST nodes).
````

---

## DAY-164 — Implement DML Statement AST Nodes (`InsertStmt`, `SelectStmt`, `UpdateStmt`, `DeleteStmt`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-164
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement DML Statement AST Nodes (`InsertStmt`, `SelectStmt`, `UpdateStmt`, `DeleteStmt`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-163 complete.

### Objective
Implement `InsertStmt` (table, optional column list, multi-row `VALUES` list), `SelectStmt` (`select_all` or column list, table, optional `where` `unique_ptr<Expression>`), `UpdateAssignment` + `UpdateStmt`, and `DeleteStmt` in `include/tinydb/parser/ast_stmt.hpp` and `src/parser/ast.cpp`.

### Concepts & Daily Learning
- **Concept:** Modeling optional SQL clauses (`[(col1, col2)]`, `[WHERE expr]`, `SELECT *` vs explicit column list) cleanly in C++ ASTs.
- **Why Database Engines Need It:** `INSERT INTO t VALUES (1, 'a'), (2, 'b');` supports multiple tuples per statement; `SELECT *` differs structurally from `SELECT id, name`; optional `WHERE` uses nullable `std::unique_ptr<Expression>` (`nullptr` = unconditional!).
- **How It Works:** `InsertStmt` stores `table_name`, `std::vector<std::string> columns` (empty = all table columns in schema order), `std::vector<std::vector<std::unique_ptr<Expression>>> rows`. `UpdateStmt` holds `std::vector<Assignment { std::string col; std::unique_ptr<Expression> value; }>` + optional `where`.
- **Design Trade-offs:** Representing `INSERT` values as `Expression` (`LiteralExpr`) rather than raw `Value` lets `Parser` report uniform syntax trees while `InsertExecutor` checks types against target `Schema`.
- **Common Mistakes:** Assuming `SELECT` always has a `WHERE` clause (dereferencing null `where` without checking `if (stmt.where)`!).
- **Understanding Checkpoint:** How does `InsertStmt` distinguish `INSERT INTO t VALUES (1, 'a')` from `INSERT INTO t (id, name) VALUES (1, 'a')`?

### Component Context
- **Why It Exists:** Completes all v1 SQL AST statement nodes.
- **Module Relationships:** Output of `Parser` (`DAY-170`–`DAY-173`); input to `Planner` (`Phase 07`).

### Files Expected to Change
- `include/tinydb/parser/ast_stmt.hpp`
- `src/parser/ast.cpp`
- `tests/unit/parser/ast_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `InsertStmt`, `SelectStmt`, `UpdateStmt`, and `DeleteStmt` with canonical `ToString()`.
2. Write AST construction and printer unit tests for all 4 DML nodes.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All DML AST nodes build, print cleanly, and clean up via `unique_ptr` under ASan.

### Definition of Done
DML AST nodes complete and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-164: implement Insert, Select, Update, and Delete AST nodes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-165` (`Parser` skeleton & token helpers).
````

---

## DAY-165 — Implement Recursive-Descent `Parser` Skeleton and Token Stream Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-165
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Recursive-Descent `Parser` Skeleton and Token Stream Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-164 complete.

### Objective
Create `tinydb::Parser` (`include/tinydb/parser/parser.hpp`, `src/parser/parser.cpp`) accepting SQL input or token stream, implementing `Peek()`, `PeekNext()`, `Advance()`, `Match(TokenType)`, `Expect(TokenType, msg)`, `ExpectIdentifier()`, `SyntaxErrorAt(token, msg)`, and recursion depth guard (`DepthGuard` tracking `<= config::kMaxAstDepth`).

### Concepts & Daily Learning
- **Concept:** Recursive-descent parser primitives (`Peek`/`Advance`/`Match`/`Expect`), RAII recursion depth guard, and human-friendly syntax errors.
- **Why Database Engines Need It:** Unbounded recursive expressions like `((((((...` 10,000 deep cause stack overflow crashes in a recursive-descent parser! An RAII `DepthGuard` increments `depth_` on entry, decrements on exit, and fails cleanly at `kMaxAstDepth` (`64`)!
- **How It Works:** `Parser` lexes via `Lexer::TokenizeAll()`, guards token stream non-empty (`[kEof]`), provides `Expect(type, "Expected ')'")` formatting `"Syntax error at line X, column Y: Expected ')', found 'FROM'"`, and stubbed `ParseStatement()` dispatch.
- **Design Trade-offs:** Tokenizing upfront into `std::vector<Token>` makes `Peek(k)` lookahead O(1) and simplifies clean syntax diagnostics at `kEof`.
- **Common Mistakes:** Advancing past `kEof` into out-of-bounds vector indexing; forgetting to decrement `depth_` on early error return (RAII `DepthGuard` prevents this!).
- **Understanding Checkpoint:** Why is an RAII `DepthGuard` strictly better than manually writing `++depth_; ... --depth_;` inside `ParseExpression()`?

### Component Context
- **Why It Exists:** Core recursive-descent parsing engine infrastructure.
- **Module Relationships:** Completed across `DAY-166`–`DAY-175`.

### Files Expected to Change
- `include/tinydb/parser/parser.hpp` (new)
- `src/parser/parser.cpp` (new)
- `tests/unit/parser/parser_core_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Parser` skeleton, `DepthGuard`, `Peek`, `Advance`, `Match`, `Expect`, and error helpers.
2. Add unit tests verifying empty query handling, lexical error propagation, and clean unknown-keyword diagnostic formatting (`"Syntax error at line 1, column 1: ..."`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Empty/whitespace query and unexpected leading tokens produce clean errors with exact line/col positions.

### Definition of Done
`Parser` skeleton tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-165: create recursive-descent Parser skeleton with DepthGuard`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-166` (Expression & precedence parsing).
````

---

## DAY-166 — Implement Expression Parsing, Parentheses, Literals, and Operator Precedence

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-166
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Expression Parsing, Parentheses, Literals, and Operator Precedence
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-165 complete.

### Objective
Implement `ParseExpression()`, `ParseAndExpr()`, `ParseComparisonExpr()`, and `ParsePrimaryExpr()` (literals including signed unary `-` integer literals handling `INT64_MIN` accurately!, string literals, `TRUE`/`FALSE`, column identifiers, and parenthesized `( expr )` guarded by `DepthGuard`).

### Concepts & Daily Learning
- **Concept:** Precedence climbing via recursive-descent hierarchy (`primary > comparison > AND`) and associative left-fold (`(a AND b) AND c`).
- **Why Database Engines Need It:** Incorrect precedence turns `WHERE a = 1 AND b = 2` into garbage or mis-parses `(a = 1 AND b = 2) AND c > 3`; also, `INT64_MIN` (`-9223372036854775808`) must negate cleanly without signed-overflow UB!
- **How It Works:**
  - `ParseExpression()` -> `ParseAndExpr()`.
  - `ParseAndExpr()` -> parses `lhs = ParseComparisonExpr()`, loops `while (Match(kAnd)) { rhs = ParseComparisonExpr(); lhs = BinaryOpExpr(kAnd, lhs, rhs); }`.
  - `ParseComparisonExpr()` -> parses `lhs = ParsePrimaryExpr()`, if next token is `=`, `!=`/`<>`, `<`, `<=`, `>`, `>=`, consumes op and parses `rhs = ParsePrimaryExpr()` (rejecting chained `a = b = c` cleanly!).
  - `ParsePrimaryExpr()` -> enters `DepthGuard`, handles `( expr )`, `- int_literal`, `int_literal` (rejecting `+9223372036854775808` overflow!), `string_literal`, `TRUE`/`FALSE`, and `identifier`!
- **Design Trade-offs:** Disallowing non-parenthesized chained comparisons (`a = b = c`) catches classic SQL mistakes cleanly.
- **Common Mistakes:** Returning `9223372036854775808` positive overflow without checking unary `-`; forgetting `Expect(kRParen)` after `( expr )`.
- **Understanding Checkpoint:** Trace `ParseExpression()` on `a = 1 AND (b > 2 AND c != 'hi')`—what exact AST tree does it build?

### Component Context
- **Why It Exists:** Expression parser shared by `WHERE`, `INSERT VALUES`, and `UPDATE SET`.
- **Module Relationships:** Invoked in `DAY-170`–`DAY-173`.

### Files Expected to Change
- `include/tinydb/parser/parser.hpp`
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_expr_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Expose/test `ParseStandaloneExpression` (or test via parser helper) implementing `ParseExpression` hierarchy.
2. Add tests: literals (`-9223372036854775808`, `9223372036854775807`, `'abc'`, `TRUE`), `+9223372036854775808` rejected, precedence (`a = 1 AND b < 2`), parentheses `((a = 1))`, unclosed `(a = 1`, chained `a = b = c` rejected, and 100-deep `((((...` rejected cleanly by `DepthGuard`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Precedence matches EBNF 100%; `INT64_MIN` parses without UBSan warnings; 100-deep nesting rejects without stack overflow.

### Definition of Done
Expression parser tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-166: implement SQL expression parsing, precedence, and DepthGuard`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-167` (Parsing `CREATE TABLE`).
````

---

## DAY-167 — Implement `CREATE TABLE` Statement Parsing and Type Bounds Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-167
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `CREATE TABLE` Statement Parsing and Type Bounds Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-166 complete.

### Objective
Implement `ParseCreateTable()` supporting `CREATE TABLE <ident> ( <col_def>, ... )`, parsing `INT`, `BIGINT`, `BOOL`, and `VARCHAR(<positive_int>)` (rejecting `VARCHAR(0)`, `VARCHAR(-5)`, `VARCHAR(100000)`, empty column list `()`, and trailing commas `(id INT,)` with exact location errors!).

### Concepts & Daily Learning
- **Concept:** Parsing comma-separated definition lists (`col1 type1, col2 type2`) and parameterized SQL type syntax (`VARCHAR(n)`).
- **Why Database Engines Need It:** `CREATE TABLE t (id INT,)` (trailing comma) or `CREATE TABLE t ()` or `VARCHAR(0)` must yield precise diagnostic messages rather than cryptic "expected identifier" after `)`!
- **How It Works:** Expect `TABLE`, expect `table_name` (`kIdentifier`), expect `(`, reject immediate `)` ("Table must have at least one column"), parse `ColumnDef` loop checking `Match(kComma)`, expect `)`. Cap column count `<= kMaxColumnsPerTable`.
- **Design Trade-offs:** Validating `1 <= n <= kMaxVarcharLength` inside `ParseColumnDef` gives immediate line/col diagnostics at parse time!
- **Common Mistakes:** Allowing trailing commas `(id INT,)` or missing `(` in `VARCHAR 20`.
- **Understanding Checkpoint:** Why does a `do { ... } while (Match(TokenType::kComma));` loop naturally forbid trailing commas like `(a INT,)` when paired with an up-front non-empty check?

### Component Context
- **Why It Exists:** Parses table creation DDL into `CreateTableStmt`.
- **Module Relationships:** Dispatched when `CREATE TABLE` is seen in `ParseStatement()`.

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_ddl_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseCreate()` dispatching to `ParseCreateTable()` (and reserving `CREATE INDEX` for `DAY-168`).
2. Write unit tests covering valid multi-column `CREATE TABLE`, empty column list, trailing comma, `VARCHAR(0)`, `VARCHAR(5000)`, missing `)`, and unsupported types (`FLOAT`, `TEXT`) rejected clearly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid `CREATE TABLE` builds exact `CreateTableStmt`; all syntax/type violations produce crisp line/col diagnostics.

### Definition of Done
`CREATE TABLE` parser implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-167: implement CREATE TABLE SQL parsing and type validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-168` (Parsing `DROP TABLE` and `CREATE INDEX`).
````

---

## DAY-168 — Implement `DROP TABLE` and `CREATE INDEX` Statement Parsing

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-168
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `DROP TABLE` and `CREATE INDEX` Statement Parsing
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-167 complete.

### Objective
Implement `ParseDropTable()` (`DROP TABLE <table_name>`) and `ParseCreateIndex()` (`CREATE INDEX <index_name> ON <table_name> ( <col_name> )`), explicitly rejecting unsupported v1 forms like multi-column indexes `ON t (a, b)` or `DROP INDEX` / `UNIQUE` cleanly!

### Concepts & Daily Learning
- **Concept:** Extending `CREATE` lookahead (`CREATE TABLE` vs `CREATE INDEX`) and explicit unsupported-feature diagnostics for v2 constructs.
- **Why Database Engines Need It:** Users frequently try `CREATE INDEX idx ON t (a, b)` or `CREATE UNIQUE INDEX ...`; explaining clearly `"Multi-column indexes are not supported in TinyDB v1"` is vastly better than `"Expected ')' found ','"`!
- **How It Works:** After `CREATE`, inspect next token: `TABLE` -> `ParseCreateTable`, `INDEX` -> `ParseCreateIndex`, else syntax/unsupported error. In `ParseCreateIndex`, expect `index_name`, `ON`, `table_name`, `(`, single `column_name`; if `,` follows `column_name`, emit explicit `"Multi-column indexes are not supported in v1"`!
- **Design Trade-offs:** Tailored error hints on `CREATE UNIQUE` and `(a, b)` improve developer experience with tiny code cost.
- **Common Mistakes:** Confusing `table_name` and `index_name` field assignments inside `CreateIndexStmt`.
- **Understanding Checkpoint:** How does `ParseCreate` dispatch between `CREATE TABLE` and `CREATE INDEX` in LL(2) without backtracking?

### Component Context
- **Why It Exists:** Completes all v1 DDL statement parsing.
- **Module Relationships:** Dispatches cleanly alongside `CREATE TABLE` (`DAY-167`).

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_ddl_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseDropTable` and `ParseCreateIndex` in `src/parser/parser.cpp`.
2. Add tests verifying `DROP TABLE`, `CREATE INDEX`, multi-column index rejection, `CREATE UNIQUE INDEX` rejection, and missing `ON` / `(` / `)`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both DDL statements produce accurate ASTs; unsupported v2 index syntax yields helpful messages.

### Definition of Done
`DROP TABLE` and `CREATE INDEX` parsers tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-168: implement DROP TABLE and CREATE INDEX SQL parsing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-169` (Parsing `INSERT INTO`).
````

---

## DAY-169 — Implement `INSERT INTO` Statement Parsing (Single & Multi-Row `VALUES`, Optional Columns)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-169
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `INSERT INTO` Statement Parsing (Single & Multi-Row `VALUES`, Optional Columns)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-168 complete.

### Objective
Implement `ParseInsert()` handling `INSERT INTO <table_name> [(col1, ...)] VALUES (val1, ...), (...);`, validating non-empty value tuples, optional column list matching non-empty identifiers, consistent row arities within a multi-row `INSERT` (optional or checked cleanly!), and rejecting non-literal expressions if v1 `VALUES` accepts literals/signed literals cleanly!

### Concepts & Daily Learning
- **Concept:** Parsing multi-row tuples (`VALUES (...), (...)`) and optional parenthesized column projection lists (`[(col1, col2)]`).
- **Why Database Engines Need It:** Both `INSERT INTO users VALUES (1, 'a')` and `INSERT INTO users (id, name) VALUES (1, 'a'), (2, 'b')` are core SQL idioms; distinguishing optional column lists via `Peek().type == TokenType::kLParen` before `VALUES` makes parsing unambiguous!
- **How It Works:** Expect `INSERT`, `INTO`, `table_name`. If `Match(kLParen)`, parse comma-separated `columns` until `)`. Expect `VALUES`. Loop comma-separated tuple groups `( val_1, ... )` checking each tuple is non-empty (`()`) and capping max rows/values (`config::kMaxInsertRowsPerStmt`).
- **Design Trade-offs:** Checking value tuple non-emptiness and capping row count protects against malformed memory spikes.
- **Common Mistakes:** Failing to reject `INSERT INTO t () VALUES (1)` or `INSERT INTO t VALUES ()` or `INSERT INTO t VALUES (1),`.
- **Understanding Checkpoint:** Why can `ParseInsert` distinguish `INSERT INTO t (a, b) VALUES ...` from `INSERT INTO t VALUES ...` with just 1 token lookahead after `table_name`?

### Component Context
- **Why It Exists:** Parses row insertions into `InsertStmt`.
- **Module Relationships:** Paired with `InsertExecutor` (Phase 07).

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_dml_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseInsert()` in `src/parser/parser.cpp`.
2. Write unit tests verifying: single row, multi-row, explicit column list, negative integer literal in `VALUES`, empty `VALUES ()` rejected, and missing `INTO`/`VALUES` rejected.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid `INSERT` statements map accurately to `InsertStmt`; malformed lists produce clear syntax errors.

### Definition of Done
`INSERT INTO` parser tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-169: implement INSERT INTO SQL statement parsing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-170` (Parsing `SELECT`).
````

---

## DAY-170 — Implement `SELECT` Statement Parsing (`*` vs Column List, `FROM`, Optional `WHERE`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-170
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `SELECT` Statement Parsing (`*` vs Column List, `FROM`, Optional `WHERE`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-169 complete.

### Objective
Implement `ParseSelect()` supporting `SELECT * FROM <table> [WHERE <expr>]` and `SELECT <col1>, <col2>, ... FROM <table> [WHERE <expr>]`, cleanly rejecting unsupported v2 SQL clauses (`JOIN`, `ORDER BY`, `GROUP BY`, `LIMIT`, subqueries, `SELECT *, a`) with clear diagnostics!

### Concepts & Daily Learning
- **Concept:** Parsing projection targets (`*` vs identifier lists) and optional trailing `WHERE` predicates.
- **Why Database Engines Need It:** `SELECT` is the most queried SQL statement; catching unsupported clauses like `JOIN` or `ORDER BY` right after `table_name` with explicit `"JOIN is not supported in TinyDB v1"` prevents confusing `"Expected ';' found 'JOIN'"` messages!
- **How It Works:** Expect `SELECT`. If `Match(kStar)`, set `select_all = true`; else parse comma-separated column identifiers. Expect `FROM`, `table_name`. If `Match(kWhere)`, call `ParseExpression()`. Verify next token isn't an unsupported keyword (`JOIN`/`ORDER`/`GROUP`/`HAVING`/`LIMIT` -> `StatusCode::kUnsupported`!).
- **Design Trade-offs:** Returning `StatusCode::kUnsupported` (distinct from `kSyntaxError`) for recognized SQL v2 keywords gives users crystal-clear feedback!
- **Common Mistakes:** Allowing `SELECT *` followed by `, col2` without failing; crashing when `WHERE` has no predicate (`SELECT * FROM t WHERE;`).
- **Understanding Checkpoint:** Why is detecting `JOIN`, `ORDER`, `GROUP`, or `LIMIT` after `FROM <table>` and returning `StatusCode::kUnsupported` better UX than a generic syntax error?

### Component Context
- **Why It Exists:** Parses queries into `SelectStmt`.
- **Module Relationships:** Consumed by `Planner` -> `SeqScan`/`IndexScan` + `Filter` + `Projection` executors.

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_dml_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseSelect()` in `src/parser/parser.cpp`.
2. Add tests covering `SELECT *`, `SELECT a, b`, `SELECT` with nested `WHERE` (`AND` + parens), empty `SELECT FROM`, `SELECT *, a` rejection, and `JOIN`/`ORDER BY` explicit `kUnsupported` diagnostics!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid `SELECT` forms build exact `SelectStmt`; malformed and unsupported forms return exact error codes and positions.

### Definition of Done
`SELECT` parser implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-170: implement SELECT SQL statement parsing and v2 rejection hints`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-171` (Parsing `UPDATE`).
````

---

## DAY-171 — Implement `UPDATE` Statement Parsing (`SET col = expr`, Optional `WHERE`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-171
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `UPDATE` Statement Parsing (`SET col = expr`, Optional `WHERE`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-170 complete.

### Objective
Implement `ParseUpdate()` handling `UPDATE <table> SET col1 = val1 [, col2 = val2 ...] [WHERE <expr>]`, validating non-empty assignment list, `=` operator (rejecting `!=` or `>` inside `SET`!), and optional `WHERE` clause.

### Concepts & Daily Learning
- **Concept:** Context-sensitive role of `=` (assignment inside `SET col = val` vs comparison inside `WHERE col = val`).
- **Why Database Engines Need It:** Notice `TokenType::kEqual` means **assignment** in `SET` and **equality test** in `WHERE`; parsing `SET` explicitly as `identifier '=' primary_literal_or_expr` avoids mistakenly accepting `SET a > 5`!
- **How It Works:** Expect `UPDATE`, `table_name`, `SET`. Loop parsing `col_name`, `Expect(kEqual, "Expected '=' in SET assignment")`, `val = ParsePrimaryExpr()` (or literal), while `Match(kComma)`. Optionally parse `WHERE` expression.
- **Design Trade-offs:** Reusing `ParsePrimaryExpr()` in `SET` handles literals (`42`, `-5`, `'str'`, `TRUE`) cleanly and allows future expression expansion.
- **Common Mistakes:** Calling `ParseComparisonExpr()` inside `SET` (which would parse `a = 5` as a boolean comparison instead of an assignment target!).
- **Understanding Checkpoint:** Why must `ParseUpdate` parse `col`, `=`, `value` explicitly rather than delegating the `SET` item to `ParseComparisonExpr()`?

### Component Context
- **Why It Exists:** Parses table modification statements into `UpdateStmt`.
- **Module Relationships:** Paired with `UpdateExecutor` (Phase 07).

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_dml_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseUpdate()` in `src/parser/parser.cpp`.
2. Add tests covering single-column `UPDATE`, multi-column `UPDATE` with `WHERE`, unconditional `UPDATE` (no `WHERE`), `SET a > 1` rejected, and empty `SET` rejected!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid `UPDATE` statements produce accurate `UpdateStmt`; invalid `SET` operators/missing `=` fail cleanly.

### Definition of Done
`UPDATE` parser implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-171: implement UPDATE SQL statement parsing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-172` (Parsing `DELETE`).
````

---

## DAY-172 — Implement `DELETE` Statement Parsing (`DELETE FROM <table> [WHERE <expr>]`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-172
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement `DELETE` Statement Parsing (`DELETE FROM <table> [WHERE <expr>]`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-171 complete.

### Objective
Implement `ParseDelete()` handling `DELETE FROM <table_name> [WHERE <expr>]` and wire top-level `ParseStatement()` dispatch for all 7 v1 statements (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`).

### Concepts & Daily Learning
- **Concept:** Complete single-statement dispatch (`ParseStatement()`) across DDL and DML.
- **Why Database Engines Need It:** `DELETE FROM users` (all rows) vs `DELETE FROM users WHERE id = 1` both share the exact same AST (`where == nullptr` vs non-null); enforcing `FROM` catches common typos like `DELETE users` cleanly!
- **How It Works:** Expect `DELETE`, `FROM`, `table_name`. If `Match(kWhere)`, parse `where = ParseExpression()`. Check no trailing garbage tokens remain before `;` / `EOF`!
- **Design Trade-offs:** Enforcing `FROM` strictly follows standard SQL rather than sloppy non-standard dialects.
- **Common Mistakes:** Allowing `DELETE users` without `FROM`, or silently ignoring trailing tokens after a statement (`DELETE FROM users extra_junk;`).
- **Understanding Checkpoint:** Why MUST `ParseStatement` require `;` or `kEof` after a statement instead of returning early and silently ignoring trailing tokens like `DELETE FROM users foo`?

### Component Context
- **Why It Exists:** Completes all single-statement parsing in Phase 06.
- **Module Relationships:** Dispatches all 7 AST statement types.

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_dml_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ParseDelete()` and `ParseStatement()` full switch dispatch.
2. Add tests covering unconditional `DELETE`, filtered `DELETE`, missing `FROM`, and trailing garbage tokens after statement.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 7 SQL statement types parse via `Parser::ParseOne` / `ParseStatement`; trailing tokens fail cleanly.

### Definition of Done
`DELETE` and top-level dispatch tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-172: implement DELETE SQL parsing and full statement dispatch`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-173` (Multi-statement parsing & semicolons).
````

---

## DAY-173 — Implement Multi-Statement Parsing and Semicolon Delimiter Rules

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-173
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Multi-Statement Parsing and Semicolon Delimiter Rules
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-172 complete.

### Objective
Implement `Result<std::vector<std::unique_ptr<Statement>>> Parser::ParseAll(std::string_view sql)` supporting multiple semicolon-separated statements (`stmt1; stmt2; stmt3;`), empty statement skipping (`;;` or trailing `;`), and rejecting missing semicolons between consecutive statements (`SELECT * FROM a SELECT * FROM b`).

### Concepts & Daily Learning
- **Concept:** Statement boundary framing (`;` separator vs terminator) and empty-statement handling in batch SQL scripts.
- **Why Database Engines Need It:** Both interactive CLI multi-line input and `.sql` batch scripts pass multiple statements together; forgetting `;` between statements must fail cleanly rather than fusing two statements!
- **How It Works:** While `!IsAtEnd()`: consume leading `;` (empty statements are harmless no-ops, or document behavior clearly!), if `IsAtEnd()` break; parse 1 statement; if `!IsAtEnd()`, require `Expect(TokenType::kSemicolon, "Expected ';' between statements")` (or allow omitting `;` ONLY before `kEof` on the last statement!). Cap `statements.size() <= config::kMaxStatementsPerBatch`.
- **Design Trade-offs:** Requiring `;` between statements while making final trailing `;` optional handles both single CLI queries (`SELECT * FROM t`) and full SQL scripts (`CREATE ...; INSERT ...; SELECT ...;`) gracefully!
- **Common Mistakes:** Entering an infinite loop on consecutive `;;` semicolons (`while (!IsAtEnd())` without advancing over `;`!).
- **Understanding Checkpoint:** What happens if someone submits `SELECT * FROM a; ; SELECT * FROM b;`—how should `ParseAll` advance without looping infinitely or crashing?

### Component Context
- **Why It Exists:** Enables multi-statement scripts in `ExecutionEngine` and CLI.
- **Module Relationships:** Primary public entry point of `Parser`.

### Files Expected to Change
- `include/tinydb/parser/parser.hpp`
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_multi_stmt_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Parser::ParseAll(std::string_view)`.
2. Write unit tests verifying: 0 statements (`""`, `";;;"`), 1 statement without trailing `;`, 5 mixed DDL/DML statements with `;`, extra `;;`, and missing `;` between two statements rejected cleanly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-statement scripts parse cleanly; missing semicolons between statements fail at exact line/column.

### Definition of Done
Multi-statement `ParseAll` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-173: implement multi-statement ParseAll with semicolon framing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-174` (Input resource limits hardening).
````

---

## DAY-174 — Enforce Parser Resource Limits (Max Query Bytes, Tokens, Depth, Columns, Statements)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-174
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Enforce Parser Resource Limits (Max Query Bytes, Tokens, Depth, Columns, Statements)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-173 complete.

### Objective
Audit and test all security/DoS resource limits across `Lexer` and `Parser` (`kMaxSqlQueryBytes = 65536`, `kMaxTokensPerQuery = 4096`, `kMaxAstDepth = 64`, `kMaxColumnsPerTable = 64`, `kMaxInsertRowsPerStmt = 1024`, `kMaxStatementsPerBatch = 256`), ensuring every limit triggers cleanly with `StatusCode::kResourceExhausted` / `kInvalidArgument` before unbounded RAM/stack growth.

### Concepts & Daily Learning
- **Concept:** Denial-of-Service (DoS) mitigation in language parsers via bounded resource quotas.
- **Why Database Engines Need It:** Without explicit limits, crafted SQL inputs (`10,000` nested parentheses, `50,000` `AND 1=1` clauses, or `100,000` columns) crash the database with stack overflow (`SIGSEGV`) or OOM!
- **How It Works:** Verify BOTH `DepthGuard` (for recursive parentheses) AND `kMaxAndTerms` / token bounds (for flat `1=1 AND 1=1 AND ...` chains—note that a left-iterative `while (Match(kAnd))` loop doesn't blow the parser stack, yet building a left-deep `BinaryOpExpr` tree of depth 5,000 WOULD blow the recursive `~Expression()` destructor stack or `Evaluate()` stack on teardown unless capped by `kMaxAstDepth`!).
- **Design Trade-offs:** Enforcing `kMaxAndTerms <= kMaxAstDepth` (64) bounds BOTH parsing depth AND recursive AST destruction/evaluation depth without rewriting destructors iteratively.
- **Common Mistakes:** Only bounding parentheses recursion while letting a flat `while (Match(kAnd))` loop build a depth-4,000 `BinaryOpExpr` chain that crashes inside `~BinaryOpExpr()`!
- **Understanding Checkpoint:** Why can an iterative `while (Match(kAnd))` parser loop STILL cause a stack overflow during AST *destruction* or *printing*, and how do we prevent it cleanly?

### Component Context
- **Why It Exists:** Hardens the untrusted SQL entry boundary against pathological inputs.
- **Module Relationships:** Protects `Lexer`, `Parser`, and `AST` destruction.

### Files Expected to Change
- `include/tinydb/core/config.hpp`
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_limits_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Enforce `kMaxAndClauses <= kMaxAstDepth` (or cap expression tree height explicitly!) alongside query size, token count, column count, row count, and batch count.
2. Add tests pushing right past every single limit (`65537` bytes, `65` nested parens, `65` chained `AND`s, `65` columns, `1025` insert rows, `257` batch statements) under ASan/UBSan!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every resource cap rejects cleanly with zero stack overflow or leak under ASan.

### Definition of Done
Resource limits enforced and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-174: enforce SQL parser resource and AST depth bounds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-175` (Explicit unsupported v2 SQL rejection).
````

---

## DAY-175 — Add Explicit Rejection & Diagnostics for Unsupported v1 SQL Constructs

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-175
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Add Explicit Rejection & Diagnostics for Unsupported v1 SQL Constructs
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-174 complete.

### Objective
Write explicit detection and clear `StatusCode::kUnsupported` messages for excluded v2/unsupported SQL constructs (`NULL`, `IS NULL`, `FLOAT`, `DOUBLE`, `DECIMAL`, `FOREIGN KEY`, `PRIMARY KEY`, `JOIN`, subqueries `(SELECT ...)`, `OR`, `NOT`, `LIKE`, `ORDER BY`, `GROUP BY`, `LIMIT`, `ALTER TABLE`) in `src/parser/parser.cpp` and verify in `tests/unit/parser/parser_unsupported_test.cpp`.

### Concepts & Daily Learning
- **Concept:** Helpful unsupported-feature diagnostics vs opaque syntax failures.
- **Why Database Engines Need It:** Users experimenting with `tinydb` will naturally type `PRIMARY KEY`, `NULL`, `OR`, `JOIN`, or `LIMIT 10`; telling them clearly `"Feature 'JOIN' is excluded in TinyDB v1"` aligns expectations with `docs/sql-spec.md`.
- **How It Works:** Recognize key unsupported keywords in `Lexer` or contextual identifier checks inside `Parser` (`NULL`, `OR`, `JOIN`, `PRIMARY`, `FOREIGN`, `REFERENCES`, `FLOAT`, `DECIMAL`, `ORDER`, `GROUP`, `LIMIT`, `ALTER`) so they produce clear `StatusCode::kUnsupported` errors with line/column coordinates!
- **Design Trade-offs:** Checking unsupported keywords only in statement/type/predicate positions still lets users name a column `limit_val` or `order_id` without unnecessary keyword reservation conflicts (keeping un-cluttered identifier spaces while still rejecting `LIMIT 5` or `ORDER BY`!).
- **Common Mistakes:** Reserving `order` or `key` globally so `CREATE TABLE orders (order INT)` breaks when only `ORDER BY` or `PRIMARY KEY` needed rejection!
- **Understanding Checkpoint:** Why is checking `ORDER` followed by `BY` (or `NULL` as a literal/type modifier) cleaner than banning `order` as a column name?

### Component Context
- **Why It Exists:** Enforces exact v1 SQL boundary contract cleanly.
- **Module Relationships:** Protects `Parser` and `docs/sql-spec.md` scope boundaries.

### Files Expected to Change
- `src/parser/parser.cpp`
- `tests/unit/parser/parser_unsupported_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add explicit `kUnsupported` checks for excluded v1 constructs (`NULL`, `FLOAT`/`DECIMAL`, `PRIMARY KEY`/`FOREIGN KEY`, `JOIN`, subqueries, `OR`, `ORDER BY`/`GROUP BY`, `LIMIT`).
2. Verify all negative test cases return `StatusCode::kUnsupported` with line/col coordinates.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every excluded feature listed in Section 6 rejects explicitly with `kUnsupported` and clear message.

### Definition of Done
Unsupported SQL guards tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-175: add clear diagnostics for unsupported v1 SQL constructs`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-176` (Comprehensive Parser unit tests).
````

---

## DAY-176 — Write Comprehensive Valid SQL Parser Unit Test Suite

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-176
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Write Comprehensive Valid SQL Parser Unit Test Suite
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-175 complete.

### Objective
Write an exhaustive positive parser test suite (`tests/unit/parser/parser_comprehensive_test.cpp`) verifying exact AST node fields and canonical `ToString()` trees across every combination of v1 SQL syntax (case variations, multi-line formatting, inline comments, string escapes, extreme integers, nested `WHERE` clauses, multi-row inserts, and batch scripts).

### Concepts & Daily Learning
- **Concept:** Grammar coverage completeness testing (testing every production rule of the EBNF specification).
- **Why Database Engines Need It:** Guarantees no EBNF production or operator combination (`!=` vs `<>`, `<=`, `>=`, `SELECT *` with `WHERE`, multi-assignment `UPDATE`) was overlooked or mis-wired before `ExecutionEngine` depends on the AST.
- **How It Works:** Walk systematically through every rule in `docs/sql-spec.md`, asserting exact typed downcast fields (`CreateTableStmt`, `DropTableStmt`, `CreateIndexStmt`, `InsertStmt`, `SelectStmt`, `UpdateStmt`, `DeleteStmt`) and exact `Where` AST structures.
- **Design Trade-offs:** Asserting BOTH structural fields AND `ToString()` catches both structural and ordering errors.
- **Common Mistakes:** Only checking `ParseAll(sql).ok()` without inspecting the actual produced AST fields!
- **Understanding Checkpoint:** Why is checking `ASSERT_TRUE(ParseAll(...).ok())` completely insufficient without inspecting the resulting AST tree fields?

### Component Context
- **Why It Exists:** Locks down correctness of all valid SQL parsing.
- **Module Relationships:** Verifies the entire Phase 06 `Parser` output contract.

### Files Expected to Change
- `tests/unit/parser/parser_comprehensive_test.cpp` (new)
- `src/parser/parser.cpp` (any corner bug fixed)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write comprehensive AST verification tests covering all 7 statement types and all EBNF rules.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 100% of EBNF productions covered and verified with exact AST assertions.

### Definition of Done
Comprehensive valid-SQL tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-176: add comprehensive valid SQL AST parser test suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-177` (Parser error-recovery & fuzz testing).
````

---

## DAY-177 — Implement Malformed SQL Boundary and Fuzz Testing Suite

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-177
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Implement Malformed SQL Boundary and Fuzz Testing Suite
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-176 complete.

### Objective
Write structured mutation and random fuzz tests (`tests/stress/parser_fuzz_test.cpp`) throwing 5,000+ truncated, token-deleted, token-swapped, random ASCII, and binary byte strings at `Lexer` + `Parser::ParseAll` under ASan+UBSan, verifying zero crashes, zero hangs, zero memory leaks, and clean error `Status` on malformed inputs!

### Concepts & Daily Learning
- **Concept:** Grammar-aware mutation fuzzing + raw byte fuzzing for parser security hardening.
- **Why Database Engines Need It:** Parsers directly ingest untrusted user strings; truncated tokens, missing closing parens, random byte sequences, and token swaps commonly trigger null dereferences or infinite loops (`while (!IsAtEnd())` forgetting `Advance()`)!
- **How It Works:** Seed valid SQL queries -> generate: (1) all prefix truncations, (2) token-deletion mutations, (3) random token-stream shuffles, (4) random ASCII/binary strings up to 4KB. Assert `Parser::ParseAll` ALWAYS returns normally (either `ok()` or explicit error `Status`—never crashes/hangs/leaks!).
- **Design Trade-offs:** Deterministic seeded fuzzing inside CTest gives continuous ASan/UBSan protection in CI without needing libFuzzer linker flags on macOS Apple Clang!
- **Common Mistakes:** Only fuzzing completely random bytes (which fail at token 1 in the lexer!) instead of also mutating *valid* token streams so deep parser rules get fuzzed!
- **Understanding Checkpoint:** Why do prefix-truncation and token-deletion mutations of *valid* SQL queries expose deeper parser bugs than purely random bytes?

### Component Context
- **Why It Exists:** Proves `Lexer` and `Parser` cannot crash or hang on arbitrary untrusted input.
- **Module Relationships:** Exercises `Lexer`, `Parser`, and `AST` cleanup.

### Files Expected to Change
- `tests/stress/parser_fuzz_test.cpp` (new)
- `src/parser/lexer.cpp`, `src/parser/parser.cpp` (fix any crash/infinite-loop found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `parser_fuzz_test.cpp` (truncations, token-mutators, random bytes, 5,000+ iterations).
2. Run under Debug + ASan/UBSan (`-Werror`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 5,000+ fuzzed inputs run cleanly in `< 2s` with zero crashes, zero hangs, and zero ASan/UBSan/LSan errors.

### Definition of Done
Parser fuzz suite passing under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-177: add structured mutation and random SQL parser fuzz suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-178` (Finalize `docs/sql-spec.md` & sign off Phase 06).
````

---

## DAY-178 — Finalize `docs/sql-spec.md` and Sign Off Phase 06

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-178
- **Phase:** Phase 06 — SQL Lexer and Parser
- **Session Title:** Finalize `docs/sql-spec.md` and Sign Off Phase 06
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-153 through DAY-177 complete.

### Objective
Audit `docs/sql-spec.md` against the implemented `Lexer`, `Parser`, AST, and resource limits, run all tests across Debug/ASan+UBSan/Release + `clang-format`, and formally sign off Phase 06 (`138.0 / 220.0` hours).

### Concepts & Daily Learning
- **Concept:** Frontend specification alignment before building the query execution engine.
- **Why Database Engines Need It:** Clear AST and grammar contracts ensure `Planner` and `ExecutionEngine` handle every parsed node identically to `docs/sql-spec.md`.
- **How It Works:** Verify every supported token, escape sequence, resource limit, and rejected construct in `docs/sql-spec.md` matches the parser code; run the full test matrix; update `PROGRESS.md` and `NEXT_SESSION.md` (`DAY-179`).
- **Design Trade-offs:** Clean sign-off keeps `src/parser/` completely decoupled from `src/execution/`.
- **Common Mistakes:** Leaving undocumented string escape behavior or undocumented limits out of `docs/sql-spec.md`.
- **Understanding Checkpoint:** Explain the full data flow from raw SQL string -> `Lexer` tokens -> `Parser` AST -> resource guards, and why the parser avoids holding `CatalogManager`.

### Component Context
- **Why It Exists:** Completes Phase 06 (`20.0` hours).
- **Module Relationships:** Hands off clean ASTs to Phase 07 (`Query Engine & CLI`).

### Files Expected to Change
- `docs/sql-spec.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit and update `docs/sql-spec.md`.
2. Run Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format --dry-run --Werror`.
3. Update `PROGRESS.md` (`138.0 / 220.0` hours, Phase 06 complete) and `NEXT_SESSION.md` -> `DAY-179`.

### Testing Instructions
- Full Debug, ASan+UBSan, Release `ctest --output-on-failure` + formatting check.

### Acceptance Criteria
- 100% of Phase 00–06 tests pass across all configs with zero warnings.

### Definition of Done
Phase 06 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-178: finalize SQL spec documentation and sign off Phase 06`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 07 (`DAY-179` — Pull-based Volcano Executor interface).
````
