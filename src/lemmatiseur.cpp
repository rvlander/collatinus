/*      lemmatiseur.cpp
 *
 *  This file is part of COLLATINUS.
 *
 *  COLLATINUS is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  COLLATINVS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with COLLATINUS; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * © Yves Ouvrard, 2009 - 2016
 */

#include "lemmatiseur.h"
#include "lemme.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Helper: split a string at word boundaries, returning alternating
// [separator, word, separator, word, ..., trailing_separator].
// Odd indices are words, even indices are separators (same as Qt's
// QString::split(QRegExp("\\b"))).
// ---------------------------------------------------------------------------
static std::vector<std::string> splitWordBoundary(const std::string &s)
{
    std::vector<std::string> result;
    // Match sequences of Latin letters (ASCII + extended Latin U+00C0-U+024F)
    // encoded as UTF-8.  We match on raw bytes; non-ASCII lead bytes in the
    // range C3-C9 followed by 80-BF cover U+00C0-U+024F.
    std::regex wordRe("[A-Za-z]+|(?:[\xC3-\xC9][\x80-\xBF])+");
    auto it  = std::sregex_iterator(s.begin(), s.end(), wordRe);
    auto end = std::sregex_iterator();
    size_t pos = 0;
    for (; it != end; ++it) {
        const std::smatch &m = *it;
        result.push_back(s.substr(pos, (size_t)m.position() - pos)); // separator
        result.push_back(m.str());                                     // word
        pos = (size_t)m.position() + (size_t)m.length();
    }
    result.push_back(s.substr(pos)); // trailing separator
    return result;
}

