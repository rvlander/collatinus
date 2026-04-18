/*      lemCore.h  */

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

struct SLem {
    std::string grq;
    int morpho;
    std::string sufq;
};

typedef std::map<Lemme *, std::vector<SLem>> MapLem;
typedef std::map<Modele *, std::vector<SLem>> ModLem;
typedef std::pair<std::regex, std::string>    Reglep;

class LemCore
{
   private:
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

    std::vector<std::string> abr;
    std::map<std::string, std::string> assims;
    std::map<std::string, std::string> assimsq;
    std::map<std::string, std::string> _contractions;
    std::multimap<std::string, Desinence *> _desinences;
    std::multimap<std::string, Irreg *> _irregs;
    std::map<std::string, std::string> _cibles;
    std::map<std::string, Lemme *> _lemmes;
    std::map<std::string, Modele *> _modeles;
    std::map<std::string, std::vector<std::string>> _morphos;
    std::map<std::string, std::vector<std::string>> _cas;
    std::map<std::string, std::vector<std::string>> _genres;
    std::map<std::string, std::vector<std::string>> _nombres;
    std::map<std::string, std::vector<std::string>> _temps;
    std::map<std::string, std::vector<std::string>> _modes;
    std::map<std::string, std::vector<std::string>> _voix;
    std::map<std::string, std::vector<std::string>> _motsClefs;
    std::multimap<std::string, Radical *> _radicaux;
    std::map<std::string, std::string> _variables;
    std::vector<Reglep> _reglesMed;
    bool _medieval;
    std::map<std::string, std::string> _desMed;
    std::map<std::string, std::string> _irrMed;
    std::multimap<std::string, std::string> _radMed;
    bool _extension;
    std::string _cible;
    std::map<std::string, int> _tagOcc;
    std::map<std::string, int> _tagTot;
    std::map<std::string, int> _trigram;
    std::string _resDir;
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
