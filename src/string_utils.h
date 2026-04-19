#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <fstream>

// UTF-8 helpers
inline size_t utf8CharLen(unsigned char c) {
    if (c < 0x80) return 1;
    if (c < 0xE0) return 2;
    if (c < 0xF0) return 3;
    return 4;
}

// Position of last codepoint
inline size_t utf8LastCharPos(const std::string &s) {
    if (s.empty()) return std::string::npos;
    size_t i = s.size() - 1;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) --i;
    return i;
}

// Remove n codepoints from the end of a UTF-8 string (safe chop)
inline void utf8Chop(std::string &s, int n) {
    for (int k = 0; k < n && !s.empty(); ++k) {
        size_t pos = s.size() - 1;
        while (pos > 0 && ((unsigned char)s[pos] & 0xC0) == 0x80) --pos;
        s.erase(pos);
    }
}

// Byte length of the codepoint starting at pos
inline size_t utf8CharLenAt(const std::string &s, size_t pos) {
    if (pos >= s.size()) return 0;
    return utf8CharLen((unsigned char)s[pos]);
}

// The codepoint (as substring) at byte position pos
inline std::string utf8CharAt(const std::string &s, size_t pos) {
    size_t len = utf8CharLenAt(s, pos);
    return s.substr(pos, len);
}

// True if the char (as UTF-8 substring) appears in haystack
inline bool utf8SetContains(const std::string &haystack, const std::string &ch) {
    if (ch.empty()) return false;
    return haystack.find(ch) != std::string::npos;
}

// True if the codepoint at s[pos] appears in chars
inline bool utf8CharInSet(const std::string &s, size_t pos, const std::string &chars) {
    return utf8SetContains(chars, utf8CharAt(s, pos));
}

// split by single char delimiter
inline std::vector<std::string> split(const std::string &s, char sep) {
    std::vector<std::string> result;
    std::string cur;
    for (unsigned char c : s) {
        if (c == (unsigned char)sep) { result.push_back(cur); cur.clear(); }
        else cur += (char)c;
    }
    result.push_back(cur);
    return result;
}

// split by string delimiter
inline std::vector<std::string> split(const std::string &s, const std::string &sep) {
    std::vector<std::string> result;
    if (sep.empty()) { result.push_back(s); return result; }
    size_t start = 0, pos;
    while ((pos = s.find(sep, start)) != std::string::npos) {
        result.push_back(s.substr(start, pos - start));
        start = pos + sep.size();
    }
    result.push_back(s.substr(start));
    return result;
}

inline std::string join(const std::vector<std::string> &v, const std::string &sep) {
    std::string r;
    for (size_t i = 0; i < v.size(); ++i) { if (i) r += sep; r += v[i]; }
    return r;
}

inline std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= ' ') ++a;
    while (b > a && (unsigned char)s[b-1] <= ' ') --b;
    return s.substr(a, b - a);
}

inline std::string simplified(const std::string &s) {
    std::string r = trim(s);
    std::string out; bool sp = false;
    for (char c : r) {
        if ((unsigned char)c <= ' ') { if (!sp) { out += ' '; sp = true; } }
        else { out += c; sp = false; }
    }
    return out;
}

inline bool startsWith(const std::string &s, const std::string &pre) {
    return s.size() >= pre.size() && s.compare(0, pre.size(), pre) == 0;
}

inline bool endsWith(const std::string &s, const std::string &suf) {
    return s.size() >= suf.size() && s.compare(s.size()-suf.size(), suf.size(), suf) == 0;
}

inline bool contains(const std::string &s, const std::string &sub) {
    return s.find(sub) != std::string::npos;
}

inline bool contains(const std::string &s, char c) {
    return s.find(c) != std::string::npos;
}

