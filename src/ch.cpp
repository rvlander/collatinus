/*      ch.cpp  */

#include "ch.h"
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Helpers local to this file
// ---------------------------------------------------------------------------

// Table-driven multi-byte replacement (replaces UTF-8 char sequences)
static std::string replaceU(std::string s,
                             const char *from, const char *to)
{
    return replaceAll(s, std::string(from), std::string(to));
}

// Count occurrences of UTF-8 substring in s
static int countOcc(const std::string &s, const std::string &sub)
{
    int n = 0;
    size_t pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) { ++n; pos += sub.size(); }
    return n;
}

// ---------------------------------------------------------------------------
// Ch::ajoute
// ---------------------------------------------------------------------------
std::vector<std::string> Ch::ajoute(const std::string &mot,
                                     std::vector<std::string> liste)
{
    for (auto &s : liste) s = mot + s;
    return liste;
}

// ---------------------------------------------------------------------------
// Ch::allonge  — lengthens the last vowel of *f before a consonant
// Replaces patterns like: (a|ă)(cons)$ -> ā\1  etc.
// All done by inspecting the last two bytes/codepoints.
// ---------------------------------------------------------------------------

// Mapping: short vowel UTF-8 -> long vowel UTF-8
struct VowelPair { const char *shortv; const char *longv; };
static const VowelPair vowelPairs[] = {
    { "a",       "\xC4\x81" },   // a  -> ā
    { "\xC4\x83","\xC4\x81" },   // ă  -> ā
    { "e",       "\xC4\x93" },   // e  -> ē
    { "\xC4\x95","\xC4\x93" },   // ĕ  -> ē
    { "i",       "\xC4\xAB" },   // i  -> ī
    { "\xC4\xAD","\xC4\xAB" },   // ĭ  -> ī
    { "o",       "\xC5\x8D" },   // o  -> ō
    { "\xC5\x8F","\xC5\x8D" },   // ŏ  -> ō
    { "u",       "\xC5\xAB" },   // u  -> ū
    { "\xC5\xAD","\xC5\xAB" },   // ŭ  -> ū
    { "y",       "\xC8\xB3" },   // y  -> ȳ
    { "\xC8\xB3","\xC8\xB3" },   // ȳ  stays ȳ
    { "A",       "\xC4\x80" },   // A  -> Ā
    { "\xC4\x82","\xC4\x80" },   // Ă  -> Ā
    { "E",       "\xC4\x92" },   // E  -> Ē
    { "\xC4\x94","\xC4\x92" },   // Ĕ  -> Ē
    { "I",       "\xC4\xAA" },   // I  -> Ī
    { "\xC4\xAC","\xC4\xAA" },   // Ĭ  -> Ī
    { "O",       "\xC5\x8C" },   // O  -> Ō
    { "\xC5\x8E","\xC5\x8C" },   // Ŏ  -> Ō
    { "U",       "\xC5\xAA" },   // U  -> Ū
    { "\xC5\xAC","\xC5\xAA" },   // Ŭ  -> Ū
    { "Y",       "\xC8\xB2" },   // Y  -> Ȳ
    { "\xC8\xB2","\xC8\xB2" },   // Ȳ  stays Ȳ
    { nullptr, nullptr }
};

