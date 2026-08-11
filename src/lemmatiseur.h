/*      lemmatiseur.h
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
    // Créateur de la classe
    std::vector<std::string> frequences(const std::string &txt);
    std::vector<std::string> lemmatiseF(const std::string &f, bool deb);
    std::string lemmatiseFichier(const std::string &f, bool alpha = false,
                                 bool cumVocibus = false, bool cumMorpho = false,
                                 bool nreconnu = true);
    // lemmatiseT lemmatise un texte
    std::string lemmatiseT(std::string &t);
    std::string lemmatiseT(std::string &t, bool alpha, bool cumVocibus = false,
                           bool cumMorpho = false, bool nreconnu = false);

    void verbaOut(const std::string &fichier);    // Connaître l'usage des mots connus
    void verbaCognita(const std::string &fichier, bool vb = false); // Coloriser le texte avec les mots connus

    // accesseurs d'options
    bool optAlpha();
    bool optHtml();
    bool optFormeT();
    bool optMajPert();
    bool optMorpho();
    bool optNonRec();
    std::string cible();

    // modificateurs d'options
    void setAlpha(bool a);
    void setCible(const std::string &c);
    void setHtml(bool h);
    void setFormeT(bool f);
    void setMajPert(bool mp);
    void setMorpho(bool m);
    void setNonRec(bool n);

private:
    LemCore * _lemCore;
    std::string _resDir;
    std::unordered_map<std::string, int> _hLem;
    std::vector<std::string> _couleurs;
    // options
    bool _alpha;
    bool _formeT;
    bool _html;
    bool _majPert;
    bool _morpho;
    bool _nonRec;
    std::string _cible;  // langue courante, 2 caractères ou plus
};

#endif
