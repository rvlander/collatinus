/*      lemCore.h
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

#ifndef LEMCORE_H
#define LEMCORE_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include "ch.h"
#include "string_utils.h"

class Irreg;
class Lemme;
class Radical;
class Desinence;
class Modele;

/**
 * @brief structure pour stocker le résultat d'une lemmatisation
 *
 * Cette structure groupe la forme avec quantités, un éventuel suffixe
 * et l'analyse morphologique associée.
 */
struct SLem {
    std::string grq; /*!< la forme avec quantités */
    int morpho;      /*!< l'analyse morphologique (entier) */
    std::string sufq; /*!< l'éventuel suffixe avec quantités */
};

/**
 * @brief Une MapLem regroupe par lemme les résultats d'une lemmatisation
 *
 * Il s'agit d'une map qui associe le pointeur (clef) vers un objet Lemme
 * à une liste (valeur) de SLem qui contiennent chacun un résultat de
 * lemmatisation et d'analyse.
 * C'est donc un moyen simple de regrouper, par lemme,
 * les différentes analyses possibles d'une forme.
 */
typedef std::map<Lemme *, std::vector<SLem>> MapLem;

/**
 * @brief ModLem est une variante de MapLem pour l'identification des formes inconnues.
 *
 * Pour l'identification des formes inconnues, je dois garder
 * le modèle, le supposé radical et la morpho.
 * Je réutilise le SLem pour ces deux derniers et je les
 * ordonne en fonction du modèle.
 * Si je fais cette tentative d'identification sur l'ensemble
 * des formes non-reconnues d'un texte, je peux privilégier
 * les identifications liées au même modèle de formes différentes
 * avec le même supposé radical.
 */
typedef std::map<Modele *, std::vector<SLem>> ModLem;

/**
 * @brief Une Reglep regroupe une expression rationnelle et la chaine de remplacement
 *
 * Une liste de telles Reglep permet de faire des remplacements à la chaine.
 * Utilisée dans LemCore::transfMed
 */
typedef std::pair<std::regex, std::string>    Reglep;

/**
 * @brief La classe LemCore est le noyau de lemmatisation
 *
 * Ce module est le cœur du programme :
 * c'est lui qui va organiser les données et lemmatiser les formes.
 * Il est donc appelé par les modules *intermédiaires*,
 * Lemmatiseur, Scandeur et Tagueur.
 * A priori, c'est plutôt aux classes *intermédiaires*
 * que l'on s'adressera pour ré-utiliser ce code.
 *
 * En lisant les fichiers de données, il va créer les collections
 * d'objets dont il a besoin : Lemme, Modele, Desinence, Radical et Irreg.
 * Les noms de ces classes sont assez explicites.
 * Leur fonctionnement est détaillé dans les pages correspondantes.
 *
 * La fonction importante dans cette classe est surtout
 * LemCore::lemmatiseM qui lemmatise un mot en cherchant
 * les diverses transformations qu'il a pu subir.
 * Elle appelle LemCore::lemmatise qui lemmatise la forme
 * sans transformation.
 */
class LemCore
{
   private:
    // fonction d'initialisation
    void ajAssims();
    void ajAbrev();
    void ajContractions();
    int  aRomano(const std::string &f);
    void lisIrreguliers();
    void lisFichierLexique(const std::string &filepath);
    void lisLexique();
    void lisExtension();
    void lisModeles();
    void lisMorphos(const std::string &lang);
    void lisTraductions(bool base, bool extension);
    void lisTags(bool tout = false);
    void lisTransfMed();
    std::string transfMed(const std::string &f, bool rad = false);
    std::string decontracte(const std::string &d);

