/*      lemme.cpp
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
 * \file lemme.cpp
 * \brief définit les classes Radical et Lemme
 *
 * Ces classes font partie des couches profondes utilisées par
 * le noyau de lemmatisation, LemCore.
 */

#include "lemme.h"
#include "irregs.h"
#include "lemCore.h"
#include "modele.h"
#include <sstream>
#include <regex>
#include <algorithm>

LemCore *Lemme::_lemCore = nullptr;
// J'ai défini _lemCore comme variable statique dans Lemme (lemme.h).
// Initialement, je lui donne la valeur NULL (ici, dans lemme.cpp).
// Quand j'aurai créé le vrai LemCore, je fixerai la bonne valeur (dans LemCore::LemCore).

/////////////
// RADICAL //
/////////////

/**
 * \fn Radical::Radical (QString g, int n, QObject *parent)
 * \brief Créateur de la classe Radical. g est la forme
 *        canonique avec ses quantités, n est le numéro du radical
 */
Radical::Radical(const std::string &g, int n, Lemme *parent)
{
    _lemme  = parent;
    _grq    = Ch::communes(g);
    _gr     = Ch::atone(g);
    _numero = n;
}

/**
 * \fn QString Radical::gr ()
 * \brief Renvoie la graphie du radical
 *        dépourvue de diacritiques.
 */
std::string Radical::gr()       { return _gr; }

/**
 * \fn QString Radical::grq ()
 * \brief Renvoie la graphie du radical
 *        pourvue de ses diacritiques.
 */
std::string Radical::grq() const { return _grq; }

/**
 * \fn Lemme* Radical::lemme ()
 * \brief Le lemme auquel appartient le radical.
 */
Lemme *Radical::lemme()         { return _lemme; }
/**
 * \fn Modele* Radical::modele ()
 * \brief Le modèle de flexion du radical
 */
Modele *Radical::modele()       { return _lemme->modele(); }
/**
 * \fn int Radical::numRad ()
 * \brief Le numéro du radical.
 */
int Radical::numRad()           { return _numero; }

///////////
// LEMME //
///////////

/**
 * \fn Lemme::Lemme (QString linea, QObject *parent)
 * \brief Constructeur de la classe Lemme à partir de la
 *        ligne linea. *parent est le noyau de lemmatisation (classe LemCore).
 */
Lemme::Lemme(const std::string &linea, int origin)
{
    // cădo|lego|cĕcĭd|cās|is, ere, cecidi, casum|687
    std::vector<std::string> eclats = split(linea, '|');
    std::vector<std::string> lg     = split(eclats.at(0), '=');
    _nh     = 1;
    _cle    = Ch::atone(Ch::deramise(lg.at(0)));
    _grd    = oteNh(lg.at(0), _nh);
    _grq    = (lg.size() == 1) ? _grd : lg.at(1);
    _gr     = Ch::atone(section(_grq, ',', 0, 0));
    _grModele = eclats.at(1);
    _modele   = _lemCore->modele(_grModele);
    _hyphen   = "";
    _origin   = origin;
    _nbOcc    = 1;

    if ((int)eclats.size() < 6) {
        // mal formée — continue anyway
    }

    // Radicaux champs 2 et 3
    for (int i = 2; i < 4; ++i) {
        if (i >= (int)eclats.size()) break;
        if (!eclats.at(i).empty()) {
            std::vector<std::string> lrad = split(eclats.at(i), ',');
            for (auto &rad : lrad)
                _radicaux[i-1].push_back(new Radical(rad, i-1, this));
        }
    }
    _lemCore->ajRadicaux(this);

    _indMorph = (eclats.size() > 4) ? eclats.at(4) : "";

    // renvoi
    std::regex c("cf\\.\\s(\\w+)$");
    std::smatch sm;
    if (std::regex_search(_indMorph, sm, c))
        _renvoi = sm[1].str();
    else
        _renvoi = "";

    // POS
    _pos.clear();
    if (contains(_indMorph, "adj."))  _pos += 'a';
    if (contains(_indMorph, "conj"))  _pos += 'c';
    if (contains(_indMorph, "excl.")) _pos += 'e';
    if (contains(_indMorph, "interj")) _pos += 'i';
    if (contains(_indMorph, "num."))  _pos += 'm';
    if (contains(_indMorph, "pron.")) _pos += 'p';
    if (contains(_indMorph, "prép"))  _pos += 'r';
    if (contains(_indMorph, "adv"))   _pos += 'd';
    if (contains(_indMorph, " nom ") || contains(_indMorph, "npr.")) _pos += 'n';
    if (_pos.empty()) {
        _pos += _modele->pos();
        if (_pos == "d" && !_renvoi.empty()) _pos = "";
    }

    if (eclats.size() > 5)
        _nbOcc = toInt(eclats.at(5));
}

