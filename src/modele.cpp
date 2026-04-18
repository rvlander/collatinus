/*          modele.cpp
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

/**
 * \file modele.cpp
 * \brief définit les classes Desinence et Modele
 *
 * Ces classes font partie des couches profondes utilisées par
 * le noyau de lemmatisation, LemCore.
 */

#include "modele.h"
#include "lemCore.h"
#include <regex>
#include <algorithm>

///////////////
// DESINENCE //
///////////////

/**
 * \fn Desinence::Desinence (QString d, int morph, int nr, Modele *parent)
 * \brief Constructeur de la classe Desinence.
 * \param d est la graphie avec quantités
 * \param morph est le numéro de morphologie (dans la liste de la classe
 * LemCore)
 * \param nr est le numéro de radical accepté par la désinence
 * \param parent est
 *        un pointeur sur le modèle qui utilise cette
 *        désinence.
 *
 * Un paradigme, Modele, est associé à un (ou plusieurs) radical(aux), Radical,
 * et une collection de désinences.
 * Chaque Desinence est donnée par sa graphie (avec quantité)
 * mais doit aussi contenir des informations cruciales :
 *  * l'analyse morphologique à laquelle elle est associée
 *  * le numéro du radical auquel elle peut se coller
 *  * le modèle.
 */
Desinence::Desinence(const std::string &d, int morph, int nr, Modele *parent)
{
    int der = -1;
    if (!d.empty()) {
        char last = d.back();
        if (isdigit((unsigned char)last)) der = last - '0';
    }
    std::string ds = d;
    if (der > 0) {
        _rarete = der;
        ds.pop_back();
    } else {
        _rarete = 10;
    }
    // '-' est la désinence zéro
    if (ds == "-") ds = "";
    _grq = ds;
    _gr = Ch::atone(_grq);
    _morpho = morph;
    _numR = nr;
    _modele = parent;
}

/**
 * \fn QString Desinence::gr ()
 * \brief Graphie de la désinence, ramiste et sans quantités.
 */
std::string Desinence::gr()      { return _gr; }

/**
 * \fn QString Desinence::grq ()
 * \brief Graphie ramiste avec quantités.
 */
std::string Desinence::grq()     { return _grq; }

/**
 * \fn Modele* Desinence::modele ()
 * \brief Modèle de la désinence.
 */
Modele *Desinence::modele()      { return _modele; }

/**
 * \fn int Desinence::morphoNum ()
 * \brief Numéro de morpho de la désinence.
 */
int Desinence::morphoNum()       { return _morpho; }

/**
 * \fn int Desinence::numRad ()
 * \brief Numéro de radical de la désinence.
 */
int Desinence::numRad()          { return _numR; }

/**
 * @brief accesseur de la rareté
 * @return la valeur de Desinence::_rarete
 *
 * La rareté est un paramètre qui permet de séparer l'utilisation
 * d'une désinence en analyse et en flexion.
 * En effet, certains paradigmes admettent des désinences rares ou archaïques
 * qu'il faut reconnaître quand on les rencontre dans un texte.
 * En revanche, les tableaux de flexion sont plutôt destinés aux débutants.
 * Il ne serait donc pas opportun d'encombrer leur mémoire de formes
 * qu'ils ont peu de chance de rencontrer (dans l'immédiat).
 *
 * @note Le nom semble mal choisi car les désinences usuelles
 * sont associées à la valeur @c 10. Les plus rares ont @c 0.
 */
int Desinence::rarete()          { return _rarete; }

/**
 * \brief Attribue un modèle à la désinence.
 *
 * @deprecated Semble inutilisé.
 * La valeur de Desinence::_modele est définie lors de la création.
 */
void Desinence::setModele(Modele *m) { _modele = m; }

////////////
// MODELE //
////////////

/**
 * \fn Modele::Modele (QStringList ll, LemCore *parent)
 * \brief Constructeur de la classe modèle.
 * \param ll : liste de chaines de caractères
 * \param parent : pointeur vers le noyau de lemmatisation, LemCore
 *
 * Chaque item
 *        de la liste \a ll est constitué de champs séparés par
 *        le caractère <tt>':'</tt>. Le premier champ est un mot clé
 * (voir Modele::cles).
 * Pour le format du
 *        fichier <tt>data/modeles.la</tt>, consulter la documentation
 *        utilisateur.
 */

const std::vector<std::string> Modele::cles = {
    "modele",  // 0
    "pere",    // 1
    "des",     // 2
    "des+",    // 3
    "R",       // 4
    "abs",     // 5
    "suf",     // 6
    "sufd",    // 7
    "abs+",    // 8
    "pos",     // 9
    "nbr"      // 10
};

