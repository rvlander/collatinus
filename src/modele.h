/*           modele.h
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

#ifndef MODELE_H
#define MODELE_H

#include <string>
#include <vector>
#include <map>
#include "ch.h"
#include "string_utils.h"

class LemCore;
class Modele;

/**
 * @brief La classe Desinence décrit les désinences associées aux modèles
 */
class Desinence
{
   private:
    std::string _gr;  /*!< voir Desinence::gr */
    std::string _grq; /*!< voir Desinence::grq */
    int _morpho;      /*!< voir Desinence::morphoNum */
    Modele *_modele;  /*!< voir Desinence::modele */
    int _numR;        /*!< voir Desinence::numRad */
    int _rarete;      /*!< voir Desinence::rarete */

   public:
    Desinence(const std::string &d, int morph, int nr, Modele *parent = nullptr);
    std::string gr();
    std::string grq();
    int rarete();
    Modele *modele();
    int morphoNum();
    int numRad();
    void setModele(Modele *m);
};

/**
 * @brief La classe Modele contient les désinences associées aux paradigmes de flexion
 */
class Modele
{
   private:
    std::vector<int> _absents;       /*!< Liste des morphos absentes du modèle. */
    static const std::vector<std::string> cles; /*!< ensemble des clefs utilisées dans la descriptions des modèles */
    std::multimap<int, Desinence *> _desinences; /*!< Liste des désinences du modèle. */
    std::map<int, std::string> _genRadicaux; /*!< Générateurs des radicaux du modèle. */
    std::string _gr;       /*!< Nom du modèle. */
    LemCore *_lemCore;     /*!< Un pointeur vers le noyau de lemmatisation. */
    Modele *_pere;         /*!< Un pointeur vers le père du modèle. */
    char _pos;             /*!< POS associé au modèle. */
    std::string _suf;      /*!< Suffixe à ajouter aux désinences du père. */
    int _nbr;              /*!< Le nombre d'occurrences du modèle dans le corpus du LASLA. */

   public:
    Modele(std::vector<std::string> ll, LemCore *parent = nullptr);
    bool absent(int a);
    std::vector<int> absents();
    std::vector<int> clesR();
    Desinence *clone(Desinence *d);
    bool deja(int m);
    std::vector<Desinence *> desinences(int d);
    std::vector<Desinence *> desinences();
    bool estUn(const std::string &m);
    std::string genRadical(int r);
    std::string gr();
    static std::vector<int> listeI(const std::string &l);
    std::vector<int> morphos();
    char pos();
    int nbr();
};

#endif