void Lemme::setLemCore(LemCore *l)
{
    _lemCore = l;
}

/**
 * \fn void Lemme::ajIrreg (Irreg *irr)
 * \brief Ajoute au lemme l'obet irr, qui représente
 *        une forme irrégulière. Lorsque les formes irrégulières
 *        sont trop nombreuses, ou lorsque plusieurs lemmes
 *        ont des formes analogues, mieux vaut ajouter un modèle
 *        dans data/modeles.la.
 */
void Lemme::ajIrreg(Irreg *irr)
{
    _irregs.push_back(irr);
    // ajouter les numéros de morpho à la liste
    // des morphos irrégulières du lemme :
    if (irr->exclusif()) {
        auto mv = irr->morphos();
        _morphosIrrExcl.insert(_morphosIrrExcl.end(), mv.begin(), mv.end());
    }
}

/**
 * \fn void Lemme::ajNombre(int n)
 * \brief Ajoute l'entier n au nombre d'occurrences du lemme.
 *
 *      Un lemme de Collatinus peut être associé à plusieurs lemmes du LASLA.
 *      D'où la somme.
 */
void Lemme::ajNombre(int n) { _nbOcc += n; }

/**
 * \fn void Lemme::ajRadical (int i, Radical* r)
 * \brief Ajoute le radical r de numéro i à la map des
 *        radicaux du lemme.
 */
void Lemme::ajRadical(int i, Radical *r) { _radicaux[i].push_back(r); }

/**
 * \fn void Lemme::ajTrad (QString t, QString l)
 * \brief ajoute la traduction t de langue l à
 *        la map des traductions du lemme.
 */
void Lemme::ajTrad(const std::string &t, const std::string &l) { _traduction[l] = t; }

/**
 * \fn QString Lemme::ambrogio()
 * \brief Renvoie dans une chaîne un résumé
 *        de la traduction du lemme dans toutes les
 *        langues cibles disponibles.
 */
std::string Lemme::ambrogio()
{
    std::ostringstream ss;
    ss << "<hr/>" << humain() << "<br/>";
    ss << "<table>";
    for (auto &kv : _traduction) {
        const std::string &lang  = kv.first;
        const std::string &trad  = kv.second;
        std::string langue = map_value(_lemCore->cibles(), lang);
        if (!trad.empty())
            ss << "<tr><td>- " << langue << "</td><td>&nbsp;" << trad << "</td></tr>\n";
    }
    ss << "</table>";
    return ss.str();
}

/**
 * \fn QString Lemme::cle ()
 * \brief Renvoie la clé sous laquel le
 *        lemme est enregistré dans le lemmatiseur parent.
 */
std::string Lemme::cle() { return _cle; }

/**
 * \fn QList<int> Lemme::clesR ()
 * \brief Retourne toutes les clés (formes non-ramistes
 *        sans diacritiques) de la map des radicaux du lemme.
 */
std::vector<int> Lemme::clesR()
{
    return map_keys(_radicaux);
}