Modele::Modele(std::vector<std::string> ll, LemCore *parent)
{
    _lemCore = parent;
    _pere = nullptr;
    _pos = '\0';
    _nbr = 0;
    std::multimap<std::string, int> msuff;

    // Regex for variable substitution: [:;](\w*)\+?(\$\w+)
    std::regex re("[:;]([\\w]*)\\+{0,1}(\\$\\w+)");

    for (auto &line : ll) {
        std::string l = line;
        // Substitute variables
        std::smatch m;
        while (std::regex_search(l, m, re)) {
            std::string v   = m[2].str();      // e.g. "$VAR"
            std::string var = _lemCore->variable(v);
            std::string pre = m[1].str();
            if (!pre.empty())
                var = replaceAll(var, ";", ";" + pre);
            l = replaceAll(l, v, var);
        }

        std::vector<std::string> eclats = split(simplified(l), ':');
        if (eclats.empty()) continue;

        auto it = std::find(cles.begin(), cles.end(), eclats.front());
        int p = (it != cles.end()) ? (int)(it - cles.begin()) : -1;

        switch (p) {
        case 0: // modele
            _gr = eclats.at(1);
            break;
        case 1: // pere
            _pere = parent->modele(eclats.at(1));
            break;
        case 2: // des
        case 3: // des+
        {
            std::vector<int> li = listeI(eclats.at(1));
            int r = toInt(eclats.at(2));
            std::vector<std::string> ld = split(eclats.at(3), ';');
            for (size_t i = 0; i < li.size(); ++i) {
                std::vector<std::string> ldd;
                if (i < ld.size())
                    ldd = split(ld.at(i), ',');
                else
                    ldd = split(ld.back(), ',');
                for (auto &g : ldd) {
                    Desinence *nd = new Desinence(g, li.at(i), r, this);
                    _desinences.insert({nd->morphoNum(), nd});
                    _lemCore->ajDesinence(nd);
                }
            }
            // des+ : also inherit from father for those morphos
            if (p == 3 && _pere != nullptr) {
                for (int i : li) {
                    for (Desinence *dp : _pere->desinences(i)) {
                        Desinence *dh = clone(dp);
                        _desinences.insert({i, dh});
                        _lemCore->ajDesinence(dh);
                    }
                }
            }
            break;
        }
        case 4: // R:n:gen
        {
            int nr = toInt(eclats.at(1));
            _genRadicaux[nr] = eclats.at(2);
            break;
        }
        case 8: // abs+
        {
            auto extra = listeI(eclats.at(1));
            _absents.insert(_absents.end(), extra.begin(), extra.end());
            break;
        }
        case 5: // abs
            _absents = listeI(eclats.at(1));
            break;
        case 6: // suf
        {
            std::vector<int> lsuf = listeI(eclats.at(1));
            std::string gr = eclats.at(2);
            for (int mi : lsuf)
                msuff.insert({gr, mi});
            break;
        }
        case 7: // sufd
        {
            if (_pere != nullptr) {
                _suf = eclats.at(1);
                for (Desinence *d : _pere->desinences()) {
                    if (std::find(_absents.begin(), _absents.end(), d->morphoNum()) != _absents.end())
                        continue;
                    std::string nd = d->grq();
                    Ch::allonge(&nd);
                    Desinence *dsuf = new Desinence(nd + _suf, d->morphoNum(), d->numRad(), this);
                    _desinences.insert({dsuf->morphoNum(), dsuf});
                    _lemCore->ajDesinence(dsuf);
                }
            }
            break;
        }
        case 9: // pos
            if (!eclats.at(1).empty()) _pos = eclats.at(1)[0];
            break;
        case 10: // nbr : introduit le 19 décembre 2021
            _nbr = toInt(eclats.at(1));
            // C'est le nombre d'occurrences du modèle dans le corpus du LASLA.
            break;
        default:
            break;
        }
    }  // fin de l'interprétation des lignes

    // père
    // Inherit from father
    if (_pere != nullptr) {
        if (_pos == '\0') _pos = _pere->pos();
        for (int mi : _pere->morphos()) {
            if (deja(mi)) continue;
            for (Desinence *d : _pere->desinences(mi)) {
                if (std::find(_absents.begin(), _absents.end(), d->morphoNum()) != _absents.end())
                    continue;
                Desinence *dh = clone(d);
                _desinences.insert({dh->morphoNum(), dh});
                _lemCore->ajDesinence(dh);
            }
        }
        // inherit radical generators
        for (auto &kv : _desinences) {
            int nr = kv.second->numRad();
            if (_genRadicaux.find(nr) == _genRadicaux.end()) {
                std::string gen = _pere->genRadical(nr);
                _genRadicaux[nr] = gen;
            }
        }
        _absents = _pere->absents();
    }

    // Generate suffixed desinences
    std::vector<Desinence *> ldsuf;
    std::vector<std::string> clefsSuff = mm_unique_keys(msuff);
    for (auto &suff : clefsSuff) {
        std::vector<int> vals = mm_values(msuff, suff);
        for (auto &kv : _desinences) {
            Desinence *d = kv.second;
            if (std::find(vals.begin(), vals.end(), d->morphoNum()) != vals.end()) {
                std::string gq = d->grq();
                if (gq == "-") gq.clear();
                gq += suff;
                Desinence *dsuf = new Desinence(gq, d->morphoNum(), d->numRad(), this);
                ldsuf.push_back(dsuf);
            }
        }
    }
    for (Desinence *dsuf : ldsuf) {
        _desinences.insert({dsuf->morphoNum(), dsuf});
        _lemCore->ajDesinence(dsuf);
    }
}

