# Lemmatizer Init Performance — Design Spec

**Date:** 2026-05-02
**Branch:** no-qt-improve-loading-perf
**Scope:** Two targeted fixes to reduce startup time, each covered by unit tests written first.

---

## Problem

Lemmatizer initialization is noticeably slow for users. Two root causes:

1. `Ch::atone()` makes 23 sequential `replaceAll` passes over every string, called ~70,000 times during init — ~1.6M full string scans total.
2. `LemCore::lisTraductions()` loads all 10 translation language files unconditionally, regardless of the `_cible` setting — ~145k unnecessary lemma lookups for a single-language user.

---

## Fix 1 — Single-pass `Ch::atone()`

### Current behavior
`atone()` strips diacritics (macron, breve) by running 23 independent `replaceAll` calls on the same string, one per known diacritic UTF-8 sequence. Most passes find nothing on a typical Latin word, yet each scans the full string.

### New behavior
One pass over the raw UTF-8 bytes:
- If the current byte begins a known 2-byte diacritic sequence (first byte in `{0xC4, 0xC5, 0xC8, 0xD1}`), look up the second byte in a small static table and emit the ASCII replacement.
- If the current 2 bytes are the combining breve `0xCC 0x86`, consume both and emit nothing.
- Otherwise copy the byte as-is.

Result is built into a `std::string` in one pass with a single allocation. Same function signature, same semantics — all callers unchanged.

### Unit tests (`tests/test_ch.cpp`)
Each case is independent and non-repetitive:

| Test | Input | `bdc` | Expected output |
|---|---|---|---|
| Each lowercase diacritic | `"ā"`, `"ă"`, `"ē"`, `"ĕ"`, `"ī"`, `"ĭ"`, `"ō"`, `"ŏ"`, `"ū"`, `"ŭ"`, `"ȳ"`, `"ў"` | false | `"a"`, `"a"`, `"e"`, `"e"`, `"i"`, `"i"`, `"o"`, `"o"`, `"u"`, `"u"`, `"y"`, `"y"` |
| Each uppercase diacritic | `"Ā"`, `"Ē"`, `"Ī"`, `"Ō"`, `"Ū"`, `"Ȳ"` | false | `"A"`, `"E"`, `"I"`, `"O"`, `"U"`, `"Y"` |
| Uppercase kept with `bdc=true` | `"Ā"` | true | `"Ā"` (unchanged) |
| Combining breve stripped | `"ā\xCC\x86"` (ā + U+0306) | false | `"a"` |
| Plain ASCII unchanged | `"amicus"` | false | `"amicus"` |
| Empty string | `""` | false | `""` |
| Mixed realistic word | `"āmīcus"` | false | `"amicus"` |
| Special chars | `"ı"` (U+0131), `"ụ"` (U+1EE5) | false | `"i"`, `"u"` |

---

## Fix 2 — Language-filtered `lisTraductions()`

### Current behavior
`lisTraductions()` enumerates all `lemmes.XX` files in the resource directory and loads every one unconditionally. `_cible` is never consulted.

### New behavior
Before reading each file, check whether its 2-letter suffix should be loaded:
- Always load `fr` and `en` — they are the hardcoded fallbacks in `Lemme::traduction()`.
- Load any other language whose 2-letter code appears in `_cible`.
- Skip all others.

No interface changes. `_cible` defaults to `"fr en"`, so existing behavior is fully preserved when no cible is set.

### Fallback chain (unchanged)
`Lemme::traduction(l)` already implements:
1. Return translation in `l` if available
2. Fall back to `fr`
3. Fall back to `en`
4. Return `"non traduit / Translation not available."`

### Unit tests (`tests/test_lemcore.cpp`)
These tests use the real resource directory with a test `LemCore` instance.

| Test | `_cible` | Expected loaded languages | Notes |
|---|---|---|---|
| Single non-fallback language | `"de"` | `fr`, `en`, `de` | `de` is in cible |
| Default cible | `"fr en"` | `fr`, `en` | No extras needed |
| Single fallback language | `"fr"` | `fr`, `en` | `en` always loaded |
| Unlisted language absent | `"fr"` | no `es`, `it`, etc. | 8 files skipped |
| Fallback works at runtime | `"fr"`, request `"it"` | — | Returns `fr` translation |
| Fallback chain: no fr | `"en"` only loaded | request `"it"` | Returns `en` translation |

---

## Execution Order

1. Write `tests/test_ch.cpp` — `atone()` tests
2. Confirm tests compile and fail (TDD gate)
3. Rewrite `Ch::atone()` in `src/ch.cpp`
4. Confirm `atone` tests pass
5. Write `tests/test_lemcore.cpp` — translation loading tests
6. Confirm tests compile and fail
7. Fix `lisTraductions()` in `src/lemCore.cpp`
8. Confirm all tests pass

---

## Files Changed

| File | Change |
|---|---|
| `src/ch.cpp` | Rewrite `atone()` body — single-pass byte decoder |
| `src/lemCore.cpp` | Add language filter in `lisTraductions()` |
| `tests/test_ch.cpp` | New — unit tests for `atone()` |
| `tests/test_lemcore.cpp` | New — unit tests for translation loading |

No header changes. No interface changes. No callers modified.