void Ch::allonge(std::string *f)
{
    if (f->empty()) return;
    // last codepoint must be a consonant
    size_t lastPos = utf8LastCharPos(*f);
    std::string lastCh = utf8CharAt(*f, lastPos);
    if (lastCh.size() != 1 || !contains(consonnes, lastCh))
        return;
    // diphthong guard: āe, āu, ēu, ōe before the consonant
    static const char * const diphthongs[] = {
        "\xC4\x81""e", "\xC4\x81""u", "\xC4\x93""u", "\xC5\x8D""e", nullptr };
    if (f->size() >= 3) {
        std::string pre3 = f->substr(f->size() >= 4 ? lastPos - 2 : 0);
        for (int i = 0; diphthongs[i]; ++i)
            if (endsWith(f->substr(0, lastPos), diphthongs[i]))
                return;
    }
    // try to replace the vowel before the final consonant
    size_t consPos = lastPos;
    // find start of the previous codepoint
    size_t vPos = consPos;
    if (vPos == 0) return;
    --vPos;
    while (vPos > 0 && ((unsigned char)(*f)[vPos] & 0xC0) == 0x80) --vPos;
    std::string vCh = utf8CharAt(*f, vPos);
    for (int i = 0; vowelPairs[i].shortv; ++i) {
        if (vCh == vowelPairs[i].shortv) {
            f->replace(vPos, vCh.size(),
                       std::string(vowelPairs[i].longv));
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Ch::atone  — strip all diacritics (macron, breve) from the string
// ---------------------------------------------------------------------------
std::string Ch::atone(const std::string &a, bool bdc)
{
    std::string s = a;
    // minuscules
    s = replaceAll(s, "\xC4\x81", "a");   // ā
    s = replaceAll(s, "\xC4\x83", "a");   // ă
    s = replaceAll(s, "\xC4\x93", "e");   // ē
    s = replaceAll(s, "\xC4\x95", "e");   // ĕ
    s = replaceAll(s, "\xC4\xAB", "i");   // ī
    s = replaceAll(s, "\xC4\xAD", "i");   // ĭ
    s = replaceAll(s, "\xC5\x8D", "o");   // ō
    s = replaceAll(s, "\xC5\x8F", "o");   // ŏ
    s = replaceAll(s, "\xC5\xAB", "u");   // ū
    s = replaceAll(s, "\xC5\xAD", "u");   // ŭ
    s = replaceAll(s, "\xC8\xB3", "y");   // ȳ (U+0233)
    s = replaceAll(s, "\xD1\x9E", "y");   // ў (U+045E)
    if (!bdc) {
        // majuscules
        s = replaceAll(s, "\xC4\x80", "A");  // Ā
        s = replaceAll(s, "\xC4\x82", "A");  // Ă
        s = replaceAll(s, "\xC4\x92", "E");  // Ē
        s = replaceAll(s, "\xC4\x94", "E");  // Ĕ
        s = replaceAll(s, "\xC4\xAA", "I");  // Ī
        s = replaceAll(s, "\xC4\xAC", "I");  // Ĭ
        s = replaceAll(s, "\xC5\x8C", "O");  // Ō
        s = replaceAll(s, "\xC5\x8E", "O");  // Ŏ
        s = replaceAll(s, "\xC5\xAA", "U");  // Ū
        s = replaceAll(s, "\xC5\xAC", "U");  // Ŭ
        s = replaceAll(s, "\xC8\xB2", "Y");  // Ȳ (U+0232)
        s = replaceAll(s, "\xD0\x8E", "Y");  // Ў (U+040E)
    }
    s = replaceAll(s, "\xC4\xB1", "i");   // ı (U+0131)
    s = replaceAll(s, "\xE1\xBB\xA5", "u"); // ụ (U+1EE5)
    // combining breve U+0306 = 0xCC 0x86
    s = replaceAll(s, "\xCC\x86", "");
    return s;
}

// ---------------------------------------------------------------------------
// Ch::communes  — mark vowels without quantity as communes (ā̆, ē̆, …)
// Combining breve U+0306 = \xCC\x86
// ---------------------------------------------------------------------------
std::string Ch::communes(const std::string &gin)
{
    // Check if first byte is uppercase ASCII
    bool maj = !gin.empty() && isupper((unsigned char)gin[0]);
    std::string g = atone(toLower(gin));

    bool hasVowel = contains(g,"a") || contains(g,"e") || contains(g,"i") ||
                    contains(g,"o") || contains(g,"u") || contains(g,"y");
    if (!hasVowel) {
        if (maj && !g.empty()) g[0] = (char)toupper((unsigned char)g[0]);
        return g;
    }

    // We build the result character by character, doing context-sensitive replacements.
    // ā̆ = ā + combining breve = \xC4\x81\xCC\x86
    // ē̆ = ē + combining breve = \xC4\x93\xCC\x86
    // ī̆ = ī + combining breve = \xC4\xAB\xCC\x86
    // ō̆ = ō + combining breve = \xC5\x8D\xCC\x86
    // ū̆ = ū + combining breve = \xC5\xAB\xCC\x86
    // ȳ̆ = ȳ + combining breve = \xC8\xB3\xCC\x86

    // For simplicity: after atone+toLower, input is all ASCII
    // a -> ā̆, e -> ē̆ (context), i -> ī̆, o -> ō̆, u -> ū̆ (context), y -> ȳ̆ (context)
    std::string r;
    r.reserve(g.size() * 3);
    for (size_t i = 0; i < g.size(); ++i) {
        char c = g[i];
        if (c == 'a') {
            r += "\xC4\x81\xCC\x86"; // ā̆
        } else if (c == 'e') {
            // not after ā̆ sequences — but since we already converted above,
            // check what we appended: if last non-empty output ends in breve
            // for ae/oe context; simpler: check if previous char was part of ae/oe
            // Original: replace e not after [āăō]. Since input is now plain ASCII
            // after atone, we just check the preceding char in g.
            bool afterVow = (i > 0 && (g[i-1]=='a' || g[i-1]=='o'));
            if (!afterVow)
                r += "\xC4\x93\xCC\x86"; // ē̆
            else
                r += c;
        } else if (c == 'i') {
            r += "\xC4\xAB\xCC\x86"; // ī̆
        } else if (c == 'o') {
            r += "\xC5\x8D\xCC\x86"; // ō̆
        } else if (c == 'u') {
            // not after [aeoq]
            bool afterVow = (i > 0 && (g[i-1]=='a' || g[i-1]=='e' || g[i-1]=='o' || g[i-1]=='q'));
            if (!afterVow)
                r += "\xC5\xAB\xCC\x86"; // ū̆
            else
                r += c;
        } else if (c == 'y') {
            // not after [a]
            bool afterVow = (i > 0 && g[i-1]=='a');
            if (!afterVow)
                r += "\xC8\xB3\xCC\x86"; // ȳ̆
            else
                r += c;
        } else {
            r += c;
        }
    }

    if (maj && !r.empty())
        r[0] = (char)toupper((unsigned char)r[0]);
    return r;
}

// ---------------------------------------------------------------------------
// Ch::deQuant  — remove quantity from last vowel (used in elision)
// ---------------------------------------------------------------------------
void Ch::deQuant(std::string *c)
{
    // Remove trailing combining breve U+0306 if present
    if (endsWith(*c, "\xCC\x86")) c->erase(c->size() - 2);

    // Helper: if string ends with longvowel + optional 'm', replace with shortvowel
    struct Pair { const char *lv; const char *sv; };
    static const Pair pairs[] = {
        { "\xC4\x81", "a" }, { "\xC4\x83", "a" }, // ā ă
        { "\xC4\x93", "e" }, { "\xC4\x95", "e" }, // ē ĕ
        { "\xC4\xAB", "i" }, { "\xC4\xAD", "i" }, // ī ĭ
        { "\xC5\x8D", "o" }, { "\xC5\x8F", "o" }, // ō ŏ
        { "\xC5\xAB", "u" }, { "\xC5\xAD", "u" }, // ū ŭ
        { "\xC8\xB3", "y" }, { "\xC8\xB2", "Y" }, // ȳ Ȳ
        { nullptr, nullptr }
    };
    for (int i = 0; pairs[i].lv; ++i) {
        std::string lv = pairs[i].lv;
        // ends with longvowel
        if (endsWith(*c, lv)) {
            c->erase(c->size() - lv.size());
            c->append(pairs[i].sv);
            return;
        }
        // ends with longvowel + 'm'
        if (endsWith(*c, lv + "m")) {
            c->erase(c->size() - lv.size() - 1);
            c->append(std::string(pairs[i].sv) + "m");
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Ch::deAccent  — remove all accent/diacritic combining marks
// (simplified: direct table of composed → base)
// ---------------------------------------------------------------------------
std::string Ch::deAccent(const std::string &cin)
{
    std::string c = cin;
    // Remove combining marks (U+0300-U+0308, U+0304, U+0306, U+0327, U+0328)
    // as their UTF-8 byte sequences
    c = replaceAll(c, "\xCC\x80", ""); // U+0300 grave
    c = replaceAll(c, "\xCC\x81", ""); // U+0301 acute
    c = replaceAll(c, "\xCC\x82", ""); // U+0302 circumflex
    c = replaceAll(c, "\xCC\x83", ""); // U+0303 tilde
    c = replaceAll(c, "\xCC\x84", ""); // U+0304 macron
    c = replaceAll(c, "\xCC\x86", ""); // U+0306 breve
    c = replaceAll(c, "\xCC\x88", ""); // U+0308 diaeresis
    c = replaceAll(c, "\xCC\xA7", ""); // U+0327 cedilla
    c = replaceAll(c, "\xCC\xA8", ""); // U+0328 ogonek
    // Also strip precomposed accented chars to base
    c = replaceAll(c, "\xC3\xA1", "a"); // á
    c = replaceAll(c, "\xC3\xA9", "e"); // é
    c = replaceAll(c, "\xC3\xAD", "i"); // í
    c = replaceAll(c, "\xC3\xB3", "o"); // ó
    c = replaceAll(c, "\xC3\xBA", "u"); // ú
    c = replaceAll(c, "\xC3\xBD", "y"); // ý
    c = replaceAll(c, "\xC3\x81", "A"); // Á
    c = replaceAll(c, "\xC3\x89", "E"); // É
    c = replaceAll(c, "\xC3\x8D", "I"); // Í
    c = replaceAll(c, "\xC3\x93", "O"); // Ó
    c = replaceAll(c, "\xC3\x9A", "U"); // Ú
    c = replaceAll(c, "\xC3\x9D", "Y"); // Ý
    return c;
}

// ---------------------------------------------------------------------------
// Ch::deramise
// ---------------------------------------------------------------------------
std::string Ch::deramise(const std::string &rin)
{
    std::string r = rin;
    r = replaceChar(r, 'J', 'I');
    r = replaceChar(r, 'j', 'i');
    r = replaceChar(r, 'v', 'u');
    r = replaceAll(r, "\xC3\xA6", "ae");  // æ
    r = replaceAll(r, "\xC3\x86", "Ae");  // Æ
    r = replaceAll(r, "\xC5\x93", "oe");  // œ
    r = replaceAll(r, "\xC5\x92", "Oe");  // Œ
    r = replaceAll(r, "\xC8\xA9", "ae");  // ȩ
    r = replaceAll(r, "\xC4\x99", "ae");  // ę
    r = replaceAll(r, "\xE1\xBB\xA5", "u"); // ụ
    r = replaceChar(r, 'V', 'U');
    return r;
}

// ---------------------------------------------------------------------------
// Ch::elide
// ---------------------------------------------------------------------------
void Ch::elide(std::string *mp)
{
    if (mp->size() <= 1) return;
    size_t taille = mp->size();
    size_t lastPos = utf8LastCharPos(*mp);
    std::string lastCh = utf8CharAt(*mp, lastPos);

    // ā (U+0101) = \xC4\x81
    const std::string ae_long = "\xC4\x81""e"; // āe

    bool cond1 = (taille > 1) &&
        (lastCh == "m" ||
         endsWith(*mp, ae_long) ||
         endsWith(*mp, "\xCC\x86")); // combining breve

    if (cond1) {
        // check penultimate char is a vowel
        size_t prevPos = lastPos;
        if (lastCh.size() == 1 && prevPos > 0) --prevPos;
        else if (prevPos >= lastCh.size()) prevPos -= lastCh.size();
        while (prevPos > 0 && ((unsigned char)(*mp)[prevPos] & 0xC0) == 0x80) --prevPos;
        if (utf8CharInSet(*mp, prevPos, voyelles)) {
            deQuant(mp);
            mp->insert(prevPos, "[");
            mp->append("]");
            return;
        }
    }

    if (utf8CharInSet(*mp, lastPos, voyelles) &&
        *mp != "\xC5\x8D") // ō
    {
        deQuant(mp);
        mp->insert(lastPos, "[");
        mp->append("]");
    }
}

// ---------------------------------------------------------------------------
// Ch::genStrNum
// ---------------------------------------------------------------------------
void Ch::genStrNum(const std::string &s, std::string *ch, int *n)
{
    ch->clear();
    *n = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i]))
            *ch += s[i];
        else {
            *n = toInt(s.substr(i));
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Ch::sort_i / inv_sort_i
// ---------------------------------------------------------------------------
bool Ch::sort_i(const std::string &a, const std::string &b)
{
    std::string la = toLower(atone(a));
    std::string lb = toLower(atone(b));
    return la < lb;
}

bool Ch::inv_sort_i(const std::string &a, const std::string &b)
{
    std::string la = toLower(atone(a));
    std::string lb = toLower(atone(b));
    return la > lb;
}

// ---------------------------------------------------------------------------
// Ch::versPC
// ---------------------------------------------------------------------------
std::string Ch::versPC(const std::string &kin)
{
    std::string k = toLower(kin);
    if (contains(k, "[")) k = section(k, '[', 0, 0) + "`";
    k = replaceAll(k, "qu", "");
    k = replaceAll(k, "gu", "");
    k = replaceAll(k, "\xC4\x81""e", "+"); // āe
    k = replaceAll(k, "\xC5\x8D""e", "+"); // ōe
    k = replaceAll(k, "\xC4\x81""u", "+"); // āu
    k = replaceAll(k, "\xC4\x93""u", "+"); // ēu
    k = replaceAll(k, "a", "*");
    k = replaceAll(k, "e", "*");
    k = replaceAll(k, "i", "*");
    k = replaceAll(k, "o", "*");
    k = replaceAll(k, "u", "*");
    k = replaceAll(k, "y", "*");
    return versPedeCerto(k);
}

// ---------------------------------------------------------------------------
// Ch::versPedeCerto
// ---------------------------------------------------------------------------
std::string Ch::versPedeCerto(const std::string &kin)
{
    std::string k = kin;
    // long vowels -> +
    k = replaceAll(k, "\xC4\x81", "+"); // ā
    k = replaceAll(k, "\xC4\x93", "+"); // ē
    k = replaceAll(k, "\xC4\xAB", "+"); // ī
    k = replaceAll(k, "\xC5\x8D", "+"); // ō
    k = replaceAll(k, "\xC5\xAB", "+"); // ū
    k = replaceAll(k, "\xC8\xB3", "+"); // ȳ
    k = replaceAll(k, "\xC4\x80", "+"); // Ā
    k = replaceAll(k, "\xC4\x92", "+"); // Ē
    k = replaceAll(k, "\xC4\xAA", "+"); // Ī
    k = replaceAll(k, "\xC5\x8C", "+"); // Ō
    k = replaceAll(k, "\xC5\xAA", "+"); // Ū
    k = replaceAll(k, "\xC8\xB2", "+"); // Ȳ
    // short vowels -> -
    k = replaceAll(k, "\xC4\x83", "-"); // ă
    k = replaceAll(k, "\xC4\x95", "-"); // ĕ
    k = replaceAll(k, "\xC4\xAD", "-"); // ĭ
    k = replaceAll(k, "\xC5\x8F", "-"); // ŏ
    k = replaceAll(k, "\xC5\xAD", "-"); // ŭ
    k = replaceAll(k, "\xD1\x9E", "-"); // ў
    k = replaceAll(k, "\xC4\x82", "-"); // Ă
    k = replaceAll(k, "\xC4\x94", "-"); // Ĕ
    k = replaceAll(k, "\xC4\xAC", "-"); // Ĭ
    k = replaceAll(k, "\xC5\x8E", "-"); // Ŏ
    k = replaceAll(k, "\xC5\xAC", "-"); // Ŭ
    k = replaceAll(k, "\xD0\x8E", "-"); // Ў
    // long + combining breve -> commune *
    k = replaceAll(k, "+\xCC\x86", "*");
    if (contains(k, "[")) k = section(k, '[', 0, 0) + "`";
    k = replaceAll(k, "\xE1\xBB\xA5", ""); // ụ
    k = removeLetters(k);
    return k;
}

// ---------------------------------------------------------------------------
// Ch::transforme
// ---------------------------------------------------------------------------
std::string Ch::transforme(const std::string &kin)
{
    std::string k = kin;
    k = replaceAll(k, "\xC4\x81""e", "\xC3\xA6+");   // āe -> æ+
    k = replaceAll(k, "\xC5\x8D""e", "\xC5\x93+");   // ōe -> œ+
    k = replaceAll(k, "\xC4\x83""e", "\xC3\xA6-");   // ăe -> æ-
    k = replaceAll(k, "\xC4\x80""e", "\xC3\x86+");   // Āe -> Æ+
    k = replaceAll(k, "\xC5\x8C""e", "\xC5\x92+");   // Ōe -> Œ+
    // minuscules long -> X+, short -> X-
    k = replaceAll(k, "\xC4\x81", "a+"); // ā
    k = replaceAll(k, "\xC4\x83", "a-"); // ă
    k = replaceAll(k, "\xC4\x93", "e+"); // ē
    k = replaceAll(k, "\xC4\x95", "e-"); // ĕ
    k = replaceAll(k, "\xC4\xAB", "i+"); // ī
    k = replaceAll(k, "\xC4\xAD", "i-"); // ĭ
    k = replaceAll(k, "\xC5\x8D", "o+"); // ō
    k = replaceAll(k, "\xC5\x8F", "o-"); // ŏ
    k = replaceAll(k, "\xC5\xAB", "u+"); // ū
    k = replaceAll(k, "\xC5\xAD", "u-"); // ŭ
    k = replaceAll(k, "\xC8\xB3", "y+"); // ȳ
    k = replaceAll(k, "\xD1\x9E", "y-"); // ў
    // majuscules
    k = replaceAll(k, "\xC4\x80", "A+"); // Ā
    k = replaceAll(k, "\xC4\x82", "A-"); // Ă
    k = replaceAll(k, "\xC4\x92", "E+"); // Ē
    k = replaceAll(k, "\xC4\x94", "E-"); // Ĕ
    k = replaceAll(k, "\xC4\xAA", "I+"); // Ī
    k = replaceAll(k, "\xC4\xAC", "I-"); // Ĭ
    k = replaceAll(k, "\xC5\x8C", "O+"); // Ō
    k = replaceAll(k, "\xC5\x8E", "O-"); // Ŏ
    k = replaceAll(k, "\xC5\xAA", "U+"); // Ū
    k = replaceAll(k, "\xC5\xAC", "U-"); // Ŭ
    k = replaceAll(k, "\xC8\xB2", "Y+"); // Ȳ
    k = replaceAll(k, "\xD0\x8E", "Y-"); // Ў
    k = replaceAll(k, "+\xCC\x86", "*");
    k = replaceAll(k, "\xE1\xBB\xA5", "u"); // ụ
    return k;
}

// ---------------------------------------------------------------------------
// Ch::accentue
// ---------------------------------------------------------------------------
std::string Ch::accentue(const std::string &l)
{
    if (l == "\xC5\x93" || l == "\xC5\x92") // œ Œ
        return l + "\xCC\x81"; // + combining acute
    if (l == "\xC3\xA6") return "\xC7\xBD"; // æ -> ǽ
    if (l == "\xC3\x86") return "\xC7\xBC"; // Æ -> Ǽ
    if (l.empty()) return l;
    switch ((unsigned char)l[0]) {
        case 'a': return "\xC3\xA1"; // á
        case 'e': return "\xC3\xA9"; // é
        case 'i': return "\xC3\xAD"; // í
        case 'o': return "\xC3\xB3"; // ó
        case 'u': return "\xC3\xBA"; // ú
        case 'y': return "\xC3\xBD"; // ý
        case 'A': return "\xC3\x81"; // Á
        case 'E': return "\xC3\x89"; // É
        case 'I': return "\xC3\x8D"; // Í
        case 'O': return "\xC3\x93"; // Ó
        case 'U': return "\xC3\x9A"; // Ú
        case 'Y': return "\xC3\x9D"; // Ý
        default:  return l;
    }
}

// ---------------------------------------------------------------------------
// Ch::ajoutSuff
// In the transformed string all chars are ASCII except separSyll (\xC2\xB7, 2 bytes).
// We use an internal 1-byte marker (0x01) for separSyll during processing,
// then replace back at the end.
// ---------------------------------------------------------------------------
std::string Ch::ajoutSuff(const std::string &fqin,
                           const std::string &suffixe,
                           const std::string &l_etym,
                           int accent)
{
    std::string fq = fqin;
    bool illius = false, cesure = false, sansAccent = false;
    if (accent > 7) { illius = true; accent -= 8; }
    if (accent > 3) { cesure = true; accent -= 4; }

    if (accent > 0) {
        const std::string signes = "+-*";
        fq = transforme(fq);
        // Replace separSyll with internal 1-byte marker for index arithmetic
        fq = replaceAll(fq, separSyll, "\x01");

        int l = countOcc(fq, "+") + countOcc(fq, "-") + countOcc(fq, "*");
        int i = (int)fq.size() - 1;

        if (suffixe.empty() || suffixe == "st") {
            if (l > 2) {
                if (illius && endsWith(fq, "i*u-s")) {
                    fq.erase(fq.size() - 5);
                    fq += "\xC3\xAD*u-s"; // í*u-s
                } else {
                    while (i >= 0 && !contains(signes, fq[i])) i--;
                    i--;
                    while (i >= 0 && !contains(signes, fq[i])) i--;
                    sansAccent = (fq[i] == '*') && (accent == 3);
                    if ((fq[i] == '-') || ((fq[i] == '*') && (accent == 2))) {
                        i--;
                        while (i >= 0 && !contains(signes, fq[i])) i--;
                    }
                    if (!sansAccent) {
                        if (i > 1)
                            fq = fq.substr(0, i-1) + accentue(fq.substr(i-1, 1)) + fq.substr(i);
                        else
                            fq = accentue(fq.substr(i-1, 1)) + fq.substr(i);
                    }
                }
            }
            fq += suffixe;
        } else {
            if (l > 1) {
                while (i >= 0 && !contains(signes, fq[i])) i--;
                if (i > 1)
                    fq = fq.substr(0, i-1) + accentue(fq.substr(i-1, 1)) + fq.substr(i);
                else
                    fq = accentue(fq.substr(i-1, 1)) + fq.substr(i);
            }
            fq += suffixe;
            fq = replaceAll(fq, "\xC4\x95", "e-"); // ĕ
            l++;
        }

        if ((l > 1) && cesure) {
            int j = (int)fq.size() - 1;
            while (j >= 0 && !contains(signes, fq[j])) j--;
            int k = j;
            j -= 2;
            while (j >= 0 && !contains(signes, fq[j])) j--;
            while (j > 0) {
                if (k == j + 2)
                    fq.insert(j, "\x01");
                else {
                    int nbCons = 0;
                    for (int n = j+1; n < k-1; n++)
                        if (contains(consonnes, fq[n]) || fq[n]=='h') nbCons++;
                    if (nbCons == 0) {
                        fq.insert(k-1, "\x01");
                    } else {
                        while (!contains(consonnes, std::string(1,fq[k])) && fq[k]!='h') k--;
                        if (nbCons == 1) {
                            fq.insert(k, "\x01");
                        } else {
                            bool remonte = ((fq[k]=='l') && (fq[k-1]!='l') && (fq[k-1]!='r'));
                            remonte = remonte || ((fq[k]=='r') && (fq[k-1]!='r') && (fq[k-1]!='l'));
                            remonte = remonte || (fq[k]=='h');
                            if (remonte) k--;
                            remonte = ((fq[k]=='c') && (fq[k-1]=='s') &&
                                       (fq[k+1]!='a') && (fq[k+1]!='o') &&
                                       (fq[k+1]!='u') && (fq[k+1]!='h'));
                            remonte = remonte || ((fq[k]=='p') && (fq[k-1]=='s'));
                            remonte = remonte || ((fq[k]=='n') && (fq[k-1]=='g'));
                            if (remonte) k--;
                            fq.insert(k, "\x01");
                        }
                    }
                }
                k = j; j -= 2;
                while (j > 0 && !contains(signes, fq[j])) j--;
            }

            // Apply etymological hyphen
            if (!l_etym.empty()) {
                for (auto &etym : split(l_etym, ',')) {
                    std::string fq1 = fq;
                    size_t ei = 0, fi = 0;
                    int changement = 0;
                    bool OK = true;
                    while (ei < etym.size() && fi < fq.size() && OK) {
                        if (etym[ei] == fq[fi] || fq.substr(fi,1) == accentue(etym.substr(ei,1))) {
                            ei++; fi++;
                        } else if (contains(signes, fq[fi]) || fq[fi]=='\xCC') {
                            fi++;
                        } else if (etym[ei] != '\x01' && fq[fi] != '\x01') {
                            OK = false;
                        } else {
                            if (etym[ei] == '\x01') {
                                fq.insert(fi, "\x01");
                                changement++; fi++; ei++;
                            } else {
                                fq.erase(fi, 1);
                                changement--;
                            }
                        }
                    }
                    if (changement == 1) {
                        while (fq[fi] != '\x01') fi++;
                        fq.erase(fi, 1);
                    }
                    if (!OK) fq = fq1;
                }
            }
        }
        fq = replaceAll(fq, "+", "");
        fq = replaceAll(fq, "-", "");
        fq = replaceAll(fq, "*", "");
        // Restore separSyll
        fq = replaceAll(fq, "\x01", separSyll);
        return fq;
    }

    if (suffixe.empty()) return fq;
    allonge(&fq);
    if (suffixe == "st") {
        fq += "s";
        allonge(&fq);
        return fq + "t";
    }
    std::string suf = replaceAll(suffixe, "\xC4\x95""st", "\xC4\x93""st"); // ĕst -> ēst
    return fq + suf;
}
