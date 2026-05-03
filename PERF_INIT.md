# Lemmatizer Initialization — Performance Analysis

## Context

When `Lemmatiseur` is constructed it triggers `LemCore::LemCore(resDir)`, which runs the
full initialization chain:

```
Lemmatiseur::Lemmatiseur()
  └─ LemCore::LemCore(resDir)
       ├─ ajAssims()
       ├─ ajAbrev()
       ├─ ajContractions()
       ├─ lisMorphos(lang)
       ├─ lisModeles()
       ├─ lisLexique()
       ├─ lisTags(false)
       └─ lisTraductions(true, false)
```

The Latin lexicon alone has ~24,176 lemmas. Across all languages the data files total
~184k lines.

---

## Root Cause: `replaceAll` is Everywhere

`replaceAll` (defined in `src/string_utils.h:122`) does a full string scan + in-place
`std::string::replace` per occurrence — meaning a full copy on every call.

It is the primitive used by every hot string-processing function:

| Function | `replaceAll` calls per invocation | Primary callers |
|---|---|---|
| `Ch::atone()` | 18–23 | `Lemme` ctor (×2), `Radical` ctor (×1 via `communes`) |
| `Ch::communes()` | 18–23 (via `atone`) | `Radical` ctor |
| `Ch::deramise()` | 5 | multimap inserts, translation lookups |

Rough total at init: **~2.1 million `replaceAll` calls**.

---

## Bottlenecks Ranked

### 1. `Ch::atone()` — 23 sequential full-string scans per word

**File**: `src/ch.cpp:151`
**Called**: ~70,000 times during init (×2 in `Lemme` ctor + ×1 in `Radical` ctor via `communes`)

`atone()` strips diacritics (macron, breve) by running 23 independent `replaceAll` passes
on the same string:

```cpp
s = replaceAll(s, "\xC4\x81", "a");   // ā  — pass 1
s = replaceAll(s, "\xC4\x83", "a");   // ă  — pass 2
// ... 21 more passes
```

Most passes find nothing (a typical Latin word has 0–1 diacritics), yet every pass scans
the full string. This is O(n × 23) when O(n × 1) is achievable with a single-pass
byte decoder.

**Fix**: One pass over the bytes; dispatch on the first byte of each UTF-8 sequence and
emit the ASCII replacement directly. All known diacritics are 2-byte sequences with a
small set of first bytes (`0xC4`, `0xC5`, `0xC8`, `0xD1`).

---

### 2. `std::multimap` insertions with `deramise()` on the key

**File**: `src/lemCore.cpp:329, 359, 383`
**Estimated insertions**: ~41,000 (36k radicals + 5k desinences)

Each insertion:
1. Calls `Ch::deramise()` to compute the key — 5 more `replaceAll` calls
2. Triggers O(log n) comparisons of UTF-8 `std::string` keys during tree rebalancing

An `std::unordered_map` with a pre-normalized key would drop this from O(log n) to O(1)
amortized per insert and eliminate repeated `deramise()` computation.

---

### 3. Tags file: 115k lines, all loaded eagerly

**File**: `src/lemCore.cpp:108` (`lisTags`)
**Data file**: `bin/data/tags.la` — 115,109 lines

Each line is split, converted to int, and inserted into two `std::map<std::string, int>`
with string keys. The sheer volume makes this a major I/O + allocation cost even though
the per-line work is cheap.

**Fix**: Use `std::unordered_map` for `_tagOcc` / `_tagTot`. Consider whether tags need
to be fully loaded before the first lemmatization or can be loaded lazily / on a
background thread.

---

### 4. Translation files: ~145k lemma lookups at startup

**File**: `src/lemCore.cpp:841` (`lisTraductions`)

For each of 6+ language files, every line requires:
- `Ch::deramise()` to normalize the key (5 `replaceAll` calls)
- A `_lemmes` map lookup — O(log 24176) ≈ 15 string comparisons

At ~24k lemmas per language file × 6 languages = **~145k deramise + lookup pairs**,
even before the translations are stored.

**Fix**: Lazy-load — only load the language(s) the caller actually needs.
This is the easiest high-impact win: zero cost for unused languages.

---

### 5. `std::regex` in model parsing

**File**: `src/modele.cpp:169`

Variable substitution in model lines uses `std::regex_search` / `std::regex_replace` in
a loop. `std::regex` compilation is expensive; combined with ~6k iterations this is
measurably slow compared to a plain string scanner.

**Fix**: Replace with simple `std::string::find` + `replaceAll` for the `$var` pattern —
the substitution format is simple enough to not need a regex.

---

## Summary Table

| # | Bottleneck | Location | Est. ops | Fix |
|---|---|---|---|---|
| 1 | `atone()` — 23 `replaceAll` per word | `ch.cpp:151` | ~1.6M scans | Single-pass UTF-8 decoder |
| 2 | Multimap inserts + `deramise()` per key | `lemCore.cpp:329` | ~205k ops | `unordered_map` + pre-normalize keys |
| 3 | Tags: 115k lines into `std::map` | `lemCore.cpp:108` | ~230k inserts | `unordered_map`, lazy/async load |
| 4 | Translation lookups for all languages | `lemCore.cpp:841` | ~145k lookups | Lazy-load per language |
| 5 | `std::regex` in model variable substitution | `modele.cpp:169` | ~6k regex ops | Plain string search |

**Highest-leverage single fix**: rewrite `atone()` as a single-pass decoder — it is the
root of bottleneck #1 and also benefits `communes()` and `deramise()` which call it
internally.

**Easiest win**: lazy-load translations (#4) — pure algorithmic savings, no data-structure
changes needed.
