/*               lemme.h  */

#ifndef LEMME_H
#define LEMME_H

#include <string>
#include <vector>
#include <map>
#include "ch.h"
#include "string_utils.h"

class Irreg;
class LemCore;
class Lemme;
class Modele;

class Radical
{
   private:
    std::string _gr;
    std::string _grq;
    Lemme *_lemme;
    int _numero;

   public:
    Radical(const std::string &g, int n, Lemme *parent);
    std::string gr();
    std::string grq() const;
    Lemme *lemme();
    Modele *modele();
    int numRad();
};

class Lemme
{
   private:
    static LemCore *_lemCore;
    std::string _cle;
    std::string _gr;
    std::string _grd;
    std::string _grq;
    std::string _grModele;
    std::string _hyphen;
    std::string _indMorph;
    std::vector<Irreg *> _irregs;
    Modele *_modele;
    int _nh;
    std::vector<int> _morphosIrrExcl;
    int _nbOcc;
    int _origin;
    std::string _pos;
    std::map<int, std::vector<Radical *>> _radicaux;
    std::string _renvoi;
    std::map<std::string, std::string> _traduction;

   public:
    Lemme(const std::string &linea, int origin);
    static void setLemCore(LemCore *l);
    void ajIrreg(Irreg *irr);
    void ajNombre(int n);
    void ajRadical(int i, Radical *r);
    void ajTrad(const std::string &t, const std::string &l);
    std::string ambrogio();
    std::string cle();
    std::vector<int> clesR();
    bool estIrregExcl(int nm);
    std::string genre();
    std::string getHyphen();
    std::string gr();
    std::string grq();
    std::string grModele();
    std::string humain(bool html = false, const std::string &l = "fr", bool nbr = false);
    std::string indMorph();
    std::string irreg(int i, bool *excl);
    Modele *modele();
    int nbOcc() const;
    void clearOcc();
    int nh();
    int origin();
    static std::string oteNh(const std::string &g, int &nh);
    std::string pos();
    std::vector<Radical *> radical(int r);
    bool renvoi();
    void setHyphen(const std::string &h);
    std::string traduction(const std::string &l);
    inline bool operator<(const Lemme &l) const { return _nbOcc < l.nbOcc(); }
};

#endif