    /*! Liste des abréviations, voir LemCore::ajAbr */
    std::vector<std::string> abr;
    /*! Association des préfixes assimilés et non-assimilés sans quantité */
    std::map<std::string, std::string> assims;
    /*! Association des préfixes assimilés et non-assimilés avec quantités */
    std::map<std::string, std::string> assimsq;
    /*! Association des formes contractées et non-contractées */
    std::map<std::string, std::string> _contractions;
    /*! Liste des désinences avec forme (clef) et pointeur (valeur) */
    std::multimap<std::string, Desinence *> _desinences;
    std::multimap<std::string, Irreg *> _irregs;
    /*! Liste des langues cibles en forme abrégée (clef) et longue (valeur) */
    std::map<std::string, std::string> _cibles;
    /*! Liste des lemmes avec forme (clef) et pointeur (valeur) */
    std::map<std::string, Lemme *> _lemmes;
    /*! Liste des modèles avec nom (clef) et pointeur (valeur) */
    std::map<std::string, Modele *> _modeles;
    /*! Liste des analyses morphologiques avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _morphos;
    /*! Liste des cas avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _cas;
    /*! Liste des genres avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _genres;
    /*! Liste des nombres avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _nombres;
    /*! Liste des temps avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _temps;
    /*! Liste des modes avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _modes;
    /*! Liste des voix avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _voix;
    /*! Liste des autres mots-clefs avec langue (clef) et liste lisible (valeur) */
    std::map<std::string, std::vector<std::string>> _motsClefs;
    // Les morphos doivent pouvoir être données en anglais !
    /*! Liste des radicaux avec forme (clef) et pointeur (valeur) */
    std::multimap<std::string, Radical *> _radicaux;
    /*! Liste des méta-variables du fichier modeles.la avec nom (clef) et liste de désinences (valeur) */
    std::map<std::string, std::string> _variables;
    /*! Liste de règles pour transformer les graphies classiques en graphies médiévales */
    std::vector<Reglep> _reglesMed;
    /*! Booléen pour traiter les graphies médiévales */
    bool _medieval;
    /*! Associe une graphie médiévale (clef) à une graphie classique (valeur) pour une désinence */
    std::map<std::string, std::string> _desMed;
    /*! Associe une graphie médiévale (clef) à une graphie classique (valeur) pour un irrégulier */
    std::map<std::string, std::string> _irrMed;
    /*! Associe une graphie médiévale (clef) à une graphie classique (valeur) pour un radical */
    std::multimap<std::string, std::string> _radMed;
    /*! Option indiquant le chargement de l'extension du lexique, voir LemCore::setExtension.*/
    bool _extension;
    /*! La langue choisie, voir LemCore::setCible */
    std::string _cible;
    /*! Nombre d'occurrences du tag dans le corpus du LASLA */
    std::map<std::string, int> _tagOcc;
    /*! Nombre d'occurrences du POS (1er caractère du tag) dans le corpus du LASLA */
    std::map<std::string, int> _tagTot;
    /*! Nombre d'occurrences du trigramme dans le corpus du LASLA */
    std::map<std::string, int> _trigram;
    /*! Le nom du répertoire contenant les données. */
    std::string _resDir;
    /*! Booléen indiquant si l'extension du lexique a été chargée */
    bool _extLoaded;

   public:
    explicit LemCore(const std::string &resDir = "");
    void setResourceDir(const std::string &path);
    bool estAbr(const std::string &m);
    void ajDesinence(Desinence *d);
    void ajRadicaux(Lemme *l);
    std::string assim(const std::string &a);
    std::string assimq(const std::string &a);
    std::map<std::string, std::string> cibles();
    std::string desassim(const std::string &a);
    std::string desassimq(const std::string &a);
    static bool estRomain(const std::string &f);
    bool inv(Lemme *l, const MapLem &ml);
    MapLem lemmatise(const std::string &f);
    ModLem inconnue(const std::string &f);
    MapLem lemmatiseM(const std::string &f, bool debPhr = true, int etape = 0);
    Lemme *lemme(const std::string &l);
    int nbOcc(const std::string &l);
    std::vector<std::string> lemmes(MapLem ml);
    std::vector<std::string> lignesFichier(const std::string &nf);
    Modele *modele(const std::string &m);
    std::string morpho(int m);
    /*! Association des suffixes sans et avec quantités */
    std::map<std::string, std::string> suffixes;
    std::string variable(const std::string &v);
    void lireHyphen(const std::string &fichierHyphen);

    std::string cas(int i);
    std::string genre(int i);
    std::string nombre(int i);
    std::string temps(int i);
    std::string modes(int i);
    std::string voix(int i);
    std::string motsClefs(int i);
    void setCible(const std::string &c);
    std::string cible();
    bool optExtension();
    void setExtension(bool e);
    void setMedieval(bool e);

    std::string tag(const std::string &lp, int m);
    std::string tag(Lemme *l, int m);
    int fraction(const std::string &listTags);
    int tagOcc(const std::string &t);
    int trigram(const std::string &seq);
};

#endif