/**
 * \fn bool Modele::absent (int a)
 * \brief Renvoie true si la morpho de rang a
 *        n'existe pas dans le modèle.
 *
 * Certains substantifs n'ont pas de singulier,
 *        certains verbes n'ont pas de passif.
 * Pour afficher correctement la flexion, il faut savoir
 * quelles analyses morphologiques ne sont pas utilisées
 * pour ce modèle.
 */
bool Modele::absent(int a)
{
    return std::find(_absents.begin(), _absents.end(), a) != _absents.end();
}

/**
 * \fn QList<int> Modele::absents ()
 * \brief Retourne la liste des numéros des morphos absentes.
 */
std::vector<int> Modele::absents() { return _absents; }

/**
 * \fn QList<int> Modele::clesR ()
 * \brief Liste des numéros de radicaux utilisés, et
 *        rangés dans la map _genRadicaux.
 */
std::vector<int> Modele::clesR()
{
    return map_keys(_genRadicaux);
}

/**
 * \fn Desinence* Modele::clone (Desinence *d)
 * \brief Crée une Désinence copiée sur la désinence d.
 */
Desinence *Modele::clone(Desinence *d)
{
    return new Desinence(d->grq() + _suf, d->morphoNum(), d->numRad(), this);
}

/**
 * \fn bool Modele::deja (int m)
 * \brief Renvoie true si le modèle a déjà une désinence avec la morpho de rang m.
 *
 * Cette fonction permet de savoir s'il faut aller chercher la désinence
 *        de morpho m chez le modèle père.
 */
bool Modele::deja(int m)
{
    return _desinences.count(m) > 0;
}

/**
 * \fn QList<Desinence*> Modele::desinences (int d)
 * \brief Renvoie la liste des désinence de morpho d du modèle.
 */
std::vector<Desinence *> Modele::desinences(int d)
{
    return mm_values(_desinences, d);
}

/**
 * \fn QList<Desinence*> Modele::desinences ()
 * \brief Renvoie toutes les désinences du modèle.
 */
std::vector<Desinence *> Modele::desinences()
{
    return mm_all_values(_desinences);
}

/**
 * \fn bool Modele::estUn (QString m)
 * \brief Renvoie true si le modèle se nomme m, ou si
 *        l'un de ses ancêtres se nomme m
 */
bool Modele::estUn(const std::string &m)
{
    if (_gr == m) return true;
    if (_pere == nullptr) return false;
    return _pere->estUn(m);
}

/**
 * \fn QString Modele::gr ()
 * \brief Nom du modèle.
 */
std::string Modele::gr() { return _gr; }

/**
 * \fn QList<int> Modele::listeI (QString l)
 * \brief conversion d'une chaine de caractère en liste d'entiers
 * \param l : la chaine initiale
 * \return la liste des entiers contenus dans la chaine
 *
 * Fonction importante permettant de renvoyer
 *        une liste d'entiers à partir d'une chaîne @a l.
 *        La chaîne est une liste de sections séparées
 *        par des virgules. Une section peut être soit
 *        un entier, soit un intervalle d'entiers. On
 *        donne alors les limites inférieure et supérieure
 *        de l'intervale, séparées par le caractère '-'.
 * Les limites sont incluses.
 *
 * Nombreux exemples d'intervalles dans le fichier
 *        <tt>data/modeles.la</tt>.
 */
std::vector<int> Modele::listeI(const std::string &l)
{
    std::vector<int> result;
    std::vector<std::string> lvirg = split(l, ',');
    for (auto &virg : lvirg) {
        if (contains(virg, '-') && !virg.empty() && virg[0] != '-') {
            int deb = toInt(section(virg, '-', 0, 0));
            int fin = toInt(section(virg, '-', 1, 1));
            for (int i = deb; i <= fin; ++i) result.push_back(i);
        } else {
            result.push_back(toInt(virg));
        }
    }
    return result;
}

/**
 * \fn QList<int> Modele::morphos ()
 * \brief Liste des numéros des désinences définies par le modèle.
 */
std::vector<int> Modele::morphos()
{
    return mm_unique_keys(_desinences);
}

/**
 * \fn QChar Modele::pos()
 * \brief Retourne la catégorie du modèle, en utilisant
 *        les ancêtres du modèle.
 */
char Modele::pos()
{
    if (_pos == '\0') return 'd';
    return _pos;
}

/**
 * \fn QString Modele::genRadical (int r)
 * \brief générateur d'un radical
 * \param r est le numéro du radical.
 * \return une chaîne permettant de calculer un radical à partir
 *        de la forme canonique d'un lemme.
 */
std::string Modele::genRadical(int r)
{
    auto it = _genRadicaux.find(r);
    return it != _genRadicaux.end() ? it->second : "";
}

/**
 * @brief Accesseur du nombre d'occurrences du modèle dans le corpus du LASLA.
 * @return la valeur de Modele::_nbr
 */
int Modele::nbr() { return _nbr; }