/**
 * \fn bool Lemme::estIrregExcl (int nm)
 * \param nm : numéro de morpho
 * \brief Renvoie vrai si la forme irrégulière
 *        avec le n° nm remplace celle construite
 *        sur le radical, faux si la
 *        forme régulière existe aussi.
 */
bool Lemme::estIrregExcl(int nm)
{
    return std::find(_morphosIrrExcl.begin(), _morphosIrrExcl.end(), nm) != _morphosIrrExcl.end();
}

/**
 * @brief Le genre du lemme
 * @return : le (ou les) genre(s) du mot.
 *
 * Cette routine convertit les indications morphologiques,
 * données dans le fichier lemmes.la,
 * pour exprimer le genre du mot dans la langue courante.
 *
 * Introduite pour assurer l'accord entre un nom et son adjectif.
 *
 */
std::string Lemme::genre()
{
    std::string g;
    if (contains(_indMorph, " m.")) g += " " + _lemCore->genre(0);
    // J'ai ainsi le genre dans la langue choisie.
    if (contains(_indMorph, " f.")) g += " " + _lemCore->genre(1);
    if (contains(_indMorph, " n.")) g += " " + _lemCore->genre(2);
    g = trim(g);
    if (!_renvoi.empty() && g.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        if (lr != nullptr) return lr->genre();
    }
    return g;
}

/**
 * \brief Retourne la graphie ramiste du lemme sans diacritiques.
 * \return _gr;
 */
std::string Lemme::gr()  { return _gr; }

/**
 * \fn QString Lemme::grq ()
 * \brief Retourne la graphie ramiste du lemme avec diacritiques.
 */
std::string Lemme::grq() { return _grq; }

/**
 * \fn QString Lemme::grModele ()
 * \brief Retourne la graphie du modèle du lemme.
 */
std::string Lemme::grModele() { return _grModele; }

/**
 * \fn QString Lemme::humain (bool html, QString l)
 * \brief Retourne une chaîne donnant le lemme ramiste avec diacritiques,
 *        ses indications morphologiques et sa traduction dans la langue l.
 *        Si html est true, le retour est au format html.
 */
std::string Lemme::humain(bool html, const std::string &l, bool nbr)
{
    std::string res;
    std::string tr;
    if (!_renvoi.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        tr = (lr != nullptr) ? lr->traduction(l) : "renvoi non trouvé";
    } else {
        tr = traduction(l);
    }

    std::ostringstream flux;
    std::string grq = _grq;
    if (contains(grq, ",")) {
        grq = replaceAll(grq, ",", ", ");
        grq = replaceAll(grq, "  ", " ");
    }

    if (html) {
        if (_nh > 1)
            flux << "<strong>" << grq << "<sup>" << _nh << "</sup></strong>, "
                 << "<em>" << _indMorph << "</em>";
        else
            flux << "<strong>" << grq << "</strong>, "
                 << "<em>" << _indMorph << "</em>";
    } else {
        if (_nh > 1)
            flux << grq << "_" << _nh << ", " << _indMorph;
        else
            flux << grq << ", " << _indMorph;
    }

    if (_nbOcc != 1 && nbr) {
        if (html) flux << " <small>(" << _nbOcc << ")</small>";
        else      flux << " (" << _nbOcc << ")";
    }
    flux << " : " << tr;
    return flux.str();
}

std::string Lemme::indMorph() { return _indMorph; }

/**
 * \fn QString Lemme::irreg (int i, bool *excl)
 * \brief Renvoie la forme irrégulière de morpho i. excl devient
 *        true si elle est exclusive, false sinon.
 */
std::string Lemme::irreg(int i, bool *excl)
{
    for (Irreg *ir : _irregs) {
        auto mv = ir->morphos();
        if (std::find(mv.begin(), mv.end(), i) != mv.end()) {
            *excl = ir->exclusif();
            return ir->grq();
        }
    }
    return "";
}

