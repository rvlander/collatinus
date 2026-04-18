/*       ch.h     */

#ifndef CH_H
#define CH_H

#include <string>
#include <vector>
#include "string_utils.h"

namespace Ch
{
std::vector<std::string> ajoute(const std::string &mot, std::vector<std::string> liste);
void allonge(std::string *f);
std::string atone(const std::string &a, bool bdc = false);
std::string communes(const std::string &g);
void deQuant(std::string *c);
const std::string consonnes = "bcdfgjklmnpqrstvxz";
void genStrNum(const std::string &s, std::string *ch, int *n);
std::string deramise(const std::string &r);
std::string deAccent(const std::string &c);
void elide(std::string *mp);
bool sort_i(const std::string &a, const std::string &b);
bool inv_sort_i(const std::string &a, const std::string &b);
std::string versPC(const std::string &k);
std::string versPedeCerto(const std::string &k);
// U+00B7 MIDDLE DOT in UTF-8
const std::string separSyll = "\xC2\xB7";
// String of long/short vowels (UTF-8): āăēĕīĭōŏūŭȳўĀĂĒĔĪĬŌŎŪŬȲЎ
const std::string voyelles =
    "\xC4\x81\xC4\x83"  // ā ă
    "\xC4\x93\xC4\x95"  // ē ĕ
    "\xC4\xAB\xC4\xAD"  // ī ĭ
    "\xC5\x8D\xC5\x8F"  // ō ŏ
    "\xC5\xAB\xC5\xAD"  // ū ŭ
    "\xC8\xB3\xD1\x9E"  // ȳ ў
    "\xC4\x80\xC4\x82"  // Ā Ă
    "\xC4\x92\xC4\x94"  // Ē Ĕ
    "\xC4\xAA\xC4\xAC"  // Ī Ĭ
    "\xC5\x8C\xC5\x8E"  // Ō Ŏ
    "\xC5\xAA\xC5\xAC"  // Ū Ŭ
    "\xC8\xB2\xD0\x8E"; // Ȳ Ў
std::string transforme(const std::string &k);
std::string accentue(const std::string &l);
std::string ajoutSuff(const std::string &fq, const std::string &suffixe,
                      const std::string &l_etym, int accent);
} // namespace Ch

#endif