inline std::string replaceAll(std::string s, const std::string &from, const std::string &to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// Replace single char with single char (ASCII)
inline std::string replaceChar(std::string s, char from, char to) {
    for (char &c : s) if (c == from) c = to;
    return s;
}

// ASCII-only toLower / toUpper
inline std::string toLower(std::string s) {
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}
inline std::string toUpper(std::string s) {
    for (char &c : s) c = (char)toupper((unsigned char)c);
    return s;
}

// Qt-style section: split by sep char, return fields [from..to]
inline std::string section(const std::string &s, char sep, int from, int to = -1) {
    std::vector<std::string> parts = split(s, sep);
    if (from < 0) from = 0;
    if (to < 0) to = (int)parts.size() - 1;
    if (from >= (int)parts.size()) return "";
    if (to >= (int)parts.size()) to = (int)parts.size() - 1;
    std::string r;
    for (int i = from; i <= to; ++i) { if (i > from) r += sep; r += parts[i]; }
    return r;
}

// Safe toInt (returns 0 if not a number)
inline int toInt(const std::string &s) {
    if (s.empty()) return 0;
    try { return std::stoi(s); } catch (...) { return 0; }
}

inline float toFloat(const std::string &s) {
    if (s.empty()) return 0.0f;
    try { return std::stof(s); } catch (...) { return 0.0f; }
}

// Remove duplicates from vector (preserve order, keep first)
template<typename T>
void removeDuplicates(std::vector<T> &v) {
    std::vector<T> seen;
    auto it = std::remove_if(v.begin(), v.end(), [&seen](const T &x) {
        if (std::find(seen.begin(), seen.end(), x) != seen.end()) return true;
        seen.push_back(x); return false;
    });
    v.erase(it, v.end());
}

// Multimap helpers
template<typename K, typename V>
std::vector<V> mm_values(const std::multimap<K,V> &m, const K &key) {
    std::vector<V> r;
    auto range = m.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) r.push_back(it->second);
    return r;
}

template<typename K, typename V>
std::vector<V> mm_all_values(const std::multimap<K,V> &m) {
    std::vector<V> r;
    for (auto &p : m) r.push_back(p.second);
    return r;
}

template<typename K, typename V>
std::vector<K> mm_unique_keys(const std::multimap<K,V> &m) {
    std::vector<K> r;
    for (auto it = m.begin(); it != m.end(); it = m.upper_bound(it->first))
        r.push_back(it->first);
    return r;
}

template<typename K, typename V>
std::vector<K> map_keys(const std::map<K,V> &m) {
    std::vector<K> r;
    for (auto &p : m) r.push_back(p.first);
    return r;
}

template<typename K, typename V>
std::vector<V> map_values(const std::map<K,V> &m) {
    std::vector<V> r;
    for (auto &p : m) r.push_back(p.second);
    return r;
}

template<typename K, typename V>
V map_value(const std::map<K,V> &m, const K &key, const V &def = V()) {
    auto it = m.find(key);
    return it != m.end() ? it->second : def;
}

// Read file lines, skipping empty lines and comment lines (starting with '!')
// but keeping lines that start with "! --- "
inline std::vector<std::string> readFileLines(const std::string &path) {
    std::vector<std::string> result;
    std::ifstream f(path);
    if (!f.is_open()) return result;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '!' && !startsWith(line, "! --- ")) continue;
        result.push_back(line);
    }
    return result;
}

// List files in a directory matching a prefix (e.g. "morphos.")
// Returns filenames only (not full paths)
#include <dirent.h>
inline std::vector<std::string> listDirByPrefix(const std::string &dir, const std::string &prefix) {
    std::vector<std::string> result;
    DIR *d = opendir(dir.c_str());
    if (!d) return result;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (startsWith(name, prefix)) result.push_back(name);
    }
    closedir(d);
    return result;
}

// Get file extension (part after last dot), empty if none
inline std::string fileExtension(const std::string &filename) {
    size_t pos = filename.rfind('.');
    if (pos == std::string::npos) return "";
    return filename.substr(pos + 1);
}

// Remove ASCII letter bytes from string (for versPedeCerto)
inline std::string removeLetters(const std::string &s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) {
            if (!isalpha(c)) r += (char)c;
            ++i;
        } else {
            size_t len = utf8CharLen(c);
            r += s.substr(i, len);
            i += len;
        }
    }
    return r;
}

// Check if string is purely a decimal integer
inline bool isInteger(const std::string &s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i)
        if (!isdigit((unsigned char)s[i])) return false;
    return true;
}