/**
 * \fn Modele* Lemme::modele ()
 * \brief Renvoie l'objet modèle du lemme.
 */
Modele *Lemme::modele() { return _modele; }

/**
 * \fn int Lemme::nbOcc()
 * \brief Renvoie le nombre d'occurrences du lemme dans les textes du LASLA.
 */
int Lemme::nbOcc() const { return _nbOcc; }

/**
 * @brief Lemme::clearOcc
 * Initialise le nombre d'occurrences.
 */
void Lemme::clearOcc()   { _nbOcc = 1; }

/**
 * \fn int Lemme::nh()
 * \brief Renvoie le numéro d'homonymie du lemme.
 */
int Lemme::nh()          { return _nh; }

/**
 * \fn int Lemme::origin()
 * \brief Renvoie l'origine du lemme : 0 pour le lexique de base, 1 pour l'extension.
 */
int Lemme::origin()      { return _origin; }

/**
 * \fn QString Lemme::oteNh (QString g, int &nh)
 * \brief Supprime le dernier caractère de g si c'est
 *        un nombre et revoie le résultat après avoir
 *        donné la valeur de ce nombre à nh.
 */
std::string Lemme::oteNh(const std::string &g, int &nh)
{
    if (g.empty()) return g;
    char last = g.back();
    if (isdigit((unsigned char)last) && last > '0') {
        nh = last - '0';
        return g.substr(0, g.size() - 1);
    }
    return g;
}

/**
 * \fn QString Lemme::pos ()
 * \brief Renvoie un caractère représentant la
 *        catégorie (part of speech, pars orationis)
 *        du lemme.
 */
std::string Lemme::pos()
{
    if (_pos.empty() && !_renvoi.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        if (lr != nullptr) return lr->pos();
    }
    return _pos;
}

/**
 * \fn QList<Radical*> Lemme::radical (int r)
 * \brief Renvoie le radical numéro r du lemme.
 */
std::vector<Radical *> Lemme::radical(int r)
{
    auto it = _radicaux.find(r);
    return (it != _radicaux.end()) ? it->second : std::vector<Radical *>();
}

/**
 * \fn bool Lemme::renvoi()
 * \brief Renvoie true si le lemme est une forme
 *        alternative renvoyant à une autre entrée
 *        du lexique.
 */
bool Lemme::renvoi() { return contains(_indMorph, "cf. "); }

/**
 * \fn QString Lemme::traduction(QString l)
 * \brief Renvoie la traduction du lemme dans la langue
 *        cible l (2 caractères, éventuellement plus
 *        pour donner l'ordre des langues de secours).
 *        J'ai opté pour un format "l1.l2.l3" où
 *        les trois langues sont en 2 caractères.
 */
std::string Lemme::traduction(const std::string &l)
{
    if (l.size() == 2) {
        if (_traduction.count(l))    return _traduction.at(l);
        if (_traduction.count("fr")) return _traduction.at("fr");
        if (_traduction.count("en")) return _traduction.at("en");
        return "non traduit / Translation not available.";
    }
    auto tryLang = [&](const std::string &ll) -> std::string {
        if (_traduction.count(ll)) return _traduction.at(ll);
        return "";
    };
    std::string r;
    if (l.size() >= 2) r = tryLang(l.substr(0,2));
    if (r.empty() && l.size() >= 5) r = tryLang(l.substr(3,2));
    if (r.empty() && l.size() >= 8) r = tryLang(l.substr(6,2));
    return r.empty() ? "non traduit / Translation not available." : r;
}

/**
 * @brief Lemme::setHyphen
 * @param h : indique où se fait la césure.
 * \brief stocke l'information sur la césure étymologique du lemme
 */
void Lemme::setHyphen(const std::string &h) { _hyphen = h; }

/**
 * @brief Lemme::getHyphen
 * @return la césure étymologique du lemme
 */
std::string Lemme::getHyphen()              { return _hyphen; }