// ---------------------------------------------------------------------------
// Helper: collect multimap values in key order (ascending).
// ---------------------------------------------------------------------------
template<typename K, typename V>
static std::vector<V> mm_vals_ordered(const std::multimap<K, V> &mm)
{
    std::vector<V> result;
    for (const auto &kv : mm)
        result.push_back(kv.second);
    return result;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Lemmatiseur::Lemmatiseur(LemCore *l, const std::string &cible,
                         const std::string &resDir)
{
    if (l == nullptr) {
        _lemCore = new LemCore(resDir);
        _lemCore->setExtension(true);
    } else {
        _lemCore = l;
    }

    if (resDir.empty())
        _resDir = "";
    else if (endsWith(resDir, "/"))
        _resDir = resDir;
    else
        _resDir = resDir + "/";

    _alpha   = false;
    _formeT  = false;
    _html    = false;
    _majPert = false;
    _morpho  = false;
    _nonRec  = false;

    if (!cible.empty())
        setCible(cible);
    else
        setCible("fr en es");
}

/**
 * \fn QStringList Lemmat::lemmatiseF (QString f, bool deb)
 * \brief Lemmatise la chaîne f, sans tenir compte des majuscules
 *        si deb (= début de phrase) est à true, et renvoie le
 *        résultat dans une liste.
 */
// ---------------------------------------------------------------------------
// lemmatiseF
// ---------------------------------------------------------------------------
std::vector<std::string> Lemmatiseur::lemmatiseF(const std::string &f, bool deb)
{
    std::vector<std::string> res;
    MapLem ml = _lemCore->lemmatiseM(f, deb);
    for (auto &kv : ml)
        res.push_back(kv.first->humain(_html, _cible));
    return res;
}

/**
 * \fn QStringList Lemmat::frequences (QString txt)
 * \brief Lemmatise txt et renvoie le résultat accompagné
 *        d'informations sur la fréquence d'emploi de
 *        chaque lemme.
 */
// ---------------------------------------------------------------------------
// frequences
// ---------------------------------------------------------------------------
std::vector<std::string> Lemmatiseur::frequences(const std::string &txt)
{
    // Split text on whitespace
    std::regex wsRe("\\s+");
    std::sregex_token_iterator tit(txt.begin(), txt.end(), wsRe, -1);
    std::sregex_token_iterator tend;
    std::vector<std::string> formes_tok(tit, tend);
    // Remove empty tokens
    formes_tok.erase(
        std::remove_if(formes_tok.begin(), formes_tok.end(),
                       [](const std::string &s){ return s.empty(); }),
        formes_tok.end());

    // Regex to capture alphabetic content of a token
    std::regex reAlphas("([A-Za-z\xC0-\xFF]+)");
    // Regex for punctuation (end-of-sentence)
    std::regex rePonct("[.!?;:]");

    std::unordered_map<std::string, int> freq;
    int c = (int)formes_tok.size();
    for (int i = 0; i < c; ++i) {
        std::string forme = formes_tok[i];
        if (forme.empty()) continue;
        // Skip pure integers
        bool allDigit = !forme.empty() &&
            std::all_of(forme.begin(), forme.end(),
                        [](unsigned char ch){ return isdigit(ch); });
        if (allDigit) continue;
        // Extract alphabetic content
        std::smatch sm;
        if (!std::regex_search(forme, sm, reAlphas)) continue;
        forme = sm[1].str();
        // Mark sentence-initial forms
        bool prevPonct = (i > 0) && std::regex_search(formes_tok[i - 1], rePonct);
        if (i == 0 || prevPonct)
            forme = "*" + forme;
        freq[forme]++;
    }

    std::unordered_map<std::string, int> lemOcc;
    std::unordered_map<std::string, std::vector<std::string>> lemFormUnic;
    std::unordered_map<std::string, std::vector<std::string>> lemFormAmb;
    std::unordered_map<std::string, std::vector<std::string>> formLemAmb;

    for (auto &kv : freq) {
        std::string forme = kv.first;
        std::vector<std::string> res;
        if (startsWith(forme, "*"))
            res = lemmatiseF(forme.substr(1), true);
        else
            res = lemmatiseF(forme, false);
        int occ = kv.second;
        if (res.size() == 1) {
            lemFormUnic[res[0]].push_back(forme);
            lemOcc[res[0]] += occ;
        } else {
            for (const auto &r : res) {
                lemFormAmb[r].push_back(forme);
                lemOcc[r] += occ;
            }
            formLemAmb[forme] = res;
        }
    }

    // Collect unique lemme keys
    std::vector<std::string> lemmes_all;
    for (auto &kv : lemFormUnic) lemmes_all.push_back(kv.first);
    for (auto &kv : lemFormAmb)  lemmes_all.push_back(kv.first);
    removeDuplicates(lemmes_all);

    std::vector<std::string> sortie;
    for (const auto &lemme : lemmes_all) {
        int nUnic = 0;
        {
            auto it = lemFormUnic.find(lemme);
            if (it != lemFormUnic.end())
                for (const auto &f2 : it->second) {
                    auto fi = freq.find(f2);
                    if (fi != freq.end()) nUnic += fi->second;
                }
        }
        int nAmb = 0;
        float xAmb = 0.0f;
        {
            auto it = lemFormAmb.find(lemme);
            if (it != lemFormAmb.end()) {
                for (const auto &f2 : it->second) {
                    float nTotLem = 0.0f;
                    auto fla = formLemAmb.find(f2);
                    if (fla != formLemAmb.end())
                        for (const auto &lem2 : fla->second) {
                            auto oi = lemOcc.find(lem2);
                            if (oi != lemOcc.end()) nTotLem += oi->second;
                        }
                    auto fi = freq.find(f2);
                    int fOcc = (fi != freq.end()) ? fi->second : 0;
                    nAmb += fOcc;
                    if (nTotLem > 0.0f) {
                        auto oi = lemOcc.find(lemme);
                        int lOcc = (oi != lemOcc.end()) ? oi->second : 0;
                        xAmb += fOcc * lOcc / nTotLem;
                    }
                }
            }
        }
        int nSort  = (int)(xAmb + nUnic + 10000.5f);
        std::string numero = std::to_string(nSort).substr(1); // 4-digit zero-padded
        int n_int = (int)(xAmb + 0.5f);

        std::ostringstream line;
        if (_hLem.empty()) {
            line << numero
                 << " (" << nUnic << ", " << nAmb << ", " << n_int << ")\t"
                 << lemme << "<br/>\n";
        } else {
            // Apply colour based on whether lemme is known
            std::string lem = Ch::atone(section(lemme, ',', 0, 0));
            lem = replaceAll(lem, "j", "i");
            lem = replaceAll(lem, "J", "I");
            lem = replaceAll(lem, "<strong>", "");
            lem = replaceAll(lem, "</strong>", "");
            std::string color = (_hLem.count(lem) > 0) ? _couleurs[0] : _couleurs[1];
            line << numero
                 << " (" << nUnic << ", " << nAmb << ", " << n_int
                 << ")\t<span style=\"color:" << color << "\">"
                 << lemme << "</span><br/>\n";
        }
        sortie.push_back(line.str());
    }

    std::sort(sortie.begin(), sortie.end(), Ch::inv_sort_i);

    // Strip leading zeros from the sort key prefix
    for (size_t i = 0; i < sortie.size(); ++i) {
        std::string &ls = sortie[i];
        size_t z = 0;
        while (z < ls.size() && ls[z] == '0') ++z;
        ls = ls.substr(z);
        if (!ls.empty() && ls[0] == ' ') ls = "&lt;1" + ls;
    }

    // Prepend legend
    sortie.insert(sortie.begin(), "------------<br/>\n");
    sortie.insert(sortie.begin(),
        "c = nombre probable de formes ambigu\u00ebs rattach\u00e9es \u00e0 ce lemme<br/>\n");
    sortie.insert(sortie.begin(),
        "b = nombre de formes ambigu\u00ebs (partag\u00e9es par plusieurs lemmes)<br/>\n");
    sortie.insert(sortie.begin(),
        "a = nombre de formes rattach\u00e9es seulement \u00e0 ce lemme<br/>\n");
    sortie.insert(sortie.begin(), "n = a+c<br/>\n");
    sortie.insert(sortie.begin(), "l\u00e9gende : n (a, b, c)<br/>\n");

    return sortie;
}

// ---------------------------------------------------------------------------
// lemmatiseT (single-arg: use stored options)
// ---------------------------------------------------------------------------
std::string Lemmatiseur::lemmatiseT(std::string &t)
{
    return lemmatiseT(t, _alpha, _formeT, _morpho, _nonRec);
}

/**
 * \fn QString Lemmatiseur::lemmatiseT (QString &t,
 *  						   bool alpha,
 *  						   bool cumVocibus,
 *  						   bool cumMorpho,
 *  						   bool nreconnu)
 * \brief Renvoie sous forme de chaîne la lemmatisation
 *        et la morphologie de chaque mot du texte t.
 *        Les paramètres permettent de classer la sortie
 *        par ordre alphabétique ; de reproduire la
 *        forme du texte au début de chaque lemmatisation ;
 *        de donner les morphologies de chaque forme ; ou
 *        de rejeter les échecs en fin de liste. D'autres
 *        paramètres, comme le format de sortie txt ou html,
 *        sont donnés par des variables de classe.
 *	      Les paramètres et options true outrepassent les false,
 *        _majPert et _html sont dans les options de la classe.
 *
 *        Par effet de bord, la fonction modifie le texte
 *        t, passé par adresse dans le paramètre &t, en
 *        tenant compte de la liste des mots connus définie
 *        par l'utilisateur via l'option
 *        Fichier/Lire une liste de mots connus.
 *
 */
// ---------------------------------------------------------------------------
// lemmatiseT (full version)
// ---------------------------------------------------------------------------
std::string Lemmatiseur::lemmatiseT(std::string &t, bool alpha, bool cumVocibus,
                                    bool cumMorpho, bool nreconnu)
{
    bool cumColoribus = !_couleurs.empty();
    bool listeVide    = _hLem.empty();
    int  colPrec      = 0;
    int  formesConnues = 0;

    // Remove digits
    t = std::regex_replace(t, std::regex("\\d"), "");

    // Split at word boundaries → alternating [sep, word, sep, word, ..., sep]
    std::vector<std::string> lm = splitWordBoundary(t);

    std::vector<std::string> lsv;
    std::vector<std::string> nonReconnus;

    if (lm.size() < 2)
        return "";

    // Regex for end-of-sentence punctuation
    std::regex rePonct("[.!?;:]");

    // Process HTML entity sequences
    std::map<std::string, int> occCode;
    int i = 1;
    while (i < (int)lm.size()) {
        if ((endsWith(lm[i-1], "&") || endsWith(lm[i-1], "&#")) &&
            i + 1 < (int)lm.size() && startsWith(lm[i+1], ";"))
        {
            if (endsWith(lm[i], "gr")   || endsWith(lm[i], "aquo") ||
                endsWith(lm[i], "long") || endsWith(lm[i-1], "&#"))
            {
                // Greek or special: merge into separator
                lm[i-1] += lm[i];
                lm.erase(lm.begin() + i);
                lm[i-1] += lm[i];
                lm.erase(lm.begin() + i);
            }
            else if (endsWith(lm[i], "acute") || endsWith(lm[i], "grave") ||
                     endsWith(lm[i], "circ")  || endsWith(lm[i], "uml"))
            {
                lm[i] = lm[i].substr(0, 1);
                if (i + 1 < (int)lm.size() && lm[i+1] == ";") {
                    lm.erase(lm.begin() + i + 1);
                    if (i + 1 < (int)lm.size()) {
                        lm[i] += lm[i+1];
                        lm.erase(lm.begin() + i + 1);
                    }
                    if (lm[i-1] == "&") {
                        lm[i-2] += lm[i];
                        lm.erase(lm.begin() + i);
                        if (i - 1 >= 0 && (size_t)(i-1) < lm.size())
                            lm.erase(lm.begin() + i - 1);
                    } else {
                        if (!lm[i-1].empty()) lm[i-1].pop_back();
                        i += 2;
                    }
                } else {
                    if (i + 1 < (int)lm.size() && !lm[i+1].empty())
                        lm[i+1] = lm[i+1].substr(1);
                    if (lm[i-1] == "&") {
                        lm[i-2] += lm[i];
                        lm.erase(lm.begin() + i);
                        if (i - 1 >= 0 && (size_t)(i-1) < lm.size())
                            lm.erase(lm.begin() + i - 1);
                    } else {
                        if (!lm[i-1].empty()) lm[i-1].pop_back();
                        i += 2;
                    }
                }
            }
            else {
                occCode[lm[i]]++;
                i += 2;
            }
        }
        else {
            i += 2;
        }
    }
    for (auto &kv : occCode)
        std::cerr << kv.first << " " << kv.second << "\n";

    // Main lemmatisation loop (odd indices = words)
    for (int idx = 1; idx < (int)lm.size(); idx += 2) {
        const std::string &f = lm[idx];
        // Skip numeric tokens
        bool allDigit = !f.empty() &&
            std::all_of(f.begin(), f.end(),
                        [](unsigned char ch){ return isdigit(ch); });
        if (allDigit) continue;

        const std::string &sep = lm[idx - 1];
        bool debPhr = ((idx == 1 && lm.size() != 3) ||
                       std::regex_search(sep, rePonct));

        MapLem map = _lemCore->lemmatiseM(f, !_majPert || debPhr);

        if (map.empty()) {
            // Unrecognised form
            if (nreconnu) {
                nonReconnus.push_back(f + "\n");
            } else {
                if (_html)
                    lsv.push_back("<li style=\"color:blue;\">" + f + "</li>");
                else
                    lsv.push_back("> " + f + " \u00c9CHEC\n");
            }
            if (cumColoribus) {
                if (!listeVide) {
                    std::string lem = f;
                    lem = replaceAll(lem, "j", "i");
                    lem = replaceAll(lem, "v", "u");
                    lem = replaceAll(lem, "J", "I");
                    lem = replaceAll(lem, "V", "U");
                    if (_hLem.count(lem) > 0) {
                        _hLem[lem]++;
                        if (colPrec != 0) {
                            lm[idx] = "</span><span style=\"color:" +
                                      _couleurs[0] + "\">" + lm[idx];
                            colPrec = 0;
                        }
                    } else if (colPrec != 2) {
                        lm[idx] = "</span><span style=\"color:" +
                                  _couleurs[2] + "\">" + lm[idx];
                        colPrec = 2;
                    }
                } else if (colPrec != 2) {
                    lm[idx] = "</span><span style=\"color:" +
                              _couleurs[2] + "\">" + lm[idx];
                    colPrec = 2;
                }
            }
        } else {
            // Recognised
            bool connu = false;
            if (cumColoribus) {
                if (!listeVide) {
                    for (auto &kv : map)
                        if (_hLem.count(kv.first->cle()) > 0) {
                            connu = true;
                            _hLem[kv.first->cle()]++;
                        }
                }
                if (connu) {
                    formesConnues++;
                    if (colPrec != 0) {
                        lm[idx] = "</span><span style=\"color:" +
                                  _couleurs[0] + "\">" + lm[idx];
                        colPrec = 0;
                    }
                } else if (colPrec != 1) {
                    lm[idx] = "</span><span style=\"color:" +
                              _couleurs[1] + "\">" + lm[idx];
                    colPrec = 1;
                }
            }

            if (cumVocibus) {
                // With text forms
                std::string debMorph = "\n    . ";
                std::string sepMorph = "\n    . ";
                std::string finMorph = "";
                std::string debLem   = "  - ";
                std::string finLem   = "\n";
                if (_html) {
                    debMorph = "<ul><li>";
                    sepMorph = "</li><li>";
                    finMorph = "</li></ul>";
                    debLem   = "<li>";
                    finLem   = "</li>";
                }
                std::multimap<int, std::string> listeLem;
                for (auto &kv : map) {
                    Lemme *l = kv.first;
                    const std::vector<SLem> &slems = kv.second;
                    std::string lem = debLem + l->humain(_html, _cible, true);
                    int frMax = 0;
                    if (cumMorpho && !_lemCore->inv(l, map)) {
                        std::multimap<int, std::string> listeMorph;
                        for (const SLem &m : slems) {
                            int fr = _lemCore->fraction(_lemCore->tag(l, m.morpho));
                            if (fr > frMax) frMax = fr;
                            std::string entry = m.grq;
                            if (!m.sufq.empty())
                                entry += " + " + m.sufq;
                            entry += " " + _lemCore->morpho(m.morpho);
                            listeMorph.insert(std::make_pair(-fr, entry));
                        }
                        std::vector<std::string> lMorph = mm_vals_ordered(listeMorph);
                        lem += debMorph + join(lMorph, sepMorph) + finMorph;
                    } else {
                        for (const SLem &m : slems) {
                            int fr = _lemCore->fraction(_lemCore->tag(l, m.morpho));
                            if (fr > frMax) frMax = fr;
                        }
                    }
                    if (frMax == 0) frMax = 1024;
                    lem += finLem;
                    listeLem.insert(std::make_pair(-frMax * l->nbOcc(), lem));
                }
                std::vector<std::string> lLem = mm_vals_ordered(listeLem);
                std::string lin = join(lLem, "");
                if (_html) {
                    lin = "<li><h4>" + f + "</h4><ul>" + lin + "</ul></li>\n";
                } else {
                    lin = "* " + f + "\n" + lin;
                }
                if (!connu || listeVide) lsv.push_back(lin);
            } else {
                // Without text forms
                for (auto &kv : map) {
                    Lemme *l = kv.first;
                    const std::vector<SLem> &slems = kv.second;
                    std::string lin = l->humain(_html, _cible);
                    if (cumMorpho && !_lemCore->inv(l, map) && !alpha) {
                        std::ostringstream fl;
                        if (_html) {
                            fl << "<ul>";
                            for (const SLem &m : slems)
                                fl << "<li>" << m.grq << " "
                                   << _lemCore->morpho(m.morpho) << "</li>";
                            fl << "</ul>\n";
                        } else {
                            for (const SLem &m : slems)
                                fl << "\n    . " << m.grq << " "
                                   << _lemCore->morpho(m.morpho);
                        }
                        lin += fl.str();
                    }
                    if (_html) {
                        lin = "<li>" + lin + "</li>";
                    } else {
                        lin = "* " + lin + "\n";
                    }
                    if (!connu || listeVide) lsv.push_back(lin);
                }
            }
        }
    }  // fin de boucle de lemmatisation pour chaque mot

    if (alpha) {
        removeDuplicates(lsv);
        std::sort(lsv.begin(), lsv.end(), Ch::sort_i);
    }

    std::vector<std::string> lRet = lsv;
    if (_html) {
        lRet.insert(lRet.begin(), "<ul>");
        lRet.push_back("</ul>\n");
    }

    if (nreconnu && !nonReconnus.empty()) {
        removeDuplicates(nonReconnus);
        std::string nl = _html ? "<br/>" : "";
        if (alpha)
            std::sort(nonReconnus.begin(), nonReconnus.end(), Ch::sort_i);
        int tot = ((int)lm.size() - 1) / 2;
        std::ostringstream oss;
        oss << "--- " << nonReconnus.size() << "/" << tot << " ("
            << (((int)nonReconnus.size() * 200 + tot) / tot) / 2
            << " %) FORMES NON RECONNUES ---" << nl << "\n";
        lRet.push_back(oss.str() + nl);
        for (const auto &nr : nonReconnus)
            lRet.push_back(nr + nl);
    }

    if (cumColoribus) {
        if (!lm.empty()) lm[0] += "<span style=\"color:" + _couleurs[0] + "\">";
        if (!lm.empty()) lm.back() += "</span>";
        t = join(lm, "");
        t = replaceAll(t, "\n", "<br/>\n");
        if (!listeVide) {
            std::ostringstream stats;
            int total = (int)(lm.size() / 2);
            int denom = (int)lm.size() - 1;
            stats << "<strong>Formes connues : " << formesConnues
                  << " sur " << total
                  << " (" << (denom > 0 ? (200 * formesConnues) / denom : 0)
                  << "%)<br/></strong>";
            lRet.insert(lRet.begin(), stats.str());
        }
    }

    return join(lRet, "");
}

/**
 * \fn QString Lemmatiseur::lemmatiseFichier (QString f,
 *								  bool alpha,
 *								  bool cumVocibus,
 *								  bool cumMorpho,
 *								  bool nreconnu)
 * \brief Applique lemmatiseT sur le contenu du fichier
 *        f et renvoie le résultat. Les paramètres sont
 *        les mêmes que ceux de lemmatiseT.
 */
// ---------------------------------------------------------------------------
// lemmatiseFichier
// ---------------------------------------------------------------------------
std::string Lemmatiseur::lemmatiseFichier(const std::string &f, bool alpha,
                                          bool cumVocibus, bool cumMorpho,
                                          bool nreconnu)
{
    std::ifstream fichier(f);
    if (!fichier.is_open()) return "";
    std::ostringstream oss;
    oss << fichier.rdbuf();
    std::string texte = oss.str();
    return lemmatiseT(texte, alpha, cumVocibus, cumMorpho, nreconnu);
}

// ---------------------------------------------------------------------------
// verbaCognita
// ---------------------------------------------------------------------------
void Lemmatiseur::verbaCognita(const std::string &fichier, bool vb)
{
    _hLem.clear();
    _couleurs.clear();
    if (vb) {
        _couleurs.push_back("#00A000"); // known: green
        _couleurs.push_back("#000000"); // recognised: black
        _couleurs.push_back("#A00000"); // unrecognised: red
    }
    if (!vb || fichier.empty()) return;

    std::ifstream in(fichier);
    if (!in.is_open()) return;

    std::string ligne;
    // Skip comment and empty lines at start
    while (std::getline(in, ligne)) {
        if (!startsWith(ligne, "!") && !ligne.empty()) break;
    }
    // Read optional colour overrides (#RRGGBB lines)
    int i = 0;
    while (startsWith(ligne, "#") && !in.eof()) {
        if (i < 3 && ligne.size() == 7) _couleurs[i] = ligne;
        i++;
        if (!std::getline(in, ligne)) break;
    }
    // Read known lemma forms
    do {
        if (!startsWith(ligne, "!") && !ligne.empty()) {
            MapLem item = _lemCore->lemmatiseM(ligne, false, false);
            for (auto &kv : item)
                _hLem[kv.first->cle()] = 0;
        }
    } while (std::getline(in, ligne));
}

// ---------------------------------------------------------------------------
// verbaOut
// ---------------------------------------------------------------------------
void Lemmatiseur::verbaOut(const std::string &fichier)
{
    if (_hLem.empty()) return;
    std::ofstream file(fichier);
    if (file.is_open())
        for (const auto &kv : _hLem)
            file << kv.first << "\t" << kv.second << "\n";
}

// ---------------------------------------------------------------------------
// Option accessors
// ---------------------------------------------------------------------------
/**
 * \fn bool Lemmatiseur::optAlpha()
 * \brief Accesseur de l'option alpha, qui
 *        permet de fournir par défaut des résultats dans
 *        l'ordre alphabétique.
 */
bool        Lemmatiseur::optAlpha()  { return _alpha;   }
/**
 * \fn bool Lemmatiseur::optHtml()
 * \brief Accesseur de l'option html, qui
 *        permet de renvoyer les résultats au format html.
 */
bool        Lemmatiseur::optHtml()   { return _html;    }
/**
 * \fn bool Lemmatiseur::optFormeT()
 * \brief Accesseur de l'option formeT,
 *        qui donne en tête de lemmatisation
 *        la forme qui a été analysée.
 */
bool        Lemmatiseur::optFormeT() { return _formeT;  }
/**
 * \fn bool Lemmatiseur::optMajPert()
 * \brief Accesseur de l'option majPert,
 *        qui permet de tenir compte des majuscules
 *        dans la lemmatisation.
 */
bool        Lemmatiseur::optMajPert(){ return _majPert; }
/**
 * \fn bool Lemmatiseur::optMorpho()
 * \brief Accesseur de l'option morpho,
 *        qui donne l'analyse morphologique
 *        des formes lemmatisées.
 */
bool        Lemmatiseur::optMorpho() { return _morpho;  }
bool        Lemmatiseur::optNonRec() { return _nonRec;  }
/**
 * \fn QString Lemmatiseur::cible()
 * \brief Renvoie la langue cible dans sa forme
 *        abrégée (fr, en, de, it, etc.).
 */
std::string Lemmatiseur::cible()     { return _cible;   }

// ---------------------------------------------------------------------------
// Option mutators
// ---------------------------------------------------------------------------
/**
 * \fn void Lemmatiseur::setAlpha (bool a)
 * \brief Modificateur de l'option alpha.
 */
void Lemmatiseur::setAlpha(bool a)              { _alpha = a; }
/**
 * \fn void Lemmatiseur::setHtml (bool h)
 * \brief Modificateur de l'option html.
 */
void Lemmatiseur::setHtml(bool h)               { _html = h; }
/**
 * \fn void Lemmatiseur::setFormeT (bool f)
 * \brief Modificateur de l'option formeT.
 */
void Lemmatiseur::setFormeT(bool f)             { _formeT = f; }
/**
 * \fn void Lemmatiseur::setMajPert (bool mp)
 * \brief Modificateur de l'option majpert.
 */
void Lemmatiseur::setMajPert(bool mp)           { _majPert = mp; }
/**
 * \fn void Lemmatiseur::setMorpho (bool m)
 * \brief Modificateur de l'option morpho.
 */
void Lemmatiseur::setMorpho(bool m)             { _morpho = m; }
void Lemmatiseur::setNonRec(bool n)             { _nonRec = n; }

/**
 * \fn void Lemmatiseur::setCible(QString c)
 * \brief Permet de changer la langue cible.
 */
void Lemmatiseur::setCible(const std::string &c)
{
    _cible = c;
    _lemCore->setCible(c);
}
