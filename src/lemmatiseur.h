/*      lemmatiseur.h  */

#ifndef LEMMATISEUR_H
#define LEMMATISEUR_H

#include <string>
#include <vector>
#include <unordered_map>
#include "ch.h"
#include "lemCore.h"
#include "string_utils.h"

class Lemmatiseur
{
public:
    Lemmatiseur(LemCore *l = nullptr, const std::string &cible = "",
                const std::string &resDir = "");

    std::vector<std::string> frequences(const std::string &txt);
    std::vector<std::string> lemmatiseF(const std::string &f, bool deb);
    std::string lemmatiseFichier(const std::string &f, bool alpha = false,
                                 bool cumVocibus = false, bool cumMorpho = false,
                                 bool nreconnu = true);
    std::string lemmatiseT(std::string &t);
    std::string lemmatiseT(std::string &t, bool alpha, bool cumVocibus = false,
                           bool cumMorpho = false, bool nreconnu = false);

    void verbaOut(const std::string &fichier);
    void verbaCognita(const std::string &fichier, bool vb = false);

    bool optAlpha();
    bool optHtml();
    bool optFormeT();
    bool optMajPert();
    bool optMorpho();
    bool optNonRec();
    std::string cible();

    void setAlpha(bool a);
    void setCible(const std::string &c);
    void setHtml(bool h);
    void setFormeT(bool f);
    void setMajPert(bool mp);
    void setMorpho(bool m);
    void setNonRec(bool n);

private:
    LemCore *_lemCore;
    std::string _resDir;
    std::unordered_map<std::string, int> _hLem;
    std::vector<std::string> _couleurs;
    bool _alpha;
    bool _formeT;
    bool _html;
    bool _majPert;
    bool _morpho;
    bool _nonRec;
    std::string _cible;
};

#endif
