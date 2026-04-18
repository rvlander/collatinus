/*           modele.h  */

#ifndef MODELE_H
#define MODELE_H

#include <string>
#include <vector>
#include <map>
#include "ch.h"
#include "string_utils.h"

class LemCore;
class Modele;

class Desinence
{
   private:
    std::string _gr;
    std::string _grq;
    int _morpho;
    Modele *_modele;
    int _numR;
    int _rarete;

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

class Modele
{
   private:
    std::vector<int> _absents;
    static const std::vector<std::string> cles;
    std::multimap<int, Desinence *> _desinences;
    std::map<int, std::string> _genRadicaux;
    std::string _gr;
    LemCore *_lemCore;
    Modele *_pere;
    char _pos;
    std::string _suf;
    int _nbr;

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
